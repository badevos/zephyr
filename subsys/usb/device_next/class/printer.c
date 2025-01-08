/*!
 * \file printer.c
 * \author Klaas Hofman (klaas.hofmab@verhaert.com)
 *
 * \version 0.1
 * \date 2025-01-06
 *
 * @copyright Copyright (c) 2025 Verhaert New Products and Services
 *
 */
#include <zephyr/sys/byteorder.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/drivers/usb/udc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usb_printer, CONFIG_USBD_PRINTER_LOG_LEVEL);

#define LB_VENDOR_REQ_OUT 0x5b // @TODO: replace with actual value
#define LB_VENDOR_REQ_IN  0x5c

#ifndef CONFIG_USB_DEVICE_VID
#define CONFIG_USB_DEVICE_VID 0x0922 /* Dymo Co-star*/
#endif

#ifndef CONFIG_USB_DEVICE_PID
#define CONFIG_USB_DEVICE_PID 0x002a
#endif

#define USB_PRINTER_CLASS 0x07

static const struct usbd_cctx_vendor_req printer_vregs = USBD_VENDOR_REQ(0, 0);

struct usbd_class_api printer_api = {
	.update = NULL,
	.control_to_host = NULL,
	.control_to_dev = NULL,
	.request = NULL,
	.get_desc = NULL,
	.init = NULL,
};

struct printer_desc {
	struct usb_device_descriptor dds;
	struct usb_cfg_descriptor cds;
	struct usb_if_descriptor if0;
	struct usb_ep_descriptor if0_in_ep;
	struct usb_if_descriptor if1;
	struct usb_ep_descriptor if1_int_in_ep;
	struct usb_desc_header nil_desc;
};

// @TODO: replace with actual values
#define DEFINE_PRINTER_DESCRIPTOR(x, _)                                                            \
	static struct printer_desc printer_desc_##x = {                                            \
		.dds =                                                                             \
			{                                                                          \
				.bLength = sizeof(struct usb_device_descriptor),                   \
				.bDescriptorType = USB_DESC_DEVICE,                                \
				.bcdUSB = sys_cpu_to_le16(USB_SRN_2_0),                            \
				.bDeviceClass = 0,                                                 \
				.bDeviceSubClass = 0,                                              \
				.bDeviceProtocol = 0,                                              \
				.bMaxPacketSize0 = 64,                                             \
				.idVendor = sys_cpu_to_le16((uint16_t)CONFIG_USB_DEVICE_VID),      \
				.idProduct = sys_cpu_to_le16((uint16_t)CONFIG_USB_DEVICE_PID),     \
				.bcdDevice = sys_cpu_to_le16(0x0100),                              \
				.iManufacturer = 1,                                                \
				.iProduct = 2,                                                     \
				.iSerialNumber = 3,                                                \
				.bNumConfigurations = 1,                                           \
			},                                                                         \
		.cds =                                                                             \
			{                                                                          \
				.bLength = sizeof(struct usb_cfg_descriptor),                      \
				.bDescriptorType = USB_DESC_CONFIGURATION,                         \
				.wTotalLength = 0,                                                 \
				.bNumInterfaces = 1,                                               \
				.bConfigurationValue = 1,                                          \
				.iConfiguration = 0,                                               \
				.bmAttributes = 0,                                                 \
			},                                                                         \
		.if0 =                                                                             \
			{                                                                          \
				.bLength = sizeof(struct usb_if_descriptor),                       \
				.bDescriptorType = USB_DESC_INTERFACE,                             \
				.bInterfaceNumber = 0,                                             \
				.bAlternateSetting = 0,                                            \
				.bNumEndpoints = 1,                                                \
				.bInterfaceClass = USB_PRINTER_CLASS,                              \
				.bInterfaceSubClass = 1,                                           \
				.bInterfaceProtocol = 1,                                           \
				.iInterface = 0,                                                   \
			},                                                                         \
		.if0_in_ep =                                                                       \
			{                                                                          \
				.bLength = sizeof(struct usb_ep_descriptor),                       \
				.bDescriptorType = USB_DESC_ENDPOINT,                              \
				.bEndpointAddress = 0x03,                                          \
				.bmAttributes = USB_EP_TYPE_ISO,                                   \
				.wMaxPacketSize = sys_cpu_to_le16(64),                             \
				.bInterval = 1,                                                    \
			},                                                                         \
		.if1 =                                                                             \
			{                                                                          \
				.bLength = sizeof(struct usb_if_descriptor),                       \
				.bDescriptorType = USB_DESC_INTERFACE,                             \
				.bInterfaceNumber = 1,                                             \
				.bAlternateSetting = 0,                                            \
				.bNumEndpoints = 2,                                                \
				.bInterfaceClass = USB_PRINTER_CLASS,                              \
				.bInterfaceSubClass = 1,                                           \
				.bInterfaceProtocol = 1,                                           \
				.iInterface = 0,                                                   \
			},                                                                         \
		.if1_int_in_ep =                                                                   \
			{                                                                          \
				.bLength = sizeof(struct usb_ep_descriptor),                       \
				.bDescriptorType = USB_DESC_ENDPOINT,                              \
				.bEndpointAddress = 0x83,                                          \
				.bmAttributes = USB_EP_TYPE_ISO,                                   \
				.wMaxPacketSize = sys_cpu_to_le16(64),                             \
				.bInterval = 1,                                                    \
			},                                                                         \
		.nil_desc = {.bLength = 0, .bDescriptorType = 0, \ },                              \
	};                                                                                         \
                                                                                                   \
    const static struct usb_desc_header *printer_fs_desc_##x[] = {                            \
        (struct usb_desc_header *)&printer_desc_##x.dds,                                    \
        (struct usb_desc_header *)&printer_desc_##x.if0,                                    \
        (struct usb_desc_header *)&printer_desc_##x.if0_in_ep,                              \
        (struct usb_desc_header *)&printer_desc_##x.if1,                                    \
        (struct usb_desc_header *)&printer_desc_##x.if1_int_in_ep,                          \
        {struct usb_desc_header *)&printer_desc_##x.nil_desc,                                \
    };                                                                                             \
                                                                                                   \
    const static struct usb_desc_header *printer_hs_desc_##x[] = {                            \
        (struct usb_desc_header *)&printer_desc_##x.dds,                                    \
        (struct usb_desc_header *)&printer_desc_##x.if0,                                    \
        (struct usb_desc_header *)&printer_desc_##x.if0_in_ep,                              \
        (struct usb_desc_header *)&printer_desc_##x.if1,                                    \
        (struct usb_desc_header *)&printer_desc_##x.if1_int_in_ep,                          \
        {struct usb_desc_header *)&printer_desc_##x.nil_desc,                                \
    };

#define PRINTER_CLASS_DATA(x, _)                                                                   \
	static struct printer_data printer_data_##x = {                                            \
		.desc = &printer_desc_##x,                                                         \
		.fs_desc = printer_fs_desc_##x,                                                    \
		.hs_desc = printer_hs_desc_##x,                                                    \
	};                                                                                         \
                                                                                                   \
	USBD_DEFINE_CLASS(printer_##x, &printer_api, &printer_data_##x, &printer_vregs);

LISTIFY(CONFIG_USBD_PRINTER_INSTANCES_COUNT, DEFINE_PRINTER_DESCRIPTOR, ())
LISTIFY(CONFIG_USBD_PRINTER_MAX_INSTANCES, DEFINE_PRINTER_CLASS_DATA, ())