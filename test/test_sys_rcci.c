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

/* Mock start */
static int mock_id;
static void *mock_entry;

static int mock_request(int inst)
{
    (void) inst;
    return 0;
}

static int mock_release(int inst)
{
    (void) inst;
    return 0;
}

static int mock_setup(int inst, void *entry)
{
    (void) inst;
    mock_id   = CUPKEE_ENTRY_ID(entry);
    mock_entry = entry;

    return 0;
}

static int mock_reset(int inst)
{
    (void) inst;
    return 0;
}

static int mock_read(int inst, size_t n, void *buf)
{
    (void) inst;
    (void) n;
    (void) buf;
    return 0;
}

static int mock_write(int inst, size_t n, const void *data)
{
    (void) inst;
    (void) n;
    (void) data;
    return 0;
}

static const cupkee_driver_t mock_driver = {
    .request = mock_request,
    .release = mock_release,
    .setup   = mock_setup,
    .reset   = mock_reset,

    .read    = mock_read,
    .write   = mock_write,
};

static const cupkee_device_desc_t mock_device = {
    .name = "mock",
    .inst_max = 2,
    .conf_init = NULL,
    .driver = &mock_driver
};

/* Mock end */

static void *io;
static int test_setup(void)
{
    TU_pre_init();

    cupkee_device_register(&mock_device);

    io = cupkee_device_request("mock", 0);

    return 0;
}

static int test_clean(void)
{
    return TU_pre_deinit();
}

static void rcci_setup(void)
{
    CU_ASSERT(0 == cupkee_rcci_setup(io, "hello", NULL, 0));
}

static void rcci_msg_parse(void)
{

}

CU_pSuite test_sys_rcci(void)
{
    CU_pSuite suite = CU_add_suite("system Remote Control (Client) Interface", test_setup, test_clean);

    if (suite) {
        CU_add_test(suite, "rcci setup       ", rcci_setup);
    }

    return suite;
}

