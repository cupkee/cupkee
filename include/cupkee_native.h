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

#ifndef __CUPKEE_NATIVE_INC__
#define __CUPKEE_NATIVE_INC__

/* cupkee_shell_misc.c */
val_t native_sysinfos(env_t *env, int ac, val_t *av);
val_t native_systicks(env_t *env, int ac, val_t *av);
val_t native_print(env_t *env, int ac, val_t *av);
val_t native_erase(env_t *env, int ac, val_t *av);

val_t native_pin_enable(env_t *env, int ac, val_t *av);
val_t native_pin_group(env_t *env, int ac, val_t *av);
val_t native_pin(env_t *env, int ac, val_t *av);
val_t native_pin_toggle(env_t *env, int ac, val_t *av);

/* cupkee_shell_sdmp.c */
val_t native_report(env_t *env, int ac, val_t *av);
val_t native_interface(env_t *env, int ac, val_t *av);

/* cupkee_module.c */
val_t native_require(env_t *env, int ac, val_t *av);

/* cupkee_shell_systice.c */
val_t native_set_timeout(env_t *env, int ac, val_t *av);
val_t native_set_interval(env_t *env, int ac, val_t *av);
val_t native_clear_timeout(env_t *env, int ac, val_t *av);
val_t native_clear_interval(env_t *env, int ac, val_t *av);

/* cupkee_shell_device.c */
val_t native_create_device(env_t *env, int ac, val_t *av);
/*
val_t native_device_destroy(env_t *env, int ac, val_t *av);
val_t native_device_config(env_t *env, int ac, val_t *av);
val_t native_device_is_enabled(env_t *env, int ac, val_t *av);
val_t native_device_enable (env_t *env, int ac, val_t *av);
val_t native_device_disable(env_t *env, int ac, val_t *av);
val_t native_device_query(env_t *env, int ac, val_t *av);
val_t native_device_get(env_t *env, int ac, val_t *av);
val_t native_device_set(env_t *env, int ac, val_t *av);
val_t native_device_write(env_t *env, int ac, val_t *av);
val_t native_device_read (env_t *env, int ac, val_t *av);
val_t native_device_listen(env_t *env, int ac, val_t *av);
val_t native_device_ignore(env_t *env, int ac, val_t *av);
*/

/* cupkee_shell_timer.c */
val_t native_create_timer(env_t *env, int ac, val_t *av);

#endif /* __CUPKEE_NATIVE_INC__ */

