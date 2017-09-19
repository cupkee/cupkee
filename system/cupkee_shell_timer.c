/*
MIT License

This file is part of cupkee project.

Copyright (c) 2016-2017 Lixing Ding <ding.lixing@gmail.com>

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
#include "cupkee_shell_misc.h"

typedef struct timer_param_t {
    val_t *handle;
} timer_param_t;

static void timer_op_prop(void *env, intptr_t id, val_t *name, val_t *prop);

static const val_foreign_op_t timer_op = {
    .prop    = timer_op_prop,
};

static int timer_callback(int timer, int event, intptr_t data)
{
    timer_param_t *param = (timer_param_t *) data;
    int retval = CUPKEE_TIMER_KEEP;

    (void) timer;

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

static inline void *cupkee_object_entry (int *ac, val_t **av)
{
    if ((*ac)) {
        val_t *v = *av;
        if (val_is_foreign(v)) {
            val_foreign_t *fv = (val_foreign_t *) val_2_intptr(v);

            if (fv->op == &timer_op) {
                (*ac) --; (*av) ++;
                return (void *)fv->data;
            }
        }
    }
    return NULL;
}

static val_t native_timer_start(env_t *env, int ac, val_t *av)
{
    void *timer;
    int period;
    timer_param_t *param;
    val_t *cb;

    (void) env;

    if (NULL == (timer = cupkee_object_entry(&ac, &av))) {
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

    if (NULL == (timer = cupkee_object_entry(&ac, &av))) {
        return VAL_UNDEFINED;
    }

    return cupkee_timer_stop(timer) == 0 ? VAL_TRUE : VAL_FALSE;
}

static val_t native_timer_duration(env_t *env, int ac, val_t *av)
{
    void *timer;

    (void) env;

    if (NULL == (timer = cupkee_object_entry(&ac, &av))) {
        return VAL_UNDEFINED;
    }

    return val_mk_number(cupkee_timer_duration(timer));
}

static void timer_op_prop(void *env, intptr_t id, val_t *name, val_t *prop)
{
    const char *prop_name = val_2_cstring(name);

    (void) env;
    (void) id;

    if (prop_name) {
        if (!strcmp(prop_name, "start")) {
            val_set_native(prop, (intptr_t)native_timer_start);
            return;
        } else
        if (!strcmp(prop_name, "stop")) {
            val_set_native(prop, (intptr_t)native_timer_stop);
            return;
        } else
        if (!strcmp(prop_name, "duration")) {
            val_set_native(prop, (intptr_t)native_timer_duration);
            return;
        }
    }
    val_set_undefined(prop);
}

val_t native_timer_create(env_t *env, int ac, val_t *av)
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

    return val_create(env, &timer_op, timer);
}

