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

#include "board.h"

#ifndef COMMAND_BUF_SIZE
#define COMMAND_BUF_SIZE    80
#endif
#define COMMAND_NUM     (sizeof(command_entries) / sizeof(cupkee_command_entry_t))

static char command_buf[COMMAND_BUF_SIZE];

static const cupkee_pinmap_t board_pins[] = {
    {0, 8}, // pin0 : bank 0, port 1
};

static int command_hello(int ac, char **av)
{
    (void) ac;
    (void) av;

    console_log("hello cupkee!\r\n");

    return 0;
}

static int command_toggle(int ac, char **av)
{
    (void) ac;
    (void) av;

    cupkee_pin_toggle(0);

    return 0;
}

static cupkee_command_entry_t command_entries[] = {
    {"hello", command_hello},
    {"toggle", command_toggle},
};


void board_setup(void *stream)
{
    /**********************************************************
     * Custom Setup here
     *********************************************************/

    // Setup board pins */
    cupkee_pin_map(sizeof(board_pins) / sizeof(cupkee_pinmap_t), board_pins);


    // Setup user modules here
    //

    /**********************************************************
     * Setup shell
     *********************************************************/
    cupkee_command_init(stream, command_entries,
                        COMMAND_NUM, COMMAND_BUF_SIZE, command_buf);


    /**********************************************************
     * Let's Go!
     *********************************************************/
    cupkee_loop();
}
