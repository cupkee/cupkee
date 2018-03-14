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

#ifndef __CUPKEE_STREAM_INC__
#define __CUPKEE_STREAM_INC__

enum {
    CUPKEE_STREAM_FL_READABLE = 0x01,
    CUPKEE_STREAM_FL_WRITABLE = 0x02,
    CUPKEE_STREAM_FL_IBLOCKED = 0x04,
    CUPKEE_STREAM_FL_OBLOCKED = 0x08,

    CUPKEE_STREAM_FL_NOTIFY_ERROR = 0x10,
    CUPKEE_STREAM_FL_NOTIFY_DATA  = 0x20,
    CUPKEE_STREAM_FL_NOTIFY_DRAIN = 0x40
};

enum {
    CUPKEE_STREAM_STATE_IDLE,
    CUPKEE_STREAM_STATE_PAUSED,
    CUPKEE_STREAM_STATE_FLOWING
};

typedef struct cupkee_stream_t cupkee_stream_t;
struct cupkee_stream_t {
    uint16_t id;
    uint8_t flags;
    uint8_t rx_state;

    uint16_t rx_buf_size;
    uint16_t tx_buf_size;

    uint32_t last_push;

    void *rx_buf;
    void *tx_buf;

    int (*_read) (cupkee_stream_t *s, size_t n, void *);
    int (*_write)(cupkee_stream_t *s, size_t n, const void *);
};

int cupkee_stream_rx_cache_space(cupkee_stream_t *s);
int cupkee_stream_tx_cache_space(cupkee_stream_t *s);
int cupkee_stream_readable(cupkee_stream_t *s);
int cupkee_stream_writable(cupkee_stream_t *s);

int cupkee_stream_init(
   cupkee_stream_t *stream, int id,
   size_t rx_buf_size, size_t tx_buf_size,
   int (*_read)(cupkee_stream_t *s, size_t n, void *),
   int (*_write)(cupkee_stream_t *s, size_t n, const void *)
);
int cupkee_stream_deinit(cupkee_stream_t *s);

void cupkee_stream_listen(cupkee_stream_t *s, int event);
void cupkee_stream_ignore(cupkee_stream_t *s, int event);

void cupkee_stream_resume(cupkee_stream_t *s);
void cupkee_stream_pause(cupkee_stream_t *s);
void cupkee_stream_shutdown(cupkee_stream_t *s, uint8_t flags);

void cupkee_stream_sync(cupkee_stream_t *s, uint32_t systicks);
int cupkee_stream_push(cupkee_stream_t *s, size_t n, const void *data);
int cupkee_stream_pull(cupkee_stream_t *s, size_t n, void *data);

int cupkee_stream_read(cupkee_stream_t *s, size_t n, void *buf);
int cupkee_stream_write(cupkee_stream_t *s, size_t n, const void *data);

int cupkee_stream_read_sync(cupkee_stream_t *s, size_t n, void *buf);
int cupkee_stream_write_sync(cupkee_stream_t *s, size_t n, const void *data);

int cupkee_stream_unshift(cupkee_stream_t *s, uint8_t data);

void cupkee_stream_set_error(cupkee_stream_t *s, uint8_t err);


int cupkee_stream_push_buf(cupkee_stream_t *s, void *data);
void *cupkee_stream_pull_buf(cupkee_stream_t *s);

void *cupkee_stream_read_buf(cupkee_stream_t *s);
int cupkee_stream_write_buf(cupkee_stream_t *s, void *data);

#endif /* __CUPKEE_STREAM_INC__ */

