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

#include <stdio.h>
#include <string.h>

#include "test.h"
#include <cupkee.h>

static int test_setup(void)
{
    return 0;
}

static int test_clean(void)
{
    return 0;
}

static void test_memory_init(void)
{
    hw_mock_init(32 * 1024);

    CU_ASSERT(0 == cupkee_memory_setup());

    CU_ASSERT(0 == cupkee_free_pages(0));
    CU_ASSERT(1 == cupkee_free_pages(1));
    CU_ASSERT(1 == cupkee_free_pages(2));
    CU_ASSERT(1 == cupkee_free_pages(3));
    CU_ASSERT(1 == cupkee_free_pages(4));
    CU_ASSERT(0 == cupkee_free_pages(5));
    CU_ASSERT(0 == cupkee_free_pages(6));
    CU_ASSERT(0 == cupkee_free_pages(7));

    hw_mock_deinit();
}

static void test_page_alloc(void)
{
    int i;
    cupkee_page_t *page[15];

    hw_mock_init(16 * 1024 + 1023);


    CU_ASSERT(0 == cupkee_memory_setup());

    CU_ASSERT(1 == cupkee_free_pages(0));
    CU_ASSERT(1 == cupkee_free_pages(1));
    CU_ASSERT(1 == cupkee_free_pages(2));
    CU_ASSERT(1 == cupkee_free_pages(3));
    CU_ASSERT(0 == cupkee_free_pages(4));

    for (i = 0; i < 15; i++) {
        page[i] = cupkee_page_alloc(0);
        CU_ASSERT(NULL != page[i]);
    }
    for (i = 0; i < 15; i++) {
        cupkee_page_free(page[i]);
    }
    CU_ASSERT(1 == cupkee_free_pages(0));
    CU_ASSERT(1 == cupkee_free_pages(1));
    CU_ASSERT(1 == cupkee_free_pages(2));
    CU_ASSERT(1 == cupkee_free_pages(3));

    for (i = 0; i < 7; i++) {
        page[i] = cupkee_page_alloc(1);
        CU_ASSERT(NULL != page[i]);
    }
    for (i = 0; i < 7; i++) {
        cupkee_page_free(page[i]);
    }
    CU_ASSERT(1 == cupkee_free_pages(0));
    CU_ASSERT(1 == cupkee_free_pages(1));
    CU_ASSERT(1 == cupkee_free_pages(2));
    CU_ASSERT(1 == cupkee_free_pages(3));

    for (i = 0; i < 3; i++) {
        page[i] = cupkee_page_alloc(2);
        CU_ASSERT(NULL != page[i]);
    }
    for (i = 0; i < 3; i++) {
        cupkee_page_free(page[i]);
    }
    CU_ASSERT(1 == cupkee_free_pages(0));
    CU_ASSERT(1 == cupkee_free_pages(1));
    CU_ASSERT(1 == cupkee_free_pages(2));
    CU_ASSERT(1 == cupkee_free_pages(3));

    for (i = 0; i < 1; i++) {
        page[i] = cupkee_page_alloc(3);
        CU_ASSERT(NULL != page[i]);
    }
    for (i = 0; i < 1; i++) {
        cupkee_page_free(page[i]);
    }
    CU_ASSERT(1 == cupkee_free_pages(0));
    CU_ASSERT(1 == cupkee_free_pages(1));
    CU_ASSERT(1 == cupkee_free_pages(2));
    CU_ASSERT(1 == cupkee_free_pages(3));

    CU_ASSERT(NULL == cupkee_page_alloc(4));
    CU_ASSERT(NULL == cupkee_page_alloc(5));
    CU_ASSERT(NULL == cupkee_page_alloc(6));
    CU_ASSERT(NULL == cupkee_page_alloc(7));
    CU_ASSERT(NULL == cupkee_page_alloc(8));

    hw_mock_deinit();
}

static void test_memory_alloc(void)
{
    int i;
    void *mem[15 * 32];

    hw_mock_init(16 * 1024 + 1023);

    CU_ASSERT(0 == cupkee_memory_setup());

    // Where are 15 Pages can be use
    CU_ASSERT(1 == cupkee_free_pages(0));
    CU_ASSERT(1 == cupkee_free_pages(1));
    CU_ASSERT(1 == cupkee_free_pages(2));
    CU_ASSERT(1 == cupkee_free_pages(3));
    CU_ASSERT(0 == cupkee_free_pages(4));

    // Alloc memory 32Bytes
    for (i = 0; i < 480; i++) {
        if (NULL == (mem[i] = cupkee_malloc(32))) {
            CU_ASSERT_FATAL(0);
        }
    }

    CU_ASSERT(NULL == cupkee_malloc(32));

    CU_ASSERT(0 == cupkee_free_pages(0));
    CU_ASSERT(0 == cupkee_free_pages(1));
    CU_ASSERT(0 == cupkee_free_pages(2));
    CU_ASSERT(0 == cupkee_free_pages(3));
    CU_ASSERT(0 == cupkee_free_pages(4));

    for (i = 0; i < 480; i++) {
        cupkee_free(mem[i]);
    }
    CU_ASSERT(1 == cupkee_free_pages(0));
    CU_ASSERT(1 == cupkee_free_pages(1));
    CU_ASSERT(1 == cupkee_free_pages(2));
    CU_ASSERT(1 == cupkee_free_pages(3));
    CU_ASSERT(0 == cupkee_free_pages(4));

    // Alloc memory 64Bytes
    for (i = 0; i < 240; i++) {
        if (NULL == (mem[i] = cupkee_malloc(64))) {
            CU_ASSERT_FATAL(0);
        }
    }
    CU_ASSERT(NULL == cupkee_malloc(32));
    for (i = 0; i < 240; i++) {
        cupkee_free(mem[i]);
    }
    CU_ASSERT(1 == cupkee_free_pages(0));
    CU_ASSERT(1 == cupkee_free_pages(1));
    CU_ASSERT(1 == cupkee_free_pages(2));
    CU_ASSERT(1 == cupkee_free_pages(3));
    CU_ASSERT(0 == cupkee_free_pages(4));

    // Alloc memory 128Bytes
    for (i = 0; i < 120; i++) {
        if (NULL == (mem[i] = cupkee_malloc(128))) {
            CU_ASSERT_FATAL(0);
        }
    }
    CU_ASSERT(NULL == cupkee_malloc(32));
    for (i = 0; i < 120; i++) {
        cupkee_free(mem[i]);
    }
    CU_ASSERT(1 == cupkee_free_pages(0));
    CU_ASSERT(1 == cupkee_free_pages(1));
    CU_ASSERT(1 == cupkee_free_pages(2));
    CU_ASSERT(1 == cupkee_free_pages(3));
    CU_ASSERT(0 == cupkee_free_pages(4));

    // Alloc memory 256Bytes
    for (i = 0; i < 60; i++) {
        if (NULL == (mem[i] = cupkee_malloc(256))) {
            CU_ASSERT_FATAL(0);
        }
    }
    CU_ASSERT(NULL == cupkee_malloc(32));
    for (i = 0; i < 60; i++) {
        cupkee_free(mem[i]);
    }
    CU_ASSERT(1 == cupkee_free_pages(0));
    CU_ASSERT(1 == cupkee_free_pages(1));
    CU_ASSERT(1 == cupkee_free_pages(2));
    CU_ASSERT(1 == cupkee_free_pages(3));
    CU_ASSERT(0 == cupkee_free_pages(4));

    // Alloc memory 1024Bytes
    for (i = 0; i < 15; i++) {
        if (NULL == (mem[i] = cupkee_malloc(1024))) {
            CU_ASSERT_FATAL(0);
        }
    }
    CU_ASSERT(NULL == cupkee_malloc(32));
    CU_ASSERT(NULL == cupkee_malloc(1024));

    for (i = 0; i < 15; i++) {
        cupkee_free(mem[i]);
    }
    CU_ASSERT(1 == cupkee_free_pages(0));
    CU_ASSERT(1 == cupkee_free_pages(1));
    CU_ASSERT(1 == cupkee_free_pages(2));
    CU_ASSERT(1 == cupkee_free_pages(3));
    CU_ASSERT(0 == cupkee_free_pages(4));

    // Alloc memory 2048Bytes
    for (i = 0; i < 7; i++) {
        if (NULL == (mem[i] = cupkee_malloc(2048))) {
            CU_ASSERT_FATAL(0);
        }
    }
    CU_ASSERT(NULL == cupkee_malloc(2048));
    for (i = 0; i < 7; i++) {
        cupkee_free(mem[i]);
    }
    CU_ASSERT(1 == cupkee_free_pages(0));
    CU_ASSERT(1 == cupkee_free_pages(1));
    CU_ASSERT(1 == cupkee_free_pages(2));
    CU_ASSERT(1 == cupkee_free_pages(3));
    CU_ASSERT(0 == cupkee_free_pages(4));

    // Alloc memory 4096 Bytes
    for (i = 0; i < 3; i++) {
        if (NULL == (mem[i] = cupkee_malloc(4096))) {
            CU_ASSERT_FATAL(0);
        }
    }
    CU_ASSERT(NULL == cupkee_malloc(4096));
    for (i = 0; i < 3; i++) {
        cupkee_free(mem[i]);
    }
    CU_ASSERT(1 == cupkee_free_pages(0));
    CU_ASSERT(1 == cupkee_free_pages(1));
    CU_ASSERT(1 == cupkee_free_pages(2));
    CU_ASSERT(1 == cupkee_free_pages(3));
    CU_ASSERT(0 == cupkee_free_pages(4));

    // Alloc memory 8192 Bytes
    for (i = 0; i < 1; i++) {
        if (NULL == (mem[i] = cupkee_malloc(8192))) {
            CU_ASSERT_FATAL(0);
        }
    }
    CU_ASSERT(NULL == cupkee_malloc(8192));
    for (i = 0; i < 1; i++) {
        cupkee_free(mem[i]);
    }
    CU_ASSERT(1 == cupkee_free_pages(0));
    CU_ASSERT(1 == cupkee_free_pages(1));
    CU_ASSERT(1 == cupkee_free_pages(2));
    CU_ASSERT(1 == cupkee_free_pages(3));
    CU_ASSERT(0 == cupkee_free_pages(4));

    hw_mock_deinit();
}

CU_pSuite test_sys_memory(void)
{
    CU_pSuite suite = CU_add_suite("system memory", test_setup, test_clean);

    if (suite) {
        CU_add_test(suite, "sys memory init  ", test_memory_init);
        CU_add_test(suite, "sys page alloc   ", test_page_alloc);
        CU_add_test(suite, "sys memory alloc ", test_memory_alloc);
    }

    return suite;
}

