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

#ifndef __CUPKEE_TIMER_INC__
#define __CUPKEE_TIMER_INC__

typedef struct cupkee_timer_t {
    uint8_t inst;
    uint8_t state;
    uint32_t period;

    cupkee_callback_t cb;
    intptr_t          cb_param;
} cupkee_timer_t;

#define CUPKEE_TIMER_KEEP 0
#define CUPKEE_TIMER_STOP (-1)

enum {
    CUPKEE_TIMER_STATE_IDLE = 0,
    CUPKEE_TIMER_STATE_RUNNING,
};

int cupkee_timer_setup(void);
int cupkee_timer_tag(void);

cupkee_timer_t *cupkee_timer_request(cupkee_callback_t cb, intptr_t param);
int cupkee_timer_release(cupkee_timer_t *timer);
int cupkee_timer_state(cupkee_timer_t *timer);

int cupkee_timer_start(cupkee_timer_t *timer, int us);
int cupkee_timer_stop(cupkee_timer_t *timer);
int cupkee_timer_duration(cupkee_timer_t *timer);

int cupkee_is_timer(void *entry);

static inline intptr_t cupkee_timer_callback_param(void *timer) {
    cupkee_timer_t *t = timer;

    return t->cb_param;
};

// Should only be call in BSP
static inline void cupkee_timer_rewind(int id)
{
    cupkee_object_event_post(id, CUPKEE_EVENT_REWIND);
}

#endif /* __CUPKEE_TIMER_INC__ */

