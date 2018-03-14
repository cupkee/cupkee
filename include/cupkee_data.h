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

