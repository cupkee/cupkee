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

#include <cupkee.h>

static inline void stream_event_emit(cupkee_stream_t *s, uint8_t code)
{
    if (s->emitter) {
        cupkee_event_emitter_emit(s->emitter, s->event_offset + code);
    }
}

static inline int stream_is_readable(cupkee_stream_t *s) {
    return s && (s->flags & CUPKEE_STREAM_FL_READABLE);
}

static inline int stream_is_writable(cupkee_stream_t *s) {
    return s && (s->flags & CUPKEE_STREAM_FL_WRITABLE);
}

static inline void stream_rx_request(cupkee_stream_t *s, size_t n) {
    s->_read(s, n);
}

static inline void stream_tx_request(cupkee_stream_t *s) {
    s->_write(s);
}

static inline void *stream_tx_cache(cupkee_stream_t *s)
{
    if (s->tx_buf) {
        return s->tx_buf;
    } else {
        return (s->tx_buf = cupkee_buffer_alloc(s->tx_size_max));
    }
}

static void *stream_rx_cache(cupkee_stream_t *s)
{
    if (s->consumer) {
        cupkee_stream_t *c = s->consumer;
        if (c->tx_buf) {
            return c->tx_buf;
        } else {
            return (c->tx_buf = cupkee_buffer_alloc(c->tx_size_max));
        }
    } else {
        if (s->rx_buf) {
            return s->rx_buf;
        } else {
            return (s->rx_buf = cupkee_buffer_alloc(s->rx_size_max));
        }
    }

}

static void stream_init(cupkee_stream_t *s, cupkee_event_emitter_t *emitter, uint8_t flags)
{
    memset(s, 0, sizeof(cupkee_stream_t));

    s->flags = flags;
    s->emitter = emitter;
    s->rx_state = CUPKEE_STREAM_STATE_IDLE;
}

static inline void stream_data(cupkee_stream_t *s)
{
    if (s->consumer) {
        stream_tx_request(s->consumer);
    } else {
        stream_event_emit(s, CUPKEE_EVENT_STREAM_DATA);
    }
}

static inline void stream_drain(cupkee_stream_t *s)
{
    s->flags &= ~CUPKEE_STREAM_FL_OBLOCKED;
    if (s->producer) {
        s->producer->flags &= ~CUPKEE_STREAM_FL_IBLOCKED;
        stream_rx_request(s->producer, s->tx_size_max);
    } else {
        stream_event_emit(s, CUPKEE_EVENT_STREAM_DRAIN);
    }
}

static void stream_finish(cupkee_stream_t *s)
{
    if (s->tx_buf) {
        if (cupkee_buffer_is_empty(s->tx_buf)) {
            cupkee_buffer_release(s->tx_buf);
            s->tx_buf = NULL;
        } else {
            return;
        }
    }
    stream_event_emit(s, CUPKEE_EVENT_STREAM_FINISH);
}

static void stream_end(cupkee_stream_t *s)
{
    if (s->rx_buf) {
        if (cupkee_buffer_is_empty(s->rx_buf)) {
            cupkee_buffer_release(s->rx_buf);
            s->rx_buf = NULL;
        } else {
            return;
        }
    }
    stream_event_emit(s, CUPKEE_EVENT_STREAM_END);
}

int cupkee_stream_init_readable(
   cupkee_stream_t *s,
   cupkee_event_emitter_t *emitter,
   size_t buf_max_size,
   void (*_read)(cupkee_stream_t *s, size_t n)
) {
    if (!s || !_read) {
        return -CUPKEE_EINVAL;
    }

    stream_init(s, emitter, CUPKEE_STREAM_FL_READABLE);

    s->rx_size_max = buf_max_size;
    s->_read = _read;

    return CUPKEE_OK;
}

int cupkee_stream_init_writable(
   cupkee_stream_t *s,
   cupkee_event_emitter_t *emitter,
   size_t buf_max_size,
   void (*_write)(cupkee_stream_t *s)
) {
    if (!s || !_write) {
        return -CUPKEE_EINVAL;
    }

    stream_init(s, emitter, CUPKEE_STREAM_FL_WRITABLE);

    s->tx_size_max = buf_max_size;
    s->_write = _write;

    return CUPKEE_OK;
}

int cupkee_stream_init_duplex(
   cupkee_stream_t *s,
   cupkee_event_emitter_t *emitter,
   size_t rx_buf_max_size,
   size_t tx_buf_max_size,
   void (*_read)(cupkee_stream_t *s, size_t n),
   void (*_write)(cupkee_stream_t *s)
) {
    if (!s || !_read || !_write) {
        return -CUPKEE_EINVAL;
    }

    stream_init(s, emitter, CUPKEE_STREAM_FL_WRITABLE | CUPKEE_STREAM_FL_READABLE);

    s->rx_size_max = rx_buf_max_size;
    s->tx_size_max = tx_buf_max_size;
    s->_read = _read;
    s->_write = _write;

    return CUPKEE_OK;
}

int cupkee_stream_deinit(cupkee_stream_t *s)
{
    if (s) {
        cupkee_stream_unpipe(s);
        if (s->rx_buf) {
            cupkee_buffer_release(s->rx_buf);
        }
        if (s->tx_buf) {
            cupkee_buffer_release(s->tx_buf);
        }
    }
    return 0;
}

int cupkee_stream_readable(cupkee_stream_t *s)
{
    if (stream_is_readable(s) && s->rx_buf) {
        return cupkee_buffer_length(s->rx_buf);
    }

    return 0;
}

int cupkee_stream_writable(cupkee_stream_t *s)
{
    if (stream_is_writable(s) && !(s->flags & CUPKEE_STREAM_FL_TX_SHUTDOWN)) {
        if (s->tx_buf) {
            return cupkee_buffer_space(s->tx_buf);
        } else {
            return s->tx_size_max;
        }
    }

    return 0;
}

void cupkee_stream_shutdown(cupkee_stream_t *s, uint8_t flags)
{
    if (stream_is_readable(s) && flags & CUPKEE_STREAM_FL_READABLE) {
        s->flags |= CUPKEE_STREAM_FL_RX_SHUTDOWN;
        stream_end(s);
    }
    if (stream_is_writable(s) && flags & CUPKEE_STREAM_FL_WRITABLE) {
        s->flags |= CUPKEE_STREAM_FL_TX_SHUTDOWN;
        stream_finish(s);
    }
}

int cupkee_stream_rx_cache_space(cupkee_stream_t *s)
{
    if (stream_is_readable(s)) {
        if (s->rx_buf) {
            return cupkee_buffer_space(s->rx_buf);
        } else {
            return s->rx_size_max;
        }
    }
    return -CUPKEE_EINVAL;
}

int cupkee_stream_tx_cache_space(cupkee_stream_t *s)
{
    if (stream_is_writable(s)) {
        if (s->tx_buf) {
            return cupkee_buffer_space(s->tx_buf);
        } else {
            return s->rx_size_max;
        }
    }
    return -CUPKEE_EINVAL;
}

void cupkee_stream_set_error(cupkee_stream_t *s, uint8_t code)
{
    if (s) {
        s->error_code = code;
        if (code) {
            stream_event_emit(s, CUPKEE_EVENT_STREAM_ERROR);
        }
    }
}

int cupkee_stream_get_error(cupkee_stream_t *s)
{
    if (s) {
        return -s->error_code;
    } else {
        return -CUPKEE_EINVAL;
    }
}

int cupkee_stream_push(cupkee_stream_t *s, size_t n, const void *data)
{
    int cnt = 0;
    void *cache;

    if (!stream_is_readable(s) || !n || !data) {
        return 0;
    }

    if (s->flags & (CUPKEE_STREAM_FL_RX_SHUTDOWN | CUPKEE_STREAM_FL_IBLOCKED)) {
        return 0;
    }

    if (!(cache = stream_rx_cache(s))) {
        return -CUPKEE_ENOMEM;
    }

    if (s->rx_state == CUPKEE_STREAM_STATE_FLOWING && cupkee_buffer_is_empty(cache)) {
        stream_data(s);
    }

    cnt = cupkee_buffer_give(cache, n, data);
    if (cupkee_buffer_is_full(cache)) {
        s->flags |= CUPKEE_STREAM_FL_IBLOCKED;
        if (s->consumer) {
            s->consumer->flags |= CUPKEE_STREAM_FL_OBLOCKED;
        }
    }

    return cnt;
}

int cupkee_stream_pull(cupkee_stream_t *s, size_t n, void *data)
{
    if (stream_is_writable(s) && s->tx_buf && n && data) {
        int cnt = cupkee_buffer_take(s->tx_buf, n, data);

        if (s->flags & CUPKEE_STREAM_FL_TX_SHUTDOWN) {
            stream_finish(s);
        } else
        if (cnt > 0 && (s->flags & CUPKEE_STREAM_FL_OBLOCKED)) {
            stream_drain(s);
        }
        return cnt;
    }
    return 0;
}

int cupkee_stream_unshift(cupkee_stream_t *s, uint8_t data)
{
    if (!stream_is_readable(s)) {
        return -CUPKEE_EINVAL;
    }

    if (s->rx_buf) {
        return cupkee_buffer_unshift(s->rx_buf, data);
    } else {
        return 0;
    }
}

void cupkee_stream_pause(cupkee_stream_t *s)
{
    if (stream_is_readable(s)) {
        s->rx_state = CUPKEE_STREAM_STATE_PAUSED;
    }
}

void cupkee_stream_resume(cupkee_stream_t *s)
{
    if (stream_is_readable(s) && s->rx_state != CUPKEE_STREAM_STATE_FLOWING) {
        if (s->rx_buf) {
            stream_rx_request(s, cupkee_buffer_space(s->rx_buf));
        } else {
            stream_rx_request(s, s->rx_size_max);
        }
        s->rx_state = CUPKEE_STREAM_STATE_FLOWING;
    }
}

int cupkee_stream_read(cupkee_stream_t *s, size_t n, void *buf)
{
    int cnt;

    if (!stream_is_readable(s) || !buf) {
        return -CUPKEE_EINVAL;
    }

    if (s->rx_state == CUPKEE_STREAM_STATE_IDLE) {
        s->rx_state = CUPKEE_STREAM_STATE_PAUSED;
        stream_rx_request(s, n);
    }

    if (!s->rx_buf) {
        return 0;
    }

    if (cupkee_buffer_is_empty(s->rx_buf)) {
        stream_rx_request(s, n);
        return 0;
    }

    cnt = cupkee_buffer_take(s->rx_buf, n, buf);
    if (s->flags & CUPKEE_STREAM_FL_RX_SHUTDOWN) {
        stream_end(s);
    } else
    if (cnt >0 && s->flags & CUPKEE_STREAM_FL_IBLOCKED) {
        s->flags &= ~CUPKEE_STREAM_FL_IBLOCKED;
        stream_rx_request(s, cupkee_buffer_space(s->rx_buf));
    }
    return cnt;
}

int cupkee_stream_write(cupkee_stream_t *s, size_t n, const void *data)
{
    void *cache;
    int cached;

    if (!stream_is_writable(s) || !data) {
        return -CUPKEE_EINVAL;
    }

    if (s->flags & CUPKEE_STREAM_FL_TX_SHUTDOWN) {
        return 0;
    }

    if (!(cache = stream_tx_cache(s))) {
        return -CUPKEE_ENOMEM;
    }

    cached = cupkee_buffer_give(cache, n, data);
    if (cached != (int) n) {
        s->flags |= CUPKEE_STREAM_FL_OBLOCKED;
    }

    if (cached == (int) cupkee_buffer_length(cache)) {
        stream_tx_request(s);
    }

    return cached;
}

int cupkee_stream_pipe(cupkee_stream_t *s, cupkee_stream_t *consumer)
{
    if (!stream_is_readable(s) || !stream_is_writable(consumer)) {
        return -CUPKEE_EINVAL;
    }

    if (s->consumer || consumer->producer) {
        return -CUPKEE_EINVAL;
    }

    consumer->producer = s;
    s->consumer = consumer;
    s->rx_state = CUPKEE_STREAM_STATE_FLOWING;
    stream_event_emit(consumer, CUPKEE_EVENT_STREAM_PIPE);
    if (consumer->tx_buf) {
        stream_rx_request(s, cupkee_buffer_space(consumer->tx_buf));
    } else {
        stream_rx_request(s, consumer->tx_size_max);
    }

    return 0;
}

int cupkee_stream_unpipe(cupkee_stream_t *s)
{
    if (!stream_is_readable(s)) {
        return -CUPKEE_EINVAL;
    }

    if (s->consumer) {
        stream_event_emit(s->consumer, CUPKEE_EVENT_STREAM_UNPIPE);
        s->consumer->producer = NULL;

        if (!s->rx_buf || !cupkee_buffer_is_full(s->rx_buf)) {
            s->flags &= ~CUPKEE_STREAM_FL_IBLOCKED;
        }
        s->consumer = NULL;
    }
    s->rx_state = CUPKEE_STREAM_STATE_PAUSED;

    return 0;
}

