/*
 * Copyright (c) 2015 - 2016, Freescale Semiconductor, Inc.
 * Copyright 2016 - 2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _USB_DEVICE_DESCRIPTOR_H_
#define _USB_DEVICE_DESCRIPTOR_H_ 1

/*******************************************************************************
* Definitions
******************************************************************************/
#define USB_DEVICE_SPECIFIC_BCD_VERSION (0x0200)
#define USB_DEVICE_DEMO_BCD_VERSION (0x0101U)

/* Communication Class SubClass Codes */
#define USB_CDC_DIRECT_LINE_CONTROL_MODEL (0x01)
#define USB_CDC_ABSTRACT_CONTROL_MODEL (0x02)
#define USB_CDC_TELEPHONE_CONTROL_MODEL (0x03)
#define USB_CDC_MULTI_CHANNEL_CONTROL_MODEL (0x04)
#define USB_CDC_CAPI_CONTROL_MOPDEL (0x05)
#define USB_CDC_ETHERNET_NETWORKING_CONTROL_MODEL (0x06)
#define USB_CDC_ATM_NETWORKING_CONTROL_MODEL (0x07)
#define USB_CDC_WIRELESS_HANDSET_CONTROL_MODEL (0x08)
#define USB_CDC_DEVICE_MANAGEMENT (0x09)
#define USB_CDC_MOBILE_DIRECT_LINE_MODEL (0x0A)
#define USB_CDC_OBEX (0x0B)
#define USB_CDC_ETHERNET_EMULATION_MODEL (0x0C)

/* Communication Class Protocol Codes */
#define USB_CDC_NO_CLASS_SPECIFIC_PROTOCOL (0x00) /*also for Data Class Protocol Code */
#define USB_CDC_AT_250_PROTOCOL (0x01)
#define USB_CDC_AT_PCCA_101_PROTOCOL (0x02)
#define USB_CDC_AT_PCCA_101_ANNEX_O (0x03)
#define USB_CDC_AT_GSM_7_07 (0x04)
#define USB_CDC_AT_3GPP_27_007 (0x05)
#define USB_CDC_AT_TIA_CDMA (0x06)
#define USB_CDC_ETHERNET_EMULATION_PROTOCOL (0x07)
#define USB_CDC_EXTERNAL_PROTOCOL (0xFE)
#define USB_CDC_VENDOR_SPECIFIC (0xFF) /*also for Data Class Protocol Code */

/* Data Class Protocol Codes */
#define USB_CDC_PYHSICAL_INTERFACE_PROTOCOL (0x30)
#define USB_CDC_HDLC_PROTOCOL (0x31)
#define USB_CDC_TRANSPARENT_PROTOCOL (0x32)
#define USB_CDC_MANAGEMENT_PROTOCOL (0x50)
#define USB_CDC_DATA_LINK_Q931_PROTOCOL (0x51)
#define USB_CDC_DATA_LINK_Q921_PROTOCOL (0x52)
#define USB_CDC_DATA_COMPRESSION_V42BIS (0x90)
#define USB_CDC_EURO_ISDN_PROTOCOL (0x91)
#define USB_CDC_RATE_ADAPTION_ISDN_V24 (0x92)
#define USB_CDC_CAPI_COMMANDS (0x93)
#define USB_CDC_HOST_BASED_DRIVER (0xFD)
#define USB_CDC_UNIT_FUNCTIONAL (0xFE)

/* Descriptor SubType in Communications Class Functional Descriptors */
#define USB_CDC_HEADER_FUNC_DESC (0x00)
#define USB_CDC_CALL_MANAGEMENT_FUNC_DESC (0x01)
#define USB_CDC_ABSTRACT_CONTROL_FUNC_DESC (0x02)
#define USB_CDC_DIRECT_LINE_FUNC_DESC (0x03)
#define USB_CDC_TELEPHONE_RINGER_FUNC_DESC (0x04)
#define USB_CDC_TELEPHONE_REPORT_FUNC_DESC (0x05)
#define USB_CDC_UNION_FUNC_DESC (0x06)
#define USB_CDC_COUNTRY_SELECT_FUNC_DESC (0x07)
#define USB_CDC_TELEPHONE_MODES_FUNC_DESC (0x08)
#define USB_CDC_TERMINAL_FUNC_DESC (0x09)
#define USB_CDC_NETWORK_CHANNEL_FUNC_DESC (0x0A)
#define USB_CDC_PROTOCOL_UNIT_FUNC_DESC (0x0B)
#define USB_CDC_EXTENSION_UNIT_FUNC_DESC (0x0C)
#define USB_CDC_MULTI_CHANNEL_FUNC_DESC (0x0D)
#define USB_CDC_CAPI_CONTROL_FUNC_DESC (0x0E)
#define USB_CDC_ETHERNET_NETWORKING_FUNC_DESC (0x0F)
#define USB_CDC_ATM_NETWORKING_FUNC_DESC (0x10)
#define USB_CDC_WIRELESS_CONTROL_FUNC_DESC (0x11)
#define USB_CDC_MOBILE_DIRECT_LINE_FUNC_DESC (0x12)
#define USB_CDC_MDLM_DETAIL_FUNC_DESC (0x13)
#define USB_CDC_DEVICE_MANAGEMENT_FUNC_DESC (0x14)
#define USB_CDC_OBEX_FUNC_DESC (0x15)
#define USB_CDC_COMMAND_SET_FUNC_DESC (0x16)
#define USB_CDC_COMMAND_SET_DETAIL_FUNC_DESC (0x17)
#define USB_CDC_TELEPHONE_CONTROL_FUNC_DESC (0x18)
#define USB_CDC_OBEX_SERVICE_ID_FUNC_DESC (0x19)

/* usb descriptor length */
#define USB_DESCRIPTOR_LENGTH_CONFIGURATION_ALL (sizeof(g_UsbDeviceConfigurationDescriptor))
#define USB_CDC_VCOM_REPORT_DESCRIPTOR_LENGTH (33)
#define USB_IAD_DESC_SIZE (8)
#define USB_DESCRIPTOR_LENGTH_CDC_HEADER_FUNC (5)
#define USB_DESCRIPTOR_LENGTH_CDC_CALL_MANAG (5)
#define USB_DESCRIPTOR_LENGTH_CDC_ABSTRACT (4)
#define USB_DESCRIPTOR_LENGTH_CDC_UNION_FUNC (5)

#define USB_DEVICE_CONFIGURATION_COUNT (1)
#define USB_DEVICE_STRING_COUNT (5)
#define USB_DEVICE_LANGUAGE_COUNT (1)

#if defined (USB_DEVICE_CONFIG_MTP) && (USB_DEVICE_CONFIG_MTP > 0U)
#   define USB_INTERFACE_COUNT (3)

#   define USB_MTP_INTERFACE_INDEX (0)
#   define USB_CDC_VCOM_CIC_INTERFACE_INDEX (1)
#   define USB_CDC_VCOM_DIC_INTERFACE_INDEX (2)

#   define USB_DECRIPTOR_CONFIGURATION_LENGTH  \
    (USB_DESCRIPTOR_LENGTH_CONFIGURE + \
    USB_IAD_DESC_SIZE + \
    USB_DESCRIPTOR_LENGTH_INTERFACE + USB_DESCRIPTOR_LENGTH_CDC_HEADER_FUNC + USB_DESCRIPTOR_LENGTH_CDC_CALL_MANAG + \
    USB_DESCRIPTOR_LENGTH_CDC_ABSTRACT + USB_DESCRIPTOR_LENGTH_CDC_UNION_FUNC + USB_DESCRIPTOR_LENGTH_ENDPOINT + \
    USB_DESCRIPTOR_LENGTH_INTERFACE + USB_DESCRIPTOR_LENGTH_ENDPOINT + USB_DESCRIPTOR_LENGTH_ENDPOINT + \
    USB_DESCRIPTOR_LENGTH_INTERFACE + 3 * USB_DESCRIPTOR_LENGTH_ENDPOINT)
#else

#   define USB_INTERFACE_COUNT (2)

#   define USB_CDC_VCOM_CIC_INTERFACE_INDEX (0)
#   define USB_CDC_VCOM_DIC_INTERFACE_INDEX (1)

#   define USB_DECRIPTOR_CONFIGURATION_LENGTH  \
    (USB_DESCRIPTOR_LENGTH_CONFIGURE + \
    USB_IAD_DESC_SIZE + \
    USB_DESCRIPTOR_LENGTH_INTERFACE + USB_DESCRIPTOR_LENGTH_CDC_HEADER_FUNC + USB_DESCRIPTOR_LENGTH_CDC_CALL_MANAG + \
    USB_DESCRIPTOR_LENGTH_CDC_ABSTRACT + USB_DESCRIPTOR_LENGTH_CDC_UNION_FUNC + USB_DESCRIPTOR_LENGTH_ENDPOINT + \
    USB_DESCRIPTOR_LENGTH_INTERFACE + USB_DESCRIPTOR_LENGTH_ENDPOINT + USB_DESCRIPTOR_LENGTH_ENDPOINT)
#endif

#define USB_COMPOSITE_CONFIGURE_INDEX (1)

/* Configuration, interface and endpoint. */
#define USB_CDC_VCOM_CIC_CLASS (0x02)
#define USB_CDC_VCOM_CIC_SUBCLASS (0x02)
#define USB_CDC_VCOM_CIC_PROTOCOL (0x00)
#define USB_CDC_VCOM_DIC_CLASS (0x0A)
#define USB_CDC_VCOM_DIC_SUBCLASS (0x00)
#define USB_CDC_VCOM_DIC_PROTOCOL (0x00)

#define USB_CDC_VCOM_INTERFACE_COUNT (2)

#define USB_CDC_VCOM_CIC_ENDPOINT_COUNT (1)
#define USB_CDC_VCOM_CIC_INTERRUPT_IN_ENDPOINT (4)
#define USB_CDC_VCOM_DIC_ENDPOINT_COUNT (2)
#define USB_CDC_VCOM_DIC_BULK_IN_ENDPOINT (5)
#define USB_CDC_VCOM_DIC_BULK_OUT_ENDPOINT (6)

/* Packet size. */
#define HS_CDC_VCOM_INTERRUPT_IN_PACKET_SIZE (16)
#define FS_CDC_VCOM_INTERRUPT_IN_PACKET_SIZE (16)
#define HS_CDC_VCOM_INTERRUPT_IN_INTERVAL (0x07) /* 2^(7-1) = 8ms */
#define FS_CDC_VCOM_INTERRUPT_IN_INTERVAL (0x08)

#define HS_CDC_VCOM_BULK_IN_PACKET_SIZE (512)
#define FS_CDC_VCOM_BULK_IN_PACKET_SIZE (64)
#define HS_CDC_VCOM_BULK_OUT_PACKET_SIZE (512)
#define FS_CDC_VCOM_BULK_OUT_PACKET_SIZE (64)

#define USB_DESCRIPTOR_TYPE_CDC_CS_INTERFACE (0x24)
#define USB_DESCRIPTOR_TYPE_CDC_CS_ENDPOINT (0x25)

#define USB_MTP_DEVICE_CLASS    (0x06) /* Still Image */
#define USB_MTP_DEVICE_SUBCLASS (0x01)
#define USB_MTP_DEVICE_PROTOCOL (0x01)

#define USB_MTP_ENDPOINT_COUNT (3)
#define USB_MTP_INTERFACE_COUNT (1)


#define USB_MTP_BULK_IN_ENDPOINT (1)
#define USB_MTP_BULK_OUT_ENDPOINT (2)
#define USB_MTP_INTR_IN_ENDPOINT (3)

#define HS_MTP_BULK_IN_PACKET_SIZE (512)
#define FS_MTP_BULK_IN_PACKET_SIZE (64)
#define HS_MTP_BULK_OUT_PACKET_SIZE (512)
#define FS_MTP_BULK_OUT_PACKET_SIZE (64)
#define HS_MTP_INTR_IN_PACKET_SIZE (16)
#define FS_MTP_INTR_IN_PACKET_SIZE (16)
#define HS_MTP_INTR_IN_INTERVAL (0x07) /* 2^(7-1) = 8ms */
#define FS_MTP_INTR_IN_INTERVAL (0x08)

/* Class code. */
#define USB_DEVICE_CLASS    (0x00)
#define USB_DEVICE_SUBCLASS (0x00)
#define USB_DEVICE_PROTOCOL (0x00)

#define USB_DEVICE_MAX_POWER (0xFA)

/*******************************************************************************
* API
******************************************************************************/
/*!
 * @brief USB device set speed function.
 *
 * This function sets the speed of the USB device.
 *
 * Due to the difference of HS and FS descriptors, the device descriptors and configurations need to be updated to match
 * current speed.
 * As the default, the device descriptors and configurations are configured by using FS parameters for both EHCI and
 * KHCI.
 * When the EHCI is enabled, the application needs to call this function to update device by using current speed.
 * The updated information includes endpoint max packet size, endpoint interval, etc.
 *
 * @param handle The USB device handle.
 * @param speed Speed type. USB_SPEED_HIGH/USB_SPEED_FULL/USB_SPEED_LOW.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
extern usb_status_t USB_DeviceSetSpeed(usb_device_handle handle, uint8_t speed);
#if (defined(USB_DEVICE_CONFIG_CV_TEST) && (USB_DEVICE_CONFIG_CV_TEST > 0U))
/* Get device qualifier descriptor request */
usb_status_t USB_DeviceGetDeviceQualifierDescriptor(
    usb_device_handle handle, usb_device_get_device_qualifier_descriptor_struct_t *deviceQualifierDescriptor);
#endif
/*!
 * @brief USB device get device descriptor function.
 *
 * This function gets the device descriptor of the USB device.
 *
 * @param handle The USB device handle.
 * @param deviceDescriptor The pointer to the device descriptor structure.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
usb_status_t USB_DeviceGetDeviceDescriptor(usb_device_handle handle,
                                           usb_device_get_device_descriptor_struct_t *deviceDescriptor);

/*!
 * @brief USB device get configuration descriptor function.
 *
 * This function gets the configuration descriptor of the USB device.
 *
 * @param handle The USB device handle.
 * @param configurationDescriptor The pointer to the configuration descriptor structure.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
usb_status_t USB_DeviceGetConfigurationDescriptor(
    usb_device_handle handle, usb_device_get_configuration_descriptor_struct_t *configurationDescriptor);

/*!
 * @brief USB device get string descriptor function.
 *
 * This function gets the string descriptor of the USB device.
 *
 * @param handle The USB device handle.
 * @param stringDescriptor Pointer to the string descriptor structure.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
usb_status_t USB_DeviceGetStringDescriptor(usb_device_handle handle,
                                           usb_device_get_string_descriptor_struct_t *stringDescriptor);

/*!
 * @brief USB device set serial number string descriptor function.
 *
 * This function sets the serial number string returned in the
 * descriptor of the USB device.
 *
 * @param serialNumberAscii Pointer to the ASCII string.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
uint16_t USB_DeviceSetSerialNumberString(const char *serialNumberAscii);

/*!
 * @brief USB device set bcdDevice in descriptor function.
 *
 * This function sets the bcdVersion the USB device.
 *
 * @param handle The USB device handle.
 * @param version Device version to set in descriptor.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
uint16_t USB_DeviceSetBcdVersion(usb_device_handle *handle, const uint16_t version);

#endif /* _USB_DEVICE_DESCRIPTOR_H_ */
