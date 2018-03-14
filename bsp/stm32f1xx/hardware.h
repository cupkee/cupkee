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
#include <libopencm3/cm3/scb.h>
#include <libopencm3/stm32/desig.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/exti.h>
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

// HW const
#define HW_FL_USED              1
#define HW_FL_BUSY              2
#define HW_FL_RXE               4
#define HW_FL_TXE               8

// HW configure
#define SYS_PCLK                ((uint32_t)36000000)

#define GPIO_BANK_MAX           7
#define GPIO_BANK_MASK          7

#define GPIO_PIN_MAX            16
#define GPIO_PIN_MASK           15

#define GPIO_MAP_MAX            32

#define BOOT_PROBE_BANK         0  // GPIOA
#define BOOT_PROBE_PIN          12 // GPIO12
#define BOOT_PROBE_DEV          0  // Low

#include "hw_usb.h"
#include "hw_storage.h"

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

