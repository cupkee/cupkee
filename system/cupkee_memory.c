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

