/*
 * Copyright  Onplick <info@onplick.com> - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 */
#ifndef _USB_DEVICE_COMPOSITE_H_
#define _USB_DEVICE_COMPOSITE_H_

#include "FreeRTOS.h"
#include "semphr.h"
#include "stream_buffer.h"
#include "message_buffer.h"
#include "event_groups.h"
#include "events.h"
#include "usb_device_config.h"
#include "MIMXRT1051_features.h"
#include "virtual_com.h"
#if defined(USB_DEVICE_CONFIG_MTP) && (USB_DEVICE_CONFIG_MTP > 0U)
#include "mtp.h"
#endif

#define CONTROLLER_ID                 kUSB_ControllerEhci0
#define USB_DEVICE_INTERRUPT_PRIORITY (3U)

typedef struct _usb_device_composite_struct
{
    bool initialized;
    usb_device_handle deviceHandle;  /* USB device handle. */
#if defined(USB_DEVICE_CONFIG_USE_TASK) && (USB_DEVICE_CONFIG_USE_TASK > 0U)
    TaskHandle_t device_task_handle; /* USB device task handle */
#endif
    usb_cdc_vcom_struct_t cdcVcom;   /* CDC virtual com device structure. */
#if defined(USB_DEVICE_CONFIG_MTP) && (USB_DEVICE_CONFIG_MTP > 0U)
    usb_mtp_struct_t mtpApp;
#endif
    uint8_t speed;  /* Speed of USB device. USB_SPEED_FULL/USB_SPEED_LOW/USB_SPEED_HIGH.                 */
    uint8_t attach; /* A flag to indicate whether a usb device is attached. 1: attached, 0: not attached */
    uint8_t currentConfiguration;                              /* Current configuration value. */
    uint8_t
        currentInterfaceAlternateSetting[USB_INTERFACE_COUNT]; /* Current alternate setting value for each interface. */
    usb_event_callback_t userDefinedEventCallback;
    void *userDefinedEventCallbackArg;
} usb_device_composite_struct_t;

usb_device_composite_struct_t *composite_init(usb_event_callback_t userEventCallback,
                                              const char *serialNumber,
                                              const uint16_t bcdDeviceVersion,
                                              const char *mtpRoot);
void composite_deinit(usb_device_composite_struct_t *composite);

#if (defined(USB_DEVICE_CONFIG_CHARGER_DETECT) && (USB_DEVICE_CONFIG_CHARGER_DETECT > 0U)) &&                          \
    (defined(FSL_FEATURE_SOC_USB_ANALOG_COUNT) && (FSL_FEATURE_SOC_USB_ANALOG_COUNT > 0U))
void USB_UpdateHwTick(void);
#endif
#endif /* _USB_DEVICE_COMPOSITE_H_ */
