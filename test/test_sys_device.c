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

static void test_request(void)
{
    cupkee_device_t *dev;

    CU_ASSERT_FATAL(NULL != (dev = cupkee_device_request("spi", 0)));
    CU_ASSERT(0 == cupkee_device_release(dev));

    CU_ASSERT_FATAL(NULL != (dev = cupkee_device_request2(DEVICE_TYPE_SPI, 0)));
    CU_ASSERT(0 == cupkee_device_release(dev));
}

static void test_enable(void)
{
    cupkee_device_t *dev;

    CU_ASSERT_FATAL(NULL != (dev = cupkee_device_request("spi", 0)));

    CU_ASSERT(!cupkee_device_is_enabled(dev));
    CU_ASSERT(0 == cupkee_device_enable(dev));
    CU_ASSERT(cupkee_device_is_enabled(dev));

    CU_ASSERT(0 == cupkee_device_disable(dev));
    CU_ASSERT(!cupkee_device_is_enabled(dev));

    CU_ASSERT(0 == cupkee_device_release(dev));
}

static struct {
    cupkee_device_t *dev;
    int state;
    void *reply;
} query_reply_arg;

static void test_query_reply_clean(void)
{
    query_reply_arg.dev = NULL;
    query_reply_arg.state = 0;
    query_reply_arg.reply = NULL;
}

static void test_query_reply_set(cupkee_device_t *dev, int state, intptr_t param)
{
    (void) param;
    query_reply_arg.dev = dev;
    query_reply_arg.state = state;
    query_reply_arg.reply = cupkee_device_reply_take(dev);
}

static void test_query(void)
{
    cupkee_device_t *dev;

    CU_ASSERT_FATAL(NULL != (dev = cupkee_device_request("spi", 0)));

    CU_ASSERT(!cupkee_device_is_enabled(dev));
    CU_ASSERT(0 == cupkee_device_enable(dev));

    CU_ASSERT(0 == cupkee_device_query(dev, 0, NULL, 8, test_query_reply_set, 0));

    CU_ASSERT(dev == mock_device_curr());
    CU_ASSERT(8   == mock_device_curr_want());

    // Bsp driver should push reply to device
    CU_ASSERT(8   == cupkee_device_reply_push(dev, 8, "12345678"));

    CU_ASSERT(dev == query_reply_arg.dev);
    CU_ASSERT(0 == query_reply_arg.state);
    CU_ASSERT(NULL != query_reply_arg.reply);
    CU_ASSERT(8 == cupkee_buffer_length(query_reply_arg.reply));

    CU_ASSERT(0 == cupkee_device_disable(dev));
    CU_ASSERT(!cupkee_device_is_enabled(dev));

    CU_ASSERT(0 == cupkee_device_release(dev));
}

CU_pSuite test_sys_device(void)
{
    CU_pSuite suite = CU_add_suite("system device", test_setup, test_clean);

    if (suite) {
        CU_add_test(suite, "device request   ", test_request);
        CU_add_test(suite, "device enable    ", test_enable);

        CU_add_test(suite, "device query     ", test_query);
    }

    return suite;
}


