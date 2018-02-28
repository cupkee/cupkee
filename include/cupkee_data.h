/*
MIT License

This file is part of cupkee project.

Copyright (c) 2018 Lixing Ding <ding.lixing@gmail.com>

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

#ifndef __CUPKEE_DATA_INC__
#define __CUPKEE_DATA_INC__

enum cupkee_data_type_e {
    CUPKEE_DATA_NONE = 0,
    CUPKEE_DATA_BOOLEAN,
    CUPKEE_DATA_NUMBER,
    CUPKEE_DATA_STRING
};

typedef struct cupkee_data_entry_t  {
    uint8_t end;
    uint8_t pos;
    uint8_t *data;
} cupkee_data_entry_t;

typedef union cupkee_data_t {
    int    boolean;
    double number;
    const char  *string;
} cupkee_data_t;

static inline void cupkee_data_init(cupkee_data_entry_t *entry, uint8_t size, uint8_t *data) {
    entry->end = size;
    entry->pos = 0;
    entry->data = data;
};

int cupkee_data_shift(cupkee_data_entry_t *entry, cupkee_data_t *av);

#endif /* __CUPKEE_DATA_INC__ */

