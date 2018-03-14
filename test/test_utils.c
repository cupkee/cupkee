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

#include "test.h"

int TU_pre_init(void)
{
    hw_mock_init(1024 * 32); // 32K Ram

    cupkee_init(NULL);

    cupkee_start();

    return 0;
}

int TU_pre_deinit(void)
{
    hw_mock_deinit();
    return 0;
}

int TU_object_event_dispatch(void)
{
    cupkee_event_t e;
    if (cupkee_event_take(&e) && e.type == EVENT_OBJECT) {
        cupkee_object_event_dispatch(e.which, e.code);
        return 1;
    } else {
        return 0;
    }
}

int TU_pin_event_dispatch(void)
{
    cupkee_event_t e;
    if (cupkee_event_take(&e) && e.type == EVENT_PIN) {
        cupkee_pin_event_dispatch(e.which, e.code);
        return 1;
    } else {
        return 0;
    }
}

