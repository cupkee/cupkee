/*
MIT License

This file is part of cupkee project.

Copyright (c) 2016-2017 Lixing Ding <ding.lixing@gmail.com>

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

#ifndef __CUPKEE_SHELL_MISC_INC__
#define __CUPKEE_SHELL_MISC_INC__

#include <cupkee.h>

#define shell_reference_create  cupkee_shell_reference_create
#define shell_reference_release cupkee_shell_reference_release

void shell_reference_init(env_t *env);
uint8_t shell_reference_id(val_t *ref);
val_t  *shell_reference_ptr(uint8_t id);

void print_simple_value(val_t *v);

void shell_print_value(val_t *v);
void shell_print_error(int error);
void shell_do_callback(env_t *env, val_t *cb, uint8_t ac, val_t *av);
void shell_do_callback_error(env_t *env, val_t *cb, int code);
void shell_do_callback_buffer(env_t *env, val_t *cb, type_buffer_t *buffer);

val_t shell_error(env_t *env, int code);
int shell_val_id(val_t *v, int max, const char * const *names);
int shell_str_id(const char *s, int max, const char * const *names);
int shell_input_data(val_t *data, void **ptr);

// cupkee_shell_timer.c
void cupkee_shell_init_timer(void);

#endif /* __CUPKEE_SHELL_MISC_INC__ */

