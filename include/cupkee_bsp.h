/*
MIT License

This file is part of cupkee project.

Copyright (c) 2016,2017 Lixing Ding <ding.lixing@gmail.com>

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

#ifndef __CUPKEE_BSP_INC__
#define __CUPKEE_BSP_INC__

#include <stdint.h>

/****************************************************************/
/* hardware configure                                           */
/****************************************************************/
#define SYSTEM_TICKS_PRE_SEC                1000
#define SYSTEM_STACK_SIZE                   (8 * 1024)

/****************************************************************/
/* system define                                                */
/****************************************************************/
#define HW_STORAGE_BANK_BIN                 0
#define HW_STORAGE_BANK_BIN2                1
#define HW_STORAGE_BANK_APP                 2
#define HW_STORAGE_BANK_CFG                 3

#define HW_BOOT_STATE_PRODUCT               0
#define HW_BOOT_STATE_DEVEL                 1

enum HW_DIR {
    HW_DIR_IN,
    HW_DIR_OUT,
    HW_DIR_DUPLEX
};

typedef struct hw_info_t {
    int ram_sz;
    int rom_sz;
    void *ram_base;
    void *rom_base;
    uint32_t sys_freq;
} hw_info_t;


/****************************************************************/
/* hardware interface to implement                              */
/****************************************************************/
void hw_setup(void);
void _hw_reset(void);

void hw_poll(void);
void hw_halt(void);
int  hw_boot_state(void);

void hw_enter_critical(uint32_t *state);
void hw_exit_critical(uint32_t state);

void hw_info_get(hw_info_t *);

/* MEMORY */
void  *hw_boot_memory_alloc(size_t size, size_t align);
size_t hw_boot_memory_size(void);

/* STORAGE */
uint32_t hw_storage_size(int bank);
uint32_t hw_storage_data_length(int bank);
const char *hw_storage_data_map(int bank);
int hw_storage_erase (int bank);
int hw_storage_update(int bank, uint32_t offset, const uint8_t *data, int len);
int hw_storage_finish(int bank, uint32_t end);

/* GPIO */
int hw_gpio_enable(uint8_t bank, uint8_t port, uint8_t dir);
int hw_gpio_disable(uint8_t bank, uint8_t port);
int hw_gpio_listen(uint8_t bank, uint8_t port, uint8_t events, uint8_t which);
int hw_gpio_ignore(uint8_t bank, uint8_t port);
int hw_gpio_get(uint8_t bank, uint8_t port);
int hw_gpio_set(uint8_t bank, uint8_t port, int v);
int hw_gpio_toggle(uint8_t bank, uint8_t port);

/* TIMER */
int  hw_timer_alloc(void);
void hw_timer_release(int inst);

int hw_timer_start(int inst, int id, int us);
int hw_timer_stop(int inst);
int hw_timer_update(int inst, int us);
int hw_timer_duration_get(int inst);

/* DEVICE */
int hw_device_setup(void);

#endif /* __CUPKEE_BSP_INC__ */

