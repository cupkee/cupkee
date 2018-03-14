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

int main(void)
{
    void *stream;

    /**********************************************************
     * Cupkee system initial
     *********************************************************/
    cupkee_init(board_id());

#ifdef USE_USB_CONSOLE
    stream = cupkee_device_request("usb-cdc", 0);
#else
    stream = cupkee_device_request("uart", 0);
#endif
    if (cupkee_device_enable(stream)) {
        hw_halt();
    }

    cupkee_sdmp_init(stream);

    cupkee_shell_init(board_native_number(), board_native_entries());

    /**********************************************************
     * user setup code
     *********************************************************/
    board_setup();

    /**********************************************************
     * Let's Go!
     *********************************************************/
    cupkee_shell_loop(board_initial_script());

    /**********************************************************
     * Let's Go!
     *********************************************************/
    return 0;
}

