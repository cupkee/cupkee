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

#ifndef __CUPKEE_PROCESS_INC__
#define __CUPKEE_PROCESS_INC__

int cupkee_process_start(void (*fn)(void *entry), intptr_t data, void (*finish)(int err, intptr_t data));

intptr_t cupkee_process_data(void *entry);

int cupkee_process_step(void *entry);

void cupkee_process_goto(void *entry, int step);
void cupkee_process_next(void *entry);
void cupkee_process_done(void *entry);
void cupkee_process_fail(void *entry, int err);

#endif /* __CUPKEE_PROCESS_INC__ */

