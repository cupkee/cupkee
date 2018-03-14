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

#ifndef __HW_MOCK_INC__
#define __HW_MOCK_INC__

void hw_mock_init(size_t mem_size);
void hw_mock_deinit(void);

cupkee_device_t *mock_device_curr(void);
size_t           mock_device_curr_want(void);

int  hw_mock_device_curr_id(void);
size_t hw_mock_device_curr_want(void);

/* TIMER */
int hw_mock_timer_curr_id(void);
int hw_mock_timer_curr_state(void);
int hw_mock_timer_period(void);
void hw_mock_timer_duration_set(int us);

#endif /* __HW_MOCK_INC__ */

