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

#ifndef __CUPKEE_SDMP_INC__
#define __CUPKEE_SDMP_INC__

int cupkee_sdmp_init(void *stream);
int cupkee_sdmp_tty_write(size_t len, const char *text);
int cupkee_sdmp_tty_write_sync(size_t len, const char *text);
int cupkee_sdmp_set_tty_handler(void (*handler)(int, const void *));

int cupkee_sdmp_update_state_trigger(int id);
int cupkee_sdmp_update_state_boolean(int id, int v);
int cupkee_sdmp_update_state_number(int id, double v);
int cupkee_sdmp_update_state_string(int id, const char *s);

#endif /* __CUPKEE_SDMP_INC__ */

