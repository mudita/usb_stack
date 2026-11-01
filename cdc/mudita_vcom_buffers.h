// Copyright (c) 2017-2026, Mudita Sp. z.o.o. All rights reserved.
// For licensing, see https://github.com/mudita/MuditaOS/LICENSE.md

/*
 * Context-neutral wrappers for stream buffer access used from VCOM
 * class endpoint callbacks. With USB_DEVICE_CONFIG_USE_TASK==0 those
 * callbacks execute in the EHCI ISR context, with USE_TASK==1 they
 * execute in the dedicated USB device task. The wrappers route the
 * call to the correct FreeRTOS API (FromISR vs. plain) at compile time.
 */
#ifndef _MUDITA_VCOM_BUFFERS_H_
#define _MUDITA_VCOM_BUFFERS_H_

#include "FreeRTOS.h"
#include "stream_buffer.h"
#include "portmacro.h"
#include "task.h"
#include "usb_device_config.h"

static inline size_t mudita_vcom_stream_send(StreamBufferHandle_t stream,
                                             const void *data,
                                             size_t length)
{
#if (defined(USB_DEVICE_CONFIG_USE_TASK) && (USB_DEVICE_CONFIG_USE_TASK > 0U))
    /* Task context - do not wait, this is invoked from the USB device
     * task and any blocking would stall further endpoint processing. */
    return xStreamBufferSend(stream, data, length, 0);
#else
    BaseType_t higherPriorityTaskWoken = pdFALSE;
    const size_t bytes = xStreamBufferSendFromISR(stream, data, length, &higherPriorityTaskWoken);
    portYIELD_FROM_ISR(higherPriorityTaskWoken);
    return bytes;
#endif
}

static inline size_t mudita_vcom_stream_recv(StreamBufferHandle_t stream,
                                             void *data,
                                             size_t length)
{
#if (defined(USB_DEVICE_CONFIG_USE_TASK) && (USB_DEVICE_CONFIG_USE_TASK > 0U))
    return xStreamBufferReceive(stream, data, length, 0);
#else
    BaseType_t higherPriorityTaskWoken = pdFALSE;
    const size_t bytes = xStreamBufferReceiveFromISR(stream, data, length, &higherPriorityTaskWoken);
    portYIELD_FROM_ISR(higherPriorityTaskWoken);
    return bytes;
#endif
}

#endif /* _MUDITA_VCOM_BUFFERS_H_ */
