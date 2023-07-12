/*
 * Copyright  Onplick <info@onplick.com> - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 */
#ifndef _USB_STRING_DESCRIPTOR_H
#define _USB_STRING_DESCRIPTOR_H

#include <stdint.h>

/* Macros for build USB string database in "usb_strings.h" file,
 * that is placed somewhere in application layer. Header doesn't
 * need to have guard defines as it's only included here.
 *
 * Example usage (content of usb_string.h):
 *
 *  #define USB_STRINGS(entry) \
        entry(MANUFACTURER, "Fancy manufacturer"), \
        entry(DESCRIPTION, "Fancy device model")
 *
 * For enum, there is prefix added: USB_STRING_.
 */
#include "usb_strings.h"

#define MAX_DESCRIPTOR_STRING_LENGTH 36

#define ID(id, value) USB_STRING_##id
#define VALUE(id, value) value

typedef enum {
    USB_STRING_EMPTY = 0,
    USB_STRINGS(ID),
    USB_STRING_MAX_ID
} usb_device_string_id;

/*
 * @brief Returns USB string descriptor for id
 * @param buffer for descriptor's binary data
 * @param id of string descriptor to be read
 * @return length of descriptor's binary data or 0 for non-existing string
 */
uint16_t USB_GetStringDescriptor(void *buffer, usb_device_string_id id);

/*
 * @brief Sets new value of descriptor string
 * @param data to be set as descriptor string
 * @param id of string to be set
 * @return length of descriptor's binary data or 0 for non-existing string
 */
uint16_t USB_SetDescriptorString(const char *descriptor, usb_device_string_id id);

/*
 * @brief Returns pointer to descriptor string
 * @param id of string to be read
 * @return pointer to descriptor string
 */
const char *USB_GetDescriptorStringPtr(usb_device_string_id id);

extern uint8_t USB_StringDescriptorBuffer[];
#endif /*_USB_STRING_DESCRIPTOR_H */

