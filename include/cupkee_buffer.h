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

#ifndef __CUPKEE_BUFFER_INC__
#define __CUPKEE_BUFFER_INC__

#define CUPKEE_FLAG_OWNED    0x80

typedef struct cupkee_buffer_t {
    uint16_t flags;
    uint16_t cap;
    uint16_t bgn;
    uint16_t len;
    uint8_t  *ptr;
} cupkee_buffer_t;

void cupkee_buffer_setup(void);

static inline void cupkee_buffer_reset(cupkee_buffer_t *b) {
    b->flags = 0;
    b->cap = 0;
    b->len = 0;
    b->bgn = 0;
    b->ptr = 0;
}

static inline void cupkee_buffer_init(cupkee_buffer_t *b, size_t size, void *ptr, int flags) {
    b->flags = flags;
    b->cap = b->len = size;
    b->ptr = ptr;
    b->bgn = 0;
}

static inline int cupkee_buffer_alloc(cupkee_buffer_t *b, size_t size) {
    void *ptr = cupkee_malloc(size);

    if (ptr) {
        b->flags = CUPKEE_FLAG_OWNED;
        b->ptr = ptr;
        b->cap = size;
        b->bgn = b->len = 0;
        return size;
    } else {
        cupkee_buffer_reset(b);
        return 0;
    }
}

static inline void cupkee_buffer_deinit(cupkee_buffer_t *b) {
    if ((b->flags & CUPKEE_FLAG_OWNED) && b->ptr) {
        cupkee_free(b->ptr);
        cupkee_buffer_reset(b);
    }
}

static inline int cupkee_buffer_is_empty(cupkee_buffer_t *b) {
    return b->len == 0;
}

static inline int cupkee_buffer_is_full(cupkee_buffer_t *b) {
    return b->len == b->cap;
}

static inline size_t cupkee_buffer_capacity(cupkee_buffer_t *b) {
    return b->cap;
}

static inline size_t cupkee_buffer_space(cupkee_buffer_t *b) {
    return b->cap - b->len;
}

static inline size_t cupkee_buffer_length(cupkee_buffer_t *b) {
    return b->len;
}

static inline void *cupkee_buffer_ptr(cupkee_buffer_t *b) {
    return b->ptr;
}

int cupkee_buffer_xxx(cupkee_buffer_t *b, void **pptr);

int cupkee_buffer_space_to(cupkee_buffer_t *b, size_t n);

int cupkee_buffer_set(cupkee_buffer_t *b, int offset, uint8_t d);
int cupkee_buffer_get(cupkee_buffer_t *b, int offset, uint8_t *d);
int cupkee_buffer_push(cupkee_buffer_t *b, uint8_t d);
int cupkee_buffer_pop(cupkee_buffer_t *b, uint8_t *d);
int cupkee_buffer_unshift(cupkee_buffer_t *b, uint8_t d);
int cupkee_buffer_shift(cupkee_buffer_t *b, uint8_t *d);

int cupkee_buffer_take(cupkee_buffer_t *b, size_t n, void *buf);
int cupkee_buffer_give(cupkee_buffer_t *b, size_t n, const void *buf);

/*
void *cupkee_buffer_slice(cupkee_buffer_t *b, int start, int n);
void *cupkee_buffer_copy(cupkee_buffer_t *b);
void *cupkee_buffer_sort(cupkee_buffer_t *b);
void *cupkee_buffer_reverse(cupkee_buffer_t *b);
*/

int cupkee_buffer_read_int8  (cupkee_buffer_t *b, int offset, int8_t *i);
int cupkee_buffer_read_uint8 (cupkee_buffer_t *b, int offset, uint8_t *u);

int cupkee_buffer_read_int16_le  (cupkee_buffer_t *b, int offset, int16_t *i);
int cupkee_buffer_read_int16_be  (cupkee_buffer_t *b, int offset, int16_t *i);

int cupkee_buffer_read_uint16_le (cupkee_buffer_t *b, int offset, uint16_t *u);
int cupkee_buffer_read_uint16_be (cupkee_buffer_t *b, int offset, uint16_t *u);

int cupkee_buffer_read_int32_le  (cupkee_buffer_t *b, int offset, int32_t *i);
int cupkee_buffer_read_int32_be  (cupkee_buffer_t *b, int offset, int32_t *i);

int cupkee_buffer_read_uint32_le (cupkee_buffer_t *b, int offset, uint32_t *u);
int cupkee_buffer_read_uint32_be (cupkee_buffer_t *b, int offset, uint32_t *u);

int cupkee_buffer_read_float_be(cupkee_buffer_t *b, int offset, float *f);
int cupkee_buffer_read_float_le(cupkee_buffer_t *b, int offset, float *f);
int cupkee_buffer_read_double_be(cupkee_buffer_t *b, int offset, double *d);
int cupkee_buffer_read_double_le(cupkee_buffer_t *b, int offset, double *d);

#endif /* __CUPKEE_BUFFER_INC__ */

