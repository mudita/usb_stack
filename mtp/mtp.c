/*
 * Copyright 2020 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>
#include <stdlib.h>
/*${standard_header_anchor}*/
#include "usb_device_config.h"
#include "usb.h"
#include "usb_device.h"

#include "usb_device_class.h"
#include "usb_device_mtp.h"
#include "usb_device_ch9.h"
#include "usb_device_descriptor.h"
// #include "ff.h"
#include "mtp_file_system_adapter.h"
#include "usb_device_mtp.h"
#include "mtp_operation.h"
#include "mtp.h"
// #include "diskio.h"

#include "fsl_device_registers.h"
// #include "fsl_debug_console.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "board.h"

#if (defined(FSL_FEATURE_SOC_SYSMPU_COUNT) && (FSL_FEATURE_SOC_SYSMPU_COUNT > 0U))
#include "fsl_sysmpu.h"
#endif /* FSL_FEATURE_SOC_SYSMPU_COUNT */

#if ((defined FSL_FEATURE_SOC_USBPHY_COUNT) && (FSL_FEATURE_SOC_USBPHY_COUNT > 0U))
#include "usb_phy.h"
#endif

#if (USB_DEVICE_CONFIG_USE_TASK < 1)
// #error This application requires USB_DEVICE_CONFIG_USE_TASK value defined > 0 in usb_device_config.h. Please recompile with this option.
#endif
// #include "sdmmc_config.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/

typedef struct _usb_mtp_manipulate_disk_msgq_struct
{
    usb_device_mtp_cmd_data_struct_t dataInfo;
    uint32_t event;
} usb_mtp_disk_operation_msgq_struct_t;

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
void BOARD_InitHardware(void);
void USB_DeviceClockInit(void);
void USB_DeviceIsrEnable(void);
#if USB_DEVICE_CONFIG_USE_TASK
void USB_DeviceTaskFn(void *deviceHandle);
#if USB_DEVICE_CONFIG_USE_EVENT_TASK
extern void USB_DeviceEventTask(void *arg);
#endif
#endif

/*******************************************************************************
 * Variables
 ******************************************************************************/
// extern sd_card_t g_sd;
const uint16_t g_OpSupported[] = {
    MTP_OPERATION_GET_DEVICE_INFO,
    MTP_OPERATION_OPEN_SESSION,
    MTP_OPERATION_CLOSE_SESSION,
    MTP_OPERATION_GET_STORAGE_IDS,
    MTP_OPERATION_GET_STORAGE_INFO,
    MTP_OPERATION_GET_OBJECT_HANDLES,
    MTP_OPERATION_GET_OBJECT_INFO,
    MTP_OPERATION_GET_OBJECT,
    MTP_OPERATION_DELETE_OBJECT,
    MTP_OPERATION_SEND_OBJECT_INFO,
    MTP_OPERATION_SEND_OBJECT,
    MTP_OPERATION_MOVE_OBJECT,
    MTP_OPERATION_COPY_OBJECT,
    MTP_OPERATION_GET_DEVICE_PROP_DESC,
    MTP_OPERATION_GET_DEVICE_PROP_VALUE,
    MTP_OPERATION_SET_DEVICE_PROP_VALUE,
    MTP_OPERATION_GET_OBJECT_PROPS_SUPPORTED,
    MTP_OPERATION_GET_OBJECT_PROP_DESC,
    MTP_OPERATION_GET_OBJECT_PROP_VALUE,
    MTP_OPERATION_SET_OBJECT_PROP_VALUE,
    MTP_OPERATION_GET_OBJECT_PROP_LIST,
    MTP_OPERATION_GET_OBJECT_REFERENCES,
};

const uint16_t g_EventSupported[] = {
    MTP_EVENT_OBJECT_ADDED,
    MTP_EVENT_OBJECT_REMOVED,
    MTP_EVENT_DEVICE_PROP_CHANGED,
    MTP_EVENT_OBJECT_INFO_CHANGED,
};

const uint16_t g_DevPropSupported[] = {
    MTP_DEVICE_PROPERTY_DEVICE_FRIENDLY_NAME,
};

const uint16_t g_ObjFormatSupported[] = {
    MTP_FORMAT_UNDEFINED,
    MTP_FORMAT_ASSOCIATION,
};

usb_device_mtp_dev_prop_desc_t g_DevPropDesc[] = {
    {
        .devPropCode    = MTP_DEVICE_PROPERTY_DEVICE_FRIENDLY_NAME,
        .dataType       = MTP_TYPE_STR,
        .getSet         = 0x01U, /* Get/Set */
        .defaultVal.str = NULL,
        .currentVal.str = NULL,
        .formFlag       = 0x00U,
    },
};

usb_device_mtp_dev_prop_desc_list_t g_DevPropDescList = {
    .devPropDesc      = &g_DevPropDesc[0],
    .devPropDescCount = sizeof(g_DevPropDesc) / sizeof(g_DevPropDesc[0]),
};

usb_device_mtp_obj_prop_desc_t g_UndefinedOrAssociationObjPropDesc[] = {
    {
        .objPropCode    = MTP_OBJECT_PROPERTY_STORAGE_ID,
        .dataType       = MTP_TYPE_UINT32,
        .getSet         = 0x00U, /* Get */
        .defaultVal.u32 = 0x00U,
        .groupCode      = 0x00U,
        .formFlag       = 0x00U,
    },
    {
        .objPropCode    = MTP_OBJECT_PROPERTY_OBJECT_FORMAT,
        .dataType       = MTP_TYPE_UINT16,
        .getSet         = 0x00U,
        .defaultVal.u16 = 0x00U,
        .groupCode      = 0x00U,
        .formFlag       = 0x00U,
    },
    {
        .objPropCode    = MTP_OBJECT_PROPERTY_PROTECTION_STATUS,
        .dataType       = MTP_TYPE_UINT16,
        .getSet         = 0x00U,
        .defaultVal.u16 = 0x00U,
        .groupCode      = 0x00U,
        .formFlag       = 0x00U,
    },
    {
        .objPropCode    = MTP_OBJECT_PROPERTY_OBJECT_SIZE,
        .dataType       = MTP_TYPE_UINT64,
        .getSet         = 0x00U,
        .defaultVal.u64 = 0x00U,
        .groupCode      = 0x00U,
        .formFlag       = 0x00U,
    },
    {
        .objPropCode    = MTP_OBJECT_PROPERTY_OBJECT_FILE_NAME,
        .dataType       = MTP_TYPE_STR,
        .getSet         = 0x01U, /* Get/Set */
        .defaultVal.str = NULL,
        .groupCode      = 0x00U,
        .formFlag       = 0x00U,
    },
    {
        .objPropCode    = MTP_OBJECT_PROPERTY_DATE_MODIFIED,
        .dataType       = MTP_TYPE_STR,
        .getSet         = 0x00U,
        .defaultVal.u64 = 0x00U,
        .groupCode      = 0x00U,
        .formFlag       = 0x03U, /* DateTime form */
    },
    {
        .objPropCode = MTP_OBJECT_PROPERTY_PERSISTENT_UID,
        .dataType    = MTP_TYPE_UINT128,
        .getSet      = 0x00U,
        .defaultVal.u128 =
            NULL, /* The default value is 0x0...0 for Persistent UID. NULL will be interpret as 0x0...0. */
        .groupCode = 0x00U,
        .formFlag  = 0x00U,
    },
    {
        .objPropCode    = MTP_OBJECT_PROPERTY_PARENT_OBJECT,
        .dataType       = MTP_TYPE_UINT32,
        .getSet         = 0x00U,
        .defaultVal.u32 = 0x00U,
        .groupCode      = 0x00U,
        .formFlag       = 0x00U,
    },
    {
        .objPropCode    = MTP_OBJECT_PROPERTY_NAME,
        .dataType       = MTP_TYPE_STR,
        .getSet         = 0x00U,
        .defaultVal.str = NULL,
        .groupCode      = 0x00U,
        .formFlag       = 0x00U,
    },
    {
        .objPropCode    = MTP_OBJECT_PROPERTY_DISPLAY_NAME,
        .dataType       = MTP_TYPE_STR,
        .getSet         = 0x00U,
        .defaultVal.str = NULL,
        .groupCode      = 0x00U,
        .formFlag       = 0x00U,
    },
    {
        .objPropCode    = MTP_OBJECT_PROPERTY_DATE_ADDED,
        .dataType       = MTP_TYPE_STR,
        .getSet         = 0x00U,
        .defaultVal.str = NULL,
        .groupCode      = 0x00U,
        .formFlag       = 0x03U, /* DateTime form */
    },
};

usb_device_mtp_obj_prop_t g_ObjProp[] = {
    {
        .objFormat = MTP_FORMAT_UNDEFINED,
        .objPropDescCount =
            sizeof(g_UndefinedOrAssociationObjPropDesc) / sizeof(g_UndefinedOrAssociationObjPropDesc[0]),
        .objPropDesc = &g_UndefinedOrAssociationObjPropDesc[0],
    },
    {
        .objFormat = MTP_FORMAT_ASSOCIATION,
        .objPropDescCount =
            sizeof(g_UndefinedOrAssociationObjPropDesc) / sizeof(g_UndefinedOrAssociationObjPropDesc[0]),
        .objPropDesc = &g_UndefinedOrAssociationObjPropDesc[0],
    },
};

usb_device_mtp_obj_prop_list_t g_ObjPropList = {
    .objProp      = &g_ObjProp[0],
    .objPropCount = sizeof(g_ObjProp) / sizeof(g_ObjProp[0]),
};

/* 2-byte unicode */
USB_DMA_INIT_DATA_ALIGN(2U)
const char g_StorageRootPath[MTP_STORAGE_COUNT][MTP_PATH_MAX_LEN >> 1U] = {
    {"/sys/user/music"},
    {"/sys/user/backup"}
};

usb_device_mtp_storage_info_t g_StorageInfo[MTP_STORAGE_COUNT] = {
    {
        .rootPath         = &g_StorageRootPath[0][0], /* 2-byte unicode */
        .storageDesc      = "Music",               /* ascll code, will convert to unicode when host gets this field. */
        .volumeID         = NULL,                  /* ascll code, will convert to unicode when host gets this field. */
        .storageID        = 0x00010001U,           /* should ensure its uniqueness. */
        .storageType      = MTP_STORAGE_FIXED_RAM,
        .fileSystemType   = MTP_STORAGE_FILESYSTEM_GENERIC_HIERARCHICAL,
        .accessCapability = MTP_STORAGE_READ_WRITE,
        .flag             = 0U,
    },
    {
        .rootPath         = &g_StorageRootPath[1][0], /* 2-byte unicode */
        .storageDesc      = "Backup",              /* ascll code, will convert to unicode when host gets this field. */
        .volumeID         = NULL,                  /* ascll code, will convert to unicode when host gets this field. */
        .storageID        = 0x00010002U,           /* should ensure its uniqueness. */
        .storageType      = MTP_STORAGE_FIXED_ROM,
        .fileSystemType   = MTP_STORAGE_FILESYSTEM_GENERIC_FLAT,
        .accessCapability = MTP_STORAGE_READ_ONLY_WITHOUT_DELETE,
        .flag             = 0U,
    }
};

usb_device_mtp_storage_list_t g_StorageList = {
    .storageInfo  = &g_StorageInfo[0],
    .storageCount = sizeof(g_StorageInfo) / sizeof(g_StorageInfo[0]),
};

/* 2-byte unicode, the buffer is used to save device friendly name.
   If the device friendly name length set by host exceeds MTP_DEVICE_FRIENDLY_NAME_LEN, the name will be truncated. */
USB_DMA_INIT_DATA_ALIGN(2U)
uint8_t g_DevFriendlyName[MTP_DEVICE_FRIENDLY_NAME_LEN] = {
    'N', 0x00U, 'X', 0x00U, 'P', 0x00U, ' ', 0x00U, 'M', 0x00U, 'T', 0x00U, 'P', 0x00U, 0x00U, 0x00U,
};

/* Data structure of mtp device, store the information, such as class handle */
usb_mtp_struct_t g_mtp;

/* The buffer is used to build path, please make sure the buffer have enough space to accommodate the longest path.
   If the path length exceeds MTP_PATH_MAX_LEN, the current transaction will end with a failure. */
USB_DMA_NONINIT_DATA_ALIGN(2U) uint16_t g_pathBuffer[MTP_PATH_MAX_LEN >> 1U];
USB_DMA_NONINIT_DATA_ALIGN(USB_DATA_ALIGN_SIZE) uint32_t g_mtpTransferBuffer[USB_DEVICE_MTP_TRANSFER_BUFF_SIZE >> 2];
/*******************************************************************************
 * Code
 ******************************************************************************/


/*!
 * @brief device mtp callback function.
 *
 * This function handle the disk class specified event.
 * @param handle          The USB class  handle.
 * @param event           The USB device event type.
 * @param param           The parameter of the class specific event.
 * @return kStatus_USB_Success or error.
 */
usb_status_t USB_DeviceMtpCallback(class_handle_t handle, uint32_t event, void *param)
{
    usb_status_t error = kStatus_USB_Success;

    switch (event)
    {
        case kUSB_DeviceMtpEventOpenSession:
        case kUSB_DeviceMtpEventCloseSession:
        case kUSB_DeviceMtpEventGetDeviceInfo:
        case kUSB_DeviceMtpEventGetDevicePropDesc:
        case kUSB_DeviceMtpEventGetObjPropsSupported:
        case kUSB_DeviceMtpEventGetStorageIDs:
        case kUSB_DeviceMtpEventGetStorageInfo:
        case kUSB_DeviceMtpEventGetObjHandles:
        case kUSB_DeviceMtpEventGetObjPropDesc:
        case kUSB_DeviceMtpEventGetObjPropList:
        case kUSB_DeviceMtpEventGetObjInfo:
        case kUSB_DeviceMtpEventGetObj:
        case kUSB_DeviceMtpEventSendObjInfo:
        case kUSB_DeviceMtpEventSendObj:
        case kUSB_DeviceMtpEventGetDevicePropVal:
        case kUSB_DeviceMtpEventSetDevicePropVal:
        case kUSB_DeviceMtpEventGetObjPropVal:
        case kUSB_DeviceMtpEventSetObjPropVal:
        case kUSB_DeviceMtpEventGetObjReferences:
        case kUSB_DeviceMtpEventMoveObj:
            if (g_mtp.mutexUsbToDiskTask == 1U)
            {
                error = kStatus_USB_Error;
            }
            break;

        default:
            /* no action */
            break;
    }

    if (error != kStatus_USB_Success)
    {
        return error;
    }

    switch (event)
    {
        case kUSB_DeviceMtpEventDeviceResetRequest:
            /* Receiving class specific reset request, the device clears its command buffer,
               closes all open sessions, and returns to the configured State. */
            USB_DeviceCmdCloseSession(param);
            break;

        case kUSB_DeviceMtpEventGetExtendedEventData:
            break;

        case kUSB_DeviceMtpEventOpenSession:
            USB_DeviceCmdOpenSession(param);
            break;

        case kUSB_DeviceMtpEventCloseSession:
            USB_DeviceCmdCloseSession(param);
            break;

        case kUSB_DeviceMtpEventGetDeviceInfo:
        {
            usb_device_mtp_device_info_t deviceInfo;

            deviceInfo.functionalMode         = MTP_FUNCTIONAL_MODE_STANDARD_MODE;
            deviceInfo.mtpExtendsions         = NULL;
            deviceInfo.opSupported            = &g_OpSupported[0];
            deviceInfo.opSupportedLength      = sizeof(g_OpSupported);
            deviceInfo.eventSupported         = &g_EventSupported[0];
            deviceInfo.eventSupportedLength   = sizeof(g_EventSupported);
            deviceInfo.devPropSupported       = &g_DevPropSupported[0];
            deviceInfo.devPropSupportedLength = sizeof(g_DevPropSupported);
            deviceInfo.captureFormat          = NULL;
            deviceInfo.captureFormatLength    = 0;
            deviceInfo.playbackFormat         = &g_ObjFormatSupported[0];
            deviceInfo.playbackFormatLength   = sizeof(g_ObjFormatSupported);
            deviceInfo.manufacturer           = "Mudita";
            deviceInfo.model                  = "Pure";
            deviceInfo.deviceVersion          = "1.0";
            deviceInfo.serialNumber           = "0123456789ABCDEF";

            USB_DeviceCmdGetDeviceInfo(param, &deviceInfo);
            break;
        }

        case kUSB_DeviceMtpEventGetDevicePropDesc:
            USB_DeviceCmdGetDevicePropDesc(param);
            break;

        case kUSB_DeviceMtpEventGetObjPropsSupported:
            USB_DeviceCmdGetObjPropsSupported(param);
            break;

        case kUSB_DeviceMtpEventGetStorageIDs:
            USB_DeviceCmdGetStorageIDs(param);
            break;

        case kUSB_DeviceMtpEventGetStorageInfo:
            USB_DeviceCmdGetStorageInfo(param);
            break;

        case kUSB_DeviceMtpEventGetObjHandles:
            USB_DeviceCmdGetObjHandles(param);
            break;

        case kUSB_DeviceMtpEventGetObjPropDesc:
            USB_DeviceCmdGetObjPropDesc(param);
            break;

        case kUSB_DeviceMtpEventGetObjPropList:
            USB_DeviceCmdGetObjPropList(param);
            break;

        case kUSB_DeviceMtpEventGetObjInfo:
            USB_DeviceCmdGetObjInfo(param);
            break;

        case kUSB_DeviceMtpEventGetObj:
            USB_DeviceCmdGetObj(param);
            break;

        case kUSB_DeviceMtpEventSendObjInfo:
            USB_DeviceCmdSendObjInfo(param);
            break;

        case kUSB_DeviceMtpEventSendObj:
            USB_DeviceCmdSendObj(param);
            break;

        case kUSB_DeviceMtpEventGetDevicePropVal:
            USB_DeviceCmdGetDevicePropVal(param);
            break;

        case kUSB_DeviceMtpEventSetDevicePropVal:
            USB_DeviceCmdSetDevicePropVal(param);
            break;

        case kUSB_DeviceMtpEventGetObjPropVal:
            USB_DeviceCmdGetObjPropVal(param);
            break;

        case kUSB_DeviceMtpEventSetObjPropVal:
            USB_DeviceCmdSetObjPropVal(param);
            break;

        case kUSB_DeviceMtpEventGetObjReferences:
            USB_DeviceCmdGetObjReferences(param);
            break;

        case kUSB_DeviceMtpEventMoveObj:
            USB_DeviceCmdMoveObj(param);
            break;

        case kUSB_DeviceMtpEventCopyObj:
        case kUSB_DeviceMtpEventDeleteObj:
        {
            usb_mtp_disk_operation_msgq_struct_t msgQ;

            memcpy(&msgQ.dataInfo, param, sizeof(usb_device_mtp_cmd_data_struct_t));
            msgQ.event = event;

            xQueueSendToBack(g_mtp.queueHandle, &msgQ, 0);
            break;
        }

        default:
            break;
    }

    return error;
}

/*!
 * @brief device callback function.
 *
 * This function handle the usb standard event. more information, please refer to usb spec chapter 9.
 * @param handle          The USB device handle.
 * @param event           The USB device event type.
 * @param param           The parameter of the device specific request.
 * @return  A USB error code or kStatus_USB_Success..
 */
// usb_status_t USB_DeviceCallback(usb_device_handle handle, uint32_t event, void *param)
// {
//     usb_status_t error = kStatus_USB_Error;
//     uint16_t *temp16   = (uint16_t *)param;
//     uint8_t *temp8     = (uint8_t *)param;
//     switch (event)
//     {
//         case kUSB_DeviceEventBusReset:
//         {
//             g_mtp.attach               = 0;
//             g_mtp.currentConfiguration = 0U;
//             error                      = kStatus_USB_Success;
// #if (defined(USB_DEVICE_CONFIG_EHCI) && (USB_DEVICE_CONFIG_EHCI > 0U))
//             /* Get USB speed to configure the device, including max packet size and interval of the endpoints. */
//             if (kStatus_USB_Success == USB_DeviceClassGetSpeed(CONTROLLER_ID, &g_mtp.speed))
//             {
//                 USB_DeviceSetSpeed(handle, g_mtp.speed);
//             }
// #endif
//         }
//         break;
//
// #if (defined(USB_DEVICE_CONFIG_DETACH_ENABLE) && (USB_DEVICE_CONFIG_DETACH_ENABLE > 0U))
//         case kUSB_DeviceEventDetach:
//             USB_DeviceMtpCancelCurrentTransaction(g_mtp.mtpHandle);
//             if (0U == g_mtp.mutexUsbToDiskTask)
//             {
//                 USB_DeviceCmdCloseSession(NULL);
//             }
//             break;
// #endif
//
//         case kUSB_DeviceEventSetConfiguration:
//             if (0U == (*temp8))
//             {
//                 g_mtp.attach               = 0;
//                 g_mtp.currentConfiguration = 0U;
//             }
//             else if (USB_MTP_CONFIGURE_INDEX == (*temp8))
//             {
//                 g_mtp.attach               = 1;
//                 g_mtp.currentConfiguration = *temp8;
//             }
//             else
//             {
//                 error = kStatus_USB_InvalidRequest;
//             }
//             break;
//         case kUSB_DeviceEventSetInterface:
//             if (g_mtp.attach)
//             {
//                 uint8_t interface        = (uint8_t)((*temp16 & 0xFF00U) >> 0x08U);
//                 uint8_t alternateSetting = (uint8_t)(*temp16 & 0x00FFU);
//                 if (interface < USB_MTP_INTERFACE_COUNT)
//                 {
//                     g_mtp.currentInterfaceAlternateSetting[interface] = alternateSetting;
//                 }
//             }
//             break;
//         case kUSB_DeviceEventGetConfiguration:
//             if (param)
//             {
//                 *temp8 = g_mtp.currentConfiguration;
//                 error  = kStatus_USB_Success;
//             }
//             break;
//         case kUSB_DeviceEventGetInterface:
//             if (param)
//             {
//                 uint8_t interface = (uint8_t)((*temp16 & 0xFF00U) >> 0x08U);
//                 if (interface < USB_INTERFACE_COUNT)
//                 {
//                     *temp16 = (*temp16 & 0xFF00U) | g_mtp.currentInterfaceAlternateSetting[interface];
//                     error   = kStatus_USB_Success;
//                 }
//                 else
//                 {
//                     error = kStatus_USB_InvalidRequest;
//                 }
//             }
//             break;
//         case kUSB_DeviceEventGetDeviceDescriptor:
//             if (param)
//             {
//                 error = USB_DeviceGetDeviceDescriptor(handle, (usb_device_get_device_descriptor_struct_t *)param);
//             }
//             break;
//         case kUSB_DeviceEventGetConfigurationDescriptor:
//             if (param)
//             {
//                 error = USB_DeviceGetConfigurationDescriptor(handle,
//                                                              (usb_device_get_configuration_descriptor_struct_t *)param);
//             }
//             break;
// #if (defined(USB_DEVICE_CONFIG_CV_TEST) && (USB_DEVICE_CONFIG_CV_TEST > 0U))
//         case kUSB_DeviceEventGetDeviceQualifierDescriptor:
//             if (param)
//             {
//                 /* Get Qualifier descriptor request */
//                 error = USB_DeviceGetDeviceQualifierDescriptor(
//                     handle, (usb_device_get_device_qualifier_descriptor_struct_t *)param);
//             }
//             break;
// #endif
//         case kUSB_DeviceEventGetStringDescriptor:
//             if (param)
//             {
//                 error = USB_DeviceGetStringDescriptor(handle, (usb_device_get_string_descriptor_struct_t *)param);
//             }
//             break;
//         default:
//             break;
//     }
//     return error;
// }

void USB_DeviceDiskOperationTask(void *arg)
{
    usb_mtp_disk_operation_msgq_struct_t msgQ;
    usb_device_mtp_response_struct_t response;

    while (1)
    {
        if (pdTRUE == xQueueReceive(g_mtp.queueHandle, &msgQ, portMAX_DELAY))
        {
            g_mtp.mutexUsbToDiskTask = 1U;
            switch (msgQ.event)
            {
                case kUSB_DeviceMtpEventDeleteObj:
                    USB_DeviceCmdDeleteObj(&msgQ.dataInfo);
                    break;

                case kUSB_DeviceMtpEventCopyObj:
                    USB_DeviceCmdCopyObj(&msgQ.dataInfo);
                    break;

                default:
                    /* no action */
                    break;
            }
            g_mtp.mutexUsbToDiskTask = 0U;

            if ((pdTRUE == xQueuePeek(g_mtp.queueHandle, &msgQ, 0)) &&
                ((msgQ.dataInfo.curPhase == USB_DEVICE_MTP_PHASE_CANCELLATION) ||
                 (msgQ.event == kUSB_DeviceMtpEventDeviceResetRequest)))
            {
                /* If receiving cancellation or reset request during the above opertions, do not send reponse */
            }
            else
            {
                response.code        = msgQ.dataInfo.code;
                response.paramNumber = msgQ.dataInfo.curSize;
                while (msgQ.dataInfo.curSize != 0U)
                {
                    msgQ.dataInfo.curSize--;
                    ((uint32_t *)&response.param1)[msgQ.dataInfo.curSize] = msgQ.dataInfo.param[msgQ.dataInfo.curSize];
                }

                USB_DeviceMtpResponseSend(g_mtp.mtpHandle, &response);
            }
        }
    }
}


/*!
 * @brief device application init function.
 *
 * This function init the usb stack and sdhc driver.
 *
 * @return None.
 */
usb_status_t USB_DeviceMtpApplicationInit(void* arg)
{
    g_mtp.devPropDescList = &g_DevPropDescList;
    g_mtp.storageList     = &g_StorageList;
    g_mtp.objPropList     = &g_ObjPropList;
    g_mtp.devFriendlyName = &g_DevFriendlyName[0];
    g_mtp.path            = (uint8_t *)&g_pathBuffer[0];

    g_mtp.mtpHandle          = (class_handle_t)arg;
    g_mtp.mutexUsbToDiskTask = 0U;

    g_mtp.queueHandle = xQueueCreate(1U, sizeof(usb_mtp_disk_operation_msgq_struct_t));
    if (NULL == g_mtp.queueHandle)
    {
        usb_echo("Queue create failed\r\n");
        return kStatus_USB_Error;
    }

    if (kStatus_USB_Success != USB_DeviceMtpFSInit((const uint16_t *)g_mtp.storageList->storageInfo[0].rootPath))
    {
        usb_echo("Disk init failed\r\n");
        return kStatus_USB_Error;
    }

    if (xTaskCreate(USB_DeviceDiskOperationTask,    /* pointer to the task */
                    (char const *)"usb MTP task",  /* task name for kernel awareness debugging */
                    6000L / sizeof(portSTACK_TYPE), /* task stack size */
                    NULL,                           /* optional task startup argument */
                    tskIDLE_PRIORITY,                              /* initial priority */
                    &g_mtp.device_disk_task_handle  /* optional task handle to create */
                    ) != pdPASS)
    {
        usb_echo("usb device MTP task create failed!\r\n");
        return kStatus_USB_Error;
    }

    return kStatus_USB_Success;
}


void MtpReset(usb_mtp_struct_t *mtpApp, uint8_t speed)
{
    // mtpApp->configured = false;
    // mtpApp->in_reset = true;
    if (speed == USB_SPEED_FULL)
    {
        usb_echo("[MTP] Reset to Full-Speed 12Mbps");
        // mtpApp->usb_buffer_size = FS_MTP_BULK_OUT_PACKET_SIZE;
    } else {
        usb_echo("[MTP] Reset to High-Speed 480Mbps");
        // mtpApp->usb_buffer_size = HS_MTP_BULK_OUT_PACKET_SIZE;
    }
}

void MtpDetached(usb_mtp_struct_t *mtpApp)
{
    usb_echo("[MTP] MTP detached");
    // mtpApp->configured = false;
    // mtpApp->in_reset = true;
}

void MtpDeinit(usb_mtp_struct_t *mtpApp)
{
    // mtpApp->in_reset = true;
    // mtpApp->is_terminated = true;
    /* wait max 2 sec to terminate mtp thread */
//     if (xSemaphoreTake(mtpApp->join, 2000/portTICK_PERIOD_MS) == pdTRUE) {
    usb_echo("[MTP] Deinitialized");
//     } else {
//         PRINTF("[MTP] Mtp Deinit failed. Unable to join thread");
//     }
}

