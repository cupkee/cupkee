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

#ifndef __CUPKEE_SHELL_MISC_INC__
#define __CUPKEE_SHELL_MISC_INC__

#include <cupkee.h>

void shell_reference_init(env_t *env);
void shell_reference_release(val_t *ref);
val_t *shell_reference_create(val_t *v);
val_t *shell_reference_ptr(uint8_t id);

void shell_print_value(val_t *v);
void shell_print_error(int error);

// cupkee_shell_timer.c
void cupkee_shell_init_timer(void);

#endif /* __CUPKEE_SHELL_MISC_INC__ */

