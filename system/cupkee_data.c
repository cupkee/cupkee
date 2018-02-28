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

#include "cupkee.h"

static int cupkee_data_shift_boolean(cupkee_data_entry_t *entry, cupkee_data_t *av)
{
    uint8_t pos = entry->pos + 1;
    uint8_t end = entry->end;

    if (pos <= end) {
        av->boolean = entry->data[entry->pos + 1];
        entry->pos = pos + 1;

        return CUPKEE_DATA_BOOLEAN;
    }
    return CUPKEE_DATA_NONE;
}

static int cupkee_data_shift_string(cupkee_data_entry_t *entry, cupkee_data_t *av)
{
    uint8_t pos = entry->pos + 1;
    uint8_t end = entry->end;

    for (; pos < end; ++pos) {
        if (entry->data[pos] == 0) {
            av->string = (const char *)entry->data + (entry->pos + 1);
            entry->pos = pos + 1;

            return CUPKEE_DATA_STRING;
        }
    }

    return CUPKEE_DATA_NONE;
}

static int cupkee_data_shift_number(cupkee_data_entry_t *entry, cupkee_data_t *av)
{
    uint8_t pos = entry->pos + 1;
    uint8_t end = entry->end;
    uint8_t *ptr = entry->data + pos;

    if (pos + 8 <= end) {
        union {
            uint64_t u;
            double   d;
        } x;

        x.u = ptr[0] * 0x100000000000000 +
              ptr[1] * 0x1000000000000 +
              ptr[2] * 0x10000000000 +
              ptr[3] * 0x100000000 +
              ptr[4] * 0x1000000 +
              ptr[5] * 0x10000 +
              ptr[6] * 0x100+
              ptr[7];

        av->number = x.d;
        entry->pos = pos + 8;
        return CUPKEE_DATA_NUMBER;
    }

    return CUPKEE_DATA_NONE;
}

int cupkee_data_shift(cupkee_data_entry_t *entry, cupkee_data_t *av)
{
    if (!entry || !av) {
        return CUPKEE_DATA_NONE;
    }

    switch (entry->data[entry->pos]) {
    case CUPKEE_DATA_BOOLEAN: return cupkee_data_shift_boolean(entry, av);
    case CUPKEE_DATA_NUMBER:  return cupkee_data_shift_number(entry, av);
    case CUPKEE_DATA_STRING:  return cupkee_data_shift_string(entry, av);
    default: return CUPKEE_DATA_NONE;
    }
}

