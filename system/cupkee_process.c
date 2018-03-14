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

#include <cupkee.h>

typedef struct cupkee_process_t {
    uint8_t step;
    uint8_t flags;

    intptr_t data;

    void (*task) (void *entry);
    void (*finish) (int state, intptr_t data);
} cupkee_process_t;

int cupkee_process_start(void (*fn)(void *entry), intptr_t data, void (*finish)(int state, intptr_t data))
{
    cupkee_process_t *_entry;

    if (!fn) {
        return -CUPKEE_EINVAL;
    }

    _entry = cupkee_malloc(sizeof(cupkee_process_t));
    if (!_entry) {
        return -CUPKEE_ENOMEM;
    }

    _entry->step = 0;
    _entry->flags = 0;

    _entry->data = data;
    _entry->task = fn;
    _entry->finish = finish;

    fn(_entry);

    return CUPKEE_OK;
}

intptr_t cupkee_process_data(void *entry)
{
    cupkee_process_t *_entry = (cupkee_process_t *)entry;

    return _entry ? _entry->data : 0;
}

int cupkee_process_step(void *entry)
{
    cupkee_process_t *_entry = (cupkee_process_t *)entry;

    return _entry ? _entry->step : -1;
}

void cupkee_process_goto(void *entry, int step)
{
    cupkee_process_t *_entry = (cupkee_process_t *)entry;

    if (!_entry) {
        return;
    }

    if (step > 255 || step < 0) {
        cupkee_process_fail(_entry, -CUPKEE_EINVAL);
    } else {
        _entry->step = step;
        _entry->task(entry);
    }
}

void cupkee_process_next(void *entry)
{
    cupkee_process_t *_entry = (cupkee_process_t *)entry;

    if (!_entry) {
        return;
    }

    _entry->step++;
    _entry->task(entry);
}

void cupkee_process_done(void *entry)
{
    cupkee_process_t *_entry = (cupkee_process_t *)entry;

    if (!_entry) {
        return;
    }

    if (_entry->finish) {
        _entry->finish(CUPKEE_OK, _entry->data);
    }

    cupkee_free(entry);
}

void cupkee_process_fail(void *entry, int err)
{
    cupkee_process_t *_entry = (cupkee_process_t *)entry;

    if (!_entry) {
        return;
    }

    if (_entry->finish) {
        _entry->finish(err, _entry->data);
    }

    cupkee_free(entry);
}

