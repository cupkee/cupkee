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

#define ADC_IDLE        0
#define ADC_READY       1
#define ADC_BUSY        2      // work in process
#define ADC_INVALID     0xffff

typedef struct hw_adc_t {
    uint8_t inused;
    uint8_t dev_id;
    uint8_t state;
    uint8_t current;
    uint8_t changed;
    uint8_t reserved;
    uint16_t sleep;
    uint16_t data[HW_CHN_MAX_ADC];
    const hw_config_adc_t *config;
} hw_adc_t;

static hw_adc_t adc_controls[HW_INSTANCES_ADC];
const  uint8_t adc_chn_port[] = {
    0, 0, 0, 0,                     // channel 0 - 3
    0, 0, 0, 0,                     // channel 4 - 7
    1, 1,                           // channel 8 - 9
    2, 2, 2, 2,                     // channel 10 - 13
    2, 2,                           // channel 14 - 15
};
const  uint16_t adc_chn_pin[] = {
    0x0001, 0x0002, 0x0004, 0x0008, // channel 0 - 3
    0x0010, 0x0020, 0x0040, 0x0080, // channel 4 - 7
    0x0010, 0x0020,                 // channel 8 - 9
    0x0010, 0x0020, 0x0040, 0x0080, // channel 10 - 13
    0x0010, 0x0020,                 // channel 14 - 15
};

static inline int hw_adc_chn_setup(uint8_t chn)
{
    if (chn < 16) {
        int port     = adc_chn_port[chn];
        uint16_t pin = adc_chn_pin[chn];

        if (!hw_gpio_use_setup(port, pin, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG)) {
            return CUPKEE_ERESOURCE;
        }
    } else
    if (chn == 16) {
        return CUPKEE_EIMPLEMENT;
    } else
    if (chn == 17) {
        return CUPKEE_EIMPLEMENT;
    } else {
        return CUPKEE_EINVAL;
    }

    return CUPKEE_OK;
}

static inline void hw_adc_chn_reset(uint8_t chn) {
    if (chn < 16) {
        int port     = adc_chn_port[chn];
        uint16_t pin = adc_chn_pin[chn];

        hw_gpio_release(port, pin);
    }
}

static int channel_setup(uint8_t num, const uint8_t *seq)
{
    int i, err;

    for (i = 0; i < num; i++) {
        err = hw_adc_chn_setup(seq[i]);
        if (err) {
            num = i;
            goto DO_ERROR;
        }
    }
    return CUPKEE_OK;

DO_ERROR:
    for (i = 0; i < num; i++) {
        hw_adc_chn_reset(seq[i]);
    }
    return err;
}

static inline int hw_adc_ready(int instance) {
    (void) instance;

    return 1;
}

static inline int hw_adc_convert_ok(int instance) {
    (void) instance;

    return adc_eoc(ADC1);
}

static void adc_reset(int instance)
{
    /* Do hardware reset here */

    adc_controls[instance].dev_id = DEVICE_ID_INVALID;
    adc_controls[instance].config = NULL;
}

static void adc_release(int instance)
{
    adc_reset(instance);

    adc_controls[instance].inused = 0;
}

static int adc_setup(int instance, uint8_t dev_id, const hw_config_t *conf)
{
    hw_adc_t *control = &adc_controls[instance];
    const hw_config_adc_t *config = (const hw_config_adc_t*)conf;
    int i, err = 0;

    for (i = 0; i < config->chn_num; i++) {
        if (config->chn_seq[i] > 17) {
            err = CUPKEE_EINVAL;
            goto DO_END;
        }
        control->data[i] = ADC_INVALID;
    }
    control->state = ADC_IDLE;
    control->sleep = config->interval;
    control->current = 0;
    control->changed = 0;

    /* hardware setup here */
    err = channel_setup(config->chn_num, config->chn_seq);
    if (CUPKEE_OK != err) {
        goto DO_END;
    }

    rcc_periph_clock_enable(RCC_ADC1);

    adc_power_off(ADC1);
    adc_disable_scan_mode(ADC1);
    adc_disable_external_trigger_regular(ADC1);
    adc_set_right_aligned(ADC1);
    adc_set_single_conversion_mode(ADC1);
    adc_set_sample_time_on_all_channels(ADC1, ADC_SMPR_SMP_28DOT5CYC);

    adc_power_on(ADC1);
    adc_reset_calibration(ADC1);
    adc_calibrate(ADC1);

    control->dev_id = dev_id;
    control->config = config;

DO_END:

    return -err;
}

static void adc_sync(int instance, uint32_t systicks)
{
    hw_adc_t *control = &adc_controls[instance];

    (void) systicks;

    if (control->state == ADC_READY && control->sleep) {
        control->sleep--;
    }
}

static void adc_poll(int instance)
{
    hw_adc_t *control = &adc_controls[instance];

    switch(control->state) {
    case ADC_IDLE:
        if (hw_adc_ready(instance)) {
            const hw_config_adc_t *config = control->config;

            adc_set_regular_sequence(ADC1, config->chn_num, (uint8_t *)config->chn_seq);
            control->state = ADC_READY;
        }
        break;
    case ADC_READY:
        if (control->sleep == 0) {
            adc_start_conversion_direct(ADC1);
            control->state = ADC_BUSY;
        }
        break;
    case ADC_BUSY:
        if (hw_adc_convert_ok(instance)) {
            uint8_t  curr = control->current;
            uint16_t data = adc_read_regular(ADC1);
            uint16_t last = control->data[curr];

            if (curr + 1 >= control->config->chn_num) {
                control->current = 0;
            } else {
                control->current = curr + 1;
            }

            if (data != last) {
                control->data[curr] = data;
                control->changed = curr;
                cupkee_event_post_device_data(control->dev_id);
            }

            control->sleep = control->config->interval;
            control->state = ADC_READY;

            _TRACE("... adc convert complete\n");
        } else {
            _TRACE("... adc convert wait\n");
        }
        break;
    default:
        control->state = ADC_IDLE;
        control->current = 0;
        // Todo: error process here
        break;
    }
}

static int adc_get(int instance, int off, uint32_t *data)
{
    hw_adc_t *control = &adc_controls[instance];

    if (off < 0) {
        if (control->data[control->changed] != ADC_INVALID) {
            *data = control->changed;
            return 1;
        }
    } else
    if (off < control->config->chn_num) {
        if (control->data[off] != ADC_INVALID) {
            *data = control->data[off];
            return 1;
        }
    }

    return 0;
}

static int adc_set(int instance, int off, uint32_t data)
{
    (void) instance;
    (void) off;
    (void) data;
    return 0;
}

static int adc_size(int instance)
{
    hw_adc_t *control = &adc_controls[instance];

    return control->config->chn_num;
}

static const hw_driver_t adc_driver = {
    .release = adc_release,
    .reset   = adc_reset,
    .setup   = adc_setup,
    .poll    = adc_poll,
    .sync    = adc_sync,

    .get = adc_get,
    .set = adc_set,
    .size = adc_size
};

const hw_driver_t *hw_request_adc(int instance)
{
    if (instance >= HW_INSTANCES_ADC || adc_controls[instance].inused) {
        return NULL;
    }

    adc_controls[instance].inused = 1;
    adc_controls[instance].dev_id = DEVICE_ID_INVALID;
    adc_controls[instance].config = NULL;

    return &adc_driver;
}

void hw_setup_adc(void)
{
    int i;

    for (i = 0; i < HW_INSTANCES_ADC; i++) {
        adc_controls[i].inused = 0;
    }
}

