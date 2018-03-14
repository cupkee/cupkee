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

#ifndef __HW_MOCK_INC__
#define __HW_MOCK_INC__

/******************************************************************************
 * system interface
******************************************************************************/
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <cupkee.h>

/******************************************************************************
 * Hardware interface
******************************************************************************/
#include "hw_console.h"

#include "hw_gpio.h"
#include "hw_uart.h"
#include "hw_adc.h"
#include "hw_pwm.h"
#include "hw_pulse.h"
#include "hw_timer.h"
#include "hw_counter.h"

/******************************************************************************
 * Debug api
******************************************************************************/

// MISC
void hw_dbg_reset(void);
void hw_dbg_set_systicks(uint32_t x);

// CONSOLE
void hw_dbg_console_reset(void);
void hw_dbg_console_set_input(const char *data);
int  hw_dbg_console_get_reply(char **ptr);
void hw_dbg_console_clr_buf(void);

// pin device
void hw_dbg_pin_setup_status_set(int instance, int status);
void hw_dbg_pin_data_set(int instance, int offset, uint32_t value);
uint32_t hw_dbg_pin_data_get(int instance, int offset);
void hw_dbg_pin_trigger_error(int instance, int code);
void hw_dbg_pin_trigger_data(int instance, uint32_t data);

// uart device
void hw_dbg_uart_setup_status_set(int instance, int status);
void hw_dbg_uart_data_give(int instance, const char *data);
void hw_dbg_uart_send_state(int instance, int status);
int  hw_dbg_uart_data_take(int instance, int n);

// adc device
void hw_dbg_adc_setup_status_set(int instance, int status);
void hw_dbg_adc_update(int instance, int chn, uint16_t data);

#if 0
#define _TRACE(fmt, ...)    printf(fmt, ##__VA_ARGS__)
#else
#define _TRACE(fmt, ...)    //
#endif

#endif /* __HW_MOCK_INC__ */

