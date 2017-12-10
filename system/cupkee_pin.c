/*
MIT License

This file is part of cupkee project.

Copyright (c) 2017 Lixing Ding <ding.lixing@gmail.com>

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

#include "cupkee.h"

#define PIN_INVALID    0xFF
#define GROUP_DEF_SIZE 16

#define BANK_OF(pin)  ((pin_map_table[pin] >> 4) & 0x0F)
#define PORT_OF(pin)  (pin_map_table[pin] & 0x0F)

typedef struct cupkee_pin_group_t {
    uint8_t max;
    uint8_t num;
    uint8_t pins[0];
} cupkee_pin_group_t;

static uint8_t  pin_map_table[CUPKEE_PIN_MAX];
static cupkee_callback_t pin_event_handler;
static void * pin_event_entry;

static inline int pin_is_invalid(int pin) {
    return pin >= CUPKEE_PIN_MAX || pin_map_table[pin] == 0xFF;
}

int cupkee_pin_setup(void)
{
    memset(pin_map_table, 0xff, sizeof(pin_map_table));
    pin_event_handler = NULL;
    pin_event_entry   = NULL;

    return 0;
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

void *cupkee_pin_group_create(void)
{
    cupkee_pin_group_t *grp = cupkee_malloc(sizeof(cupkee_pin_group_t) + GROUP_DEF_SIZE);

    if (grp) {
        grp->max = GROUP_DEF_SIZE;
        grp->num = 0;
    }

    return grp;
}

int cupkee_pin_group_destroy(void *grp)
{
    if (grp) {
        cupkee_free(grp);
        return 0;
    } else {
        return -CUPKEE_EINVAL;
    }
}

int cupkee_pin_group_size(void *grp)
{
    if (grp) {
        return ((cupkee_pin_group_t *)grp)->num;
    } else {
        return -CUPKEE_EINVAL;
    }
}

int cupkee_pin_group_push(void *grp, int pin)
{
    if (grp) {
        cupkee_pin_group_t *g = grp;

        if (!pin_is_invalid(pin) && g->num < g->max) {
            g->pins[g->num++] = pin;
            return g->num;
        }
    }

    return -CUPKEE_EINVAL;
}

int cupkee_pin_group_pop(void *grp)
{
    if (grp) {
        cupkee_pin_group_t *g = grp;

        if (g->num > 0) {
            return g->pins[--g->num];
        } else {
            return -CUPKEE_EEMPTY;
        }
    }

    return -CUPKEE_EINVAL;
}

int cupkee_pin_group_get(void *grp)
{
    if (grp) {
        cupkee_pin_group_t *g = grp;
        int retv = 0;
        unsigned i;

        for (i = 0; i < g->num; i++) {
            uint8_t pin = g->pins[i];

            if (!pin_is_invalid(pin) && hw_gpio_get(BANK_OF(pin), PORT_OF(pin)) > 0) {
                retv |= 1 << i;
            }
            // else keep this bit zero
        }

        return retv;
    }

    return -CUPKEE_EINVAL;
}

int cupkee_pin_group_set(void *grp, uint32_t v)
{
    if (grp) {
        cupkee_pin_group_t *g = grp;
        unsigned i;

        for (i = 0; i < g->num; i++) {
            uint8_t pin = g->pins[i];

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

int cupkee_pin_group_elem_get(void *grp, int i)
{
    if (grp) {
        cupkee_pin_group_t *g = grp;

        if (i < g->num) {
            uint8_t pin = g->pins[i];

            if (!pin_is_invalid(pin)) {
                return hw_gpio_get(BANK_OF(pin), PORT_OF(pin));
            }
        }
    }

    return -CUPKEE_EINVAL;
}

int cupkee_pin_group_elem_set(void *grp, int i, int v)
{
    if (grp) {
        cupkee_pin_group_t *g = grp;

        if (i < g->num) {
            uint8_t pin = g->pins[i];

            if (!pin_is_invalid(pin)) {
                return hw_gpio_set(BANK_OF(pin), PORT_OF(pin), v);
            }
        }
    }

    return -CUPKEE_EINVAL;
}

void cupkee_pin_event_dispatch(uint16_t id, uint8_t code)
{
    if (pin_event_handler) {
        pin_event_handler(pin_event_entry, code, id);
    }
}

void cupkee_pin_event_handle_set(cupkee_callback_t handler, void *entry)
{
    pin_event_handler = handler;
    pin_event_entry   = entry;
}

int cupkee_pin_listen(int pin, int events)
{
    events &= CUPKEE_EVENT_PIN_RISING | CUPKEE_EVENT_PIN_FALLING;

    if (events && !pin_is_invalid(pin)) {
        return hw_gpio_listen(BANK_OF(pin), PORT_OF(pin), events, pin);
    } else {
        return -CUPKEE_EINVAL;
    }
}

int cupkee_pin_ignore(int pin)
{
    if (!pin_is_invalid(pin)) {
        return hw_gpio_ignore(BANK_OF(pin), PORT_OF(pin));
    } else {
        return -CUPKEE_EINVAL;
    }
}

