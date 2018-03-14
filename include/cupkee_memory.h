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

#ifndef __CUPKEE_MEMORY_INC__
#define __CUPKEE_MEMORY_INC__

typedef struct cupkee_page_t {
    list_head_t list;

    uint8_t flags;
    uint8_t used;
    uint16_t order;

    intptr_t blocks;
} cupkee_page_t;

int cupkee_memory_setup(void);
int cupkee_memory_extend(intptr_t base, size_t size);

int cupkee_free_pages(int order);

void *cupkee_page_memory(cupkee_page_t *page);
cupkee_page_t *cupkee_memory_page(void *ptr);

cupkee_page_t *cupkee_page_alloc(int order);
void cupkee_page_free(cupkee_page_t *page);

void *cupkee_malloc(size_t s);
void  cupkee_free(void *p);

#endif /* __CUPKEE_MEMORY_INC__ */

