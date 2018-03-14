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

typedef struct auto_complete_t {
    uint8_t prefix;
    uint8_t supply;
    uint8_t same;
    uint8_t pos;
    char buf[TOKEN_MAX_SIZE];
} auto_complete_t;

static int do_complete(const char *sym, void *param)
{
    auto_complete_t *ac = (auto_complete_t *)param;
    char *prefix = ac->buf;
    char *supply = ac->buf + ac->prefix;

    if (strncmp(sym, prefix, ac->prefix)) {
        return 0;
    }

    ac->same++;
    if (ac->same == 1) {
        char *p = strncpy(supply, sym + ac->prefix, TOKEN_MAX_SIZE - ac->prefix - 1);
        if (p == supply) {
            ac->supply = strlen(supply);
        } else {
            *p = 0;
            ac->supply = p - supply;
        }
        return 0;
    }

    /* show suggist */
    if (ac->same == 2) {
        // show the first same symbal
        console_puts("\r\n");
        console_puts(ac->buf);
        ac->pos += strlen(ac->buf);
    }

    // padding spaces, align 4
    do {
        console_putc(' ');
        ac->pos++;
    } while (ac->pos % 4);

    console_puts(sym);
    ac->pos += strlen(sym);

    if (ac->pos > 80) {
        console_puts("\r\n");
    }

    /* update supply */
    int n = 0;
    while (n < ac->supply && sym[ac->prefix + n] == supply[n]) {
        n++;
    }
    supply[n] = 0;
    ac->supply = n;

    return 0;
}

int cupkee_auto_complete(int symbal_num, const char **symbals)
{
    auto_complete_t ac;
    int i, has;

    has = console_input_token(TOKEN_MAX_SIZE - 1, ac.buf);
    if (!has) {
        console_input_insert(4, "    ");
        return CON_PREVENT_DEF;
    }

    ac.pos = 0;
    ac.prefix = has;
    ac.supply = 0;
    ac.same = 0;

    for (i = 0; i < symbal_num; i++) {
        do_complete(symbals[i], &ac);
    }

    if (ac.same) {
        char *suffix = ac.buf + has;
        if (ac.same > 1) {
            console_puts("\r\n");
            console_input_refresh();
        }
        console_input_insert(ac.supply, suffix);
        return CON_PREVENT_DEF;
    }

    return CON_EXECUTE_DEF;
}

void *cupkee_auto_complete_init(void *buf, unsigned size)
{
    auto_complete_t *ac;
    int has;

    if (!buf || size < sizeof(auto_complete_t)) {
        return NULL;
    }

    ac  = (auto_complete_t *) buf;
    has = console_input_token(TOKEN_MAX_SIZE - 1, ac->buf);

    ac->pos = 0;
    ac->prefix = has;
    ac->supply = 0;
    ac->same = 0;

    return buf;
}

void cupkee_auto_complete_update(void *buf, const char *symbal)
{
    auto_complete_t *ac = (auto_complete_t *)buf;

    if (ac && ac->prefix && symbal) {
        do_complete(symbal, ac);
    }
}

int cupkee_auto_complete_finish(void *buf)
{
    auto_complete_t *ac = (auto_complete_t *)buf;
    if (ac && ac->same) {
        char *suffix = ac->buf + ac->prefix;
        if (ac->same > 1) {
            console_puts("\r\n");
            console_input_refresh();
        }
        console_input_insert(ac->supply, suffix);
        return CON_PREVENT_DEF;
    }

    return CON_EXECUTE_DEF;
}

