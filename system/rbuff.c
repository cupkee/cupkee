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

#include <string.h>
#include "rbuff.h"

int rbuff_shift(rbuff_t *rb)
{
    if (rb->cnt <= 0) {
        return -1;
    }

    int pos = rb->head++;
    if (rb->head >= rb->size) {
        rb->head = 0;
    }
    rb->cnt--;

    return pos;
}

int rbuff_unshift(rbuff_t *rb)
{
    if (rb->cnt >= rb->size) {
        return -1;
    }

    if (rb->head == 0) {
        rb->head = rb->size - 1;
    } else {
        rb->head--;
    }
    rb->cnt++;
    return rb->head;
}


int rbuff_push(rbuff_t *rb)
{
    if (rb->cnt >= rb->size) {
        return -1;
    }

    int pos = rb->head + rb->cnt++;
    if (pos >= rb->size) {
        pos %= rb->size;
    }
    return pos;
}

int rbuff_pop(rbuff_t *rb)
{
    if (rb->cnt <= 0) {
        return -1;
    }

    int pos = rb->head + rb->cnt--;
    if (pos >= rb->size) {
        pos %= rb->size;
    }
    return pos;
}

int rbuff_get(rbuff_t *rb, int pos)
{
    if (pos < 0 || pos >= rb->cnt) {
        return -1;
    }
    pos = rb->head + pos;
    if (pos >= rb->size) {
        pos = pos % rb->size;
    }
    return pos;
}

