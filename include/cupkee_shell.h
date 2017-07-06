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


#ifndef __CUPKEE_SHELL_INC__
#define __CUPKEE_SHELL_INC__

#include <panda.h>

env_t *cupkee_shell_env(void);
val_t *cupkee_shell_reference_create(val_t *v);
void cupkee_shell_reference_release(val_t *ref);

int cupkee_shell_init(cupkee_device_t *tty, int n, const native_t *natives);
int cupkee_shell_start(const char *initial);

void cupkee_execute_function(val_t *fn, int ac, val_t *av);

void *cupkee_val2data(val_t *data, int *size);
cupkee_device_t *cupkee_val2device(val_t *v);

val_t cupkee_dev2val(env_t *env, cupkee_device_t *dev);

static inline void cupkee_shell_loop(const char *initial) {
    cupkee_shell_start(initial);
    cupkee_loop();
}

#endif /* __CUPKEE_SHELL_INC__ */

