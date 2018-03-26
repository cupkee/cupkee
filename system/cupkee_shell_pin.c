/* GPLv2 License
 *
 * Copyright (C) 2018 Lixing Ding <ding.lixing@gmail.com>
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

#include "cupkee_shell_inner.h"

static int pin_handler(void *entry, int event, intptr_t pin)
{
    if (event) {
        val_t av[2];

        val_set_number(av, (event & CUPKEE_EVENT_PIN_RISING) ? 1 : 0);
        val_set_number(av + 1, pin);

        cupkee_execute_function(entry, 2, av);
    } else {
        // ignore
        shell_reference_release(entry);
    }
    return 0;
}

static int pin_listen(int pin, val_t *fn)
{
    val_t *ref = shell_reference_create(fn);

    if (ref) {
        if (cupkee_pin_listen(pin, CUPKEE_EVENT_PIN_RISING | CUPKEE_EVENT_PIN_FALLING, pin_handler, ref)) {
            shell_reference_release(ref);
            return -1;
        } else {
            return 0;
        }
    } else {
        return -CUPKEE_ENOMEM;
    }
}

static int pin_setup(int ac, val_t *av, const char *setting)
{
    int i, dir;

    if (!strcasecmp(setting, "in")) {
        dir = HW_DIR_IN;
    } else
    if (!strcasecmp(setting, "out")) {
        dir = HW_DIR_OUT;
    } else
    if (!strcasecmp(setting, "duplex")) {
        dir = HW_DIR_DUPLEX;
    } else
    if (!strcasecmp(setting, "disable")) {
        // disable pins
        for (i = 0; i < ac; i++) {
            if (val_is_number(av + i)) {
                cupkee_pin_disable(val_2_integer(av + i));
            }
        }
        return 1;
    } else
    if (!strcasecmp(setting, "ignore")) {
        // ignore pins
        for (i = 0; i < ac; i++) {
            if (val_is_number(av + i)) {
                cupkee_pin_ignore(val_2_integer(av + i));
            }
        }
        return 1;
    } else {
        return 0;
    }

    for (i = 0; i < ac; ++i) {
        if (!val_is_number(av + i) || CUPKEE_OK != cupkee_pin_enable(val_2_integer(av + i), dir)) {
            goto DO_FAIL;
        }
    }

    return 1;

DO_FAIL:
    for (--i; i >= 0; --i) {
        cupkee_pin_disable(val_2_integer(av + i));
    }
    return 0;
}

val_t native_pin_group(env_t *env, int ac, val_t *av)
{
    void *grp = cupkee_pin_group_create();
    uint8_t i;

    (void) env;

    if (!grp) {
        return VAL_UNDEFINED;
    }

    for (i = 0; i < ac; i++) {
        if (val_is_number(av + i)) {
            uint8_t pin = val_2_integer(av + i);
            cupkee_pin_group_push(grp, pin);
        }
    }
    return cupkee_shell_object_create(env, grp);
}

val_t native_pin(env_t *env, int ac, val_t *av)
{
    if (ac == 0 || !val_is_number(av)) {
        return VAL_UNDEFINED;
    } else {
        int pin = val_2_integer(av);

        if (ac == 1) {
            return val_mk_number(cupkee_pin_get(pin));
        } else {
            if (val_is_number(av + 1)) {
                return cupkee_pin_set(pin, val_is_true(av + 1)) > 0 ? VAL_TRUE : VAL_FALSE;
            } else
            if (val_is_function(av + 1)) {
                return pin_listen(pin, av + 1) == 0 ? VAL_TRUE : VAL_FALSE;
            } else {
                int end = ac - 1;

                if (val_is_string(av + end)) {
                    return val_mk_boolean(pin_setup(end, av, val_2_cstring(av + end)));
                }
            }
        }
    }
    return VAL_UNDEFINED;
}

val_t native_pin_toggle(env_t *env, int ac, val_t *av)
{
    int i;

    (void) env;
    for (i = 0; i < ac; i++) {
        if (val_is_number(av + i)) {
            cupkee_pin_toggle(val_2_integer(av + i));
        }
    }

    return VAL_UNDEFINED;
}

