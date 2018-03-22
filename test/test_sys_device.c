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

struct mock_data_t {
    void *entry;
    int inst;
    int id;
    int want;
    int r_req;
    int w_req;
};

struct mock_handle_param_t {
    int id;
    int event;
    int   resp_len;
    void *resp_ptr;
};

static struct mock_data_t mock_data = {
    .entry = NULL,
    .inst = -1,
    .id   = -1,
    .want = 0
};

static struct mock_handle_param_t mock_handle_arg = {
    .id = 0,
    .event = 0,
    .resp_len = 0,
    .resp_ptr = NULL
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

static int mock_setup(int inst, void *entry)
{
    mock_data.entry = entry;
    mock_data.id   = CUPKEE_ENTRY_ID(entry);
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
    mock_handle_arg.id    = CUPKEE_ID_INVALID;
    mock_handle_arg.event = 0;
    mock_handle_arg.resp_len = 0;
    if (mock_handle_arg.resp_ptr) {
        cupkee_free(mock_handle_arg.resp_ptr);
        mock_handle_arg.resp_ptr = NULL;
    }
}

static void mock_arg_release(void)
{
    mock_arg_clean();
}

static int mock_handle(void *entry, int event, intptr_t param)
{
    struct mock_handle_param_t *arg = (struct mock_handle_param_t *)param;

    arg->id = CUPKEE_ENTRY_ID(entry);
    arg->event = event;
    arg->resp_len = cupkee_device_response_take(entry, &arg->resp_ptr);

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

static inline size_t mock_curr_event(void) {
    return mock_handle_arg.event;
}

static inline void *mock_curr_entry(void) {
    return mock_data.entry;
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

static const char *parity_options[] = {
    "none", "odd", "even"
};

static const cupkee_struct_desc_t mock_conf_desc[] = {
    {
        .name = "baudrate",
        .type = CUPKEE_STRUCT_UINT32
    },
    {
        .name = "databits",
        .type = CUPKEE_STRUCT_UINT8
    },
    {
        .name = "parity",
        .type = CUPKEE_STRUCT_OPT,
        .size = 3,
        .opt_names = parity_options
    },
    {
        .name = "stopbits",
        .type = CUPKEE_STRUCT_UINT8
    },
    {
        .name = "channel",
        .type = CUPKEE_STRUCT_OCT,
        .size = 8,
    },
};

static cupkee_struct_t *mock_conf_init(void *curr)
{
    cupkee_struct_t *conf;

    if (curr) {
        conf = curr;
        cupkee_struct_reset(conf);
    } else {
        conf = cupkee_struct_alloc(5, mock_conf_desc);
    }

    if (conf) {
        cupkee_struct_set_uint(conf, 0, 115200);
        cupkee_struct_set_uint(conf, 1, 8);
        cupkee_struct_set_string(conf, 2, "None");
        cupkee_struct_set_uint(conf, 3, 1);
    }

    return conf;
}

static const cupkee_device_desc_t mock_device = {
    .name = "mock",
    .inst_max = 2,
    .conf_init = mock_conf_init,
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
    void *d1;
    void *d2;

    CU_ASSERT(NULL != (d1 = cupkee_device_request("mock", 0)));
    CU_ASSERT(0 == mock_curr_inst());

    CU_ASSERT(NULL != (d2 = cupkee_device_request("mock", 1)));
    CU_ASSERT(1 == mock_curr_inst());

    CU_ASSERT(d1 != d2);
    CU_ASSERT(cupkee_is_device(d1));
    CU_ASSERT(cupkee_is_device(d2));

    cupkee_release(d1);
    TU_object_event_dispatch();
    CU_ASSERT(mock_curr_event() == CUPKEE_EVENT_DESTROY);

    cupkee_release(d2);
    TU_object_event_dispatch();
    CU_ASSERT(mock_curr_event() == CUPKEE_EVENT_DESTROY);
}

static void test_enable(void)
{
    void *d;

    CU_ASSERT_FATAL(NULL != (d = cupkee_device_request("mock", 0)));

    CU_ASSERT(cupkee_is_device(d));
    CU_ASSERT(!cupkee_device_is_enabled(d));

    CU_ASSERT(0 == cupkee_device_enable(d));
    CU_ASSERT(cupkee_device_is_enabled(d));

    CU_ASSERT(0 == cupkee_device_disable(d));
    CU_ASSERT(!cupkee_device_is_enabled(d));

    cupkee_release(d);
    TU_object_event_dispatch();
    CU_ASSERT(mock_curr_event() == CUPKEE_EVENT_DESTROY);
}

static void test_query(void)
{
    void *d;
    void *req;
    uint8_t buf[2];

    mock_arg_release();

    CU_ASSERT_FATAL(NULL != (d = cupkee_device_request("mock", 0)));

    CU_ASSERT(!cupkee_device_is_enabled(d));
    CU_ASSERT(0 == cupkee_device_enable(d));

    /*
     * query without request data
     */
    CU_ASSERT(0 == cupkee_device_query(d, 0, NULL, 8, mock_handle, (intptr_t)&mock_handle_arg));

    // Bsp driver code start
    CU_ASSERT(CUPKEE_ENTRY_ID(d) == mock_curr_id());
    CU_ASSERT(8  == mock_curr_want());
    // take request data, there is NULL
    CU_ASSERT(NULL != (req = cupkee_device_request_buffer(d)));
    CU_ASSERT(0 == cupkee_buffer_length(req));

    // push reply to device
    CU_ASSERT(8   == cupkee_device_response_push(d, 8, "12345678"));
    // Call response_end to complete query
    cupkee_device_response_end(d);
    // Bsp driver code end

    // handle should be called
    CU_ASSERT(TU_object_event_dispatch());
    CU_ASSERT(mock_handle_arg.id    == CUPKEE_ENTRY_ID(d));
    CU_ASSERT(mock_handle_arg.event == CUPKEE_EVENT_RESPONSE);
    CU_ASSERT(8 == mock_handle_arg.resp_len);

    mock_arg_release();

    /*
     * query with request data
     */
    CU_ASSERT(0 == cupkee_device_query(d, 5, "hihao", 8, mock_handle, (intptr_t)&mock_handle_arg));

    // Bsp driver code start
    CU_ASSERT(d == mock_curr_entry());
    CU_ASSERT(CUPKEE_ENTRY_ID(d) == mock_curr_id());
    CU_ASSERT(8 == mock_curr_want());
    // take request data
    CU_ASSERT(NULL != (req = cupkee_device_request_buffer(d)));
    CU_ASSERT(5 == cupkee_buffer_length(req));
    cupkee_buffer_deinit(req);

    // push reply to device
    CU_ASSERT(8   == cupkee_device_response_push(d, 8, "12345678"));
    CU_ASSERT(0   == cupkee_device_response_push(d, 8, "12345678"));

    // Call response_end to complete query
    cupkee_device_response_end(d);
    // Bsp driver code end

    CU_ASSERT(TU_object_event_dispatch());
    CU_ASSERT(mock_handle_arg.id    == CUPKEE_ENTRY_ID(d));
    CU_ASSERT(mock_handle_arg.event == CUPKEE_EVENT_RESPONSE);
    CU_ASSERT(8 == mock_handle_arg.resp_len);
    mock_arg_release();

    /*
     * query without response data
     */

    CU_ASSERT(0 == cupkee_device_query(d, 7, "0123456", 0, mock_handle, (intptr_t)&mock_handle_arg));

    // Bsp driver code start
    CU_ASSERT(CUPKEE_ENTRY_ID(d) == mock_curr_id());
    CU_ASSERT(0   == mock_curr_want());
    // take request data
    CU_ASSERT(2   == cupkee_device_request_load(d, 2, buf));
    CU_ASSERT('0' == buf[0] && '1' == buf[1]);
    CU_ASSERT(2   == cupkee_device_request_load(d, 2, buf));
    CU_ASSERT('2' == buf[0] && '3' == buf[1]);
    CU_ASSERT(2   == cupkee_device_request_load(d, 2, buf));
    CU_ASSERT('4' == buf[0] && '5' == buf[1]);
    CU_ASSERT(1   == cupkee_device_request_load(d, 2, buf));
    CU_ASSERT('6' == buf[0]);
    // push reply to device
    CU_ASSERT(0 == cupkee_device_response_push(d, 8, "12345678"));
    // Call response_end to complete query
    cupkee_device_response_end(d);
    // Bsp driver code end

    CU_ASSERT(TU_object_event_dispatch());
    CU_ASSERT(mock_handle_arg.id    == CUPKEE_ENTRY_ID(d));
    CU_ASSERT(mock_handle_arg.event == CUPKEE_EVENT_RESPONSE);
    CU_ASSERT(0 == mock_handle_arg.resp_len);

    CU_ASSERT(0 == cupkee_device_disable(d));
    CU_ASSERT(!cupkee_device_is_enabled(d));

    cupkee_release(d);
}

static void test_read(void)
{
    void *dev;
    uint8_t data;
    char buf[16];
    int i;

    CU_ASSERT_FATAL(NULL != (dev = cupkee_device_request("mock", 1)));

    CU_ASSERT(!cupkee_device_is_enabled(dev));
    CU_ASSERT(0 == cupkee_device_enable(dev));

    CU_ASSERT(0 == cupkee_read(dev, 16, buf));

    CU_ASSERT(CUPKEE_ENTRY_ID(dev) == mock_curr_id());
    CU_ASSERT(1  == mock_curr_inst());
    for (i = 0; i < 16; i++) {
        data = i;
        cupkee_device_push(dev, 1, &data);
    }
    CU_ASSERT(16 == cupkee_read(dev, 16, buf));
    CU_ASSERT(buf[0] == 0 && buf[15] == 15);

    CU_ASSERT(8 == cupkee_read_sync(dev, 8, buf));
    CU_ASSERT(buf[0] == 0x11 && buf[7] == 0x11);

    cupkee_release(dev);
}

static void test_write(void)
{
    void *dev;
    char buf[16];

    CU_ASSERT_FATAL(NULL != (dev = cupkee_device_request("mock", 2)));

    CU_ASSERT(!cupkee_device_is_enabled(dev));
    CU_ASSERT(0 == cupkee_device_enable(dev));

    memset(buf, 3, 16);
    CU_ASSERT(16 == cupkee_write(dev, 16, buf));

    CU_ASSERT(CUPKEE_ENTRY_ID(dev) == mock_curr_id());
    CU_ASSERT(2  == mock_curr_inst());

    memset(buf, 0, 16);
    CU_ASSERT(16 == cupkee_device_pull(dev, 16, buf));
    CU_ASSERT(buf[0] == 3 && buf[15] == 3);

    CU_ASSERT(8 == cupkee_write_sync(dev, 8, buf));

    cupkee_release(dev);
}

static void test_event(void)
{
    void *dev;
    int n;
    uint8_t data;
    char buf[10];

    CU_ASSERT_FATAL(NULL != (dev = cupkee_device_request("mock", 2)));

    CU_ASSERT(!cupkee_device_is_enabled(dev));
    CU_ASSERT(0 == cupkee_device_enable(dev));

    CU_ASSERT(0 == cupkee_device_handle_set(dev, mock_handle, (intptr_t) &mock_handle_arg));

    cupkee_listen(dev, CUPKEE_EVENT_DATA);
    cupkee_listen(dev, CUPKEE_EVENT_DRAIN);

    CU_ASSERT(0 == TU_object_event_dispatch());

    mock_arg_release();
    // trigger event: data
    data = 0;
    while (1 == cupkee_device_push(dev, 1, &data)) {
        data++;
    }

    CU_ASSERT(TU_object_event_dispatch());
    CU_ASSERT(mock_handle_arg.id    == CUPKEE_ENTRY_ID(dev));
    CU_ASSERT(mock_handle_arg.event == CUPKEE_EVENT_DATA);
    while (0 < (n = cupkee_read(dev, 10, buf))) {
        data -= n;
    }
    CU_ASSERT(data == 0);

    while (TU_object_event_dispatch())
        ; /* clean event */

    mock_arg_release();
    // trigger event: data
    _cupkee_systicks = 0;
    data = 9;
    CU_ASSERT(1 == cupkee_device_push(dev, 1, &data));
    CU_ASSERT(0 == TU_object_event_dispatch());

    cupkee_device_sync(30);
    CU_ASSERT(1 == TU_object_event_dispatch());
    CU_ASSERT(mock_handle_arg.id    == CUPKEE_ENTRY_ID(dev));
    CU_ASSERT(mock_handle_arg.event == CUPKEE_EVENT_DATA);
    CU_ASSERT(1 == cupkee_read(dev, 10, buf));

    // tirgger nothing (read buffer is empty)
    cupkee_device_sync(40);
    CU_ASSERT(0 == TU_object_event_dispatch());

    mock_arg_release();
    // trigger event: drain
    cupkee_write(dev, 1, &data);
    cupkee_device_pull(dev, 1, &data);

    CU_ASSERT(TU_object_event_dispatch());
    CU_ASSERT(mock_handle_arg.id    == CUPKEE_ENTRY_ID(dev));
    CU_ASSERT(mock_handle_arg.event == CUPKEE_EVENT_DRAIN);

    cupkee_release(dev);
}

static void test_config(void)
{
    void *dev;
    intptr_t n;
    const uint8_t *seq;

    CU_ASSERT_FATAL(NULL != (dev = cupkee_device_request("mock", 2)));

    // default config
    CU_ASSERT(cupkee_prop_get(dev, "baudrate", &n) == CUPKEE_OBJECT_ELEM_INT && n == 115200);
    CU_ASSERT(cupkee_prop_get(dev, "databits", &n) == CUPKEE_OBJECT_ELEM_INT && n == 8);
    CU_ASSERT(cupkee_prop_get(dev, "parity",   &n) == CUPKEE_OBJECT_ELEM_STR && (const char *)n == parity_options[0]);
    CU_ASSERT(cupkee_prop_get(dev, "stopbits", &n) == CUPKEE_OBJECT_ELEM_INT && n == 1);
    CU_ASSERT(cupkee_prop_get(dev, "channel",  &n) == CUPKEE_OBJECT_ELEM_OCT);

    // update config
    CU_ASSERT(cupkee_prop_set(dev, "baudrate", CUPKEE_OBJECT_ELEM_INT, 9600) > 0);
    return;
    CU_ASSERT(cupkee_prop_set(dev, "databits", CUPKEE_OBJECT_ELEM_INT, 9) > 0);
    CU_ASSERT(cupkee_prop_set(dev, "parity",   CUPKEE_OBJECT_ELEM_STR, (intptr_t)"odd") > 0);
    CU_ASSERT(cupkee_prop_set(dev, "stopbits", CUPKEE_OBJECT_ELEM_INT, 2) > 0);

    CU_ASSERT(cupkee_prop_set(dev, "channel", CUPKEE_OBJECT_ELEM_INT, 7) > 0);
    CU_ASSERT(cupkee_prop_set(dev, "channel", CUPKEE_OBJECT_ELEM_INT, 6) > 0);
    CU_ASSERT(cupkee_prop_set(dev, "channel", CUPKEE_OBJECT_ELEM_INT, 5) > 0);
    CU_ASSERT(cupkee_prop_set(dev, "channel", CUPKEE_OBJECT_ELEM_INT, 4) > 0);
    CU_ASSERT(cupkee_prop_set(dev, "channel", CUPKEE_OBJECT_ELEM_INT, 3) > 0);
    CU_ASSERT(cupkee_prop_set(dev, "channel", CUPKEE_OBJECT_ELEM_INT, 2) > 0);
    CU_ASSERT(cupkee_prop_set(dev, "channel", CUPKEE_OBJECT_ELEM_INT, 1) > 0);
    CU_ASSERT(cupkee_prop_set(dev, "channel", CUPKEE_OBJECT_ELEM_INT, 0) > 0);

    CU_ASSERT(cupkee_prop_get(dev, "baudrate", &n) == CUPKEE_OBJECT_ELEM_INT && n == 9600);
    CU_ASSERT(cupkee_prop_get(dev, "databits", &n) == CUPKEE_OBJECT_ELEM_INT && n == 9);
    CU_ASSERT(cupkee_prop_get(dev, "parity",   &n) == CUPKEE_OBJECT_ELEM_STR && (const char *)n == parity_options[1]);
    CU_ASSERT(cupkee_prop_get(dev, "stopbits", &n) == CUPKEE_OBJECT_ELEM_INT && n == 2);
    CU_ASSERT(cupkee_prop_get(dev, "channel",  &n) == CUPKEE_OBJECT_ELEM_OCT);
    seq = (uint8_t *) n;
    CU_ASSERT(seq[0] == 8 && seq[1] == 7 && seq[8] == 0);

    cupkee_release(dev);
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

        CU_add_test(suite, "device config    ", test_config);
    }

    return suite;
}


