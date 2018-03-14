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

static void test_register(void)
{
    CU_ASSERT((uint8_t)-1 == 0xff);
}

static void test_read(void)
{
    CU_ASSERT(1);
}

CU_pSuite test_sys_object(void)
{
    CU_pSuite suite = CU_add_suite("system object", test_setup, test_clean);

    if (suite) {
        CU_add_test(suite, "object register  ", test_register);
        CU_add_test(suite, "object read      ", test_read);
    }

    return suite;
}

