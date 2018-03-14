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

static int tag;

static int mock_curr_id, mock_curr_event;
static int mock_read_immediately = 0;
static int mock_read_trigger = 0;
static int mock_write_immediately = 0;
static int mock_write_trigger = 0;

static int mock_read(cupkee_stream_t *s, size_t n, void *buf)
{
    if (buf) { // sync read
        uint8_t *pbuf = buf;
        size_t i;
        for (i = 0; i < n; i++) {
            pbuf[i] = 0xf5;
        }
        return i;
    }

    if (mock_read_immediately) {
        uint8_t data = mock_read_immediately;
        size_t i;

        for (i = 0; i < n; i++) {
            if (1 != cupkee_stream_push(s, 1, &data)) {
                break;
            }
        }
        return i;
    } else {
        mock_read_trigger ++;
    }

    return 0;
}

static int mock_write(cupkee_stream_t *s, size_t n, const void *data)
{
    (void) data;

    if (data) { // sync
        return n;
    }

    if (mock_write_immediately > 0) {
        size_t i = 0;
        uint8_t buf;

        while (1 == cupkee_stream_pull(s, 1, &buf))
            i++;

        return i;
    } else {
        mock_write_trigger++;
    }

    return 0;
}

static void stream_event_handle(void *entry, uint8_t event)
{
    mock_curr_id = CUPKEE_ENTRY_ID(entry);
    mock_curr_event = event;
}

static const cupkee_desc_t stream_desc = {
    .event_handle = stream_event_handle
};

static int test_setup(void)
{
    TU_pre_init();

    if (0 > (tag = cupkee_object_register(sizeof(cupkee_stream_t), &stream_desc))) {
        return -1;
    }

    return 0;
}

static int test_clean(void)
{
    return TU_pre_deinit();
}

static void test_stream_init(void)
{
    int id;
    cupkee_stream_t *s;

    CU_ASSERT(0 <= (id = cupkee_id(tag)));

    CU_ASSERT(NULL != (s = (cupkee_stream_t *) cupkee_entry(id, tag)));

    CU_ASSERT(0 == cupkee_stream_init(s, id, 32, 32, mock_read, mock_write));

    CU_ASSERT(0 == cupkee_stream_deinit(s));
}

static void test_stream_read(void)
{
    int id;
    cupkee_stream_t *s;
    uint8_t buf[32];

    CU_ASSERT(0 <= (id = cupkee_id(tag)));
    CU_ASSERT(NULL != (s = (cupkee_stream_t *) cupkee_entry(id, tag)));
    CU_ASSERT(0 == cupkee_stream_init(s, id, 32, 32, mock_read, mock_write));

    mock_read_immediately = 5;
    CU_ASSERT(32 == cupkee_stream_read(s, 32, buf));
    CU_ASSERT(buf[0] == 5 && buf[31] == 5);

    mock_read_immediately = 0;
    CU_ASSERT(0 == cupkee_stream_read(s, 32, buf));
    CU_ASSERT(1 == mock_read_trigger);

    memset(buf, 6, 32);
    CU_ASSERT(32 == cupkee_stream_push(s, 32, buf));

    CU_ASSERT(16 == cupkee_stream_read(s, 16, buf));
    CU_ASSERT(buf[0] == 6 && buf[15] == 6);

    memset(buf, 7, 32);
    CU_ASSERT(16 == cupkee_stream_push(s, 32, buf));

    CU_ASSERT(32 == cupkee_stream_read(s, 32, buf));
    CU_ASSERT(buf[0] == 6 && buf[15] == 6 && buf[16] == 7 && buf[31] == 7);

    CU_ASSERT(0 == TU_object_event_dispatch());
    CU_ASSERT(0 == cupkee_stream_deinit(s));
}

static void test_stream_write(void)
{
    int id;
    cupkee_stream_t *s;
    uint8_t buf[32];

    CU_ASSERT(0 <= (id = cupkee_id(tag)));
    CU_ASSERT(NULL != (s = (cupkee_stream_t *) cupkee_entry(id, tag)));
    CU_ASSERT(0 == cupkee_stream_init(s, id, 32, 32, mock_read, mock_write));

    mock_write_trigger = 0;
    mock_write_immediately = 1;
    CU_ASSERT(32 == cupkee_stream_write(s, 32, buf));
    CU_ASSERT(32 == cupkee_stream_write(s, 32, buf));

    memset(buf, 5, 32);
    mock_write_immediately = 0;
    CU_ASSERT(32 == cupkee_stream_write(s, 32, buf));
    CU_ASSERT(0  == cupkee_stream_write(s, 32, buf));
    CU_ASSERT(1  == mock_write_trigger);

    CU_ASSERT(0 == TU_object_event_dispatch());
    CU_ASSERT(0 == cupkee_stream_deinit(s));
}

static void test_stream_sync(void)
{
    int id;
    cupkee_stream_t *s;
    uint8_t buf[64];

    CU_ASSERT(0 <= (id = cupkee_id(tag)));
    CU_ASSERT(NULL != (s = (cupkee_stream_t *) cupkee_entry(id, tag)));
    CU_ASSERT(0 == cupkee_stream_init(s, id, 32, 32, mock_read, mock_write));

    memset(buf, 5, 64);
    CU_ASSERT(64 == cupkee_stream_write_sync(s, 64, buf));
    CU_ASSERT(64 == cupkee_stream_read_sync(s, 64, buf));

    CU_ASSERT(0 == TU_object_event_dispatch());
    CU_ASSERT(0 == cupkee_stream_deinit(s));
}

static void test_stream_event(void)
{
    int id;
    cupkee_stream_t *s;
    uint8_t buf[32];

    CU_ASSERT(0 <= (id = cupkee_id(tag)));
    CU_ASSERT(NULL != (s = (cupkee_stream_t *) cupkee_entry(id, tag)));
    CU_ASSERT(0 == cupkee_stream_init(s, id, 32, 32, mock_read, mock_write));

    cupkee_stream_listen(s, CUPKEE_EVENT_DATA);
    cupkee_stream_listen(s, CUPKEE_EVENT_DRAIN);

    CU_ASSERT(16 == cupkee_stream_push(s, 16, buf))
    CU_ASSERT(0 == TU_object_event_dispatch());
    CU_ASSERT(16 == cupkee_stream_push(s, 32, buf))
    CU_ASSERT(1 == TU_object_event_dispatch());
    CU_ASSERT(mock_curr_id == id && mock_curr_event == CUPKEE_EVENT_DATA);

    CU_ASSERT(32 == cupkee_stream_write(s, 32, buf))
    CU_ASSERT(31 == cupkee_stream_pull(s, 31, buf))
    CU_ASSERT(0 == TU_object_event_dispatch());

    CU_ASSERT(1 == cupkee_stream_pull(s, 32, buf))
    CU_ASSERT(1 == TU_object_event_dispatch());
    CU_ASSERT(mock_curr_id == id && mock_curr_event == CUPKEE_EVENT_DRAIN);

    CU_ASSERT(0 == cupkee_stream_deinit(s));
}

CU_pSuite test_sys_stream(void)
{
    CU_pSuite suite = CU_add_suite("system stream", test_setup, test_clean);

    if (suite) {
        CU_add_test(suite, "stream init      ", test_stream_init);
        CU_add_test(suite, "stream read      ", test_stream_read);
        CU_add_test(suite, "stream write     ", test_stream_write);
        CU_add_test(suite, "stream sync io   ", test_stream_sync);
        CU_add_test(suite, "stream event     ", test_stream_event);
    }

    return suite;
}

