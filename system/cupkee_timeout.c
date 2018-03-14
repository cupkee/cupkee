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

#include <cupkee.h>

#define TIMEOUT_FL_REPEAT 1

static cupkee_timeout_t *timeout_head = NULL;
static int timeout_next = 0;

static int timeout_clear_by(int (*fn)(cupkee_timeout_t *, int), int x)
{
    cupkee_timeout_t *prev = NULL, *curr = timeout_head;
    int n = 0;

    while (curr) {
        cupkee_timeout_t *next = curr->next;

        if (fn(curr, x)) {
            if (prev) {
                prev->next = next;
            } else {
                timeout_head = next;
            }

            curr->handle(1, curr->param); // drop timer
            cupkee_free(curr);
            n ++;
        } else {
            prev = curr;
        }

        curr = next;
    }
    return n;
}

static int timeout_with_flag(cupkee_timeout_t *t, int flags)
{
    return t->flags == flags;
}

static int timeout_with_id(cupkee_timeout_t *t, int id)
{
    return t->id == id;
}

void cupkee_timeout_setup(void)
{
    timeout_head = NULL;
    timeout_next = 0;
}

void cupkee_timeout_sync(uint32_t curr_ticks)
{
    cupkee_timeout_t *prev = NULL, *curr = timeout_head;

    while (curr) {
        cupkee_timeout_t *next = curr->next;

        if (curr_ticks - curr->from >= curr->wait) {
            curr->handle(0, curr->param);       // wake up

            if (curr->flags & TIMEOUT_FL_REPEAT) {
                curr->from = curr_ticks;
                prev = curr;
            } else {
                curr->handle(1, curr->param);   // drop
                if (prev) {
                    prev->next = next;
                } else {
                    // Current timer is header
                    timeout_head = next;
                }
                cupkee_free(curr);
            }
        } else {
            prev = curr;
        }
        curr = next;
    }
}

cupkee_timeout_t *cupkee_timeout_register(uint32_t wait, int repeat, cupkee_timeout_handle_t handle, void *param)
{
    cupkee_timeout_t *t;

    if (!handle) {
        return NULL;
    }

    t = cupkee_malloc(sizeof(cupkee_timeout_t));
    if (t) {
        t->handle = handle;
        t->param  = param;
        t->id     = timeout_next++;
        t->wait   = wait;
        t->from   = _cupkee_systicks;
        t->flags  = repeat ? TIMEOUT_FL_REPEAT : 0;

        t->next = timeout_head;
        timeout_head = t;
    }

    return t;
}

void cupkee_timeout_unregister(cupkee_timeout_t *t)
{
    cupkee_timeout_t *prev = NULL, *curr = timeout_head;

    while (curr) {
        if (curr == t) {
            if (prev) {
                prev->next = curr->next;
            } else {
                timeout_head = curr->next;
            }

            curr->handle(1, curr->param); // drop timer
            cupkee_free(curr);

            return;
        }

        prev = curr;
        curr = curr->next;
    }
}

int cupkee_timeout_clear_all(void)
{
    cupkee_timeout_t *curr = timeout_head;
    int n = 0;

    timeout_head = NULL;
    while (curr) {
        cupkee_timeout_t *next = curr->next;

        curr->handle(1, curr->param); // drop timer
        cupkee_free(curr);

        curr = next;
        n ++;
    }

    return n;
}

int cupkee_timeout_clear_with_flags(uint32_t flags)
{
    return timeout_clear_by(timeout_with_flag, flags);
}

int cupkee_timeout_clear_with_id(uint32_t id)
{
    return timeout_clear_by(timeout_with_id, id);
}

volatile uint32_t _cupkee_systicks;

