/*
 * Copyright  Onplick <info@onplick.com> - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 */
#include "usb.hpp"
#include "log.hpp"

extern "C"
{
#include "board.h"
#include "usb_device_config.h"
#include "usb.h"
#include "usb_device.h"
#include "usb_device_class.h"
#include "usb_device_cdc_acm.h"
#include "usb_device_ch9.h"
#include "usb_device_descriptor.h"
#include "composite.h"
#include "usb_phy.h"
}

namespace bsp
{
    namespace
    {
        usb_device_composite_struct_t *usbDeviceComposite = nullptr;
        xQueueHandle USBReceiveQueue;
        xQueueHandle USBIrqQueue;

        char usbSerialBuffer[SERIAL_BUFFER_LEN];

#if USBCDC_ECHO_ENABLED
        bool usbCdcEchoEnabled = false;

        constexpr std::string_view usbCDCEchoOnCmd("UsbCdcEcho=ON");
        constexpr std::string_view usbCDCEchoOffCmd("UsbCdcEcho=OFF");

        constexpr inline auto usbCDCEchoOnCmdLength  = usbCDCEchoOnCmd.length();
        constexpr inline auto usbCDCEchoOffCmdLength = usbCDCEchoOffCmd.length();
#endif

        TimerHandle_t usbTick;
        void usbUpdateTick(TimerHandle_t)
        {
            #if (defined(USB_DEVICE_CONFIG_CHARGER_DETECT) && (USB_DEVICE_CONFIG_CHARGER_DETECT > 0U)) && \
                (defined(FSL_FEATURE_SOC_USB_ANALOG_COUNT) && (FSL_FEATURE_SOC_USB_ANALOG_COUNT > 0U))
            // This is an ugly stub for a timer.
            USB_UpdateHwTick();
            #endif
        }
    }

    int usbInit(const bsp::usbInitParams &initParams)
    {
        if (!(initParams.queueHandle &&
              initParams.irqQueueHandle &&
              initParams.serialNumber)) {
            log_error("Invalid argument(s): 0x%p/0x%p/0x%p",
                initParams.queueHandle,
                initParams.irqQueueHandle,
                initParams.serialNumber);
            return -1;
        }

        usbTick = xTimerCreate(
            "usbHWTick", pdMS_TO_TICKS(1), true, nullptr, usbUpdateTick);

        USBReceiveQueue                = initParams.queueHandle;
        USBIrqQueue                    = initParams.irqQueueHandle;

        usbDeviceComposite = composite_init(usbDeviceStateCB, (void*)initParams.serialNumber);

        return (usbDeviceComposite == NULL) ? -1 : 0;
    }

    void usbDeinit()
    {
    	log_debug("usbDeinit");
        // Restart HW tick for resume operation
        if (xTimerStart(usbTick, 0) == pdPASS ) {
			// Resume if suspended
			composite_resume(usbDeviceComposite);

			if (xTimerStop(usbTick, 0) != pdPASS){
				log_error("The usbTick timer could not be stopped");
			}
        }
        else {
        	log_error("The usbTick timer could not be started");
        }

        composite_deinit(usbDeviceComposite);
    }

    void usbReinit(const char *mtpRoot)
    {
        log_debug("usbReinit");
        if (!mtpRoot || (mtpRoot[0] == '\0'))
        {
           log_error("Attempted USB reinit with empty MTP path");
           return;
        }
        composite_reinit(usbDeviceComposite, mtpRoot);
    }

    void usbSuspend()
    {
    	log_debug("usbSuspend");
    	composite_suspend(usbDeviceComposite);
    }

    int usbCDCReceive(void *buffer)
    {
        if (usbDeviceComposite->cdcVcom.inputStream == nullptr)
            return 0;

        memset(buffer, 0, SERIAL_BUFFER_LEN);
        
        return VirtualComRecv(&usbDeviceComposite->cdcVcom, buffer, SERIAL_BUFFER_LEN);
    }

    void usbHandleDataReceived()
    {
        uint32_t dataReceivedLength = usbCDCReceive(&usbSerialBuffer);

        if (dataReceivedLength > 0) {
            log_debug("usbDeviceTask Received: %d signs", static_cast<int>(dataReceivedLength));

#if USBCDC_ECHO_ENABLED
            bool usbCdcEchoEnabledPrev = usbCdcEchoEnabled;

            auto usbEchoCmd = std::string_view{usbSerialBuffer, static_cast<size_t>(dataReceivedLength)};

            if ((dataReceivedLength == usbCDCEchoOnCmdLength) && (usbCDCEchoOnCmd == usbEchoCmd)) {
                usbCdcEchoEnabled = true;
            }
            else if ((dataReceivedLength == usbCDCEchoOffCmdLength) && (usbCDCEchoOffCmd == usbEchoCmd)) {
                usbCdcEchoEnabled = false;
            }

            if (usbCdcEchoEnabled || usbCdcEchoEnabledPrev) {
                usbCDCSendRaw(usbSerialBuffer, dataReceivedLength);
                log_debug("usbDeviceTask echoed: %d signs: [%s]", static_cast<int>(dataReceivedLength), usbSerialBuffer);
                continue;
            }
#endif

            if (uxQueueSpacesAvailable(USBReceiveQueue) != 0) {
                std::string *receiveMessage = new std::string(usbSerialBuffer, dataReceivedLength);
                if (xQueueSend(USBReceiveQueue, &receiveMessage, portMAX_DELAY) == errQUEUE_FULL) {
                    log_error("usbDeviceTask can't send data to receiveQueue");
                }
            }
        }
    }

    int usbCDCSend(std::string *message)
    {
        return usbCDCSendRaw(message->c_str(), message->length());
    }

    int usbCDCSendRaw(const char *dataPtr, size_t dataLen)
    {
        uint32_t dataSent = 0;

        do {
            uint32_t len =  VirtualComSend(&usbDeviceComposite->cdcVcom, dataPtr + dataSent, dataLen - dataSent);
            if (!len) {
                vTaskDelay(1 / portTICK_PERIOD_MS);
                continue;
            }
            dataSent += len;
        } while(dataSent < dataLen);

        return dataSent;
    }

    void usbDeviceStateCB(void *, vcomEvent event)
    {
        USBDeviceStatus notification;
        switch (event) {
        case VCOM_CONFIGURED:
            notification = USBDeviceStatus::Configured;
            xQueueSend(USBIrqQueue, &notification, 0);
            break;
        case VCOM_ATTACHED:
            xTimerStart(usbTick, 1000);
            notification = USBDeviceStatus::Connected;
            xQueueSend(USBIrqQueue, &notification, 0);
            break;
        case VCOM_DETACHED:
            xTimerStop(usbTick, 1000);
            notification = USBDeviceStatus::Disconnected;
            xQueueSend(USBIrqQueue, &notification, 0);
            break;
        case VCOM_DATA_RECEIVED:
            notification = USBDeviceStatus::DataReceived;
            xQueueSend(USBIrqQueue, &notification, 0);
            break;   
        default:
            break;
        }
    }
} // namespace bsp
