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

#include "hardware.h"

extern char _etext;  // devined in ld scripts
extern char end;     // defined in ld scripts

extern vector_table_t vector_table;
void *hw_memory_bgn = NULL;
void *hw_memory_end = NULL;

static int8_t boot_state = HW_BOOT_STATE_PRODUCT;

static void hw_setup_memory(void)
{
    hw_memory_bgn = CUPKEE_ADDR_ALIGN(&end, 16);
    hw_memory_end = (char *)(vector_table.initial_sp_value) - SYSTEM_STACK_SIZE;
}

static void hw_setup_systick(void)
{
    systick_set_frequency(SYSTEM_TICKS_PRE_SEC, 72000000);

    systick_interrupt_enable();
    systick_counter_enable();
}

/* systick interrupt handle routing  */
void sys_tick_handler(void)
{
    _cupkee_systicks++;
    cupkee_event_post_systick();
}

size_t hw_boot_memory_size(void)
{
    return hw_memory_end - hw_memory_bgn;
}

void  *hw_boot_memory_alloc(size_t size, size_t align)
{
    void *memory_align;
    size_t left;

    if (size == 0) {
        return NULL;
    }

    if (align) {
        memory_align = CUPKEE_ADDR_ALIGN(hw_memory_bgn, align);
    } else {
        memory_align = hw_memory_bgn;
    }

    left = hw_memory_end - memory_align;
    if (left < size) {
        return NULL;
    }

    hw_memory_bgn = memory_align + size;

    return memory_align;
}

void hw_enter_critical(uint32_t *state)
{
    *state = cm_mask_interrupts(1);
}

void hw_exit_critical(uint32_t state)
{
    cm_mask_interrupts(state);
}

void hw_info_get(hw_info_t *info)
{
    if (info) {
        info->sys_freq = 72000000;
        info->ram_sz = (char *)(vector_table.initial_sp_value) - (char *)0x20000000;
        //info->rom_sz = desig_get_flash_size() * 1024;
        info->rom_sz = &_etext - (char *)0x8000000;
        info->ram_base = (void *)0x20000000;
        info->rom_base = (void *)0x08000000;
    }
}

static void hw_boot_mode_probe(void)
{
    if (0 == hw_pin_map(0, BOOT_PROBE_BANK, BOOT_PROBE_PIN, HW_DIR_IN)) {
        boot_state = hw_pin_get(0) == BOOT_PROBE_DEV ? HW_BOOT_STATE_DEVEL : HW_BOOT_STATE_PRODUCT;

        hw_pin_unmap(0);
    }
}

void hw_setup(void)
{
	rcc_clock_setup_in_hse_8mhz_out_72mhz();

    hw_setup_memory();

    hw_setup_gpio();

    hw_boot_mode_probe();
    if (boot_state == HW_BOOT_STATE_PRODUCT) {
        rcc_periph_clock_enable(RCC_AFIO);

        // Disable SW and JTAG
        AFIO_MAPR = (AFIO_MAPR & ~AFIO_MAPR_SWJ_MASK) | AFIO_MAPR_SWJ_CFG_JTAG_OFF_SW_OFF;
    }

    hw_setup_timer();
    hw_setup_storage();
    hw_setup_systick();
}

int hw_device_setup(void)
{
    /* initial device resouce */
    hw_setup_usart();
    hw_setup_adc();
    hw_setup_i2c();
    hw_setup_spi();
    hw_setup_usb();

    return 0;
}

void hw_poll(void)
{
    hw_poll_usb();
}

void hw_halt(void)
{
    while (1)
        ;
}

int  hw_boot_state(void)
{
    return boot_state;
}
