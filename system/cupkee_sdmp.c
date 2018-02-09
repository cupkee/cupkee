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

#define SDMP_VERSION            0
#define SDMP_SYNC_BYTE          0xF9

#define SDMP_HEAD_SIZE          4
#define SDMP_BODY_MAX_SIZE      256
#define SDMP_SEND_BUF_SIZE      248
#define SDMP_REPORT_BUF_SIZE    (SDMP_HEAD_SIZE + SDMP_BODY_MAX_SIZE)

static void *   sdmp_io_stream = NULL;
static uint8_t  sdmp_demux_state = 0;
static uint16_t sdmp_request_len;
static uint16_t sdmp_request_pos;
static uint8_t  sdmp_request_buf[256];

static uint16_t sdmp_report_pos = 0;
static uint16_t sdmp_report_end = 0;
static uint8_t  sdmp_report_buf[SDMP_REPORT_BUF_SIZE];

static void *   sdmp_mux_text_buf = NULL;

static uint16_t sdmp_script_buf_size = 0;
static char *   sdmp_script_buf = NULL;

static void (*sdmp_demux_text_handler)(int, const void *) = NULL;

static int sdmp_request_filter(uint8_t);

static void sdmp_do_recv(void *tty)
{
    uint8_t byte;
    char buf[4];
    int  pos = 0;

    while (0 < cupkee_read(tty, 1, &byte)) {
        if (sdmp_request_filter(byte)) {
            buf[pos++] = byte;
            if (pos >= 3 && sdmp_demux_text_handler) {
                sdmp_demux_text_handler(pos, buf);
                pos = 0;
            }
        }
    }

    if (pos && sdmp_demux_text_handler) {
        sdmp_demux_text_handler(pos, buf);
    }
}

static void sdmp_do_send(void *tty)
{
    uint8_t c;

    // Send report first
    if (sdmp_report_pos < sdmp_report_end) {
        uint8_t *buf = sdmp_report_buf + sdmp_report_pos;
        int len = sdmp_report_end - sdmp_report_pos;
        int retval = cupkee_write(tty, len, buf);

        if (retval <= 0) {
            return;
        }

        sdmp_report_pos += retval;
        if (retval < len) {
            return;
        }
    }

    // Send text
    while (cupkee_buffer_shift(sdmp_mux_text_buf, &c)) {
        if (!cupkee_write(tty, 1, &c)) {
            cupkee_buffer_unshift(sdmp_mux_text_buf, c);
            break;
        }
    }
}

static inline uint8_t *sdmp_response_init(size_t body_size)
{
    size_t total_size = SDMP_HEAD_SIZE + body_size;
    uint8_t *head;

    if (sdmp_report_pos != sdmp_report_end || total_size > SDMP_REPORT_BUF_SIZE) {
        return NULL;
    }

    sdmp_report_pos = total_size;
    sdmp_report_end = total_size;

    head = sdmp_report_buf;
    head[0] = SDMP_SYNC_BYTE;
    head[1] = 0x00;
    head[2] = body_size;
    head[3] = ~(SDMP_SYNC_BYTE+ body_size) + 1;

    return head + SDMP_HEAD_SIZE;
}

static inline void sdmp_report_send(void)
{
    if (sdmp_report_end == sdmp_report_pos) {
        sdmp_report_pos = 0;

        sdmp_do_send(sdmp_io_stream);
    }
}

enum sdmp_response_code_e {
    SDMP_OK = 0,
    SDMP_CONT,
    SDMP_InvalidReq = 10,
    SDMP_InvalidParam,
    SDMP_InvalidContent,
    SDMP_ProcessError,
    SDMP_MemNotEnought,
    SDMP_NotImplemented,
    SDMP_Unreadable,
    SDMP_Unwriteable,
    SDMP_ExecuteError,
};

enum sdmp_message_code_e {
    SDMP_REQ_HELLO = 0x00,
    SDMP_REQ_RESET,
    SDMP_REQ_SYSINFO,
    SDMP_REQ_SYSDATA,
    SDMP_REQ_ERASE_SYSDATA,
    SDMP_REQ_WRITE_SYSDATA,
    SDMP_REQ_EXECUTE_SCRIPT,

    SDMP_REQ_APP_INTERFACE,
    SDMP_REQ_APP_STATE,
    SDMP_REQ_APP_DATA,
    SDMP_REQ_APP_WRITE_DATA,
    SDMP_REQ_APP_FN,

    SDMP_RESPONSE = 0x80,
    SDMP_REPORT = 0x81,
};

static void sdmp_response_status(uint8_t req, uint8_t err)
{
    uint8_t *report = sdmp_response_init(3);
    if (report) {
        report[0] = SDMP_RESPONSE;
        report[1] = req;
        report[2] = err;

        sdmp_report_send();
    }
}

static void sdmp_response_cont(uint8_t req, uint8_t next)
{
    uint8_t *report = sdmp_response_init(4);
    if (report) {
        report[0] = SDMP_RESPONSE;
        report[1] = req;
        report[2] = SDMP_CONT;
        report[3] = next;

        sdmp_report_send();
    }
}


static void sdmp_response_hello(void)
{
    uint8_t *report = sdmp_response_init(4);

    if (report) {
        report[0] = SDMP_RESPONSE;
        report[1] = SDMP_REQ_HELLO;
        report[2] = SDMP_OK;
        report[3] = SDMP_VERSION;

        sdmp_report_send();
    }
}

static void sdmp_report_sysinfo(uint16_t req_len, uint8_t *req)
{
    (void) req_len;

    sdmp_response_status(req[0], SDMP_NotImplemented);
}

static void sdmp_report_sysdata(uint16_t req_len, uint8_t *req)
{
    (void) req_len;

    sdmp_response_status(req[0], SDMP_NotImplemented);
}

static inline void sdmp_script_buf_free(void)
{
    if (sdmp_script_buf) {
        cupkee_free(sdmp_script_buf);
        sdmp_script_buf = NULL;
    }

    sdmp_script_buf_size = 0;
}

static void sdmp_execute_script(uint16_t req_len, uint8_t *req)
{
    uint8_t *script;
    uint8_t cur, next, end, len;
    int offset, error;

    if (req_len <= 4) {
        error = SDMP_InvalidParam;
        goto DO_ERROR;
    }

    cur = req[2];
    end = req[3];
    len = req_len - 4;
    if (cur >= end || (cur + 1 != end && len != 252)) {
        error = SDMP_InvalidParam;
        goto DO_ERROR;
    }

    if (cur == 0) {
        int total = end * 252;
        sdmp_script_buf_free();

        sdmp_script_buf = cupkee_malloc(total);
        if (!sdmp_script_buf) {
            error = SDMP_MemNotEnought;
            goto DO_ERROR;
        }
        sdmp_script_buf_size = total;

        memset(sdmp_script_buf, 0, total);
    }

    script = req + 4;
    offset = cur * 252;

    if (!sdmp_script_buf || offset + len > sdmp_script_buf_size) {
        sdmp_script_buf_free();

        error = SDMP_ProcessError;
        goto DO_ERROR;
    } else {
        memcpy(sdmp_script_buf + offset, script, len);
    }

    next = cur + 1;
    if (next == end) {
        if (0 > cupkee_execute_string(sdmp_script_buf, NULL)) {
            sdmp_response_status(req[0], SDMP_ExecuteError);
        } else {
            sdmp_response_status(req[0], SDMP_OK);
        }
    } else {
        sdmp_response_cont(req[0], next);
    }

    return;

DO_ERROR:
    sdmp_response_status(req[0], error);
}

static void sdmp_erase_sysdata(uint16_t req_len, uint8_t *req)
{
    (void) req_len;

    sdmp_response_status(req[0], SDMP_NotImplemented);
}

static void sdmp_write_sysdata(uint16_t req_len, uint8_t *req)
{
    (void) req_len;

    sdmp_response_status(req[0], SDMP_NotImplemented);
}

static void sdmp_report_app_interface(uint16_t req_len, uint8_t *req)
{
    (void) req_len;

    sdmp_response_status(req[0], SDMP_NotImplemented);
}

static void sdmp_report_app_state(uint16_t req_len, uint8_t *req)
{
    (void) req_len;

    sdmp_response_status(req[0], SDMP_NotImplemented);
}

static void sdmp_report_appdata(uint16_t req_len, uint8_t *req)
{
    (void) req_len;

    sdmp_response_status(req[0], SDMP_NotImplemented);
}

static void sdmp_execute_app_function(uint16_t req_len, uint8_t *req)
{
    (void) req_len;

    sdmp_response_status(req[0], SDMP_NotImplemented);
}

static void sdmp_write_appdata(uint16_t req_len, uint8_t *req)
{
    (void) req_len;

    sdmp_response_status(req[0], SDMP_NotImplemented);
}

static void sdmp_request_handler(uint16_t len, uint8_t *req)
{
    uint8_t code = req[0];

    // console_log("Get Msg[%u], len:%u\r\n", code, len);

    switch(code) {
    case SDMP_REQ_HELLO:            sdmp_response_hello(); break;
    case SDMP_REQ_RESET:            hw_reset(); break;
    case SDMP_REQ_SYSINFO:          sdmp_report_sysinfo(len, req); break;
    case SDMP_REQ_SYSDATA:          sdmp_report_sysdata(len, req); break;
    case SDMP_REQ_EXECUTE_SCRIPT:   sdmp_execute_script(len, req); break;
    case SDMP_REQ_ERASE_SYSDATA:    sdmp_erase_sysdata(len, req); break;
    case SDMP_REQ_WRITE_SYSDATA:    sdmp_write_sysdata(len, req); break;

    case SDMP_REQ_APP_INTERFACE:    sdmp_report_app_interface(len, req); break;
    case SDMP_REQ_APP_STATE:        sdmp_report_app_state(len, req); break;
    case SDMP_REQ_APP_DATA:         sdmp_report_appdata(len, req); break;
    case SDMP_REQ_APP_WRITE_DATA:   sdmp_write_appdata(len, req); break;
    case SDMP_REQ_APP_FN:           sdmp_execute_app_function(len, req); break;
    default: sdmp_response_status(code, SDMP_InvalidReq);
    }
}

static int sdmp_request_head_verify(uint8_t *head)
{
    if (head[1] != SDMP_VERSION) {
        return 0; // invalid
    }

    return (uint8_t) (head[0] + head[1] + head[2] + head[3]) == 0;
}

static int sdmp_request_filter(uint8_t byte)
{
    if (sdmp_demux_state == DEMUX_KEY) {
        if (byte != SDMP_SYNC_BYTE) {
            return 1;
        } else {
            sdmp_demux_state = DEMUX_MSG_HEAD;
            sdmp_request_pos = 0;
        }
    }

    if (sdmp_demux_state == DEMUX_MSG_HEAD) {
        sdmp_request_buf[sdmp_request_pos++] = byte;

        if (sdmp_request_pos >= 4) {
            if (sdmp_request_head_verify(sdmp_request_buf)) {
                sdmp_request_len = sdmp_request_buf[2];
                sdmp_request_pos = 0;
                sdmp_demux_state = DEMUX_MSG_BODY;
            } else {
                sdmp_demux_state = DEMUX_KEY;
            }
        }
    } else
    if (sdmp_demux_state == DEMUX_MSG_BODY) {
        sdmp_request_buf[sdmp_request_pos++] = byte;
        if (sdmp_request_pos > sdmp_request_len) {
            sdmp_demux_state = DEMUX_KEY;
            sdmp_request_handler(sdmp_request_pos,
                                 sdmp_request_buf);
        }
    } else {
        sdmp_demux_state = DEMUX_KEY;
        return 1;
    }

    return 0;
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
    sdmp_request_len = 0;
    sdmp_request_pos = 0;
    sdmp_demux_state = DEMUX_KEY;

    sdmp_report_pos = 0;
    sdmp_report_end = 0;

    sdmp_script_buf_size = 0;
    sdmp_script_buf = NULL;

    sdmp_io_stream = stream;

    if (0 != cupkee_device_handle_set(stream, sdmp_stream_handle, 0)) {
        return -CUPKEE_EINVAL;
    }

    cupkee_listen(stream, CUPKEE_EVENT_DATA);
    cupkee_listen(stream, CUPKEE_EVENT_DRAIN);

    sdmp_mux_text_buf = cupkee_buffer_alloc(SDMP_SEND_BUF_SIZE);
    if (!sdmp_mux_text_buf) {
        return -CUPKEE_ERESOURCE;
    }

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

