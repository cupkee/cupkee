/*
MIT License

This file is part of cupkee project.

Copyright (c) 2016 Lixing Ding <ding.lixing@gmail.com>

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

#ifndef __HARDWARE_INC__
#define __HARDWARE_INC__

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/cm3/cortex.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/vector.h>
#include <libopencm3/stm32/desig.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/cdc.h>
#include <libopencm3/usb/msc.h>

#include <cupkee.h>

#define SYS_PCLK   ((uint32_t)36000000)

#define HW_FL_USED              1
#define HW_FL_BUSY              2
#define HW_FL_RXE               4
#define HW_FL_TXE               8

static inline int hw_use_instance(int instance, uint8_t *use_map) {
    uint8_t bit = 1 << instance;

    if (*use_map & bit) {
        return 0;
    } else {
        *use_map |= bit;
        return 1;
    }
}

static inline void hw_release_instance(int instance, uint8_t *use_map) {
    uint8_t bit = 1 << instance;
    *use_map &= ~bit;
}

#include "hw_usb.h"
#include "hw_misc.h"

#include "hw_gpio.h"
#include "hw_timer.h"
#include "hw_usart.h"
#include "hw_adc.h"
#include "hw_i2c.h"
#include "hw_spi.h"


#if 0
#define _TRACE(fmt, ...)    printf(fmt, ##__VA_ARGS__)
#else
#define _TRACE(fmt, ...)    //
#endif

#endif /* __HARDWARE_INC__ */

