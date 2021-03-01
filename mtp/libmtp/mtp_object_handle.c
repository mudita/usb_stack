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
static SemaphoreHandle_t s_ObjHandleMutex = NULL;

/* 2-byte unicode, the file created when session is opened is used to save object handle lists. */
USB_RAM_ADDRESS_ALIGNMENT(2U)
static usb_mtp_obj_handle_store_t usb_mtp_obj_handle_store;

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

    usb_mtp_obj_handle_store.maxSize = MTP_OBJ_HANDLE_STORE_INITIAL_SZ;

    free(usb_mtp_obj_handle_store.objHandles);

    usb_mtp_obj_handle_store.objHandles = calloc(MTP_OBJ_HANDLE_STORE_INITIAL_SZ, sizeof(usb_mtp_obj_handle_t));

    if (!usb_mtp_obj_handle_store.objHandles)
    {
        return kStatus_USB_Error;
    }

    return kStatus_USB_Success;
}

usb_status_t USB_DeviceMtpObjHandleDeinit(void)
{
    (void)xSemaphoreTakeRecursive(s_ObjHandleMutex, portMAX_DELAY);

    free(usb_mtp_obj_handle_store.objHandles);

    usb_mtp_obj_handle_store.maxSize = MTP_OBJ_HANDLE_STORE_INITIAL_SZ;

    (void)xSemaphoreGiveRecursive(s_ObjHandleMutex);

    return kStatus_USB_Success;
}

usb_status_t USB_DeviceMtpObjHandleRead(uint32_t objHandle, usb_mtp_obj_handle_t *objHandleStruct)
{
    (void)xSemaphoreTakeRecursive(s_ObjHandleMutex, portMAX_DELAY);

    if (objHandle > usb_mtp_obj_handle_store.maxSize)
    {
        (void)xSemaphoreGiveRecursive(s_ObjHandleMutex);
        return kStatus_USB_Error;
    }

    memcpy(objHandleStruct, &usb_mtp_obj_handle_store.objHandles[(objHandle - 1U)], sizeof(usb_mtp_obj_handle_t));

    (void)xSemaphoreGiveRecursive(s_ObjHandleMutex);

    return kStatus_USB_Success;
}

usb_status_t USB_DeviceMtpObjHandleWrite(uint32_t objHandle, usb_mtp_obj_handle_t *objHandleStruct)
{
    (void)xSemaphoreTakeRecursive(s_ObjHandleMutex, portMAX_DELAY);

    if (objHandle > usb_mtp_obj_handle_store.maxSize) {
        uint32_t tempMaxSize = usb_mtp_obj_handle_store.maxSize * 2;

        void* tempStore = realloc(usb_mtp_obj_handle_store.objHandles, tempMaxSize * sizeof(usb_mtp_obj_handle_t));

        if (!tempStore) {
            (void)xSemaphoreGiveRecursive(s_ObjHandleMutex);
            return kStatus_USB_Error;
        }

        usb_mtp_obj_handle_store.maxSize = tempMaxSize;
        usb_mtp_obj_handle_store.objHandles = tempStore;
    }

    memcpy(&usb_mtp_obj_handle_store.objHandles[(objHandle - 1U)], objHandleStruct, sizeof(usb_mtp_obj_handle_t));

    (void)xSemaphoreGiveRecursive(s_ObjHandleMutex);

    return kStatus_USB_Success;
}
