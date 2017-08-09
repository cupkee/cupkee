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

#define HW_TIMER_NUM    4

typedef struct hw_timer_t {
    uint8_t inused;
    int     timer_id;
} hw_timer_t;

static hw_timer_t  device_controls[HW_TIMER_NUM];
static const uint32_t device_base[] = {TIM2, TIM3, TIM4, TIM5};
static const uint32_t device_rcc[] = {RCC_TIM2, RCC_TIM3, RCC_TIM4, RCC_TIM5};
static const uint32_t device_irq[] = {NVIC_TIM2_IRQ, NVIC_TIM3_IRQ, NVIC_TIM4_IRQ, NVIC_TIM5_IRQ};

static inline hw_timer_t *hw_device(int id) {
    if (0 >= id && id < HW_TIMER_NUM) {
        return &device_controls[id];
    } else {
        return NULL;
    }
}

void hw_setup_timer(void)
{
    int i;

    for (i = 0; i < HW_TIMER_NUM; i++) {
        device_controls[i].inused = 0;
        device_controls[i].timer_id = -1;
    }
}

int hw_timer_alloc(void)
{
    int inst;

    for (inst = 0; inst < HW_TIMER_NUM; inst++) {
        hw_timer_t *timer = hw_device(inst);

        if (!timer->inused) {
            timer->inused = 1;
            rcc_periph_clock_enable(device_rcc[inst]);

            return inst;
        }
    }

    return -1;
}

void hw_timer_release(int inst)
{
    hw_timer_t *timer = hw_device(inst);

    if (timer && timer->inused) {
        hw_timer_stop(inst);
        rcc_periph_clock_disable(device_rcc[inst]);
        timer->inused = 0;
    }
}

int hw_timer_start(int inst, int id, int us)
{
    hw_timer_t *timer = hw_device(inst);
    uint32_t base, arr;

    if (!timer || !timer->inused) {
        return -CUPKEE_EINVAL;
    }
    timer->timer_id = id;
    arr = us / 20;

    if (arr < 1) {
        arr = 1;
    }

    base = device_base[inst];

    TIM_SR(base) = 0;
    TIM_CNT(base) = 0;
    TIM_PSC(base) = 1440;
    TIM_ARR(base) = 50000;
    TIM_DIER(base) = TIM_DIER_UIE;

    nvic_enable_irq(device_irq[inst]);
    TIM_CR1(base) = TIM_CR1_CEN;

    return 0;
}

int hw_timer_stop(int inst)
{
    hw_timer_t *timer = hw_device(inst);
    uint32_t base;

    if (!timer || !timer->inused) {
        return -CUPKEE_EINVAL;
    }

    nvic_enable_irq(device_irq[inst]);

    base = device_base[inst];
    TIM_CR1(base) &= ~TIM_CR1_CEN;

    return 0;
}

int hw_timer_update(int inst, int us)
{
    hw_timer_t *timer = hw_device(inst);
    uint32_t base, arr;

    if (!timer || !timer->inused) {
        return -CUPKEE_EINVAL;
    }
    base = device_base[inst];

    arr = us / 20;
    TIM_ARR(base) = arr > 0 ? arr : 1;

    return 0;
}

int hw_timer_duration_get(int inst)
{
    hw_timer_t *timer = hw_device(inst);
    uint32_t base;

    if (!timer || !timer->inused) {
        return -CUPKEE_EINVAL;
    }

    base = device_base[inst];
    return TIM_CNT(base) * 20;
}

static inline void timer_isr(int x) {
    TIM_SR(device_base[x]) &= ~TIM_SR_UIF;

    hw_led_toggle();
    if (device_controls[x].inused) {
        cupkee_timer_rewind(device_controls[x].timer_id);
    }
}

void tim2_isr(void)
{
    timer_isr(0);
}

void tim3_isr(void)
{
    timer_isr(1);
}

void tim4_isr(void)
{
    timer_isr(2);
}

void tim5_isr(void)
{
    timer_isr(3);
}


