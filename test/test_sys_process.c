/*
MIT License

This file is part of cupkee project

Copyright (c) 2016 Lixing Ding <ding.lixing@gmail.com>

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
        CU_add_test(suite, "process common", test_common);
        CU_add_test(suite, "process next",   test_next);
        CU_add_test(suite, "process goto",   test_goto);
        CU_add_test(suite, "process fail",   test_fail);
    }

    return suite;
}


