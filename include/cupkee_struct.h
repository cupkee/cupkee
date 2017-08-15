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

#ifndef __CUPKEE_STRUCT_INC__
#define __CUPKEE_STRUCT_INC__

enum CUPKEE_STRUCT_FLAG {
    CUPKEE_STRUCT_FL_ALLOC = 1,
};

enum CUPKEE_STRUCT_TYPE {
    CUPKEE_STRUCT_OPT,    // option

    CUPKEE_STRUCT_INT8,
    CUPKEE_STRUCT_UINT8,

    CUPKEE_STRUCT_INT16,
    CUPKEE_STRUCT_UINT16,

    CUPKEE_STRUCT_INT32,
    CUPKEE_STRUCT_UINT32,

    CUPKEE_STRUCT_FLOAT,

    CUPKEE_STRUCT_STR, // char string

    CUPKEE_STRUCT_OCT, // byte string
};

typedef struct cupkee_struct_desc_t {
    const char *name;   // item name
    uint8_t type;       // item type, see CUPKEE_STRUCT_TYPE

    uint8_t size;       // for option: means options
                        // for [byte] string: means max length
                        // for number: 0: 1Byte, 1: 2Byte, 3: 4Byte

    const char **opt_names;
} cupkee_struct_desc_t;

typedef struct cupkee_struct_t {
    const cupkee_struct_desc_t * item_descs;

    uint8_t  flags;
    uint8_t  item_num;
    uint8_t  size;

    uint8_t *data;
} cupkee_struct_t;

cupkee_struct_t *cupkee_struct_alloc(int item_num, const cupkee_struct_desc_t *desc);
void cupkee_struct_release(cupkee_struct_t *st);

int  cupkee_struct_init(cupkee_struct_t *conf, int item_num, const cupkee_struct_desc_t *desc);
void cupkee_struct_deinit(cupkee_struct_t *conf);

void cupkee_struct_clear(cupkee_struct_t *conf);
int cupkee_struct_item_id(cupkee_struct_t *conf, const char *name);

static inline const char *cupkee_struct_item_name(cupkee_struct_t *conf, int id) {
    return (conf && id >= 0 && id < conf->item_num) ? conf->item_descs[id].name : NULL;
};

int cupkee_struct_set_int(cupkee_struct_t *conf, int id, int v);
int cupkee_struct_get_int(cupkee_struct_t *conf, int id, int *pv);

int cupkee_struct_set_uint(cupkee_struct_t *conf, int id, unsigned int v);
int cupkee_struct_get_uint(cupkee_struct_t *conf, int id, unsigned int *pv);

int cupkee_struct_set_float(cupkee_struct_t *conf, int id, double v);
int cupkee_struct_get_float(cupkee_struct_t *conf, int id, double *pv);

int cupkee_struct_set_string(cupkee_struct_t *conf, int id, const char *v);
int cupkee_struct_get_string(cupkee_struct_t *conf, int id, const char **pv);

int cupkee_struct_push(cupkee_struct_t *conf, int id, int v);
int cupkee_struct_get_bytes(cupkee_struct_t *conf, int id, const uint8_t **pv);

static inline int cupkee_struct_set_int2(cupkee_struct_t *conf, const char *name, int v) {
    return cupkee_struct_set_int(conf, cupkee_struct_item_id(conf, name), v);
};

static inline int cupkee_struct_get_int2(cupkee_struct_t *conf, const char *name, int *pv) {
    return cupkee_struct_get_int(conf, cupkee_struct_item_id(conf, name), pv);
};

static inline int cupkee_struct_set_uint2(cupkee_struct_t *conf, const char *name, unsigned int v) {
    return cupkee_struct_set_uint(conf, cupkee_struct_item_id(conf, name), v);
};

static inline int cupkee_struct_get_uint2(cupkee_struct_t *conf, const char *name, unsigned int *pv) {
    return cupkee_struct_get_uint(conf, cupkee_struct_item_id(conf, name), pv);
};

static inline int cupkee_struct_set_float2(cupkee_struct_t *conf, const char *name, double v) {
    return cupkee_struct_set_float(conf, cupkee_struct_item_id(conf, name), v);
};

static inline int cupkee_struct_get_float2(cupkee_struct_t *conf, const char *name, double *pv) {
    return cupkee_struct_get_float(conf, cupkee_struct_item_id(conf, name), pv);
};

static inline int cupkee_struct_set_string2(cupkee_struct_t *conf, const char *name, const char *v) {
    return cupkee_struct_set_string(conf, cupkee_struct_item_id(conf, name), v);
};

static inline int cupkee_struct_get_string2(cupkee_struct_t *conf, const char *name, const char **pv) {
    return cupkee_struct_get_string(conf, cupkee_struct_item_id(conf, name), pv);
};

static inline int cupkee_struct_push2(cupkee_struct_t *conf, const char *name, int v) {
    return cupkee_struct_push(conf, cupkee_struct_item_id(conf, name), v);
};

static inline int cupkee_struct_get_bytes2(cupkee_struct_t *conf, const char *name, const uint8_t **pv) {
    return cupkee_struct_get_bytes(conf, cupkee_struct_item_id(conf, name), pv);
};

#endif /* __CUPKEE_STRUCT_INC__ */

