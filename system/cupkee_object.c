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

#include "cupkee.h"

#define CUPKEE_OBJECT_TAG_MAX   (16)
#define CUPKEE_OBJECT_NUM_DEF   (32)

typedef struct cupkee_object_info_t {
    size_t size;
    const cupkee_desc_t *desc;
    void *meta;
} cupkee_object_info_t;

static list_head_t      obj_list_head;

static cupkee_object_t **obj_map;
static int              obj_map_size;
static int              obj_map_num;

static uint8_t              obj_tag_end;
static cupkee_object_info_t obj_infos[CUPKEE_OBJECT_TAG_MAX];

static inline const cupkee_desc_t *object_desc(cupkee_object_t *obj) {
    if (obj && obj->tag < obj_tag_end) {
        return obj_infos[obj->tag].desc;
    } else {
        return NULL;
    }
}

static inline void object_map(cupkee_object_t *obj, int id)
{
    if (!obj_map[id]) {
        obj->id = id;
        obj_map[id] = obj;
        ++obj_map_num;
    }
}

static inline void object_unmap(cupkee_object_t *obj)
{
    if (obj->id < obj_map_size && obj == obj_map[obj->id]) {
        obj_map[obj->id] = NULL;
        --obj_map_num;
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
    if ((unsigned)id >= (unsigned)obj_map_size) {
        return NULL;
    }

    return obj_map[id];
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

void cupkee_object_event_dispatch(uint16_t id, uint8_t code)
{
    cupkee_object_t *obj = id_object(id);
    const cupkee_desc_t *desc = object_desc(obj);

    if (code == CUPKEE_EVENT_DESTROY) {
        cupkee_object_destroy(obj);
    } else
    // printf("object[%u, %p] event: %u\n", which, obj, code);
    if (desc && desc->event_handle) {
        desc->event_handle(obj->entry, code);
    }
}

int cupkee_object_register(size_t size, const cupkee_desc_t *desc)
{
    if (obj_tag_end >= CUPKEE_OBJECT_TAG_MAX) {
        return -1;
    }

    obj_infos[obj_tag_end].size = size;
    obj_infos[obj_tag_end].desc = desc;

    return obj_tag_end++;
}

void cupkee_object_set_meta(int tag, void *meta)
{
    if (tag < obj_tag_end) {
        obj_infos[tag].meta = meta;
    }
}

const void *cupkee_object_desc(cupkee_object_t *obj)
{
    if (obj && obj->tag < obj_tag_end) {
        return obj_infos[obj->tag].desc;
    } else {
        return NULL;
    }
}

const void *cupkee_object_meta(cupkee_object_t *obj)
{
    if (obj && obj->tag < obj_tag_end) {
        return obj_infos[obj->tag].meta;
    } else {
        return NULL;
    }
}

cupkee_object_t *cupkee_object_create(int tag)
{
    if ((unsigned)tag < obj_tag_end) {
        cupkee_object_t *obj = (cupkee_object_t *)cupkee_malloc(sizeof(cupkee_object_t) + obj_infos[tag].size);
        if (obj) {
            obj->tag = tag;
            obj->ref = 1;
            obj->id  = CUPKEE_ID_INVALID;

            list_add_tail(&obj->list, &obj_list_head);
        }
        return obj;
    }

    return NULL;
}

cupkee_object_t *cupkee_object_create_with_id(int tag)
{
    int id;
    cupkee_object_t *obj;

    if (0 > (id = id_alloc())) {
        return NULL;
    }

    if (!(obj = cupkee_object_create(tag))) {
        return NULL;
    } else {
        object_map(obj, id);
        return obj;
    }
}

void cupkee_object_destroy(cupkee_object_t *obj)
{
    const cupkee_desc_t *desc = object_desc(obj);

    if (desc) {
        if (desc->destroy) {
            desc->destroy(obj->entry);
        }
        object_unmap(obj);

        list_del(&obj->list);

        cupkee_free(obj);
    }
}

void cupkee_object_error_set(cupkee_object_t *obj, int err)
{
    if (obj) {
        const cupkee_desc_t *desc = object_desc(obj);
        if (desc->error_handle) {
            desc->error_handle(obj->entry, err);
        }
        if (obj->id != CUPKEE_ID_INVALID) {
            cupkee_object_event_post(obj->id, CUPKEE_EVENT_ERROR);
        }
    }
}

void cupkee_object_listen(cupkee_object_t *obj, int event)
{
    const cupkee_desc_t *desc = object_desc(obj);

    if (desc && desc->listen) {
        desc->listen(obj->entry, event);
    }
}

void cupkee_object_ignore(cupkee_object_t *obj, int event)
{
    const cupkee_desc_t *desc = object_desc(obj);

    if (desc && desc->ignore) {
        desc->ignore(obj->entry, event);
    }
}

int  cupkee_object_read(cupkee_object_t *obj, size_t n, void *buf)
{
    const cupkee_desc_t *desc = object_desc(obj);
    cupkee_stream_t *s;

    if (!desc || !n || !buf) {
        return -CUPKEE_EINVAL;
    }

    if (!desc->streaming || NULL == (s = desc->streaming(obj->entry))) {
        return -CUPKEE_EIMPLEMENT;
    }

    return cupkee_stream_read(s, n, buf);
}

int  cupkee_object_read_sync(cupkee_object_t *obj, size_t n, void *buf)
{
    const cupkee_desc_t *desc = object_desc(obj);
    cupkee_stream_t *s;

    if (!desc || !n || !buf) {
        return -CUPKEE_EINVAL;
    }

    if (!desc->streaming || NULL == (s = desc->streaming(obj->entry))) {
        return -CUPKEE_EIMPLEMENT;
    }

    return cupkee_stream_read_sync(s, n, buf);
}

int cupkee_object_unshift(cupkee_object_t *obj, uint8_t data)
{
    const cupkee_desc_t *desc = object_desc(obj);
    cupkee_stream_t *s;

    if (!desc) {
        return -CUPKEE_EINVAL;
    }

    if (!desc->streaming || NULL == (s = desc->streaming(obj->entry))) {
        return -CUPKEE_EIMPLEMENT;
    }

    return cupkee_stream_unshift(s, data);
}

int  cupkee_object_write(cupkee_object_t *obj, size_t n, const void *data)
{
    const cupkee_desc_t *desc = object_desc(obj);
    cupkee_stream_t *s;

    if (!desc || !n || !data) {
        return -CUPKEE_EINVAL;
    }

    if (!desc->streaming || NULL == (s = desc->streaming(obj->entry))) {
        return -CUPKEE_EIMPLEMENT;
    }

    return cupkee_stream_write(s, n, data);
}

int  cupkee_object_write_sync(cupkee_object_t *obj, size_t n, const void *data)
{
    const cupkee_desc_t *desc = object_desc(obj);
    cupkee_stream_t *s;

    if (!desc || !n || !data) {
        return -CUPKEE_EINVAL;
    }

    if (!desc->streaming || NULL == (s = desc->streaming(obj->entry))) {
        return -CUPKEE_EIMPLEMENT;
    }

    return cupkee_stream_write_sync(s, n, data);
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

    object_map(obj, id);

    return id;
}

void cupkee_release(void *entry)
{
    cupkee_object_t *obj = CUPKEE_OBJECT_PTR(entry);

    if (obj) {
        cupkee_object_destroy(obj);
    }
}

void *cupkee_entry(int id, uint8_t tag)
{
    cupkee_object_t *obj = id_object(id);

    if (obj && obj->tag == tag) {
        return obj->entry;
    }

    return NULL;
}

int cupkee_tag(void *entry)
{
    cupkee_object_t *obj = CUPKEE_OBJECT_PTR(entry);

    if (obj) {
        return obj->tag;
    }

    return -1;
}

void cupkee_error_set(void *entry, int err)
{
    cupkee_object_error_set(CUPKEE_OBJECT_PTR(entry), err);
}

void cupkee_listen(void *entry, int event)
{
    cupkee_object_listen(CUPKEE_OBJECT_PTR(entry), event);
}

void cupkee_ignore(void *entry, int event)
{
    cupkee_object_ignore(CUPKEE_OBJECT_PTR(entry), event);
}

int cupkee_read(void *entry, size_t n, void *buf)
{
    return cupkee_object_read(CUPKEE_OBJECT_PTR(entry), n, buf);
}

int cupkee_read_sync(void *entry, size_t n, void *buf)
{
    return cupkee_object_read_sync(CUPKEE_OBJECT_PTR(entry), n, buf);
}

int cupkee_write(void *entry, size_t n, const void *data)
{
    return cupkee_object_write(CUPKEE_OBJECT_PTR(entry), n, data);
}

int cupkee_write_sync(void *entry, size_t n, const void *data)
{
    return cupkee_object_write_sync(CUPKEE_OBJECT_PTR(entry), n, data);
}

int cupkee_unshift(void *entry, uint8_t data)
{
    return cupkee_object_unshift(CUPKEE_OBJECT_PTR(entry), data);
}

int  cupkee_elem_set(void *entry, int i, int t, intptr_t data)
{
    const cupkee_desc_t *desc = object_desc(CUPKEE_OBJECT_PTR(entry));

    if (!desc) {
        return -CUPKEE_EINVAL;
    }

    if (!desc->elem_set) {
        return -CUPKEE_EIMPLEMENT;
    }

    return desc->elem_set(entry, i, t, data);
}

int  cupkee_prop_set(void *entry, const char *k, int t, intptr_t data)
{
    const cupkee_desc_t *desc = object_desc(CUPKEE_OBJECT_PTR(entry));

    if (!desc || !k) {
        return -CUPKEE_EINVAL;
    }

    if (!desc->prop_set) {
        return -CUPKEE_EIMPLEMENT;
    }

    return desc->prop_set(entry, k, t, data);
}


int  cupkee_elem_get(void *entry, int i, intptr_t *p)
{
    const cupkee_desc_t *desc = object_desc(CUPKEE_OBJECT_PTR(entry));

    if (!desc || !p) {
        return -CUPKEE_EINVAL;
    }

    if (!desc->elem_get) {
        return -CUPKEE_EIMPLEMENT;
    }

    return desc->elem_get(entry, i, p);
}

int  cupkee_prop_get(void *entry, const char *k, intptr_t *p)
{
    const cupkee_desc_t *desc = object_desc(CUPKEE_OBJECT_PTR(entry));

    if (!desc || !k || !p) {
        return -CUPKEE_EINVAL;
    }

    if (!desc->prop_get) {
        return -CUPKEE_EIMPLEMENT;
    }

    return desc->prop_get(entry, k, p);
}

