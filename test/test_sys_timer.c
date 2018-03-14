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

static int timer_event = 0;
static int timer_count = 0;
static int timer_ctrol = CUPKEE_TIMER_STOP; // CUPKEE_TIMER_KEEP, CUPKEE_TIMER_RELOAD

static int test_timer_counter(void *entry, int event, intptr_t param)
{
    (void) entry;
    (void) param;

    if (event == CUPKEE_EVENT_REWIND) {
        timer_count++;
    }
    timer_event = event;

    return timer_ctrol;
}

static void test_timer_request(void)
{
    void *timer;

    CU_ASSERT(0 <= (timer = cupkee_timer_request(test_timer_counter, 0)));
    CU_ASSERT(cupkee_timer_state(timer) == CUPKEE_TIMER_STATE_IDLE);

    CU_ASSERT(0 == cupkee_timer_release(timer));
    CU_ASSERT(TU_object_event_dispatch());
    CU_ASSERT(timer_event == CUPKEE_EVENT_DESTROY);
}

static void test_timer_start(void)
{
    void *timer;

    CU_ASSERT(0 <= (timer = cupkee_timer_request(test_timer_counter, 0)));

    CU_ASSERT(0 == cupkee_timer_start(timer, 10));
    CU_ASSERT(cupkee_timer_state(timer) == CUPKEE_TIMER_STATE_RUNNING);

    hw_mock_timer_duration_set(7);
    CU_ASSERT(7 == cupkee_timer_duration(timer));

    CU_ASSERT(0 == cupkee_timer_stop(timer));
    CU_ASSERT(TU_object_event_dispatch());
    CU_ASSERT(timer_event == CUPKEE_EVENT_STOP);

    CU_ASSERT(0 == cupkee_timer_release(timer));
    CU_ASSERT(TU_object_event_dispatch());
    CU_ASSERT(timer_event == CUPKEE_EVENT_DESTROY);
}

static void test_timer_running(void)
{
    void *timer;

    CU_ASSERT(0 <= (timer = cupkee_timer_request(test_timer_counter, 0)));

    timer_count = 0;
    CU_ASSERT(0 == cupkee_timer_start(timer, 10));
    CU_ASSERT(timer_event == CUPKEE_EVENT_START);
    CU_ASSERT(cupkee_timer_state(timer) == CUPKEE_TIMER_STATE_RUNNING);
    CU_ASSERT(CUPKEE_ENTRY_ID(timer) == hw_mock_timer_curr_id());
    CU_ASSERT(10 == hw_mock_timer_period());

    timer_ctrol = 0;
    cupkee_timer_rewind(CUPKEE_ENTRY_ID(timer));
    CU_ASSERT(TU_object_event_dispatch());
    CU_ASSERT(timer_event == CUPKEE_EVENT_REWIND);
    CU_ASSERT(timer_count == 1);
    CU_ASSERT(10 == hw_mock_timer_period());

    timer_ctrol = 20;
    cupkee_timer_rewind(CUPKEE_ENTRY_ID(timer));
    CU_ASSERT(TU_object_event_dispatch());
    CU_ASSERT(timer_event == CUPKEE_EVENT_REWIND);
    CU_ASSERT(timer_count == 2);
    CU_ASSERT(20 == hw_mock_timer_period());

    timer_ctrol = -1;
    cupkee_timer_rewind(CUPKEE_ENTRY_ID(timer));
    CU_ASSERT(TU_object_event_dispatch());
    CU_ASSERT(timer_event == CUPKEE_EVENT_REWIND);
    CU_ASSERT(timer_count == 3);

    CU_ASSERT(TU_object_event_dispatch());
    CU_ASSERT(timer_event == CUPKEE_EVENT_STOP);
    CU_ASSERT(timer_count == 3);

    CU_ASSERT(0 == cupkee_timer_release(timer));
    CU_ASSERT(TU_object_event_dispatch());
    CU_ASSERT(timer_event == CUPKEE_EVENT_DESTROY);
}

CU_pSuite test_sys_timer(void)
{
    CU_pSuite suite = CU_add_suite("system timer", test_setup, test_clean);

    if (suite) {
        CU_add_test(suite, "timer request    ", test_timer_request);
        CU_add_test(suite, "timer start      ", test_timer_start);
        CU_add_test(suite, "timer running    ", test_timer_running);
    }

    return suite;
}

