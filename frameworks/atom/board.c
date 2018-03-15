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

// Request from www.cupkee.com
static const uint8_t BOARD_ID[] = {
    0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00
};

const uint8_t *board_id(void)
{
    return BOARD_ID;
}

static const native_t native_entries[] = {
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

int board_native_number(void)
{
    return sizeof(native_entries) / sizeof(native_t);
}

const native_t *board_native_entries(void)
{
    return native_entries;
}

void board_setup(void)
{
    // Setup code here
}

const char *board_initial_script(void)
{
    return "";
}

