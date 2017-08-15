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

#ifndef __CUPKEE_OBJECT_INC__
#define __CUPKEE_OBJECT_INC__

typedef struct cupkee_meta_t {
    void (*event_handle) (int id, uint8_t event);
} cupkee_meta_t;

int  cupkee_object_setup(void);
void cupkee_object_event_dispatch(uint16_t which, uint8_t code);

static inline void cupkee_object_event_post(int id, uint8_t code) {
    cupkee_event_post(EVENT_OBJECT, code, id);
}

int cupkee_object_register(size_t size, const cupkee_meta_t *meta);

int  cupkee_object_alloc(int tag);
void cupkee_object_release(int id);

void cupkee_object_error_set(int id, int err);
int  cupkee_object_error_get(int id);

int   cupkee_object_tag(int id);
void *cupkee_object_data(int id, uint8_t tag);

#endif /* __CUPKEE_OBJECT_INC__ */

