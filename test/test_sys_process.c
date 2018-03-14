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

static int test_setup(void)
{
    return TU_pre_init();
}

static int test_clean(void)
{
    return TU_pre_deinit();
}

static void *curr_process_entry = NULL;
static int call_process_count = 0;
static int last_process_state;
static intptr_t  last_process_data = 0;

static void test_process_task(void *entry)
{
    curr_process_entry = entry;
    call_process_count++;
}

static void test_process_finish(int err, intptr_t data)
{
    last_process_state = err;
    last_process_data  = data;
}

static void test_common(void)
{
    call_process_count = 0;

    CU_ASSERT(0 == cupkee_process_start(test_process_task, 3, test_process_finish));

    CU_ASSERT(call_process_count == 1);
    CU_ASSERT(cupkee_process_step(curr_process_entry) == 0);
    CU_ASSERT(cupkee_process_data(curr_process_entry) == 3);


    cupkee_process_done(curr_process_entry);

    // finish should be call
    CU_ASSERT(last_process_state == CUPKEE_OK);
    CU_ASSERT(last_process_data == 3);
}

static void test_next(void)
{
    call_process_count = 0;
    last_process_state = -1;

    CU_ASSERT(0 == cupkee_process_start(test_process_task, 9, test_process_finish));
    CU_ASSERT(call_process_count == 1);
    CU_ASSERT(cupkee_process_step(curr_process_entry) == 0);
    CU_ASSERT(cupkee_process_data(curr_process_entry) == 9);

    cupkee_process_next(curr_process_entry);
    CU_ASSERT(call_process_count == 2);
    CU_ASSERT(cupkee_process_step(curr_process_entry) == 1);
    CU_ASSERT(cupkee_process_data(curr_process_entry) == 9);

    cupkee_process_next(curr_process_entry);
    CU_ASSERT(call_process_count == 3);
    CU_ASSERT(cupkee_process_step(curr_process_entry) == 2);
    CU_ASSERT(cupkee_process_data(curr_process_entry) == 9);

    // finish should be call
    cupkee_process_done(curr_process_entry);
    CU_ASSERT(last_process_state == CUPKEE_OK);
}

static void test_goto(void)
{
    call_process_count = 0;
    last_process_state = -1;

    CU_ASSERT(0 == cupkee_process_start(test_process_task, 8, test_process_finish));
    CU_ASSERT(call_process_count == 1);
    CU_ASSERT(cupkee_process_step(curr_process_entry) == 0);
    CU_ASSERT(cupkee_process_data(curr_process_entry) == 8);

    cupkee_process_goto(curr_process_entry, 15);
    CU_ASSERT(call_process_count == 2);
    CU_ASSERT(cupkee_process_step(curr_process_entry) == 15);
    CU_ASSERT(cupkee_process_data(curr_process_entry) == 8);

    cupkee_process_next(curr_process_entry);
    CU_ASSERT(call_process_count == 3);
    CU_ASSERT(cupkee_process_step(curr_process_entry) == 16);
    CU_ASSERT(cupkee_process_data(curr_process_entry) == 8);

    // finish should be call
    cupkee_process_done(curr_process_entry);
    CU_ASSERT(last_process_state == CUPKEE_OK);
}

static void test_fail(void)
{
    call_process_count = 0;
    last_process_state = -1;

    CU_ASSERT(0 == cupkee_process_start(test_process_task, 9, test_process_finish));
    CU_ASSERT(call_process_count == 1);
    CU_ASSERT(cupkee_process_step(curr_process_entry) == 0);
    CU_ASSERT(cupkee_process_data(curr_process_entry) == 9);

    cupkee_process_next(curr_process_entry);
    CU_ASSERT(call_process_count == 2);
    CU_ASSERT(cupkee_process_step(curr_process_entry) == 1);
    CU_ASSERT(cupkee_process_data(curr_process_entry) == 9);

    // finish should be call
    cupkee_process_fail(curr_process_entry, -5);
    CU_ASSERT(last_process_state == -5);
}

CU_pSuite test_sys_process(void)
{
    CU_pSuite suite = CU_add_suite("system process", test_setup, test_clean);

    if (suite) {
        CU_add_test(suite, "process common   ", test_common);
        CU_add_test(suite, "process next     ", test_next);
        CU_add_test(suite, "process goto     ", test_goto);
        CU_add_test(suite, "process fail     ", test_fail);
    }

    return suite;
}


