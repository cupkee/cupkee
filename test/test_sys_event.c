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

static void test_post_take(void)
{
    int i;
    cupkee_event_t e;

    cupkee_event_setup();

    CU_ASSERT_EQUAL(cupkee_event_post(0, 0, 0), 1);
    CU_ASSERT_EQUAL(cupkee_event_take(&e), 1);
    CU_ASSERT_EQUAL(e.type, 0);
    CU_ASSERT_EQUAL(e.code, 0);
    CU_ASSERT_EQUAL(e.which, 0);

    CU_ASSERT_EQUAL(cupkee_event_post(1, 2, 3), 1);
    CU_ASSERT_EQUAL(cupkee_event_take(&e), 1);
    CU_ASSERT_EQUAL(e.type, 1);
    CU_ASSERT_EQUAL(e.code, 2);
    CU_ASSERT_EQUAL(e.which, 3);

    for (i = 0; i < 16; i++) {
        cupkee_event_post(i, i * 2, i * 4);
        cupkee_event_take(&e);
    }
    CU_ASSERT_EQUAL(e.type, 15);
    CU_ASSERT_EQUAL(e.code, 30);
    CU_ASSERT_EQUAL(e.which, 60);

    cupkee_event_reset();
}

static uint8_t emitter1_storage;
static uint8_t emitter2_storage;
static void emitter1_event_handle(cupkee_event_emitter_t *emitter, uint8_t e)
{
    (void) emitter;
    emitter1_storage = e;
}
static void emitter2_event_handle(cupkee_event_emitter_t *emitter, uint8_t e)
{
    (void) emitter;
    emitter2_storage = e;
}

static void test_emitter(void)
{
    cupkee_event_emitter_t emitter1, emitter2;

    emitter1_storage = 0;
    emitter2_storage = 0;
    cupkee_event_setup();

    CU_ASSERT(cupkee_event_emitter_init(&emitter1, emitter1_event_handle) >= 0);
    CU_ASSERT(cupkee_event_emitter_init(&emitter2, emitter2_event_handle) >= 0);

    cupkee_event_post(EVENT_EMITTER, 3, emitter1.id);
    TU_emitter_event_dispatch();
    CU_ASSERT(emitter1_storage == 3);

    cupkee_event_post(EVENT_EMITTER, 2, emitter1.id);
    TU_emitter_event_dispatch();
    CU_ASSERT(emitter1_storage == 2);

    cupkee_event_post(EVENT_EMITTER, 3, emitter2.id);
    TU_emitter_event_dispatch();
    CU_ASSERT(emitter2_storage == 3);

    CU_ASSERT(cupkee_event_emitter_deinit(&emitter1) == CUPKEE_OK);
    CU_ASSERT(cupkee_event_emitter_deinit(&emitter2) == CUPKEE_OK);

    cupkee_event_reset();
}

static void test_emitter_emit(void)
{
    cupkee_event_emitter_t emitter1, emitter2;

    emitter1_storage = 0;
    emitter2_storage = 0;
    cupkee_event_setup();

    CU_ASSERT(cupkee_event_emitter_init(&emitter1, emitter1_event_handle) >= 0);
    CU_ASSERT(cupkee_event_emitter_init(&emitter2, emitter2_event_handle) >= 0);

    cupkee_event_emitter_emit(&emitter1, 3);
    cupkee_event_emitter_emit(&emitter2, 7);
    TU_emitter_event_dispatch();
    TU_emitter_event_dispatch();
    CU_ASSERT(emitter1_storage == 3);
    CU_ASSERT(emitter2_storage == 7);

    CU_ASSERT(cupkee_event_emitter_deinit(&emitter1) == CUPKEE_OK);
    CU_ASSERT(cupkee_event_emitter_deinit(&emitter2) == CUPKEE_OK);

    /* Do nothing when emitter had deinited */
    cupkee_event_emitter_emit(&emitter1, 5);
    cupkee_event_emitter_emit(&emitter2, 5);
    TU_emitter_event_dispatch();
    TU_emitter_event_dispatch();
    CU_ASSERT(emitter1_storage == 3);
    CU_ASSERT(emitter2_storage == 7);

    cupkee_event_reset();
}

CU_pSuite test_sys_event(void)
{
    CU_pSuite suite = CU_add_suite("system event", test_setup, test_clean);

    if (suite) {
        CU_add_test(suite, "post & take",   test_post_take);
        CU_add_test(suite, "emitter",       test_emitter);
        CU_add_test(suite, "emitter emit",  test_emitter_emit);
    }

    return suite;
}


