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

int TU_pre_init(void)
{
    hw_mock_init(1024 * 32); // 32K Ram

    cupkee_init();

    cupkee_start();

    return 0;
}

int TU_pre_deinit(void)
{
    hw_mock_deinit();
    return 0;
}

int TU_object_event_dispatch(void)
{
    cupkee_event_t e;
    if (cupkee_event_take(&e) && e.type == EVENT_OBJECT) {
        cupkee_object_event_dispatch(e.which, e.code);
        return 1;
    } else {
        return 0;
    }
}

int TU_pin_event_dispatch(void)
{
    cupkee_event_t e;
    if (cupkee_event_take(&e) && e.type == EVENT_PIN) {
        cupkee_pin_event_dispatch(e.which, e.code);
        return 1;
    } else {
        return 0;
    }
}

