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

