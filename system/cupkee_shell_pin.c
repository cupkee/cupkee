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
        val_t av[3];

        val_set_number(av, (event & CUPKEE_EVENT_PIN_RISING) ? 1 : 0);
        val_set_number(av + 1, cupkee_pin_duration(pin));
        val_set_number(av + 2, pin);

        cupkee_execute_function(entry, 3, av);
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

static int pin_mode_parse(const char *name)
{
    if (!name) {
        return -1;
    }

    if (!strcasecmp(name, "in")) {
        return CUPKEE_PIN_MODE_IN;
    } else
    if (!strcasecmp(name, "out")) {
        return CUPKEE_PIN_MODE_OUT;
    } else
    if (!strcasecmp(name, "ain")) {
        return CUPKEE_PIN_MODE_AIN;
    } else
    if (!strcasecmp(name, "aout")) {
        return CUPKEE_PIN_MODE_AOUT;
    } else
    if (!strcasecmp(name, "in-pullup")) {
        return CUPKEE_PIN_MODE_IN_PULLUP;
    } else
    if (!strcasecmp(name, "in-pulldown")) {
        return CUPKEE_PIN_MODE_IN_PULLDOWN;
    } else
    if (!strcasecmp(name, "opendrain")) {
        return CUPKEE_PIN_MODE_OPENDRAIN;
    } else {
        // default
        return CUPKEE_PIN_MODE_NE;
    }
}

static const char *pin_mode_name(int mode)
{
    switch (mode) {
    case CUPKEE_PIN_MODE_IN:            return "in";
    case CUPKEE_PIN_MODE_OUT:           return "out";
    case CUPKEE_PIN_MODE_AIN:           return "ain";
    case CUPKEE_PIN_MODE_AOUT:          return "aout";
    case CUPKEE_PIN_MODE_IN_PULLUP:     return "in-pullup";
    case CUPKEE_PIN_MODE_IN_PULLDOWN:   return "in-pulldown";
    case CUPKEE_PIN_MODE_OPENDRAIN:     return "opendrain";
    case CUPKEE_PIN_MODE_ALTFN:         return "altfn";
    default:                    return "NE";
    }
}

val_t native_pin_mode(env_t *env, int ac, val_t *av)
{
    array_t *list;
    int i, mode;
    const char *name;

    (void) env;

    if (ac == 1) {  // Get mode
        if (val_is_number(av) && 0 <= (mode = cupkee_pin_mode_get(val_2_integer(av)))) {
            name = pin_mode_name(mode);
            return val_mk_foreign_string((intptr_t)name);
        } else {
            // Invalid parameter
            return VAL_UNDEFINED;
        }
    } else
    if (ac < 1 || 0 > (mode = pin_mode_parse(val_2_cstring(av + (ac - 1))))) {
        // Invalid parameter
        return VAL_UNDEFINED;
    }

    // Set mode
    list = array_entry(av);
    if (list) {
        int n = array_length(list);
        for (i = 0; i < n; ++i) {
            val_t *v = array_get(list, i);
            if (val_is_number(v)) {
                cupkee_pin_mode_set(val_2_integer(v), mode);
            } else {
                break;
            }
        }
    } else {
        int n = ac - 1;
        for (i = 0; i < n; ++i) {
            val_t *v = av + i;
            if (val_is_number(v)) {
                cupkee_pin_mode_set(val_2_integer(v), mode);
            } else {
                break;
            }
        }
    }

    return val_mk_number(i);
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

val_t native_pin_read(env_t *env, int ac, val_t *av)
{
    uint32_t retv = 0;

    (void) env;

    if (ac > 0) {
        array_t *list = array_entry(av);
        int i = 0, n;

        if (list) {
            n = array_length(list);
            for (; i < n; ++i) {
                val_t *v = array_get(list, i);
                if (val_is_number(v) && cupkee_pin_get(val_2_integer(v)) > 0) {
                    retv = (retv << 1) | 1;
                } else {
                    retv <<= 1;
                }
            }
        } else {
            n = ac;
            for (; i < n; ++i) {
                val_t *v = av + i;
                if (val_is_number(v) && cupkee_pin_get(val_2_integer(v)) > 0) {
                    retv = (retv << 1) | 1;
                } else {
                    retv <<= 1;
                }
            }
        }
    }

    return val_mk_number(retv);
}

val_t native_pin_write(env_t *env, int ac, val_t *av)
{
    int n = ac - 1;

    (void) env;

    if (n > 0 && val_is_number(av + n)) {
        uint32_t data = val_2_integer(av + n);
        array_t *list = array_entry(av);

        if (list) {
            for (n = array_length(list) - 1; n >= 0; --n) {
                val_t *v = array_get(list, n);
                if (val_is_number(v)) {
                    cupkee_pin_set(val_2_integer(v), data & 1);
                }
                data >>= 1;
            }
        } else {
            for (n -= 1; n >= 0; --n) {
                val_t *v = av + n;
                if (val_is_number(v)) {
                    cupkee_pin_set(val_2_integer(v), data & 1);
                }
                data >>= 1;
            }
        }
        return VAL_TRUE;
    } else {
        return VAL_FALSE;
    }
}

val_t native_pin_squarewave(env_t *env, int ac, val_t *av)
{
    int i, n, param[3];
    int retv = -1;

    (void) env;

    for (i = 0, n = 0; i < ac && n < 3; i++) {
        if (val_is_number(av + i)) {
            param[n++] = val_2_integer(av + i);
        }
    }

    if (n == 3) {
        uint32_t first = param[2];
        uint32_t second = param[1] - first;

        if (param[1] > param[2]) {
            retv = cupkee_pin_wave_gen(param[0], first, second, NULL, NULL);
        } else
        if (param[1] == 0) {
            retv = cupkee_pin_wave_stop(param[0], param[2]);
        }
    } else
    if (n == 2) {
        if (param[1] < 1) {
            retv = cupkee_pin_wave_stop(param[0], 0);
        } else {
            retv = cupkee_pin_wave_update(param[0], param[1]);
        }
    }

    return retv ? VAL_FALSE : VAL_TRUE;
}

val_t native_pin_watch(env_t *env, int ac, val_t *av)
{
    (void) env;

    if (ac > 1 && val_is_number(av)) {
        int pin = val_2_integer(av);

        if (val_is_function(av + 1)) {
            return pin_listen(pin, av + 1) == 0 ? VAL_TRUE : VAL_FALSE;
        } else
        if (!val_is_true(av + 1)) {
            return pin_ignore(pin) == 0 ? VAL_TRUE : VAL_FALSE;
        }
    }
    return VAL_FALSE;
}

val_t native_pin_read_analog(env_t *env, int ac, val_t *av)
{
    (void) env;
    if (ac > 0 && val_is_number(av)) {
        float v;

        if (cupkee_pin_get_analog(val_2_integer(av), &v) == 0) {
            return val_mk_number(v);
        }
    }

    return VAL_UNDEFINED;
}

val_t native_pin_write_analog(env_t *env, int ac, val_t *av)
{
    (void) env;
    if (ac > 1 && val_is_number(av) && val_is_number(av + 1)) {
        float v = val_2_double(av + 1);

        if (v >= 0 && v <= 1 && cupkee_pin_set_analog(val_2_integer(av), v) == 0) {
            return VAL_TRUE;
        }
    }

    return VAL_FALSE;
}

