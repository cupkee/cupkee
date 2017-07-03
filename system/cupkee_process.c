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

