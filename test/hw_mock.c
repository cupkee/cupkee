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

#include "test.h"

#define FLASH_SIZE  (1024 * 256)

static uint8_t  mock_flash_base[FLASH_SIZE];
static size_t   mock_flash_size = FLASH_SIZE;
static uint8_t *mock_memory_base = NULL;
static size_t   mock_memory_size = 0;
static size_t   mock_memory_off  = 0;
static int mock_timer_curr_inst = 0;
static int mock_timer_curr_id   = -1;
static int mock_timer_curr_period = -1;
static int mock_timer_curr_duration = -1;
static int mock_timer_curr_state = -1;  // 0: stop, 1: start, -1: noused

void hw_mock_init(size_t mem_size)
{
    if (mock_memory_base) {
        free(mock_memory_base);
    }

    mock_memory_base = malloc(mem_size);
    mock_memory_size = mem_size;
    mock_memory_off = 0;
}

void hw_mock_deinit(void)
{
    if (mock_memory_base) {
        free(mock_memory_base);
        mock_memory_base = NULL;
        mock_memory_size = 0;
        mock_memory_off = 0;
    }
}

int hw_mock_timer_curr_id(void)
{
    return mock_timer_curr_id;
}

void hw_mock_timer_duration_set(int us)
{
    mock_timer_curr_duration = us;
}

int hw_mock_timer_curr_state(void)
{
    return mock_timer_curr_state;
}

int hw_mock_timer_period(void)
{
    return mock_timer_curr_period;
}
void hw_enter_critical(uint32_t *state)
{
    (void) state;
}

void hw_exit_critical(uint32_t state)
{
    (void) state;
}

void *hw_memory_alloc(size_t size, size_t align)
{
    size_t off = CUPKEE_SIZE_ALIGN(mock_memory_off, align);

    if (off + size > mock_memory_size) {
        return NULL;
    }

    mock_memory_off = off + size;

    return mock_memory_base + off;
}

size_t hw_memory_size(void)
{
    return mock_memory_size - mock_memory_off;
}

int hw_boot_state(void)
{
    return HW_BOOT_STATE_PRODUCT;
}

void hw_setup(hw_info_t *info)
{
    hw_info_get(info);
}

void hw_reset(int mode)
{
    (void) mode;
}

void hw_poll(void)
{}

void hw_halt(void)
{}

void hw_info_get(hw_info_t *info)
{
    info->ram_base = mock_memory_base;
    info->rom_base = mock_flash_base;

    info->ram_sz = mock_memory_size;
    info->rom_sz = mock_flash_size;
}

void hw_cuid_get(uint8_t *cuid)
{
    memset(cuid, 0, CUPKEE_UID_SIZE);
}

intptr_t hw_storage_base(void)
{
    return (intptr_t)&mock_flash_base;
}

int hw_storage_erase(uint32_t base, uint32_t size)
{
    (void) base;
    (void) size;
    return -1;
}

int hw_storage_program(uint32_t base, uint32_t len, const uint8_t *data)
{
    (void) base;
    (void) len;
    (void) data;

    return -1;
}


/* GPIO */
#define GPIO_BANK_MAX 8
#define GPIO_PORT_MAX 32
static uint8_t  gpio_event_id[GPIO_BANK_MAX * GPIO_PORT_MAX];
static uint32_t gpio_listen_rising[GPIO_BANK_MAX];
static uint32_t gpio_listen_falling[GPIO_BANK_MAX];
static uint32_t gpio_state[GPIO_BANK_MAX];
static uint32_t gpio_value[GPIO_BANK_MAX];

int hw_gpio_enable(uint8_t bank, uint8_t port, uint8_t dir)
{
    (void) dir;

    if (bank >= GPIO_BANK_MAX || port >= GPIO_PORT_MAX) {
        return -CUPKEE_EINVAL;
    }

    gpio_state[bank] |= 1 << port;

    return 0;
}

int hw_gpio_disable(uint8_t bank, uint8_t port)
{
    if (bank >= GPIO_BANK_MAX || port >= GPIO_PORT_MAX) {
        return -CUPKEE_EINVAL;
    }

    gpio_state[bank] &= ~(1 << port);

    return 0;
}

static void gpio_changed(uint8_t bank, uint8_t port)
{
    uint16_t pin = gpio_event_id[bank * GPIO_PORT_MAX + port];

    if ((gpio_value[bank] & (1 << port)) && (gpio_listen_rising[bank] & (1 << port))) {
        cupkee_event_post_pin(pin, 1);
    }
    if (!(gpio_value[bank] & (1 << port)) && (gpio_listen_falling[bank] & (1 << port))) {
        cupkee_event_post_pin(pin, 0);
    }
}

int hw_gpio_get(uint8_t bank, uint8_t port)
{
    if (bank >= GPIO_BANK_MAX || port >= GPIO_PORT_MAX) {
        return -CUPKEE_EINVAL;
    }

    return (gpio_value[bank] >> port) & 1;

    return 0;
}

int hw_gpio_set(uint8_t bank, uint8_t port, int v)
{
    if (bank >= GPIO_BANK_MAX || port >= GPIO_PORT_MAX) {
        return -CUPKEE_EINVAL;
    }

    if (v) {
        gpio_value[bank] |= (1 << port);
    } else {
        gpio_value[bank] &= ~(1 << port);
    }

    gpio_changed(bank, port);

    return 0;
}

int hw_gpio_toggle(uint8_t bank, uint8_t port)
{
    if (bank >= GPIO_BANK_MAX || port >= GPIO_PORT_MAX) {
        return -CUPKEE_EINVAL;
    }

    gpio_value[bank] ^= (1 << port);

    gpio_changed(bank, port);

    return 0;
}

int hw_gpio_listen(uint8_t bank, uint8_t port, uint8_t events, uint8_t pin)
{
    if (bank >= GPIO_BANK_MAX || port >= GPIO_PORT_MAX) {
        return -CUPKEE_EINVAL;
    }

    gpio_event_id[bank * GPIO_PORT_MAX + port] = pin;

    if (events & CUPKEE_EVENT_PIN_RISING) {
        gpio_listen_rising[bank] |= 1 << port;
    }

    if (events & CUPKEE_EVENT_PIN_FALLING) {
        gpio_listen_falling[bank] |= 1 << port;
    }

    gpio_value[bank] ^= (1 << port);

    return 0;
}

int hw_gpio_ignore(uint8_t bank, uint8_t port)
{
    if (bank >= GPIO_BANK_MAX || port >= GPIO_PORT_MAX) {
        return -CUPKEE_EINVAL;
    }

    gpio_listen_rising[bank] &= ~(1 << port);
    gpio_listen_falling[bank] &= ~(1 << port);

    return 0;
}

/* TIMER */
int hw_timer_alloc(void)
{
    return mock_timer_curr_inst;
}

void hw_timer_release(int inst)
{
    if (inst == mock_timer_curr_inst) {
        mock_timer_curr_state = -1;
    }
}

int hw_timer_start(int inst, int id, int us)
{
    mock_timer_curr_inst = inst;
    mock_timer_curr_id   = id;
    mock_timer_curr_period = us;

    mock_timer_curr_state = 1;

    return 0;
}

int hw_timer_stop(int inst)
{
    if (inst != mock_timer_curr_inst) {
        return -1;
    }

    mock_timer_curr_state = 0;
    return 0;
}

int hw_timer_update(int inst, int us)
{
    if (inst != mock_timer_curr_inst) {
        return -1;
    }

    if (us < 1) {
        return -1;
    }

    mock_timer_curr_period = us;

    return 0;
}

int hw_timer_duration_get(int inst)
{
    if (inst != mock_timer_curr_inst) {
        return -1;
    }

    return mock_timer_curr_duration;
}

int hw_device_setup(void)
{
    return 0;
}

