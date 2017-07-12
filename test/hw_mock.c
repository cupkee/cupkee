/*
MIT License

This file is part of cupkee project.

Copyright (c) 2016 Lixing Ding <ding.lixing@gmail.com>

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

#include "test.h"

static uint8_t *mock_memory_base = NULL;
static size_t   mock_memory_size = 0;
static size_t   mock_memory_off  = 0;

void hw_mock_init(size_t mem_size)
{
    if (mock_memory_base) {
        free(mock_memory_base);
    }

    mock_memory_base = malloc(mem_size);
    mock_memory_size = mem_size;
    mock_memory_off = 0;
}

void hw_mock_deinit(void)
{
    if (mock_memory_base) {
        free(mock_memory_base);
        mock_memory_base = NULL;
        mock_memory_size = 0;
        mock_memory_off = 0;
    }
}

void hw_enter_critical(uint32_t *state)
{
    (void) state;
}

void hw_exit_critical(uint32_t state)
{
    (void) state;
}

void *hw_malloc(size_t size, size_t align)
{
    size_t off = CUPKEE_SIZE_ALIGN(mock_memory_off, align);

    if (off + size > mock_memory_size) {
        return NULL;
    }

    mock_memory_off = off + size;

    return mock_memory_base + off;
}

