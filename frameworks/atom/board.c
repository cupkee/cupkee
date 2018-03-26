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
    {0, 8}, // pin0 : bank 0, port 1
};

static const native_t board_entries[] = {
    /* Panda natives */

    /* Cupkee natives */
    {"sysinfos",        native_sysinfos},
    {"systicks",        native_systicks},
    {"require",         native_require},
    {"report",          native_report},
    {"interface",       native_interface},

    {"print",           native_print},
    {"pinEnable",       native_pin_enable},
    {"pinGroup",        native_pin_group},
    {"pin",             native_pin},
    {"toggle",          native_pin_toggle},

    {"setTimeout",      native_set_timeout},
    {"setInterval",     native_set_interval},
    {"clearTimeout",    native_clear_timeout},
    {"clearInterval",   native_clear_interval},

    {"Device",          native_create_device},
    {"Timer",           native_create_timer},
};

void board_setup(void)
{
    // Setup user modules here
    // add user modules

    // Setup board pins */
    cupkee_pin_map(sizeof(board_pins) / sizeof(cupkee_pinmap_t), board_pins);

    /* Setup shell */
    cupkee_shell_init(sizeof(board_entries) / sizeof(native_t), board_entries);
}

const char *board_initial_script(void)
{
    return "";
}

