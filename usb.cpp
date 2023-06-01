// Copyright (c) 2017-2023, Mudita Sp. z.o.o. All rights reserved.
// For licensing, see https://github.com/mudita/MuditaOS/LICENSE.md

#include "log.hpp"

#include <module-bsp/bsp/usb/usb.hpp>

extern "C"
{
#include "board.h"
#include "usb_device_config.h"
#include "composite.h"
#include "usb_phy.h"
}

#include <string>

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

        void clearUSBSerialBuffer()
        {
            memset(usbSerialBuffer, 0, sizeof(usbSerialBuffer));
        }

        TimerHandle_t usbTick;
        void usbUpdateTick(TimerHandle_t)
        {
#if (defined(USB_DEVICE_CONFIG_CHARGER_DETECT) && (USB_DEVICE_CONFIG_CHARGER_DETECT > 0U)) &&                          \
    (defined(FSL_FEATURE_SOC_USB_ANALOG_COUNT) && (FSL_FEATURE_SOC_USB_ANALOG_COUNT > 0U))
            /// NXP low-level code calls for the 1ms tick rate, preferably provided by hardware timer. In this case we
            /// are using FreeRTOS software timer configured to 10ms instead. It is still not the best solution, but at
            /// least we won't choke the OS by spamming OS with 1ms timer events.
            USB_UpdateHwTick();
#endif
        }

        void usbDeviceStateCB(void *const /*unused*/, const usb_events_t event)
        {
            USBDeviceStatus notification;
            switch (event) {
            case USB_EVENT_CONFIGURED:
                notification = USBDeviceStatus::Configured;
                break;
            case USB_EVENT_ATTACHED:
                notification = USBDeviceStatus::Connected;
                break;
            case USB_EVENT_DETACHED:
                notification = USBDeviceStatus::Disconnected;
                break;
            case USB_EVENT_RESET:
                notification = USBDeviceStatus::Reset;
                break;
            case USB_EVENT_DATA_RECEIVED:
                notification = USBDeviceStatus::DataReceived;
                break;
            default:
                break;
            }

            if (0U != __get_IPSR()) {
                BaseType_t shouldYield = 0;
                xQueueSendFromISR(USBIrqQueue, &notification, &shouldYield);
                portYIELD_FROM_ISR(shouldYield);
            }
            else {
                xQueueSend(USBIrqQueue, &notification, 500);
            }
        }
    } // namespace

    int usbInit(const bsp::usbInitParams &initParams)
    {
        if (initParams.queueHandle == nullptr or initParams.irqQueueHandle == nullptr) {
            log_error("Invalid argument(s): 0x%p/0x%p", initParams.queueHandle, initParams.irqQueueHandle);
            return -1;
        }

        clearUSBSerialBuffer();
        usbTick = xTimerCreate("usbHWTick", pdMS_TO_TICKS(10), pdTRUE, nullptr, usbUpdateTick);

        USBReceiveQueue = initParams.queueHandle;
        USBIrqQueue     = initParams.irqQueueHandle;

        usbDeviceComposite = composite_init(
                usbDeviceStateCB,
                initParams.serialNumber.c_str(),
                initParams.deviceVersion,
                initParams.rootPath.c_str(),
                initParams.mtpLockedAtInit
        );

        xTimerStart(usbTick, 500);
        return (usbDeviceComposite == NULL) ? -1 : 0;
    }

    void usbDeinit()
    {
        log_debug("usbDeinit");
        xTimerStop(usbTick, 500);
        xTimerDelete(usbTick, 500);

        composite_deinit(usbDeviceComposite);
    }

    void usbUnlockMTP()
    {
        log_debug("mtpUnlock");
        MtpUnlock(&usbDeviceComposite->mtpApp);
    }

    int usbCDCReceive(void *buffer)
    {
        if (usbDeviceComposite->cdcVcom.inputStream == nullptr) {
            return 0;
        }

        memset(buffer, 0, SERIAL_BUFFER_LEN);

        return VirtualComRecv(&usbDeviceComposite->cdcVcom, buffer, SERIAL_BUFFER_LEN);
    }

    void usbHandleDataReceived()
    {
        const std::uint32_t dataReceivedLength = usbCDCReceive(&usbSerialBuffer);
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
                log_debug(
                    "usbDeviceTask echoed: %d signs: [%s]", static_cast<int>(dataReceivedLength), usbSerialBuffer);
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

    int usbCDCSendRaw(const char *dataPtr, size_t dataLen)
    {
        std::uint32_t dataSent = 0;

        do {
            const auto len = VirtualComSend(&usbDeviceComposite->cdcVcom, dataPtr + dataSent, dataLen - dataSent);
            if (len == 0) {
                vTaskDelay(1 / portTICK_PERIOD_MS);
                continue;
            }
            dataSent += len;
        } while (dataSent < dataLen);

        return dataSent;
    }

    int usbCDCSend(std::string *message)
    {
        return usbCDCSendRaw(message->c_str(), message->length());
    }
} // namespace bsp
