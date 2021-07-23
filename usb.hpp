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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <module-bsp/bsp/usb/usb.hpp>

namespace bsp
{
    int usbInit(const bsp::usbInitParams &initParams);
    int usbCDCReceive(void *buffer);
    int usbCDCSend(std::string *sendMsg);
    int usbCDCSendRaw(const char *dataPtr, size_t dataLen);
    void usbDeviceTask(void *);
    void usbDeinit();
    void usbReinit(const char *mtpRoot);
    void usbSuspend();

    /* Callback fired on low level events */
    void usbDeviceStateCB(void *, vcomEvent event);
} // namespace bsp
