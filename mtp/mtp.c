/*
 * Copyright  Onplick <info@onplick.com> - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 */
#include "usb_device_config.h"
#include "usb.h"
#include "usb_phy.h"
#include "usb_device.h"
#include "usb_device_ch9.h"
#include "usb_device_class.h"
#include "usb_device_mtp.h"

#include "usb_device_descriptor.h"
#include "composite.h"

#include "mtp_responder.h"
#include "mtp_fs.h"

#if DEBUG_MTP
#define PRINTF LOG_DEBUG
#else
#define PRINTF(...)
#endif

/* Number of buffers that fit into input and output stream */
#define CONFIG_RX_STREAM_SIZE (4)
#define CONFIG_TX_STREAM_SIZE (1)
#define CONFIG_MTP_STORAGE_ID (0x00010001)

USB_GLOBAL USB_RAM_ADDRESS_ALIGNMENT(USB_DATA_ALIGN_SIZE) uint8_t rx_buffer[HS_MTP_BULK_IN_PACKET_SIZE];
USB_GLOBAL USB_RAM_ADDRESS_ALIGNMENT(USB_DATA_ALIGN_SIZE) uint8_t tx_buffer[HS_MTP_BULK_OUT_PACKET_SIZE];
USB_GLOBAL USB_RAM_ADDRESS_ALIGNMENT(USB_DATA_ALIGN_SIZE) uint8_t event_response[HS_MTP_INTR_IN_PACKET_SIZE];
USB_GLOBAL USB_RAM_ADDRESS_ALIGNMENT(USB_DATA_ALIGN_SIZE) static uint8_t mtp_request[sizeof(rx_buffer)];
USB_GLOBAL USB_RAM_ADDRESS_ALIGNMENT(USB_DATA_ALIGN_SIZE) static uint8_t mtp_response[sizeof(tx_buffer)];

static mtp_device_info_t dummy_device = {
    .manufacturer = "Mudita",
    .model = "Pure",
    .version = "0.1",
    .serial = "0123456789ABCDEF0123456789ABCDEF",
};

static usb_status_t RescheduleRecv(usb_mtp_struct_t *mtpApp)
{
    // -4 because length of message needs to be stored too
    size_t endpoint_size = mtpApp->usb_buffer_size;
    usb_status_t error = kStatus_USB_Success;
    if (!USB_DeviceClassMtpIsBusy(mtpApp->classHandle, USB_MTP_BULK_OUT_ENDPOINT)
            && !mtpApp->in_reset) {
        size_t available = xMessageBufferSpaceAvailable(mtpApp->inputBox) - 4;
        if (available >= endpoint_size)  {
            error = USB_DeviceClassMtpRecv(mtpApp->classHandle,
                    USB_MTP_BULK_OUT_ENDPOINT,
                    rx_buffer,
                    endpoint_size);
        }
    }
    return error;
}

static size_t SliceToStream(usb_mtp_struct_t *mtpApp, void *buffer, size_t length)
{
    size_t total = 0;
    size_t remaining = length;

    while (remaining > 0) {
        size_t to_send = (remaining < mtpApp->usb_buffer_size) ? remaining : mtpApp->usb_buffer_size;
        size_t buffered = xMessageBufferSend(mtpApp->outputBox, &((uint8_t*)buffer)[total], to_send, 0);
        if (buffered <= 0) {
            break;
        }

        total += buffered;
        remaining -= buffered;
    }
    return total;
}

static usb_status_t USBSend(usb_mtp_struct_t *mtpApp, void *buffer, size_t length)
{
    usb_status_t error = kStatus_USB_Error;
    uint32_t timeout_ms = 1;
    int retries = 30;

    while (--retries
            && !mtpApp->in_reset
            && !mtpApp->is_terminated)
    {
        error = USB_DeviceClassMtpSend(mtpApp->classHandle,
                    USB_MTP_BULK_IN_ENDPOINT, buffer, length);
        if (error == kStatus_USB_Success) {
            break;
        } else if (error == kStatus_USB_Busy) {
            vTaskDelay(timeout_ms / portTICK_PERIOD_MS);
            timeout_ms *= 2;
        }
    }
    return error;
}

static size_t Send(usb_mtp_struct_t *mtpApp, void *buffer, size_t length)
{
    size_t sent = 0;
    if (!mtpApp->configured || !length)
        return kStatus_USB_InvalidParameter;

    taskENTER_CRITICAL();

    PRINTF("[MTP] want to send: %d", (int)length);

    if (xMessageBufferIsEmpty(mtpApp->outputBox)) {

        taskEXIT_CRITICAL();

        size_t send_now = (length < mtpApp->usb_buffer_size) ? length : mtpApp->usb_buffer_size;
        size_t remaining = (length - send_now);
        size_t buffered;

        if (remaining) {
            buffered = SliceToStream(mtpApp, &((uint8_t*)buffer)[mtpApp->usb_buffer_size], remaining);
            sent = send_now + buffered;
        } else {
            sent = send_now;
        }

        memcpy(tx_buffer, buffer, send_now);

        if (USBSend(mtpApp, tx_buffer, send_now) != kStatus_USB_Success) {
            xMessageBufferReset(mtpApp->outputBox);
            PRINTF("[MTP] FATAL: Couldn't send data");
            sent = 0;
        }
    } else {
        // fill buffer up
        taskEXIT_CRITICAL();
        PRINTF("[MTP] TX is busy and we want to queue more data");
    }

    PRINTF("[MTP] accepted to send: %d", (int)sent);
    return sent;
}

static usb_status_t OnConfigurationComplete(usb_mtp_struct_t* mtpApp, void *param)
{
    mtpApp->configured = true;
    PRINTF("[MTP] Configured");
    xSemaphoreGiveFromISR(mtpApp->join, NULL);
    return kStatus_USB_Success;
}

static usb_status_t OnIncomingFrame(usb_mtp_struct_t* mtpApp, void *param)
{
    usb_device_endpoint_callback_message_struct_t *epCbParam = (usb_device_endpoint_callback_message_struct_t*) param;

    if (mtpApp->configured) {
        if (epCbParam->length == 0xFFFFFFFF) {
            PRINTF("[MTP] Rx notification from controller: 0x%x - configured",
                    (unsigned int)epCbParam->length);
        } else if (epCbParam->length > 0) {
            if (xMessageBufferSendFromISR(mtpApp->inputBox, epCbParam->buffer, epCbParam->length, NULL) != epCbParam->length) {
                PRINTF("[MTP] RX dropped incoming bytes: %u",
                        (unsigned int)epCbParam->length);
            }
        } else {
            PRINTF("[MTP] RX Zero length frame");
        }

        RescheduleRecv(mtpApp);
    } else {
        PRINTF("[MTP] Rx notification from controller - not configured");
    }

    return kStatus_USB_Success;
}


static usb_status_t OnOutgoingFrameSent(usb_mtp_struct_t* mtpApp, void *param)
{
    usb_device_endpoint_callback_message_struct_t *epCbParam = (usb_device_endpoint_callback_message_struct_t*) param;

    if (mtpApp->configured) {
        PRINTF("[MTP] already sent");
        size_t length = xMessageBufferReceiveFromISR(mtpApp->outputBox, tx_buffer, sizeof(tx_buffer), NULL);
        PRINTF("[MTP] TX: %d, queued: %d", (int)epCbParam->length, length);
        if (length && USB_DeviceClassMtpSend(mtpApp->classHandle, USB_MTP_BULK_IN_ENDPOINT, tx_buffer, length) != kStatus_USB_Success) {
            PRINTF("[MTP] Dropped outgoing bytes: 0x%d:", (int)length);
            return kStatus_USB_Error;
        }
    } else {
        PRINTF("[MTP] Tx notification from controller - not configured");
    }

    return kStatus_USB_Success;
}

static usb_status_t OnCancelTransaction(usb_mtp_struct_t *mtpApp, void *param)
{
    mtpApp->in_reset = true;
    return kStatus_USB_Success;
}

static usb_status_t OnGetStatus(usb_mtp_struct_t *mtpApp, void *param)
{
    usb_device_control_request_struct_t *request = (usb_device_control_request_struct_t*)param;
    uint16_t status = MTP_RESPONSE_OK;
    size_t event_length = 0;
    if (mtpApp->in_reset || mtp_responder_data_transaction_open(mtpApp->responder)) {
        status = MTP_RESPONSE_DEVICE_BUSY;
    }
    mtp_responder_get_event(mtpApp->responder, status, event_response, &event_length);
    request->buffer = event_response;
    request->length = event_length;
    PRINTF("[MTP] Control Device Status Response: 0x%04x", status);
    return kStatus_USB_Success;
}

usb_status_t MtpUSBCallback(uint32_t event, void *param, void *userArg)
{
    usb_status_t error = kStatus_USB_Error;
    usb_mtp_struct_t* mtpApp = (usb_mtp_struct_t*)userArg;

    switch(event) {
        case kUSB_DeviceMtpEventConfigured:
            error = OnConfigurationComplete(mtpApp, param);
            break;
        case kUSB_DeviceMtpEventSendResponse:
            error = OnOutgoingFrameSent(mtpApp, param);
            break;
        case kUSB_DeviceMtpEventRecvResponse:
            error = OnIncomingFrame(mtpApp, param);
            break;
        case kUSB_DeviceMtpEventCancelTransaction:
            error = OnCancelTransaction(mtpApp, param);
            break;
        case kUSB_DeviceMtpEventRequestDeviceStatus:
            error = OnGetStatus(mtpApp, param);
            break;
        default:
            PRINTF("[MTP] Unknown event from device class driver: %d", (int)event);
    }

    return error;
}

static void send_response(usb_mtp_struct_t *mtpApp, uint16_t status)
{
    usb_status_t send_status;
    size_t result_len = 0;
    mtp_responder_t *responder = mtpApp->responder;

    mtp_responder_get_response(responder, status, mtp_response, &result_len);

    if (!Send(mtpApp, mtp_response, result_len)) {
        PRINTF("[MTP] Transfer failed");
    }
}

static void poll_new_data(usb_mtp_struct_t *mtpApp, size_t *request_len)
{
    do {
        taskENTER_CRITICAL();
        RescheduleRecv(mtpApp);
        taskEXIT_CRITICAL();
        *request_len = xMessageBufferReceive(mtpApp->inputBox, mtp_request, sizeof(mtp_request), 100/portTICK_PERIOD_MS);
    } while(*request_len == 0 && !mtpApp->in_reset);
}

static void MtpTask(void *handle)
{
    usb_mtp_struct_t* mtpApp = (usb_mtp_struct_t*)handle;
    mtp_responder_t* responder;

    if (!(mtpApp->mtp_fs = mtp_fs_alloc(NULL))) {
        PRINTF("[MTP] MTP FS initialization failed!");
        return;
    }

    mtp_responder_init(mtpApp->responder);
    if (mtp_responder_set_device_info(mtpApp->responder, &dummy_device)) {
        PRINTF("[MTP] Invalide device info!");
        return;
    }
    mtp_responder_set_data_buffer(mtpApp->responder, mtp_response, sizeof(mtp_response));

    mtp_responder_set_storage(mtpApp->responder,
            CONFIG_MTP_STORAGE_ID,
            &simple_fs_api,
            mtpApp->mtp_fs);
    responder = mtpApp->responder;

    xSemaphoreTake(mtpApp->join, portMAX_DELAY);

    while(!mtpApp->is_terminated) {

        if (!mtpApp->configured) {
            PRINTF("[MTP] Wait for configuration");
            xSemaphoreTake(mtpApp->join, portMAX_DELAY);
        }

        xMessageBufferReset(mtpApp->inputBox);
        xMessageBufferReset(mtpApp->outputBox);
        mtp_responder_transaction_reset(mtpApp->responder);

        PRINTF("[MTP] Ready");

        mtpApp->in_reset = false;

        while(!mtpApp->in_reset) {
            uint16_t status = 0;
            size_t request_len;
            size_t result_len;

            poll_new_data(mtpApp, &request_len);

            if (request_len == 0) {
                PRINTF("[MTP] Expected MTP message. Reset: %s", mtpApp->in_reset ? "true" : "false");
                continue;
            }

            // Incoming data transaction open:
            if (mtp_responder_data_transaction_open(responder)) {
                status = mtp_responder_set_data(responder, mtp_request, request_len);
                if (status == MTP_RESPONSE_INCOMPLETE_TRANSFER)
                {
                    // This happens with Linux (Nautilus) client. Cancelation procedure
                    // is to just stop sending data in this transaction.
                    // When user triggers another action, client sends GET_OBJECT_INFO
                    // request, which is valid MTP frame and has to be handled.
                    // Don't use timeout here (Windows host can freeze communication
                    // for a while, when assemling file at the end of transacion).
                    PRINTF("[MTP] Incomplete transfer. Expected more data");
                    mtp_responder_transaction_reset(mtpApp->responder);
                } else {
                    if (status == MTP_RESPONSE_OK) {
                        PRINTF("[MTP] Incoming transfer complete");
                        send_response(mtpApp, status);
                    } else if (status == MTP_RESPONSE_OBJECT_TOO_LARGE) {
                        PRINTF("[MTP] Object is too large");
                        send_response(mtpApp, status);
                    }
                    continue;
                }
            }

            status = mtp_responder_handle_request(responder, mtp_request, request_len);

            if (status != MTP_RESPONSE_UNDEFINED) {
                while((result_len = mtp_responder_get_data(responder)) && !mtpApp->in_reset) {
                    usb_status_t send_status;

                    if (!xMessageBufferIsEmpty(mtpApp->inputBox)) {
                        // According to spec, initiator can't issue new transacation, before
                        // current one ends. In this case, assume initiator sends new frame
                        // with cancellation request.
                        PRINTF("[MTP] incoming message during data transfer phase. Abort.");
                        mtp_responder_transaction_reset(mtpApp->responder);
                        status = 0;
                        break;
                    }

                    if (!Send(mtpApp, mtp_response, result_len)) {
                        PRINTF("[MTP] Outgoing data canceled (unable to send)");
                        mtpApp->in_reset = true;
                        break;
                    }
                }

                if (status && !mtpApp->in_reset) {
                    send_response(mtpApp, status);
                }
            }
        }
    }
    mtp_fs_free(mtpApp->mtp_fs);
    xSemaphoreGive(mtpApp->join);
    PRINTF("[MTP] Task terminated");
    vTaskDelete(NULL);
}

usb_status_t MtpInit(usb_mtp_struct_t *mtpApp, class_handle_t classHandle)
{
    mtpApp->configured = 0;
    mtpApp->is_terminated = false;
    mtpApp->classHandle = classHandle;

    if ((mtpApp->join = xSemaphoreCreateBinary())  == NULL) {
        return kStatus_USB_AllocFail;
    }
    xSemaphoreGive(mtpApp->join);

    if ((mtpApp->inputBox = xMessageBufferCreate(CONFIG_RX_STREAM_SIZE*sizeof(rx_buffer))) == NULL) {
        return kStatus_USB_AllocFail;
    }

    /* sizeof(uint32_t) additional bytes to store number of bytes in the stream */
    if ((mtpApp->outputBox = xMessageBufferCreate(sizeof(uint32_t) + CONFIG_TX_STREAM_SIZE*sizeof(tx_buffer))) == NULL) {
        return kStatus_USB_AllocFail;
    }

    if ((mtpApp->responder = mtp_responder_alloc()) == NULL) {
        return kStatus_USB_AllocFail;
    }

    if (xTaskCreate(MtpTask,                  /* pointer to the task */
                    "mtp task",               /* task name for kernel awareness debugging */
                    4096 / sizeof(portSTACK_TYPE), /* task stack size */
                    mtpApp,                   /* optional task startup argument */
                    tskIDLE_PRIORITY,               /* initial priority */
                    NULL             /* optional task handle to create */
                    ) != pdPASS)
    {
        PRINTF("[MTP] Create task failed");
        return kStatus_USB_AllocFail;
    }
   return kStatus_USB_Success;
}

void MtpDeinit(usb_mtp_struct_t *mtpApp)
{
    mtpApp->in_reset = true;
    mtpApp->is_terminated = true;
    /* wait max 2 sec to terminate mtp thread */
    if (xSemaphoreTake(mtpApp->join, 2000/portTICK_PERIOD_MS) == pdTRUE) {
        mtp_responder_free(mtpApp->responder);
        vStreamBufferDelete(mtpApp->outputBox);
        vStreamBufferDelete(mtpApp->inputBox);
        vSemaphoreDelete(mtpApp->join);
        mtpApp->responder = NULL;
        mtpApp->outputBox = NULL;
        mtpApp->outputBox = NULL;
        mtpApp->join = NULL;
        PRINTF("[MTP] Deinitialized");
    } else {
        PRINTF("[MTP] Mtp Deinit failed. Unable to join thread");
    }
}

void MtpReset(usb_mtp_struct_t *mtpApp, uint8_t speed)
{
    mtpApp->configured = false;
    mtpApp->in_reset = true;
    if (speed == USB_SPEED_FULL)
    {
        PRINTF("[MTP] Reset to Full-Speed 12Mbps");
        mtpApp->usb_buffer_size = FS_MTP_BULK_OUT_PACKET_SIZE;
    } else {
        PRINTF("[MTP] Reset to High-Speed 480Mbps");
        mtpApp->usb_buffer_size = HS_MTP_BULK_OUT_PACKET_SIZE;
    }
}

void MtpDetached(usb_mtp_struct_t *mtpApp)
{
    PRINTF("[MTP] MTP detached");
    mtpApp->configured = false;
    mtpApp->in_reset = true;
}
