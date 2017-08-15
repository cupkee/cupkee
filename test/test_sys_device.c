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

struct mock_data_t {
    int inst;
    int id;
    int want;
    int r_req;
    int w_req;
};

struct mock_handle_param_t {
    int id;
    int event;
    void *resp;
};

static struct mock_data_t mock_data = {
    .inst = -1,
    .id   = -1,
    .want = 0
};

static struct mock_handle_param_t mock_handle_arg = {
    .id = 0,
    .event = 0,
    .resp = NULL
};

static int mock_request(int inst)
{
    mock_data.inst = inst;
    return 0;
}

static int mock_release(int inst)
{
    (void) inst;
    return 0;
}

static int mock_setup(int inst, int id)
{
    mock_data.id   = id;
    mock_data.inst = inst;

    return 0;
}

static int mock_reset(int inst)
{
    (void) inst;
    return 0;
}

static int mock_query(int inst, int want)
{
    mock_data.inst = inst;
    mock_data.want = want;

    return 0;
}

static int mock_read(int inst, size_t n, void *buf)
{
    mock_data.inst = inst;
    if (buf) {
        memset(buf, 0x11, n);
        return n;
    } else {
        mock_data.r_req = n;
    }

    return 0;
}

static int mock_write(int inst, size_t n, const void *data)
{
    mock_data.inst = inst;

    if (data) {
        return n;
    } else {
        mock_data.w_req = n;
    }

    return 0;
}

static inline void mock_arg_clean(void)
{
    mock_handle_arg.id    = -1;
    mock_handle_arg.event = 0;
    mock_handle_arg.resp  = NULL;
}

static void mock_arg_release(void)
{
    if (mock_handle_arg.resp) {
        cupkee_buffer_release(mock_handle_arg.resp);
    }
    mock_arg_clean();
}

static int mock_handle(int id, int event, intptr_t param)
{
    struct mock_handle_param_t *arg = (struct mock_handle_param_t *)param;

    arg->id = id;
    arg->event = event;
    arg->resp = cupkee_device_response_take(id);

    return 0;
}

static inline int mock_curr_id(void) {
    return mock_data.id;
}

static inline int mock_curr_inst(void) {
    return mock_data.inst;
}

static inline size_t mock_curr_want(void) {
    return mock_data.want;
}

static const cupkee_driver_t mock_driver = {
    .request = mock_request,
    .release = mock_release,
    .setup   = mock_setup,
    .reset   = mock_reset,

    .query   = mock_query,

    .read    = mock_read,
    .write   = mock_write,
};

static const cupkee_device_desc_t mock_device = {
    .name = "mock",
    .inst_max = 2,
    .conf_num = 2,
    .conf_desc = NULL,
    .driver = &mock_driver
};

static int test_setup(void)
{
    TU_pre_init();

    cupkee_device_register(&mock_device);

    return 0;
}

static int test_clean(void)
{
    return TU_pre_deinit();
}

static void test_request(void)
{
    int id1, id2;

    CU_ASSERT(-1 < (id1 = cupkee_device_request("mock", 0)));
    CU_ASSERT(0 == mock_curr_inst());

    CU_ASSERT(-1 < (id2 = cupkee_device_request("mock", 1)));
    CU_ASSERT(1 == mock_curr_inst());

    CU_ASSERT(id1 != id2);
    CU_ASSERT(cupkee_is_device(id1));
    CU_ASSERT(cupkee_is_device(id2));

    cupkee_device_release(id1);
    cupkee_device_release(id2);
}

static void test_enable(void)
{
    int id;

    CU_ASSERT_FATAL(-1 < (id = cupkee_device_request("mock", 0)));

    CU_ASSERT(cupkee_is_device(id));
    CU_ASSERT(!cupkee_device_is_enabled(id));

    CU_ASSERT(0 == cupkee_device_enable(id));
    CU_ASSERT(cupkee_device_is_enabled(id));

    CU_ASSERT(0 == cupkee_device_disable(id));
    CU_ASSERT(!cupkee_device_is_enabled(id));

    cupkee_device_release(id);
}

static void test_query(void)
{
    int id;
    uint8_t buf[2];
    void *req;

    mock_arg_release();

    CU_ASSERT_FATAL(-1 < (id = cupkee_device_request("mock", 0)));

    CU_ASSERT(!cupkee_device_is_enabled(id));
    CU_ASSERT(0 == cupkee_device_enable(id));

    /*
     * query without request data
     */
    CU_ASSERT(0 == cupkee_device_query(id, 0, NULL, 8, mock_handle, (intptr_t)&mock_handle_arg));

    // Bsp driver code start
    CU_ASSERT(id == mock_curr_id());
    CU_ASSERT(8  == mock_curr_want());
    // take request data, there is NULL
    CU_ASSERT(NULL == (req = cupkee_device_request_take(id)));
    // push reply to device
    CU_ASSERT(8   == cupkee_device_response_push(id, 8, "12345678"));
    // Call response_end to complete query
    cupkee_device_response_end(id);
    // Bsp driver code end

    // handle should be called
    CU_ASSERT(TU_object_event_dispatch());
    CU_ASSERT(mock_handle_arg.id    == id);
    CU_ASSERT(mock_handle_arg.event == CUPKEE_EVENT_RESPONSE);
    CU_ASSERT_FATAL(NULL != mock_handle_arg.resp);
    CU_ASSERT(8 == cupkee_buffer_length(mock_handle_arg.resp));

    mock_arg_release();

    /*
     * query with request data
     */
    CU_ASSERT(0 == cupkee_device_query(id, 5, "hihao", 8, mock_handle, (intptr_t)&mock_handle_arg));

    // Bsp driver code start
    CU_ASSERT(id  == mock_curr_id());
    CU_ASSERT(8   == mock_curr_want());
    // take request data
    CU_ASSERT(NULL != (req = cupkee_device_request_take(id)));
    CU_ASSERT(5   == cupkee_buffer_length(req));
    cupkee_buffer_release(req);

    // push reply to device
    CU_ASSERT(8   == cupkee_device_response_push(id, 8, "12345678"));
    CU_ASSERT(0   == cupkee_device_response_push(id, 8, "12345678"));

    // Call response_end to complete query
    cupkee_device_response_end(id);
    // Bsp driver code end

    CU_ASSERT(TU_object_event_dispatch());
    CU_ASSERT(mock_handle_arg.id    == id);
    CU_ASSERT(mock_handle_arg.event == CUPKEE_EVENT_RESPONSE);
    CU_ASSERT_FATAL(NULL != mock_handle_arg.resp);
    CU_ASSERT(8 == cupkee_buffer_length(mock_handle_arg.resp));
    mock_arg_release();

    /*
     * query without response data
     */

    CU_ASSERT(0 == cupkee_device_query(id, 7, "0123456", 0, mock_handle, (intptr_t)&mock_handle_arg));

    // Bsp driver code start
    CU_ASSERT(id  == mock_curr_id());
    CU_ASSERT(0   == mock_curr_want());
    // take request data
    CU_ASSERT(2   == cupkee_device_request_load(id, 2, buf));
    CU_ASSERT('0' == buf[0] && '1' == buf[1]);
    CU_ASSERT(2   == cupkee_device_request_load(id, 2, buf));
    CU_ASSERT('2' == buf[0] && '3' == buf[1]);
    CU_ASSERT(2   == cupkee_device_request_load(id, 2, buf));
    CU_ASSERT('4' == buf[0] && '5' == buf[1]);
    CU_ASSERT(1   == cupkee_device_request_load(id, 2, buf));
    CU_ASSERT('6' == buf[0]);
    // push reply to device
    CU_ASSERT(0   > cupkee_device_response_push(id, 8, "12345678"));
    // Call response_end to complete query
    cupkee_device_response_end(id);
    // Bsp driver code end

    CU_ASSERT(TU_object_event_dispatch());
    CU_ASSERT(mock_handle_arg.id    == id);
    CU_ASSERT(mock_handle_arg.event == CUPKEE_EVENT_RESPONSE);
    CU_ASSERT(NULL == mock_handle_arg.resp);
    mock_arg_release();

    CU_ASSERT(0 == cupkee_device_disable(id));
    CU_ASSERT(!cupkee_device_is_enabled(id));

    cupkee_device_release(id);
}

static void test_read(void)
{
    int id, i;
    uint8_t data;
    char buf[16];

    CU_ASSERT_FATAL(-1 < (id = cupkee_device_request("mock", 1)));

    CU_ASSERT(!cupkee_device_is_enabled(id));
    CU_ASSERT(0 == cupkee_device_enable(id));

    CU_ASSERT(0 == cupkee_read(id, 16, buf));

    CU_ASSERT(id == mock_curr_id());
    CU_ASSERT(1  == mock_curr_inst());
    for (i = 0; i < 16; i++) {
        data = i;
        cupkee_device_push(id, 1, &data);
    }
    CU_ASSERT(16 == cupkee_read(id, 16, buf));
    CU_ASSERT(buf[0] == 0 && buf[15] == 15);

    CU_ASSERT(8 == cupkee_read_sync(id, 8, buf));
    CU_ASSERT(buf[0] == 0x11 && buf[7] == 0x11);

    cupkee_device_release(id);
}

static void test_write(void)
{
    int id;
    char buf[16];

    CU_ASSERT_FATAL(-1 < (id = cupkee_device_request("mock", 2)));

    CU_ASSERT(!cupkee_device_is_enabled(id));
    CU_ASSERT(0 == cupkee_device_enable(id));

    memset(buf, 3, 16);
    CU_ASSERT(16 == cupkee_write(id, 16, buf));

    CU_ASSERT(id == mock_curr_id());
    CU_ASSERT(2  == mock_curr_inst());

    memset(buf, 0, 16);
    CU_ASSERT(16 == cupkee_device_pull(id, 16, buf));
    CU_ASSERT(buf[0] == 3 && buf[15] == 3);

    CU_ASSERT(8 == cupkee_write_sync(id, 8, buf));

    cupkee_device_release(id);
}

static void test_event(void)
{
    int id;
    uint8_t data;
    char buf[32];

    CU_ASSERT_FATAL(-1 < (id = cupkee_device_request("mock", 2)));

    CU_ASSERT(!cupkee_device_is_enabled(id));
    CU_ASSERT(0 == cupkee_device_enable(id));

    CU_ASSERT(0 == cupkee_device_handle_set(id, mock_handle, (intptr_t) &mock_handle_arg));

    cupkee_listen(id, CUPKEE_EVENT_DATA);
    cupkee_listen(id, CUPKEE_EVENT_DRAIN);

    mock_arg_release();
    // trigger event: data
    data = 0;
    while (1 == cupkee_device_push(id, 1, &data)) {
        data++;
    }

    CU_ASSERT(TU_object_event_dispatch());
    CU_ASSERT(mock_handle_arg.id    == id);
    CU_ASSERT(mock_handle_arg.event == CUPKEE_EVENT_DATA);
    CU_ASSERT(32 == cupkee_read(id, 32, buf));

    mock_arg_release();
    // trigger event: data
    _cupkee_systicks = 0;
    data = 9;
    CU_ASSERT(1 == cupkee_device_push(id, 1, &data));
    CU_ASSERT(0 == TU_object_event_dispatch());

    cupkee_device_sync(10);
    CU_ASSERT(1 == TU_object_event_dispatch());
    CU_ASSERT(mock_handle_arg.id    == id);
    CU_ASSERT(mock_handle_arg.event == CUPKEE_EVENT_DATA);
    CU_ASSERT(1 == cupkee_read(id, 32, buf));

    mock_arg_release();
    // trigger event: drain
    cupkee_write(id, 1, &data);
    cupkee_device_pull(id, 1, &data);

    CU_ASSERT(TU_object_event_dispatch());
    CU_ASSERT(mock_handle_arg.id    == id);
    CU_ASSERT(mock_handle_arg.event == CUPKEE_EVENT_DRAIN);

    cupkee_device_release(id);
}

CU_pSuite test_sys_device(void)
{
    CU_pSuite suite = CU_add_suite("system device", test_setup, test_clean);

    if (suite) {
        CU_add_test(suite, "device request   ", test_request);
        CU_add_test(suite, "device enable    ", test_enable);

        CU_add_test(suite, "device query     ", test_query);
        CU_add_test(suite, "device read      ", test_read);
        CU_add_test(suite, "device write     ", test_write);

        CU_add_test(suite, "device event     ", test_event);
    }

    return suite;
}


