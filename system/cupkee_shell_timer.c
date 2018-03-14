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
#include "cupkee_shell_util.h"

typedef struct timer_param_t {
    val_t *handle;
} timer_param_t;

static int timer_callback(void *entry, int event, intptr_t data)
{
    timer_param_t *param = (timer_param_t *) data;
    int retval = CUPKEE_TIMER_KEEP;

    (void) entry;

    switch (event) {
    case CUPKEE_EVENT_STOP:
        if (param && param->handle) {
            shell_reference_release(param->handle);
            param->handle = NULL;
        }
        break;

    case CUPKEE_EVENT_REWIND:
        if (param && param->handle) {
            val_t ret = cupkee_execute_function(param->handle, 0, NULL);

            if (val_is_number(&ret)) {
                retval = val_2_integer(&ret);
            } else
            if (!val_is_true(&ret)) {
                retval = CUPKEE_TIMER_STOP;
            }
        }
        break;
    case CUPKEE_EVENT_DESTROY:
        if (param) {
            if (param->handle) {
                shell_reference_release(param->handle);
                param->handle = NULL;
            }
            cupkee_free(param);
        }
        break;
    case CUPKEE_EVENT_START:
        break;
    default:
        break;
    }

    return retval;
}

static val_t native_timer_start(env_t *env, int ac, val_t *av)
{
    void *timer;
    int period;
    timer_param_t *param;
    val_t *cb;

    (void) env;

    if (NULL == (timer = cupkee_shell_object_entry(&ac, &av))) {
        return VAL_UNDEFINED;
    }

    param = (timer_param_t *) cupkee_timer_callback_param(timer);
    if (!param) {
        return VAL_FALSE;
    }

    if (ac && val_is_function(av)) {
        if (!(cb = shell_reference_create(av))) {
            return VAL_FALSE;
        }
        ac--, av++;
    } else {
        return VAL_FALSE;
    }

    if (ac && val_is_number(av)) {
        period = val_2_integer(av);
    } else {
        period = 1000; // 1Ms
    }

    param->handle = cb;
    if (0 != cupkee_timer_start(timer, period)) {
        shell_reference_release((val_t *)cb);
        param->handle = NULL;
        return VAL_FALSE;
    }
    return VAL_TRUE;
}

static val_t native_timer_stop(env_t *env, int ac, val_t *av)
{
    void *timer;

    (void) env;

    if (NULL == (timer = cupkee_shell_object_entry(&ac, &av))) {
        return VAL_UNDEFINED;
    }

    return cupkee_timer_stop(timer) == 0 ? VAL_TRUE : VAL_FALSE;
}

static int timer_prop_get(void *entry, const char *key, val_t *prop)
{
    if (!strcmp(key, "start")) {
        val_set_native(prop, (intptr_t)native_timer_start);
        return 1;
    } else
    if (!strcmp(key, "stop")) {
        val_set_native(prop, (intptr_t)native_timer_stop);
        return 1;
    } else
    if (!strcmp(key, "duration")) {
        val_set_number(prop, cupkee_timer_duration(entry));
        return 1;
    } else {
        return 0;
    }
}

static const cupkee_meta_t timer_meta = {
    .prop_get = timer_prop_get
};

void cupkee_shell_init_timer(void)
{
    cupkee_object_set_meta(cupkee_timer_tag(), (void *)&timer_meta);
}

val_t native_create_timer(env_t *env, int ac, val_t *av)
{
    timer_param_t *param;
    void *timer;

    (void) ac;
    (void) av;

    param = (timer_param_t *) cupkee_malloc(sizeof(timer_param_t));
    if (!param) {
        return VAL_UNDEFINED;
    }

    timer = cupkee_timer_request(timer_callback, (intptr_t) param);
    if (!timer) {
        cupkee_free(param);
        return VAL_UNDEFINED;
    }

    return cupkee_shell_object_create(env, timer);
}

