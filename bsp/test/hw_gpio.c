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

typedef struct hw_pin_t {
    uint8_t inused;
    uint8_t dev_id;
    uint16_t  data;
    const hw_config_pin_t *config;
} hw_pin_t;

static hw_pin_t pin_controls[HW_INSTANCES_PIN];

/****************************************************************************
 * Debug start                                                             */
static int dbg_setup_status[HW_INSTANCES_PIN];
static int dbg_update[HW_INSTANCES_PIN];
static uint32_t dbg_data[HW_INSTANCES_PIN];

void hw_dbg_pin_setup_status_set(int instance, int status)
{
    dbg_setup_status[instance] = status;
}

void hw_dbg_pin_data_set(int instance, int offset, uint32_t value)
{
    if (offset < 0) {
        pin_controls[instance].data = value;
    } else
    if (offset < 32) {
        if (value) {
            pin_controls[instance].data |= (1 << offset);
        } else {
            pin_controls[instance].data &= ~(1 << offset);
        }
    }
}

uint32_t hw_dbg_pin_data_get(int instance, int offset)
{
    if (offset < 0) {
        return pin_controls[instance].data;
    } else
    if (offset < 32) {
        return (pin_controls[instance].data & (1 << offset)) ? 1 : 0;
    } else {
        return 0;
    }
}

void hw_dbg_pin_trigger_error(int instance, int code)
{
    device_error_post(pin_controls[instance].dev_id, code);
}

void hw_dbg_pin_trigger_data(int instance, uint32_t data)
{
    dbg_update[instance] = 1;
    dbg_data[instance] = data;
}

/* debug end                                                               *
 ***************************************************************************/

static void pin_release(int instance)
{
    /* Do hardware release here */

    pin_controls[instance].inused = 0;
}

static void pin_reset(int instance)
{
    /* Do hardware reset here */

    pin_controls[instance].dev_id = DEVICE_ID_INVALID;
    pin_controls[instance].config = NULL;
}

static int pin_setup(int instance, uint8_t dev_id, const hw_config_t *config)
{
    hw_pin_t *control = &pin_controls[instance];
    int err;

    /* hardware setup here */
    err = -dbg_setup_status[instance];

    if (!err) {
        control->dev_id = dev_id;
        control->config = (const hw_config_pin_t *)config;
    }
    return err;
}

static void pin_poll(int instance)
{
    hw_pin_t *control = &pin_controls[instance];

    if (dbg_update[instance]) {
        dbg_update[instance] = 0;
        control->data = dbg_data[instance];
        device_data_post(control->dev_id);
    }
}

static int pin_get(int instance, int off, uint32_t *data)
{
    hw_pin_t *control = &pin_controls[instance];

    if (off < 0) {
        *data = control->data;
        return 1;
    } else
    if (off < control->config->num) {
        *data = (control->data & (1 << off)) ? 1 : 0;
        return 1;
    }

    return 0;
}

static int pin_set(int instance, int off, uint32_t data)
{
    hw_pin_t *control = &pin_controls[instance];

    if (off < 0) {
        control->data = data;
        return 1;
    } else
    if (off < control->config->num) {
        if (data) {
            control->data |= 1 << off;
        } else {
            control->data &= ~(1 << off);
        }
        return 1;
    }
    return 0;
}

static int pin_size(int instance)
{
    (void) instance;

    return 2;
}

static const hw_driver_t pin_driver = {
    .release = pin_release,
    .reset   = pin_reset,
    .setup   = pin_setup,
    .poll    = pin_poll,
    .io.map  = {
        .get = pin_get,
        .set = pin_set,
        .size = pin_size
    }
};

const hw_driver_t *hw_request_pin(int instance)
{
    if (instance >= HW_INSTANCES_PIN || pin_controls[instance].inused) {
        return NULL;
    }

    pin_controls[instance].inused = 1;
    pin_controls[instance].dev_id = DEVICE_ID_INVALID;
    pin_controls[instance].config = NULL;

    return &pin_driver;
}

void hw_setup_gpio(void)
{
    int i;

    for (i = 0; i < HW_INSTANCES_PIN; i++) {
        pin_controls[i].inused = 0;

        // for debug
        dbg_setup_status[i] = 0;
    }
}

