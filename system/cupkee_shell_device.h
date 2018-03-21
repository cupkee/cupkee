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

#ifndef __CUPKEE_SHELL_DEVICE_INC__
#define __CUPKEE_SHELL_DEVICE_INC__

const cupkee_device_desc_t *cupkee_device_query_by_name(const char *name);
const cupkee_device_desc_t *cupkee_device_query_by_type(uint16_t type);

val_t cupkee_device_config_set_one(cupkee_device_t *dev, env_t *env, val_t *which, val_t *val);
val_t cupkee_device_config_get_one(cupkee_device_t *dev, env_t *env, val_t *which);
int   cupkee_device_config_set_all(cupkee_device_t *dev, env_t *env, val_t *settings);
val_t cupkee_device_config_get_all(cupkee_device_t *dev);

void cupkee_shell_init_device(void);

#endif /* __CUPKEE_SHELL_DEVICE_INC__ */

