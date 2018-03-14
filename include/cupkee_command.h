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


#ifndef __CUPKEE_COMMAND_INC__
#define __CUPKEE_COMMAND_INC__

typedef struct cupkee_command_entry_t {
    const char *name;
    int (*handle)(int ac, char **av);
} cupkee_command_entry_t;

int cupkee_command_init(int command_num, cupkee_command_entry_t *command_entrys,
                        int command_buf_size, char *command_buf);
int cupkee_command_handle(int type, int ch);

#endif /* __CUPKEE_COMMAND_INC__ */

