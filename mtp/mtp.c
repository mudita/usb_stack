/*
 * Copyright  Onplick <info@onplick.com> - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 */
#include "usb.h"
#include "usb_device.h"
#include "usb_device_class.h"
#include "usb_device_mtp.h"

#include "usb_device_descriptor.h"
#include "usb_string_descriptor.h"

#include "composite.h"

#include "mtp_responder.h"
#include "mtp_fs.h"
#include "log.hpp"

/* Number of buffers that fit into input and output stream */
#define CONFIG_RX_STREAM_SIZE (4)
#define CONFIG_TX_STREAM_SIZE (1)
#define CONFIG_MTP_STORAGE_ID (0x00010001)

USB_GLOBAL USB_RAM_ADDRESS_ALIGNMENT(USB_DATA_ALIGN_SIZE)
uint8_t rx_buffer[HS_MTP_BULK_IN_PACKET_SIZE];
USB_GLOBAL USB_RAM_ADDRESS_ALIGNMENT(USB_DATA_ALIGN_SIZE)
uint8_t tx_buffer[HS_MTP_BULK_OUT_PACKET_SIZE];
USB_GLOBAL USB_RAM_ADDRESS_ALIGNMENT(USB_DATA_ALIGN_SIZE)
uint8_t event_response[HS_MTP_INTR_IN_PACKET_SIZE];
USB_GLOBAL USB_RAM_ADDRESS_ALIGNMENT(USB_DATA_ALIGN_SIZE) static uint8_t mtp_request[sizeof(rx_buffer)];
USB_GLOBAL USB_RAM_ADDRESS_ALIGNMENT(USB_DATA_ALIGN_SIZE) static uint8_t mtp_response[sizeof(tx_buffer)];
USB_GLOBAL USB_RAM_ADDRESS_ALIGNMENT(USB_DATA_ALIGN_SIZE) static char mtpRootPath[256];

#define MTP_TASK_STACK_SIZE (3U * 1024U)

const char *DEVICE_STRING_VALUE[] = {USB_STRINGS(VALUE)};
static mtp_device_info_t getDevice()
{
    const int indexOffset    = 1;
    mtp_device_info_t device = {.manufacturer = DEVICE_STRING_VALUE[USB_STRING_MANUFACTURER - indexOffset],
                                .model        = DEVICE_STRING_VALUE[USB_STRING_PRODUCT - indexOffset],
                                .version      = "1",
                                .serial       = DEVICE_STRING_VALUE[USB_STRING_SERIAL_NUMBER - indexOffset]};
    return device;
}

static usb_status_t RescheduleRecv(usb_mtp_struct_t *mtpApp)
{
    // -4 because length of message needs to be stored too
    size_t endpoint_size = mtpApp->usb_buffer_size;
    usb_status_t error   = kStatus_USB_Success;
    if (!USB_DeviceClassMtpIsBusy(mtpApp->classHandle, USB_MTP_BULK_OUT_ENDPOINT) && !mtpApp->in_reset) {
        size_t available = xMessageBufferSpaceAvailable(mtpApp->inputBox) - 4;
        if (available >= endpoint_size) {
            error = USB_DeviceClassMtpRecv(mtpApp->classHandle, USB_MTP_BULK_OUT_ENDPOINT, rx_buffer, endpoint_size);
        }
    }
    return error;
}

static size_t SliceToStream(usb_mtp_struct_t *mtpApp, void *buffer, size_t length)
{
    size_t total     = 0;
    size_t remaining = length;

    while (remaining > 0) {
        size_t to_send  = (remaining < mtpApp->usb_buffer_size) ? remaining : mtpApp->usb_buffer_size;
        size_t buffered = xMessageBufferSend(mtpApp->outputBox, &((uint8_t *)buffer)[total], to_send, 0);
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
    usb_status_t error  = kStatus_USB_Error;
    uint32_t timeout_ms = 1;
    int retries         = 30;

    while (--retries && !mtpApp->in_reset && !mtpApp->is_terminated) {
        error = USB_DeviceClassMtpSend(mtpApp->classHandle, USB_MTP_BULK_IN_ENDPOINT, buffer, length);
        if (error == kStatus_USB_Success) {
            break;
        }
        else if (error == kStatus_USB_Busy) {
            vTaskDelay(timeout_ms / portTICK_PERIOD_MS);
            timeout_ms *= 2;
        }
    }
    return error;
}

static size_t Send(usb_mtp_struct_t *mtpApp, void *buffer, size_t length)
{
    size_t sent = 0;
    if (!mtpApp->configured || !length) {
        return kStatus_USB_InvalidParameter;
    }

    log_debug("[MTP] want to send: %dB", (int)length);

    if (xMessageBufferIsEmpty(mtpApp->outputBox)) {

        size_t send_now  = (length < mtpApp->usb_buffer_size) ? length : mtpApp->usb_buffer_size;
        size_t remaining = (length - send_now);
        size_t buffered;

        if (remaining > 0) {
            buffered = SliceToStream(mtpApp, &((uint8_t *)buffer)[mtpApp->usb_buffer_size], remaining);
            sent     = send_now + buffered;
        }
        else {
            sent = send_now;
        }

        memcpy(tx_buffer, buffer, send_now);

        if (USBSend(mtpApp, tx_buffer, send_now) != kStatus_USB_Success) {
            xMessageBufferReset(mtpApp->outputBox);
            log_debug("[MTP] FATAL: Couldn't send data");
            sent = 0;
        }
    }
    else {
        // fill buffer up
        log_debug("[MTP] TX is busy and we want to queue more data");
    }

    log_debug("[MTP] accepted to send: %dB", (int)sent);
    return sent;
}

static usb_status_t OnConfigurationComplete(usb_mtp_struct_t *mtpApp, void *param)
{
    mtpApp->configured = true;
    xSemaphoreGiveFromISR(mtpApp->configuring, NULL);
    return kStatus_USB_Success;
}

static usb_status_t OnIncomingFrame(usb_mtp_struct_t *mtpApp, void *param)
{
    usb_device_endpoint_callback_message_struct_t *epCbParam = (usb_device_endpoint_callback_message_struct_t *)param;

    if (mtpApp->configured) {
        if (epCbParam->length == 0xFFFFFFFF) {
            log_debug("[MTP] Rx notification from controller: 0x%x - configured", (unsigned int)epCbParam->length);
        }
        else if (epCbParam->length > 0) {
            if (xMessageBufferSendFromISR(mtpApp->inputBox, epCbParam->buffer, epCbParam->length, NULL) !=
                epCbParam->length) {
                log_debug("[MTP] RX dropped incoming bytes: %u", (unsigned int)epCbParam->length);
            }
        }
        else {
            log_debug("[MTP] RX Zero length frame");
        }

        RescheduleRecv(mtpApp);
    }
    else {
        log_debug("[MTP] Rx notification from controller - not configured");
    }

    return kStatus_USB_Success;
}

static usb_status_t OnOutgoingFrameSent(usb_mtp_struct_t *mtpApp, void *param)
{
    if (mtpApp->configured) {
        log_debug("[MTP] already sent");
        if (mtpApp->outputBox == NULL) {
            log_error("[MTP] output stream buffer is NULL!");
            return kStatus_USB_Error;
        }
        size_t length = xMessageBufferReceiveFromISR(mtpApp->outputBox, tx_buffer, sizeof(tx_buffer), NULL);
        if (length && USB_DeviceClassMtpSend(mtpApp->classHandle, USB_MTP_BULK_IN_ENDPOINT, tx_buffer, length) !=
                          kStatus_USB_Success) {
            log_debug("[MTP] Dropped outgoing bytes: 0x%X:", (int)length);
            return kStatus_USB_Error;
        }
    }
    else {
        log_debug("[MTP] Tx notification from controller - not configured");
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
    usb_device_control_request_struct_t *request = (usb_device_control_request_struct_t *)param;
    uint16_t status                              = MTP_RESPONSE_OK;
    size_t event_length                          = 0;
    if (mtpApp->in_reset || mtp_responder_data_transaction_open(mtpApp->responder)) {
        status = MTP_RESPONSE_DEVICE_BUSY;
    }
    mtp_responder_get_event(mtpApp->responder, status, event_response, &event_length);
    request->buffer = event_response;
    request->length = event_length;
    log_debug("[MTP] Control Device Status Response: 0x%04x", status);
    return kStatus_USB_Success;
}

usb_status_t MtpUSBCallback(uint32_t event, void *param, void *userArg)
{
    usb_status_t error       = kStatus_USB_Error;
    usb_mtp_struct_t *mtpApp = (usb_mtp_struct_t *)userArg;

    switch (event) {
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
        log_debug("[MTP] Unknown event from device class driver: %d", (int)event);
    }

    return error;
}

static void send_response(usb_mtp_struct_t *mtpApp, uint16_t status)
{
    size_t result_len          = 0;
    mtp_responder_t *responder = mtpApp->responder;

    mtp_responder_get_response(responder, status, mtp_response, &result_len);

    if (!Send(mtpApp, mtp_response, result_len)) {
        log_debug("[MTP] Transfer failed");
    }
}

static void poll_new_data(usb_mtp_struct_t *mtpApp, size_t *request_len)
{
    do {
        taskENTER_CRITICAL();
        RescheduleRecv(mtpApp);
        taskEXIT_CRITICAL();
        *request_len = xMessageBufferReceive(mtpApp->inputBox, mtp_request, sizeof(mtp_request), pdMS_TO_TICKS(100));
    } while (*request_len == 0 && !mtpApp->in_reset);
}

static void MtpTask(void *handle)
{
    usb_mtp_struct_t *mtpApp = (usb_mtp_struct_t *)handle;
    mtp_responder_t *responder;

    if (!(mtpApp->mtp_fs = mtp_fs_alloc(mtpRootPath))) {
        log_debug("[MTP] MTP FS initialization failed!");
        return;
    }

    mtp_responder_init(mtpApp->responder);
    const mtp_device_info_t device = getDevice();
    if (mtp_responder_set_device_info(mtpApp->responder, &device)) {
        log_debug("[MTP] Invalid device info!");
        mtp_fs_free(mtpApp->mtp_fs);
        mtpApp->mtp_fs = NULL;
        return;
    }
    mtp_responder_set_data_buffer(mtpApp->responder, mtp_response, sizeof(mtp_response));

    mtp_responder_set_storage(mtpApp->responder, CONFIG_MTP_STORAGE_ID, &simple_fs_api, mtpApp->mtp_fs);
    responder = mtpApp->responder;

    while (!mtpApp->is_terminated) {

        if (!mtpApp->configured) {
            log_debug("[MTP] Wait for configuration");
            xSemaphoreTake(mtpApp->configuring, portMAX_DELAY);
            continue;
        }

        xMessageBufferReset(mtpApp->inputBox);
        xMessageBufferReset(mtpApp->outputBox);
        mtp_responder_transaction_reset(mtpApp->responder);

        log_debug("[MTP] Ready");

        mtpApp->in_reset = false;

        while (!mtpApp->in_reset) {
            uint16_t status = 0;
            size_t request_len;
            size_t result_len;

            poll_new_data(mtpApp, &request_len);

            if (request_len == 0) {
                log_debug("[MTP] Expected MTP message. Reset: %s", mtpApp->in_reset ? "true" : "false");
                continue;
            }

            // Incoming data transaction open:
            if (mtp_responder_data_transaction_open(responder)) {
                status = mtp_responder_set_data(responder, mtp_request, request_len);
                if (status == MTP_RESPONSE_INCOMPLETE_TRANSFER) {
                    // This happens with Linux (Nautilus) client. Cancelation procedure
                    // is to just stop sending data in this transaction.
                    // When user triggers another action, client sends GET_OBJECT_INFO
                    // request, which is valid MTP frame and has to be handled.
                    // Don't use timeout here (Windows host can freeze communication
                    // for a while, when assemling file at the end of transacion).
                    log_debug("[MTP] Incomplete transfer. Expected more data");
                    mtp_responder_transaction_reset(mtpApp->responder);
                }
                else {
                    if (status == MTP_RESPONSE_OK) {
                        log_debug("[MTP] Incoming transfer complete");
                        send_response(mtpApp, status);
                    }
                    else if (status == MTP_RESPONSE_OBJECT_TOO_LARGE) {
                        log_debug("[MTP] Object is too large");
                        send_response(mtpApp, status);
                    }
                    continue;
                }
            }

            status = mtp_responder_handle_request(responder, mtp_request, request_len);

            if (status != MTP_RESPONSE_UNDEFINED) {
                while ((result_len = mtp_responder_get_data(responder)) && !mtpApp->in_reset) {

                    if (!xMessageBufferIsEmpty(mtpApp->inputBox)) {
                        // According to spec, initiator can't issue new transacation, before
                        // current one ends. In this case, assume initiator sends new frame
                        // with cancellation request.
                        log_debug("[MTP] incoming message during data transfer phase. Abort.");
                        mtp_responder_transaction_reset(mtpApp->responder);
                        status = 0;
                        break;
                    }

                    if (!Send(mtpApp, mtp_response, result_len)) {
                        log_debug("[MTP] Outgoing data canceled (unable to send)");
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
    mtpApp->mtp_fs = NULL;
    xSemaphoreGive(mtpApp->join);
    log_debug("[MTP] mtp task end");
    vTaskDelete(NULL);
}

usb_status_t MtpInit(usb_mtp_struct_t *mtpApp, class_handle_t classHandle, const char *mtpRoot)
{
    mtpApp->configured    = false;
    mtpApp->is_terminated = false;
    mtpApp->classHandle   = classHandle;

    if ((mtpApp->join = xSemaphoreCreateBinary()) == NULL) {
        return kStatus_USB_AllocFail;
    }

    if ((mtpApp->configuring = xSemaphoreCreateBinary()) == NULL) {
        return kStatus_USB_AllocFail;
    }

    if ((mtpApp->inputBox = xMessageBufferCreate(CONFIG_RX_STREAM_SIZE * sizeof(rx_buffer))) == NULL) {
        return kStatus_USB_AllocFail;
    }

    /* sizeof(uint32_t) additional bytes to store number of bytes in the stream */
    if ((mtpApp->outputBox = xMessageBufferCreate(sizeof(uint32_t) + CONFIG_TX_STREAM_SIZE * sizeof(tx_buffer))) ==
        NULL) {
        return kStatus_USB_AllocFail;
    }

    if ((mtpApp->responder = mtp_responder_alloc()) == NULL) {
        return kStatus_USB_AllocFail;
    }

    strncpy(mtpRootPath, mtpRoot, sizeof(mtpRootPath));

    if (xTaskCreate(MtpTask,                                      /* pointer to the task */
                    "mtp task",                                   /* task name for kernel awareness debugging */
                    MTP_TASK_STACK_SIZE / sizeof(portSTACK_TYPE), /* task stack size */
                    mtpApp,                                       /* optional task startup argument */
                    tskIDLE_PRIORITY,                             /* initial priority */
                    &mtpApp->mtp_task_handle                      /* optional task handle to create */
                    ) != pdPASS) {
        log_debug("[MTP] Create task failed");
        return kStatus_USB_AllocFail;
    }
    return kStatus_USB_Success;
}

void MtpDeinit(usb_mtp_struct_t *mtpApp)
{
    if (!mtpApp->configured) {
        /* If MTP was never attached to the host,
           pretend it's configured and a poke the task */
        mtpApp->configured = true;
        xSemaphoreGive(mtpApp->configuring);
    }

    mtpApp->in_reset      = true;
    mtpApp->is_terminated = true;

    /* wait max 150 msec to terminate MTP task */
    if (!xSemaphoreTake(mtpApp->join, pdMS_TO_TICKS(150))) {
        log_debug("[MTP] Unable to join MTP thread");
    }

    mtp_responder_free(mtpApp->responder);
    vStreamBufferDelete(mtpApp->outputBox);
    vStreamBufferDelete(mtpApp->inputBox);
    vSemaphoreDelete(mtpApp->join);
    vSemaphoreDelete(mtpApp->configuring);
    mtpApp->responder   = NULL;
    mtpApp->outputBox   = NULL;
    mtpApp->outputBox   = NULL;
    mtpApp->join        = NULL;
    mtpApp->configuring = NULL;
    mtpRootPath[0]      = '\0';

    log_debug("[MTP] Deinitialized");
}

void MtpReset(usb_mtp_struct_t *mtpApp, uint8_t speed)
{
    mtpApp->configured = false;
    mtpApp->in_reset   = true;
    if (speed == USB_SPEED_FULL) {
        log_debug("[MTP] Reset to Full-Speed 12Mbps");
        mtpApp->usb_buffer_size = FS_MTP_BULK_OUT_PACKET_SIZE;
    }
    else {
        log_debug("[MTP] Reset to High-Speed 480Mbps");
        mtpApp->usb_buffer_size = HS_MTP_BULK_OUT_PACKET_SIZE;
    }
}

void MtpDetached(usb_mtp_struct_t *mtpApp)
{
    log_debug("[MTP] MTP detached");
    mtpApp->configured = false;
    mtpApp->in_reset   = true;
}
