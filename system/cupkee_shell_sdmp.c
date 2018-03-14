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

#include "cupkee_shell_sdmp.h"
#include "cupkee_shell_util.h"

static val_t *user_state_ref = NULL;
static val_t *user_func_ref[16];

static int state_handler(uint16_t flags)
{
    (void) flags;

    if (user_state_ref) {
        cupkee_execute_function(user_state_ref, 0, NULL);
        return 0;
    } else {
        return -CUPKEE_EINVAL;
    }
}

static int call_handler(int which, void *args)
{
    if (which < 16 && user_func_ref[which]) {
        cupkee_data_entry_t *entry = args;
        val_t av[2];
        int   ac;

        for (ac = 0; ac < 2; ac++) {
            cupkee_data_t data;
            int type = cupkee_data_shift(entry, &data);

            if (type == CUPKEE_DATA_NONE) {
                break;
            } else
            if (type == CUPKEE_DATA_BOOLEAN) {
                val_set_boolean(av + ac, data.boolean);
            } else
            if (type == CUPKEE_DATA_NUMBER) {
                val_set_number(av + ac, data.number);
            } else
            if (type == CUPKEE_DATA_STRING) {
                av[ac] = string_create_heap(cupkee_shell_env(), data.string);
            }
        }

        cupkee_execute_function(user_func_ref[which], ac, av);
        return 0;
    } else {
        return -CUPKEE_EINVAL;
    }
}

void shell_sdmp_init(void)
{
    user_state_ref = NULL;

    memset(user_func_ref, 0, sizeof(user_func_ref));
}

val_t native_report(env_t *env, int ac, val_t *av)
{
    uint8_t state_id;

    (void) env;

    if (ac > 0 && val_is_number(av)) {
        state_id = val_2_integer(av);
    } else {
        return VAL_FALSE;
    }

    if (ac > 1) {
        ++av;

        if (val_is_boolean(av)) {
            return cupkee_sdmp_update_state_boolean(state_id, val_is_true(av)) ? VAL_FALSE : VAL_TRUE;
        } else
        if (val_is_number(av)) {
            return cupkee_sdmp_update_state_number(state_id, val_2_double(av)) ? VAL_FALSE : VAL_TRUE;
        } else {
            const char *s = val_2_cstring(av);

            if (s) {
                return cupkee_sdmp_update_state_string(state_id, s) ? VAL_FALSE : VAL_TRUE;
            } else {
                return VAL_FALSE;
            }
        }
    } else {
        return cupkee_sdmp_update_state_trigger(state_id) ? VAL_FALSE : VAL_TRUE;
    }
}

val_t native_interface(env_t *env, int ac, val_t *av)
{
    const char *id;
    int i;

    (void) env;

    if (ac < 2 || (!val_is_function(av + 1) || NULL == (id = val_2_cstring(av)))) {
        return VAL_FALSE;
    }

    if (cupkee_sdmp_set_interface_id(id)) {
        return VAL_FALSE;
    }

    shell_reference_release(user_state_ref);
    user_state_ref = shell_reference_create(av + 1);
    if (!user_state_ref) {
        return -1;
    }

    ac -= 2;
    for (i = 0, av += 2; i < ac && i < 16 && val_is_function(av); ++i, ++av) {
        shell_reference_release(user_func_ref[i]);
        user_func_ref[i] = shell_reference_create(av);
        if (!user_func_ref[i]) {
            goto DO_ERROR;
        }
    }

    cupkee_sdmp_set_query_handler(state_handler);
    cupkee_sdmp_set_call_handler(call_handler);

    return VAL_TRUE;

DO_ERROR:
    shell_reference_release(user_state_ref);
    for (; i >= 0; --i) {
        shell_reference_release(user_func_ref[i]);
    }
    return VAL_FALSE;
}

