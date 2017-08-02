/*
MIT License

This file is part of cupkee project.

Copyright (c) 2017 Lixing Ding <ding.lixing@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef __CUPKEE_MEMORY_INC__
#define __CUPKEE_MEMORY_INC__

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

/* new interface */

typedef struct cupkee_page_t {
    list_head_t list;

    uint8_t flags;
    uint8_t used;
    uint16_t order;

    intptr_t blocks;
} cupkee_page_t;

int cupkee_memory_init(void);
int cupkee_memory_extend(intptr_t base, size_t size);

int cupkee_free_pages(int order);

void *cupkee_page_memory(cupkee_page_t *page);
cupkee_page_t *cupkee_memory_page(void *ptr);

cupkee_page_t *cupkee_page_alloc(int order);
void cupkee_page_free(cupkee_page_t *page);

void *cupkee_malloc(size_t s);
void  cupkee_free(void *p);

#endif /* __CUPKEE_MEMORY_INC__ */

