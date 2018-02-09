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

static inline int stream_is_readable(cupkee_stream_t *s) {
    return s && (s->flags & CUPKEE_STREAM_FL_READABLE);
}

static inline int stream_is_writable(cupkee_stream_t *s) {
    return s && (s->flags & CUPKEE_STREAM_FL_WRITABLE);
}

static inline int stream_rx_request(cupkee_stream_t *s, size_t n) {
    return s->_read(s, n, NULL);
}

static inline int stream_tx_request(cupkee_stream_t *s) {
    return s->_write(s, 0, NULL);
}

static inline void *stream_tx_cache(cupkee_stream_t *s)
{
    if (s->tx_buf) {
        return s->tx_buf;
    } else {
        return (s->tx_buf = cupkee_buffer_alloc(s->tx_buf_size));
    }
}

static inline void *stream_rx_cache(cupkee_stream_t *s)
{
    if (s->rx_buf) {
        return s->rx_buf;
    } else {
        return (s->rx_buf = cupkee_buffer_alloc(s->rx_buf_size));
    }
}

int cupkee_stream_init(
   cupkee_stream_t *s, int id,
   size_t rx_buf_size, size_t tx_buf_size,
   int (*_read)(cupkee_stream_t *s, size_t n, void *),
   int (*_write)(cupkee_stream_t *s, size_t n, const void *)
)
{
    uint8_t flags = 0;

    if (!s || id < 0) {
        return -CUPKEE_EINVAL;
    }

    memset(s, 0, sizeof(cupkee_stream_t));
    if (rx_buf_size && _read) {
        s->_read = _read;
        s->rx_buf_size = rx_buf_size;
        flags |= CUPKEE_STREAM_FL_READABLE;
    }

    if (tx_buf_size && _write) {
        s->_write = _write;
        s->tx_buf_size = tx_buf_size;
        flags |= CUPKEE_STREAM_FL_WRITABLE;
    }
    s->id = id;
    s->rx_state = CUPKEE_STREAM_STATE_IDLE;

    s->flags = flags;
    return 0;
}

int cupkee_stream_deinit(cupkee_stream_t *s)
{
    if (s) {
        s->id = -1;

        if (s->rx_buf) {
            cupkee_buffer_release(s->rx_buf);
        }
        if (s->tx_buf) {
            cupkee_buffer_release(s->tx_buf);
        }
    }
    return 0;
}

void cupkee_stream_listen(cupkee_stream_t *s, int event)
{
    if (s) {
        switch (event) {
        case CUPKEE_EVENT_ERROR: s->flags |= CUPKEE_STREAM_FL_NOTIFY_ERROR; break;
        case CUPKEE_EVENT_DATA:  s->flags |= CUPKEE_STREAM_FL_NOTIFY_DATA;  stream_rx_request(s, s->rx_buf_size); break;
        case CUPKEE_EVENT_DRAIN: s->flags |= CUPKEE_STREAM_FL_NOTIFY_DRAIN; break;
        default: break;
        }
    }
}

void cupkee_stream_ignore(cupkee_stream_t *s, int event)
{
    if (s) {
        switch (event) {
        case CUPKEE_EVENT_ERROR: s->flags &= ~CUPKEE_STREAM_FL_NOTIFY_ERROR; break;
        case CUPKEE_EVENT_DATA:  s->flags &= ~CUPKEE_STREAM_FL_NOTIFY_DATA;  break;
        case CUPKEE_EVENT_DRAIN: s->flags &= ~CUPKEE_STREAM_FL_NOTIFY_DRAIN; break;
        default: break;
        }
    }
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
    if (stream_is_writable(s)) {
        if (s->tx_buf) {
            return cupkee_buffer_space(s->tx_buf);
        } else {
            return s->tx_buf_size;
        }
    }

    return 0;
}

int cupkee_stream_rx_cache_space(cupkee_stream_t *s)
{
    if (stream_is_readable(s)) {
        if (s->rx_buf) {
            return cupkee_buffer_space(s->rx_buf);
        } else {
            return s->rx_buf_size;
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
            return s->rx_buf_size;
        }
    }
    return -CUPKEE_EINVAL;
}

int cupkee_stream_push(cupkee_stream_t *s, size_t n, const void *data)
{
    int cnt = 0;
    void *cache;

    if (!stream_is_readable(s) || !n || !data) {
        return 0;
    }

    if (!(cache = stream_rx_cache(s))) {
        return -CUPKEE_ENOMEM;
    }

    cnt = cupkee_buffer_give(cache, n, data);
    if (cnt > 0) {
        if (cupkee_buffer_length(cache) > s->rx_buf_size / 2 && s->flags & CUPKEE_STREAM_FL_NOTIFY_DATA) {
            cupkee_object_event_post(s->id, CUPKEE_EVENT_DATA);
        }
        s->last_push = _cupkee_systicks;
    }

    return cnt;
}

void cupkee_stream_sync(cupkee_stream_t *s, uint32_t systicks)
{
    if (s->flags & CUPKEE_STREAM_FL_NOTIFY_DATA
        && s->rx_buf && !cupkee_buffer_is_empty(s->rx_buf)
        && (systicks - s->last_push) > 20) {
        cupkee_object_event_post(s->id, CUPKEE_EVENT_DATA);
    }
}

int cupkee_stream_pull(cupkee_stream_t *s, size_t n, void *data)
{
    if (stream_is_writable(s) && s->tx_buf && n && data) {
        int cnt = cupkee_buffer_take(s->tx_buf, n, data);

        if (cnt > 0 && cupkee_buffer_is_empty(s->tx_buf) && s->flags & CUPKEE_STREAM_FL_NOTIFY_DRAIN) {
            cupkee_object_event_post(s->id, CUPKEE_EVENT_DRAIN);
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
            stream_rx_request(s, s->rx_buf_size);
        }
        s->rx_state = CUPKEE_STREAM_STATE_FLOWING;
    }
}

int cupkee_stream_read(cupkee_stream_t *s, size_t n, void *buf)
{
    size_t max;

    if (!stream_is_readable(s) || !buf) {
        return -CUPKEE_EINVAL;
    }

    if (s->rx_state == CUPKEE_STREAM_STATE_IDLE) {
        s->rx_state = CUPKEE_STREAM_STATE_PAUSED;
    }

    if (!s->rx_buf) {
        stream_rx_request(s, n);
    } else {
        max = cupkee_buffer_length(s->rx_buf);
        if (max < n) {
            stream_rx_request(s, n - max);
        }
    }

    if (s->rx_buf) {
        return cupkee_buffer_take(s->rx_buf, n, buf);
    } else {
        return 0;
    }
}

int cupkee_stream_write(cupkee_stream_t *s, size_t n, const void *data)
{
    void *cache;
    int cached;

    if (!stream_is_writable(s) || !data) {
        return -CUPKEE_EINVAL;
    }

    if (!(cache = stream_tx_cache(s))) {
        return -CUPKEE_ENOMEM;
    }

    cached = cupkee_buffer_give(cache, n, data);
    if (cached == (int) cupkee_buffer_length(cache)) {
        stream_tx_request(s);
    }

    return cached;
}

int cupkee_stream_read_sync(cupkee_stream_t *s, size_t n, void *buf)
{
    if (!stream_is_readable(s) || !buf) {
        return -CUPKEE_EINVAL;
    }

    return s->_read(s, n, buf);
}

int cupkee_stream_write_sync(cupkee_stream_t *s, size_t n, const void *data)
{
    if (!stream_is_writable(s) || !data) {
        return -CUPKEE_EINVAL;
    }

    return s->_write(s, n, data);
}

