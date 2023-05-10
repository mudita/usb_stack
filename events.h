// Copyright (c) 2017-2023, Mudita Sp. z.o.o. All rights reserved.
// For licensing, see https://github.com/mudita/MuditaOS/LICENSE.md

#pragma once

typedef enum
{
    USB_EVENT_ATTACHED,
    USB_EVENT_CONFIGURED,
    USB_EVENT_DETACHED,
    USB_EVENT_RESET,
    USB_EVENT_ERROR_MALFORMED_USB_PACKET,
    USB_EVENT_ERROR_MISSED_INCOMING_DATA,
    USB_EVENT_ERROR_RX_BUFFER_OVERFLOW,
    USB_EVENT_ERROR_TX_BUFFER_OVERFLOW,
    USB_EVENT_WARNING_RESCHEDULE_BUSY,
    USB_EVENT_WARNING_NOT_CONFIGURED,
    USB_EVENT_DATA_RECEIVED,
#if (defined(USB_DEVICE_CONFIG_CHARGER_DETECT) && (USB_DEVICE_CONFIG_CHARGER_DETECT > 0U)) &&                          \
    (defined(FSL_FEATURE_SOC_USB_ANALOG_COUNT) && (FSL_FEATURE_SOC_USB_ANALOG_COUNT > 0U))
    USB_EVENT_DCD_FINISHED
#endif
} usb_events_t;

/* @NOTE: function may be called from ISR context. */
typedef void (*usb_event_callback_t)(void *const userArg, const usb_events_t event);
