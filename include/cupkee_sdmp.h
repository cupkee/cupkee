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

#ifndef __CUPKEE_RCCI_INC__
#define __CUPKEE_RCCI_INC__

int cupkee_rcci_setup(void *stream, const char *uuid, cupkee_callback_t cb, intptr_t param);

int cupkee_rcci_service_id(void *entry);              // call in cb
int cupkee_rcci_service_pull_int(void *entry, int *); // call in cb, with event service_set
int cupkee_rcci_service_push_int(void *entry, int);   // call in cb, with event service_get
int cupkee_rcci_service_update(int id);

int cupkee_rcci_stream_size(void *stream);
void *cupkee_rcci_create_istream(int id, cupkee_callback_t cb, intptr_t param);
void *cupkee_rcci_create_ostream(int id, cupkee_callback_t cb, intptr_t param);

#endif /* __CUPKEE_RCCI_INC__ */

