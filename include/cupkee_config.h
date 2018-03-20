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

#ifndef __CUPKEE_CONFIG_INC__
#define __CUPKEE_CONFIG_INC__

// Device
#define CUPKEE_DEVICE_TYPE_MAX          16

// Pin
#define CUPKEE_PIN_MAX                  32

// Memory
#define CUPKEE_ZONE_MAX                 2

#define CUPKEE_PAGE_SHIFT               (10)
#define CUPKEE_PAGE_SIZE                (1U << CUPKEE_PAGE_SHIFT)
#define CUPKEE_PAGE_MASK                (((intptr_t)(-1)) << CUPKEE_PAGE_SHIFT)
#define CUPKEE_PAGE_ORDERR_MAX          (8)

#define CUPKEE_MUNIT_SHIFT              (5)
#define CUPKEE_MUNIT_SIZE               (1U << CUPKEE_MUNIT_SHIFT)

#endif /* __CUPKEE_CONFIG_INC__ */

