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

#ifndef __CUPKEE_INC__
#define __CUPKEE_INC__

#include "cupkee_version.h"

/* Todo: cupkee_config.h ? */
// Device config
#define CUPKEE_DEVICE_TYPE_MAX          16

// Pin
#define CUPKEE_PIN_MAX                  32

// Memory config
#define CUPKEE_ZONE_MAX                 2

#define CUPKEE_PAGE_SHIFT               (10)
#define CUPKEE_PAGE_SIZE                (1U << CUPKEE_PAGE_SHIFT)
#define CUPKEE_PAGE_MASK                (((intptr_t)(-1)) << CUPKEE_PAGE_SHIFT)
#define CUPKEE_PAGE_ORDERR_MAX          (8)

#define CUPKEE_MUNIT_SHIFT              (5)
#define CUPKEE_MUNIT_SIZE               (1U << CUPKEE_MUNIT_SHIFT)


/* Cupkee api */
#include "cupkee_def.h"
#include "cupkee_bsp.h"
#include "cupkee_errno.h"
#include "cupkee_data.h"
#include "cupkee_utils.h"
#include "cupkee_storage.h"
#include "cupkee_memory.h"
#include "cupkee_event.h"
#include "cupkee_vector.h"
#include "cupkee_stream.h"
#include "cupkee_block.h"
#include "cupkee_buffer.h"
#include "cupkee_process.h"
#include "cupkee_struct.h"
#include "cupkee_object.h"
#include "cupkee_sdmp.h"
#include "cupkee_console.h"

#include "cupkee_pin.h"
#include "cupkee_timer.h"

#include "cupkee_timeout.h"
#include "cupkee_device.h"
#include "cupkee_auto_complete.h"
#include "cupkee_history.h"

// core module

void cupkee_init(const uint8_t *id);
void cupkee_loop(void);
void cupkee_event_poll(void);

static inline void cupkee_start(void) {
    _cupkee_systicks = 0;
}

static inline void cupkee_poll(void) {
    cupkee_device_poll();
    cupkee_event_poll();
}

int cupkee_sysdisk_read(uint32_t lba, uint8_t *copy_to);
int cupkee_sysdisk_write(uint32_t lba, const uint8_t *copy_from);

#include "cupkee_command.h"

#include "cupkee_shell.h"
#include "cupkee_module.h"
#include "cupkee_native.h"

#endif /* __CUPKEE_INC__ */

