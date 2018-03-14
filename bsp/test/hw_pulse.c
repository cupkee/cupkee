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

typedef struct hw_pulse_t {
    uint8_t inused;
    uint8_t dev_id;
    const hw_config_pulse_t *config;
} hw_pulse_t;

static hw_pulse_t pulse_controls[HW_INSTANCES_PULSE];

static void pulse_release(int instance)
{
    /* Do hardware release here */

    pulse_controls[instance].inused = 0;
}

static void pulse_reset(int instance)
{
    /* Do hardware reset here */

    pulse_controls[instance].dev_id = DEVICE_ID_INVALID;
    pulse_controls[instance].config = NULL;
}

static int pulse_setup(int instance, uint8_t dev_id, const hw_config_t *conf)
{
    hw_pulse_t *control = &pulse_controls[instance];
    const hw_config_pulse_t *config = (const hw_config_pulse_t*)conf;
    int err;

    /* hardware setup here */

    err = 0;

    if (!err) {
        control->dev_id = dev_id;
        control->config = config;
    }
    return err;
}

static int pulse_set(int instance, int off, uint32_t data)
{
    (void) instance;
    (void) off;
    (void) data;
    return 1;
}

static int pulse_size(int instance)
{
    hw_pulse_t *control = &pulse_controls[instance];

    return control->config->chn_num;
}

static const hw_driver_t pulse_driver = {
    .release = pulse_release,
    .reset   = pulse_reset,
    .setup   = pulse_setup,
    .io.map  = {
        .set  = pulse_set,
        .size = pulse_size
    }
};

const hw_driver_t *hw_request_pulse(int instance)
{
    if (instance >= HW_INSTANCES_PULSE || pulse_controls[instance].inused) {
        return NULL;
    }

    pulse_controls[instance].inused = 1;
    pulse_controls[instance].dev_id = DEVICE_ID_INVALID;
    pulse_controls[instance].config = NULL;

    return &pulse_driver;
}

void hw_setup_pulse(void)
{
    int i;

    for (i = 0; i < HW_INSTANCES_PULSE; i++) {
        pulse_controls[i].inused = 0;
    }
}

