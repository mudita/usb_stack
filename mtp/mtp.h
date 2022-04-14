/*
 * Copyright  Onplick <info@onplick.com> - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 */
#ifndef _MTP_H_
#define _MTP_H_

#include "mtp_responder.h"
#include "mtp_fs.h"

// refactor name to mtp_app_struct_t
typedef struct {
    class_handle_t classHandle;
    mtp_responder_t *responder;
    struct mtp_fs *mtp_fs;

    uint8_t configured;
    uint8_t in_reset;
    uint8_t is_terminated;
    size_t usb_buffer_size;
    MessageBufferHandle_t inputBox;
    MessageBufferHandle_t outputBox;
    SemaphoreHandle_t join;
    SemaphoreHandle_t configuring;
    TaskHandle_t mtp_task_handle; /* USB MTP task handle */
} usb_mtp_struct_t;

usb_status_t MtpUSBCallback(uint32_t event, void *param, void *userArg);
usb_status_t MtpInit(usb_mtp_struct_t *mtpApp, class_handle_t classHandle);
void MtpReset(usb_mtp_struct_t *mtpApp, uint8_t speed);
void MtpDeinit(usb_mtp_struct_t *mtpApp);
usb_status_t MtpReinit(usb_mtp_struct_t *mtpApp, class_handle_t classHandle, const char *mtpRoot);
void MtpDetached(usb_mtp_struct_t *mtpApp);

#endif /* _MTP_H_ */
