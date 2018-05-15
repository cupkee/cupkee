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

static void board_setup(void *stream)
{
    module_example_init();

    /**********************************************************
     * Setup shell
     *********************************************************/
    cupkee_shell_init(stream, sizeof(board_entries) / sizeof(native_t), board_entries);

    /**********************************************************
     * Let's Go!
     *********************************************************/
    cupkee_shell_loop(NULL);
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
    void *stream;

    /**********************************************************
     * Cupkee system initial
     *********************************************************/
    cupkee_init(NULL);

    stream = cupkee_device_request("uart", 0);
    if (cupkee_device_enable(stream)) {
        hw_halt();
    }

    /**********************************************************
     * user setup code
     *********************************************************/
    board_setup(stream);

    /**********************************************************
     * Should not go here, make gcc happy!
     *********************************************************/
    return 0;
}

