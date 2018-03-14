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
    return 0;
}

static int test_clean(void)
{
    return 0;
}

static void test_template(void)
{
    // add test code here
    //CU_ASSERT(1);
}

CU_pSuite test_hello(void)
{
    CU_pSuite suite = CU_add_suite("hello", test_setup, test_clean);

    if (suite) {
        CU_add_test(suite, "template", test_template);
    }

    return suite;
}

