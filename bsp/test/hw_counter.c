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

typedef struct hw_counter_t {
    uint8_t inused;
    uint8_t dev_id;
    const hw_config_counter_t *config;
} hw_counter_t;

static hw_counter_t counter_controls[HW_INSTANCES_COUNTER];

static void counter_release(int instance)
{
    /* Do hardware release here */

    counter_controls[instance].inused = 0;
}

static void counter_reset(int instance)
{
    /* Do hardware reset here */

    counter_controls[instance].dev_id = DEVICE_ID_INVALID;
    counter_controls[instance].config = NULL;
}

static int counter_setup(int instance, uint8_t dev_id, const hw_config_t *conf)
{
    hw_counter_t *control = &counter_controls[instance];
    const hw_config_counter_t *config = (const hw_config_counter_t*)conf;
    int err;

    /* hardware setup here */

    err = 0;

    if (!err) {
        control->dev_id = dev_id;
        control->config = config;
    }
    return err;
}

static int counter_set(int instance, int off, uint32_t data)
{
    (void) instance;
    (void) off;
    (void) data;
    return 1;
}

static int counter_size(int instance)
{
    hw_counter_t *control = &counter_controls[instance];

    return control->config->chn_num;
}

static const hw_driver_t counter_driver = {
    .release = counter_release,
    .reset   = counter_reset,
    .setup   = counter_setup,
    .io.map  = {
        .set  = counter_set,
        .size = counter_size
    }
};

const hw_driver_t *hw_request_counter(int instance)
{
    if (instance >= HW_INSTANCES_COUNTER || counter_controls[instance].inused) {
        return NULL;
    }

    counter_controls[instance].inused = 1;
    counter_controls[instance].dev_id = DEVICE_ID_INVALID;
    counter_controls[instance].config = NULL;

    return &counter_driver;
}

void hw_setup_counter(void)
{
    int i;

    for (i = 0; i < HW_INSTANCES_COUNTER; i++) {
        counter_controls[i].inused = 0;
    }
}

