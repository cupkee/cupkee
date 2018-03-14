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

#include "cupkee.h"
#include "rbuff.h"

#define EVENTQ_SIZE         16
#define EMITTER_CODE_MAX    65535

static rbuff_t eventq;
static cupkee_event_t eventq_mem[EVENTQ_SIZE];

void cupkee_event_setup(void)
{
    rbuff_init(&eventq, EVENTQ_SIZE);
}

void cupkee_event_reset(void)
{
    rbuff_reset(&eventq);
}

int cupkee_event_post(uint8_t type, uint8_t code, uint16_t which)
{
    uint32_t state;
    int pos;

    hw_enter_critical(&state);
    pos = rbuff_push(&eventq);
    hw_exit_critical(state);

    if (pos < 0) {
        return 0;
    }

    eventq_mem[pos].type  = type;
    eventq_mem[pos].code  = code;
    eventq_mem[pos].which = which;

    return 1;
}

int cupkee_event_take(cupkee_event_t *e)
{
    uint32_t state;
    int pos;

    hw_enter_critical(&state);
    pos = rbuff_shift(&eventq);
    hw_exit_critical(state);

    if (pos < 0) {
        return 0;
    }

    *e = eventq_mem[pos];

    return 1;
}

