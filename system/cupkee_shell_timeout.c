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

#include <cupkee.h>

#include "cupkee_shell_misc.h"

static void timeout_handle(int drop, void *param)
{
    if (drop) {
        shell_reference_release(param);
    } else {
        shell_do_callback(cupkee_shell_env(), param, 0, NULL);
    }
}

static int timeout_register(int ac, val_t *av, int repeat)
{
    val_t   *handle;
    uint32_t wait;
    cupkee_timeout_t *timeout;
    val_t *ref;

    if (ac < 1 || !val_is_function(av)) {
        return -1;
    }
    handle = av++;


    if (ac > 1 && val_is_number(av)) {
        wait = val_2_double(av);
    } else {
        wait = 0;
    }

    ref = shell_reference_create(handle);
    if (!ref) {
        return -1;
    }

    timeout = cupkee_timeout_register(wait, repeat, timeout_handle, ref);
    if (!timeout) {
        shell_reference_release(ref);
        return -1;
    }

    return timeout->id;
}

static int timeout_unregister(int ac, val_t *av, int repeat)
{
    int32_t tid = -1; // all

    if (ac > 0) {
        if (val_is_number(av)) {
            tid = val_2_intptr(av);
        } else {
            return -1;
        }
    }

    if (tid >= 0) {
        return cupkee_timeout_clear_with_id(tid);
    } else {
        return cupkee_timeout_clear_with_flags(repeat ? 1: 0);
    }
}

val_t native_set_timeout(env_t *env, int ac, val_t *av)
{
    int tid = timeout_register(ac, av, 0);

    (void) env;

    return tid < 0 ? val_mk_boolean(0) : val_mk_number(tid);
}

val_t native_set_interval(env_t *env, int ac, val_t *av)
{
    int tid = timeout_register(ac, av, 1);

    (void) env;

    return tid < 0 ? val_mk_boolean(0) : val_mk_number(tid);
}

val_t native_clear_timeout(env_t *env, int ac, val_t *av)
{
    int n = timeout_unregister(ac, av, 0);

    (void) env;

    return n < 0 ? val_mk_boolean(0) : val_mk_number(n);
}

val_t native_clear_interval(env_t *env, int ac, val_t *av)
{
    int n = timeout_unregister(ac, av, 1);

    (void) env;

    return n < 0 ? val_mk_boolean(0) : val_mk_number(n);
}

