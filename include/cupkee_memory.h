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

