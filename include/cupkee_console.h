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

#ifndef __CONSOLE_INC__
#define __CONSOLE_INC__

#include <stdarg.h>

enum CONSOLE_CTRL_TYPE {
    CON_CTRL_IDLE = 0,
    CON_CTRL_CHAR = 1,
    CON_CTRL_BACKSPACE,
    CON_CTRL_DELETE,
    CON_CTRL_TABLE,
    CON_CTRL_ENTER,
    CON_CTRL_ESCAPE,
    CON_CTRL_UP,
    CON_CTRL_DOWN,
    CON_CTRL_RIGHT,
    CON_CTRL_LEFT,
    CON_CTRL_F1,
    CON_CTRL_F2,
    CON_CTRL_F3,
    CON_CTRL_F4
};

enum CONSOLE_HANDLE_RET {
    CON_EXECUTE_DEF = 0,
    CON_PREVENT_DEF = 1
};

typedef int (*console_handle_t)(int type, int ch);

int cupkee_console_init(console_handle_t handle);

void console_prompt_set(const char *prompt);
int console_input_clean(void);
int console_input_char(int ch);

int console_input_token(int size, char *buf);
int console_input_insert(int len, char *s);
int console_input_refresh(void);
int console_input_load(int size, char *buf);

int console_putc(int ch);
int console_puts(const char *s);

int console_log(const char *fmt, ...);

int console_putc_sync(int ch);
int console_puts_sync(const char *s);

int console_log_sync(const char *fmt, ...);

#endif /* __CONSOLE_INC__ */

