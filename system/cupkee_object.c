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

#define CUPKEE_OBJECT_TAG_MAX   (16)
#define CUPKEE_OBJECT_NUM_DEF   (32)

#define ID_INVALID              ((uint16_t)(-1))

typedef struct cupkee_object_info_t {
    size_t size;
    const cupkee_meta_t *meta;
} cupkee_object_info_t;

static list_head_t      obj_list_head;

static cupkee_object_t **obj_map;
static int              obj_map_size;
static int              obj_map_num;

static uint8_t              obj_tag_end;
static cupkee_object_info_t obj_infos[CUPKEE_OBJECT_TAG_MAX];

static inline const cupkee_meta_t *object_meta(cupkee_object_t *obj) {
    if (obj && obj->tag < obj_tag_end) {
        return obj_infos[obj->tag].meta;
    } else {
        return NULL;
    }
}

static int id_alloc(void)
{
    if (obj_map_num < obj_map_size) {
        int id;
        for (id = 0; id < obj_map_size; id++) {
            if (!obj_map[id]) {
                return id;
            }
        }
    }
    return -1;
}

static inline cupkee_object_t *id_object(int id) {
    if (id < 0 || id >= obj_map_size) {
        return NULL;
    }

    return obj_map[id];
}

static inline const cupkee_meta_t *id_meta(int id) {
    return object_meta(id_object(id));
}

int cupkee_object_setup(void)
{
    obj_map_size = CUPKEE_OBJECT_NUM_DEF;
    obj_map = (cupkee_object_t **)cupkee_malloc(obj_map_size * sizeof(void *));
    if (!obj_map) {
        return -1;
    }
    memset(obj_map, 0, obj_map_size * sizeof(void *));

    list_head_init(&obj_list_head);

    obj_tag_end = 0;
    memset(obj_infos, 0, CUPKEE_OBJECT_TAG_MAX * sizeof(cupkee_object_info_t));

    obj_map_num = 0;

    return 0;
}

void cupkee_object_event_dispatch(uint16_t which, uint8_t code)
{
    const cupkee_meta_t *meta = id_meta(which);

    // printf("object[%u, %p] event: %u\n", which, obj, code);
    if (meta && meta->event_handle) {
        meta->event_handle(which, code);
    }
}

int cupkee_object_register(size_t size, const cupkee_meta_t *meta)
{
    if (obj_tag_end >= CUPKEE_OBJECT_TAG_MAX) {
        return -1;
    }

    obj_infos[obj_tag_end].size = size;
    obj_infos[obj_tag_end].meta = meta;

    return obj_tag_end++;
}

cupkee_object_t *cupkee_object_create(int tag)
{
    if ((unsigned)tag < obj_tag_end) {
        cupkee_object_t *obj = (cupkee_object_t *)cupkee_malloc(sizeof(cupkee_object_t) + obj_infos[tag].size);
        if (obj) {
            obj->tag = tag;
            obj->ref = 1;
            obj->id  = ID_INVALID;

            list_add_tail(&obj->list, &obj_list_head);
        }
        return obj;
    }

    return NULL;
}

void cupkee_object_destroy(cupkee_object_t *obj)
{
    const cupkee_meta_t *meta = object_meta(obj);

    if (meta) {
        if (meta->destroy) {
            meta->destroy(obj->data);
        }

        list_del(&obj->list);

        cupkee_free(obj);
    }
}

int cupkee_id(int tag)
{
    int id;
    cupkee_object_t *obj;

    if (0 > (id = id_alloc())) {
        return -CUPKEE_ERESOURCE;
    }

    if (!(obj = cupkee_object_create(tag))) {
        return -CUPKEE_ENOMEM;
    }

    obj->id = id;
    obj_map[id] = obj;

    return id;
}

void cupkee_release(int id)
{
    cupkee_object_t *obj = id_object(id);

    if (obj) {
        cupkee_object_destroy(obj);
        obj_map[id] = NULL;
    }
}

void *cupkee_data(int id, uint8_t tag)
{
    cupkee_object_t *obj = id_object(id);

    if (obj && obj->tag == tag) {
        return obj->data;
    }

    return NULL;
}

int cupkee_tag(int id)
{
    cupkee_object_t *obj = id_object(id);

    if (obj) {
        return obj->tag;
    }

    return -1;
}

void cupkee_error_set(int id, int err)
{
    cupkee_object_t *obj = id_object(id);

    if (obj) {
        obj->err = err;
        cupkee_object_event_post(id, CUPKEE_EVENT_ERROR);
    }
}

int cupkee_error_get(int id)
{
    cupkee_object_t *obj = id_object(id);

    if (obj) {
        return obj->err;
    } else {
        return CUPKEE_EINVAL;
    }
}

void cupkee_listen(int id, int event)
{
    const cupkee_meta_t *meta = id_meta(id);

    if (meta && meta->listen) {
        meta->listen(id, event);
    }
}

void cupkee_ignore(int id, int event)
{
    const cupkee_meta_t *meta = id_meta(id);

    if (meta && meta->ignore) {
        meta->ignore(id, event);
    }
}

int cupkee_read(int id, size_t n, void *buf)
{
    const cupkee_meta_t *meta = id_meta(id);
    cupkee_stream_t *s;

    if (!meta || !n || !buf) {
        return -CUPKEE_EINVAL;
    }

    if (!meta->streaming || NULL == (s = meta->streaming(id))) {
        return -CUPKEE_EIMPLEMENT;
    }

    return cupkee_stream_read(s, n, buf);
}

int cupkee_read_sync(int id, size_t n, void *buf)
{
    const cupkee_meta_t *meta = id_meta(id);
    cupkee_stream_t *s;

    if (!meta || !n || !buf) {
        return -CUPKEE_EINVAL;
    }

    if (!meta->streaming || NULL == (s = meta->streaming(id))) {
        return -CUPKEE_EIMPLEMENT;
    }

    return cupkee_stream_read_sync(s, n, buf);
}

int cupkee_write(int id, size_t n, const void *data)
{
    const cupkee_meta_t *meta = id_meta(id);
    cupkee_stream_t *s;

    if (!meta || !n || !data) {
        return -CUPKEE_EINVAL;
    }

    if (!meta->streaming || NULL == (s = meta->streaming(id))) {
        return -CUPKEE_EIMPLEMENT;
    }

    return cupkee_stream_write(s, n, data);
}

int cupkee_write_sync(int id, size_t n, const void *data)
{
    const cupkee_meta_t *meta = id_meta(id);
    cupkee_stream_t *s;

    if (!meta || !n || !data) {
        return -CUPKEE_EINVAL;
    }

    if (!meta->streaming || NULL == (s = meta->streaming(id))) {
        return -CUPKEE_EIMPLEMENT;
    }

    return cupkee_stream_write_sync(s, n, data);
}

