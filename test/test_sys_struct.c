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

static const char *test_options[] = {
    "a", "b", "c", "d"
};

static cupkee_struct_desc_t test_desc[] = {
    {
        .name = "int8",
        .type = CUPKEE_STRUCT_INT8
    },
    {
        .name = "uint8",
        .type = CUPKEE_STRUCT_UINT8
    },
    {
        .name = "int16",
        .type = CUPKEE_STRUCT_INT16
    },
    {
        .name = "uint16",
        .type = CUPKEE_STRUCT_UINT16
    },
    {
        .name = "int32",
        .type = CUPKEE_STRUCT_INT32
    },
    {
        .name = "uint32",
        .type = CUPKEE_STRUCT_UINT32
    },
    {
        .name = "float",
        .type = CUPKEE_STRUCT_FLOAT
    },
    {
        .name = "string",
        .type = CUPKEE_STRUCT_STR,
        .size = 15
    },
    {
        .name = "option",
        .type = CUPKEE_STRUCT_OPT,
        .size = 4,
        .opt_names = test_options
    },
    {
        .name = "bytes",
        .type = CUPKEE_STRUCT_OCT,
        .size = 4,
    },
};

static void test_struct_number(void)
{
    cupkee_struct_t conf;
    int vi;
    unsigned vu;
    double vf;

    CU_ASSERT(0 == cupkee_struct_init(&conf, 7, test_desc));

    // Int8
    CU_ASSERT(1 == cupkee_struct_set_int(&conf, 0, 1));
    CU_ASSERT(1 == cupkee_struct_get_int(&conf, 0, &vi) && vi == 1);

    CU_ASSERT(1 == cupkee_struct_set_int2(&conf, "int8", 2));
    CU_ASSERT(1 == cupkee_struct_get_int2(&conf, "int8", &vi) && vi == 2);

    CU_ASSERT(1 == cupkee_struct_set_int(&conf, 0, -1));
    CU_ASSERT(1 == cupkee_struct_get_int(&conf, 0, &vi) && vi == -1);

    CU_ASSERT(1 == cupkee_struct_set_int(&conf, 0, 127));
    CU_ASSERT(1 == cupkee_struct_get_int(&conf, 0, &vi) && vi == 127);

    CU_ASSERT(1 == cupkee_struct_set_int(&conf, 0, 128));
    CU_ASSERT(1 == cupkee_struct_get_int(&conf, 0, &vi) && vi == -128);

    CU_ASSERT(1 == cupkee_struct_set_int(&conf, 0, 255));
    CU_ASSERT(1 == cupkee_struct_get_int(&conf, 0, &vi) && vi == -1);

    CU_ASSERT(1 == cupkee_struct_set_int(&conf, 0, 256));
    CU_ASSERT(1 == cupkee_struct_get_int(&conf, 0, &vi) && vi == 0);

    // UInt8
    CU_ASSERT(1 == cupkee_struct_set_uint(&conf, 1, 255));
    CU_ASSERT(1 == cupkee_struct_get_uint(&conf, 1, &vu) && vu == 255);

    // Int16
    CU_ASSERT(1 == cupkee_struct_set_int(&conf, 2, 0x7fff));
    CU_ASSERT(1 == cupkee_struct_get_int(&conf, 2, &vi) && vi == 32767);

    CU_ASSERT(1 == cupkee_struct_set_int(&conf, 2, 32768));
    CU_ASSERT(1 == cupkee_struct_get_int(&conf, 2, &vi) && vi == -32768);

    CU_ASSERT(1 == cupkee_struct_set_int(&conf, 2, 65535));
    CU_ASSERT(1 == cupkee_struct_get_int(&conf, 2, &vi) && vi == -1);

    CU_ASSERT(1 == cupkee_struct_set_int(&conf, 2, 65536));
    CU_ASSERT(1 == cupkee_struct_get_int(&conf, 2, &vi) && vi == 0);

    // UInt16
    CU_ASSERT(1 == cupkee_struct_set_uint(&conf, 3, 65535));
    CU_ASSERT(1 == cupkee_struct_get_uint(&conf, 3, &vu) && vu == 65535);

    // Int32
    CU_ASSERT(1 == cupkee_struct_set_int(&conf, 4, 1));
    CU_ASSERT(1 == cupkee_struct_get_int(&conf, 4, &vi) && vi == 1);

    CU_ASSERT(1 == cupkee_struct_set_int(&conf, 4, -1));
    CU_ASSERT(1 == cupkee_struct_get_int(&conf, 4, &vi) && vi == -1);

    // UInt32
    CU_ASSERT(1 == cupkee_struct_set_uint(&conf, 5, -1));
    CU_ASSERT(1 == cupkee_struct_get_uint(&conf, 5, &vu) && vu == 4294967295);

    // Float
    CU_ASSERT(1 == cupkee_struct_set_float(&conf, 6, 0.1));
    CU_ASSERT(1 == cupkee_struct_get_float(&conf, 6, &vf) && vf == 0.1);

    CU_ASSERT(1 == cupkee_struct_set_float(&conf, 6, -0.1));
    CU_ASSERT(1 == cupkee_struct_get_float(&conf, 6, &vf) && vf == -0.1);

    CU_ASSERT(1 == cupkee_struct_set_float2(&conf, "float", -0.2));
    CU_ASSERT(1 == cupkee_struct_get_float2(&conf, "float", &vf) && vf == -0.2);

    cupkee_struct_deinit(&conf);
}

static void test_struct_string(void)
{
    cupkee_struct_t conf;
    const char *v;

    CU_ASSERT(0 == cupkee_struct_init(&conf, 8, test_desc));

    CU_ASSERT(1 == cupkee_struct_set_string(&conf, 7, "hello world"));
    CU_ASSERT(1 == cupkee_struct_get_string(&conf, 7, &v) && !strcmp(v, "hello world"));

    CU_ASSERT(1 == cupkee_struct_set_string2(&conf, "string", "hello cupkee"));
    CU_ASSERT(1 == cupkee_struct_get_string2(&conf, "string", &v) && !strcmp(v, "hello cupkee"));


    cupkee_struct_deinit(&conf);
}

static void test_struct_option(void)
{
    cupkee_struct_t conf;
    unsigned v;

    CU_ASSERT(0 == cupkee_struct_init(&conf, 9, test_desc));

    CU_ASSERT(1 == cupkee_struct_set_uint(&conf, 8, 2));
    CU_ASSERT(1 == cupkee_struct_get_uint(&conf, 8, &v) && v == 2);

    CU_ASSERT(1 == cupkee_struct_set_string(&conf, 8, "d"));
    CU_ASSERT(1 == cupkee_struct_get_uint(&conf, 8, &v) && v == 3);

    CU_ASSERT(0 == cupkee_struct_set_string(&conf, 8, "e"));

    cupkee_struct_deinit(&conf);
}

static void test_struct_bytes(void)
{
    cupkee_struct_t conf;
    const uint8_t *v;

    CU_ASSERT(0 == cupkee_struct_init(&conf, 10, test_desc));

    CU_ASSERT(1 == cupkee_struct_push(&conf, 9, 3));
    CU_ASSERT(1 == cupkee_struct_get_bytes(&conf, 9, &v) && v[0] == 3);

    CU_ASSERT(1 == cupkee_struct_push2(&conf, "bytes", 4));
    CU_ASSERT(1 == cupkee_struct_push(&conf, 9, 5));
    CU_ASSERT(1 == cupkee_struct_push(&conf, 9, 6));
    CU_ASSERT(0 == cupkee_struct_push(&conf, 9, 7));

    CU_ASSERT(4 == cupkee_struct_get_bytes2(&conf, "bytes", &v));
    CU_ASSERT(v[0] == 3 && v[1] == 4 && v[2] == 5 && v[3] == 6);

    cupkee_struct_deinit(&conf);
}

CU_pSuite test_sys_struct(void)
{
    CU_pSuite suite = CU_add_suite("system struct", test_setup, test_clean);

    if (suite) {
        CU_add_test(suite, "conf number      ", test_struct_number);
        CU_add_test(suite, "conf string      ", test_struct_string);
        CU_add_test(suite, "conf option      ", test_struct_option);
        CU_add_test(suite, "conf bytes       ", test_struct_bytes);
    }

    return suite;
}



