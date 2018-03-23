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

#ifndef __CUPKEE_DEF_INC__
#define __CUPKEE_DEF_INC__

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define CUPKEE_VER_SIZE                 4

#define CUPKEE_UID_SIZE                 24

#define CUPKEE_INFO_SIZE                (CUPKEE_VER_SIZE + CUPKEE_UID_SIZE * 2)

#define CUPKEE_BLOCK_SIZE               128
#define CUPKEE_SECTOR_SIZE              (CUPKEE_BLOCK_SIZE * 64)

#define CUPKEE_MEMBER_OFFSET(T, m)      (intptr_t)(&(((T*)0)->m))
#define CUPKEE_CONTAINER_OF(p, T, m)    ((T*)((intptr_t)(p) - CUPKEE_MEMBER_OFFSET(T, m)))

#define CUPKEE_SIZE_ALIGN(v, a)         (((size_t)(v) + ((a) - 1)) & ~((a) - 1))
#define CUPKEE_ADDR_ALIGN(p, a)         (void *)(((intptr_t)(p) + ((a) - 1)) & ~(intptr_t)((a) - 1))

#define CUPKEE_TRUE                     1
#define CUPKEE_FALSE                    0

#define CUPKEE_SYSDISK_SECTOR_COUNT		1024 * 32

#define CUPKEE_FLAG_REPEAT              0x01
#define CUPKEE_FLAG_KEEP                0x40
#define CUPKEE_FLAG_LANG                0x80

typedef int (*cupkee_callback_t)(void *entry, int event, intptr_t param);

void cupkee_sysinfo_get(uint8_t *info_buf);


#endif /* __CUPKEE_DEF_INC__ */

