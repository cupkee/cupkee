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

#if 0
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
#endif

CU_pSuite test_sys_event(void)
{
    CU_pSuite suite = CU_add_suite("system event", test_setup, test_clean);

    if (suite) {
        CU_add_test(suite, "post & take      ", test_post_take);
//        CU_add_test(suite, "emitter          ", test_emitter);
//        CU_add_test(suite, "emitter emit     ", test_emitter_emit);
    }

    return suite;
}


