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

#define ADC_MAX         1

#define ADC_IDLE        0
#define ADC_READY       1
#define ADC_BUSY        2      // work in process
#define ADC_INVALID     0xffff

typedef struct hw_adc_t {
    int16_t id;
    uint8_t flags;
    uint8_t state;

    uint8_t current;
    uint8_t changed;
    uint16_t sleep;

    uint16_t data[8];
} hw_adc_t;

static hw_adc_t adcs[ADC_MAX];

const  uint8_t chn_port[] = {
    0, 0, 0, 0,                     // channel 0 - 3
    0, 0, 0, 0,                     // channel 4 - 7
    1, 1,                           // channel 8 - 9
    2, 2, 2, 2,                     // channel 10 - 13
    2, 2,                           // channel 14 - 15
};
const  uint16_t chn_pin[] = {
    0x0001, 0x0002, 0x0004, 0x0008, // channel 0 - 3
    0x0010, 0x0020, 0x0040, 0x0080, // channel 4 - 7
    0x0010, 0x0020,                 // channel 8 - 9
    0x0010, 0x0020, 0x0040, 0x0080, // channel 10 - 13
    0x0010, 0x0020,                 // channel 14 - 15
};

static inline hw_adc_t *device_block(int inst) {
    return (unsigned)inst < ADC_MAX ? &adcs[inst] : NULL;
}

static inline int device_chn_setup(uint8_t chn)
{
    if (chn < 16) {
        int port     = chn_port[chn];
        uint16_t pin = chn_pin[chn];

        if (!hw_gpio_use_setup(port, pin, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG)) {
            return -CUPKEE_ERESOURCE;
        }
    } else
    if (chn == 16) {
        return -CUPKEE_EIMPLEMENT;
    } else
    if (chn == 17) {
        return -CUPKEE_EIMPLEMENT;
    } else {
        return -CUPKEE_EINVAL;
    }

    return 0;
}

static inline void device_chn_reset(uint8_t chn) {
    if (chn < 16) {
        int port     = chn_port[chn];
        uint16_t pin = chn_pin[chn];

        hw_gpio_release(port, pin);
    }
}

static int device_channel_setup(uint8_t num, const uint8_t *seq)
{
    int i;

    for (i = 0; i < num; i++) {
        int err;
        if (0 != (err = device_chn_setup(seq[i]))) {
            for (; i > -1; i--) {
                device_chn_reset(seq[i]);
            }
            return err;
        }
    }
    return 0;
}

static inline int device_ready(int inst) {
    (void) inst;

    return 1;
}

static inline int device_convert_ok(int inst) {
    (void) inst;

    return adc_eoc(ADC1);
}

static int device_reset(int inst)
{
    hw_adc_t *device = device_block(inst);

    if (device) {

        device->id = CUPKEE_ID_INVALID;
        return 0;
    } else {
        return -CUPKEE_EINVAL;
    }
}

static uint8_t mock_seq[] = {0, 1, 2, 3};

static int device_setup(int inst, void *entry)
{
    hw_adc_t *device = device_block(inst);
    int err = 0;

    if (!device) {
        return -CUPKEE_EINVAL;
    }

    device->state = ADC_IDLE;
    device->sleep = 5;
    device->current = 0;
    device->changed = 0;

    /* hardware setup here */
    if (0 != (err = device_channel_setup(4, mock_seq))) {
        return err;
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

    device->id = CUPKEE_ENTRY_ID(entry);
    return 0;
}

static int device_request(int inst)
{
    if (inst >= ADC_MAX || adcs[inst].flags) {
        return -CUPKEE_EINVAL;
    }

    adcs[inst].flags = HW_FL_USED;
    adcs[inst].id    = CUPKEE_ID_INVALID;

    return 0;
}

static int device_release(int inst)
{
    hw_adc_t *device = device_block(inst);

    if (device) {
        device_reset(inst);
        device->flags = 0;
        return 0;
    } else {
        return -CUPKEE_EINVAL;
    }
}

static int device_poll(int inst)
{
    (void) inst;

    return 0;

#if 0
    if (device) {
        switch(device->state) {
        case ADC_IDLE:
            if (device_ready(inst)) {
                adc_set_regular_sequence(ADC1, 4, mock_seq);
                device->state = ADC_READY;
            }
            break;
        case ADC_READY:
            if (device->sleep == 0) {
                adc_start_conversion_direct(ADC1);
                device->state = ADC_BUSY;
            } else {
                device->sleep--;
            }
            break;
        case ADC_BUSY:
            if (device_convert_ok(inst)) {
                uint8_t  curr = device->current;
                uint16_t data = adc_read_regular(ADC1);
                uint16_t last = device->data[curr];

                if (curr + 1 >= device->config->chn_num) {
                    device->current = 0;
                } else {
                    device->current = curr + 1;
                }

                if (data != last) {
                    device->data[curr] = data;
                    device->changed = curr;
                    cupkee_event_post_device_data(device->dev_id);
                }

                device->sleep = device->config->interval;
                device->state = ADC_READY;
            }
            break;
        default:
            device->state = ADC_IDLE;
            device->current = 0;
            // Todo: error process here
            break;
        }
        return 0;
    } else {
        return -CUPKEE_EINVAL;
    }
#endif
}

static int device_get(int inst, int off, uint32_t *data)
{
    (void) inst;
    (void) off;
    (void) data;
    return 0;
}

static int device_set(int inst, int off, uint32_t data)
{
    (void) inst;
    (void) off;
    (void) data;
    return 0;
}

static const cupkee_driver_t device_driver = {
    .request = device_request,
    .release = device_release,
    .reset   = device_reset,
    .setup   = device_setup,
    .poll    = device_poll,

    .get    = device_get,
    .set    = device_set,
};

static const cupkee_device_desc_t hw_device_adc = {
    .name = "adc",
    .inst_max = ADC_MAX,
    .conf_init = NULL,
    .driver = &device_driver
};

void hw_setup_adc(void)
{
    int i;

    for (i = 0; i < ADC_MAX; i++) {
        adcs[i].flags = 0;
    }

    cupkee_device_register(&hw_device_adc);
}

