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

#define BANK_MAX                7
#define BANK_MASK               7

#define PIN_MAX                 16
#define PIN_MASK                15

#define PIN_MAP_MAX             32


static uint8_t  pin_maps[PIN_MAP_MAX];
static uint16_t hw_gpio_used[BANK_MAX];

static const uint32_t hw_gpio_rcc[BANK_MAX] = {
    RCC_GPIOA, RCC_GPIOB, RCC_GPIOC, RCC_GPIOD, RCC_GPIOE, RCC_GPIOF, RCC_GPIOF
};
static const uint32_t hw_gpio_bank[BANK_MAX] = {
    GPIOA, GPIOB, GPIOC, GPIOD, GPIOE, GPIOF, GPIOG
};

static inline int map_is_valid(uint8_t map)
{
    return map != 0xff;
}

static inline uint8_t map_gpio(int bank, int pin)
{
    return (bank << 4) | (pin & PIN_MASK);
}

static inline int map_bank(uint8_t map) {
    return (map >> 4) & BANK_MASK;
}

static inline int map_pin(uint8_t map) {
    return map & PIN_MASK;
}

static inline void map_set_mode(uint8_t map, uint8_t mode, uint8_t cnf) {
    uint32_t bank_base = hw_gpio_bank[map_bank(map)];
    int pins = 1 << map_pin(map);

    gpio_set_mode(bank_base, mode, cnf, pins);
}

static inline void map_write(uint8_t map, int v) {
    uint32_t bank_base = hw_gpio_bank[map_bank(map)];
    int pin = map_pin(map);

    if (v)
        GPIO_BSRR(bank_base) = 1 << pin;
    else
        GPIO_BSRR(bank_base) = 1 << (pin + 16);
}

static inline int map_read(uint8_t map) {
    uint32_t bank_base = hw_gpio_bank[(map >> 4) & BANK_MAX];
    int pin = map & PIN_MASK;

    return (GPIO_IDR(bank_base) & (1 << pin)) != 0;
}

static inline void map_toggle(uint8_t map) {
    uint32_t bank_base = hw_gpio_bank[(map >> 4) & BANK_MAX];
    int pin = map & PIN_MASK;

    gpio_toggle(bank_base, 1 << pin);
}

static inline void map_release(uint8_t map) {
    if (map_is_valid(map)) {
        hw_gpio_release(map_bank(map), 1 << map_pin(map));
    }
}

int hw_setup_gpio(void)
{
    int i;

    /* initial all pin useable */
    for (i = 0; i < BANK_MAX; i++) {
        hw_gpio_used[i] = 0;
    }

    /* initial all pin map invalid */
    for (i = 0; i < PIN_MAP_MAX; i++) {
        pin_maps[i] = 0xff;
    }

    /* initial pin device control blocks */

    return 0;
}

int hw_pin_map(int id, uint8_t bank, uint8_t pin, uint8_t dir)
{
    uint8_t cnf, mod;

    if ((unsigned)id >= PIN_MAP_MAX || bank >= BANK_MAX || pin >= PIN_MAX) {
        return -CUPKEE_EINVAL;
    }

    switch (dir) {
    case HW_DIR_IN:
        mod = GPIO_MODE_INPUT;
        cnf = GPIO_CNF_INPUT_PULL_UPDOWN;
        break;
    case HW_DIR_OUT:
        mod = GPIO_MODE_OUTPUT_10_MHZ;
        cnf = GPIO_CNF_OUTPUT_PUSHPULL;
        break;
    case HW_DIR_DUPLEX:
        mod = GPIO_MODE_OUTPUT_2_MHZ;
        cnf = GPIO_CNF_OUTPUT_OPENDRAIN;
        break;
    default:
        return -CUPKEE_EINVAL;
    }

    if (hw_gpio_use_setup(bank, 1 << pin, mod, cnf)) {
        uint8_t map = map_gpio(bank, pin);
        uint8_t old = pin_maps[id];

        map_release(old);
        pin_maps[id] = map;

        return CUPKEE_OK;
    }

    return -CUPKEE_ERESOURCE;
}

void hw_pin_set(int id, int v)
{
    if ((unsigned)id < PIN_MAP_MAX && map_is_valid(pin_maps[id])) {
        map_write(pin_maps[id], v);
    }
}

int  hw_pin_get(int id)
{
    if ((unsigned)id < PIN_MAP_MAX && map_is_valid(pin_maps[id])) {
        return map_read(pin_maps[id]);
    } else {
        return -CUPKEE_EINVAL;
    }
}

void hw_pin_toggle(int id)
{
    if ((unsigned)id < PIN_MAP_MAX && map_is_valid(pin_maps[id])) {
        map_toggle(pin_maps[id]);
    }
}

int hw_gpio_use(int bank, uint16_t pins)
{
    if (bank >= BANK_MAX || (hw_gpio_used[bank] & pins)) {
        return CUPKEE_FALSE;
    }

    if (!hw_gpio_used[bank]) {
        rcc_periph_clock_enable(hw_gpio_rcc[bank]);
    }

    hw_gpio_used[bank] |= pins;

    return CUPKEE_TRUE;
}

int hw_gpio_use_setup(int bank, uint16_t pins, uint8_t mode, uint8_t cnf)
{
    uint32_t bank_base = hw_gpio_bank[bank];

    if (hw_gpio_use(bank, pins)) {
        gpio_set_mode(bank_base, mode, cnf, pins);
        return CUPKEE_TRUE;
    }
    return CUPKEE_FALSE;
}

int hw_gpio_release(int bank, uint16_t pins)
{
    if (bank >= BANK_MAX) {
        return 0;
    }

    hw_gpio_used[bank] &= ~pins;
    if (!hw_gpio_used[bank]) {
        rcc_periph_clock_disable(hw_gpio_rcc[bank]);
    }
    return 1;
}

