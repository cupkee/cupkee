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

#include "hardware.h"

#define USART_TOUT_THRESHOLD    20

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
static const uint32_t device_base[] = {
    USART1, USART2, USART3, UART4, UART5
};
static const uint32_t device_rcc[] = {
    RCC_USART1, RCC_USART2, RCC_USART3, RCC_UART4, RCC_UART5
};

static int uart_gpio_setup(int instance)
{
    uint32_t bank_rx, bank_tx;
    uint16_t gpio_rx, gpio_tx;
    int port_rx, port_tx;

    switch(instance) {
    case 0:
        gpio_rx = GPIO_USART1_RX; gpio_tx = GPIO_USART1_TX;
        bank_rx = bank_tx = GPIOA;
        port_rx = port_tx = 0;
        break;
    case 1:
        gpio_rx = GPIO_USART2_RX; gpio_tx = GPIO_USART2_TX;
        bank_rx = bank_tx = GPIOA;
        port_rx = port_tx = 0;
        break;
    case 2:
        gpio_rx = GPIO_USART3_RX; gpio_tx = GPIO_USART3_TX;
        bank_rx = bank_tx = GPIOB;
        port_rx = port_tx = 1;
        break;
    case 3:
        gpio_rx = GPIO_UART4_RX; gpio_tx = GPIO_UART4_TX;
        bank_rx = bank_tx = GPIOC;
        port_rx = port_tx = 2;
        break;
    case 4:
        gpio_rx = GPIO_UART5_RX; gpio_tx = GPIO_UART5_TX;
        bank_rx = GPIO_BANK_UART5_RX; bank_tx = GPIO_BANK_UART5_TX;
        port_rx = 3; port_tx = 2;
        break;
    default: return -1;
    }

    if (port_rx == port_tx) {
        if (!hw_gpio_use(port_rx, gpio_rx | gpio_tx)) {
            return -1;
        }
    } else {
        if (!hw_gpio_use(port_rx, gpio_rx)) {
            return -1;
        }
        if (!hw_gpio_use(port_tx, gpio_tx)) {
            hw_gpio_release(port_rx, gpio_rx);
            return -1;
        }
    }

    gpio_set_mode(bank_tx, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, gpio_tx);
    gpio_set_mode(bank_rx, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, gpio_rx);

    return CUPKEE_OK;
}

static inline hw_uart_t *uart_get(int instance) {
    return &uart_controls[instance];
}

static inline int uart_has_data(int instance) {
    return USART_SR(device_base[instance]) & USART_SR_RXNE;
}

static inline int uart_not_busy(int instance) {
    return USART_SR(device_base[instance]) & USART_SR_TXE;
}

static inline uint8_t uart_data_get(int instance) {
    return USART_DR(device_base[instance]);
}

static inline void uart_data_put(int instance, uint8_t data) {
    USART_DR(device_base[instance]) = data;
}

static void uart_reset(int instance)
{
    hw_uart_t *control = uart_get(instance);

    /* Do hardware reset here */

    control->dev_id = DEVICE_ID_INVALID;
    control->config = NULL;
}

static void uart_release(int instance)
{
    hw_uart_t *control = uart_get(instance);

    uart_reset(instance);

    cupkee_buffer_release(control->rx_buff);
    cupkee_buffer_release(control->tx_buff);

    control->flags = 0;
}

static int uart_setup(int instance, uint8_t dev_id, const hw_config_t *conf)
{
    hw_uart_t *control = uart_get(instance);
    const hw_config_uart_t *config = (const hw_config_uart_t *) conf;
    uint32_t databits, stopbits, parity;
    int err = CUPKEE_OK;

    switch(config->stop_bits) {
    case DEVICE_OPT_STOPBITS_1: stopbits = USART_STOPBITS_1; break;
    case DEVICE_OPT_STOPBITS_2: stopbits = USART_STOPBITS_2; break;
    case DEVICE_OPT_STOPBITS_0_5: stopbits = USART_STOPBITS_0_5; break;
    case DEVICE_OPT_STOPBITS_1_5: stopbits = USART_STOPBITS_1_5; break;
    default:
        err = CUPKEE_EINVAL; goto DO_END;
    }

    if (config->data_bits != 8) {
        err = CUPKEE_EINVAL; goto DO_END;
    } else {
        databits = 8;
    }

    switch(config->stop_bits) {
    case DEVICE_OPT_PARITY_NONE: parity = USART_PARITY_NONE; break;
    case DEVICE_OPT_PARITY_ODD:  parity = USART_PARITY_ODD; break;
    case DEVICE_OPT_PARITY_EVEN: parity = USART_PARITY_EVEN; break;
    default:
        err = CUPKEE_EINVAL; goto DO_END;
    }

    if (CUPKEE_OK != uart_gpio_setup(instance)) {
        err = CUPKEE_ERESOURCE; goto DO_END;
    }

    rcc_periph_clock_enable(device_rcc[instance]);
	usart_set_baudrate(device_base[instance], config->baudrate);
	usart_set_databits(device_base[instance], databits);
	usart_set_stopbits(device_base[instance], stopbits);
	usart_set_parity  (device_base[instance], parity);
	usart_set_mode    (device_base[instance], USART_MODE_TX_RX);
	usart_set_flow_control(device_base[instance], USART_FLOWCONTROL_NONE);
    usart_enable(device_base[instance]);

    control->dev_id = dev_id;
    control->config = config;

DO_END:
    return -err;
}

static void uart_poll(int instance)
{
    hw_uart_t *control = uart_get(instance);

    if (uart_has_data(instance)) {
        do {
            if (cupkee_buffer_push(control->rx_buff, uart_data_get(instance)) == 0) {
                //device_error_post(control->dev_id, CUPKEE_EOVERFLOW);
                cupkee_event_post_device_error(control->dev_id);
                break;
            }
            control->last_tick = _cupkee_systicks;
        } while (uart_has_data(instance));

        if (cupkee_buffer_is_full(control->rx_buff)) {
            cupkee_event_post_device_data(control->dev_id);
        }
    } else {
        if (!cupkee_buffer_is_empty(control->rx_buff)) {
            if (_cupkee_systicks - control->last_tick > 10) {
                cupkee_event_post_device_data(control->dev_id);
            }
        }
    }

    if (uart_not_busy(instance) && !cupkee_buffer_is_empty(control->tx_buff)) {
        do {
            uint8_t d;
            if (cupkee_buffer_shift(control->tx_buff, &d)) {
                uart_data_put(instance, d);
            } else {
                cupkee_event_post_device_drain(control->dev_id);
                break;
            }
        } while(uart_not_busy(instance));
    }
}

static int uart_recv(int instance, size_t n, void *buf)
{
    hw_uart_t *control = uart_get(instance);

    return cupkee_buffer_take(control->rx_buff, n, buf);
}

static int uart_send(int instance, size_t n, const void *data)
{
    hw_uart_t *control = uart_get(instance);

    return cupkee_buffer_give(control->tx_buff, n, data);
}

static int uart_send_sync(int instance, size_t n, const void *data)
{
    const uint8_t *ptr = data;
    size_t i = 0;

    while (i < n) {
        while (!uart_not_busy(instance)) {
        }
        uart_data_put(instance, ptr[i++]);
    }

    return i;
}

static int uart_recv_sync(int instance, size_t n, void *data)
{
    uint8_t *ptr = data;
    uint32_t begin = cupkee_systicks();
    size_t i = 0;

    while (i < n) {
        while (!uart_has_data(instance)) {
            if (cupkee_systicks() - begin > USART_TOUT_THRESHOLD) {
                return -1;
            }
        }
        ptr[i++] = uart_data_get(instance);
    }

    return i;
}

static int uart_io_cached(int instance, size_t *in, size_t *out)
{
    hw_uart_t *control = uart_get(instance);

    if (!control) {
        return -CUPKEE_EINVAL;
    }

    if (in) {
        *in = cupkee_buffer_length(control->rx_buff);
    }
    if (out) {
        *out = cupkee_buffer_length(control->tx_buff);
    }
    return 0;
}


static const hw_driver_t uart_driver = {
    .release = uart_release,
    .reset   = uart_reset,
    .setup   = uart_setup,
    .poll    = uart_poll,

    .read    = uart_recv,
    .write   = uart_send,
    .read_sync    = uart_recv_sync,
    .write_sync   = uart_send_sync,
    .io_cached    = uart_io_cached
};

const hw_driver_t *hw_request_uart(int instance)
{
    void *rx_buff;
    void *tx_buff;

    if (instance >= HW_INSTANCES_UART || uart_controls[instance].flags) {
        return NULL;
    }

    rx_buff = cupkee_buffer_alloc(128);
    if (!rx_buff) {
        return NULL;
    }

    tx_buff = cupkee_buffer_alloc(128);
    if (!tx_buff) {
        cupkee_buffer_release(rx_buff);
        return NULL;
    }

    uart_controls[instance].flags  = HW_FL_USED;
    uart_controls[instance].dev_id = DEVICE_ID_INVALID;
    uart_controls[instance].rx_buff= rx_buff;
    uart_controls[instance].tx_buff= tx_buff;
    uart_controls[instance].config = NULL;

    return &uart_driver;
}

void hw_setup_usart(void)
{
    int i;

    for (i = 0; i < HW_INSTANCES_UART; i++) {
        uart_controls[i].flags = 0;
    }
}

