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

#define is_timer(t)  cupkee_is_object((t), timer_tag)

static uint8_t timer_tag = -1;

static void timer_do_rewind(cupkee_timer_t *timer)
{
    // printf("get rewind event\n");
    if (timer && timer->cb) {
        int res = timer->cb(timer, CUPKEE_EVENT_REWIND, timer->cb_param);

        if (res < 0) {
            cupkee_timer_stop(timer);
        } else
        if (res > 0) {
            hw_timer_update(timer->inst, res);
        }
    }
}

static void timer_event_handle(void *entry, uint8_t code)
{
    cupkee_timer_t *timer = (cupkee_timer_t *)entry;

    // printf("get timer event\n");
    switch (code) {
    case CUPKEE_EVENT_STOP:
        timer->state = CUPKEE_TIMER_STATE_IDLE;
        // no break;
    case CUPKEE_EVENT_ERROR:
    case CUPKEE_EVENT_START:
        if (timer && timer->cb) {
            timer->cb(entry, code, timer->cb_param);
        }
        break;
    case CUPKEE_EVENT_REWIND:
        timer_do_rewind(timer); break;
    default:
        break;
    }
}

static int timer_get_prop (void *entry, const char *k, intptr_t *p)
{
    cupkee_timer_t *t = entry;

    if (t && !strcmp(k, "duration")) {
        *p = cupkee_timer_duration(t);
        return CUPKEE_OBJECT_ELEM_INT;
    } else {
        return CUPKEE_OBJECT_ELEM_NV;
    }
}

static void timer_destroy(void *entry)
{
    cupkee_timer_t *timer = (cupkee_timer_t *)entry;

    hw_timer_release(timer->inst);
    if (timer->cb) {
        timer->cb(entry, CUPKEE_EVENT_DESTROY, timer->cb_param);
    }
}

static const cupkee_desc_t timer_desc = {
    .name         = "Timer",

    .destroy      = timer_destroy,
    .event_handle = timer_event_handle,

    .prop_get     = timer_get_prop,
};

int cupkee_timer_setup(void)
{
    if (0 >= (timer_tag = cupkee_object_register(sizeof(cupkee_timer_t), &timer_desc))) {
        return -1;
    }

    return 0;
}

cupkee_timer_t *cupkee_timer_request(cupkee_callback_t cb, intptr_t param)
{
    cupkee_timer_t *timer;
    cupkee_object_t *obj;
    int8_t inst;

    inst = hw_timer_alloc();
    if (inst < 0) {
        return NULL;
    }

    obj = cupkee_object_create_with_id(timer_tag);
    if (!obj) {
        hw_timer_release(inst);
        return NULL;
    }

    timer = (cupkee_timer_t *)obj->entry;
    timer->inst = inst;
    timer->state = CUPKEE_TIMER_STATE_IDLE;
    timer->cb = cb;
    timer->cb_param = param;
    timer->period = 0;

    return timer;
}

int cupkee_timer_state(void *entry)
{
    if (!is_timer(entry)) {
        return -CUPKEE_EINVAL;
    } else {
        cupkee_timer_t *timer = entry;
        return timer->state;
    }
}

int cupkee_timer_start(void *entry, int us)
{
    if (!is_timer(entry) || us < 1) {
        return -CUPKEE_EINVAL;
    } else {
        cupkee_timer_t *timer = entry;

        if (timer->state != CUPKEE_TIMER_STATE_IDLE) {
            return -CUPKEE_EBUSY;
        }

        timer->period = us;
        if (hw_timer_start(timer->inst, CUPKEE_ENTRY_ID(timer), us)) {
            return -CUPKEE_EHARDWARE;
        }
        timer->state = CUPKEE_TIMER_STATE_RUNNING;

        if (timer->cb) {
            timer->cb(timer, CUPKEE_EVENT_START, timer->cb_param);
        }
        // cupkee_object_event_post(cupkee_entry_id(timer), CUPKEE_EVENT_START);

        return 0;
    }
}

int cupkee_timer_stop(cupkee_timer_t *timer)
{
    if (!is_timer(timer)) {
        return -CUPKEE_EINVAL;
    }

    if (0 == hw_timer_stop(timer->inst)) {
        cupkee_object_event_post(CUPKEE_ENTRY_ID(timer), CUPKEE_EVENT_STOP);
    } else {
        cupkee_object_event_post(CUPKEE_ENTRY_ID(timer), CUPKEE_EVENT_ERROR);
    }

    return 0;
}

int cupkee_timer_duration(cupkee_timer_t *timer)
{
    if (!is_timer(timer)) {
        return -CUPKEE_EINVAL;
    }

    return hw_timer_duration_get(timer->inst);
}

int cupkee_timer_tag(void)
{
    return timer_tag;
}

int cupkee_is_timer(void *p)
{
    return is_timer(p);
}

