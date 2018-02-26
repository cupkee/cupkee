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

#include "cupkee.h"

#include "cupkee_sysdisk.h"

static const uint8_t *cupkee_board_id = NULL;

void cupkee_event_poll(void)
{
    cupkee_event_t e;

    while (cupkee_event_take(&e)) {
        if (e.type == EVENT_SYSTICK) {
            cupkee_device_sync(_cupkee_systicks);
            cupkee_timeout_sync(_cupkee_systicks);
        } else
        if (e.type == EVENT_OBJECT) {
            cupkee_object_event_dispatch(e.which, e.code);
        } else
        if (e.type == EVENT_PIN) {
            cupkee_pin_event_dispatch(e.which, e.code);
        }
    }
}

void cupkee_sysinfo_get(uint8_t *info_buf)
{
    // cupkee version info
    info_buf[0] = CUPKEE_MAJOR;
    info_buf[1] = CUPKEE_MINOR;
    info_buf[2] = (CUPKEE_REVISION >> 8);
    info_buf[3] = (CUPKEE_REVISION & 0xFF);

    // Device ID
    info_buf[4] = 0x80;
    info_buf[5] = 0x00;
    info_buf[6] = 0x00;
    info_buf[7] = 0x00;
    hw_cuid_get(info_buf + 8);

    // OS ID
    if (cupkee_board_id) {
        memcpy(info_buf + CUPKEE_VER_SIZE + CUPKEE_UID_SIZE, cupkee_board_id, CUPKEE_UID_SIZE);
    } else {
        memset(info_buf + CUPKEE_VER_SIZE + CUPKEE_UID_SIZE, 0, CUPKEE_UID_SIZE);
    }
}

void cupkee_init(const uint8_t *id)
{
    hw_info_t info;

    /* Hardware startup */
    hw_setup(&info);

    cupkee_storage_init(info.rom_sz / CUPKEE_SECTOR_SIZE);

    /* System setup */
    cupkee_memory_setup();

    cupkee_object_setup();

    cupkee_timeout_setup();

    cupkee_timer_setup();

    cupkee_event_setup();

    cupkee_pin_setup();

    cupkee_device_setup();

    cupkee_sysdisk_init();

    cupkee_module_init();

    /* Board device setup */
    hw_device_setup();

    cupkee_board_id = id;
}

void cupkee_loop(void)
{
    // Reset systick at first
    _cupkee_systicks = 0;

    while (1) {
        cupkee_device_poll();

        cupkee_event_poll();
    }
}

