/*
MIT License

This file is part of cupkee project.

Copyright (c) 2016-2017 Lixing Ding <ding.lixing@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef __CUPKEE_INC__
#define __CUPKEE_INC__

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/* Todo: need cupkee_config.h ? */
/* Todo: put to cupkee_type.h ? */
#define CUPKEE_TRUE                     1
#define CUPKEE_FALSE                    0

#define CUPKEE_ZONE_MAX                 2

#define CUPKEE_PAGE_SHIFT               (10)
#define CUPKEE_PAGE_SIZE                (1U << CUPKEE_PAGE_SHIFT)
#define CUPKEE_PAGE_MASK                (((intptr_t)(-1)) << CUPKEE_PAGE_SHIFT)
#define CUPKEE_PAGE_ORDERR_MAX          (8)

#define CUPKEE_MUNIT_SHIFT              (5)
#define CUPKEE_MUNIT_SIZE               (1U << CUPKEE_MUNIT_SHIFT)

#define CUPKEE_MEMBER_OFFSET(T, m)      (intptr_t)(&(((T*)0)->m))
#define CUPKEE_CONTAINER_OF(p, T, m)    ((T*)((intptr_t)(p) - CUPKEE_MEMBER_OFFSET(T, m)))

#define CUPKEE_SIZE_ALIGN(v, a)         (((size_t)(v) + ((a) - 1)) & ~((a) - 1))
#define CUPKEE_ADDR_ALIGN(p, a)         (void *)(((intptr_t)(p) + ((a) - 1)) & ~(intptr_t)((a) - 1))

typedef int (*cupkee_callback_t)(int id, int event, intptr_t param);

/* Todo: put to User configure ? */
#define APP_DEV_MAX                     8

/* Cupkee api */
void cupkee_init(void);
void cupkee_loop(void);
void cupkee_event_poll(void);

#include "cupkee_bsp.h"
#include "cupkee_errno.h"
#include "cupkee_utils.h"
#include "cupkee_memory.h"
#include "cupkee_event.h"
#include "cupkee_vector.h"
#include "cupkee_stream.h"
#include "cupkee_block.h"
#include "cupkee_buffer.h"
#include "cupkee_process.h"
#include "cupkee_struct.h"
#include "cupkee_object.h"

#include "cupkee_timer.h"

#include "cupkee_timeout.h"
#include "cupkee_device.h"
#include "cupkee_console.h"
#include "cupkee_auto_complete.h"
#include "cupkee_history.h"
#include "cupkee_command.h"
#include "cupkee_shell.h"
#include "cupkee_module.h"
#include "cupkee_native.h"

static inline void cupkee_start(void) {
    _cupkee_systicks = 0;
}

static inline void cupkee_poll(void) {
    cupkee_device_poll();
    cupkee_event_poll();
}

#define CUPKEE_SYSDISK_SECTOR_COUNT		1024 * 32
int cupkee_sysdisk_read(uint32_t lba, uint8_t *copy_to);
int cupkee_sysdisk_write(uint32_t lba, const uint8_t *copy_from);

#endif /* __CUPKEE_INC__ */

