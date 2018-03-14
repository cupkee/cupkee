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

typedef struct hw_timer_t {
    uint8_t inused;
    uint8_t dev_id;
    const hw_config_timer_t *config;
} hw_timer_t;

static hw_timer_t timer_controls[HW_INSTANCES_TIMER];

static void timer_release(int instance)
{
    /* Do hardware release here */

    timer_controls[instance].inused = 0;
}

static void timer_reset(int instance)
{
    /* Do hardware reset here */

    timer_controls[instance].dev_id = DEVICE_ID_INVALID;
    timer_controls[instance].config = NULL;
}

static int timer_setup(int instance, uint8_t dev_id, const hw_config_t *conf)
{
    hw_timer_t *control = &timer_controls[instance];
    const hw_config_timer_t *config = (const hw_config_timer_t*)conf;
    int err;

    /* hardware setup here */

    err = 0;

    if (!err) {
        control->dev_id = dev_id;
        control->config = config;
    }
    return err;
}

static int timer_set(int instance, int off, uint32_t data)
{
    (void) instance;
    (void) off;
    (void) data;
    return 1;
}

static int timer_size(int instance)
{
    hw_timer_t *control = &timer_controls[instance];

    return control->config->chn_num;
}

static const hw_driver_t timer_driver = {
    .release = timer_release,
    .reset   = timer_reset,
    .setup   = timer_setup,
    .io.map  = {
        .set  = timer_set,
        .size = timer_size
    }
};

const hw_driver_t *hw_request_timer(int instance)
{
    if (instance >= HW_INSTANCES_TIMER || timer_controls[instance].inused) {
        return NULL;
    }

    timer_controls[instance].inused = 1;
    timer_controls[instance].dev_id = DEVICE_ID_INVALID;
    timer_controls[instance].config = NULL;

    printf("request timer %d ok\n", instance);
    return &timer_driver;
}

void hw_setup_timer(void)
{
    int i;

    for (i = 0; i < HW_INSTANCES_TIMER; i++) {
        timer_controls[i].inused = 0;
    }
}

