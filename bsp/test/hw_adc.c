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

#define ADC_INVALID     0xffff

typedef struct hw_adc_t {
    uint8_t inused;
    uint8_t dev_id;
    uint8_t state;
    uint8_t current;
    uint16_t  data[HW_CHN_MAX_ADC];
    const hw_config_adc_t *config;
} hw_adc_t;

static hw_adc_t adc_controls[HW_INSTANCES_ADC];

/****************************************************************************
 * Debug start                                                             */
static int dbg_setup_status[HW_INSTANCES_ADC];
static int dbg_update[HW_INSTANCES_ADC];
static uint32_t dbg_data[HW_INSTANCES_ADC];

void hw_dbg_adc_setup_status_set(int instance, int status)
{
    dbg_setup_status[instance] = status;
}

void hw_dbg_adc_update(int instance, int chn, uint16_t data)
{
    dbg_update[instance] = 1;
    dbg_data[instance] = data;
    adc_controls[instance].current = chn;
}

/* debug end                                                               *
 ***************************************************************************/

static void adc_release(int instance)
{
    /* Do hardware release here */

    adc_controls[instance].inused = 0;
}

static void adc_reset(int instance)
{
    /* Do hardware reset here */

    adc_controls[instance].dev_id = DEVICE_ID_INVALID;
    adc_controls[instance].config = NULL;
}

static int adc_setup(int instance, uint8_t dev_id, const hw_config_t *conf)
{
    hw_adc_t *control = &adc_controls[instance];
    const hw_config_adc_t *config = (const hw_config_adc_t*)conf;
    int err, i;

    for (i = 0; i < config->chn_num; i++) {
        control->data[i] = ADC_INVALID;
    }

    /* hardware setup here */

    err = -dbg_setup_status[instance];

    if (!err) {
        control->dev_id = dev_id;
        control->config = config;
    }
    return err;
}

static void adc_poll(int instance)
{
    hw_adc_t *control = &adc_controls[instance];

    if (dbg_update[instance]) {
        dbg_update[instance] = 0;
        control->data[control->current] = dbg_data[instance];
        device_data_post(control->dev_id);
    }
}

static int adc_get(int instance, int off, uint32_t *data)
{
    hw_adc_t *control = &adc_controls[instance];

    if (off < 0) {
        *data = control->current;
        return 1;
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
    .io.map  = {
        .get = adc_get,
        .set = adc_set,
        .size = adc_size
    }
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

        // for debug
        dbg_setup_status[i] = 0;
    }
}

