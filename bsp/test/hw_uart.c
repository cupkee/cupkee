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

#include "hardware.h"

#define HW_FL_USED      1

typedef struct hw_uart_t {
    uint8_t flags;
    uint8_t dev_id;
    uint8_t reserved[2];
    void   *rx_buff;
    void   *tx_buff;
    uint32_t last_tick;
    const hw_config_uart_t *config;
} hw_uart_t;

static hw_uart_t uart_controls[HW_INSTANCES_UART];

/****************************************************************************
 * Debug start                                                             */
static int dbg_setup_status[HW_INSTANCES_UART];
static int dbg_data_pos[HW_INSTANCES_UART];
static int dbg_data_len[HW_INSTANCES_UART];
static const char *dbg_data_ptr[HW_INSTANCES_UART];
static int dbg_data_send[HW_INSTANCES_UART];
static int dbg_send_enable[HW_INSTANCES_UART];

void hw_dbg_uart_setup_status_set(int instance, int status)
{
    dbg_setup_status[instance] = status;
}

void hw_dbg_uart_send_state(int instance, int status)
{
    dbg_send_enable[instance] = status;
}

void hw_dbg_uart_data_give(int instance, const char *data)
{
    dbg_data_pos[instance] = 0;
    dbg_data_len[instance] = strlen(data);
    dbg_data_ptr[instance] = data;
}

int  hw_dbg_uart_data_take(int instance, int n)
{
    int send = dbg_data_send[instance];

    if (send > n) {
        dbg_data_send[instance] -= n;
    } else {
        n = send;
        dbg_data_send[instance] = 0;
    }
    return n;
}

/* Debug end                                                               *
 ***************************************************************************/
static inline hw_uart_t *uart_get(int instance) {
    return &uart_controls[instance];
}

static inline int uart_has_data(int instance) {
    return dbg_data_len[instance] > dbg_data_pos[instance];
}

static inline int uart_not_busy(int instance) {
    return dbg_send_enable[instance];
}

static inline uint8_t uart_data_get(int instance) {
    return dbg_data_ptr[instance][dbg_data_pos[instance]++];
}

static inline void uart_data_put(int instance, uint8_t data) {
    (void) data;
    dbg_data_send[instance]++;
    return;
}

static void uart_release(int instance)
{
    hw_uart_t *control = uart_get(instance);

    cupkee_buf_release(control->rx_buff);
    cupkee_buf_release(control->tx_buff);

    /* Do hardware release here */

    control->flags = 0;
}

static void uart_reset(int instance)
{
    hw_uart_t *control = uart_get(instance);

    /* Do hardware reset here */

    control->dev_id = DEVICE_ID_INVALID;
    control->config = NULL;
}

static int uart_setup(int instance, uint8_t dev_id, const hw_config_t *config)
{
    hw_uart_t *control = uart_get(instance);
    int err;

    /* Do hardware setup here */
    err = -dbg_setup_status[instance];

    if (!err) {
        control->dev_id = dev_id;
        control->config = (const hw_config_uart_t *)config;
    }
    return err;
}

static void uart_poll(int instance)
{
    hw_uart_t *control = uart_get(instance);

    if (uart_has_data(instance)) {
        do {
            if (cupkee_buf_push(control->rx_buff, uart_data_get(instance)) == 0) {
                device_error_post(control->dev_id, CUPKEE_EOVERFLOW);
                break;
            }
            control->last_tick = system_ticks_count;
        } while (uart_has_data(instance));

        if (cupkee_buf_is_full(control->rx_buff)) {
            device_data_post(control->dev_id);
        }
    } else {
        if (!cupkee_buf_is_empty(control->rx_buff)) {
            if (system_ticks_count - control->last_tick > 10) {
                device_data_post(control->dev_id);
            }
        }
    }

    if (uart_not_busy(instance) && !cupkee_buf_is_empty(control->tx_buff)) {
        do {
            uint8_t d;
            if (cupkee_buf_shift(control->tx_buff, &d)) {
                uart_data_put(instance, d);
            } else {
                device_drain_post(control->dev_id);
                break;
            }
        } while(uart_not_busy(instance));
    }
}

static int uart_recv(int instance, int size, void *buf)
{
    hw_uart_t *control = uart_get(instance);

    return cupkee_buf_take(control->rx_buff, size, buf);
}

static int uart_send(int instance, int len, void *data)
{
    hw_uart_t *control = uart_get(instance);

    return cupkee_buf_give(control->tx_buff, len, data);
}

static int uart_received(int instance)
{
    hw_uart_t *control = uart_get(instance);

    return cupkee_buf_length(control->rx_buff);
}

static const hw_driver_t uart_driver = {
    .release = uart_release,
    .reset   = uart_reset,
    .setup   = uart_setup,
    .poll    = uart_poll,
    .io.stream = {
        .recv = uart_recv,
        .send = uart_send,
        .received = uart_received,
    }
};

const hw_driver_t *hw_request_uart(int instance)
{
    void *rx_buff;
    void *tx_buff;

    if (instance >= HW_INSTANCES_UART || uart_controls[instance].flags) {
        return NULL;
    }

    rx_buff = cupkee_buf_alloc();
    if (!rx_buff) {
        return NULL;
    }

    tx_buff = cupkee_buf_alloc();
    if (!tx_buff) {
        cupkee_buf_release(rx_buff);
        return NULL;
    }

    uart_controls[instance].flags  = HW_FL_USED;
    uart_controls[instance].dev_id = DEVICE_ID_INVALID;
    uart_controls[instance].rx_buff= rx_buff;
    uart_controls[instance].tx_buff= tx_buff;
    uart_controls[instance].config = NULL;

    return &uart_driver;
}

void hw_setup_uart(void)
{
    int i;

    for (i = 0; i < HW_INSTANCES_UART; i++) {
        uart_controls[i].flags = 0;

        // dbg init
        dbg_data_len[i] = 0;
        dbg_data_pos[i] = 0;
        dbg_data_send[i] = 0;
        dbg_send_enable[i] = 0;
    }
}

