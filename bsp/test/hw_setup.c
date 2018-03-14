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

/*******************************************************************************
 * dbg field
*******************************************************************************/
#define MEMORY_SIZE     (32 * 1024)

static char memory_buf[MEMORY_SIZE];
static int  memory_alloced = 0;

static void hw_dbg_memory_reset(void)
{
    memset(memory_buf, 0, MEMORY_SIZE);
    memory_alloced = 0;
}

void hw_dbg_set_systicks(uint32_t x)
{
    system_ticks_count = x;
}

void hw_dbg_reset(void)
{
    hw_dbg_set_systicks(0);

    hw_dbg_memory_reset();

    hw_dbg_console_reset();

    hw_scripts_erase();
}

/*******************************************************************************
 * dbg field end
*******************************************************************************/
uint32_t system_ticks_count = 0;

void hw_setup(void)
{
    /* device resouce setup */
    hw_setup_gpio();
    hw_setup_uart();
    hw_setup_adc();
    hw_setup_pwm();
    hw_setup_pulse();
    hw_setup_timer();
    hw_setup_counter();
}

void hw_poll(void)
{
    static uint32_t system_ticks_count_pre = 0;

    if (system_ticks_count_pre != system_ticks_count) {
        system_ticks_count_pre = system_ticks_count;
        systick_event_post();
    }
}

void hw_halt(void)
{
    printf("\nSystem into halt!\n");
    while(1)
        ;
}

void hw_info_get(hw_info_t * info)
{
    (void) info;

    return;
}

int hw_memory_alloc(void **p, int size, int align)
{
    int start = (intptr_t) memory_buf + memory_alloced;
    int shift = 0;

    if (size == 0) {
        return 0;
    }

    if (start % align) {
        shift = align - (start % align);
    } else {
        shift = 0;
    }
    memory_alloced += shift;


    if (size < 0) {
        size = MEMORY_SIZE - memory_alloced;
    } else
    if (memory_alloced + size > MEMORY_SIZE) {
        return -1;
    }

    if (p) {
        *p = memory_buf + memory_alloced;
        memory_alloced += size;
    }

    return size;
}

int hw_pin_map(int id, int port, int pin)
{
    (void) id;
    (void) port;
    (void) pin;

    return 0;
}

int hw_led_map(int port, int pin)
{
    (void) port;
    (void) pin;

    return 0;
}

const hw_driver_t *hw_device_request(int type, int instance)
{
    switch (type) {
    case DEVICE_TYPE_PIN:       return hw_request_pin(instance);
    case DEVICE_TYPE_ADC:       return hw_request_adc(instance);
    case DEVICE_TYPE_DAC:       return NULL;
    case DEVICE_TYPE_PWM:       return hw_request_pwm(instance);
    case DEVICE_TYPE_PULSE:     return hw_request_pulse(instance);
    case DEVICE_TYPE_TIMER:     return hw_request_timer(instance);
    case DEVICE_TYPE_COUNTER:   return hw_request_counter(instance);
    case DEVICE_TYPE_UART:      return hw_request_uart(instance);
    case DEVICE_TYPE_USART:
    case DEVICE_TYPE_SPI:
    default:                    return NULL;
    }
}

int hw_device_instances(int type)
{
    switch (type) {
    case DEVICE_TYPE_PIN:       return HW_INSTANCES_PIN;
    case DEVICE_TYPE_ADC:       return HW_INSTANCES_ADC;
    case DEVICE_TYPE_DAC:       return 0;
    case DEVICE_TYPE_PWM:       return HW_INSTANCES_PWM;
    case DEVICE_TYPE_PULSE:     return HW_INSTANCES_PULSE;
    case DEVICE_TYPE_TIMER:     return HW_INSTANCES_TIMER;
    case DEVICE_TYPE_COUNTER:   return HW_INSTANCES_COUNTER;
    case DEVICE_TYPE_UART:      return HW_INSTANCES_UART;
    case DEVICE_TYPE_USART:
    case DEVICE_TYPE_SPI:
    default:                    return 0;
    }
}

