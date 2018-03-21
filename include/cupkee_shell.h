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


#ifndef __CUPKEE_SHELL_INC__
#define __CUPKEE_SHELL_INC__

#include <panda.h>

typedef struct cupkee_meta_t {
    int (*prop_get)(void *entry, const char *key, val_t *res);
} cupkee_meta_t;

int cupkee_shell_init(int n, const native_t *natives);
int cupkee_shell_start(const char *initial);

static inline void cupkee_shell_loop(const char *initial) {
    cupkee_shell_start(initial);
    cupkee_loop();
}


env_t *cupkee_shell_env(void);
val_t *cupkee_shell_reference_create(val_t *v);
void cupkee_shell_reference_release(val_t *ref);

val_t *cupkee_ref_inc(val_t *v);
void   cupkee_ref_dec(val_t *r);

int   cupkee_execute_string(const char *script, val_t **res);
val_t cupkee_execute_function(val_t *fn, int ac, val_t *av);

val_t cupkee_shell_object_create(env_t *env, void *entry);

void *cupkee_shell_object_entry (val_t *av);

#endif /* __CUPKEE_SHELL_INC__ */

