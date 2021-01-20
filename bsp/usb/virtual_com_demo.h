/*
 * Copyright  Onplick <info@onplick.com> - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 */
#ifndef _VIRTUAL_COM_DEMO_H_
#define _VIRTUAL_COM_DEMO_H_

#include "FreeRTOS.h"
#include "semphr.h"
#include "stream_buffer.h"
#include "message_buffer.h"
#include "event_groups.h"

#include "composite.h"

void VirtualComDemoInit(usb_device_composite_struct_t *usbComposite);

#endif /* _VIRTUAL_COM_DEMO_H_ */
