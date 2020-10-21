#pragma once

extern "C"
{
#include "FreeRTOS.h"
#include "task.h"
#include <queue.h>
}

#include <errno.h>
#include <iostream>
#include <log/log.hpp>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

namespace bsp
{
    int usbInit(xQueueHandle);
    void usbCDCReceive(void *ptr);
    int usbCDCSend(std::string *sendMsg);
} // namespace bsp
