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

#define OBJECT_PTR(p)    CUPKEE_CONTAINER_OF(p, cupkee_object_t, data)

typedef struct cupkee_meta_t {
    void (*destroy) (void *data);
    void (*listen) (void *data, int event);
    void (*ignore) (void *data, int event);

    void (*event_handle) (int, uint8_t event);

    cupkee_stream_t *(*streaming) (void *data);
} cupkee_meta_t;

typedef struct cupkee_object_t {
    list_head_t list;

    uint8_t tag;
    uint8_t err;
    uint16_t flags;

    uint16_t id;
    uint16_t ref;

    uint8_t data[0];
} cupkee_object_t;

int  cupkee_object_setup(void);
void cupkee_object_event_dispatch(uint16_t which, uint8_t code);

static inline void cupkee_object_event_post(int id, uint8_t code) {
    cupkee_event_post(EVENT_OBJECT, code, id);
}

int cupkee_object_register(size_t size, const cupkee_meta_t *meta);

cupkee_object_t *cupkee_object_create(int tag);
void cupkee_object_destroy(cupkee_object_t *obj);

void cupkee_object_error_set(cupkee_object_t *obj, int err);
int  cupkee_object_error_get(cupkee_object_t *obj);

void cupkee_object_listen(cupkee_object_t *obj, int event);
void cupkee_object_ignore(cupkee_object_t *obj, int event);
int  cupkee_object_read(cupkee_object_t *obj, size_t n, void *buf);
int  cupkee_object_read_sync(cupkee_object_t *obj, size_t n, void *buf);
int  cupkee_object_write(cupkee_object_t *obj, size_t n, const void *data);
int  cupkee_object_write_sync(cupkee_object_t *obj, size_t n, const void *data);


int  cupkee_id(int tag);
void cupkee_release(int id);
int  cupkee_tag(int id);
void *cupkee_data(int id, uint8_t tag);

void cupkee_error_set(int id, int err);
int  cupkee_error_get(int id);

void cupkee_listen(int id, int event);
void cupkee_ignore(int id, int event);
int  cupkee_read(int id, size_t n, void *buf);
int  cupkee_read_sync(int id, size_t n, void *buf);
int  cupkee_write(int id, size_t n, const void *data);
int  cupkee_write_sync(int id, size_t n, const void *data);

#endif /* __CUPKEE_OBJECT_INC__ */

