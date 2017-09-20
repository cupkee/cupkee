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

static void op_prop(void *env, intptr_t id, val_t *name, val_t *prop)
{
    cupkee_object_t *obj = (cupkee_object_t *)id;
    cupkee_meta_t   *meta = cupkee_object_meta(obj);
    const char *prop_name = val_2_cstring(name);

    (void) env;

    if (meta && prop_name && meta->prop_get) {
        if (0 < meta->prop_get(obj->entry, prop_name, prop)) {
            return;
        }
    }

    val_set_undefined(prop);
}


static const val_foreign_op_t object_op = {
    .prop    = op_prop,
};

val_t cupkee_shell_object_create(env_t *env, void *entry)
{
    if (entry) {
        return val_create(env, &object_op, (intptr_t)CUPKEE_OBJECT_PTR(entry));
    } else {
        return VAL_UNDEFINED;
    }
}

