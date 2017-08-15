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

#ifndef __CUPKEE_TIMEOUT_INC__
#define __CUPKEE_TIMEOUT_INC__

extern volatile uint32_t _cupkee_systicks;

typedef void (*cupkee_timeout_handle_t)(int drop, void *param);
typedef struct cupkee_timeout_t {
    struct cupkee_timeout_t *next;
    cupkee_timeout_handle_t handle;
    int      id;
    int      flags;
    uint32_t wait;
    uint32_t from;
    void    *param;
} cupkee_timeout_t;

void cupkee_timeout_setup(void);
void cupkee_timeout_sync(uint32_t ticks);

cupkee_timeout_t *cupkee_timeout_register(uint32_t wait, int repeat, cupkee_timeout_handle_t handle, void *param);
void cupkee_timeout_unregister(cupkee_timeout_t *t);

int cupkee_timeout_clear_all(void);
int cupkee_timeout_clear_with_flags(uint32_t flags);
int cupkee_timeout_clear_with_id(uint32_t id);

static inline uint32_t cupkee_systicks(void) {
    return _cupkee_systicks;
}


#endif /* __CUPKEE_TIMEOUT_INC__ */

