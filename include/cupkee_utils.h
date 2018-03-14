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

#ifndef __CUPKEE_UTILS_INC__
#define __CUPKEE_UTILS_INC__

/* list_head interface */
typedef struct list_head_t list_head_t;

struct list_head_t {
    list_head_t *prev;
    list_head_t *next;
};

static inline void list_head_init(list_head_t *head) {
    head->prev = head;
    head->next = head;
}

static inline void __list_add(list_head_t *node, list_head_t *prev, list_head_t *next) {
    node->prev = prev;
    node->next = next;

    prev->next = node;
    next->prev = node;
}

static inline void __list_del(list_head_t *prev, list_head_t *next) {
    prev->next = next;
    next->prev = prev;
}

static inline int list_is_empty(list_head_t *head) {
    return head == head->next;
}

static inline void list_del(list_head_t *node) {
    if (node) {
        __list_del(node->prev, node->next);
    }
}

static inline void list_add(list_head_t *node, list_head_t *head) {
    __list_add(node, head, head->next);
}

static inline void list_add_tail(list_head_t *node, list_head_t *head) {
    __list_add(node, head->prev, head);
}

#define list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

/* list_head */


#endif /* __CUPKEE_UTILS_INC__ */

