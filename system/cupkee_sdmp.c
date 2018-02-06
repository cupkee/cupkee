/*
MIT License

This file is part of cupkee project.

Copyright (c) 2017, 2018 Lixing Ding <ding.lixing@gmail.com>

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

enum sdmp_demux_state_e {
    DEMUX_KEY = 0,
    DEMUX_MSG_HEAD = 8,
    DEMUX_MSG_BODY,
};

#define SDMP_SEND_BUF_SIZE      512

static void *   sdmp_io_stream = NULL;
static uint8_t  sdmp_demux_state = 0;
static uint8_t  sdmp_demux_msg_ver;
static uint8_t  sdmp_demux_msg_len;
static uint16_t sdmp_demux_msg_pos;
static uint8_t  sdmp_demux_msg_buf[256];
static void (*sdmp_demux_text_handler)(int, const void *) = NULL;
static void *   sdmp_mux_text_buf = NULL;

static void sdmp_msg_handler(uint8_t ver, uint16_t end, uint8_t *body)
{
    console_log("Get Msg[%u] ver:%u, s:%u\r\n", body[0], ver, end);
}

static int sdmp_msg_head_verify(uint8_t *head)
{
    return (head[0] + head[1] + head[2] + head[3]) == 0;
}

static int sdmp_msg_filter(uint8_t byte)
{
    if (sdmp_demux_state == DEMUX_KEY) {
        if (byte != 0xFE) {
            return 1;
        } else {
            sdmp_demux_state = DEMUX_MSG_HEAD;
            sdmp_demux_msg_pos = 0;
        }
    }

    if (sdmp_demux_state == DEMUX_MSG_HEAD) {
        sdmp_demux_msg_buf[sdmp_demux_msg_pos++] = byte;

        if (sdmp_demux_msg_len >= 4) {
            if (sdmp_msg_head_verify(sdmp_demux_msg_buf)) {
                sdmp_demux_msg_ver = sdmp_demux_msg_buf[1];
                sdmp_demux_msg_len = sdmp_demux_msg_buf[2];
                sdmp_demux_msg_pos = 0;
                sdmp_demux_state = DEMUX_MSG_BODY;
            } else {
                sdmp_demux_state = DEMUX_KEY;
            }
        }
    } else
    if (sdmp_demux_state == DEMUX_MSG_BODY) {
        sdmp_demux_msg_buf[sdmp_demux_msg_pos++] = byte;
        if (sdmp_demux_msg_pos > sdmp_demux_msg_len) {
            sdmp_demux_state = DEMUX_KEY;
            sdmp_msg_handler(sdmp_demux_msg_ver,
                             sdmp_demux_msg_len,
                             sdmp_demux_msg_buf);
        }
    } else {
        sdmp_demux_state = DEMUX_KEY;
        return 1;
    }

    return 0;
}

static void sdmp_do_recv(void *tty)
{
    uint8_t byte;
    char buf[4];
    int  pos = 0;

    while (0 < cupkee_read(tty, 1, &byte)) {
        if (sdmp_msg_filter(byte)) {
            buf[pos++] = byte;
            if (pos >= 3 && sdmp_demux_text_handler) {
                sdmp_demux_text_handler(pos, buf);
                pos = 0;
            }
        }
    }

    if (pos) {
        sdmp_demux_text_handler(pos, buf);
    }
}

static void sdmp_do_send(void *tty)
{
    uint8_t c;

    while (cupkee_buffer_shift(sdmp_mux_text_buf, &c)) {
        if (!cupkee_write(tty, 1, &c)) {
            cupkee_buffer_unshift(sdmp_mux_text_buf, c);
            break;
        }
    }
}

static int sdmp_stream_handle(void *tty, int event, intptr_t param)
{
    (void) param;

    if (event == CUPKEE_EVENT_DATA) {
        sdmp_do_recv(tty);
    } else
    if (event == CUPKEE_EVENT_DRAIN) {
        sdmp_do_send(tty);
    }

    return 0;
}

int cupkee_sdmp_init(void *stream)
{
    (void) stream;

    sdmp_mux_text_buf = cupkee_buffer_alloc(SDMP_SEND_BUF_SIZE);

    if (!sdmp_mux_text_buf) {
        return -CUPKEE_ERESOURCE;
    }

    sdmp_demux_state = DEMUX_KEY;
    sdmp_io_stream = stream;

    if (0 != cupkee_device_handle_set(stream, sdmp_stream_handle, 0)) {
        return -CUPKEE_EINVAL;
    }

    cupkee_listen(stream, CUPKEE_EVENT_DATA);
    cupkee_listen(stream, CUPKEE_EVENT_DRAIN);

    return 0;
}

int cupkee_sdmp_send_text(size_t len, const char *text)
{

    if (sdmp_io_stream) {
        int cached = cupkee_buffer_give(sdmp_mux_text_buf, len, text);

        if (cached > 0 && (size_t) cached == cupkee_buffer_length(sdmp_mux_text_buf)) {
            sdmp_do_send(sdmp_io_stream);
        }
        return cached;
    } else {
        return -CUPKEE_ERROR;
    }
}

int cupkee_sdmp_send_text_sync(size_t len, const char *s)
{
    if (sdmp_io_stream) {
        size_t pos = 0;

        while (pos < len) {
            char ch = s[pos++];

            if (ch == '\n' && (pos < 2 || s[pos - 2] != '\r')) {
                char cr = '\r';
                cupkee_write_sync(sdmp_io_stream, 1, &cr);
            }

            cupkee_write_sync(sdmp_io_stream, 1, &ch);
        }

        return pos;
    } else {
        return -CUPKEE_ERROR;
    }
}

int cupkee_sdmp_set_demux_text_handler(void (*handler)(int, const void *))
{
    sdmp_demux_text_handler = handler;
    return 0;
}

