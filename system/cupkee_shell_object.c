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


static void create_array_by_octstr(env_t *env, uint8_t *seq, val_t *res)
{
    if (seq) {
        uint8_t n = seq[0];
        array_t *a = _array_create(env, n);
        uint8_t i;

        if (a) {
            for (i = 0; i < n; i++) {
                val_set_number(_array_elem(a, i), seq[1 + i]);
            }
            val_set_array(res, (intptr_t) a);
            return;
        }
    }
    val_set_undefined(res);
}

static void object_elem_set(void *env, intptr_t o, val_t *key, val_t *val)
{
    cupkee_object_t *obj = (cupkee_object_t *)o;

    (void) env;

    if (val_is_number(key)) {
        int i = val_2_integer(key);

        if (val_is_number(val)) {
            cupkee_elem_set(obj->entry, i, CUPKEE_OBJECT_ELEM_INT, val_2_integer(val));
        } else
        if (val_is_string(val)) {
            cupkee_elem_set(obj->entry, i, CUPKEE_OBJECT_ELEM_STR, (intptr_t)val_2_cstring(val));
        }
    } else {
        const char *name = val_2_cstring(key);
        if (name) {
            if (val_is_number(val)) {
                cupkee_prop_set(obj->entry, name, CUPKEE_OBJECT_ELEM_INT, val_2_integer(val));
            } else
            if (val_is_string(val)) {
                cupkee_prop_set(obj->entry, name, CUPKEE_OBJECT_ELEM_STR, (intptr_t)val_2_cstring(val));
            } else
            if (val_is_array(val)){
                array_t *array = (array_t *)val_2_intptr(val);
                val_t   *elems = array_values(array);
                int n = array_len(array), i;

                for (i = 0; i < n; i++, elems++) {
                    if (val_is_number(elems) && cupkee_prop_set(obj->entry, name, CUPKEE_OBJECT_ELEM_INT, val_2_integer(elems)) < 1) {
                        break;
                    }
                }
            }
        }
    }
}

static void object_elem_get(void *env, intptr_t o, val_t *key, val_t *prop)
{
    cupkee_object_t *obj = (cupkee_object_t *)o;
    const cupkee_desc_t   *desc = cupkee_object_desc(obj);
    const char *name = NULL;
    intptr_t v;
    int t = CUPKEE_OBJECT_ELEM_NV;

    (void) env;

    if (val_is_number(key)) {
        int i = val_2_integer(key);

        if (desc->elem_get) {
            t = desc->elem_get(obj->entry, i, &v);
        }
    } else
    if ((name = val_2_cstring(key)) != NULL) {
        const cupkee_meta_t *meta = cupkee_object_meta(obj);
        if (meta && meta->prop_get && meta->prop_get(obj->entry, name, prop) > 0) {
            return;
        } else
        if (desc->prop_get) {
            t = desc->prop_get(obj->entry, name, &v);
        }
    }

    switch (t) {
    case CUPKEE_OBJECT_ELEM_INT:
        val_set_number(prop, v); return;
    case CUPKEE_OBJECT_ELEM_STR:
        val_set_foreign_string(prop, v); return;
    case CUPKEE_OBJECT_ELEM_BOOL:
        val_set_boolean(prop, v); return;
    case CUPKEE_OBJECT_ELEM_OCT:
        create_array_by_octstr(env, (uint8_t *)v, prop); return;
    default:
        break;
    }

    val_set_undefined(prop);
}

static const val_foreign_op_t object_op = {
    .elem_get = object_elem_get,
    .elem_set = object_elem_set,
};

val_t cupkee_shell_object_create(env_t *env, void *entry)
{
    if (entry) {
        return val_create(env, &object_op, (intptr_t)CUPKEE_OBJECT_PTR(entry));
    } else {
        return VAL_UNDEFINED;
    }
}

