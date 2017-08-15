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

typedef struct cupkee_module_t cupkee_module_t;

typedef struct kv_pair_t {
    intptr_t key;
    val_t    val;
} kv_pair_t;

struct cupkee_module_t {
    uint8_t reserved[2];
    uint8_t prop_cap;
    uint8_t prop_num;
    cupkee_module_t *next;
    const char *name;
    kv_pair_t props[0];
};

static cupkee_module_t *module_head;

void cupkee_module_init(void)
{
    module_head = NULL;
}

void *cupkee_module_create(const char *name, int prop_max)
{
    cupkee_module_t *mod;

    if (!name || !prop_max) {
        return NULL;
    }
    mod = cupkee_malloc(sizeof(cupkee_module_t) + sizeof(kv_pair_t) * prop_max);
    if (mod) {
        mod->prop_cap = prop_max;
        mod->prop_num = 0;
        mod->name = name;
        mod->next = NULL;
    }
    return mod;
}

void cupkee_module_release(void *mod)
{
    cupkee_module_t *curr = module_head, *prev = NULL;

    // drop from module list
    while (curr) {
        cupkee_module_t *next = curr->next;

        if (curr == mod) {
            if (prev) {
                prev->next = next;
            } else {
                module_head = next;
            }
            break;
        }
        prev = curr;
        curr = next;
    }

    cupkee_free(mod);
}

int cupkee_module_export_number(void *m, const char *key, double n)
{
    cupkee_module_t *mod = (cupkee_module_t *) m;

    if (mod->prop_num < mod->prop_cap) {
        kv_pair_t *kv = &mod->props[mod->prop_num++];

        kv->key = (intptr_t) key;
        val_set_number(&kv->val, n);
        return CUPKEE_OK;
    }

    return -CUPKEE_ELIMIT;
}

int cupkee_module_export_boolean(void *m, const char *key, int b)
{
    cupkee_module_t *mod = (cupkee_module_t *) m;

    if (mod->prop_num < mod->prop_cap) {
        kv_pair_t *kv = &mod->props[mod->prop_num++];

        kv->key = (intptr_t) key;
        val_set_boolean(&kv->val, b);
        return 0;
    }

    return -CUPKEE_ELIMIT;
}

int cupkee_module_export_string(void *m, const char *key, const char *s)
{
    cupkee_module_t *mod = (cupkee_module_t *) m;

    if (mod->prop_num < mod->prop_cap) {
        kv_pair_t *kv = &mod->props[mod->prop_num++];

        kv->key = (intptr_t) key;
        val_set_foreign_string(&kv->val, (intptr_t) s);
        return 0;
    }

    return -CUPKEE_ELIMIT;
}

int cupkee_module_export_native(void *m, const char *key, void *fn)
{
    cupkee_module_t *mod = (cupkee_module_t *) m;

    if (mod->prop_num < mod->prop_cap) {
        kv_pair_t *kv = &mod->props[mod->prop_num++];

        kv->key = (intptr_t) key;
        val_set_native(&kv->val, (intptr_t) fn);
        return 0;
    }

    return -CUPKEE_ELIMIT;
}

int cupkee_module_register(void *m)
{
    cupkee_module_t *mod = (cupkee_module_t *) m;
    cupkee_module_t *cur = module_head;

    while (cur) {
        if (strcmp(cur->name, mod->name) == 0) {
            return -CUPKEE_ENAME;
        } else {
            cur = cur->next;
        }
    }

    mod->next = module_head;
    module_head = mod;
    return 0;
}

static int module_is_true(intptr_t ptr)
{
    (void) ptr;

    return 1;
}

static void module_op_prop(void *env, intptr_t id, val_t *key, val_t *prop)
{
    cupkee_module_t *mod = (cupkee_module_t *)id;
    const char *prop_key = val_2_cstring(key);

    (void) env;

    if (mod && prop_key) {
        unsigned i = 0;

        while (i < mod->prop_num) {
            if (strcmp(prop_key, (const char *) (mod->props[i].key)) == 0) {
                *prop = mod->props[i].val;
                return;
            }
            i++;
        }
    }

    val_set_undefined(prop);
}

static const val_foreign_op_t module_op = {
    .is_true = module_is_true,
    .prop = module_op_prop,
    .elem = module_op_prop,
};

val_t native_require(env_t *env, int ac, val_t *av)
{
    const char *name = ac ? val_2_cstring(av) : NULL;
    val_t mod = VAL_UNDEFINED;

    if (name) {
        cupkee_module_t *cur = module_head;
        while (cur) {
            if (strcmp(cur->name, name) == 0) {
                val_foreign_create(env, &module_op, (intptr_t) cur, &mod);
                break;
            } else {
                cur = cur->next;
            }
        }
    }

    return mod;
}

