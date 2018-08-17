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

static const cupkee_pinmap_t board_pins[] = {
    {2, 13}, // pin0 : bank C, port 13
};

static const native_t board_entries[] = {
    /* Panda natives */

    /* Cupkee natives */
    {"sysinfos",        native_sysinfos},
    {"systicks",        native_systicks},
    {"require",         native_require_module},
    {"reset",           native_reset},
    {"erase",           native_erase},
    {"report",          native_report},
    {"interface",       native_interface},
    {"print",           native_print},

    {"pinMode",         native_pin_mode},
    {"pinRead",         native_pin_read},
    {"pinWrite",        native_pin_read},
    {"pinToggle",       native_pin_toggle},
    {"pinWave",         native_pin_squarewave},
    {"pinWatch",        native_pin_watch},
    /*
    {"Serial",          native_pin_serial},
    */

    {"setTimeout",      native_set_timeout},
    {"setInterval",     native_set_interval},
    {"clearTimeout",    native_clear_timeout},
    {"clearInterval",   native_clear_interval},
    {"updateInterval",  native_update_interval},

    {"Device",          native_create_device},
    {"Timer",           native_create_timer},
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
    cupkee_shell_init(stream, sizeof(board_entries) / sizeof(native_t), board_entries, NULL);

    /**********************************************************
     * Let's Go!
     *********************************************************/
    cupkee_shell_loop(NULL);
}

