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

#ifndef __CUPKEE_SDMP_INC__
#define __CUPKEE_SDMP_INC__

int cupkee_sdmp_init(void *stream);
int cupkee_sdmp_tty_write(size_t len, const char *text);
int cupkee_sdmp_tty_write_sync(size_t len, const char *text);

int cupkee_sdmp_set_interface_id(const char *id);
int cupkee_sdmp_set_tty_handler(void (*handler)(int, const void *));
int cupkee_sdmp_set_call_handler(int (*handler)(int x, void *args));
int cupkee_sdmp_set_query_handler(int (*handler)(uint16_t flags));

int cupkee_sdmp_update_state_trigger(int id);
int cupkee_sdmp_update_state_boolean(int id, int v);
int cupkee_sdmp_update_state_number(int id, double v);
int cupkee_sdmp_update_state_string(int id, const char *s);

#endif /* __CUPKEE_SDMP_INC__ */

