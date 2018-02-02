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

