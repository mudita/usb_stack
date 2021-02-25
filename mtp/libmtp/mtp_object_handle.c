/*
 * Copyright 2020 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "usb.h"
#include "stdio.h"
#include "mtp_file_system_adapter.h"
#include "mtp_object_handle.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/
static FILE* s_File;
static SemaphoreHandle_t s_ObjHandleMutex = NULL;

/* 2-byte unicode, the file created when session is opened is used to save object handle lists. */
USB_RAM_ADDRESS_ALIGNMENT(2U)
const char g_ObjHandlePath[] = "/sys/user/mtp_obj.txt";

/*******************************************************************************
 * Code
 ******************************************************************************/

usb_status_t USB_DeviceMtpObjHandleInit(void)
{
    if (NULL != s_ObjHandleMutex)
    {
        (void)vSemaphoreDelete(s_ObjHandleMutex);
    }

    s_ObjHandleMutex = xSemaphoreCreateRecursiveMutex();

    if (NULL == s_ObjHandleMutex)
    {
        return kStatus_USB_Error;
    }

    (void)fclose(s_File);

    s_File = fopen(&g_ObjHandlePath[0], "w+");

    if (!s_File)
    {
        return kStatus_USB_Error;
    }

    return kStatus_USB_Success;
}

usb_status_t USB_DeviceMtpObjHandleDeinit(void)
{
    (void)xSemaphoreTakeRecursive(s_ObjHandleMutex, portMAX_DELAY);

    (void)fclose(s_File);

    (void)xSemaphoreGiveRecursive(s_ObjHandleMutex);

    return kStatus_USB_Success;
}

usb_status_t USB_DeviceMtpObjHandleRead(uint32_t objHandle, usb_mtp_obj_handle_t *objHandleStruct)
{
    uint32_t result;
    uint32_t size;

    (void)xSemaphoreTakeRecursive(s_ObjHandleMutex, portMAX_DELAY);

    result = fseek(s_File, (objHandle - 1U) * sizeof(usb_mtp_obj_handle_t), SEEK_SET);

    if (!result)
    {
        size = fread(objHandleStruct, sizeof(usb_mtp_obj_handle_t), 1, s_File);
    }

    (void)xSemaphoreGiveRecursive(s_ObjHandleMutex);

    if ((result) || (size < sizeof(usb_mtp_obj_handle_t)))
    {
        return kStatus_USB_Error;
    }

    return kStatus_USB_Success;
}

usb_status_t USB_DeviceMtpObjHandleWrite(uint32_t objHandle, usb_mtp_obj_handle_t *objHandleStruct)
{
    uint32_t result;
    uint32_t size;

    (void)xSemaphoreTakeRecursive(s_ObjHandleMutex, portMAX_DELAY);

    result = fseek(s_File, (objHandle - 1U) * sizeof(usb_mtp_obj_handle_t), SEEK_SET);

    if (!result)
    {
        size = fwrite(objHandleStruct, sizeof(usb_mtp_obj_handle_t), 1, s_File);
    }

    (void)xSemaphoreGiveRecursive(s_ObjHandleMutex);

    if ((result) || (size < sizeof(usb_mtp_obj_handle_t)))
    {
        return kStatus_USB_Error;
    }

    return kStatus_USB_Success;
}
