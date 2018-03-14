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

void cupkee_struct_reset(cupkee_struct_t *conf);
int cupkee_struct_item_id(cupkee_struct_t *conf, const char *name);

static inline const char *cupkee_struct_item_name(cupkee_struct_t *conf, int id) {
    return (conf && id >= 0 && id < conf->item_num) ? conf->item_descs[id].name : NULL;
};

int cupkee_struct_clear(cupkee_struct_t *conf, int id);
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

static inline int cupkee_struct_clear2(cupkee_struct_t *conf, int id) {
    return cupkee_struct_clear(conf, id);
}

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

