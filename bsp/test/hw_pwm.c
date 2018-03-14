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

typedef struct hw_pwm_t {
    uint8_t inused;
    uint8_t dev_id;
    const hw_config_pwm_t *config;
} hw_pwm_t;

static hw_pwm_t pwm_controls[HW_INSTANCES_PWM];

static void pwm_release(int instance)
{
    /* Do hardware release here */

    pwm_controls[instance].inused = 0;
}

static void pwm_reset(int instance)
{
    /* Do hardware reset here */

    pwm_controls[instance].dev_id = DEVICE_ID_INVALID;
    pwm_controls[instance].config = NULL;
}

static int pwm_setup(int instance, uint8_t dev_id, const hw_config_t *conf)
{
    hw_pwm_t *control = &pwm_controls[instance];
    const hw_config_pwm_t *config = (const hw_config_pwm_t*)conf;
    int err;

    /* hardware setup here */

    err = 0;

    if (!err) {
        control->dev_id = dev_id;
        control->config = config;
    }
    return err;
}

static int pwm_set(int instance, int off, uint32_t data)
{
    (void) instance;
    (void) off;
    (void) data;
    return 1;
}

static int pwm_size(int instance)
{
    hw_pwm_t *control = &pwm_controls[instance];

    return control->config->chn_num;
}

static const hw_driver_t pwm_driver = {
    .release = pwm_release,
    .reset   = pwm_reset,
    .setup   = pwm_setup,
    .io.map  = {
        .set  = pwm_set,
        .size = pwm_size
    }
};

const hw_driver_t *hw_request_pwm(int instance)
{
    if (instance >= HW_INSTANCES_PWM || pwm_controls[instance].inused) {
        return NULL;
    }

    pwm_controls[instance].inused = 1;
    pwm_controls[instance].dev_id = DEVICE_ID_INVALID;
    pwm_controls[instance].config = NULL;

    return &pwm_driver;
}

void hw_setup_pwm(void)
{
    int i;

    for (i = 0; i < HW_INSTANCES_PWM; i++) {
        pwm_controls[i].inused = 0;
    }
}

