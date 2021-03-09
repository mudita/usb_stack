/*
 * Copyright 2020 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __USB_MTP_H__
#define __USB_MTP_H__

#ifdef __cplusplus
extern "C" {
#endif
#include "mtp_operation.h"
#include "mtp_file_system_adapter.h"

/* USB MTP config*/
/*buffer size for mtp example. the larger the buffer size ,the faster the data transfer speed is ,*/
/*the block size should be multiple of 512, the least value is 1024*/

#define USB_DEVICE_MTP_TRANSFER_BUFF_SIZE (512 * 9U)

typedef struct _usb_mtp_struct
{
    usb_device_handle deviceHandle;
    class_handle_t mtpHandle;
    TaskHandle_t device_task_handle;
    TaskHandle_t application_task_handle;
    TaskHandle_t device_disk_task_handle;
    QueueHandle_t queueHandle;
    usb_device_mtp_dev_prop_desc_list_t *devPropDescList;
    usb_device_mtp_storage_list_t *storageList;
    usb_device_mtp_obj_prop_list_t *objPropList;
    uint32_t nextHandleID;
    uint8_t *path;
    uint8_t *devFriendlyName;
    uint64_t transferDoneSize;
    uint64_t transferTotalSize;
    usb_device_mtp_file_handle_t file;          /* file handle is used when receiving or sending a file. */
    usb_device_mtp_file_time_stamp_t timeStamp; /* timeStamp is used when receiving a file. */
    uint16_t functionalMode;
    volatile uint8_t mutexUsbToDiskTask;
    uint8_t validObjInfo;
    uint8_t read_write_error;
    uint8_t currentConfiguration;
    uint8_t currentInterfaceAlternateSetting[USB_MTP_INTERFACE_COUNT];
    uint8_t speed;
    uint8_t attach;
} usb_mtp_struct_t;

extern usb_mtp_struct_t g_mtp;

usb_status_t USB_DeviceMtpCallback(class_handle_t handle, uint32_t event, void *param);
usb_status_t USB_DeviceMtpApplicationInit(void* arg);
void MtpReset(usb_mtp_struct_t *mtpApp, uint8_t speed);
void MtpDetached(usb_mtp_struct_t *mtpApp);
void MtpDeinit(usb_mtp_struct_t *mtpApp);

#ifdef __cplusplus
}; //extern "C" {
#endif
#endif
