/*
 * Copyright  Onplick <info@onplick.com> - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 */
#include <mutex.hpp>
#include "usb.hpp"

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

#ifndef DEUBG_USB
    #undef LOG_PRINTF
    #undef LOG_TRACE
    #undef LOG_DEBUG
    #undef LOG_INFO
    #undef LOG_WARN
    #undef LOG_ERROR
    #undef LOG_FATAL
    #undef LOG_CUSTOM

    #define LOG_PRINTF(...)
    #define LOG_TRACE(...)
    #define LOG_DEBUG(...)
    #define LOG_INFO(...)
    #define LOG_WARN(...)
    #define LOG_ERROR(...)
    #define LOG_FATAL(...)
    #define LOG_CUSTOM(loggerLevel, ...)
#endif
namespace bsp
{
    usb_device_composite_struct_t *usbDeviceComposite = nullptr;
    TaskHandle_t usbTaskHandle = NULL;
    xQueueHandle USBReceiveQueue;
    xQueueHandle USBIrqQueue;
    static cpp_freertos::MutexStandard mutex;

    char usbSerialBuffer[SERIAL_BUFFER_LEN];

#if USBCDC_ECHO_ENABLED
    bool usbCdcEchoEnabled = false;

    constexpr std::string_view usbCDCEchoOnCmd("UsbCdcEcho=ON");
    constexpr std::string_view usbCDCEchoOffCmd("UsbCdcEcho=OFF");

    constexpr inline auto usbCDCEchoOnCmdLength  = usbCDCEchoOnCmd.length();
    constexpr inline auto usbCDCEchoOffCmdLength = usbCDCEchoOffCmd.length();
#endif

    int usbInit(const bsp::usbInitParams &initParams)
    {
        if (!(initParams.queueHandle &&
              initParams.irqQueueHandle &&
              initParams.deviceListener &&
              initParams.serialNumber)) {
            LOG_ERROR("Invalid argument(s): 0x%p/0x%p/0x%p/0x%p",
                initParams.queueHandle,
                initParams.irqQueueHandle,
                initParams.deviceListener,
                initParams.serialNumber);
            return -1;
        }

        BaseType_t xReturned = xTaskCreate(reinterpret_cast<TaskFunction_t>(&bsp::usbDeviceTask),
                                           "bsp::usbDeviceTask",
                                           8192L / sizeof(portSTACK_TYPE),
                                           initParams.deviceListener, 2,
                                           &bsp::usbTaskHandle);

        if (xReturned == pdPASS) {
            LOG_DEBUG("init created device task");
        } else {
            LOG_DEBUG("init can't create device task");
            return -1;
        }

        USBReceiveQueue                = initParams.queueHandle;
        USBIrqQueue                    = initParams.irqQueueHandle;

        usbDeviceComposite = composite_init(usbDeviceStateCB, (void*)initParams.serialNumber);

        return (usbDeviceComposite == NULL) ? -1 : 0;
    }

    void usbDeinit()
    {
        LOG_INFO("usbDeinit");
        composite_deinit(usbDeviceComposite);
        vTaskDelete(bsp::usbTaskHandle);
    }

    void usbReinit(const char *mtpRoot)
    {
        LOG_INFO("usbReinit");
        if (!mtpRoot || (mtpRoot[0] == '\0'))
        {
           LOG_ERROR("Attempted USB reinit with empty MTP path");
           return;
        }
        composite_reinit(usbDeviceComposite, mtpRoot);
    }

    void usbSuspend()
    {
    	LOG_INFO("usbSuspend");
    	composite_suspend(usbDeviceComposite);
    }

    void usbDeviceTask(void *handle)
    {
        USBDeviceListener *deviceListener = static_cast<USBDeviceListener *>(handle);
        uint32_t dataReceivedLength;

        vTaskDelay(3000 / portTICK_PERIOD_MS);

        while (1) {
            #if (defined(USB_DEVICE_CONFIG_CHARGER_DETECT) && (USB_DEVICE_CONFIG_CHARGER_DETECT > 0U)) && \
                (defined(FSL_FEATURE_SOC_USB_ANALOG_COUNT) && (FSL_FEATURE_SOC_USB_ANALOG_COUNT > 0U))
            // This is an ugly stub for a timer.
            USB_UpdateHwTick();
            #endif

            dataReceivedLength = usbCDCReceive(&usbSerialBuffer);

            if (dataReceivedLength > 0) {
                LOG_INFO("usbDeviceTask Received: %d signs", static_cast<int>(dataReceivedLength));

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
                    LOG_DEBUG("usbDeviceTask echoed: %d signs: [%s]", static_cast<int>(dataReceivedLength), usbSerialBuffer);
                    continue;
                }
#endif

                if (deviceListener->getRawMode()) {
                    cpp_freertos::LockGuard lock(mutex);
                    deviceListener->rawDataReceived(&usbSerialBuffer, dataReceivedLength);
                }
                else if (uxQueueSpacesAvailable(USBReceiveQueue) != 0) {
                    std::string *receiveMessage = new std::string(usbSerialBuffer, dataReceivedLength);
                    if (xQueueSend(USBReceiveQueue, &receiveMessage, portMAX_DELAY) == errQUEUE_FULL) {
                        LOG_ERROR("usbDeviceTask can't send data [%s] to receiveQueue", receiveMessage->c_str());
                    }
                }
            }
            else {
                vTaskDelay(1 / portTICK_PERIOD_MS);
            }
        }
    }

    int usbCDCReceive(void *buffer)
    {
        if (usbDeviceComposite->cdcVcom.inputStream == nullptr)
            return 0;

        memset(buffer, 0, SERIAL_BUFFER_LEN);

        return VirtualComRecv(&usbDeviceComposite->cdcVcom, buffer, SERIAL_BUFFER_LEN);
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
            notification = USBDeviceStatus::Connected;
            xQueueSend(USBIrqQueue, &notification, 0);
            break;
        case VCOM_DETACHED:
            notification = USBDeviceStatus::Disconnected;
            xQueueSend(USBIrqQueue, &notification, 0);
            break;
        default:
            break;
        }
    }
} // namespace bsp
