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

typedef struct cupkee_buffer_t {
    uint16_t flags;
    uint16_t cap;
    uint16_t bgn;
    uint16_t len;
    uint8_t  ptr[0];
} cupkee_buffer_t;

void cupkee_buffer_reset(void *p)
{
    cupkee_buffer_t *b = (cupkee_buffer_t *)p;

    b->len = 0;
    b->bgn = 0;
}

void *cupkee_buffer_alloc(size_t size)
{
    cupkee_buffer_t *buf = cupkee_malloc(size + sizeof(cupkee_buffer_t));

    if (buf) {
        buf->cap = size;
        buf->len = 0;
        buf->bgn = 0;
    }
    return buf;
}

void *cupkee_buffer_create(size_t n, const void *data)
{
    cupkee_buffer_t *buf = cupkee_malloc(n + sizeof(cupkee_buffer_t));

    if (buf) {
        buf->cap = n;
        buf->len = n;
        buf->bgn = 0;
        memcpy(buf->ptr, data, n);
    }
    return buf;
}

void cupkee_buffer_release(void *p)
{
    cupkee_free(p);
}

int cupkee_buffer_is_empty(void *p)
{
    cupkee_buffer_t *b = (cupkee_buffer_t *)p;

    return b->len == 0;
}

int cupkee_buffer_is_full(void *p)
{
    cupkee_buffer_t *b = (cupkee_buffer_t *)p;

    return b->len == b->cap;
}

size_t cupkee_buffer_capacity(void *p)
{
    cupkee_buffer_t *b = (cupkee_buffer_t *)p;

    return b->cap;
}

size_t cupkee_buffer_space(void *p)
{
    cupkee_buffer_t *b = (cupkee_buffer_t *)p;

    return b->cap - b->len;
}

size_t cupkee_buffer_length(void *p)
{
    cupkee_buffer_t *b = (cupkee_buffer_t *)p;

    return b->len;
}

int cupkee_buffer_extend(void *p, int n)
{
    cupkee_buffer_t *b = (cupkee_buffer_t *)p;

    if (n > 0) {
        if (b->len + n <= b->cap) {
            b->len += n;
        }
    } else {
        if (b->len >= -n) {
            b->len = n + b->len;
        }
    }
    return b->len;
}

int cupkee_buffer_push(void *p, uint8_t d)
{
    cupkee_buffer_t *b = (cupkee_buffer_t *)p;

    if (b->len < b->cap) {
        int tail = b->bgn + b->len++;
        if (tail >= b->cap) {
            tail -= b->cap;
        }
        b->ptr[tail] = d;
        return 1;
    } else {
        return 0;
    }
}

int cupkee_buffer_pop(void *p, uint8_t *d)
{
    cupkee_buffer_t *b = (cupkee_buffer_t *)p;

    if (b->len) {
        int tail = b->bgn + (--b->len);
        if (tail >= b->cap) {
            tail -= b->cap;
        }
        *d = b->ptr[tail];
        return 1;
    } else {
        return 0;
    }
}

int cupkee_buffer_shift(void *p, uint8_t *d)
{
    cupkee_buffer_t *b = (cupkee_buffer_t *)p;

    if (b->len) {
        *d = b->ptr[b->bgn++];
        if (b->bgn >= b->cap) {
            b->bgn = 0;
        }
        b->len--;
        return 1;
    }
    return 0;
}

int cupkee_buffer_unshift(void *p, uint8_t d)
{
    cupkee_buffer_t *b = (cupkee_buffer_t *)p;

    if (b->len < b->cap) {
        b->len++;
        if (b->bgn) {
            b->bgn--;
        } else {
            b->bgn = b->cap - 1;
        }
        b->ptr[b->bgn] = d;

        return 1;
    } else {
        return 0;
    }
}

int cupkee_buffer_take(void *p, size_t n, void *buf)
{
    cupkee_buffer_t *b = (cupkee_buffer_t *)p;

    if (n > b->len) {
        n = b->len;
    }

    if (n) {
        int tail = b->bgn + n;
        int size = n;

        if (tail > b->cap) {
            tail -= b->cap;
            size -= tail;
            memcpy(buf + size, b->ptr, tail);
        } else
        if (tail == b->cap) {
            tail = 0;
        }

        memcpy(buf, b->ptr + b->bgn, size);
        b->bgn = tail;
        b->len -= n;
    }

    return n;
}

int cupkee_buffer_give(void *p, size_t n, const void *buf)
{
    cupkee_buffer_t *b = (cupkee_buffer_t *)p;

    if (n + b->len > b->cap) {
        n = b->cap - b->len;
    }

    if (n) {
        int head = b->bgn + b->len;
        int size = n;

        if (head >= b->cap) {
            head -= b->cap;
        } else
        if (head + n > b->cap) {
            int wrap = head + n - b->cap;

            size -= wrap;
            memcpy(b->ptr, buf + size, wrap);
        }
        memcpy(b->ptr + head, buf, size);

        b->len += n;
    }

    return n;
}

void *cupkee_buffer_ptr(void *buf)
{
    cupkee_buffer_t *b = (cupkee_buffer_t *)buf;
    int wrap = b->bgn + b->len - b->cap;

    if (wrap > 0) {
        int gap = b->cap - b->len;
        uint8_t *ptr = b->ptr;
        if (gap >= wrap) {
            memmove(ptr + wrap, ptr + b->bgn, b->len - wrap);
            memcpy(ptr + b->len - wrap, ptr, wrap);
            b->bgn = wrap;
        } else {
            // Todo: alloc new ptr
            return NULL;
        }
    }

    return b->ptr + b->bgn;
}
