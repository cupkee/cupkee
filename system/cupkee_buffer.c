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

void *cupkee_buffer_alloc_x(size_t size)
{
    cupkee_buffer_t *buf = cupkee_malloc(size + sizeof(cupkee_buffer_t));

    if (buf) {
        buf->cap = size;
        buf->len = 0;
        buf->bgn = 0;
    }
    return buf;
}

void *cupkee_buffer_create_x(size_t n, const void *data)
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

void cupkee_buffer_release_x(void *p)
{
    cupkee_free(p);
}

int cupkee_buffer_xxx(cupkee_buffer_t *b, void **pptr)
{
    int retv = b->len;

    if (retv) {
        if (b->flags & CUPKEE_FLAG_OWNED) {
            *pptr = b->ptr;
            cupkee_buffer_reset(b);
        } else {
            if (NULL == (*pptr = cupkee_malloc(retv))) {
                return -CUPKEE_ENOMEM;
            }
            cupkee_buffer_take(b, retv, *pptr);
        }
    }

    return retv;
}

int cupkee_buffer_space_to(cupkee_buffer_t *b, size_t n) {
    if (n > b->cap) {
        void *ptr= cupkee_malloc(n);
        if (ptr) {
            if (b->ptr && (b->flags & CUPKEE_FLAG_OWNED)) {
                cupkee_free(b->ptr);
            }
            b->flags |= CUPKEE_FLAG_OWNED;
            b->ptr = ptr;
            b->cap = n;
        } else {
            return b->cap;
        }
    }
    b->len = 0;
    b->bgn = 0;
    return b->cap;
}

int cupkee_buffer_push(cupkee_buffer_t *b, uint8_t d)
{
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

int cupkee_buffer_pop(cupkee_buffer_t *b, uint8_t *d)
{
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

int cupkee_buffer_shift(cupkee_buffer_t *b, uint8_t *d)
{
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

int cupkee_buffer_unshift(cupkee_buffer_t *b, uint8_t d)
{
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

int cupkee_buffer_take(cupkee_buffer_t *b, size_t n, void *buf)
{
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

int cupkee_buffer_give(cupkee_buffer_t *b, size_t n, const void *buf)
{
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

