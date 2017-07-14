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

typedef struct hw_pwm_t {
    uint8_t dev_id;
    const hw_config_pwm_t *config;
} hw_pwm_t;

typedef struct hw_pulse_t {
    uint8_t dev_id;
    const hw_config_pulse_t *config;
} hw_pulse_t;

typedef struct hw_timer_t {
    uint8_t dev_id;
    uint8_t last;
    uint8_t high_tail;
    uint8_t update;
    int32_t duration[4]; // 4 CCR
    int32_t data[4];
    const hw_config_timer_t *config;
} hw_timer_t;

typedef struct hw_counter_t {
    uint8_t dev_id;
    uint8_t last;
    uint8_t update;
    int32_t duration[HW_CHN_MAX_COUNTER];
    int32_t data[HW_CHN_MAX_COUNTER];
    const hw_config_counter_t *config;
} hw_counter_t;

typedef union {
    hw_pwm_t        pwm;
    hw_pulse_t      pulse;
    hw_timer_t      timer;
    hw_counter_t    counter;
} hw_device_t;

static uint8_t     device_used;
static uint8_t     device_type[HW_TIMER_INSTANCE];
static uint16_t    device_status_mask[HW_TIMER_INSTANCE];
static hw_device_t device_controls[HW_TIMER_INSTANCE];
static const uint32_t device_base[] = {TIM2, TIM3, TIM4, TIM5};
static const uint32_t device_irq[] = {NVIC_TIM2_IRQ, NVIC_TIM3_IRQ, NVIC_TIM4_IRQ, NVIC_TIM5_IRQ};

static hw_pwm_t   *pwm_controls         = (hw_pwm_t *)     device_controls;
static hw_pulse_t *pulse_controls       = (hw_pulse_t *)   device_controls;
static hw_timer_t *timer_controls       = (hw_timer_t *)   device_controls;
static hw_counter_t *counter_controls   = (hw_counter_t *) device_controls;

static void device_timer_isr(int instance, uint32_t base)
{
    uint32_t status = TIM_SR(base);
    hw_timer_t *control = &timer_controls[instance];

    TIM_SR(base) = 0;
    if (status & TIM_SR_UIF) {
        control->duration[0] += 50000; // add 1 second
        control->duration[1] += 50000; // add 1 second
        control->duration[2] += 50000; // add 1 second
        control->duration[3] += 50000; // add 1 second

        // Hardware bug?
        // Disabled cc channel IF be set!
        status &= device_status_mask[instance];
    }

    if (status & TIM_SR_CC1IF) {
        uint32_t cnt = TIM_CCR1(base);

        control->data[0] = control->duration[0] + cnt;
        control->update |= 1;

        control->duration[1] = -cnt;
    } else
    if (status & TIM_SR_CC2IF) {
        uint32_t cnt = TIM_CCR2(base);

        control->data[1] = control->duration[1] + cnt;
        control->update |= 2;

        control->duration[0] = -cnt;
    }

    if (status & TIM_SR_CC3IF) {
        uint32_t cnt = TIM_CCR3(base);

        control->data[2] = control->duration[2] + cnt;
        control->update |= 4;

        control->duration[3] = -cnt;
    } else
    if (status & TIM_SR_CC4IF) {
        uint32_t cnt = TIM_CCR4(base);

        control->data[3] = control->duration[3] + cnt;
        control->update |= 8;

        control->duration[2] = -cnt;
    }
}

static void device_count_isr(int instance, uint32_t base)
{
    uint32_t status = TIM_SR(base);
    hw_counter_t *control = &counter_controls[instance];
    const hw_config_counter_t *config = control->config;
    TIM_SR(base) = 0;

    if (status & TIM_SR_UIF) {
        control->duration[0] += 50000; // add 1 second
        control->duration[1] += 50000; // add 1 second
        control->duration[2] += 50000; // add 1 second
        control->duration[3] += 50000; // add 1 second

        // Hardware bug?
        // Disabled cc channel IF be set!
        status &= device_status_mask[instance];
    }

    if (status & TIM_SR_CC1IF) {
        if ((control->duration[0] + TIM_CCR1(base)) * 20 > config->period) {
            control->duration[0] = -TIM_CCR1(base);
            control->data[0]++;
            control->update |= 1;
        }
    }
    if (status & TIM_SR_CC2IF) {
        if ((control->duration[1] + TIM_CCR2(base)) * 20 > config->period) {
            control->duration[1] = -TIM_CCR2(base);
            control->data[1]++;
            control->update |= 2;
        }
    }
    if (status & TIM_SR_CC3IF) {
        if ((control->duration[2] + TIM_CCR3(base)) * 20 > config->period) {
            control->duration[2] = -TIM_CCR3(base);
            control->data[2]++;
            control->update |= 4;
        }
    }
    if (status & TIM_SR_CC4IF) {
        if ((control->duration[3] + TIM_CCR4(base)) * 20 > config->period) {
            control->duration[3] = -TIM_CCR4(base);
            control->data[3]++;
            control->update |= 8;
        }
    }
}

void tim2_isr(void)
{
    if (device_type[0]) {
        device_timer_isr(0, TIM2);
    } else {
        device_count_isr(0, TIM2);
    }
}

void tim3_isr(void)
{
    if (device_type[1]) {
        device_timer_isr(1, TIM3);
    } else {
        device_count_isr(1, TIM3);
    }
}

void tim4_isr(void)
{
    if (device_type[2]) {
        device_timer_isr(2, TIM4);
    } else {
        device_count_isr(2, TIM4);
    }
}

void tim5_isr(void)
{
    if (device_type[3]) {
        device_timer_isr(3, TIM5);
    } else {
        device_count_isr(3, TIM5);
    }
}

static inline int pwm_period(uint16_t setting) {
    uint16_t period = setting + 1;

    // 1 - 1000 ms
    if (period > 1000) {
        period = 1000;
    }

    return period * 50 - 1;
}

static int device_alloc(int instance)
{
    if (device_used & (1 << instance)) {
        return 0;
    }
    device_used |= 1 << instance;
    return 1;
}

static void device_release(int instance)
{
    device_used &= ~(1 << instance);
    device_type[instance] = 0;
}

static int device_channel_convert(uint32_t *channel, uint8_t n, const uint8_t *seq) {
    uint32_t setting = 0;
    uint32_t i;

    for (i = 0; i < n; i++) {
        if (seq[i] > 3) {
            return -CUPKEE_EINVAL;
        }
        setting |= 1 << seq[i];
    }
    *channel = setting;
    return CUPKEE_OK;
}

static int device_channel_convert_timer(uint32_t *channel, uint8_t n, const uint8_t *seq) {
    int err = device_channel_convert(channel, n, seq);

    if (!err) {
        if ((*channel & 3) == 3 || (*channel & 12) == 12) {
            err = -CUPKEE_EINVAL;
        }
    }
    return err;
}

static void device_channel_reset(int instance, uint32_t channel)
{
    switch(instance) {
    case 0:
        hw_gpio_release(0, channel);
        rcc_periph_clock_disable(RCC_TIM2);
        break;
    case 1: {
        uint16_t chn12 = (channel & 3) << 6;
        uint16_t chn34 = (channel >> 2) & 3;
        if (chn12)
            hw_gpio_release(0, chn12);
        if (chn34)
            hw_gpio_release(1, chn34);
        rcc_periph_clock_disable(RCC_TIM3);
        break;
    }
    case 2:
        hw_gpio_release(1, channel << 6);
        rcc_periph_clock_disable(RCC_TIM4);
        break;
    case 3:
        hw_gpio_release(0, channel);
        rcc_periph_clock_disable(RCC_TIM5);
        break;
    default:
        break;
    }
}

static uint32_t device_channel_setup(int instance, uint32_t channel, uint8_t mode, uint8_t cnf)
{
    switch(instance) {
    case 0:
        if (!hw_gpio_use_setup(0, channel, mode, cnf)) {
            return 0;
        }
        rcc_periph_clock_enable(RCC_TIM2);
        break;
    case 1: {
        uint16_t chn12 = (channel & 3) << 6;
        uint16_t chn34 = (channel >> 2) & 3;
        if (chn12) {
            if (!hw_gpio_use_setup(0, chn12, mode, cnf)) {
                return 0;
            }
        }
        if (chn34) {
            if (!hw_gpio_use_setup(1, chn34, mode, cnf)) {
                hw_gpio_release(1, chn12);
                return 0;
            }
        }
        rcc_periph_clock_enable(RCC_TIM3);
        break;
    }
    case 2:
        if (!hw_gpio_use_setup(1, channel << 6, mode, cnf)) {
            return 0;
        }
        rcc_periph_clock_enable(RCC_TIM4);
        break;
    case 3:
        if (!hw_gpio_use_setup(0, channel, mode, cnf)) {
            return 0;
        }
        rcc_periph_clock_enable(RCC_TIM5);
        break;
    default:
        return 0;
    }

    return device_base[instance];
}

static void device_reset(int instance, uint32_t channel)
{
    uint32_t base = device_base[instance];

    nvic_disable_irq(device_irq[instance]);
    device_channel_reset(instance, channel);

    TIM_DIER(base) = 0;
    TIM_CCER(base) = 0;
    TIM_CR2(base) = 0;
    TIM_CR1(base) = 0;
}

static void device_setup_out(uint32_t base, uint32_t channel, uint8_t polarity)
{
    uint32_t enable = 0;
    TIM_CCMR1(base) = 0;
    TIM_CCMR2(base) = 0;

    if (channel & 1) {
        TIM_CCMR1(base) |= TIM_CCMR1_OC1M_PWM1 | TIM_CCMR1_OC1PE;
        TIM_CCR1(base) = 0;
        enable |= TIM_CCER_CC1P | TIM_CCER_CC1E;
    }
    if (channel & 2) {
        TIM_CCMR1(base) |= TIM_CCMR1_OC2M_PWM1 | TIM_CCMR1_OC2PE;
        TIM_CCR2(base) = 0;
        enable |= TIM_CCER_CC2P | TIM_CCER_CC2E;
    }
    if (channel & 4) {
        TIM_CCMR2(base) |= TIM_CCMR2_OC3M_PWM1 | TIM_CCMR2_OC3PE;
        TIM_CCR3(base) = 0;
        enable |= TIM_CCER_CC3P | TIM_CCER_CC3E;
    }
    if (channel & 8) {
        TIM_CCMR2(base) |= TIM_CCMR2_OC4M_PWM1 | TIM_CCMR2_OC4PE;
        TIM_CCR4(base) = 0;
        enable |= TIM_CCER_CC4P | TIM_CCER_CC4E;
    }

    if (polarity == DEVICE_OPT_POLARITY_NEGATIVE) {
        enable &= ~(TIM_CCER_CC1P | TIM_CCER_CC2P | TIM_CCER_CC3P | TIM_CCER_CC4P);
    }

    TIM_CCER(base) = enable;
    TIM_CR1(base) = TIM_CR1_CKD_CK_INT | TIM_CR1_CMS_EDGE;
}

static void device_setup_timer(int instance, uint32_t channel)
{
    uint32_t base = device_base[instance];
    uint32_t ccr_enable = 0, isr_enable = 0;

    TIM_CR1(base) = TIM_CR1_CKD_CK_INT | TIM_CR1_CMS_EDGE | TIM_CR1_DIR_UP;
    TIM_PSC(base) = 1440;     // 20uS pre tick
    TIM_ARR(base) = 50000;
    TIM_EGR(base) = TIM_EGR_UG; // update to real register

    device_status_mask[instance] = 0;
    TIM_CCER(base) = 0;
    if (channel & 3) {
        if (channel & 1) {
            TIM_CCMR1(base) = TIM_CCMR1_CC1S_IN_TI1 | TIM_CCMR1_CC2S_IN_TI1 |
                              TIM_CCMR1_IC1F_CK_INT_N_8;
        } else {
            TIM_CCMR1(base) = TIM_CCMR1_CC1S_IN_TI2 | TIM_CCMR1_CC2S_IN_TI2 |
                              TIM_CCMR1_IC2F_CK_INT_N_8;
        }
        ccr_enable = TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC2P;
        isr_enable = TIM_DIER_CC1IE | TIM_DIER_CC2IE;
        device_status_mask[instance] |= 6;
    } else {
        TIM_CCMR1(base) = 0;
    }

    if (channel & 12) {
        if (channel & 4) {
            TIM_CCMR2(base) = TIM_CCMR2_CC3S_IN_TI3 | TIM_CCMR2_CC4S_IN_TI3 |
                              TIM_CCMR2_IC3F_CK_INT_N_8;
        } else {
            TIM_CCMR2(base) = TIM_CCMR2_CC3S_IN_TI4 | TIM_CCMR2_CC4S_IN_TI4 |
                              TIM_CCMR2_IC4F_CK_INT_N_8;
        }
        ccr_enable |= TIM_CCER_CC3E | TIM_CCER_CC4E | TIM_CCER_CC4P;
        isr_enable |= TIM_DIER_CC3IE | TIM_DIER_CC4IE;
        device_status_mask[instance] |= 0x18;
    } else {
        TIM_CCMR2(base) = 0;
    }

    nvic_enable_irq(device_irq[instance]);

    TIM_SR(base) = 0;
    TIM_DIER(base) = isr_enable | TIM_DIER_UIE;
    TIM_CCER(base) = ccr_enable;
    TIM_CR1(base) |= TIM_CR1_CEN;
}

static int device_setup_counter(int instance, uint32_t channel)
{
    uint32_t base, enable = 0;
    hw_counter_t *control = &counter_controls[instance];
    const hw_config_counter_t *config = control->config;

    base = device_channel_setup(instance, channel, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT);
    if (!base) {
        return -CUPKEE_ERESOURCE;
    }

    TIM_CCMR1(base) = 0;
    TIM_CCMR2(base) = 0;
    device_status_mask[instance] = 0;
    if (channel & 1) {
        TIM_CCMR1(base) |= TIM_CCMR1_IC1F_CK_INT_N_8 | 1;
        enable |= TIM_CCER_CC1P | TIM_CCER_CC1E;
        device_status_mask[instance] |= 0x2;
    }
    if (channel & 2) {
        TIM_CCMR1(base) |= TIM_CCMR1_IC2F_CK_INT_N_8 | (1 << 8);
        enable |= TIM_CCER_CC2P | TIM_CCER_CC2E;
        device_status_mask[instance] |= 0x4;
    }
    if (channel & 4) {
        TIM_CCMR2(base) |= TIM_CCMR2_IC3F_CK_INT_N_8 | 1;
        enable |= TIM_CCER_CC3P | TIM_CCER_CC3E;
        device_status_mask[instance] |= 0x8;
    }
    if (channel & 8) {
        TIM_CCMR2(base) |= TIM_CCMR2_IC4F_CK_INT_N_8 | (1 << 8);
        enable |= TIM_CCER_CC4P | TIM_CCER_CC4E;
        device_status_mask[instance] |= 0x10;
    }
    if (config->polarity != DEVICE_OPT_POLARITY_NEGATIVE) {
        enable &= ~(TIM_CCER_CC1P | TIM_CCER_CC2P | TIM_CCER_CC3P | TIM_CCER_CC4P);
    }
    TIM_CCER(base) = enable;
    TIM_DIER(base) = ((channel & 0x0f) << 1) | 1;
    TIM_CR1(base) = TIM_CR1_CKD_CK_INT | TIM_CR1_CMS_EDGE;

    TIM_CNT(base) = 0;
    TIM_PSC(base) = 1440;           // 20uS pre tick
    TIM_ARR(base) = 50000;
    TIM_EGR(base) = TIM_EGR_UG;     // update PSC,ARR to shadow register

    TIM_SR(base) = 0;
    nvic_enable_irq(device_irq[instance]);

    TIM_CR1(base) |= TIM_CR1_ARPE | TIM_CR1_CEN;

    return CUPKEE_OK;
}

static uint32_t device_channel_get(int instance, uint8_t chn)
{
    uint32_t base = device_base[instance];

    switch (chn) {
    case 0: return TIM_CCR1(base);
    case 1: return TIM_CCR2(base);
    case 2: return TIM_CCR3(base);
    case 3: return TIM_CCR4(base);
    default: return 0;
    }
}

static void device_channel_set(int instance, uint8_t chn, uint32_t data)
{
    uint32_t base = device_base[instance];

    switch (chn) {
    case 0: TIM_CCR1(base) = data; break;
    case 1: TIM_CCR2(base) = data; break;
    case 2: TIM_CCR3(base) = data; break;
    case 3: TIM_CCR4(base) = data; break;
    default: break;
    }
}

static void device_channel_set_all(int instance, uint32_t channel, uint32_t data)
{
    uint32_t base = device_base[instance];

    if (channel & 1) TIM_CCR1(base) = data;
    if (channel & 2) TIM_CCR2(base) = data;
    if (channel & 4) TIM_CCR3(base) = data;
    if (channel & 8) TIM_CCR4(base) = data;
}

static void pwm_reset(int instance)
{
    hw_pwm_t *control = &pwm_controls[instance];
    const hw_config_pwm_t *config = control->config;
    uint32_t channel;

    if (!device_channel_convert(&channel, config->chn_num, config->chn_seq)) {
        device_reset(instance, channel);
    }

    control->dev_id = DEVICE_ID_INVALID;
    control->config = NULL;
}

static int pwm_setup(int instance, uint8_t dev_id, const hw_config_t *conf)
{
    hw_pwm_t *control = &pwm_controls[instance];
    const hw_config_pwm_t *config = (const hw_config_pwm_t*)conf;
    uint32_t base, channel;
    int err = CUPKEE_OK;

    /* hardware setup here */
    err = device_channel_convert(&channel, config->chn_num, config->chn_seq);
    if (err) {
        goto DO_END;
    }

    base = device_channel_setup(instance, channel, GPIO_MODE_OUTPUT_10_MHZ,
                                                GPIO_CNF_OUTPUT_ALTFN_PUSHPULL);
    if (!base) {
        err = -CUPKEE_ERESOURCE;
        goto DO_END;
    }

    device_setup_out(base, channel, config->polarity);

    TIM_PSC(base) = 1440;  // 20uS
    TIM_ARR(base) = pwm_period(config->period);
    TIM_CNT(base) = 0;
    TIM_EGR(base) = TIM_EGR_UG; // update to real register

    TIM_CR1(base) |= TIM_CR1_ARPE | TIM_CR1_CEN;

    control->dev_id = dev_id;
    control->config = config;

DO_END:
    return err;
}

static int pwm_get(int instance, int off, uint32_t *data)
{
    hw_pwm_t *control = &pwm_controls[instance];
    const hw_config_pwm_t *config = control->config;
    uint32_t duty, max = pwm_period(config->period);

    if (off < 0 || off >= config->chn_num) {
        return 0;
    }

    duty = device_channel_get(instance, config->chn_seq[off]);
    *data = 1000 - (duty * 1000 / max);

    return 1;
}

static int pwm_set(int instance, int off, uint32_t data)
{
    hw_pwm_t *control = &pwm_controls[instance];
    const hw_config_pwm_t *config = control->config;
    uint32_t max = pwm_period(config->period);

    if (data > 1000) {
        data = 0;
    } else {
        data = (max * (1000 - data)) / 1000;
    }

    if (off < 0) {
        uint32_t channel;
        if (!device_channel_convert(&channel, config->chn_num, config->chn_seq)) {
            device_channel_set_all(instance, channel, data);
        } else {
            return 0;
        }
    } else
    if (off < control->config->chn_num) {
        device_channel_set(instance, config->chn_seq[off], data);
    } else {
        return 0;
    }

    return 1;
}

static int pwm_size(int instance)
{
    hw_pwm_t *control = &pwm_controls[instance];

    return control->config->chn_num;
}

static void pulse_reset(int instance)
{
    hw_pulse_t *control = &pulse_controls[instance];
    const hw_config_pulse_t *config = control->config;
    uint32_t channel;

    if (!device_channel_convert(&channel, config->chn_num, config->chn_seq)) {
        device_reset(instance, channel);
    }

    control->dev_id = DEVICE_ID_INVALID;
    control->config = NULL;
}

static int pulse_setup(int instance, uint8_t dev_id, const hw_config_t *conf)
{
    hw_pulse_t *control = &pulse_controls[instance];
    const hw_config_pulse_t *config = (const hw_config_pulse_t*)conf;
    uint32_t base, channel;
    int err = CUPKEE_OK;

    /* hardware setup here */
    err = device_channel_convert(&channel, config->chn_num, config->chn_seq);
    if (err) {
        goto DO_END;
    }

    base = device_channel_setup(instance, channel, GPIO_MODE_OUTPUT_10_MHZ,
                                                GPIO_CNF_OUTPUT_ALTFN_PUSHPULL);
    if (!base) {
        err = -CUPKEE_ERESOURCE;
        goto DO_END;
    }

    device_setup_out(base, channel, config->polarity);

    control->dev_id = dev_id;
    control->config = config;

DO_END:
    return err;
}

static int pulse_set(int instance, int off, uint32_t data)
{
    hw_pulse_t *control = &pulse_controls[instance];
    const hw_config_pulse_t *config = control->config;
    uint32_t base = device_base[instance];

    if (TIM_CR1(base) & TIM_CR1_CEN) {
        // Pulse busy
        return 0;
    }

    data /= 10;         //
    if (data < 1) {
        data = 1;       // 10us is bottom limit
    } else
    if (data > 50000) {
        data = 50000;   // 500ms is upper limit
    }

    if (off < 0) {
        uint32_t channel;
        if (!device_channel_convert(&channel, config->chn_num, config->chn_seq)) {
            device_channel_set_all(instance, channel, 1);
        } else {
            return 0;
        }
    } else
    if (off < control->config->chn_num) {
        device_channel_set(instance, config->chn_seq[off], 1);
    } else {
        return 0;
    }

    TIM_PSC(base) = 720;        // 10uS pre tick
    TIM_ARR(base) = data + 1;
    TIM_CNT(base) = 0;
    TIM_EGR(base) = TIM_EGR_UG;
    TIM_CR1(base) |= TIM_CR1_ARPE | TIM_CR1_OPM | TIM_CR1_CEN;

    return 1;
}

static int pulse_size(int instance)
{
    hw_pulse_t *control = &pulse_controls[instance];

    return control->config->chn_num;
}

static void hw_timer_reset(int instance)
{
    hw_timer_t *control = &timer_controls[instance];
    const hw_config_timer_t *config = control->config;
    uint32_t channel;

    if (!device_channel_convert(&channel, config->chn_num, config->chn_seq)) {
        device_reset(instance, channel);
    }

    control->dev_id = DEVICE_ID_INVALID;
    control->config = NULL;
}

static int timer_setup(int instance, uint8_t dev_id, const hw_config_t *conf)
{
    hw_timer_t *control = &timer_controls[instance];
    const hw_config_timer_t *config = (const hw_config_timer_t*)conf;
    uint32_t base, channel;
    int err = CUPKEE_OK, i;

    control->update = 0;
    for (i = 0; i < HW_CHN_MAX_TIMER; i++) {
        control->data[i] = 0;
        control->duration[i] = 0;
    }

    if (config->chn_num > 1 && config->chn_seq[1] > 1) {
        // 2 channels & high channel in tail of sequence
        control->high_tail = 1;
    } else {
        control->high_tail = 0;
    }

    /* hardware setup here */
    err = device_channel_convert_timer(&channel, config->chn_num, config->chn_seq);
    if (err) {
        goto DO_END;
    }

    base = device_channel_setup(instance, channel, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT);
    if (!base) {
        err = -CUPKEE_ERESOURCE;
        goto DO_END;
    }

    control->dev_id = dev_id;
    control->config = config;

    device_setup_timer(instance, channel);

DO_END:
    return err;
}

static void timer_poll(int instance)
{
    hw_timer_t *control = &timer_controls[instance];

    if (control->update) {
        const hw_config_timer_t *config = control->config;
        uint8_t update = control->update;
        int trigger = 0;

        if (update & 3) {
            if (control->update & 1) {
                if (config->polarity != DEVICE_OPT_POLARITY_POSITIVE) {
                    trigger = 1;
                }
            } else {
                if (config->polarity != DEVICE_OPT_POLARITY_NEGATIVE) {
                    control->data[0] = control->data[1]; // Valid data always in data[0]
                    trigger = 1;
                }
            }
            control->update &= ~3;

            if (trigger) {
                if (config->chn_num > 1 && !control->high_tail) {
                    control->last = 1;
                } else {
                    control->last = 0;
                }
                cupkee_event_post_device_data(control->dev_id);
            }
        } else {
            if (control->update & 4) {
                if (config->polarity != DEVICE_OPT_POLARITY_POSITIVE) {
                    trigger = 1;
                }
            } else {
                if (config->polarity != DEVICE_OPT_POLARITY_NEGATIVE) {
                    control->data[2] = control->data[3]; // Valid data always in data[2]
                    trigger = 1;
                }
            }
            control->update = 0;

            if (trigger) {
                if (config->chn_num > 1 && control->high_tail) {
                    control->last = 1;
                } else {
                    control->last = 0;
                }
                cupkee_event_post_device_data(control->dev_id);
            }
        }
    }
}

static int timer_get(int instance, int off, uint32_t *data)
{
    hw_timer_t *control = &timer_controls[instance];
    const hw_config_timer_t *config = control->config;

    if (off < 0) {
        *data = control->last;
    } else
    if (off < config->chn_num) {
        off = config->chn_seq[off] > 1 ? 2 : 0;
        *data = control->data[off] * 20;
    } else {
        return 0;
    }

    return 1;
}

static int timer_size(int instance)
{
    hw_timer_t *control = &timer_controls[instance];

    return control->config->chn_num;
}

static void counter_reset(int instance)
{
    hw_counter_t *control = &counter_controls[instance];
    const hw_config_counter_t *config = control->config;
    uint32_t channel;

    if (!device_channel_convert(&channel, config->chn_num, config->chn_seq)) {
        device_reset(instance, channel);
    }

    control->dev_id = DEVICE_ID_INVALID;
    control->config = NULL;
}

static int counter_setup(int instance, uint8_t dev_id, const hw_config_t *conf)
{
    hw_counter_t *control = &counter_controls[instance];
    const hw_config_counter_t *config = (const hw_config_counter_t*)conf;
    uint32_t channel;
    int err = CUPKEE_OK, i;

    control->update = 0;
    for (i = 0; i < HW_CHN_MAX_COUNTER; i++) {
        control->data[i] = 0;
        control->duration[i] = 0;
    }

    /* hardware setup here */
    err = device_channel_convert(&channel, config->chn_num, config->chn_seq);
    if (err) {
        goto DO_END;
    }

    err = device_setup_counter(instance, channel);
    if (!err) {
        control->dev_id = dev_id;
        control->config = config;
    }

DO_END:
    return err;
}

static void counter_poll(int instance)
{
    hw_counter_t *control = &counter_controls[instance];

    if (control->update) {
        const hw_config_counter_t *config = control->config;
        int i;

        for (i = 0; i < config->chn_num; i++) {
            uint8_t ch = config->chn_seq[i];
            uint8_t x  = 1 << ch;

            if (control->update & x) {
                control->update &= ~x;
                control->last = i;
                cupkee_event_post_device_data(control->dev_id);

                break;
            }
        }
    }
}

static int counter_get(int instance, int off, uint32_t *data)
{
    hw_counter_t *control = &counter_controls[instance];
    const hw_config_counter_t *config = control->config;

    if (off < 0) {
        *data = control->last;
    } else
    if (off < config->chn_num){
        *data = control->data[off];
    } else {
        return 0;
    }

    return 1;
}

static int counter_size(int instance)
{
    hw_counter_t *control = &counter_controls[instance];

    return control->config->chn_num;
}

static const hw_driver_t pwm_driver = {
    .release = device_release,
    .reset   = pwm_reset,
    .setup   = pwm_setup,

    .get  = pwm_get,
    .set  = pwm_set,
    .size = pwm_size
};

static const hw_driver_t pulse_driver = {
    .release = device_release,
    .reset   = pulse_reset,
    .setup   = pulse_setup,

    .set  = pulse_set,
    .size = pulse_size
};

static const hw_driver_t timer_driver = {
    .release = device_release,
    .reset   = hw_timer_reset,
    .setup   = timer_setup,
    .poll    = timer_poll,

    .get  = timer_get,
    .size = timer_size
};

static const hw_driver_t counter_driver = {
    .release = device_release,
    .reset   = counter_reset,
    .setup   = counter_setup,
    .poll    = counter_poll,

    .get  = counter_get,
    .size = counter_size
};

const hw_driver_t *hw_request_pwm(int instance)
{
    if (instance >= HW_TIMER_INSTANCE || !device_alloc(instance)) {
        return NULL;
    }

    pwm_controls[instance].dev_id = DEVICE_ID_INVALID;
    pwm_controls[instance].config = NULL;

    return &pwm_driver;
}

const hw_driver_t *hw_request_pulse(int instance)
{
    if (instance >= HW_TIMER_INSTANCE || !device_alloc(instance)) {
        return NULL;
    }

    pulse_controls[instance].dev_id = DEVICE_ID_INVALID;
    pulse_controls[instance].config = NULL;

    return &pulse_driver;
}

const hw_driver_t *hw_request_timer(int instance)
{
    if (instance >= HW_TIMER_INSTANCE || !device_alloc(instance)) {
        return NULL;
    }

    device_type[instance] = 1;
    timer_controls[instance].dev_id = DEVICE_ID_INVALID;
    timer_controls[instance].config = NULL;

    return &timer_driver;
}

const hw_driver_t *hw_request_counter(int instance)
{
    if (instance >= HW_TIMER_INSTANCE || !device_alloc(instance)) {
        return NULL;
    }

    device_type[instance] = 0;
    counter_controls[instance].dev_id = DEVICE_ID_INVALID;
    counter_controls[instance].config = NULL;

    return &counter_driver;
}

void hw_setup_timer(void)
{
    device_used = 0;
}

