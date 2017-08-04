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

#ifndef __CUPKEE_STREAM_INC__
#define __CUPKEE_STREAM_INC__

enum {
    CUPKEE_STREAM_FL_READABLE = 0x01,
    CUPKEE_STREAM_FL_WRITABLE = 0x02,
    CUPKEE_STREAM_FL_TRANSFORM = 0x04,

    CUPKEE_STREAM_FL_RX_SHUTDOWN = 0x10,
    CUPKEE_STREAM_FL_TX_SHUTDOWN = 0x20,
    CUPKEE_STREAM_FL_IBLOCKED = 0x40,
    CUPKEE_STREAM_FL_OBLOCKED = 0x80
};

enum {
    CUPKEE_STREAM_STATE_IDLE,
    CUPKEE_STREAM_STATE_PAUSED,
    CUPKEE_STREAM_STATE_FLOWING
};

enum {
    CUPKEE_EVENT_STREAM_ERROR,   // rw
    CUPKEE_EVENT_STREAM_DATA,    // r
    CUPKEE_EVENT_STREAM_DRAIN,   //  w
    CUPKEE_EVENT_STREAM_END,     // r
    CUPKEE_EVENT_STREAM_FINISH,  //  w
    CUPKEE_EVENT_STREAM_CLOSE,   // rw
    CUPKEE_EVENT_STREAM_PIPE,    //  w
    CUPKEE_EVENT_STREAM_UNPIPE,  //  w
    CUPKEE_EVENT_STREAM_MAX
};

typedef struct cupkee_stream_t cupkee_stream_t;
struct cupkee_stream_t {
    cupkee_event_emitter_t *emitter;

    uint8_t flags;
    uint8_t rx_state;
    uint8_t event_offset;
    uint8_t error_code;

    uint16_t rx_size_max;
    uint16_t tx_size_max;

    void *rx_buf;
    void *tx_buf;

    void (*_read) (cupkee_stream_t *s, size_t n);
    void (*_write)(cupkee_stream_t *s);

    cupkee_stream_t *consumer;
    cupkee_stream_t *producer;
};

int cupkee_stream_rx_cache_space(cupkee_stream_t *s);
int cupkee_stream_tx_cache_space(cupkee_stream_t *s);
int cupkee_stream_readable(cupkee_stream_t *s);
int cupkee_stream_writable(cupkee_stream_t *s);

int cupkee_stream_init_readable(
   cupkee_stream_t *stream,
   cupkee_event_emitter_t *emitter,
   size_t buf_max_size,
   void (*_read)(cupkee_stream_t *s, size_t n)
);

int cupkee_stream_init_writable(
   cupkee_stream_t *stream,
   cupkee_event_emitter_t *emitter,
   size_t buf_max_size,
   void (*_write)(cupkee_stream_t *s)
);

int cupkee_stream_init_duplex(
   cupkee_stream_t *stream,
   cupkee_event_emitter_t *emitter,
   size_t rx_buf_max_size,
   size_t tx_buf_max_size,
   void (*_read)(cupkee_stream_t *s, size_t n),
   void (*_write)(cupkee_stream_t *s)
);
int cupkee_stream_deinit(cupkee_stream_t *s);

void cupkee_stream_resume(cupkee_stream_t *s);
void cupkee_stream_pause(cupkee_stream_t *s);
void cupkee_stream_shutdown(cupkee_stream_t *s, uint8_t flags);

int cupkee_stream_push(cupkee_stream_t *s, size_t n, const void *data);
int cupkee_stream_pull(cupkee_stream_t *s, size_t n, void *data);

int cupkee_stream_read(cupkee_stream_t *s, size_t n, void *buf);
int cupkee_stream_write(cupkee_stream_t *s, size_t n, const void *data);

int cupkee_stream_unshift(cupkee_stream_t *s, uint8_t data);

void cupkee_stream_set_error(cupkee_stream_t *s, uint8_t err);
int cupkee_stream_get_error(cupkee_stream_t *s);

int cupkee_stream_pipe(cupkee_stream_t *s, cupkee_stream_t *consumer);
int cupkee_stream_unpipe(cupkee_stream_t *s);

int cupkee_stream_push_buf(cupkee_stream_t *s, void *data);
void *cupkee_stream_pull_buf(cupkee_stream_t *s);

void *cupkee_stream_read_buf(cupkee_stream_t *s);
int cupkee_stream_write_buf(cupkee_stream_t *s, void *data);

#endif /* __CUPKEE_STREAM_INC__ */

