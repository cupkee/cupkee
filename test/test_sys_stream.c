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

static int     implement_consume_cnt;
static uint8_t implement_send_trigger_cnt;
static uint8_t implement_load_trigger_cnt;
static uint8_t implement_data;
static uint8_t implement_event = 0xff;

static int test_setup(void)
{
    return TU_pre_init();
}

static int test_clean(void)
{
    return TU_pre_deinit();
}

static int event_match(uint8_t code)
{
    if (implement_event == code) {
        implement_event = 0xFF;
        return 1;
    } else {
        return 0;
    }
}

static void rw_implement_loop(cupkee_stream_t *s)
{
    uint8_t data;

    if (implement_load_trigger_cnt) {
        int space = cupkee_stream_rx_cache_space(s);

        while (space && 1 == cupkee_stream_pull(s, 1, &data)) {
            cupkee_stream_push(s, 1, &data);
            space--;
        }

        if (space == 0) {
            implement_load_trigger_cnt = 0;
        }
    }
}

static int read_implement_load(cupkee_stream_t *s, int n)
{
    int count = 0;
    while (implement_load_trigger_cnt && count < n) {
        if (1 != cupkee_stream_push(s, 1, &implement_data)) {
            implement_load_trigger_cnt = 0;
            break;
        }
        count++;
    }
    return count;
}

static void read_implement_idle(cupkee_stream_t *s, size_t n)
{
    (void) s;
    (void) n;
}

static void read_implement_immediately(cupkee_stream_t *s, size_t n)
{
    (void) n;

    while (1 == cupkee_stream_push(s, 1, &implement_data))
        ;
}

static void read_implement_trigger(cupkee_stream_t *s, size_t n)
{
    (void) s;
    (void) n;
    implement_load_trigger_cnt++;
}

static void read_implement_loop(cupkee_stream_t *s, size_t n)
{
    (void) n;

    implement_load_trigger_cnt++;

    rw_implement_loop(s);
}

static int write_implement_consume(cupkee_stream_t *s, int n)
{
    int count = 0;
    uint8_t data;

    while (count < n && 1 == cupkee_stream_pull(s, 1, &data)) {
        implement_consume_cnt++;
        count++;
    }
    return count;
}

static void write_implement_immediately(cupkee_stream_t *s)
{
    write_implement_consume(s, 9999);
}

static void write_implement_trigger(cupkee_stream_t *s)
{
    (void) s;
    implement_send_trigger_cnt++;
}

static void write_implement_loop(cupkee_stream_t *s)
{
    rw_implement_loop(s);
}

static void event_handle(cupkee_event_emitter_t *emitter, uint8_t code)
{
    (void) emitter;
    implement_event = code;
}

static void test_readable_init(void)
{
    cupkee_event_emitter_t emitter;
    cupkee_stream_t stream;

    CU_ASSERT(0 <= cupkee_event_emitter_init(&emitter, NULL));

    CU_ASSERT(0 != cupkee_stream_init_readable(NULL, &emitter, 32, read_implement_idle));
    CU_ASSERT(0 != cupkee_stream_init_readable(&stream, &emitter, 32, NULL));

    // init without emitter
    CU_ASSERT(0 == cupkee_stream_init_readable(&stream, NULL, 32, read_implement_idle));

    // init with emitter
    CU_ASSERT(0 == cupkee_stream_init_readable(&stream, &emitter, 32, read_implement_idle));

    cupkee_event_emitter_deinit(&emitter);
    cupkee_stream_deinit(&stream);
}

static void test_writable_init(void)
{
    cupkee_event_emitter_t emitter;
    cupkee_stream_t stream;

    CU_ASSERT(0 <= cupkee_event_emitter_init(&emitter, NULL));

    CU_ASSERT(0 != cupkee_stream_init_writable(NULL, &emitter, 32, write_implement_immediately));
    CU_ASSERT(0 != cupkee_stream_init_writable(&stream, &emitter, 32, NULL));

    // init without emitter
    CU_ASSERT(0 == cupkee_stream_init_writable(&stream, NULL, 32, write_implement_immediately));

    // init with emitter
    CU_ASSERT(0 == cupkee_stream_init_writable(&stream, &emitter, 32, write_implement_immediately));

    cupkee_stream_deinit(&stream);
    cupkee_event_emitter_deinit(&emitter);
}

static void test_duplex_init(void)
{
}

static void test_read_immediately(void)
{
    cupkee_stream_t stream;
    uint8_t buf[30];

    // fill rx cache with 1
    implement_data = 1;

    CU_ASSERT(0 == cupkee_stream_init_readable(&stream, NULL, 30, read_implement_immediately));

    CU_ASSERT(30 == cupkee_stream_read(&stream, 100, buf));

    memset(buf, 0, 30);

    // appends 2 to rx cache, after read
    implement_data = 2;
    CU_ASSERT(10 == cupkee_stream_read(&stream, 10, buf));
    CU_ASSERT(buf[0] == 1 && buf[9] == 1);

    // appends 3 to rx cache, after read
    implement_data = 3;
    CU_ASSERT(10 == cupkee_stream_read(&stream, 10, buf + 10));
    CU_ASSERT(buf[10] == 1 && buf[19] == 1);

    memset(buf, 0, 30);
    CU_ASSERT(30 == cupkee_stream_read(&stream, 30, buf));
    CU_ASSERT(buf[0] == 1 && buf[9] == 1);
    CU_ASSERT(buf[10] == 2 && buf[19] == 2);
    CU_ASSERT(buf[20] == 3 && buf[29] == 3);

    cupkee_stream_deinit(&stream);
}

static void test_read_trigger(void)
{
    cupkee_stream_t stream;
    uint8_t buf[32];

    CU_ASSERT(0 == cupkee_stream_init_readable(&stream, NULL, 32, read_implement_trigger));

    implement_load_trigger_cnt = 0;
    // rx cache is empty, trigger load
    CU_ASSERT(0 == cupkee_stream_read(&stream, 10, buf));
    CU_ASSERT(1 == implement_load_trigger_cnt);

    // full stream rx buffer with 1
    implement_data = 1;
    CU_ASSERT(32 == read_implement_load(&stream, 999));
    CU_ASSERT(0 == implement_load_trigger_cnt);

    memset(buf, 0, 32);
    CU_ASSERT(10 == cupkee_stream_read(&stream, 10, buf));
    CU_ASSERT(buf[0] == 1 && buf[9] == 1);
    // load should be trigger, since the reading on full cache
    CU_ASSERT(1 == implement_load_trigger_cnt);

    memset(buf, 0, 32);
    CU_ASSERT(10 == cupkee_stream_read(&stream, 10, buf));
    CU_ASSERT(buf[0] == 1 && buf[9] == 1);
    // load should not be trigger
    CU_ASSERT(1 == implement_load_trigger_cnt);

    // Cached less than want, should return all cached
    memset(buf, 0, 32);
    CU_ASSERT(12 == cupkee_stream_read(&stream, 13, buf));
    CU_ASSERT(buf[0] == 1 && buf[11] == 1);

    // load should not be trigger
    CU_ASSERT(1 == implement_load_trigger_cnt);

    CU_ASSERT(32 == read_implement_load(&stream, 999));
    CU_ASSERT(0 == implement_load_trigger_cnt);

    // Cached equal want
    CU_ASSERT(32 == cupkee_stream_read(&stream, 32, buf));

    CU_ASSERT(32 == read_implement_load(&stream, 999));

    cupkee_stream_deinit(&stream);
}

static void test_write_immediately(void)
{
    cupkee_stream_t stream;

    implement_consume_cnt = 0;
    CU_ASSERT(0 == cupkee_stream_init_writable(&stream, NULL, 32, write_implement_immediately));

    CU_ASSERT(1 == cupkee_stream_write(&stream, 1, "test code"));
    CU_ASSERT(1 == implement_consume_cnt);
    CU_ASSERT(2 == cupkee_stream_write(&stream, 2, "test code"));
    CU_ASSERT(3 == implement_consume_cnt);
    CU_ASSERT(9 == cupkee_stream_write(&stream, 9, "test code"));
    CU_ASSERT(12 == implement_consume_cnt);

    cupkee_stream_deinit(&stream);
}

static void test_write_trigger(void)
{
    cupkee_stream_t stream;

    implement_consume_cnt = 0;
    implement_send_trigger_cnt = 0;

    CU_ASSERT(0 == cupkee_stream_init_writable(&stream, NULL, 16, write_implement_trigger));
    CU_ASSERT(16 == cupkee_stream_writable(&stream));

    // trigger send, when first write to writable
    CU_ASSERT(10 == cupkee_stream_write(&stream, 10, "0123456789"));
    CU_ASSERT(1 == implement_send_trigger_cnt);

    CU_ASSERT(6 == cupkee_stream_writable(&stream));
    CU_ASSERT(6 == cupkee_stream_write(&stream, 10, "0123456789"));
    CU_ASSERT(0 == cupkee_stream_writable(&stream));

    CU_ASSERT(2 == write_implement_consume(&stream, 2));
    CU_ASSERT(2 == cupkee_stream_writable(&stream));

    // send would not be trigger, when write data to tx_buf not empty
    CU_ASSERT(2 == cupkee_stream_write(&stream, 10, "0123456789"));
    CU_ASSERT(1 == implement_send_trigger_cnt);

    // clear tx_buff
    CU_ASSERT(16 == write_implement_consume(&stream, 20));

    // trigger send, when write data to tx_buf empty
    CU_ASSERT(10 == cupkee_stream_write(&stream, 10, "0123456789"));
    CU_ASSERT(10 == write_implement_consume(&stream, 20));
    CU_ASSERT(2 == implement_send_trigger_cnt);

    CU_ASSERT(10 == cupkee_stream_write(&stream, 10, "0123456789"));
    CU_ASSERT(10 == write_implement_consume(&stream, 20));
    CU_ASSERT(3 == implement_send_trigger_cnt);

    cupkee_stream_deinit(&stream);
}

static void test_rw_loop(void)
{
    cupkee_stream_t stream;
    char buf[16];

    implement_load_trigger_cnt = 0;
    CU_ASSERT(0 == cupkee_stream_init_duplex(&stream, NULL, 16, 16, read_implement_loop, write_implement_loop));
    CU_ASSERT(16 == cupkee_stream_writable(&stream));

    CU_ASSERT(10 == cupkee_stream_write(&stream, 10, "0123456789"));
    CU_ASSERT(10 == cupkee_stream_read(&stream, 10, buf));
    CU_ASSERT(1 == implement_load_trigger_cnt);
    CU_ASSERT(0 == memcmp(buf, "0123456789", 10));

    CU_ASSERT(10 == cupkee_stream_write(&stream, 10, "0123456789"));
    CU_ASSERT(16 == cupkee_stream_writable(&stream));
    CU_ASSERT(1 == implement_load_trigger_cnt);
    CU_ASSERT(10 == cupkee_stream_readable(&stream));

    CU_ASSERT(10 == cupkee_stream_write(&stream, 10, "0123456789"));
    CU_ASSERT(0 == implement_load_trigger_cnt);

    CU_ASSERT(16 == cupkee_stream_readable(&stream));

    CU_ASSERT(12 == cupkee_stream_writable(&stream));
    CU_ASSERT(12 == cupkee_stream_write(&stream, 16, "0123456789012345"));
    CU_ASSERT(0 == implement_load_trigger_cnt);

    CU_ASSERT(16 == cupkee_stream_readable(&stream));
    CU_ASSERT( 0 == cupkee_stream_writable(&stream));

    CU_ASSERT(16 == cupkee_stream_read(&stream, 16, buf));
    CU_ASSERT(16 == cupkee_stream_read(&stream, 16, buf));

    cupkee_stream_deinit(&stream);
}

static void test_event_error(void)
{
    cupkee_stream_t stream;
    cupkee_event_emitter_t emitter;

    CU_ASSERT(0 <= cupkee_event_emitter_init(&emitter, event_handle));
    CU_ASSERT(0 == cupkee_stream_init_duplex(&stream, &emitter, 16, 16, read_implement_loop, write_implement_loop));

    cupkee_stream_set_error(&stream, 1);

    CU_ASSERT(1 == TU_emitter_event_dispatch());
    CU_ASSERT(event_match(CUPKEE_EVENT_STREAM_ERROR));
    CU_ASSERT(-1 == cupkee_stream_get_error(&stream));

    cupkee_stream_deinit(&stream);
    cupkee_event_emitter_deinit(&emitter);
}

static void test_event_data(void)
{
    cupkee_stream_t stream;
    cupkee_event_emitter_t emitter;
    uint8_t buf[30];

    implement_load_trigger_cnt = 0;

    CU_ASSERT(0 <= cupkee_event_emitter_init(&emitter, event_handle));
    CU_ASSERT(0 == cupkee_stream_init_readable(&stream, &emitter, 30, read_implement_trigger));
    CU_ASSERT(0 == cupkee_stream_readable(&stream));

    cupkee_stream_resume(&stream);

    read_implement_load(&stream, 999);
    CU_ASSERT(1 == TU_emitter_event_dispatch());
    CU_ASSERT(event_match(CUPKEE_EVENT_STREAM_DATA));
    // event post once
    CU_ASSERT(0 == TU_emitter_event_dispatch());

    // event should not post, rx_buf not be read clean
    CU_ASSERT(10 == cupkee_stream_read(&stream, 10, buf));
    CU_ASSERT(20 == cupkee_stream_readable(&stream));
    read_implement_load(&stream, 999);
    CU_ASSERT(30 == cupkee_stream_readable(&stream));
    CU_ASSERT(0 == TU_emitter_event_dispatch());

    // event should post, rx_buf be read clean
    CU_ASSERT(30 == cupkee_stream_read(&stream, 30, buf));
    CU_ASSERT(0 == cupkee_stream_readable(&stream));
    read_implement_load(&stream, 999);
    CU_ASSERT(30 == cupkee_stream_readable(&stream));
    CU_ASSERT(1 == TU_emitter_event_dispatch());
    CU_ASSERT(event_match(CUPKEE_EVENT_STREAM_DATA));

    cupkee_stream_deinit(&stream);
    cupkee_event_emitter_deinit(&emitter);
}

static void test_event_drain(void)
{
    cupkee_stream_t stream;
    cupkee_event_emitter_t emitter;

    implement_consume_cnt = 0;
    implement_send_trigger_cnt = 0;

    CU_ASSERT(0 <= cupkee_event_emitter_init(&emitter, event_handle));
    CU_ASSERT(0 == cupkee_stream_init_writable(&stream, &emitter, 16, write_implement_trigger));
    CU_ASSERT(16 == cupkee_stream_writable(&stream));

    // trigger send, when first write to writable
    CU_ASSERT(10 == cupkee_stream_write(&stream, 10, "0123456789"));
    CU_ASSERT(1 == implement_send_trigger_cnt);

    CU_ASSERT(6 == cupkee_stream_write(&stream, 10, "0123456789"));
    CU_ASSERT(1 == implement_send_trigger_cnt);
    CU_ASSERT(0 == cupkee_stream_writable(&stream));
    CU_ASSERT(0 == cupkee_stream_write(&stream, 10, "0123456789"));

    write_implement_consume(&stream, 5);
    CU_ASSERT(1 == TU_emitter_event_dispatch());
    CU_ASSERT(event_match(CUPKEE_EVENT_STREAM_DRAIN));

    // post 'drain' once
    write_implement_consume(&stream, 5);
    CU_ASSERT(0 == TU_emitter_event_dispatch());
    CU_ASSERT(!event_match(CUPKEE_EVENT_STREAM_DRAIN));

    CU_ASSERT(10 == cupkee_stream_writable(&stream));
    CU_ASSERT(10 == cupkee_stream_write(&stream, 10, "0123456789"));
    CU_ASSERT(0 == cupkee_stream_writable(&stream));

    // not 'drain' post, if write block not block
    write_implement_consume(&stream, 5);
    CU_ASSERT(0 == TU_emitter_event_dispatch());
    CU_ASSERT(!event_match(CUPKEE_EVENT_STREAM_DRAIN));

    CU_ASSERT(5 == cupkee_stream_write(&stream, 10, "0123456789"));
    write_implement_consume(&stream, 1);
    CU_ASSERT(1 == TU_emitter_event_dispatch());
    CU_ASSERT(event_match(CUPKEE_EVENT_STREAM_DRAIN));

    cupkee_stream_deinit(&stream);
    cupkee_event_emitter_deinit(&emitter);
}

static void test_shutdown_writable(void)
{
    cupkee_stream_t stream;
    cupkee_event_emitter_t emitter;

    implement_consume_cnt = 0;
    implement_send_trigger_cnt = 0;

    CU_ASSERT(0 <= cupkee_event_emitter_init(&emitter, event_handle));

    CU_ASSERT(0 == cupkee_stream_init_writable(&stream, &emitter, 16, write_implement_trigger));
    cupkee_stream_shutdown(&stream, CUPKEE_STREAM_FL_WRITABLE);
    CU_ASSERT(1 == TU_emitter_event_dispatch());
    CU_ASSERT(event_match(CUPKEE_EVENT_STREAM_FINISH));
    CU_ASSERT(0 == cupkee_stream_writable(&stream));

    CU_ASSERT(0 == cupkee_stream_init_writable(&stream, &emitter, 16, write_implement_trigger));
    CU_ASSERT(16 == cupkee_stream_writable(&stream));

    CU_ASSERT(10 == cupkee_stream_write(&stream, 10, "0123456789"));
    CU_ASSERT(1  == write_implement_consume(&stream, 1));

    cupkee_stream_shutdown(&stream, CUPKEE_STREAM_FL_WRITABLE);
    CU_ASSERT(0 == cupkee_stream_writable(&stream));
    CU_ASSERT(0 == TU_emitter_event_dispatch());

    CU_ASSERT(0 == cupkee_stream_write(&stream, 10, "0123456789"));

    CU_ASSERT(9 == write_implement_consume(&stream, 10));
    CU_ASSERT(1 == TU_emitter_event_dispatch());
    CU_ASSERT(event_match(CUPKEE_EVENT_STREAM_FINISH));
    CU_ASSERT(0 == TU_emitter_event_dispatch());

    cupkee_stream_deinit(&stream);
    cupkee_event_emitter_deinit(&emitter);
}

static void test_shutdown_readable(void)
{
    cupkee_stream_t stream;
    cupkee_event_emitter_t emitter;
    uint8_t buf[30];

    CU_ASSERT(0 <= cupkee_event_emitter_init(&emitter, event_handle));

    CU_ASSERT(0 == cupkee_stream_init_readable(&stream, &emitter, 30, read_implement_trigger));
    cupkee_stream_shutdown(&stream, CUPKEE_STREAM_FL_READABLE);
    CU_ASSERT(1 == TU_emitter_event_dispatch());
    CU_ASSERT(event_match(CUPKEE_EVENT_STREAM_END));

    CU_ASSERT(0 == cupkee_stream_init_readable(&stream, &emitter, 30, read_implement_trigger));

    cupkee_stream_resume(&stream);
    read_implement_load(&stream, 999);

    CU_ASSERT(30 == cupkee_stream_readable(&stream));
    CU_ASSERT(10 == cupkee_stream_read(&stream, 10, buf));
    CU_ASSERT(20 == cupkee_stream_readable(&stream));
    CU_ASSERT(1 == TU_emitter_event_dispatch());
    CU_ASSERT(event_match(CUPKEE_EVENT_STREAM_DATA));

    cupkee_stream_shutdown(&stream, CUPKEE_STREAM_FL_READABLE);
    CU_ASSERT(0 == read_implement_load(&stream, 999));
    CU_ASSERT(20 == cupkee_stream_readable(&stream));

    CU_ASSERT(10 == cupkee_stream_read(&stream, 10, buf));
    CU_ASSERT(0 == TU_emitter_event_dispatch());

    CU_ASSERT(10 == cupkee_stream_read(&stream, 10, buf));
    CU_ASSERT(1 == TU_emitter_event_dispatch());
    CU_ASSERT(event_match(CUPKEE_EVENT_STREAM_END));

    CU_ASSERT(0 == read_implement_load(&stream, 999));

    cupkee_stream_deinit(&stream);
    cupkee_event_emitter_deinit(&emitter);
}

static void test_pipe(void)
{
    cupkee_stream_t reader, writer;

    implement_load_trigger_cnt = 0;
    implement_send_trigger_cnt = 0;

    CU_ASSERT(0 == cupkee_stream_init_readable(&reader, NULL, 30, read_implement_trigger));
    CU_ASSERT(0 == cupkee_stream_init_writable(&writer, NULL, 30, write_implement_trigger));

    CU_ASSERT(implement_send_trigger_cnt == 0);
    CU_ASSERT(implement_load_trigger_cnt == 0);

    CU_ASSERT(0 == cupkee_stream_pipe(&reader, &writer));

    CU_ASSERT(implement_load_trigger_cnt == 1);
    CU_ASSERT(implement_send_trigger_cnt == 0);

    // push data, not full
    CU_ASSERT(15 == read_implement_load(&reader, 15));

    CU_ASSERT(implement_load_trigger_cnt == 1);
    CU_ASSERT(implement_send_trigger_cnt == 1);

    // pull data, not empty
    CU_ASSERT(14 == write_implement_consume(&writer, 14));

    CU_ASSERT(implement_load_trigger_cnt == 1);
    CU_ASSERT(implement_send_trigger_cnt == 1);

    // pull data, empty
    CU_ASSERT(1 == write_implement_consume(&writer, 14));

    CU_ASSERT(implement_load_trigger_cnt == 1);
    CU_ASSERT(implement_send_trigger_cnt == 1);

    // push data, full
    CU_ASSERT(30 == read_implement_load(&reader, 999));

    CU_ASSERT(implement_load_trigger_cnt == 0);
    CU_ASSERT(implement_send_trigger_cnt == 2);

    // pull data, empty
    CU_ASSERT(30 == write_implement_consume(&writer, 30));
    CU_ASSERT(implement_load_trigger_cnt == 1);
    CU_ASSERT(implement_send_trigger_cnt == 2);

    // push data, not full
    CU_ASSERT(15 == read_implement_load(&reader, 15));

    CU_ASSERT(implement_load_trigger_cnt == 1);
    CU_ASSERT(implement_send_trigger_cnt == 3);

    CU_ASSERT(15 == write_implement_consume(&writer, 999));

    CU_ASSERT(implement_load_trigger_cnt == 1);
    CU_ASSERT(implement_send_trigger_cnt == 3);

    cupkee_stream_deinit(&reader);
    cupkee_stream_deinit(&writer);
}

static void test_unpipe(void)
{
    cupkee_stream_t reader, writer;
    char buf[30];

    implement_load_trigger_cnt = 0;
    implement_send_trigger_cnt = 0;

    CU_ASSERT(0 == cupkee_stream_init_readable(&reader, NULL, 30, read_implement_trigger));
    CU_ASSERT(0 == cupkee_stream_init_writable(&writer, NULL, 30, write_implement_trigger));

    CU_ASSERT(implement_send_trigger_cnt == 0);
    CU_ASSERT(implement_load_trigger_cnt == 0);

    CU_ASSERT(0 == cupkee_stream_pipe(&reader, &writer));

    CU_ASSERT(implement_load_trigger_cnt == 1);
    CU_ASSERT(implement_send_trigger_cnt == 0);

    CU_ASSERT(30 == read_implement_load(&reader, 999));
    CU_ASSERT(30 == write_implement_consume(&writer, 999));

    CU_ASSERT(0 == cupkee_stream_unpipe(&reader));

    CU_ASSERT(30 == cupkee_stream_write(&writer, 30, buf));
    CU_ASSERT(30 == write_implement_consume(&writer, 999));

    CU_ASSERT(0 == cupkee_stream_read(&reader, 30, buf));
    CU_ASSERT(30 == read_implement_load(&reader, 999));

    cupkee_stream_deinit(&reader);
    cupkee_stream_deinit(&writer);
}

CU_pSuite test_sys_stream(void)
{
    CU_pSuite suite = CU_add_suite("system stream", test_setup, test_clean);

    if (suite) {
        CU_add_test(suite, "readable init    ", test_readable_init);
        CU_add_test(suite, "read immediately ", test_read_immediately);
        CU_add_test(suite, "read trigger     ", test_read_trigger);

        CU_add_test(suite, "writable init    ", test_writable_init);
        CU_add_test(suite, "write immediately", test_write_immediately);
        CU_add_test(suite, "write trigger    ", test_write_trigger);

        CU_add_test(suite, "duplex init      ", test_duplex_init);
        CU_add_test(suite, "r&w loop         ", test_rw_loop);

        CU_add_test(suite, "event error      ", test_event_error);
        CU_add_test(suite, "event data       ", test_event_data);
        CU_add_test(suite, "event drain      ", test_event_drain);

        CU_add_test(suite, "shutdown readable", test_shutdown_readable);
        CU_add_test(suite, "shutdown writable", test_shutdown_writable);

        CU_add_test(suite, "pipe             ", test_pipe);
        CU_add_test(suite, "unpipe           ", test_unpipe);
    }

    return suite;
}

