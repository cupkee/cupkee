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

#include "module_example.h"

static const native_t native_entries[] = {
    /* Panda natives */

    /* Cupkee natives */
    {"sysinfos",        native_sysinfos},
    {"systicks",        native_systicks},
    {"print",           native_print},
    {"pinMap",          native_pin_map},
    {"pin",             native_pin},

    {"setTimeout",      native_set_timeout},
    {"setInterval",     native_set_interval},
    {"clearTimeout",    native_clear_timeout},
    {"clearInterval",   native_clear_interval},

    {"require",         native_require},
};

static void board_setup(void)
{
    module_example_init();
}

static int board_native_number(void)
{
    return sizeof(native_entries) / sizeof(native_t);
}

static const native_t *board_native_entries(void)
{
    return native_entries;
}

int main(void)
{
    void *tty;

    /**********************************************************
     * Cupkee system initial
     *********************************************************/
    cupkee_init();

#ifdef USE_USB_CONSOLE
    tty = cupkee_device_request("usb-cdc", 0);
#else
    tty = cupkee_device_request("uart", 0);
#endif
    cupkee_device_enable(tty);

    cupkee_shell_init(tty, board_native_number(), board_native_entries());

    /**********************************************************
     * user setup code
     *********************************************************/
    board_setup();

    /**********************************************************
     * Let's Go!
     *********************************************************/
    cupkee_shell_loop(NULL);

    /**********************************************************
     * Let's Go!
     *********************************************************/
    return 0;
}

