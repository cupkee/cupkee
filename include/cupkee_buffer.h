/*
MIT License

This file is part of cupkee project.

Copyright (c) 2016,2017 Lixing Ding <ding.lixing@gmail.com>

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

