/*
 * Copyright  Onplick <info@onplick.com> - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 */

#include "bsp/usb/usb.hpp"

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

    usb_cdc_vcom_struct_t *usbInit(xQueueHandle)
    {
        return composite_init();
    }

    void usbCDCReceive(void *ptr)
    {
    }

    int usbCDCSend(std::string *sendMsg)
    {
        return -1;
    }

}


