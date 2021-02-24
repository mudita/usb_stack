/*
 * Copyright  Onplick <info@onplick.com> - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 */
#include "board.h"
#include "usb_device_config.h"
#include "usb.h"
#include "usb_device.h"
#include "usb_device_class.h"
#include "usb_device_cdc_acm.h"
#include "usb_device_ch9.h"
#include "usb_device_descriptor.h"
#include "composite.h"
#include "mtp.h"

#if (defined(USB_DEVICE_CONFIG_CHARGER_DETECT) && (USB_DEVICE_CONFIG_CHARGER_DETECT > 0U)) && \
    (defined(FSL_FEATURE_SOC_USB_ANALOG_COUNT) && (FSL_FEATURE_SOC_USB_ANALOG_COUNT > 0U))
#include "usb_charger_detect.h"
#endif

/* Composite device structure. */
usb_device_composite_struct_t g_composite;


#if (defined(USB_DEVICE_CONFIG_CHARGER_DETECT) && (USB_DEVICE_CONFIG_CHARGER_DETECT > 0U)) && \
    (defined(FSL_FEATURE_SOC_USB_ANALOG_COUNT) && (FSL_FEATURE_SOC_USB_ANALOG_COUNT > 0U))

void USB_UpdateHwTick(void) {
    USB_DeviceUpdateHwTick(g_composite.deviceHandle, g_composite.hwTick++);
}

extern void USB_ChargerDetectedCB(uint8_t detectionState);

static void charger_detected_callback(uint8_t type)
{
    switch (type)
    {
        case kUSB_DcdSDP:
            LOG_DEBUG("SDP detected. Max current is 500mA\r\n");
            break;
        case kUSB_DcdCDP:
            LOG_DEBUG("CDP detected. Max current is 1500mA\r\n");
            break;
        case kUSB_DcdDCP:
            LOG_DEBUG("DCP detected. Max current is 1500mA\r\n");
            break;
        default:
            LOG_DEBUG("Unknown charger type. Max current is 500mA\r\n");
    }

    USB_ChargerDetectedCB(type);
}
#endif

usb_status_t USB_DeviceCallback(usb_device_handle handle, uint32_t event, void *param)
{
    usb_status_t error = kStatus_USB_Error;
    uint16_t *temp16 = (uint16_t *)param;
    uint8_t *temp8 = (uint8_t *)param;

    switch (event)
    {
        case kUSB_DeviceEventBusReset:
        {
            g_composite.attach = 0;
            g_composite.currentConfiguration = 0U;
            error = kStatus_USB_Success;
#if (defined(USB_DEVICE_CONFIG_EHCI) && (USB_DEVICE_CONFIG_EHCI > 0U))
            /* Get USB speed to configure the device, including max packet size and interval of the endpoints. */
            if (kStatus_USB_Success == USB_DeviceClassGetSpeed(CONTROLLER_ID, &g_composite.speed))
            {
                USB_DeviceSetSpeed(handle, g_composite.speed);
            }
#endif
            VirtualComReset(&g_cdcVcom, g_composite.speed);
            MtpReset(&g_mtp, g_composite.speed);
        }
        break;
        case kUSB_DeviceEventSetConfiguration:
            if (0U ==(*temp8))
            {
                g_composite.attach = 0;
                g_composite.currentConfiguration = 0U;
            }
            else if (USB_COMPOSITE_CONFIGURE_INDEX == (*temp8))
            {
                g_composite.attach = 1;
                g_composite.currentConfiguration = *temp8;
                VirtualComUSBSetConfiguration(&g_cdcVcom, *temp8);
                error = kStatus_USB_Success;
            }
            else
            {
                error = kStatus_USB_InvalidRequest;
            }
            break;
        case kUSB_DeviceEventSetInterface:
            if (g_composite.attach)
            {
                uint8_t interface = (uint8_t)((*temp16 & 0xFF00U) >> 0x08U);
                uint8_t alternateSetting = (uint8_t)(*temp16 & 0x00FFU);
                if (interface < USB_INTERFACE_COUNT)
                {
                    g_composite.currentInterfaceAlternateSetting[interface] = alternateSetting;
                    error = kStatus_USB_Success;
                }
            }
            break;
        case kUSB_DeviceEventGetConfiguration:
            if (param)
            {
                *temp8 = g_composite.currentConfiguration;
                error = kStatus_USB_Success;
            }
            break;
        case kUSB_DeviceEventGetInterface:
            if (param)
            {
                uint8_t interface = (uint8_t)((*temp16 & 0xFF00U) >> 0x08U);
                if (interface < USB_INTERFACE_COUNT)
                {
                    *temp16 = (*temp16 & 0xFF00U) | g_composite.currentInterfaceAlternateSetting[interface];
                    error = kStatus_USB_Success;
                }
                else
                {
                    error = kStatus_USB_InvalidRequest;
                }
            }
            break;
        case kUSB_DeviceEventGetDeviceDescriptor:
            if (param)
            {
                error = USB_DeviceGetDeviceDescriptor(handle, (usb_device_get_device_descriptor_struct_t *)param);
            }
            break;
        case kUSB_DeviceEventGetConfigurationDescriptor:
            if (param)
            {
                error = USB_DeviceGetConfigurationDescriptor(handle,
                                                             (usb_device_get_configuration_descriptor_struct_t *)param);
            }
            break;
        case kUSB_DeviceEventGetStringDescriptor:
            if (param)
            {
                error = USB_DeviceGetStringDescriptor(handle, (usb_device_get_string_descriptor_struct_t *)param);
            }
            break;
        #if (defined(USB_DEVICE_CONFIG_CV_TEST) && (USB_DEVICE_CONFIG_CV_TEST > 0U))
        case kUSB_DeviceEventGetDeviceQualifierDescriptor:
            if (param)
            {
                error = USB_DeviceGetDeviceQualifierDescriptor(handle, (usb_device_get_device_qualifier_descriptor_struct_t *)param);
            }
            break;
        #endif

        case kUSB_DeviceEventAttach:
            VirtualComAttached(&composite.cdcVcom);
        break;

        case kUSB_DeviceEventDetach:
            VirtualComDetached(&g_cdcVcom);
            MtpDetached(&g_mtp);

            break;
        #if (defined(USB_DEVICE_CONFIG_CHARGER_DETECT) && (USB_DEVICE_CONFIG_CHARGER_DETECT > 0U)) && \
            (defined(FSL_FEATURE_SOC_USB_ANALOG_COUNT) && (FSL_FEATURE_SOC_USB_ANALOG_COUNT > 0U))
        case kUSB_DeviceEventDcdDetectionfinished:
            charger_detected_callback(*(uint8_t*)param);
            break;
        #endif

        default:
            break;
    }

    return error;
}

usb_device_composite_struct_t* composite_init(userCbFunc callback, void* userArg)
{
    if (USB_DeviceClockInit() != kStatus_USB_Success) {
        LOG_ERROR("[Composite] USB Device Clock init failed");
    }

    composite.speed = USB_SPEED_FULL;
    composite.attach = 0;
    composite.cdcVcom.cdcAcmHandle = (class_handle_t)NULL;
    composite.deviceHandle = NULL;

    PMU->REG_3P0 |= PMU_REG_3P0_ENABLE_ILIMIT(1);
    PMU->REG_3P0 |= PMU_REG_3P0_ENABLE_LINREG(1);

    LOG_DEBUG("VBUS_DETECT: 0x%08x\r\n", (unsigned int)USB_ANALOG->INSTANCE[0].VBUS_DETECT);
    LOG_DEBUG("VBUS_DETECT_STAT: 0x%08x\r\n", (unsigned int)USB_ANALOG->INSTANCE[0].VBUS_DETECT_STAT);

    if (kStatus_USB_Success !=
        USB_DeviceClassInit(CONTROLLER_ID, &g_UsbDeviceCompositeConfigList, &composite.deviceHandle))
    {
        LOG_ERROR("[Composite] USB Device init failed");
        return NULL;
    }
    else
    {
        /* TODO: pass event handling function here */
        if (VirtualComInit(&composite.cdcVcom, g_CompositeClassConfig[1].classHandle, callback, userArg) !=
            kStatus_USB_Success)
            LOG_ERROR("[Composite] VirtualCom initialization failed");

        if (MtpInit(&composite.mtpApp, g_CompositeClassConfig[0].classHandle) != kStatus_USB_Success)
            LOG_ERROR("[Composite] MTP initialization failed");
    }

    USB_DeviceSetIsr(true);

    if (USB_DeviceRun(composite.deviceHandle) != kStatus_USB_Success) {
        LOG_ERROR("[Composite] USB Device run failed");
    }

    LOG_DEBUG("[Composite] USB initialized");
    return &composite;
}

void composite_deinit(usb_device_composite_struct_t *composite)
{
    usb_status_t err;
    if ((err = USB_DeviceStop(composite->deviceHandle)) != kStatus_USB_Success) {
        LOG_ERROR("[Composite] Device stop failed: 0x%x", err);
    }
    USB_DeviceSetIsr(false);
    MtpDeinit(&composite->mtpApp);
    VirtualComDeinit(&composite->cdcVcom);
    if ((err = USB_DeviceClassDeinit(CONTROLLER_ID)) != kStatus_USB_Success) {
        LOG_ERROR("[Composite] Device class deinit failed: 0x%x", err);
    }
    USB_DeviceClockDeinit();
}
