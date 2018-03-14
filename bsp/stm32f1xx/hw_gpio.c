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

#define PIN_MASK        0x3F

typedef struct hw_gpio_isr_t {
    uint8_t pin;
    uint8_t bank;
} hw_gpio_isr_t;

static uint16_t hw_gpio_used[GPIO_BANK_MAX];
static hw_gpio_isr_t hw_gpio_isr_info[GPIO_PIN_MAX];

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

    /* initial all port isr useable */
    for (i = 0; i < 16; i++) {
        hw_gpio_isr_info[i].pin = ~PIN_MASK;
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
        cnf = GPIO_CNF_INPUT_FLOAT;
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
        return 1;
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

int hw_gpio_listen(uint8_t bank, uint8_t port, uint8_t events, uint8_t which)
{
    if (bank < GPIO_BANK_MAX && port < GPIO_PIN_MAX && which <= PIN_MASK) {
        hw_gpio_isr_t *isr_info = &hw_gpio_isr_info[port];
        uint32_t exti;
        enum exti_trigger_type type;

        //console_log("exti %u:%u %u\r\n", bank, port, isr_info->pin);
        if ((isr_info->pin & PIN_MASK)) {
            return -CUPKEE_ERESOURCE;
        }

        isr_info->pin = which;
        isr_info->bank = bank;

        exti = 1 << port;
        rcc_periph_clock_enable(RCC_AFIO);
        if (port < 5) {
            nvic_enable_irq(NVIC_EXTI0_IRQ + port);
        } else
        if (port < 10) {
            nvic_enable_irq(NVIC_EXTI9_5_IRQ);
        } else {
            nvic_enable_irq(NVIC_EXTI15_10_IRQ);
        }

        if (events == CUPKEE_EVENT_PIN_RISING) {
            type = EXTI_TRIGGER_RISING;
        } else
        if (events == CUPKEE_EVENT_PIN_FALLING) {
            type = EXTI_TRIGGER_FALLING;
        } else {
            type = EXTI_TRIGGER_BOTH;
        }

        exti_select_source(exti, hw_gpio_bank[bank]);
        exti_set_trigger(exti, type);
        exti_enable_request(exti);

        //console_log("exti %u:%u enable\r\n", bank, port);

        return 0;
    } else {
        return -CUPKEE_EINVAL;
    }
}

int hw_gpio_ignore(uint8_t bank, uint8_t port)
{
    if (bank < GPIO_BANK_MAX && port < GPIO_PIN_MAX) {
        hw_gpio_isr_t *isr_info = &hw_gpio_isr_info[port];
        int i;

        if ((isr_info->pin & PIN_MASK) == 0) {
            return 0;
        }

        if (port < 5) {
            nvic_disable_irq(NVIC_EXTI0_IRQ + port);
        } else
        if (port < 10) {
            nvic_disable_irq(NVIC_EXTI9_5_IRQ);
        } else {
            nvic_disable_irq(NVIC_EXTI15_10_IRQ);
        }

        exti_disable_request(1 << port);
        isr_info->pin = ~PIN_MASK;

        for (i = 0; i < GPIO_PIN_MAX; i++) {
            if (hw_gpio_isr_info[i].pin & PIN_MASK) {
                return 0;
            }
        }

        rcc_periph_clock_enable(RCC_AFIO);

        return 0;
    } else {
        return -CUPKEE_EINVAL;
    }
}

static inline void exti_isr_handler(int i)
{
    hw_gpio_isr_t *isr_info = &hw_gpio_isr_info[i];

    cupkee_event_post_pin(isr_info->pin, (GPIO_IDR(hw_gpio_bank[isr_info->bank]) >> i) & 1);
}

void exti0_isr(void)
{
    exti_isr_handler(0);
    EXTI_PR = EXTI0;
}

void exti1_isr(void)
{
    exti_isr_handler(1);
    EXTI_PR = EXTI1;
}

void exti2_isr(void)
{
    exti_isr_handler(2);
    EXTI_PR = EXTI2;
}

void exti3_isr(void)
{
    exti_isr_handler(3);
    EXTI_PR = EXTI3;
}

void exti4_isr(void)
{
    exti_isr_handler(4);
    EXTI_PR = EXTI4;
}

void exti9_5_isr(void)
{
    uint16_t bits = EXTI_PR & 0x3E0;
    int i;

    for (i = 5; i < 10; i++) {
        if (bits & (1 << i)) {
            exti_isr_handler(i);
        }
    }
    EXTI_PR = bits;
}

void exti15_10_isr(void)
{
    uint16_t bits = EXTI_PR & 0xFC00;
    int i;

    for (i = 10; i < 16; i++) {
        if (bits & (1 << i)) {
            exti_isr_handler(i);
        }
    }
    EXTI_PR = bits;
}

