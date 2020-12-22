#pragma once

extern "C"
{
#include "FreeRTOS.h"
#include "task.h"
#include "usb.h"
#include "usb_device.h"
#include "usb_device_class.h"
#include "usb_device_cdc_acm.h"
#include "usb_device_ch9.h"
#include "usb_device_descriptor.h"
#include "composite.h"
#include <queue.h>
}

#include <errno.h>
#include <iostream>
#include <log/log.hpp>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <module-bsp/bsp/usb/usb.hpp>


namespace bsp
{
    int usbInit(xQueueHandle queueHandle, USBDeviceListener *deviceListener);
    int usbCDCReceive(void *buffer);
    int usbCDCSend(std::string *sendMsg);
    int usbCDCSendRaw(const char *dataPtr, size_t dataLen);
    void usbDeviceTask(void *);
    void usbDeinit();
} // namespace bsp
