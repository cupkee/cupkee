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

#ifndef __CUPKEE_DEF_INC__
#define __CUPKEE_DEF_INC__

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define CUPKEE_VER_SIZE                 4

#define CUPKEE_UID_SIZE                 24

#define CUPKEE_INFO_SIZE                (CUPKEE_VER_SIZE + CUPKEE_UID_SIZE * 2)

#define CUPKEE_BLOCK_SIZE               128
#define CUPKEE_SECTOR_SIZE              (CUPKEE_BLOCK_SIZE * 64)

#define CUPKEE_MEMBER_OFFSET(T, m)      (intptr_t)(&(((T*)0)->m))
#define CUPKEE_CONTAINER_OF(p, T, m)    ((T*)((intptr_t)(p) - CUPKEE_MEMBER_OFFSET(T, m)))

#define CUPKEE_SIZE_ALIGN(v, a)         (((size_t)(v) + ((a) - 1)) & ~((a) - 1))
#define CUPKEE_ADDR_ALIGN(p, a)         (void *)(((intptr_t)(p) + ((a) - 1)) & ~(intptr_t)((a) - 1))

#define CUPKEE_TRUE                     1
#define CUPKEE_FALSE                    0

typedef int (*cupkee_callback_t)(void *entry, int event, intptr_t param);

void cupkee_sysinfo_get(uint8_t *info_buf);


#endif /* __CUPKEE_DEF_INC__ */

