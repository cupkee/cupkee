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

#include "cupkee.h"

typedef struct cupkee_timer_t {
    uint8_t inst;
    uint8_t state;
    uint32_t period;

    cupkee_callback_t cb;
    intptr_t          cb_param;
} cupkee_timer_t;

static int timer_tag = -1;

static inline cupkee_timer_t *timer_block(int id) {
    return cupkee_entry(id, timer_tag);
}

static void timer_rewind(cupkee_timer_t *timer, int id)
{
    // printf("get rewind event\n");
    if (timer && timer->cb) {
        int res = timer->cb(id, CUPKEE_EVENT_REWIND, timer->cb_param);

        if (res < 0) {
            cupkee_timer_stop(id);
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
    case CUPKEE_EVENT_DESTROY:
    case CUPKEE_EVENT_ERROR:
    case CUPKEE_EVENT_START:
        if (timer && timer->cb) {
            timer->cb(CUPKEE_ENTRY_ID(entry), code, timer->cb_param);
        }
        break;
    case CUPKEE_EVENT_REWIND:
        timer_rewind(timer, CUPKEE_ENTRY_ID(entry)); break;
    default:
        break;
    }
}

static const cupkee_meta_t timer_meta = {
    .event_handle = timer_event_handle
};

int cupkee_timer_setup(void)
{
    if (0 > (timer_tag = cupkee_object_register(sizeof(cupkee_timer_t), &timer_meta))) {
        return -1;
    }

    return 0;
}

int cupkee_timer_request(cupkee_callback_t cb, intptr_t param)
{
    cupkee_timer_t *timer;
    int8_t inst;
    int id;

    inst = hw_timer_alloc();
    if (inst < 0) {
        return -CUPKEE_ERESOURCE;
    }

    id = cupkee_id(timer_tag);
    if (id < 0 || NULL == (timer = timer_block(id))) {
        hw_timer_release(inst);
        return -CUPKEE_ENOMEM;
    }

    timer->inst = inst;
    timer->state = CUPKEE_TIMER_STATE_IDLE;
    timer->cb = cb;
    timer->cb_param = param;
    timer->period = 0;

    return id;
}

int cupkee_timer_release(int id)
{
    cupkee_timer_t *timer;

    if (NULL == (timer = timer_block(id))) {
        return -CUPKEE_EINVAL;
    }

    hw_timer_release(timer->inst);
    if (timer->cb) {
        timer->cb(id, CUPKEE_EVENT_DESTROY, timer->cb_param);
    }

    cupkee_release(id);
    return 0;
}

int cupkee_timer_state(int id)
{
    cupkee_timer_t *timer;

    if (NULL == (timer = timer_block(id))) {
        return -CUPKEE_EINVAL;
    }

    return timer->state;
}

int cupkee_timer_start(int id, int us)
{
    cupkee_timer_t *timer;

    if (NULL == (timer = timer_block(id))) {
        return -CUPKEE_EINVAL;
    }

    if (us < 1) {
        return -CUPKEE_EINVAL;
    }

    if (timer->state != CUPKEE_TIMER_STATE_IDLE) {
        return -CUPKEE_EBUSY;
    }

    timer->period = us;
    if (hw_timer_start(timer->inst, id, us)) {
        return -CUPKEE_EHARDWARE;
    }
    timer->state = CUPKEE_TIMER_STATE_RUNNING;

    if (timer->cb) {
        timer->cb(id, CUPKEE_EVENT_START, timer->cb_param);
    }
    // cupkee_object_event_post(id, CUPKEE_EVENT_START);

    return 0;
}

int cupkee_timer_stop(int id)
{
    cupkee_timer_t *timer;

    if (NULL == (timer = timer_block(id))) {
        return -CUPKEE_EINVAL;
    }

    if (0 == hw_timer_stop(timer->inst)) {
        cupkee_object_event_post(id, CUPKEE_EVENT_STOP);
    } else {
        cupkee_object_event_post(id, CUPKEE_EVENT_ERROR);
    }

    return 0;
}

int cupkee_timer_duration(int id)
{
    cupkee_timer_t *timer;

    if (NULL == (timer = timer_block(id))) {
        return -CUPKEE_EINVAL;
    }

    return hw_timer_duration_get(timer->inst);
}

intptr_t cupkee_timer_callback_param(int id)
{
    cupkee_timer_t *timer;

    if (NULL == (timer = timer_block(id))) {
        return 0;
    }

    return timer->cb_param;
}

int cupkee_is_timer(int id)
{
    return cupkee_tag(id) == timer_tag;
}

