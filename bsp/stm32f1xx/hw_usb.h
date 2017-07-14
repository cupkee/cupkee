

#ifndef __HW_USB_INC__
#define __HW_USB_INC__

void hw_setup_usb(void);
void hw_poll_usb(void);
const hw_driver_t *hw_request_cdc(int instance);

#endif /* __HW_USB_INC__ */

