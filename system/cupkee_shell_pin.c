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

val_t native_pin_enable(env_t *env, int ac, val_t *av)
{
    int pin, dir;
    const char *str;
    (void) env;

    if (ac < 1 || !val_is_number(av)) {
        return VAL_FALSE;
    }
    pin  = val_2_integer(av);

    if (ac > 2 && val_is_number(av + 1) && val_is_number(av + 2)) {
        int bank = val_2_integer(av + 1);
        int port = val_2_integer(av + 2);

        if (0 != cupkee_pin_map(pin, bank, port)) {
            return VAL_FALSE;
        }
        ac -= 2; av += 2;
    }

    dir = HW_DIR_OUT;  // default is output
    if (ac > 1) {
        str = val_2_cstring(av + 1);
        if (str) {
            if (!strcmp(str, "in")) {
                dir = HW_DIR_IN;
            } else
            if (!strcmp(str, "duplex")) {
                dir = HW_DIR_DUPLEX;
            }
        }
    }

    if (CUPKEE_OK == cupkee_pin_enable(pin, dir)) {
        return VAL_TRUE;
    } else {
        return VAL_FALSE;
    }
}

val_t native_pin(env_t *env, int ac, val_t *av)
{
    (void) env;

    if (ac > 0 && val_is_number(av)) {
        int pin = val_2_integer(av);

        if (ac > 1) {
            return cupkee_pin_set(pin, val_is_true(av + 1)) > 0 ? VAL_TRUE : VAL_FALSE;
        } else {
            return val_mk_number(cupkee_pin_get(pin));
        }
    }

    return VAL_UNDEFINED;
}

val_t native_pin_toggle(env_t *env, int ac, val_t *av)
{
    (void) env;

    if (ac > 0 && val_is_number(av)) {
        return cupkee_pin_toggle(val_2_integer(av)) == 0 ? VAL_TRUE : VAL_FALSE;
    }

    return VAL_UNDEFINED;
}

