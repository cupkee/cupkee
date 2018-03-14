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


#ifndef __RBUFF_INC__
#define __RBUFF_INC__

typedef struct rbuff_t {
    int size;
    int head;
    int  cnt;
} rbuff_t;

static inline void rbuff_init(rbuff_t *rb, int size) {
    rb->size = size;
    rb->head = 0;
    rb->cnt = 0;
}

static inline void rbuff_reset(rbuff_t *rb) {
    rb->head = 0;
    rb->cnt = 0;
}

static inline int _rbuff_get(rbuff_t *rb, int pos)
{
    pos = rb->head + pos;
    if (pos >= rb->size) {
        pos = pos % rb->size;
    }
    return pos;
}

static inline int rbuff_remove(rbuff_t *rb, int n) {
    if (rb->cnt >= n) {
        rb->cnt -= n;
        return n;
    } else {
        return 0;
    }
}

static inline int rbuff_remove_left(rbuff_t *rb, int n) {
    int head;

    if (n < 0 || rb->cnt < n) {
        return 0;
    }

    head = rb->head + n;
    if (head >= rb->size) {
        head = head - rb->size;
    }
    rb->head = head;
    rb->cnt -= n;

    return n;
}

static inline int rbuff_append(rbuff_t *rb, int n) {
    if (rb->cnt + n <= rb->size) {
        rb->cnt += n;
        return n;
    } else {
        return 0;
    }
}

static inline int rbuff_end(rbuff_t *rb) {
    return rb->cnt;
}

static inline int rbuff_is_empty(rbuff_t *rb) {
    return !rb->cnt;
}

static inline int rbuff_is_full(rbuff_t *rb) {
    return rb->cnt == rb->size;
}

static inline int rbuff_has_space(rbuff_t *rb, int space) {
    return rb->size > (rb->cnt + space);
}

int rbuff_shift(rbuff_t *rb);
int rbuff_unshift(rbuff_t *rb);
int rbuff_push(rbuff_t *rb);
int rbuff_pop(rbuff_t *rb);
int rbuff_get(rbuff_t *rb, int pos);

#endif /* __RBUFF_INC__ */

