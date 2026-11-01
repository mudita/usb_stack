// Copyright (c) 2017-2026, Mudita Sp. z.o.o. All rights reserved.
// For licensing, see https://github.com/mudita/MuditaOS/LICENSE.md

/*
 * Context-neutral wrappers for FreeRTOS primitives invoked from MTP
 * USB class callbacks. With USB_DEVICE_CONFIG_USE_TASK==0 the callbacks
 * execute in EHCI ISR context, with USE_TASK==1 they run in the
 * dedicated USB device task. The wrappers select the correct API at
 * compile time so callsites stay free of #ifdef noise.
 */
#ifndef _MUDITA_MTP_BUFFERS_H_
#define _MUDITA_MTP_BUFFERS_H_

#include "FreeRTOS.h"
#include "message_buffer.h"
#include "semphr.h"
#include "portmacro.h"
#include "task.h"
#include "usb_device_config.h"

static inline size_t mudita_mtp_msg_send(MessageBufferHandle_t mb,
                                         const void *data,
                                         size_t length)
{
#if (defined(USB_DEVICE_CONFIG_USE_TASK) && (USB_DEVICE_CONFIG_USE_TASK > 0U))
    return xMessageBufferSend(mb, data, length, 0);
#else
    BaseType_t higherPriorityTaskWoken = pdFALSE;
    const size_t bytes = xMessageBufferSendFromISR(mb, data, length, &higherPriorityTaskWoken);
    portYIELD_FROM_ISR(higherPriorityTaskWoken);
    return bytes;
#endif
}

static inline size_t mudita_mtp_msg_recv(MessageBufferHandle_t mb,
                                         void *data,
                                         size_t length)
{
#if (defined(USB_DEVICE_CONFIG_USE_TASK) && (USB_DEVICE_CONFIG_USE_TASK > 0U))
    return xMessageBufferReceive(mb, data, length, 0);
#else
    BaseType_t higherPriorityTaskWoken = pdFALSE;
    const size_t bytes = xMessageBufferReceiveFromISR(mb, data, length, &higherPriorityTaskWoken);
    portYIELD_FROM_ISR(higherPriorityTaskWoken);
    return bytes;
#endif
}

static inline BaseType_t mudita_mtp_sem_give(SemaphoreHandle_t sem)
{
#if (defined(USB_DEVICE_CONFIG_USE_TASK) && (USB_DEVICE_CONFIG_USE_TASK > 0U))
    return xSemaphoreGive(sem);
#else
    BaseType_t higherPriorityTaskWoken = pdFALSE;
    const BaseType_t r = xSemaphoreGiveFromISR(sem, &higherPriorityTaskWoken);
    portYIELD_FROM_ISR(higherPriorityTaskWoken);
    return r;
#endif
}

#endif /* _MUDITA_MTP_BUFFERS_H_ */
