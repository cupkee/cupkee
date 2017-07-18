/*
MIT License

This file is part of cupkee project.

Copyright (c) 2016-2017 Lixing Ding <ding.lixing@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "hardware.h"

static uint8_t cdc_state = 0;
static uint8_t cdc_devid = 0;
static uint8_t cdc_ready = 0;

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

static void cdcacm_data_rx_cb(usbd_device *usbd_dev, uint8_t ep)
{
    (void)usbd_dev;
	(void)ep;

    if (!cdc_ready) {
        cdc_ready = 1;
        // Send Ok!
        cupkee_event_post_device_drain(cdc_devid);
    }

    if (cdc_state == 2) {
        cupkee_event_post_device_data(cdc_devid);
    }
}

static void cdcacm_data_tx_cb(usbd_device *usbd_dev, uint8_t ep)
{
    (void)usbd_dev;
	(void)ep;

    if (cdc_state == 2) {
        cupkee_event_post_device_drain(cdc_devid);
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

static usbd_device *usbd_dev;

static void cdc_release(int instance)
{
    if (instance == 0) {
        cdc_state = 0;
    }
}

static void cdc_reset(int instance)
{
    if (instance == 0) {
        cdc_state = 1;
    }
}

static int cdc_setup(int instance, uint8_t dev_id, const hw_config_t *conf)
{
    (void) conf;
    if (instance == 0) {
        cdc_state = 2;
        cdc_devid = dev_id;
    }
    return 0;
}

static int cdc_send_byte(uint8_t c)
{
    if (cdc_ready) {
        return usbd_ep_write_packet(usbd_dev, 0x82, &c, 1);
    } else {
        return 0;
    }
}

static int cdc_recv_byte(uint8_t *c)
{
    return usbd_ep_read_packet(usbd_dev, 0x01, c, 1);
}

static int cdc_send(int instance, size_t n, const void *data)
{
    const uint8_t *ptr = data;
    size_t i = 0;

    (void) instance;

    while (i < n && cdc_send_byte(ptr[i])) {
        i++;
    }

    return i;
}

static int cdc_recv(int instance, size_t n, void *data)
{
    (void) instance;
	return usbd_ep_read_packet(usbd_dev, 0x01, data, n);
}

static int cdc_send_sync(int instance, size_t n, const void *data)
{
    const uint8_t *ptr = data;
    size_t i;

    (void) instance;

    for (i = 0; i < n; i++) {
        while (!cdc_send_byte(ptr[i]))
            ;
    }

    return i;
}

static int cdc_recv_sync(int instance, size_t n, void *data)
{
    uint8_t *ptr = data;
    size_t i;

    (void) instance;

    for (i = 0; i < n; i++) {
        while (!cdc_recv_byte(ptr + i))
            ;
    }

    return i;
}

static int cdc_io_cached(int instance, size_t *in, size_t *out)
{
    (void) instance;

    //Todo: real counter
    if (in) {
        *in = 1;
    }
    if (out) {
        *out = 0;
    }
    return 0;
}

static const hw_driver_t cdc_driver = {
    .release = cdc_release,
    .reset   = cdc_reset,
    .setup   = cdc_setup,

    .read = cdc_recv,
    .write = cdc_send,
    .read_sync = cdc_recv_sync,
    .write_sync = cdc_send_sync,
    .io_cached = cdc_io_cached,
};

const hw_driver_t *hw_request_cdc(int instance)
{
    if (instance != 0 || cdc_state) {
        return NULL;
    }
    cdc_state = 1;

    return &cdc_driver;
}

void hw_setup_usb(void)
{
	usbd_dev = usbd_init(&st_usbfs_v1_usb_driver, &dev, &config, usb_strings, 3, usbd_control_buffer, sizeof(usbd_control_buffer));

	usbd_register_set_config_callback(usbd_dev, cdcacm_set_config);

    cdc_state = 0;
    cdc_devid = 0;
    cdc_ready = 0;

    //_usbd_reset(usbd_dev);
}

void hw_poll_usb(void)
{
    usbd_poll(usbd_dev);
}

void hw_usb_msc_init(const char *vendor, const char *product, const char *version, uint32_t blocks,
                     int (*read_cb)(uint32_t lba, uint8_t *),
                     int (*write_cb)(uint32_t lba, const uint8_t *))
{
    usb_msc_init(usbd_dev, 0x85, 64, 0x04, 64, vendor, product, version,
            blocks, read_cb, write_cb);

    //_usbd_reset(usbd_dev);
}

