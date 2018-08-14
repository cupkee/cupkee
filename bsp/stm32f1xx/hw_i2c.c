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

#define HW_FL_I2C_RX        0x10

enum {
    S_IDLE = 0,
    S_WAIT_START,
    S_WAIT_ADDR,
    S_WAIT_RECV1,
    S_WAIT_RECV2,
    S_WAIT_RECVn,
    S_WAIT_SEND,
    S_WAIT_STOP,
};

#define I2C_MAX         2

typedef struct hw_i2c_t {
    uint32_t base;

    uint8_t flags;
    uint8_t state;
    uint8_t pos;
    uint8_t self_addr;
    uint8_t peer_addr;

    uint8_t send_len;
    uint8_t recv_len;
    uint8_t *recv_buf;
    uint8_t *send_buf;
    void *entry;
} hw_i2c_t;

static hw_i2c_t device_data[I2C_MAX];

static const cupkee_struct_desc_t device_conf_desc[] = {
    {
        .name = "speed",
        .type = CUPKEE_STRUCT_UINT32
    },
    {
        .name = "addr",
        .type = CUPKEE_STRUCT_UINT32
    },
};

static inline hw_i2c_t *device_block(int inst)
{
    return inst < I2C_MAX ? &device_data[inst] : NULL;
}

static int device_setup_io(int inst)
{
    uint16_t pins;

    if (inst == 0) {
        pins = GPIO6 | GPIO7;
    } else {
        pins = GPIO10 | GPIO11;
    }

    if (hw_gpio_setup(1, pins, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_OPENDRAIN, 0)) {
        return 0;
    } else {
        return -1;
    }
}

static inline void device_reset_io(int inst)
{
    uint16_t pins;

    if (inst == 0) {
        pins = GPIO6 | GPIO7;
    } else {
        pins = GPIO10 | GPIO11;
    }

    hw_gpio_unuse(1, pins);
}

static inline void device_send_start(hw_i2c_t *i2c) {
    i2c->pos = 0;
    // Start
    I2C_CR1(i2c->base) |= I2C_CR1_START;
    i2c->state = S_WAIT_START;

    // console_log("Tx Start\r\n");
}

static void device_recv_start(hw_i2c_t *i2c)
{
    i2c->flags |= HW_FL_I2C_RX;
    i2c->pos = 0;

    // Start
    I2C_CR1(i2c->base) |= I2C_CR1_START;
    i2c->state = S_WAIT_START;

    // console_log("Rx Start\r\n");
}

static void device_stop(hw_i2c_t *i2c)
{
    i2c->flags = HW_FL_USED; // Clear all software flags

    cupkee_device_response_submit(i2c->entry, i2c->recv_len);

    i2c->state = S_IDLE;

    // console_log("Complete+\r\n");
}

static void device_start(hw_i2c_t *i2c)
{
//    device_setup_dma(i2c);

    // Enable i2c device
    // I2C_CR2(i2c->base) |= I2C_CR2_ITEVTEN | I2C_CR2_ITERREN;

    if (i2c->send_len == 0) {
        device_recv_start(i2c);
    } else {
        device_send_start(i2c);
    }
}

static cupkee_struct_t *device_conf_init(void *curr)
{
    cupkee_struct_t *conf;

    if (curr) {
        conf = curr;
    } else {
        conf = cupkee_struct_alloc(2, device_conf_desc);
    }

    if (conf) {
        cupkee_struct_set_uint(conf, 0, 50000);
        cupkee_struct_set_uint(conf, 1, 0);
    }

    return conf;
}

static inline void device_poll_start(hw_i2c_t *i2c) {
    if (I2C_SR1(i2c->base) & I2C_SR1_SB) {
        // Enter master mode
        // Send addr & Set mode (Rx | Tx) & Clear SB flag
        I2C_DR(i2c->base) = i2c->peer_addr | (i2c->flags & HW_FL_I2C_RX ? 1 : 0);
        i2c->state = S_WAIT_ADDR;
    }
}

static void device_poll_addr(hw_i2c_t *i2c) {
    if (I2C_SR1(i2c->base) & I2C_SR1_ADDR) {
        uint32_t base = i2c->base;
        uint32_t SR2;

        // Note: ACK always as reset
        if (i2c->flags & HW_FL_I2C_RX) {
            if (i2c->recv_len == 1) {
                uint32_t ctx = cm_mask_interrupts(1);
                SR2 = I2C_SR2(base); // Clear ADDR by read SR2
                I2C_CR1(base) |= I2C_CR1_STOP;
                cm_mask_interrupts(ctx);
                i2c->state = S_WAIT_RECV1;
            } else
            if (i2c->recv_len == 2) {
                I2C_CR1(base) |= I2C_CR1_POS;
                SR2 = I2C_SR2(base); // Clear ADDR by read SR2
                i2c->state = S_WAIT_RECV2;
            } else {
                I2C_CR1(i2c->base) |= I2C_CR1_ACK;
                SR2 = I2C_SR2(base); // Clear ADDR by read SR2
                i2c->state = S_WAIT_RECVn;
            }
        } else {
            SR2 = I2C_SR2(base); // Clear ADDR by read SR2
            I2C_DR(base) = i2c->send_buf[i2c->pos++];
            i2c->state = S_WAIT_SEND;
        }

        // Dummy code, make gcc happy
        if (SR2 == 0) {
            I2C_CR1(i2c->base) = 0;
        }
    }
}


static inline void device_poll_recv1(hw_i2c_t *i2c) {
    if (I2C_SR1(i2c->base) & I2C_SR1_RxNE) {
        i2c->recv_buf[i2c->pos++] = I2C_DR(i2c->base);
        i2c->state = S_WAIT_STOP;
    }
}

static inline void device_poll_recv2(hw_i2c_t *i2c) {
    if (I2C_SR1(i2c->base) & I2C_SR1_BTF) {
        uint32_t base = i2c->base;
        uint32_t ctx;

        I2C_CR1(base) &= ~(I2C_CR1_ACK | I2C_CR1_POS);
        ctx = cm_mask_interrupts(1);
        I2C_CR1(base) |= I2C_CR1_STOP;
        i2c->recv_buf[i2c->pos++] = I2C_DR(base);
        cm_mask_interrupts(ctx);

        i2c->recv_buf[i2c->pos++] = I2C_DR(base);
        i2c->state = S_WAIT_STOP;

        //console_log("recv %u\r\n", i2c->pos);
    }
}

static inline void device_poll_recvn(hw_i2c_t *i2c) {
    if (I2C_SR1(i2c->base) & I2C_SR1_BTF) {
        uint32_t base = i2c->base;
        uint8_t lft = i2c->recv_len - i2c->pos;
        if (lft > 3) {
            i2c->recv_buf[i2c->pos++] = I2C_DR(base);
        } else
        if (lft == 3) {
            I2C_CR1(i2c->base) &= ~I2C_CR1_ACK;
            i2c->recv_buf[i2c->pos++] = I2C_DR(base);
        } else {
            uint32_t ctx = cm_mask_interrupts(1);
            I2C_CR1(base) &= ~(I2C_CR1_ACK | I2C_CR1_POS);
            I2C_CR1(i2c->base) |= I2C_CR1_STOP;
            i2c->recv_buf[i2c->pos++] = I2C_DR(base);
            cm_mask_interrupts(ctx);

            i2c->recv_buf[i2c->pos++] = I2C_DR(base);
            i2c->state = S_WAIT_STOP;

            //console_log("recv %u\r\n", i2c->pos);
        }
    }
}

static inline void device_poll_send(hw_i2c_t *i2c) {
    if (I2C_SR1(i2c->base) & I2C_SR1_BTF) {
        if (i2c->pos < i2c->send_len) {
            I2C_DR(i2c->base) = i2c->send_buf[i2c->pos++];
        } else {
            I2C_CR1(i2c->base) |= I2C_CR1_STOP;
            i2c->state = S_WAIT_STOP;
            //console_log("send: %u\r\n", i2c->pos);
        }
    }
}

static inline void device_poll_stop(hw_i2c_t *i2c) {
    if (!(I2C_CR1(i2c->base) & I2C_CR1_STOP)) {
        if (i2c->recv_len && !(i2c->flags & HW_FL_I2C_RX)) {
            device_recv_start(i2c);
        } else {
            device_stop(i2c);
        }
    }
}

static int device_poll(int inst)
{
    hw_i2c_t *i2c = device_block(inst);

    if (i2c && i2c->state) {
        switch (i2c->state) {
        case S_WAIT_START: device_poll_start(i2c); break;
        case S_WAIT_ADDR:  device_poll_addr(i2c); break;
        case S_WAIT_RECV1: device_poll_recv1(i2c); break;
        case S_WAIT_RECV2: device_poll_recv2(i2c); break;
        case S_WAIT_RECVn: device_poll_recvn(i2c); break;
        case S_WAIT_SEND:  device_poll_send(i2c); break;
        case S_WAIT_STOP:  device_poll_stop(i2c); break;
        default: break;
        }
    }

    return 0;
}

static int device_setup(int inst, void *entry)
{
    hw_i2c_t *i2c = device_block(inst);
    cupkee_struct_t *conf;
    unsigned speed, addr;
    uint32_t base;

    if (!i2c) {
        return -CUPKEE_EINVAL;
    }

    conf = cupkee_device_config(entry);
    if (!conf) {
        return -CUPKEE_ERROR;
    }

    if (i2c == NULL || device_setup_io(inst)) {
        return -CUPKEE_ERESOURCE;
    }

    i2c->entry = entry;
    cupkee_struct_get_uint(conf, 0, &speed);
    cupkee_struct_get_uint(conf, 1, &addr);

    if (inst == 0) {
        rcc_periph_clock_enable(RCC_I2C1);
    } else {
        rcc_periph_clock_enable(RCC_I2C2);
    }

    base = i2c->base;
    I2C_CR1(base) = I2C_CR1_SWRST;

    speed = (speed > 400000) ? 400000 : speed;
    speed = (speed < 500) ? 500 : speed;

    I2C_CR1(base) = 0;
    I2C_CR2(base) = 8; // FREQ = 8M
    I2C_OAR1(base) = addr & 0xFE;

    // Set clock
    if (speed <= 100000) { // SM
        uint16_t ccr = 4000000 / speed;

        I2C_CCR(base) = ccr;
        I2C_TRISE(base) = 9;
    } else { // FM
        uint16_t ccr = 8000000 / (speed * 3);

        I2C_CCR(base) = I2C_CCR_FS | ccr;
        I2C_TRISE(base) = 5;
    }
    i2c->state = S_IDLE;
    I2C_CR1(i2c->base) |= I2C_CR1_PE;

    return 0; // CUPKEE_OK;
}

static int device_reset(int inst)
{
    hw_i2c_t *i2c = device_block(inst);

    // Reset hardware
    I2C_CR1(i2c->base) = I2C_CR1_SWRST;

    // Release pins
    device_reset_io(inst);

    // Release dma
    // device_reset_dma(inst);

    I2C_CR1(i2c->base) = 0;
    I2C_CR2(i2c->base) = 0;
    I2C_OAR1(i2c->base) = 0;
    I2C_OAR2(i2c->base) = 0;
    I2C_TRISE(i2c->base) = 0;
    I2C_CCR(i2c->base) = 0;

    if (inst == 0) {
        rcc_periph_clock_disable(RCC_I2C1);
    } else {
        rcc_periph_clock_disable(RCC_I2C2);
    }

    return 0;
}

static int device_request(int inst)
{
    if (inst >= I2C_MAX || device_data[inst].flags) {
        return -CUPKEE_EINVAL;
    }

    device_data[inst].flags = HW_FL_USED;
    device_data[inst].entry = NULL;

    return 0;
}

static int device_release(int inst)
{
    hw_i2c_t *i2c = device_block(inst);

    if (i2c) {
        i2c->flags = 0;
        return 0;
    } else {
        return -CUPKEE_EINVAL;
    }
}

static int device_get(int inst, int i, uint32_t *data)
{
    hw_i2c_t *i2c = device_block(inst);

    if (i2c && i == 0) {
        *data = i2c->peer_addr;
        return 1;
    }
    return 0;
}

static int device_set(int inst, int i, uint32_t data)
{
    hw_i2c_t *i2c = device_block(inst);

    if (i == 0) {
        i2c->peer_addr = data & 0xFE;
    }

    return 1;
}

static int device_query(int inst, int want)
{
    hw_i2c_t *i2c = device_block(inst);

    if (!i2c) {
        return -CUPKEE_EINVAL;
    }

    i2c->send_buf = cupkee_device_request_ptr(i2c->entry);
    i2c->send_len = cupkee_device_request_len(i2c->entry);
    if (want > 0) {
        i2c->recv_buf = cupkee_device_response_ptr(i2c->entry);
        i2c->recv_len = want;
    } else {
        i2c->recv_len = 0;
    }

    device_start(i2c);

    return 0;
}

static const cupkee_driver_t device_driver = {
    .request = device_request,
    .release = device_release,
    .reset   = device_reset,
    .setup   = device_setup,
    .poll    = device_poll,

    .query = device_query,
    .set = device_set,
    .get = device_get,
};

static const cupkee_device_desc_t hw_device_i2c = {
    .name = "i2c",
    .inst_max = I2C_MAX,
    .conf_init = device_conf_init,
    .driver = &device_driver
};

void hw_setup_i2c(void)
{
    device_data[0].flags = 0;
    device_data[0].state = 0;
    device_data[0].base = I2C1;

    device_data[1].flags = 0;
    device_data[1].state = 0;
    device_data[1].base = I2C2;

    cupkee_device_register(&hw_device_i2c);
}

