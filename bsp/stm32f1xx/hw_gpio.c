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

static uint16_t hw_gpio_used[GPIO_BANK_MAX];

static const uint32_t hw_gpio_rcc[GPIO_BANK_MAX] = {
    RCC_GPIOA, RCC_GPIOB, RCC_GPIOC, RCC_GPIOD, RCC_GPIOE, RCC_GPIOF, RCC_GPIOF
};
static const uint32_t hw_gpio_bank[GPIO_BANK_MAX] = {
    GPIOA, GPIOB, GPIOC, GPIOD, GPIOE, GPIOF, GPIOG
};

int hw_setup_gpio(void)
{
    int i;

    /* initial all pin useable */
    for (i = 0; i < GPIO_BANK_MAX; i++) {
        hw_gpio_used[i] = 0;
    }

    /* initial pin device control blocks */

    return 0;
}

int hw_gpio_use(int bank, uint16_t pins)
{
    if (bank >= GPIO_BANK_MAX || (hw_gpio_used[bank] & pins)) {
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
    if (bank >= GPIO_BANK_MAX) {
        return 0;
    }

    hw_gpio_used[bank] &= ~pins;
    if (!hw_gpio_used[bank]) {
        rcc_periph_clock_disable(hw_gpio_rcc[bank]);
    }
    return 1;
}

int hw_gpio_enable(uint8_t bank, uint8_t port, uint8_t dir)
{
    uint8_t cnf, mod;

    if (bank >= GPIO_BANK_MAX || port >= GPIO_PIN_MAX) {
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

    if (hw_gpio_use_setup(bank, 1 << port, mod, cnf)) {
        return CUPKEE_OK;
    }

    return -CUPKEE_ERESOURCE;
}

int hw_gpio_disable(uint8_t bank, uint8_t port)
{
    hw_gpio_release(bank, 1 << port);

    return 0;
}

int hw_gpio_set(uint8_t bank, uint8_t port, int v)
{
    if (bank < GPIO_BANK_MAX && port < GPIO_PIN_MAX) {
        uint32_t bank_base = hw_gpio_bank[bank];

        GPIO_BSRR(bank_base) = v ? (1 << port) : (1 << (port + 16));
        return v != 0;
    } else {
        return -CUPKEE_EINVAL;
    }
}

int hw_gpio_get(uint8_t bank, uint8_t port)
{
    if (bank < GPIO_BANK_MAX && port < GPIO_PIN_MAX) {
        uint32_t bank_base = hw_gpio_bank[bank];
        return (GPIO_IDR(bank_base) & (1 << port)) != 0;
    } else {
        return -CUPKEE_EINVAL;
    }
}

int hw_gpio_toggle(uint8_t bank, uint8_t port)
{
    if (bank < GPIO_BANK_MAX && port < GPIO_PIN_MAX) {
        uint32_t bank_base = hw_gpio_bank[bank];

        gpio_toggle(bank_base, 1 << port);
        return 0;
    } else {
        return -CUPKEE_EINVAL;
    }
}

