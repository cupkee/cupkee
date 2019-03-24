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

#include "cupkee_sysdisk.h"

static const uint8_t *cupkee_board_id = NULL;
static void (*cupkee_user_dispatch)(uint16_t witch, uint8_t code);
static void (*cupkee_user_sync)(uint32_t ticks);

void cupkee_event_poll(void)
{
    cupkee_event_t e;

    while (cupkee_event_take(&e)) {
        if (e.type == EVENT_SYSTICK) {
            cupkee_device_sync(_cupkee_systicks);
            cupkee_timeout_sync(_cupkee_systicks);
            if (cupkee_user_sync) {
                cupkee_user_sync(_cupkee_systicks);
            }
        } else
        if (e.type == EVENT_AUXTICK) {
            cupkee_pin_schedule(_cupkee_auxticks);
        } else
        if (e.type == EVENT_OBJECT) {
            cupkee_object_event_dispatch(e.which, e.code);
        } else
        if (e.type == EVENT_PIN) {
            cupkee_pin_event_dispatch(e.which, e.code);
        } else
        if (e.type == EVENT_USER && cupkee_user_dispatch) {
            cupkee_user_dispatch(e.which, e.code);
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

    // BOARD ID
    if (cupkee_board_id) {
        memcpy(info_buf + CUPKEE_VER_SIZE + CUPKEE_UID_SIZE, cupkee_board_id, CUPKEE_UID_SIZE);
    } else {
        memset(info_buf + CUPKEE_VER_SIZE + CUPKEE_UID_SIZE, 0, CUPKEE_UID_SIZE);
    }
}

void cupkee_loader_init(void)
{
    hw_info_t info;

    /* Hardware startup */
    hw_setup_loader(&info);

    cupkee_storage_init(info.rom_sz / CUPKEE_SECTOR_SIZE);
}

void cupkee_init(const uint8_t *id)
{
    hw_info_t info;

    /* Hardware startup */
    hw_setup(&info);

    cupkee_storage_init(info.rom_sz / CUPKEE_SECTOR_SIZE);

    /* System setup */
    cupkee_memory_setup();

    cupkee_event_setup();

    cupkee_object_setup();

    cupkee_timeout_setup();

    cupkee_timer_setup();

    cupkee_pin_setup();

    cupkee_device_setup();

    cupkee_sysdisk_init();

    /* Board device setup */
    hw_device_setup();

    cupkee_board_id = id;
    cupkee_user_sync = NULL;
    cupkee_user_dispatch = NULL;
}

void cupkee_loop(void)
{
    // Reset systick at first
    _cupkee_systicks = 0;
    _cupkee_auxticks = 0;

    while (1) {
        cupkee_device_poll();

        cupkee_event_poll();
    }
}

void cupkee_set_user_dispatch(void (*dispatch)(uint16_t witch, uint8_t code))
{
    cupkee_user_dispatch = dispatch;
}

void cupkee_set_user_sync(void (*sync)(uint32_t))
{
    cupkee_user_sync = sync;
}

