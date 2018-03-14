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
#include "rbuff.h"

#define HISTORY_MAX         16
#define HISTORY_BUF_SIZE    256

typedef struct history_index_t {
    uint16_t pos;
    uint16_t len;
} history_index_t;

static rbuff_t history_idx;
static rbuff_t history_buf;
static int16_t history_cursor;

static history_index_t history_indexs[HISTORY_MAX];
static char            history_memory[HISTORY_BUF_SIZE];

static int history_shift(void)
{
    int pos = rbuff_shift(&history_idx);
    history_index_t *index;

    if (pos < 0) {
        return 0;
    }

    index = &history_indexs[pos];
    rbuff_remove_left(&history_buf, index->len);

    return 1;
}

static int history_push(int len, const char *data)
{
    int i, start, pos = rbuff_push(&history_idx);
    history_index_t *index;

    if (pos < 0) {
        return -1;
    }
    index = &history_indexs[pos];

    start = rbuff_end(&history_buf);

    for (i = 0, pos = rbuff_push(&history_buf);
         i < len && pos >= 0;
         i++, pos = rbuff_push(&history_buf)) {
        history_memory[pos] = data[i];
    }

    index->pos = start;
    index->len = i;

    return i;
}

int cupkee_history_init(void)
{
    rbuff_init(&history_idx, HISTORY_MAX);
    rbuff_init(&history_buf, HISTORY_BUF_SIZE);
    history_cursor = 0;

    return 0;
}

int cupkee_history_push(int len, const char *data)
{
    if (len > HISTORY_BUF_SIZE) {
        return 0;
    }

    while(!rbuff_has_space(&history_idx, 1) || !rbuff_has_space(&history_buf, len)) {
        history_shift();
    }

    history_push(len, data);

    // Make cursour at the end of historys
    history_cursor = rbuff_end(&history_idx);

    return 1;
}

int cupkee_history_load(int dir)
{
    history_index_t *index;
    unsigned i, start;

    history_cursor += dir;
    if (history_cursor < 0) {
        history_cursor = 0;
        return CON_EXECUTE_DEF;
    } else
    if (history_cursor >= rbuff_end(&history_idx)){
        history_cursor = rbuff_end(&history_idx);
        return CON_EXECUTE_DEF;
    }
    index = &history_indexs[_rbuff_get(&history_idx, history_cursor)];

    console_input_clean();

    start = index->pos;
    for (i = 0; i < index->len; i++) {
        int pos = rbuff_get(&history_buf, start + i);

        if (pos < 0) {
            // assert(0);
            break;
        }

        console_input_char(history_memory[pos]);
    }

    return CON_PREVENT_DEF;
}

