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

#define DATA_SIZE_MAX   256

static const uint8_t items_size[] = {
  1, // options
  1, // int8
  1, // uint8

  2, // int16
  2, // uint16

  4, // int32
  4, // uint32

  sizeof(double)  // float
};

#define ITEM_TYPE(i, desc) ((desc)[i].type);

static int struct_item_offset(int item, const cupkee_struct_desc_t *desc)
{
    int offset = 0, i;

    for (i = 0; i < item; i++) {
        int type = desc[i].type;

        if (type < CUPKEE_STRUCT_STR) {
            offset += items_size[type];
        } else
        if (type == CUPKEE_STRUCT_STR || type == CUPKEE_STRUCT_OCT) {
            offset += desc[i].size + 1;
        } else {
            return -1;
        }
    }

    return offset;
}

static int struct_item_info(cupkee_struct_t *conf, int id, uint8_t *type)
{
    int off;

    if (!conf || id < 0 || id >= conf->item_num) {
        return -1;
    }

    off = struct_item_offset(id, conf->item_descs);
    if (off < 0 || off >= conf->size) {
        return -1;
    }

    if (type) {
        *type = conf->item_descs[id].type;
    }

    return off;
}

static inline int struct_data_size(int item_num, const cupkee_struct_desc_t *desc)
{
    int size = struct_item_offset(item_num, desc);

    return size > 255 ? -1 : size;
}

static void *struct_number_data(cupkee_struct_t *conf, int id, uint8_t *type)
{
    uint8_t t;
    int pos;

    pos = struct_item_info(conf, id, &t);
    if (pos < 0) {
        return NULL;
    }

    if (t >= CUPKEE_STRUCT_FLOAT) {
        return 0;
    }

    if (type) {
        *type = t;
    }

    return (conf->data + pos);
}

int cupkee_struct_init(cupkee_struct_t *conf, int item_num, const cupkee_struct_desc_t *desc)
{
    int size;

    if (!item_num || !desc) {
        return -CUPKEE_EINVAL;
    }
    size = struct_data_size(item_num, desc);
    if (size < 1) {
        return -CUPKEE_EINVAL;
    }

    if (NULL == (conf->data = cupkee_malloc(size))) {
        return -CUPKEE_ENOMEM;
    }

    conf->item_descs = desc;
    conf->item_num = item_num;
    conf->size = size;
    memset(conf->data, 0, size);

    return 0;
}

void cupkee_struct_deinit(cupkee_struct_t *conf)
{
    if (conf && conf->data) {
        cupkee_free(conf->data);
        conf->data = NULL;
        conf->size = 0;
    }
}

int cupkee_struct_item_id(cupkee_struct_t *conf, const char *name)
{
    int i;

    if (!conf) {
        return -1;
    }

    for (i = 0; i < conf->item_num; i++) {
        if (!strcmp(name, conf->item_descs[i].name)) {
            return i;
        }
    }

    return -1;
}

void cupkee_struct_clear(cupkee_struct_t *conf)
{
    if (conf) {
        memset(conf->data, 0, conf->size);
    }
}

int cupkee_struct_set_int(cupkee_struct_t *conf, int id, int v)
{
    int8_t *p;
    uint8_t t;
    int i;

    if (!(p = (int8_t *) struct_number_data(conf, id, &t))) {
        return -CUPKEE_EINVAL;
    }

    for (i = items_size[t] - 1; i >= 0; i--) {
        p[i] = v;
        v = v >> 8;
    }

    return 1;
}

int cupkee_struct_get_int(cupkee_struct_t *conf, int id, int *pv)
{
    int8_t *p;
    uint8_t t;
    int v, i;

    if (!(p = (int8_t *) struct_number_data(conf, id, &t))) {
        return -CUPKEE_EINVAL;
    }

    for (v = *p, i = 1; i < items_size[t]; i++) {
        v = (v << 8) | (uint8_t)p[i];
    }
    *pv = v;

    return 1;
}

int cupkee_struct_set_uint(cupkee_struct_t *conf, int id, unsigned int v)
{
    uint8_t *p;
    uint8_t t;
    int i;

    if (!(p = (uint8_t *) struct_number_data(conf, id, &t))) {
        return -CUPKEE_EINVAL;
    }

    for (i = items_size[t] - 1; i >= 0; i--) {
        p[i] = v;
        v = v >> 8;
    }

    return 1;
}

int cupkee_struct_get_uint(cupkee_struct_t *conf, int id, unsigned int *pv)
{
    uint8_t *p;
    uint8_t t;
    unsigned v;
    int i;

    if (!(p = (uint8_t *) struct_number_data(conf, id, &t))) {
        return -CUPKEE_EINVAL;
    }

    for (v = *p, i = 1; i < items_size[t]; i++) {
        v = v << 8;
        v |= p[i];
    }
    *pv = v;

    return 1;
}

int cupkee_struct_set_float(cupkee_struct_t *conf, int id, double v)
{
    uint8_t *p;
    uint8_t t;
    int pos;

    pos = struct_item_info(conf, id, &t);
    if (pos < 0) {
        return -CUPKEE_EINVAL;
    }

    p = conf->data + pos;
    memcpy(p, &v, sizeof(double));

    return 1;
}

int cupkee_struct_get_float(cupkee_struct_t *conf, int id, double *pv)
{
    uint8_t *p;
    uint8_t t;
    int pos;

    pos = struct_item_info(conf, id, &t);
    if (pos < 0 || t != CUPKEE_STRUCT_FLOAT) {
        return -CUPKEE_EINVAL;
    }
    p = conf->data + pos;
    memcpy(pv, p, sizeof(double));

    return 1;
}

int cupkee_struct_set_string(cupkee_struct_t *conf, int id, const char *v)
{
    uint8_t t;
    int pos;

    pos = struct_item_info(conf, id, &t);
    if (pos < 0) {
        return -CUPKEE_EINVAL;
    }

    if (t == CUPKEE_STRUCT_STR) {
        char *p = (char *)(conf->data + pos);
        strncpy(p, v, conf->item_descs[id].size);
        p[conf->item_descs[id].size] = 0;

        return 1;
    } else
    if (t == CUPKEE_STRUCT_OPT) {
        uint8_t *p = conf->data + pos;
        const char **names = conf->item_descs[id].opt_names;

        if (names) {
            for (pos = 0; pos < conf->item_descs[id].size; pos++) {
                if (!strcmp(v, names[pos])) {
                    *p = pos;
                    return 1;
                }
            }
        }
    }
    return 0;
}

int cupkee_struct_get_string(cupkee_struct_t *conf, int id, const char **pv)
{
    uint8_t t;
    int pos;

    pos = struct_item_info(conf, id, &t);
    if (pos < 0 || t != CUPKEE_STRUCT_STR) {
        return -CUPKEE_EINVAL;
    }
    *pv = (const char *)(conf->data + pos);

    return 1;
}

int cupkee_struct_push(cupkee_struct_t *conf, int id, int v)
{
    uint8_t t, *p, end;
    int pos;

    pos = struct_item_info(conf, id, &t);
    if (pos < 0 || t != CUPKEE_STRUCT_OCT) {
        return -CUPKEE_EINVAL;
    }
    p = conf->data + pos;

    end = p[0];
    if (end < conf->item_descs[id].size) {
        p[1 + end] = v; p[0] ++;
        return 1;
    }

    return 0;
}

int cupkee_struct_get_bytes(cupkee_struct_t *conf, int id, const uint8_t **pv)
{
    uint8_t t, *p;
    int pos;

    pos = struct_item_info(conf, id, &t);
    if (pos < 0 || t != CUPKEE_STRUCT_OCT) {
        return -CUPKEE_EINVAL;
    }

    p = conf->data + pos;
    *pv = p + 1;

    return p[0];
}

