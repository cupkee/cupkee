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


#ifndef __CUPKEE_MODULE_INC__
#define __CUPKEE_MODULE_INC__

void cupkee_module_init(void);

void *cupkee_module_create(const char *name, int members);
void cupkee_module_release(void *mod);

int cupkee_module_export_number(void *mod, const char *name, double n);
int cupkee_module_export_boolean(void *mod, const char *name, int b);
int cupkee_module_export_string(void *mod, const char *name, const char *s);
int cupkee_module_export_native(void *mod, const char *name, void *fn);

int cupkee_module_register(void *mod);


#endif /* __CUPKEE_MODULE_INC__ */

