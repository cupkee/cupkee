/*
MIT License

This file is part of cupkee project.

Copyright (C) 2017 Lixing Ding <ding.lixing@gmail.com>

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

void cupkee_timeout_init(void)
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

