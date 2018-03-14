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

/* Page flags bit map
 *******************************************************
 * +---+---+---+---+---+---+---+---+
 * | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
 * +---+---+---+---+---+---+---+---+
 *   \   \   \   \   \   \   \___\____ ZONE_ID
 *    \   \   \   \___\___\___________ Reserved
 *     \   \   \______________________ Page is in MBCQ
 *      \   \_________________________ Page is in used
 *       \____________________________ Page is head of pages
 ******************************************************/
#define PAGE_HEAD       (0x80)
#define PAGE_INUSED     (0x40)
#define PAGE_MBCQ       (0x20)
#define PAGE_ZONE_MASK  (0x03)

/* Memory Block Cache queue */
#define CUPKEE_MBCQ_MAX    (4)

#define MBLOCK_MAGIC    (0xF1)
#define MBLOCK_SIZE(b)  (CUPKEE_MUNIT_SIZE << (b))

typedef struct cupkee_zone_t {
    intptr_t base;
    uint32_t page_num;
    list_head_t   pages_free[CUPKEE_PAGE_ORDERR_MAX];
    cupkee_page_t pages[0];
} cupkee_zone_t;

typedef struct mblock_head_t {
    intptr_t next;
    intptr_t comp;
} mblock_head_t;

static uint8_t memory_zone_num = 0;

static cupkee_zone_t *memory_zone[CUPKEE_ZONE_MAX];
static list_head_t    memory_mbcq[CUPKEE_MBCQ_MAX];

static inline size_t zone_block_size(int pages)
{
    return sizeof(cupkee_zone_t) + sizeof(cupkee_page_t) * pages;
}

static inline cupkee_zone_t *page_zone(cupkee_page_t *page) {
    int id = page->flags & PAGE_ZONE_MASK;

    return id < memory_zone_num ? memory_zone[id] : NULL;
}

static int page_clip(int page_num, uint16_t *order)
{
    int i;

    for (i = CUPKEE_PAGE_ORDERR_MAX - 1; i >= 0; i--) {
        int n = 1 << i;
        if (page_num >= n) {
            *order = (uint16_t)i;
            return n;
        }
    }
    return 0;
}

static int zone_init(cupkee_zone_t *zone, intptr_t page_base, int page_num)
{
    int page_off = 0;
    int i;

    if (memory_zone_num >= CUPKEE_ZONE_MAX) {
        return -1;
    }

    memset(zone, 0, sizeof(cupkee_zone_t));
    for (i = 0; i < CUPKEE_PAGE_ORDERR_MAX; i++) {
        list_head_init(&zone->pages_free[i]);
    }

    for (i = 0; i < page_num; i++) {
        zone->pages[i].flags = memory_zone_num;
        zone->pages[i].used  = 0;
        zone->pages[i].order = 0;

        zone->pages[i].blocks = 0;

        list_head_init(&zone->pages[i].list);
    }

    while (page_num > page_off) {
        uint16_t order;
        int pages = page_clip(page_num - page_off, &order);

        if (pages) {
            zone->pages[page_off].flags |= PAGE_HEAD;
            zone->pages[page_off].order = order;

            list_add(&zone->pages[page_off].list, &zone->pages_free[order]);

            page_off += pages;
        } else {
            break;
        }
    }

    zone->base = page_base;
    zone->page_num = page_num;

    memory_zone[memory_zone_num++] = zone;

    return 0;
}

int cupkee_memory_setup(void)
{
    size_t mem_size;
    size_t zone_size;
    size_t page_size;
    intptr_t mem_base;
    intptr_t mem_end;
    intptr_t zone_base;
    intptr_t page_base;
    cupkee_zone_t *zone;
    int i;

    memory_zone_num = 0;
    for (i = 0; i < CUPKEE_MBCQ_MAX; i++) {
        list_head_init(&memory_mbcq[i]);
    }

    /* boot zone init */
    mem_size = hw_memory_size();
    mem_base = (intptr_t) hw_memory_alloc(mem_size, 1);
    if (!mem_base) {
        return -1;
    }
    mem_end = mem_base + mem_size;

    zone_base = (intptr_t) CUPKEE_ADDR_ALIGN(mem_base, sizeof(intptr_t));
    zone_size = zone_block_size(mem_size / CUPKEE_PAGE_SIZE);

    // printf("zone block size: %lu = %lu + %lu * %lu\n", zone_size, sizeof(cupkee_zone_t), sizeof(cupkee_page_t), mem_size / CUPKEE_PAGE_SIZE);

    page_base = (intptr_t) CUPKEE_ADDR_ALIGN((zone_base + zone_size), CUPKEE_PAGE_SIZE);
    page_size = mem_end - page_base;

    // printf("page base: %p, size: %lu\n", page_base, page_size);

    zone = (cupkee_zone_t *) zone_base;

    return zone_init(zone, page_base, page_size / CUPKEE_PAGE_SIZE);
}

int cupkee_memory_extend(intptr_t base, size_t size)
{
    (void) base;
    (void) size;

    return -1;
}

cupkee_page_t *cupkee_memory_page(void *ptr)
{
    cupkee_zone_t *zone;
    intptr_t page_base = (intptr_t) ptr;
    int i;

    for (i = 0; i < memory_zone_num; i++) {
        zone = memory_zone[i];

        if (page_base >= zone->base) {
            unsigned id = (page_base - zone->base) >> CUPKEE_PAGE_SHIFT;

            if (id < zone->page_num) {
                return &zone->pages[id];
            }
        }
    }

    return NULL;
}

void *cupkee_page_memory(cupkee_page_t *page)
{
    cupkee_zone_t *zone = page_zone(page);

    if (zone) {
        unsigned off = page - zone->pages;

        if (off < zone->page_num) {
            return (void *)(zone->base + off * CUPKEE_PAGE_SIZE);
        }
    }
    return NULL;
}

int cupkee_free_pages(int order)
{
    int count = 0;
    int i;

    if (order >= CUPKEE_PAGE_ORDERR_MAX) {
        return 0;
    }

    for (i = 0; i < memory_zone_num; i++) {
        list_head_t *pos;
        list_head_t *head = &memory_zone[i]->pages_free[order];

        list_for_each(pos, head) {
            count++;
        }
    }
    // printf("free page: %d\n", count);

    return count;
}

static cupkee_page_t *page_division(cupkee_page_t *page)
{
    cupkee_zone_t *zone;
    cupkee_page_t *buddy;
    uint16_t order = page->order - 1;

    if (order >= CUPKEE_PAGE_ORDERR_MAX || NULL == (zone = page_zone(page))) {
        return NULL;
    }

    page->order = order;

    buddy = page + (1 << order);
    buddy->order = order;
    buddy->flags |= PAGE_HEAD;

    return buddy;
}

static cupkee_page_t *page_combine(cupkee_page_t *page, cupkee_zone_t *zone)
{
    cupkee_page_t *buddy;
    int order = page->order;
    int pos, dis;

    pos = page - zone->pages;
    if ((unsigned)pos >= zone->page_num) {
        return NULL;
    }

    if ((pos >> (page->order)) & 1) {
        dis = -(1 << order);
    } else {
        dis = 1 << order;
        if ((unsigned)(pos + dis) >= zone->page_num) {
            return NULL;
        }
    }

    buddy = page + dis;
    if ((buddy->flags & PAGE_INUSED) || (buddy->order != page->order)) {
        return NULL;
    }
    list_del(&buddy->list);

    if (dis < 0) {
        page->flags &= ~PAGE_HEAD;

        buddy->order = order + 1;
        return buddy;
    } else {
        buddy->flags &= ~PAGE_HEAD;

        page->order = order + 1;
        return page;
    }
}

static cupkee_page_t *zone_page_alloc(cupkee_zone_t *zone, int order)
{
    cupkee_page_t *page;
    int supor;


    if (!list_is_empty(&zone->pages_free[order])) {
        page = (cupkee_page_t *)(zone->pages_free[order].next);

        list_del(&page->list);

        page->flags |= PAGE_INUSED;

        return page;
    }

    for (supor = order + 1; supor < CUPKEE_PAGE_ORDERR_MAX; supor ++) {
        if(!list_is_empty(&zone->pages_free[supor])) {
            page = (cupkee_page_t *)(zone->pages_free[supor].next);
            list_del(&page->list);
            while (page->order != order) {
                cupkee_page_t *buddy = page_division(page);
                if (!buddy) {
                    return NULL;
                }

                list_add(&buddy->list, &zone->pages_free[buddy->order]);
            }

            page->flags |= PAGE_INUSED;

            return page;
        }
    }

    return NULL;
}

static void page_block_init(cupkee_page_t *page, size_t block_size)
{
    void *mem = cupkee_page_memory(page);
    int i, max = CUPKEE_PAGE_SIZE / block_size;
    intptr_t head = 0;

    page->flags |= PAGE_MBCQ;
    page->used   = 0;

    for (i = 0; i < max; i++) {
        mblock_head_t *mb = (mblock_head_t *)(mem + block_size * i);

        mb->next = head;
        mb->comp = ~(head) + 1;

        head = (intptr_t)mb;
    }

    page->blocks = head;
}

static void *page_block_alloc(cupkee_page_t *page)
{
    mblock_head_t *mb = (mblock_head_t *)page->blocks;

    if (mb) {
        // assert(mb->comp + mb->next == 0);
        page->used++;
        page->blocks = mb->next;
    }

    return mb;
}

static void page_block_free(cupkee_page_t *page, void *b)
{
    mblock_head_t *mb = (mblock_head_t *)b;

    mb->next = page->blocks;
    mb->comp = ~(mb->next) + 1;

    page->blocks = (intptr_t) mb;
    if (--page->used == 0) {
        list_del(&page->list);
        cupkee_page_free(page);
    }
}

static inline cupkee_page_t *mbcq_page_first(int q)
{
    list_head_t *head = &memory_mbcq[q];

    return list_is_empty(head) ? NULL : (cupkee_page_t *)(head->next);
}

static void *mbcq_alloc(size_t size)
{
    int q;

    if (size > MBLOCK_SIZE(2)) {
        q = 3;
    } else {
    if (size > MBLOCK_SIZE(1)) {
        q = 2;
    } else
    if (size > MBLOCK_SIZE(0)) {
        q = 1;
    } else
        q = 0;
    }

    while (q < CUPKEE_MBCQ_MAX) {
        cupkee_page_t *page = mbcq_page_first(q);

        if (!page || !page->blocks) {
            page = cupkee_page_alloc(0);
            if (page) {
                page_block_init(page, MBLOCK_SIZE(q));
                list_add(&page->list, &memory_mbcq[q]);
            } else {
                q++;
                continue;
            }
        }

        return page_block_alloc(page);
    }

    return NULL;
}

cupkee_page_t *cupkee_page_alloc(int order)
{
    int zone_id;
    cupkee_page_t *page;

    if (order >= CUPKEE_PAGE_ORDERR_MAX) {
        return NULL;
    }

    for (zone_id = 0, page = NULL; !page && zone_id < memory_zone_num; zone_id++) {
        page = zone_page_alloc(memory_zone[zone_id], order);
    }

    return page;
}

void cupkee_page_free(cupkee_page_t *page)
{
    cupkee_page_t *super;
    cupkee_zone_t *zone = page_zone(page);

    if (!zone) {
        return;
    }

    // printf("\nfree: %d, %u\n", page - zone->pages, page->order);

    page->flags &= ~(PAGE_INUSED | PAGE_MBCQ);

    while (NULL != (super = page_combine(page, zone))) {
        page = super;
    }

    // printf("real free: %d, %u\n", page - zone->pages, page->order);

    list_add(&page->list, &zone->pages_free[page->order]);
}

void *cupkee_malloc(size_t size)
{
    if (size <= MBLOCK_SIZE(CUPKEE_MBCQ_MAX - 1)) {
        return mbcq_alloc(size);
    } else {
        int order = 0;

        while (size > (CUPKEE_PAGE_SIZE << order)) {
            if (++order >= CUPKEE_PAGE_ORDERR_MAX) {
                break;
            }
        }

        while (order < CUPKEE_PAGE_ORDERR_MAX) {
            cupkee_page_t *page = cupkee_page_alloc(order++);

            if (page) {
                return cupkee_page_memory(page);
            }
        }
    }

    return NULL;
}

void  cupkee_free(void *p)
{
    cupkee_page_t *page = cupkee_memory_page(p);

    if (!page) {
        return;
    }
    // assert(page->flags & (PAGE_HEAD | PAGE_INUSED);

    if (page->flags & PAGE_MBCQ) {
        page_block_free(page, p);
    } else {
        cupkee_page_free(page);
    }
}

