/* GPLv2 License
 *
 * Copyright (C) 2018 Lixing Ding <ding.lixing@gmail.com>
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

static const cupkee_pinmap_t board_pins[] = {
    {0, 8}, // pin0 : bank 0, port 1
};

int main(void)
{
    int i;

    cupkee_loader_init();

    cupkee_pin_map(sizeof(board_pins) / sizeof(cupkee_pinmap_t), board_pins);

    cupkee_pin_enable(0, CUPKEE_PIN_OUT);

    /**********************************************************
     * Never go here!
     *********************************************************/
    while (1) {
        for (i = 0; i < 10000000; i++)
            cupkee_pin_toggle(0);
    }
}


