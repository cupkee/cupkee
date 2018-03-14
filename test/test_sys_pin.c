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
    TU_pre_init();

    cupkee_pin_map(0, 0, 0);
    cupkee_pin_map(1, 0, 1);
    cupkee_pin_map(2, 0, 2);
    cupkee_pin_map(3, 0, 3);
    return 0;
}

static int test_clean(void)
{
    return TU_pre_deinit();
}

static void test_basic(void)
{
    // remap
    CU_ASSERT(-CUPKEE_EINVAL == cupkee_pin_map(0, 0, 0));
    CU_ASSERT(-CUPKEE_EINVAL == cupkee_pin_map(1, 0, 1));
    CU_ASSERT(-CUPKEE_EINVAL == cupkee_pin_map(2, 0, 2));
    CU_ASSERT(-CUPKEE_EINVAL == cupkee_pin_map(3, 0, 3));

    // enable & disable
    CU_ASSERT(0 == cupkee_pin_enable(0, CUPKEE_PIN_IN));
    CU_ASSERT(0 == cupkee_pin_disable(0));

    // get & set
    hw_gpio_set(0, 0, 1);
    hw_gpio_set(0, 1, 0);
    hw_gpio_set(0, 2, 1);
    hw_gpio_set(0, 3, 0);

    CU_ASSERT(1 == cupkee_pin_get(0));
    CU_ASSERT(0 == cupkee_pin_get(1));
    CU_ASSERT(1 == cupkee_pin_get(2));
    CU_ASSERT(0 == cupkee_pin_get(3));

    CU_ASSERT(0 == cupkee_pin_set(0, 0));
    CU_ASSERT(0 == cupkee_pin_set(1, 1));
    CU_ASSERT(0 == cupkee_pin_set(2, 0));
    CU_ASSERT(0 == cupkee_pin_set(3, 1));

    CU_ASSERT(0 == hw_gpio_get(0, 0));
    CU_ASSERT(1 == hw_gpio_get(0, 1));
    CU_ASSERT(0 == hw_gpio_get(0, 2));
    CU_ASSERT(1 == hw_gpio_get(0, 3));

}

static void test_group(void)
{
    void *grp;

    CU_ASSERT(NULL != (grp = cupkee_pin_group_create()));

    CU_ASSERT(1 == cupkee_pin_group_push(grp, 0));
    CU_ASSERT(2 == cupkee_pin_group_push(grp, 1));
    CU_ASSERT(3 == cupkee_pin_group_push(grp, 2));
    CU_ASSERT(4 == cupkee_pin_group_push(grp, 3));

    hw_gpio_set(0, 0, 0); // lowest bit
    hw_gpio_set(0, 1, 1);
    hw_gpio_set(0, 2, 0);
    hw_gpio_set(0, 3, 1);
    CU_ASSERT(10 == cupkee_pin_group_get(grp));

    CU_ASSERT(0 == cupkee_pin_group_set(grp, 0xc));
    CU_ASSERT(0 == cupkee_pin_group_elem_get(grp, 0));
    CU_ASSERT(0 == cupkee_pin_group_elem_get(grp, 1));
    CU_ASSERT(1 == cupkee_pin_group_elem_get(grp, 2));
    CU_ASSERT(1 == cupkee_pin_group_elem_get(grp, 3));

    CU_ASSERT(0 == cupkee_pin_group_elem_set(grp, 0, 1));
    CU_ASSERT(0 == cupkee_pin_group_elem_set(grp, 1, 1));
    CU_ASSERT(0 == cupkee_pin_group_elem_set(grp, 2, 0));
    CU_ASSERT(0 == cupkee_pin_group_elem_set(grp, 3, 0));
    CU_ASSERT(3 == cupkee_pin_group_get(grp));

    CU_ASSERT(-CUPKEE_EINVAL == cupkee_pin_group_elem_set(grp, 4, 1));
    CU_ASSERT(-CUPKEE_EINVAL == cupkee_pin_group_elem_get(grp, 4));

    CU_ASSERT(0 == cupkee_pin_group_destroy(grp));
}

static int changed_pin;
static int change_type;

static int test_event_handler(void *entry, int event, intptr_t which)
{
    (void) entry;
    changed_pin = which;
    change_type = event;

    return 0;
}

static void test_event(void)
{
    hw_gpio_set(0, 0, 0);
    hw_gpio_set(0, 1, 0);
    hw_gpio_set(0, 2, 0);
    hw_gpio_set(0, 3, 0);

//    cupkee_pin_event_handle_set(test_event_handler, NULL);

    CU_ASSERT(0 == cupkee_pin_listen(0, CUPKEE_EVENT_PIN_RISING, test_event_handler, NULL));
    CU_ASSERT(0 == cupkee_pin_listen(1, CUPKEE_EVENT_PIN_FALLING, test_event_handler, NULL));
    CU_ASSERT(0 == cupkee_pin_listen(2, CUPKEE_EVENT_PIN_RISING | CUPKEE_EVENT_PIN_FALLING, test_event_handler, NULL));

    /* pin 0 listen RISING event */
    hw_gpio_set(0, 0, 1);
    CU_ASSERT(1 == TU_pin_event_dispatch());
    CU_ASSERT(0 == changed_pin && CUPKEE_EVENT_PIN_RISING == change_type);

    hw_gpio_set(0, 0, 0);
    CU_ASSERT(0 == TU_pin_event_dispatch());

    /* pin 1 listen FALLING event*/
    hw_gpio_set(0, 1, 1);
    CU_ASSERT(0 == TU_pin_event_dispatch());

    hw_gpio_set(0, 1, 0);
    CU_ASSERT(1 == TU_pin_event_dispatch());
    CU_ASSERT(1 == changed_pin && CUPKEE_EVENT_PIN_FALLING == change_type);

    /* pin 2 listen RISING & FALLING event */
    hw_gpio_set(0, 2, 1);
    CU_ASSERT(1 == TU_pin_event_dispatch());
    CU_ASSERT(2 == changed_pin && CUPKEE_EVENT_PIN_RISING == change_type);
    hw_gpio_set(0, 2, 0);
    CU_ASSERT(1 == TU_pin_event_dispatch());
    CU_ASSERT(2 == changed_pin && CUPKEE_EVENT_PIN_FALLING == change_type);

    /* pin 3 listen nothing */
    hw_gpio_set(0, 3, 1);
    CU_ASSERT(0 == TU_pin_event_dispatch());
    hw_gpio_set(0, 3, 0);
    CU_ASSERT(0 == TU_pin_event_dispatch());

    cupkee_pin_ignore(0);
    cupkee_pin_ignore(1);
    cupkee_pin_ignore(2);

    /* pin all listen nothing */
    hw_gpio_set(0, 0, 0);
    hw_gpio_set(0, 1, 0);
    hw_gpio_set(0, 2, 0);

    hw_gpio_set(0, 1, 1);
    hw_gpio_set(0, 2, 1);
    hw_gpio_set(0, 0, 1);

    hw_gpio_set(0, 0, 0);
    hw_gpio_set(0, 1, 0);
    hw_gpio_set(0, 2, 0);

    CU_ASSERT(0 == TU_pin_event_dispatch());
}

CU_pSuite test_sys_pin(void)
{
    CU_pSuite suite = CU_add_suite("system pin", test_setup, test_clean);

    if (suite) {
        CU_add_test(suite, "pin basic        ", test_basic);
        CU_add_test(suite, "pin group        ", test_group);
        CU_add_test(suite, "pin event        ", test_event);
    }

    return suite;
}

