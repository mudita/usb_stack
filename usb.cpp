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

namespace bsp
{
    usb_device_composite_struct_t *usbDeviceComposite = nullptr;
    TaskHandle_t usbTaskHandle = NULL;
    xQueueHandle USBReceiveQueue;
    static cpp_freertos::MutexStandard mutex;

    char usbSerialBuffer[SERIAL_BUFFER_LEN];

    int usbInit(xQueueHandle queueHandle, USBDeviceListener *deviceListener)
    {
        BaseType_t xReturned = xTaskCreate(reinterpret_cast<TaskFunction_t>(&bsp::usbDeviceTask),
                                           "bsp::usbDeviceTask",
                                           2048L / sizeof(portSTACK_TYPE),
                                           deviceListener, 2,
                                           &bsp::usbTaskHandle);

        if (xReturned == pdPASS) {
            LOG_DEBUG("init created device task");
        } else {
            LOG_DEBUG("init can't create device task");
            return -1;
        }

        USBReceiveQueue = queueHandle;
        usbDeviceComposite= composite_init();

        return (usbDeviceComposite == NULL) ? -1 : 0;
    }

    void usbDeviceTask(void *handle)
    {
        USBDeviceListener *deviceListener = static_cast<USBDeviceListener *>(handle);
        uint32_t dataReceivedLength;

        vTaskDelay(3000 / portTICK_PERIOD_MS);

        while (1) {
            dataReceivedLength = usbCDCReceive(&usbSerialBuffer);

            if (dataReceivedLength > 0) {

                if (deviceListener->getRawMode()) {
                    cpp_freertos::LockGuard lock(mutex);
                    deviceListener->rawDataReceived(&usbSerialBuffer, dataReceivedLength);
                }
                else if (uxQueueSpacesAvailable(USBReceiveQueue) != 0) {
                    static std::string receiveMessage;
                    receiveMessage = std::string(usbSerialBuffer, dataReceivedLength);
                    if (xQueueSend(USBReceiveQueue, &receiveMessage, portMAX_DELAY) == errQUEUE_FULL) {
                        LOG_ERROR("deviceTask can't send data [%s] to receiveQueue", receiveMessage.c_str());
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
        return VirtualComRecv(&usbDeviceComposite->cdcVcom, buffer, SERIAL_BUFFER_LEN);
    }

    int usbCDCSend(std::string *message)
    {
        uint32_t dataSent = 0;
        const char *dataPtr = (*message).c_str();
        do {
            uint32_t len =  VirtualComSend(&usbDeviceComposite->cdcVcom, dataPtr + dataSent, message->length() - dataSent);
            if (!len) {
                vTaskDelay(1 / portTICK_PERIOD_MS);
                continue;
            }
            dataSent += len;
        } while(dataSent < message->length());

        return dataSent;
    }
}
