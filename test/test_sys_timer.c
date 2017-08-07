/*
MIT License

This file is part of cupkee project

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

#include <stdio.h>
#include <string.h>

#include "test.h"
#include <cupkee.h>

/* */

/* */

static int test_setup(void)
{
    return TU_pre_init();
}

static int test_clean(void)
{
    return TU_pre_deinit();
}

static int timer_count = 0;
static int timer_ctrol = CUPKEE_TIMER_STOP; // CUPKEE_TIMER_KEEP, CUPKEE_TIMER_RELOAD
static int test_timer_counter(int timer, int event, intptr_t param)
{
    (void) timer;
    (void) event;
    (void) param;

    timer_count++;
    return timer_ctrol;
}

static void test_timer_start(void)
{
    int timer;

    timer_count = 0;
    CU_ASSERT(0 <= (timer = cupkee_timer_start(10, test_timer_counter, 0)));
}

static void test_timer_stop(void)
{
    CU_ASSERT(0);
}

static void test_timer_read(void)
{
    CU_ASSERT(0);
}

CU_pSuite test_sys_timer(void)
{
    CU_pSuite suite = CU_add_suite("system timer", test_setup, test_clean);

    if (suite) {
        CU_add_test(suite, "timer start      ", test_timer_start);
        CU_add_test(suite, "timer stop       ", test_timer_stop);
        CU_add_test(suite, "timer read       ", test_timer_read);
    }

    return suite;
}

