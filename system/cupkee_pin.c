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

#define BANK_OF(pin)  ((pin_map_table[pin] >> 4) & 0x0F)
#define PORT_OF(pin)  (pin_map_table[pin] & 0x0F)

typedef struct pin_group_t {
    uint8_t max;
    uint8_t num;
    uint8_t *pin;
} pin_group_t;

typedef struct pin_event_handle_info_t {
    struct pin_event_handle_info_t *next;
    int pin;
    cupkee_callback_t handler;
    void *entry;
} pin_event_handle_info_t;

static uint8_t  pin_map_table[CUPKEE_PIN_MAX];
static uint8_t  pin_group_tag;
static pin_event_handle_info_t *pin_event_handle_head;

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

static inline int pin_is_invalid(int pin) {
    return pin >= CUPKEE_PIN_MAX || pin_map_table[pin] == 0xFF;
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

        if (!pin_is_invalid(pin)) {
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

        if (!pin_is_invalid(pin)) {
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
    pin_event_handle_info_t *info = cupkee_malloc(sizeof(pin_event_handle_info_t));

    if (info) {
        info->next = pin_event_handle_head;
        info->pin  = pin;
        info->handler = handler;
        info->entry   = entry;

        pin_event_handle_head = info;

        return 0;
    }
    return -CUPKEE_ERESOURCE;
}

static void pin_event_handle_clear(int pin)
{
    pin_event_handle_info_t *prev = NULL;
    pin_event_handle_info_t *curr = pin_event_handle_head;

    while (curr) {
        pin_event_handle_info_t *next = curr->next;

        if (curr->pin == pin) {
            if (prev) {
                prev->next = next;
            } else {
                pin_event_handle_head = next;
            }

            if (curr->handler) {
                curr->handler(curr->entry, CUPKEE_EVENT_PIN_IGNORE, pin);
            }
            cupkee_free(curr);
        } else {
            prev = curr;
        }
        curr = next;
    }
}

int cupkee_pin_setup(void)
{
    int tag;

    memset(pin_map_table, 0xff, sizeof(pin_map_table));
    pin_event_handle_head = NULL;

    tag = cupkee_object_register(sizeof(pin_group_t), &pin_group_desc);
    if (tag < 0) {
        return tag;
    }
    pin_group_tag = tag;

    return 0;
}

void cupkee_pin_event_dispatch(uint16_t id, uint8_t code)
{
    pin_event_handle_info_t *info = pin_event_handle_head;

    while (info) {
        if (info->pin == id && info->handler) {
            info->handler(info->entry, code ? CUPKEE_EVENT_PIN_RISING : CUPKEE_EVENT_PIN_FALLING, id);
        }
        info = info->next;
    }
}

int cupkee_pin_map(int pin, int bank, int port)
{
    if (pin >= CUPKEE_PIN_MAX || pin_map_table[pin] != 0xFF) {
        return -CUPKEE_EINVAL;
    }

    pin_map_table[pin] = ((bank & 0xf) << 4) | (port & 0xf);

    return 0;
}

int cupkee_pin_enable(int pin, int dir)
{
    if (!pin_is_invalid(pin)) {
        return hw_gpio_enable(BANK_OF(pin), PORT_OF(pin), dir);
    } else {
        return -CUPKEE_EINVAL;
    }
}

int cupkee_pin_disable(int pin)
{
    if (!pin_is_invalid(pin)) {
        return hw_gpio_disable(BANK_OF(pin), PORT_OF(pin));
    } else {
        return -CUPKEE_EINVAL;
    }
}

int cupkee_pin_get(int pin)
{
    if (!pin_is_invalid(pin)) {
        return hw_gpio_get(BANK_OF(pin), PORT_OF(pin));
    } else {
        return -CUPKEE_EINVAL;
    }
}

int cupkee_pin_set(int pin, int v)
{
    if (!pin_is_invalid(pin)) {
        return hw_gpio_set(BANK_OF(pin), PORT_OF(pin), v);
    } else {
        return -CUPKEE_EINVAL;
    }
}

int cupkee_pin_toggle(int pin)
{
    if (!pin_is_invalid(pin)) {
        return hw_gpio_toggle(BANK_OF(pin), PORT_OF(pin));
    } else {
        return 0;
    }
}

int cupkee_pin_listen(int pin, int events, cupkee_callback_t handler, void *entry)
{
    events &= CUPKEE_EVENT_PIN_RISING | CUPKEE_EVENT_PIN_FALLING;

    if (events && !pin_is_invalid(pin)) {
        int err;
        err = pin_event_handle_set(pin, handler, entry);
        if (err) {
            return err;
        }
        err = hw_gpio_listen(BANK_OF(pin), PORT_OF(pin), events, pin);
        if (err) {
            pin_event_handle_clear(pin);
        }
        return err;
    } else {
        return -CUPKEE_EINVAL;
    }
}

int cupkee_pin_ignore(int pin)
{
    if (!pin_is_invalid(pin)) {
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
        if (!pin_is_invalid(pin) && g->num < g->max) {
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

            if (!pin_is_invalid(pin) && hw_gpio_get(BANK_OF(pin), PORT_OF(pin)) > 0) {
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

            if (!pin_is_invalid(pin)) {
                hw_gpio_set(BANK_OF(pin), PORT_OF(pin), v & 1);
            }
            v >>= 1;
            // else keep this bit zero
        }

        return 0;
    }

    return -CUPKEE_EINVAL;
}

