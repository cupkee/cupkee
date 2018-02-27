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

#define SDMP_VERSION            0
#define SDMP_SYNC_BYTE          0xF9

#define SDMP_HEAD_SIZE          4
#define SDMP_BODY_MAX_SIZE      256

#define SDMP_SEND_BUF_SIZE      248
#define SDMP_MSG_BUF_SIZE    (SDMP_HEAD_SIZE + SDMP_BODY_MAX_SIZE)

enum sdmp_demux_state_e {
    DEMUX_KEY = 0,
    DEMUX_MSG_HEAD = 8,
    DEMUX_MSG_BODY,
};

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
    SDMP_REQ_EXECUTE_FUNC,
    SDMP_REQ_QUERY_SYSINFO,
    SDMP_REQ_QUERY_SYSDATA,
    SDMP_REQ_ERASE_SYSDATA,
    SDMP_REQ_WRITE_SYSDATA,
    SDMP_REQ_EXECUTE_SCRIPT,

    SDMP_REQ_QUERY_INTERFACE,
    SDMP_REQ_QUERY_APPSTATE,
    SDMP_REQ_QUERY_APPDATA,
    SDMP_REQ_WRITE_APPDATA,

    SDMP_RESPONSE = 0x80,
    SDMP_REPORT   = 0x81,
};

typedef struct sdmp_message_t {
    uint8_t *param;
    uint8_t *data;
} sdmp_message_t;

static void *   sdmp_io_stream = NULL;
static uint8_t  sdmp_demux_state = 0;
static uint16_t sdmp_request_len;
static uint16_t sdmp_request_pos;
static uint8_t  sdmp_request_buf[256];
static uint8_t  sdmp_app_interface[CUPKEE_UID_SIZE];

static uint16_t sdmp_message_pos = 0;
static uint16_t sdmp_message_end = 0;
static uint8_t  sdmp_message_buf[SDMP_MSG_BUF_SIZE];

static void *   sdmp_mux_text_buf = NULL;

static uint16_t sdmp_script_buf_size = 0;
static char *   sdmp_script_buf = NULL;

static void (*sdmp_text_handler)(int, const void *) = NULL;
static int (*sdmp_user_call_handler)(int, void *) = NULL;
static int (*sdmp_user_query_handler)(uint16_t flags) = NULL;

static int sdmp_request_filter(uint8_t);

static void sdmp_do_recv(void *tty)
{
    uint8_t byte;
    char buf[4];
    int  pos = 0;

    while (0 < cupkee_read(tty, 1, &byte)) {
        if (sdmp_request_filter(byte)) {
            buf[pos++] = byte;
            if (pos >= 3 && sdmp_text_handler) {
                sdmp_text_handler(pos, buf);
                pos = 0;
            }
        }
    }

    if (pos && sdmp_text_handler) {
        sdmp_text_handler(pos, buf);
    }
}

static void sdmp_do_send(void *tty)
{
    uint8_t c;

    // Send report first
    if (sdmp_message_pos < sdmp_message_end) {
        uint8_t *buf = sdmp_message_buf + sdmp_message_pos;
        int len = sdmp_message_end - sdmp_message_pos;
        int retval = cupkee_write(tty, len, buf);

        if (retval <= 0) {
            return;
        }

        sdmp_message_pos += retval;
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

static inline int sdmp_message_init(sdmp_message_t *msg, uint8_t code, uint8_t param_size, uint8_t data_size)
{
    size_t body_size = param_size + data_size;
    size_t total_size = SDMP_HEAD_SIZE + 1 + body_size;
    uint8_t *head;

    if (sdmp_message_pos == sdmp_message_end) {
        sdmp_message_pos = 0;
        sdmp_message_end = 0;
    }

    if (sdmp_message_end + total_size > SDMP_MSG_BUF_SIZE) {
        return 0; // false
    }

    head = sdmp_message_buf + sdmp_message_end;
    head[0] = SDMP_SYNC_BYTE;
    head[1] = 0x00;
    head[2] = body_size;
    head[3] = ~(SDMP_SYNC_BYTE+ body_size) + 1; // CheckSum

    head[4] = code;
    msg->param = head + SDMP_HEAD_SIZE + 1;
    msg->data  = msg->param + param_size;

    return total_size; // ok
}

static inline void sdmp_message_send(int len)
{
    sdmp_message_end += len;
    sdmp_do_send(sdmp_io_stream);
}

static void sdmp_response_status(uint8_t req, uint8_t err)
{
    sdmp_message_t msg;
    int len;

    if ((len = sdmp_message_init(&msg, SDMP_RESPONSE, 2, 0)) > 0) {
        msg.param[0] = req;
        msg.param[1] = err;

        sdmp_message_send(len);
    }
}

static inline void sdmp_script_buf_free(void)
{
    if (sdmp_script_buf) {
        cupkee_free(sdmp_script_buf);
        sdmp_script_buf = NULL;
    }

    sdmp_script_buf_size = 0;
}

static void sdmp_response_cont(uint8_t req, uint8_t next)
{
    sdmp_message_t msg;
    int len;

    if ((len = sdmp_message_init(&msg, SDMP_RESPONSE, 3, 0)) > 0) {
        msg.param[0] = req;
        msg.param[1] = SDMP_CONT;
        msg.param[2] = next;

        sdmp_message_send(len);
    }
}

static void sdmp_hello(void)
{
    sdmp_message_t msg;
    int len;

    if ((len = sdmp_message_init(&msg, SDMP_RESPONSE, 3, 0)) > 0) {
        msg.param[0] = SDMP_REQ_HELLO;
        msg.param[1] = SDMP_CONT;
        msg.param[2] = SDMP_VERSION;

        sdmp_message_send(len);
    }
}

static void sdmp_query_sysinfo(void)
{
    sdmp_message_t msg;
    int len;

    if ((len = sdmp_message_init(&msg, SDMP_RESPONSE, 2 + CUPKEE_INFO_SIZE, 0)) > 0) {
        msg.param[0] = SDMP_REQ_QUERY_SYSINFO;
        msg.param[1] = SDMP_OK;

        cupkee_sysinfo_get(msg.param + 2);
        sdmp_message_send(len);
    } else {
        sdmp_response_status(SDMP_REQ_QUERY_SYSINFO, SDMP_MemNotEnought);
    }
}

static void sdmp_query_sysdata(uint16_t req_len, uint8_t *req)
{
    sdmp_message_t msg;
    uint8_t sector;
    uint8_t block;
    int len;

    if (req_len < 3) {
        sdmp_response_status(SDMP_REQ_QUERY_SYSDATA, SDMP_InvalidParam);
        return;
    }

    sector = req[1];
    block  = req[2];

    if ((len = sdmp_message_init(&msg, SDMP_RESPONSE, 6, CUPKEE_BLOCK_SIZE)) > 0) {
        msg.param[0] = SDMP_REQ_QUERY_SYSDATA;
        msg.param[1] = SDMP_OK;
        msg.param[2] = sector;
        msg.param[3] = block;
        msg.param[4] = 0;
        msg.param[5] = 0;

        if (0 != cupkee_storage_block_read(sector, block, msg.data)) {
            msg.param[1] = SDMP_Unreadable;
            len -= CUPKEE_BLOCK_SIZE;
        }

        sdmp_message_send(len);
    }
}

static uint8_t sdmp_do_report_state(uint16_t flags)
{
    if (sdmp_user_query_handler) {
        return sdmp_user_query_handler(flags) ? SDMP_ExecuteError : SDMP_OK;
    } else {
        return SDMP_NotImplemented;
    }
}

static uint8_t sdmp_do_call(uint8_t func_id, cupkee_data_entry_t *entry)
{
    if (func_id == 0x80) {
        cupkee_data_t av;

        if (CUPKEE_DATA_NUMBER == cupkee_data_shift(entry, &av)) {
            hw_reset(av.number);
        } else {
            hw_reset(HW_RESET_NORMAL);
        }
        return 0; // Make gcc happy
    } else
    if (func_id < 16){
        if (sdmp_user_call_handler) {
            return sdmp_user_call_handler(func_id, entry) ? SDMP_ExecuteError : SDMP_OK;
        } else {
            return SDMP_NotImplemented;
        }
    } else {
        return SDMP_InvalidParam;
    }
}

static void sdmp_execute_func(uint16_t req_len, uint8_t *req)
{
    sdmp_message_t msg;
    int len;

    if (req_len >= 2 && (len = sdmp_message_init(&msg, SDMP_RESPONSE, 3, 0)) > 0) {
        cupkee_data_entry_t args;
        uint8_t func_id = req[1];

        cupkee_data_init(&args, req_len - 2, req + 2);

        msg.param[0] = SDMP_REQ_EXECUTE_FUNC;
        msg.param[2] = sdmp_do_call(func_id, &args);
        msg.param[2] = func_id;

        sdmp_message_send(len);
    } else {
        sdmp_response_status(SDMP_REQ_EXECUTE_FUNC, SDMP_InvalidParam);
    }
}

static void sdmp_erase_sysdata(uint16_t req_len, uint8_t *req)
{
    sdmp_message_t msg;
    int len;

    if (req_len >= 3 && (len = sdmp_message_init(&msg, SDMP_RESPONSE, 4, 0)) > 0) {
        uint8_t sect_start = req[1];
        uint16_t sect_num  = req[2] + 1;

        msg.param[0] = SDMP_REQ_ERASE_SYSDATA;
        msg.param[2] = sect_start;
        msg.param[3] = sect_num - 1;

        if (0 != cupkee_storage_sector_erase(sect_start, sect_num)) {
            msg.param[1] = SDMP_Unwriteable;
        } else {
            msg.param[1] = SDMP_OK;
        }

        sdmp_message_send(len);
    } else {
        sdmp_response_status(SDMP_REQ_ERASE_SYSDATA, SDMP_InvalidParam);
    }
}

static void sdmp_write_sysdata(uint16_t req_len, uint8_t *req)
{
    sdmp_message_t msg;
    uint8_t sector;
    uint8_t block;
    uint8_t *data;
    int len;

    if (req_len != 5 + CUPKEE_BLOCK_SIZE) {
        sdmp_response_status(SDMP_REQ_WRITE_SYSDATA, SDMP_InvalidParam);
        return;
    }

    sector = req[1];
    block  = req[2];
    // checkSum == req[3] * 256 + req[4]
    data = req + 5;

    if ((len = sdmp_message_init(&msg, SDMP_RESPONSE, 4, 0)) > 0) {
        msg.param[0] = SDMP_REQ_WRITE_SYSDATA;
        msg.param[1] = SDMP_OK;
        msg.param[2] = sector;
        msg.param[3] = block;

        if (0 > cupkee_storage_block_write(sector, block, data)) {
            msg.param[1] = SDMP_Unwriteable;
        }

        sdmp_message_send(len);
    }
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

static void sdmp_query_interface(void)
{
    sdmp_message_t msg;
    int len;

    if ((len = sdmp_message_init(&msg, SDMP_RESPONSE, 2 + CUPKEE_UID_SIZE, 0)) > 0) {
        char *ptr = (char *) msg.param + 2;

        msg.param[0] = SDMP_REQ_QUERY_INTERFACE;
        msg.param[1] = SDMP_OK;

        memcpy(ptr, sdmp_app_interface, CUPKEE_UID_SIZE);

        sdmp_message_send(len);
    } else {
        sdmp_response_status(SDMP_REQ_QUERY_INTERFACE, SDMP_MemNotEnought);
    }
}

static void sdmp_query_appstate(uint16_t req_len, uint8_t *req)
{
    uint16_t pos = 1;
    uint16_t flags = 0;

    for (; pos < req_len; ++pos) {
        if (req[pos] < 16) {
            flags |= 1 << req[pos];
        } else {
            sdmp_response_status(SDMP_REQ_QUERY_APPSTATE, SDMP_InvalidParam);
            return;
        }
    }

    sdmp_response_status(SDMP_REQ_QUERY_APPSTATE, sdmp_do_report_state(flags));
}

static void sdmp_query_appdata(uint16_t req_len, uint8_t *req)
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
    case SDMP_REQ_HELLO:            sdmp_hello(); break;
    case SDMP_REQ_EXECUTE_FUNC:     sdmp_execute_func(len, req); break;
    case SDMP_REQ_QUERY_SYSINFO:    sdmp_query_sysinfo(); break;
    case SDMP_REQ_QUERY_SYSDATA:    sdmp_query_sysdata(len, req); break;
    case SDMP_REQ_ERASE_SYSDATA:    sdmp_erase_sysdata(len, req); break;
    case SDMP_REQ_WRITE_SYSDATA:    sdmp_write_sysdata(len, req); break;
    case SDMP_REQ_EXECUTE_SCRIPT:   sdmp_execute_script(len, req); break;

    case SDMP_REQ_QUERY_INTERFACE:  sdmp_query_interface(); break;
    case SDMP_REQ_QUERY_APPSTATE:   sdmp_query_appstate(len, req); break;
    case SDMP_REQ_QUERY_APPDATA:    sdmp_query_appdata(len, req); break;
    case SDMP_REQ_WRITE_APPDATA:    sdmp_write_appdata(len, req); break;
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

    sdmp_message_pos = 0;
    sdmp_message_end = 0;

    sdmp_script_buf_size = 0;
    sdmp_script_buf = NULL;

    memset(sdmp_app_interface, 0, CUPKEE_UID_SIZE);

    sdmp_text_handler = NULL;
    sdmp_user_call_handler = NULL;
    sdmp_user_query_handler = NULL;

    if (0 != cupkee_device_handle_set(stream, sdmp_stream_handle, 0)) {
        return -CUPKEE_EINVAL;
    }

    cupkee_listen(stream, CUPKEE_EVENT_DATA);
    cupkee_listen(stream, CUPKEE_EVENT_DRAIN);

    sdmp_mux_text_buf = cupkee_buffer_alloc(SDMP_SEND_BUF_SIZE);
    if (!sdmp_mux_text_buf) {
        return -CUPKEE_ERESOURCE;
    }

    sdmp_io_stream = stream;

    return 0;
}

int cupkee_sdmp_tty_write(size_t len, const char *text)
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

int cupkee_sdmp_tty_write_sync(size_t len, const char *s)
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

int cupkee_sdmp_set_tty_handler(void (*handler)(int, const void *))
{
    sdmp_text_handler = handler;
    return 0;
}

int cupkee_sdmp_set_call_handler(int (*handler)(int x, void *args))
{
    sdmp_user_call_handler = handler;
    return 0;
}

int cupkee_sdmp_set_query_handler(int (*handler)(uint16_t flags))
{
    sdmp_user_query_handler = handler;
    return 0;
}

int cupkee_sdmp_update_state_trigger(int id)
{
    sdmp_message_t msg;
    int len;

    if ((len = sdmp_message_init(&msg, SDMP_REPORT, 2, 0))) {
        msg.param[0] = id;
        msg.param[1] = CUPKEE_DATA_NONE;

        sdmp_message_send(len);
        return 0;
    }

    return CUPKEE_EBUSY;
}

int cupkee_sdmp_update_state_boolean(int id, int v)
{
    sdmp_message_t msg;
    int len;

    if ((len = sdmp_message_init(&msg, SDMP_REPORT, 2, 1))) {
        msg.param[0] = id;
        msg.param[1] = CUPKEE_DATA_BOOLEAN;
        msg.data[0] = v != 0;

        sdmp_message_send(len);
        return 0;
    }

    return CUPKEE_EBUSY;
}

int cupkee_sdmp_update_state_number(int id, double v)
{
    sdmp_message_t msg;
    int len;

    if ((len = sdmp_message_init(&msg, SDMP_REPORT, 2, 8))) {
        union {
            uint64_t u;
            double   d;
        } *x = (void *)&v;

        msg.param[0] = id;
        msg.param[1] = CUPKEE_DATA_NUMBER;

        msg.data[0] = (uint8_t) (x->u >> 56);
        msg.data[1] = (uint8_t) (x->u >> 48);
        msg.data[2] = (uint8_t) (x->u >> 40);
        msg.data[3] = (uint8_t) (x->u >> 32);
        msg.data[4] = (uint8_t) (x->u >> 24);
        msg.data[5] = (uint8_t) (x->u >> 16);
        msg.data[6] = (uint8_t) (x->u >> 8);
        msg.data[7] = (uint8_t) (x->u);

        sdmp_message_send(len);
        return 0;
    }

    return CUPKEE_EBUSY;
}

int cupkee_sdmp_update_state_string(int id, const char *s)
{
    sdmp_message_t msg;
    int data_len = strlen(s);
    int len;

    if ((len = sdmp_message_init(&msg, SDMP_REPORT, 2, data_len))) {
        msg.param[0] = id;
        msg.param[1] = CUPKEE_DATA_STRING;

        memcpy(msg.data, s, data_len);

        sdmp_message_send(len);
        return 0;
    }

    return CUPKEE_ERESOURCE;
}

static int char2hex(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    } else
    if (c >= 'a' && c <= 'f') {
        return c - ('a' - 10);
    } else
    if (c >= 'A' && c <= 'F') {
        return c - ('A' - 10);
    } else {
        return -1;
    }
}

int cupkee_sdmp_set_interface_id(const char *id)
{
    int pos = 0;


    while (*id) {
        int hi = char2hex(*id++);
        int lo = char2hex(*id++);

        if (hi >= 0 && lo >= 0) {
            sdmp_app_interface[pos++] = hi * 16 + lo;
        } else {
            break;
        }

        if (pos == CUPKEE_UID_SIZE) {
            return 0;
        }
    }

    return -CUPKEE_EINVAL;
}

