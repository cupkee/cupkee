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

typedef struct cupkee_object_t {
    list_head_t list;

    uint8_t tag;
    uint8_t err;
    uint8_t res[2];

    uint16_t id;
    uint16_t ref;

    uint8_t data[0];
} cupkee_object_t;

typedef struct cupkee_object_info_t {
    size_t size;
    const cupkee_meta_t *meta;
} cupkee_object_info_t;

static list_head_t      obj_list_head;
static cupkee_object_t **obj_map;
static int              obj_map_size;
static int              obj_num;

static uint8_t              obj_tag_end;
static cupkee_object_info_t obj_infos[CUPKEE_OBJECT_TAG_MAX];

static inline cupkee_object_t *id_2_block(int id) {
    if (id < 0 || id >= obj_map_size) {
        return NULL;
    }

    return obj_map[id];
}

static void object_destroy(cupkee_object_t *obj)
{
    // Todo call meta.destroy();

    cupkee_free(obj);
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

    obj_num = 0;

    return 0;
}

void cupkee_object_event_dispatch(uint16_t which, uint8_t code)
{
    cupkee_object_t *obj = id_2_block(which);

    // printf("object[%u, %p] event: %u\n", which, obj, code);
    if (obj && obj->tag < obj_tag_end) {
        cupkee_object_info_t *info = &obj_infos[obj->tag];

        if (info && info->meta && info->meta->event_handle) {
            info->meta->event_handle(which, code);
        }
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

int cupkee_object_alloc(int tag)
{
    cupkee_object_t *obj;
    int id;

    if (tag >= obj_tag_end) {
        return -1;
    }

    if (obj_num >= obj_map_size) {
        return -1;
    }

    obj = (cupkee_object_t *)cupkee_malloc(sizeof(cupkee_object_t) + obj_infos[tag].size);
    if (!obj) {
        return -1;
    }

    for (id = 0; id < obj_map_size; id++) {
        if (!obj_map[id]) {
            obj_map[id] = obj;
            break;
        }
    }

    obj->id  = id;
    obj->tag = tag;
    obj->ref = 1;

    list_add_tail(&obj->list, &obj_list_head);

    return id;
}

void cupkee_object_release(int id)
{
    cupkee_object_t *obj = id_2_block(id);

    if (obj) {
         if (obj_map[obj->id] != obj) {
             printf("hello!\n");
         }

         obj_map[obj->id] = NULL;
         list_del(&obj->list);

         object_destroy(obj);
    }
}

void *cupkee_object_data(int id)
{
    cupkee_object_t *obj = id_2_block(id);

    if (obj) {
        return obj->data;
    }

    return NULL;
}

int cupkee_object_tag(int id)
{
    cupkee_object_t *obj = id_2_block(id);

    if (obj) {
        return obj->tag;
    }

    return -1;
}

void cupkee_object_error_set(int id, int err)
{
    cupkee_object_t *obj = id_2_block(id);

    if (obj) {
        obj->err = err;
        cupkee_object_event_post(id, CUPKEE_EVENT_ERROR);
    }
}

int cupkee_object_error_get(int id)
{
    cupkee_object_t *obj = id_2_block(id);

    if (obj) {
        return obj->err;
    } else {
        return CUPKEE_EINVAL;
    }
}

