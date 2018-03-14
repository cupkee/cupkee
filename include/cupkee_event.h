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

