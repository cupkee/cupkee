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

static char command_buf[COMMAND_BUF_SIZE];

int main(void)
{
    void *stream;

    cupkee_init();


#ifdef USE_USB_CONSOLE
    stream = cupkee_device_request("usb-cdc", 0);
#else
    stream = cupkee_device_request("uart", 0);
#endif
    cupkee_device_enable(stream);
    cupkee_sdmp_init(stream);

    cupkee_command_init(board_commands(), board_command_entries(),
                        COMMAND_BUF_SIZE, command_buf);
    cupkee_history_init();
    cupkee_console_init(cupkee_command_handle);

    /**********************************************************
     * app device create & setup
     *********************************************************/
    board_setup();

    /**********************************************************
     * Let's Go!
     *********************************************************/
    cupkee_loop();

    /**********************************************************
     * Should not go here, make gcc happy!
     *********************************************************/
    return 0;
}

