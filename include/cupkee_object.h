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

#ifndef __CUPKEE_OBJECT_INC__
#define __CUPKEE_OBJECT_INC__

#define CUPKEE_OBJECT_PTR(p)    CUPKEE_CONTAINER_OF(p, cupkee_object_t, entry)
#define CUPKEE_ENTRY_ID(p)      (CUPKEE_OBJECT_PTR(p)->id)
#define CUPKEE_ID_INVALID       (-1)

enum cupkee_object_elem_type {
    CUPKEE_OBJECT_ELEM_NV,
    CUPKEE_OBJECT_ELEM_INT,
    CUPKEE_OBJECT_ELEM_STR,
    CUPKEE_OBJECT_ELEM_OCT,
    CUPKEE_OBJECT_ELEM_BOOL,
    CUPKEE_OBJECT_ELEM_ENTRY,
};

typedef struct cupkee_desc_t {
    const char *name;

    void (*destroy) (void *entry);

    void (*error_handle) (void *entry, int error);
    void (*event_handle) (void *entry, uint8_t event);

    void (*listen) (void *entry, int event);
    void (*ignore) (void *entry, int event);

    int (*set) (void *entry, int t, intptr_t v);

    int (*elem_set) (void *entry, int i, int t, intptr_t v);
    int (*elem_get) (void *entry, int i, intptr_t *p);

    int (*prop_set) (void *entry, const char *k, int t, intptr_t v);
    int (*prop_get) (void *entry, const char *k, intptr_t *p);

    cupkee_stream_t *(*streaming) (void *entry);
} cupkee_desc_t;

typedef struct cupkee_object_t {
    list_head_t list;

    int16_t id;
    uint8_t ref;
    uint8_t tag;

    uint8_t entry[0];
} cupkee_object_t;

int  cupkee_object_setup(void);
void cupkee_object_event_dispatch(uint16_t which, uint8_t code);

static inline int cupkee_is_object(void *entry, uint8_t tag) {
    return entry && (CUPKEE_OBJECT_PTR(entry)->tag == tag);
};

static inline void cupkee_object_event_post(int id, uint8_t code) {
    cupkee_event_post(EVENT_OBJECT, code, id);
}

int  cupkee_object_register(size_t size, const cupkee_desc_t *desc);
void cupkee_object_set_meta(int tag, void *meta);


cupkee_object_t *cupkee_object_create(int tag);
cupkee_object_t *cupkee_object_create_with_id(int tag);
void cupkee_object_destroy(cupkee_object_t *obj);

void cupkee_object_error_set(cupkee_object_t *obj, int err);

void cupkee_object_listen(cupkee_object_t *obj, int event);
void cupkee_object_ignore(cupkee_object_t *obj, int event);

int cupkee_object_read(cupkee_object_t *obj, size_t n, void *buf);
int cupkee_object_read_sync(cupkee_object_t *obj, size_t n, void *buf);
int cupkee_object_write(cupkee_object_t *obj, size_t n, const void *data);
int cupkee_object_write_sync(cupkee_object_t *obj, size_t n, const void *data);
int cupkee_object_unshift(cupkee_object_t *obj, uint8_t data);

const void *cupkee_object_meta(cupkee_object_t *obj);
const void *cupkee_object_desc(cupkee_object_t *obj);

int  cupkee_id(int tag);
void *cupkee_entry(int id, uint8_t tag);

int cupkee_release(void *entry);
int cupkee_tag(void *entry);

void cupkee_error_set(void *entry, int err);

const char *cupkee_name(void *entry);
const void *cupkee_meta(void *entry);
void cupkee_listen(void *entry, int event);
void cupkee_ignore(void *entry, int event);
int  cupkee_read(void *entry, size_t n, void *buf);
int  cupkee_read_sync(void *entry, size_t n, void *buf);
int  cupkee_write(void *entry, size_t n, const void *data);
int  cupkee_write_sync(void *entry, size_t n, const void *data);
int  cupkee_unshift(void *entry, uint8_t data);
int  cupkee_set(void *entry, int t, intptr_t data);

int  cupkee_elem_set(void *entry, int i, int t, intptr_t data);
int  cupkee_elem_get(void *entry, int i, intptr_t *p);

int  cupkee_prop_set(void *entry, const char *k, int t, intptr_t data);
int  cupkee_prop_get(void *entry, const char *k, intptr_t *p);

#endif /* __CUPKEE_OBJECT_INC__ */

