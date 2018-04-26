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

#define HW_FL_I2C_RX    0x10

#define I2C_MAX         2

typedef struct hw_i2c_t {
    uint8_t flags;
    uint8_t nv;
    uint8_t self_addr;
    uint8_t peer_addr;

    uint32_t base;

    void *entry;

    uint8_t send_len;
    uint8_t recv_len;
    uint8_t *recv_buf;
    uint8_t *send_buf;
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

    if (hw_gpio_use_setup(1, pins, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_OPENDRAIN)) {
        return 0;
    } else {
        return -1;
    }
}



static void device_setup_dma(hw_i2c_t *i2c)
{
    if (i2c->base == I2C1) {
        if (i2c->send_len > 0) {
            DMA1_CMAR6 = (uint32_t)i2c->send_buf;
            DMA1_CPAR6 = (uint32_t)&I2C1_DR;
            DMA1_CNDTR6 = i2c->send_len;
            DMA1_CCR6 = DMA_CCR_PL_HIGH |
                        DMA_CCR_MSIZE_8BIT |
                        DMA_CCR_PSIZE_8BIT |
                        DMA_CCR_MINC |
                        DMA_CCR_DIR | // READ from memory
                        DMA_CCR_TCIE;
            nvic_set_priority(NVIC_DMA1_CHANNEL6_IRQ, 0);
        }
        if (i2c->recv_len > 1) {
            DMA1_CPAR7 = (uint32_t)&I2C1_DR;
            DMA1_CMAR7 = (uint32_t)i2c->recv_buf;
            DMA1_CNDTR7 = i2c->recv_len;
            DMA1_CCR7 = DMA_CCR_PL_HIGH |
                        DMA_CCR_MSIZE_8BIT |
                        DMA_CCR_PSIZE_8BIT |
                        DMA_CCR_MINC |
                        // READ from peripheral
                        DMA_CCR_TCIE;
            nvic_set_priority(NVIC_DMA1_CHANNEL7_IRQ, 0);
        }
    } else {
        if (i2c->send_len > 0) {
            DMA1_CMAR4 = (uint32_t)i2c->send_buf;
            DMA1_CPAR4 = (uint32_t)&I2C2_DR;
            DMA1_CNDTR4 = i2c->send_len;
            DMA1_CCR4 = DMA_CCR_PL_HIGH |
                        DMA_CCR_MSIZE_8BIT |
                        DMA_CCR_PSIZE_8BIT |
                        DMA_CCR_MINC |
                        DMA_CCR_DIR | // READ from memory
                        DMA_CCR_TCIE;
            nvic_set_priority(NVIC_DMA1_CHANNEL4_IRQ, 0);
        }
        if (i2c->recv_len > 1) {
            DMA1_CPAR5 = (uint32_t)&I2C2_DR;
            DMA1_CMAR5 = (uint32_t)i2c->recv_buf;
            DMA1_CNDTR5 = i2c->recv_len;
            DMA1_CCR5 = DMA_CCR_PL_HIGH |
                        DMA_CCR_MSIZE_8BIT |
                        DMA_CCR_PSIZE_8BIT |
                        DMA_CCR_MINC |
                        // READ from peripheral
                        DMA_CCR_TCIE;
            nvic_set_priority(NVIC_DMA1_CHANNEL5_IRQ, 0);
        }
    }
}

static void device_reset_dma(int inst)
{
    if (inst == 0) {
        DMA1_CCR6 = 0;
        DMA1_CCR7 = 0;
        nvic_disable_irq(NVIC_DMA1_CHANNEL6_IRQ);
        nvic_disable_irq(NVIC_DMA1_CHANNEL7_IRQ);

        if (!device_data[1].flags & HW_FL_USED)
            rcc_periph_clock_disable(RCC_DMA1);
    } else {
        DMA1_CCR4 = 0;
        DMA1_CCR5 = 0;
        nvic_disable_irq(NVIC_DMA1_CHANNEL4_IRQ);
        nvic_disable_irq(NVIC_DMA1_CHANNEL5_IRQ);

        if (!device_data[0].flags & HW_FL_USED)
            rcc_periph_clock_disable(RCC_DMA1);
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

    hw_gpio_release(1, pins);
}

static inline void device_send_start(hw_i2c_t *i2c) {

    // Enable DMA
    if (i2c->base == I2C1) {
        DMA1_CCR6 |= DMA_CCR_EN;
        nvic_enable_irq(NVIC_DMA1_CHANNEL6_IRQ);
    } else {
        DMA1_CCR4 |= DMA_CCR_EN;
        nvic_enable_irq(NVIC_DMA1_CHANNEL4_IRQ);
    }

    // Enable I2C DMA
    I2C_CR2(i2c->base) |= I2C_CR2_DMAEN;
    // Start
    I2C_CR1(i2c->base) |= I2C_CR1_START;
}

static inline void device_recv_start(hw_i2c_t *i2c) {
    i2c->flags = HW_FL_I2C_RX | HW_FL_USED;

    if (i2c->recv_len > 1) {
        // Enable DMA
        if (i2c->base == I2C1) {
            DMA1_CCR7 |= DMA_CCR_EN;
            nvic_enable_irq(NVIC_DMA1_CHANNEL7_IRQ);
        } else {
            DMA1_CCR5 |= DMA_CCR_EN;
            nvic_enable_irq(NVIC_DMA1_CHANNEL5_IRQ);
        }

        // Enable I2C DMA
        I2C_CR2(i2c->base) |= I2C_CR2_DMAEN | I2C_CR2_LAST;
    }

    // Start
    I2C_CR1(i2c->base) |= I2C_CR1_START;
}

static void device_stop(hw_i2c_t *i2c)
{
    i2c->flags = HW_FL_USED; // Clear all software flags

    I2C_CR1(i2c->base) |= I2C_CR1_STOP;

    cupkee_device_response_submit(i2c->entry, i2c->recv_len);

    I2C_CR1(i2c->base) &= ~I2C_CR1_PE;
}

static void device_start(hw_i2c_t *i2c)
{
    device_setup_dma(i2c);

    // Enable i2c device
    I2C_CR2(i2c->base) |= I2C_CR2_ITEVTEN | I2C_CR2_ITERREN;
    I2C_CR1(i2c->base) |= I2C_CR1_PE;

    if (i2c->send_len == 0) {
        device_recv_start(i2c);
    } else {
        device_send_start(i2c);
    }
}

static void device_isr(int inst)
{
    hw_i2c_t *i2c = device_block(inst);

    if (i2c) {
        uint32_t base = i2c->base;
        uint16_t SR1 = I2C_SR1(i2c->base);
        uint16_t SR2;

        if (SR1 & I2C_SR1_SB) {
            // Enter master mode
            // Send addr & Set mode (Rx | Tx) & Clear SB flag
            I2C_DR(base) = i2c->peer_addr | (i2c->flags & HW_FL_I2C_RX ? 1 : 0);
        } else
        if (SR1 & I2C_SR1_ADDR) {
            // Note: ACK always as reset
            // Clear ADDR by read SR2 & Trigger TxE or TxNE event
            SR2 = I2C_SR2(base);

            // Receive one
            if ((SR2 & I2C_SR2_TRA)  && i2c->recv_len == 1) {
                // Enable RxNE to trigger interrupt
                I2C_CR2(base) |= I2C_CR2_ITBUFEN;
                I2C_CR1(base) |= I2C_CR1_STOP;
            }
        } else
        if (SR1 & I2C_SR1_BTF) {
            if (i2c->flags & HW_FL_I2C_RX) {
                if (i2c->recv_len == 1) {
                    // Should not go here, some defence code
                    I2C_CR2(base) &= ~I2C_CR2_ITBUFEN;
                    i2c->recv_buf[0] = 1;
                    device_stop(i2c);
                } else {
                    device_stop(i2c);
                }
            } else { // Transmit complete
                if (i2c->recv_len) {
                    device_recv_start(i2c);
                } else {
                    device_stop(i2c);
                }
            }
        } else
        if (SR1 & I2C_SR1_RxNE) {
            // Only when 1 byte receive mode
            // Disable RxNE to trigger interrupt
            I2C_CR2(base) &= ~I2C_CR2_ITBUFEN;
            i2c->recv_buf[0] = I2C_DR(base);
            device_stop(i2c);
        }
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

    // Enable DMA
    rcc_periph_clock_enable(RCC_DMA1);

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
    device_reset_dma(inst);

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
    device_data[0].base = I2C1;

    device_data[1].flags = 0;
    device_data[1].base = I2C2;

    cupkee_device_register(&hw_device_i2c);
}

void i2c1_ev_isr(void)
{
    device_isr(0);
}

void i2c2_ev_isr(void)
{
    device_isr(1);
}

void i2c1_er_isr(void)
{
    device_stop(&device_data[0]);
}

void i2c2_er_isr(void)
{
    device_stop(&device_data[1]);
}

void dma1_channel4_isr(void)
{
    hw_i2c_t *i2c = &device_data[1];

    // Clear dma event flag
    if (DMA1_ISR & DMA_ISR_TCIF4) {
        DMA1_IFCR |= DMA_IFCR_CTCIF4;
    }

    // Stop DMA
    DMA1_CCR4 &= ~DMA_CCR_EN;
    nvic_disable_irq(NVIC_DMA1_CHANNEL4_IRQ);

    I2C_CR2(i2c->base) &= ~I2C_CR2_DMAEN;
}

void dma1_channel5_isr(void)
{
    hw_i2c_t *i2c = &device_data[1];

    // Clear dma event flag
    if (DMA1_ISR & DMA_ISR_TCIF5) {
        DMA1_IFCR |= DMA_IFCR_CTCIF5;
    }

    // Stop DMA
    DMA1_CCR5 &= ~DMA_CCR_EN;
    nvic_disable_irq(NVIC_DMA1_CHANNEL5_IRQ);

    I2C_CR2(i2c->base) &= ~(I2C_CR2_DMAEN | I2C_CR2_LAST);
}

void dma1_channel6_isr(void)
{
    hw_i2c_t *i2c = &device_data[0];

    // Clear dma event flag
    if (DMA1_ISR & DMA_ISR_TCIF6) {
        DMA1_IFCR |= DMA_IFCR_CTCIF6;
    }

    // Stop DMA
    DMA1_CCR6 &= ~DMA_CCR_EN;
    nvic_disable_irq(NVIC_DMA1_CHANNEL6_IRQ);

    I2C_CR2(i2c->base) &= ~I2C_CR2_DMAEN;
}

void dma1_channel7_isr(void)
{
    hw_i2c_t *i2c = &device_data[0];

    // Clear dma event flag
    if (DMA1_ISR & DMA_ISR_TCIF7) {
        DMA1_IFCR |= DMA_IFCR_CTCIF7;
    }

    // Stop DMA
    DMA1_CCR7 &= ~DMA_CCR_EN;
    nvic_disable_irq(NVIC_DMA1_CHANNEL7_IRQ);

    I2C_CR2(i2c->base) &= ~(I2C_CR2_DMAEN | I2C_CR2_LAST);
}

