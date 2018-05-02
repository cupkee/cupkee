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

typedef struct module_entry_t {
    void *data;
    int (*prop_get)(void *entry, const char *k, val_t *p);
} module_entry_t;

static uint8_t module_tag = 0;
static uint8_t module_max = 0;
static const cupkee_module_t *module_descs;
static void **module_entries;
static const cupkee_desc_t module_desc = {
    .name = "Module",
};

static int module_prop_get(void *entry, const char *key, val_t *prop)
{
    module_entry_t *m = entry;

    return m->prop_get(m->data, key, prop);
}

static const cupkee_meta_t module_meta = {
    .prop_get = module_prop_get
};

void cupkee_module_init(int max, const cupkee_module_t *mods)
{
    int i;

    module_max = max;
    module_descs = mods;
    module_entries = cupkee_malloc(sizeof(void *) * max);

    for (i = 0; i < max; i++) {
        module_entries[i] = NULL;
    }

    module_tag = cupkee_object_register(sizeof(module_entry_t), &module_desc);
    cupkee_object_set_meta(module_tag, (void *)&module_meta);
}

val_t native_require_module(env_t *env, int ac, val_t *av)
{
    const char *name = ac ? val_2_cstring(av) : NULL;
    module_entry_t *mod;
    int i;

    if (name) {
        for (i = 0; i < module_max; i++) {
            if (strcmp(module_descs[i].name, name) == 0) {
                goto DO_MOD;
            }
        }
    }
    return VAL_UNDEFINED;

DO_MOD:
    mod = module_entries[i];
    if (!mod) {
        cupkee_object_t *obj = cupkee_object_create(module_tag);

        if (obj) {
            module_entries[i] = mod = (void *)obj->entry;
            mod->data = module_descs[i].init();
            mod->prop_get = module_descs[i].prop_get;
        }
    }

    return cupkee_shell_object_create(env, mod);
}

