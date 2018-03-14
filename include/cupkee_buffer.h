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

#ifndef __CUPKEE_BUFFER_INC__
#define __CUPKEE_BUFFER_INC__

void cupkee_buffer_init(void);

void *cupkee_buffer_alloc(size_t size);
void *cupkee_buffer_create(size_t n, const void *data);
void cupkee_buffer_release(void *b);
void cupkee_buffer_reset(void *b);

size_t cupkee_buffer_capacity(void *b);
size_t cupkee_buffer_space(void *b);
size_t cupkee_buffer_length(void *b);

int cupkee_buffer_extend(void *b, int n);

int cupkee_buffer_is_empty(void *b);
int cupkee_buffer_is_full(void *b);

int cupkee_buffer_set(void *b, int offset, uint8_t d);
int cupkee_buffer_get(void *b, int offset, uint8_t *d);
int cupkee_buffer_push(void *b, uint8_t d);
int cupkee_buffer_pop(void *b, uint8_t *d);
int cupkee_buffer_unshift(void *b, uint8_t d);
int cupkee_buffer_shift(void *b, uint8_t *d);

int cupkee_buffer_take(void *b, size_t n, void *buf);
int cupkee_buffer_give(void *b, size_t n, const void *buf);

void *cupkee_buffer_slice(void *b, int start, int n);
void *cupkee_buffer_copy(void *b);
void *cupkee_buffer_sort(void *b);
void *cupkee_buffer_reverse(void *b);

//void   *cupkee_buffer_to_string(void *b);
void *cupkee_buffer_ptr(void *buf);

int cupkee_buffer_read_int8  (void *b, int offset, int8_t *i);
int cupkee_buffer_read_uint8 (void *b, int offset, uint8_t *u);

int cupkee_buffer_read_int16_le  (void *b, int offset, int16_t *i);
int cupkee_buffer_read_int16_be  (void *b, int offset, int16_t *i);

int cupkee_buffer_read_uint16_le (void *b, int offset, uint16_t *u);
int cupkee_buffer_read_uint16_be (void *b, int offset, uint16_t *u);

int cupkee_buffer_read_int32_le  (void *b, int offset, int32_t *i);
int cupkee_buffer_read_int32_be  (void *b, int offset, int32_t *i);

int cupkee_buffer_read_uint32_le (void *b, int offset, uint32_t *u);
int cupkee_buffer_read_uint32_be (void *b, int offset, uint32_t *u);

int cupkee_buffer_read_float_be(void *b, int offset, float *f);
int cupkee_buffer_read_float_le(void *b, int offset, float *f);
int cupkee_buffer_read_double_be(void *b, int offset, double *d);
int cupkee_buffer_read_double_le(void *b, int offset, double *d);

#endif /* __CUPKEE_BUFFER_INC__ */

