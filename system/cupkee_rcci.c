/*
MIT License

This file is part of cupkee project.

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

#include "cupkee.h"

#if 0 // Not implemented

#define RCCI_RX_STATE_SYNC1  0
#define RCCI_RX_STATE_SYNC2  1
#define RCCI_RX_STATE_SYNC3  2
#define RCCI_RX_STATE_VER    3
#define RCCI_RX_STATE_CODE   4
#define RCCI_RX_STATE_SIZE   5
#define RCCI_RX_STATE_DATA   6

#define RCI_ID_MAX_SIZE  16

#define RCI_MSG_HEAD_SIZE 6
#define RCI_MSG_BODY_MAX_SIZE 58
#define RCI_MSG_MAX_SIZE  (RCI_MSG_HEAD_SIZE + RCI_MSG_BODY_MAX_SIZE)

typedef struct cupkee_rci_msg_t {
    uint8_t size;
    uint8_t code;
    uint8_t body[RCI_MSG_BODY_MAX_SIZE];
} cupkee_rci_msg_t;

typedef struct cupkee_rcci_t {
    uint8_t rci_id[RCI_ID_MAX_SIZE];

    uint8_t rx_pos;
    uint8_t rx_state;
    uint8_t rx_msg_size;
    uint8_t rx_msg_code;

    uint8_t *rx_buf;
    void    *io_dev;
    cupkee_callback_t cb;
    intptr_t          cb_param;
} cupkee_rcci_t;

static cupkee_rcci_t rcci;

static void rcci_msg_process(void)
{
    printf("msg [%u] %u bytes\n", rcci.rx_msg_code, rcci.rx_msg_size);
}

static void rcci_rx_reset(void)
{
    rcci.rx_pos = 0;
    rcci.rx_msg_code = 0;
    rcci.rx_msg_size = 0;
    rcci.rx_state = RCCI_RX_STATE_SYNC1;
}

static void rcci_execute(void)
{
    uint8_t dat;

    while (1 == cupkee_read(rcci.io_dev, 1, &dat)) {
        switch (rcci.rx_state) {
        case RCCI_RX_STATE_SYNC1:
        case RCCI_RX_STATE_SYNC2:
        case RCCI_RX_STATE_SYNC3:
            if (dat == 0xFF)
                rcci.rx_state ++;
            else
                rcci.rx_state = RCCI_RX_STATE_SYNC1;
            break;
        case RCCI_RX_STATE_VER:
            if (dat == 0)
                rcci.rx_state ++;
            else
                rcci.rx_state = RCCI_RX_STATE_SYNC1;
            break;
        case RCCI_RX_STATE_CODE:
            rcci.rx_msg_code = dat;
            rcci.rx_state ++;
            break;
        case RCCI_RX_STATE_SIZE:
            if (dat <= RCI_MSG_BODY_MAX_SIZE) {
                rcci.rx_msg_size = dat;
                rcci.rx_state ++;
                rcci.rx_pos = 0;
            } else {
                rcci.rx_state = RCCI_RX_STATE_SYNC1;
            }
            break;
        case RCCI_RX_STATE_DATA:
            rcci.rx_buf[rcci.rx_pos++] = dat;
            break;
        default:
            rcci_rx_reset();
        }

        if (rcci.rx_state == RCCI_RX_STATE_DATA) {
            if (rcci.rx_pos == rcci.rx_msg_size) {
                rcci_msg_process();
                rcci_rx_reset();
            }
        }
    }
}

static void rcci_io_handle(void *entry, uint8_t event, intptr_t param)
{
    if (event == CUPKEE_EVENT_DATA) {
        rcci_execute();
    }
}

int cupkee_rcci_setup(void *stream, const char *rcid, cupkee_callback_t cb, intptr_t param)
{
    void *buf;

    if (!cupkee_is_device(stream)) {
        return -CUPKEE_EINVAL;
    }

    buf = cupkee_malloc(RCI_MSG_MAX_SIZE);
    if (!buf) {
        return -CUPKEE_ENOMEM;
    }

    memset(&rcci, 0, sizeof(rcci));

    strncpy(rcci.rci_id, rcid, RCI_ID_MAX_SIZE);
    rcci.rx_buf = buf;
    rcci.io_dev = stream;
    rcci.cb = cb;
    rcci.cb_param = param;

    cupkee_device_handle_set(stream, rcci_io_handle, 0);

    return 0;
}

int cupkee_rcci_service_id(void *entry);              // call in cb
int cupkee_rcci_service_pull_int(void *entry, int *); // call in cb, with event service_set
int cupkee_rcci_service_push_int(void *entry, int);   // call in cb, with event service_get
int cupkee_rcci_service_update(int id);

int cupkee_rcci_stream_size(void *stream);
void *cupkee_rcci_create_istream(int id, cupkee_callback_t cb, intptr_t param);
void *cupkee_rcci_create_ostream(int id, cupkee_callback_t cb, intptr_t param);
#endif

int cupkee_rcci_setup(void *stream, const char *rcid, cupkee_callback_t cb, intptr_t param)
{
    (void) stream;
    (void) rcid;
    (void) cb;
    (void) param;
    return 0;
}


