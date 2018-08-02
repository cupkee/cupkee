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

#include "cupkee.h"

#define PIN_INVALID    0xFF
#define GROUP_DEF_SIZE 32

#define BANK_OF(pin)  (pin_control.pin_map[pin].bank)
#define PORT_OF(pin)  (pin_control.pin_map[pin].port)

typedef struct pin_group_t {
    uint8_t max;
    uint8_t num;
    uint8_t *pin;
} pin_group_t;

typedef struct pin_data_t {
    uint8_t events;

    void *entry;
    cupkee_callback_t handler;

    hw_pindata_t bsp_data;
} pin_data_t;

typedef struct pin_control_t {
    uint8_t pin_num;

    const cupkee_pinmap_t *pin_map;
    pin_data_t **pin_data;
} pin_control_t;

static pin_control_t pin_control;

static uint8_t  pin_group_tag;

static int pin_group_set (void *entry, int t, intptr_t v);
static int pin_group_set_elem (void *entry, int i, int t, intptr_t v);
static int pin_group_get_elem (void *entry, int i, intptr_t *p);
static int pin_group_get_prop (void *entry, const char *k, intptr_t *p);
static void pin_group_destroy(void *entry);

static const cupkee_desc_t pin_group_desc = {
    .name         = "PinGroup",

    .destroy      = pin_group_destroy,

    .set          = pin_group_set,

    .elem_get     = pin_group_get_elem,
    .elem_set     = pin_group_set_elem,
    .prop_get     = pin_group_get_prop,
};

static inline int pin_is_valid(uint8_t pin) {
    return pin < pin_control.pin_num;
}

static int pin_group_set (void *entry, int t, intptr_t v)
{
    pin_group_t *g = entry;

    if (g && t == CUPKEE_OBJECT_ELEM_INT) {
        return cupkee_pin_group_set(entry, v);
    }
    return -CUPKEE_EINVAL;
}

static int pin_group_set_elem (void *entry, int i, int t, intptr_t v)
{
    pin_group_t *g = entry;

    if (g && t == CUPKEE_OBJECT_ELEM_INT && i >= 0 && i < g->max) {
        uint8_t pin = g->pin[i];

        if (pin_is_valid(pin)) {
            return hw_gpio_set(BANK_OF(pin), PORT_OF(pin), v);
        }
    }
    return -CUPKEE_EINVAL;
}

static int pin_group_get_elem (void *entry, int i, intptr_t *p)
{
    pin_group_t *g = entry;

    if (g && i >= 0 && i < g->max) {
        uint8_t pin = g->pin[i];

        if (pin_is_valid(pin)) {
            *p = hw_gpio_get(BANK_OF(pin), PORT_OF(pin));
            return CUPKEE_OBJECT_ELEM_INT;
        }
    }
    return CUPKEE_OBJECT_ELEM_NV;
}

static int pin_group_get_prop (void *entry, const char *k, intptr_t *p)
{
    pin_group_t *g = entry;

    if (g && !strcmp(k, "length")) {
        *p = g->num;
        return CUPKEE_OBJECT_ELEM_INT;
    } else {
        return CUPKEE_OBJECT_ELEM_NV;
    }
}

static void pin_group_destroy(void *entry)
{
    pin_group_t *g = entry;

    if (g && g->pin) {
        cupkee_free(g->pin);
    }
}

static int pin_event_handle_set(int pin, cupkee_callback_t handler, void *entry)
{
    pin_data_t *pdata = pin_control.pin_data[pin];

    if (!pdata) {
        if (NULL != (pdata = cupkee_malloc(sizeof(pin_data_t)))) {
            memset(pdata, 0, sizeof(pin_data_t));
            pdata->bsp_data.id = pin;
            pin_control.pin_data[pin] = pdata;
        }
    }

    if (!pdata) {
        return -CUPKEE_ERESOURCE;
    }

    if (pdata->handler) {
        return -CUPKEE_EBUSY;
    }

    pdata->entry   = entry;
    pdata->handler = handler;

    return 0;
}

static void pin_event_handle_clear(int pin)
{
    pin_data_t *pdata = pin_control.pin_data[pin];

    if (pdata && pdata->handler) {
        void *entry = pdata->entry;
        cupkee_callback_t callback = pdata->handler;

        pdata->entry = NULL;
        pdata->handler = NULL;

        callback(entry, CUPKEE_EVENT_PIN_IGNORE, pin);
    }
}

int cupkee_pin_setup(void)
{
    int tag;

    pin_control.pin_num = 0;
    pin_control.pin_map = NULL;
    pin_control.pin_data = NULL;

    tag = cupkee_object_register(sizeof(pin_group_t), &pin_group_desc);
    if (tag < 0) {
        return tag;
    }
    pin_group_tag = tag;

    return 0;
}

void cupkee_pin_event_dispatch(uint16_t id, uint8_t code)
{
    if (id < pin_control.pin_num) {
        pin_data_t *pdata = pin_control.pin_data[id];

        if (pdata && pdata->handler) {
            if (code > 0 && (pdata->events & CUPKEE_EVENT_PIN_RISING)) {
                pdata->handler(pdata->entry, CUPKEE_EVENT_PIN_RISING, id);
            } else
            if ((code == 0 && (pdata->events & CUPKEE_EVENT_PIN_FALLING))) {
                pdata->handler(pdata->entry, CUPKEE_EVENT_PIN_FALLING, id);
            }
        }
    }
}

void cupkee_pin_schedule(uint32_t ticks)
{
    (void) ticks;
}

int cupkee_pin_map(uint8_t num, const cupkee_pinmap_t *map)
{
    pin_data_t **data_set;

    if (num < 1 || !map) {
        return -CUPKEE_EINVAL;
    }

    if (pin_control.pin_num > 0) {
        return -CUPKEE_EBUSY;
    }

    if (NULL == (data_set = cupkee_malloc(sizeof(pin_data_t *) * num))) {
        return -CUPKEE_ERESOURCE;
    }
    memset(data_set, 0, sizeof(pin_data_t *) * num);

    pin_control.pin_num = num;
    pin_control.pin_map = map;
    pin_control.pin_data = data_set;

    return 0;
}

int cupkee_pin_enable(int pin, int dir)
{
    if (pin_is_valid(pin)) {
        return hw_gpio_enable(BANK_OF(pin), PORT_OF(pin), dir);
    } else {
        return -CUPKEE_EINVAL;
    }
}

int cupkee_pin_disable(int pin)
{
    if (pin_is_valid(pin)) {
        return hw_gpio_disable(BANK_OF(pin), PORT_OF(pin));
    } else {
        return -CUPKEE_EINVAL;
    }
}

int cupkee_pin_get(int pin)
{
    if (pin_is_valid(pin)) {
        return hw_gpio_get(BANK_OF(pin), PORT_OF(pin));
    } else {
        return -CUPKEE_EINVAL;
    }
}

int cupkee_pin_set(int pin, int v)
{
    if (pin_is_valid(pin)) {
        return hw_gpio_set(BANK_OF(pin), PORT_OF(pin), v);
    } else {
        return -CUPKEE_EINVAL;
    }
}

int cupkee_pin_toggle(int pin)
{
    if (pin_is_valid(pin)) {
        return hw_gpio_toggle(BANK_OF(pin), PORT_OF(pin));
    } else {
        return 0;
    }
}

int cupkee_pin_listen(int pin, int events, cupkee_callback_t handler, void *entry)
{
    events &= CUPKEE_EVENT_PIN_RISING | CUPKEE_EVENT_PIN_FALLING;

    if (events && pin_is_valid(pin)) {
        pin_data_t *pdata;
        int err;

        err = pin_event_handle_set(pin, handler, entry);
        if (err) {
            return err;
        }
        pdata = pin_control.pin_data[pin];

        err = hw_gpio_listen(BANK_OF(pin), PORT_OF(pin), &pdata->bsp_data);
        if (err) {
            pin_event_handle_clear(pin);
        }
        pdata->events = events;
        return 0;
    } else {
        return -CUPKEE_EINVAL;
    }
}

int cupkee_pin_ignore(int pin)
{
    if (pin_is_valid(pin)) {
        pin_event_handle_clear(pin);
        return hw_gpio_ignore(BANK_OF(pin), PORT_OF(pin));
    } else {
        return -CUPKEE_EINVAL;
    }
}

void *cupkee_pin_group_create(void)
{
    cupkee_object_t *obj = cupkee_object_create(pin_group_tag);

    if (obj) {
        uint8_t *pin = cupkee_malloc(GROUP_DEF_SIZE);
        pin_group_t *g;

        if (!pin) {
            cupkee_object_destroy(obj);
            return NULL;
        }

        g = (pin_group_t *)obj->entry;
        g->max = GROUP_DEF_SIZE;
        g->num = 0;
        g->pin = pin;

        return g;
    }

    return NULL;
}

int cupkee_pin_group_push(void *entry, int pin)
{
    pin_group_t *g = entry;
    if (g) {
        if (pin_is_valid(pin) && g->num < g->max) {
            g->pin[g->num++] = pin;
            return g->num;
        }
    }

    return -CUPKEE_EINVAL;
}

int cupkee_pin_group_pop(void *entry)
{
    pin_group_t *g = entry;
    if (g) {
        if (g->num > 0) {
            return g->pin[--g->num];
        } else {
            return -CUPKEE_EEMPTY;
        }
    }

    return -CUPKEE_EINVAL;
}

int cupkee_pin_group_get(void *entry)
{
    pin_group_t *g = entry;
    if (g) {
        int retv = 0;
        unsigned i;

        for (i = 0; i < g->num; i++) {
            uint8_t pin = g->pin[i];

            if (pin_is_valid(pin) && hw_gpio_get(BANK_OF(pin), PORT_OF(pin)) > 0) {
                retv |= 1 << i;
            }
            // else keep this bit zero
        }

        return retv;
    }

    return -CUPKEE_EINVAL;
}

int cupkee_pin_group_set(void *entry, uint32_t v)
{
    pin_group_t *g = entry;
    if (g) {
        unsigned i;

        for (i = 0; i < g->num; i++) {
            uint8_t pin = g->pin[i];

            if (pin_is_valid(pin)) {
                hw_gpio_set(BANK_OF(pin), PORT_OF(pin), v & 1);
            }
            v >>= 1;
            // else keep this bit zero
        }

        return 0;
    }

    return -CUPKEE_EINVAL;
}

int cupkee_pin_group_tag(void)
{
    return pin_group_tag;
}

int cupkee_is_pin_group(void *entry)
{
    return cupkee_is_object(entry, pin_group_tag);
}

