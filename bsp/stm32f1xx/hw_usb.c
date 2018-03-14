/* GPLv2 License
 *
 * Copyright (C) 2016-2018 Lixing Ding <ding.lixing@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 **/

#include "hardware.h"

static void   *cdc_entry = NULL;
static uint8_t cdc_flags = 0;

#ifndef USB_CLASS_MISCELLANEOUS
#define USB_CLASS_MISCELLANEOUS 0xEF
#endif

static const struct usb_device_descriptor dev = {
	.bLength = USB_DT_DEVICE_SIZE,
	.bDescriptorType = USB_DT_DEVICE,
	.bcdUSB = 0x0200,
	.bDeviceClass = USB_CLASS_MISCELLANEOUS,
	.bDeviceSubClass = 2,
	.bDeviceProtocol = 1,
	.bMaxPacketSize0 = 64,
	.idVendor = 0x1029,
	.idProduct = 0x2016,
	.bcdDevice = 0x0200,
	.iManufacturer = 1,
	.iProduct = 2,
	.iSerialNumber = 3,
	.bNumConfigurations = 1,
};

/*
 * This notification endpoint isn't implemented. According to CDC spec it's
 * optional, but its absence causes a NULL pointer dereference in the
 * Linux cdc_acm driver.
 */
static const struct usb_endpoint_descriptor comm_endp[] = {{
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = 0x83,
	.bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
	.wMaxPacketSize = 16,
	.bInterval = 255,
}};

static const struct usb_endpoint_descriptor data_endp[] = {{
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = 0x01,
	.bmAttributes = USB_ENDPOINT_ATTR_BULK,
	.wMaxPacketSize = 64,
	.bInterval = 1,
}, {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = 0x82,
	.bmAttributes = USB_ENDPOINT_ATTR_BULK,
	.wMaxPacketSize = 64,
	.bInterval = 1,
}};

static const struct usb_endpoint_descriptor msc_endp[] = {{
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = 0x04,
	.bmAttributes = USB_ENDPOINT_ATTR_BULK,
	.wMaxPacketSize = 64,
	.bInterval = 0,
}, {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = 0x85,
	.bmAttributes = USB_ENDPOINT_ATTR_BULK,
	.wMaxPacketSize = 64,
	.bInterval = 0,
}};

static const struct {
	struct usb_cdc_header_descriptor header;
	struct usb_cdc_call_management_descriptor call_mgmt;
	struct usb_cdc_acm_descriptor acm;
	struct usb_cdc_union_descriptor cdc_union;
} __attribute__((packed)) cdcacm_functional_descriptors = {
	.header = {
		.bFunctionLength = sizeof(struct usb_cdc_header_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_HEADER,
		.bcdCDC = 0x0110,
	},
	.call_mgmt = {
		.bFunctionLength =
			sizeof(struct usb_cdc_call_management_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_CALL_MANAGEMENT,
		.bmCapabilities = 0,
		.bDataInterface = 1,
	},
	.acm = {
		.bFunctionLength = sizeof(struct usb_cdc_acm_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_ACM,
		.bmCapabilities = 0,
	},
	.cdc_union = {
		.bFunctionLength = sizeof(struct usb_cdc_union_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_UNION,
		.bControlInterface = 0,
		.bSubordinateInterface0 = 1,
	}
};

static const struct usb_interface_descriptor comm_iface[] = {{
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = 0,
	.bAlternateSetting = 0,
	.bNumEndpoints = 1,
	.bInterfaceClass = USB_CLASS_CDC,
	.bInterfaceSubClass = USB_CDC_SUBCLASS_ACM,
	.bInterfaceProtocol = USB_CDC_PROTOCOL_AT,
	.iInterface = 0,

	.endpoint = comm_endp,

	.extra = &cdcacm_functional_descriptors,
	.extralen = sizeof(cdcacm_functional_descriptors)
}};

static const struct usb_interface_descriptor data_iface[] = {{
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = 1,
	.bAlternateSetting = 0,
	.bNumEndpoints = 2,
	.bInterfaceClass = USB_CLASS_DATA,
	.bInterfaceSubClass = 0,
	.bInterfaceProtocol = 0,
	.iInterface = 0,
	.endpoint = data_endp,
}};

static const struct usb_interface_descriptor msc_iface[] = {{
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = 2,
	.bAlternateSetting = 0,
	.bNumEndpoints = 2,
	.bInterfaceClass = USB_CLASS_MSC,
	.bInterfaceSubClass = USB_MSC_SUBCLASS_SCSI,
	.bInterfaceProtocol = USB_MSC_PROTOCOL_BBB,
	.iInterface = 0,

	.endpoint = msc_endp,
}};

static const struct usb_interface ifaces[] = {{
	.num_altsetting = 1,
	.altsetting = comm_iface,
}, {
	.num_altsetting = 1,
	.altsetting = data_iface,
}, {
	.num_altsetting = 1,
	.altsetting = msc_iface,
}};

static const struct usb_config_descriptor config = {
	.bLength = USB_DT_CONFIGURATION_SIZE,
	.bDescriptorType = USB_DT_CONFIGURATION,
	.wTotalLength = 0,
	.bNumInterfaces = 3,
	.bConfigurationValue = 1,
	.iConfiguration = 0,
	.bmAttributes = 0x80,
	.bMaxPower = 0x32,

	.interface = ifaces,
};

static const char *usb_strings[] = {
	"Cupkee",
	"Cupkee-Iface",
	"SN:",
};

/* Buffer to be used for control requests. */
uint8_t usbd_control_buffer[128];

static usbd_device *usb_hnd;

static int cdcacm_control_request(usbd_device *usbd_dev, struct usb_setup_data *req, uint8_t **buf,
		uint16_t *len, void (**complete)(usbd_device *usbd_dev, struct usb_setup_data *req))
{
	(void)complete;
	(void)buf;
	(void)usbd_dev;

	switch (req->bRequest) {
	case USB_CDC_REQ_SET_CONTROL_LINE_STATE: {
		}
		return USBD_REQ_HANDLED;
	case USB_CDC_REQ_SET_LINE_CODING:
		if (*len < sizeof(struct usb_cdc_line_coding))
            return USBD_REQ_NOTSUPP;
		return USBD_REQ_HANDLED;
	}
    return USBD_REQ_NOTSUPP;
}

static inline int cdc_send_byte(uint8_t c)
{
    return usbd_ep_write_packet(usb_hnd, 0x82, &c, 1);
}

static inline int cdc_recv_byte(uint8_t *c)
{
    return usbd_ep_read_packet(usb_hnd, 0x01, c, 1);
}

static void cdcacm_data_rx_cb(usbd_device *usbd_dev, uint8_t ep)
{
    (void)usbd_dev;
	(void)ep;

    if (cdc_flags & HW_FL_RXE) {
        uint8_t data;

        while (1 == cdc_recv_byte(&data)) {
            if (1 != cupkee_device_push(cdc_entry, 1, &data)) {
                cdc_flags &= ~HW_FL_RXE;
                break;
            }
        }
    }
}

static void cdcacm_data_tx_cb(usbd_device *usbd_dev, uint8_t ep)
{
    (void)usbd_dev;
	(void)ep;

    if (cdc_flags & HW_FL_TXE) {
        uint8_t data;

        while (1 == cupkee_device_pull(cdc_entry, 1, &data)) {
            if (1 != cdc_send_byte(data)) {
                cupkee_unshift(cdc_entry, data);
                cdc_flags &= ~HW_FL_TXE;
                break;
            }
        }
    }
}

static void cdcacm_set_config(usbd_device *usbd_dev, uint16_t wValue)
{
	(void)wValue;

	usbd_ep_setup(usbd_dev, 0x01, USB_ENDPOINT_ATTR_BULK, 64, cdcacm_data_rx_cb);
	usbd_ep_setup(usbd_dev, 0x82, USB_ENDPOINT_ATTR_BULK, 64, cdcacm_data_tx_cb);
	usbd_ep_setup(usbd_dev, 0x83, USB_ENDPOINT_ATTR_INTERRUPT, 16, NULL);

	usbd_register_control_callback(
				usbd_dev,
				USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE,
				USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
				cdcacm_control_request);
}

static int cdc_reset(int instance)
{
    (void) instance;
    return 0;
}

static int cdc_setup(int instance, void *entry)
{
    if (instance == 0) {
        cdc_entry = entry;
        cdc_flags |= HW_FL_RXE | HW_FL_TXE;
    }
    return 0;
}

static int cdc_request(int instance)
{
    if (instance == 0 && cdc_flags == 0) {
        cdc_flags = HW_FL_USED;
        return 0;
    } else {
        return -CUPKEE_ERESOURCE;
    }
}

static int cdc_release(int instance)
{
    if (instance == 0) {
        cdc_flags = 0;
        return 0;
    } else {
        return -CUPKEE_EINVAL;
    }
}

static int cdc_write(int instance, size_t n, const void *data)
{
    (void) instance;

    if (n && data) {
        const uint8_t *ptr = data;
        size_t i = 0;

        for (i = 0; i < n; i++) {
            while (!cdc_send_byte(ptr[i]))
                ;
        }
        return i;
    }
    cdc_flags |= HW_FL_TXE;

    return 0;
}

static int cdc_read(int instance, size_t n, void *data)
{
    (void) instance;

    if (n && data) {
        uint8_t *ptr = data;
        size_t i;

        for (i = 0; i < n; i++) {
            while (!cdc_recv_byte(ptr + i))
                ;
        }

        return i;
    }
    cdc_flags |= HW_FL_RXE;

	return 0;
}

static const cupkee_driver_t cdc_driver = {
    .request = cdc_request,
    .release = cdc_release,
    .reset   = cdc_reset,
    .setup   = cdc_setup,

    .read    = cdc_read,
    .write   = cdc_write,
};

static const cupkee_device_desc_t hw_device_cdc = {
    .name = "usb-cdc",
    .inst_max = 1,
    .conf_init = NULL,
    .driver = &cdc_driver
};

void hw_setup_usb(void)
{
	usb_hnd = usbd_init(&st_usbfs_v1_usb_driver, &dev, &config, usb_strings, 3, usbd_control_buffer, sizeof(usbd_control_buffer));

	usbd_register_set_config_callback(usb_hnd, cdcacm_set_config);

    usb_msc_init(usb_hnd, 0x85, 64, 0x04, 64,
        "cupkee", "cupdisk", "0.01", CUPKEE_SYSDISK_SECTOR_COUNT,
        cupkee_sysdisk_read, cupkee_sysdisk_write);

    cdc_entry = NULL;
    cdc_flags = 0;

    //_usbd_reset(usb_hnd);
    cupkee_device_register(&hw_device_cdc);
}

void hw_poll_usb(void)
{
    usbd_poll(usb_hnd);
}

