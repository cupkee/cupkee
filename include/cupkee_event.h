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

#ifndef __CUPKEE_EVENT_INC__
#define __CUPKEE_EVENT_INC__

enum CUPKEE_EVENT_TYPE {
    EVENT_SYSTICK = 0,
    EVENT_OBJECT  = 1,
    EVENT_PIN     = 2
};

enum CUPKEE_EVENT_OBJECT {
    CUPKEE_EVENT_DESTROY = 0,
    CUPKEE_EVENT_ERROR,
    CUPKEE_EVENT_READY,
    CUPKEE_EVENT_RESPONSE,

    CUPKEE_EVENT_DATA,
    CUPKEE_EVENT_DRAIN,

    CUPKEE_EVENT_UPDATE,

    CUPKEE_EVENT_START,
    CUPKEE_EVENT_STOP,
    CUPKEE_EVENT_REWIND,
    CUPKEE_EVENT_END,

    CUPKEE_EVENT_SERVICE_SET,
    CUPKEE_EVENT_SERVICE_GET,
    CUPKEE_EVENT_SERVICE_TRIGGER,

    CUPKEE_EVENT_USER = 128,
};

enum CUPKEE_EVENT_PIN {
    CUPKEE_EVENT_PIN_IGNORE  = 0x00,
    CUPKEE_EVENT_PIN_RISING  = 0x01,
    CUPKEE_EVENT_PIN_FALLING = 0x02,
};

typedef struct cupkee_event_t {
    uint8_t type;
    uint8_t code;
    uint16_t which;
} cupkee_event_t;

typedef int  (*cupkee_event_handle_t)(cupkee_event_t *);

void cupkee_event_setup(void);
void cupkee_event_reset(void);

int cupkee_event_post(uint8_t type, uint8_t code, uint16_t which);
int cupkee_event_take(cupkee_event_t *event);

static inline int cupkee_event_post_systick(void) {
    return cupkee_event_post(EVENT_SYSTICK, 0, 0);
}

static inline int cupkee_event_post_pin(uint8_t which, uint8_t event) {
    return cupkee_event_post(EVENT_PIN, event, which);
}

#endif /* __CUPKEE_EVENT_INC__ */

