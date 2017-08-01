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

#include "cupkee.h"

#define MEM_POOL_MAX    8

#ifdef SIZE_ALIGN
#undef SIZE_ALIGN
#endif

#ifdef ADDR_ALIGN
#undef ADDR_ALIGN
#endif

#define SIZE_ALIGN(s)   (((s) + 3) & ~3)
#define ADDR_ALIGN(a)   (void *)((((intptr_t)(a)) + 3) & ~3)

typedef struct mem_head_t {
    uint16_t tag;
    uint16_t ref;
} mem_head_t;

typedef struct mem_block_t {
    struct mem_head_t   head;
    struct mem_block_t *next;
} mem_block_t;

typedef struct mem_pool_t {
    struct mem_pool_t  *next;
    mem_block_t *block_head;
    uint16_t block_size;
    uint16_t block_num;
} mem_pool_t;

static int         mem_pool_cnt = 0;
static mem_pool_t *mem_pool[MEM_POOL_MAX];
static const cupkee_memory_desc_t mem_pool_def[3] = {
    {64,  32},
    {128, 16},
    {512,  4},
};

static int memory_pool_setup(size_t block_size, size_t block_cnt)
{
    mem_pool_t *pool;
    void *base;
    uint32_t i, pos, pool_tag;

    if (mem_pool_cnt >= MEM_POOL_MAX) {
        return -CUPKEE_ERESOURCE;
    }
    pool_tag = mem_pool_cnt++;

    block_size = SIZE_ALIGN(block_size);

    pool = (mem_pool_t *) hw_malloc(sizeof(mem_pool_t), 4);
    base = hw_malloc((sizeof(mem_head_t) + block_size) * block_cnt, 4);
    if (!base || !pool) {
        return -CUPKEE_ERESOURCE;
    }

    pool->block_size = block_size;
    pool->block_num  = block_cnt;
    pool->block_head = NULL;

    block_size += sizeof(mem_head_t);
    for (i = 0, pos = 0; i < block_cnt; i++, pos += block_size) {
        mem_block_t *block = (mem_block_t *)(base + pos);

        block->head.tag = pool_tag;
        block->head.ref = 0;

        block->next = pool->block_head;
        pool->block_head = block;
    }

    mem_pool[pool_tag] = pool;

    return CUPKEE_OK;
}

void cupkee_memory_init(int pool_cnt, cupkee_memory_desc_t *descs)
{
    int i;

    mem_pool_cnt = 0;
    if (pool_cnt == 0 || descs == NULL) {
        pool_cnt = 3;
        descs = (cupkee_memory_desc_t *) mem_pool_def;
    }

    for (i = 0; i < pool_cnt && i < MEM_POOL_MAX; i++) {
        if (0 != memory_pool_setup(descs[i].block_size, descs[i].block_cnt)) {
            break;
        }
    }

    mem_pool_cnt = i;
}

void *cupkee_malloc(size_t n)
{
    int i;

    for (i = 0; i < mem_pool_cnt; i++) {
        mem_pool_t *pool = mem_pool[i];

        if (pool->block_size >= n && pool->block_head) {
            mem_block_t *block = pool->block_head;
            pool->block_head = block->next;

            block->head.ref = 2;
            return &(block->next);
        }
    }
    return NULL;
}

void cupkee_free(void *p)
{
    mem_block_t *b = CUPKEE_CONTAINER_OF(p, mem_block_t, next);
    mem_pool_t  *pool;

    // assert (b->head.tag < mem_pool_cnt);

    if (b->head.ref > 1) {
        b->head.ref -= 2;
    }

    if (b->head.ref) {
        return;
    }

    pool = mem_pool[b->head.tag];

    b->next = pool->block_head;
    pool->block_head = b;
}

void *cupkee_mem_ref(void *p)
{
    if (p) {
        mem_block_t *b = CUPKEE_CONTAINER_OF(p, mem_block_t, next);
        b->head.ref += 2;
    }

    return p;
}

/* memory new */
#define PAGE_ZONE_MASK  (3)
#define PAGE_HEAD       (0x80)
#define PAGE_INUSED     (0x40)

typedef struct cupkee_zone_t {
    intptr_t base;
    uint32_t page_num;
    list_head_t   pages_free[CUPKEE_PAGE_ORDERR_MAX];
    cupkee_page_t pages[0];
} cupkee_zone_t;

static uint8_t memory_zone_num = 0;
static cupkee_zone_t *memory_zone[CUPKEE_ZONE_MAX];

static inline size_t zone_block_size(int pages)
{
    return sizeof(cupkee_zone_t) + sizeof(cupkee_page_t) * pages;
}

static int page_split(int page_num, uint16_t *order)
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
        zone->pages[i].units = 0;
        zone->pages[i].order = 0;

        zone->pages[i].blocks = NULL;

        list_head_init(&zone->pages[i].list);
    }

    // printf("num: %d, off: %d\n", page_num, page_off);
    while (page_num > page_off) {
        uint16_t order;
        int pages = page_split(page_num - page_off, &order);

        // printf("add order: %u, %lu\n", order, page_base + page_off * CUPKEE_PAGE_SIZE);

        zone->pages[page_off].flags |= PAGE_HEAD;
        zone->pages[page_off].order = order;

        list_add(&zone->pages[page_off].list, &zone->pages_free[order]);

        page_off += pages;
    }

    zone->base = page_base;
    zone->page_num = page_num;

    memory_zone[memory_zone_num++] = zone;

    return 0;
}

int cupkee_mm_init(void)
{
    size_t mem_size;
    size_t zone_size;
    size_t page_size;
    intptr_t mem_base;
    intptr_t mem_end;
    intptr_t zone_base;
    intptr_t page_base;
    cupkee_zone_t *zone;

    memory_zone_num = 0;

    mem_size = hw_boot_memory_size();
    mem_base = (intptr_t) hw_boot_memory_alloc(mem_size, 1);
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

int cupkee_memory_area_create(intptr_t base, size_t size)
{
    return -1;
}

cupkee_page_t *cupkee_memory_page(void *ptr)
{
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

static inline cupkee_zone_t *page_zone(cupkee_page_t *page) {
    int id = page->flags & PAGE_ZONE_MASK;

    return id < memory_zone_num ? memory_zone[id] : NULL;
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

    page->flags &= ~PAGE_INUSED;

    while (NULL != (super = page_combine(page, zone))) {
        page = super;
    }

    // printf("real free: %d, %u\n", page - zone->pages, page->order);

    list_add(&page->list, &zone->pages_free[page->order]);
}


