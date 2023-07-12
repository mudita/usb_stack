/*
 * Copyright  Onplick <info@onplick.com> - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 */
#include <string.h>
#include "usb_string_descriptor.h"
#include "usb.h"
#include "usb_spec.h"
#include "usb_misc.h"

USB_DMA_INIT_DATA_ALIGN(USB_DATA_ALIGN_SIZE) uint8_t USB_StringDescriptorBuffer[128];

/* Sadly, there's no way to create statically initialized array
 * of mutable, variable-length arrays in C: array can only hold
 * objects of the same type, but arrays of different lengths are
 * treated as different types. Thus, all strings need to have
 * predefined maximum length, even though some of them are
 * shorter than others. */
static char usb_strings[][MAX_DESCRIPTOR_STRING_LENGTH + 1] = {
    USB_STRINGS(VALUE)
};

static char *get_usb_string(usb_device_string_id id)
{
    if ((id <= USB_STRING_EMPTY) || (id > USB_STRING_MAX_ID)) {
        return NULL;
    }
    return usb_strings[id - 1];
}

static uint16_t languages_descriptor(uint8_t *ptr)
{
    const uint8_t size = 4;
    *ptr++ = size;
    *ptr++ = USB_DESCRIPTOR_TYPE_STRING;
    *ptr++ = 0x09;
    *ptr++ = 0x04;
    return size;
}

static uint16_t string_descriptor(uint8_t *ptr, const char *ascii)
{
    const uint8_t len = strlen(ascii);
    const uint8_t size = 2 + sizeof(uint16_t) * len;

    *ptr++ = size;
    *ptr++ = USB_DESCRIPTOR_TYPE_STRING;

    uint16_t *utf16 = (uint16_t *)ptr;
    for (size_t i = 0; i < len; i++) {
        *utf16++ = (uint16_t) ascii[i];
    }

    return size;
}

uint16_t USB_GetStringDescriptor(void *buffer, usb_device_string_id id)
{
    uint8_t *ptr = buffer;
    if (id == USB_STRING_EMPTY) {
        return languages_descriptor(ptr);
    }

    const char *usb_string = get_usb_string(id);
    if (usb_string == NULL) {
        return 0;
    }

    return string_descriptor(ptr, usb_string);
}

uint16_t USB_SetDescriptorString(const char *descriptor, usb_device_string_id id)
{
    char *usb_string = get_usb_string(id);
    if (usb_string == NULL) {
        return 0;
    }

    const uint16_t len = MIN(strlen(descriptor), MAX_DESCRIPTOR_STRING_LENGTH);
    memcpy(usb_string, descriptor, len);
    usb_string[len] = '\0';
    return len;
}

const char *USB_GetDescriptorStringPtr(usb_device_string_id id)
{
    return get_usb_string(id);
}
