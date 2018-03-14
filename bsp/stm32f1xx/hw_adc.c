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

#define ADC_MAX         1

#define ADC_IDLE        0
#define ADC_READY       1
#define ADC_BUSY        2      // work in process
#define ADC_INVALID     0xffff

typedef struct hw_adc_t {
    void *entry;

    uint8_t flags;
    uint8_t state;
    uint8_t current;
    uint8_t chn_num;

    const uint8_t *chn_seq;
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
        int      port = chn_port[chn];
        uint16_t pin  = chn_pin[chn];

        if (!hw_gpio_use_setup(port, pin, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG)) {
            return -CUPKEE_ERESOURCE;
        }
    } else
    if (chn < 18) {
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
    int i;

    for (i = 0; i < device->chn_num; i++) {
        device_chn_reset(device->chn_seq[i]);
    }
    adc_power_off(ADC1);
    rcc_periph_clock_disable(RCC_ADC1);

    if (device) {
        device->entry = NULL;
        return 0;
    } else {
        return -CUPKEE_EINVAL;
    }
}

static int device_setup(int inst, void *entry)
{
    hw_adc_t *device = device_block(inst);
    cupkee_struct_t *conf;
    int n = 0;

    if (!device) {
        return -CUPKEE_EINVAL;
    }

    conf = cupkee_device_config(entry);
    if (!conf) {
        return -CUPKEE_ERROR;
    }

    n = cupkee_struct_get_bytes(conf, 0, &device->chn_seq);
    if (n < 1) {
        return -CUPKEE_EINVAL;
    }

    device->state = ADC_IDLE;
    device->chn_num = n;
    device->current = 0;

    /* hardware setup here */
    if (0 != device_channel_setup(n, device->chn_seq)) {
        return -CUPKEE_EINVAL;
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

    device->entry = entry;
    return 0;
}

static int device_request(int inst)
{
    if (inst >= ADC_MAX || adcs[inst].flags) {
        return -CUPKEE_EINVAL;
    }

    adcs[inst].flags = HW_FL_USED;
    adcs[inst].entry = NULL;

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
    hw_adc_t *device = device_block(inst);

    if (device) {
        switch(device->state) {
        case ADC_IDLE:
            if (device_ready(inst)) {
                adc_set_regular_sequence(ADC1, device->chn_num, (uint8_t *)device->chn_seq);
                device->state = ADC_READY;
            }
            break;
        case ADC_READY:
            adc_start_conversion_direct(ADC1);
            device->state = ADC_BUSY;
            break;
        case ADC_BUSY:
            if (device_convert_ok(inst)) {
                uint8_t  curr = device->current;
                uint16_t data = adc_read_regular(ADC1);

                device->data[curr++] = data;

                if (curr >= device->chn_num) {
                    device->current = 0;
                } else {
                    device->current = curr;
                }

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
}

static int device_get(int inst, int i, uint32_t *data)
{
    hw_adc_t *device = device_block(inst);

    if (device && i < device->chn_num) {
        *data = device->data[i];
        return 1;
    }
    return 0;
}

static const cupkee_struct_desc_t conf_desc[] = {
    {
        .name = "channel",
        .size = 8,
        .type = CUPKEE_STRUCT_OCT
    }
};

static cupkee_struct_t *device_conf_init(void *curr)
{
    cupkee_struct_t *conf;

    if (curr) {
        conf = curr;
    } else {
        conf = cupkee_struct_alloc(1, conf_desc);
    }

    return conf;
}

static const cupkee_driver_t device_driver = {
    .request = device_request,
    .release = device_release,
    .reset   = device_reset,
    .setup   = device_setup,
    .poll    = device_poll,

    .get    = device_get,
};

static const cupkee_device_desc_t hw_device_adc = {
    .name = "adc",
    .inst_max = ADC_MAX,
    .conf_init = device_conf_init,
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

