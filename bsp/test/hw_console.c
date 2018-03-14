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


/*******************************************************************************
 * hw field
*******************************************************************************/
#include "hardware.h"

static uint8_t cdc_state = 0;
static uint8_t cdc_devid = 0;
static uint8_t cdc_ready = 0;

/*******************************************************************************
 * dbg field
*******************************************************************************/
#define CONSOLE_BUF_SIZE    (1 * 1024)

static char console_buf[CONSOLE_BUF_SIZE];
static int  console_end = 0;
static int  console_pos = 0;
static void (*console_input_cb)(void *, int);
static void (*console_drain_cb)(void);

void hw_dbg_console_clr_buf(void)
{
    console_end = 0;
    console_pos = 0;
}

void hw_dbg_console_reset(void)
{
    memset(console_buf, 0, CONSOLE_BUF_SIZE);

    console_end = 0;
    console_pos = 0;

    cdc_state = 0;
    cdc_devid = 0;
}

int hw_dbg_console_get_reply(char **ptr)
{
    if (ptr) {
        *ptr = console_buf;
    }
    console_buf[console_end] = 0;

    return console_end;
}

void hw_dbg_console_set_input(const char *data)
{
    if (cdc_state == 2) {
        int len = strlen(data);

        memcpy(console_buf, data, len);
        console_end = len;
        console_pos = 0;

        cupkee_event_post_device_data(cdc_devid);
    } else {
        printf("Hi, console input callback not register!!!\n");
    }
}

/*******************************************************************************
 * bsp interface
*******************************************************************************/
static void cdc_release(int instance)
{
    if (instance == 0) {
        cdc_state = 0;
    }
}

static void cdc_reset(int instance)
{
    if (instance == 0) {
        cdc_state = 1;
    }
}

static int cdc_setup(int instance, uint8_t dev_id, const hw_config_t *conf)
{
    (void) conf;
    if (instance == 0) {
        cdc_state = 2;
        cdc_devid = dev_id;
    }
    return 0;
}

static int cdc_send_byte(uint8_t c)
{
    if (cdc_ready && console_end < CONSOLE_BUF_SIZE) {
        console_buf[console_end++] = c;
        return 1;
    } else {
        return 0;
    }
}

static int cdc_recv_byte(uint8_t *c)
{
    if (console_pos < console_end) {
        *c = console_buf[console_pos++];
        return 1;
    } else {
        return 0;
    }
}

static int cdc_send(int instance, int len, void *data)
{
    const uint8_t *p = data;
    int i = 0;

    (void) instance;

    while (i < len && cdc_send_byte(p[i])) {
        i++;
    }

    return i;
}

static int cdc_recv(int instance, int size, void *data)
{
    int len = console_end - console_pos;

    (void) instance;
    if (len > 0) {
        memcpy(data, console_buf + console_pos, len);
    }
    return len;
}

static int cdc_send_sync(int instance, int len, const uint8_t *data)
{
    int i;

    (void) instance;

    for (i = 0; i < len; i++) {
        while (!cdc_send_byte(data[i]))
            ;
    }

    return i;
}

static int cdc_recv_sync(int instance, int size, uint8_t *data)
{
    int i;

    (void) instance;

    for (i = 0; i < size; i++) {
        while (!cdc_recv_byte(data + i))
            ;
    }

    return i;
}

static int cdc_received(int instance)
{
    //Todo: real counter
    (void) instance;
    return 1;
}

static const hw_driver_t cdc_driver = {
    .release = cdc_release,
    .reset   = cdc_reset,
    .setup   = cdc_setup,
    .io.stream = {
        .recv = cdc_recv,
        .send = cdc_send,
        .recv_sync = cdc_recv_sync,
        .send_sync = cdc_send_sync,
        .received = cdc_received,
    }
};

const hw_driver_t *hw_request_cdc(int instance)
{
    if (instance != 0 || cdc_state) {
        return NULL;
    }
    cdc_state = 1;

    return &cdc_driver;
}

void hw_setup_usb(void)
{
    cdc_state = 0;
    cdc_devid = 0;
    cdc_ready = 0;
}

