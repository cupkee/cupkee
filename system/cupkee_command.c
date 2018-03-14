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

#include <cupkee.h>

#define COMMAND_ARG_MAX      8

static short command_num;
static short command_buf_size;
static char *command_buf;
static cupkee_command_entry_t *command_entrys;
static char *command_args[COMMAND_ARG_MAX];

static int command_parse(char *input)
{
    int i;
    char *word, *p;
    const char *sep = " \t\r\n";

    for (word = strtok_r(input, sep, &p), i = 0;
         word && i < COMMAND_ARG_MAX;
         word = strtok_r(NULL, sep, &p)) {

        command_args[i++] = word;
    }

    return i;
}


static void command_do(int ac, char **av)
{
    int i;

    if (command_num < 1 || !command_entrys) {
        return;
    }

    for (i = 0; i < command_num; i++) {
        if (!strcmp(av[0], command_entrys[i].name)) {
            if (command_entrys[i].handle) {
                command_entrys[i].handle(ac, av);
            }
            break;
        }
    }
}

static int command_complete(void)
{
    void *ctx;

    // Use command_buf as auto_complete buffer
    if (!command_buf) {
        return CON_EXECUTE_DEF;
    }
    ctx = cupkee_auto_complete_init(command_buf, command_buf_size);
    if (ctx) {
        int i;

        for (i = 0; command_entrys && i < command_num; i++) {
            cupkee_auto_complete_update(ctx, command_entrys[i].name);
        }
        cupkee_auto_complete_finish(ctx);
    }
    return CON_EXECUTE_DEF;
}

int cupkee_command_init(int n, cupkee_command_entry_t *entrys, int buf_size, char *buf)
{
    command_buf = buf;
    command_buf_size = buf_size;
    command_num = n;
    command_entrys = entrys;

    return 0;
}

int cupkee_command_handle(int type, int ch)
{
    (void) ch;

    if (type == CON_CTRL_ENTER) {
        int len = console_input_load(command_buf_size, command_buf);
        int argc;

        if (len < 1) {
            return CON_EXECUTE_DEF;
        }
        command_buf[len] = 0;
        cupkee_history_push(len, command_buf);

        argc = command_parse(command_buf);
        if (argc) {
            command_do(argc, command_args);
        }
    } else
    if (type == CON_CTRL_TABLE) {
        return command_complete();
    } else
    if (type == CON_CTRL_UP) {
        return cupkee_history_load(-1);
    } else
    if (type == CON_CTRL_DOWN) {
        return cupkee_history_load(1);
    }

    return CON_EXECUTE_DEF;
}

