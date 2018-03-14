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

#define SPI_MAX     3

typedef struct hw_spi_t {
    uint8_t flags;
    uint8_t done;

    void   *entry;

    uint16_t rxmax;
    uint16_t txmax;
    uint16_t rxcnt;
    uint16_t txcnt;

    uint8_t *txbuf;
} hw_spi_t;

static const uint32_t reg_base[] = {
    SPI1, SPI2, SPI3
};

static const uint32_t rcc_base[] = {
    RCC_SPI1, RCC_SPI2, RCC_SPI3
};

static hw_spi_t spis[SPI_MAX];

static inline hw_spi_t *hw_device(int inst)
{
    if (inst >= SPI_MAX || !(spis[inst].flags & HW_FL_USED)) {
        return NULL;
    }

    return &spis[inst];
}

static void hw_reset_pin(int inst)
{
    if (inst == 0) {
        hw_gpio_release(0, GPIO4 | GPIO5 | GPIO7);
        hw_gpio_release(0, GPIO6);
    } else
    if (inst == 1) {
        hw_gpio_release(1, GPIO12 | GPIO13 | GPIO15);
        hw_gpio_release(1, GPIO14);
    } else {
        hw_gpio_release(0, GPIO15);
        hw_gpio_release(1, GPIO5 | GPIO7);
        hw_gpio_release(1, GPIO6);
    }
}

static int hw_setup_pin(int inst)
{
    if (inst == 0) {
        if (!hw_gpio_use_setup(0, GPIO4 | GPIO5 | GPIO7,
                               GPIO_MODE_OUTPUT_50_MHZ,
                               GPIO_CNF_OUTPUT_ALTFN_PUSHPULL)) {
            return -1;
        }
        if (!hw_gpio_use_setup(0, GPIO6,
                               GPIO_MODE_INPUT,
                               GPIO_CNF_INPUT_FLOAT)) {
            hw_gpio_release(0, GPIO4 | GPIO5 | GPIO7);
            return -1;
        }
    } else
    if (inst == 1) {
        if (!hw_gpio_use_setup(1, GPIO12 | GPIO13 | GPIO15,
                               GPIO_MODE_OUTPUT_50_MHZ,
                               GPIO_CNF_OUTPUT_ALTFN_PUSHPULL)) {
            return -1;
        }
        if (!hw_gpio_use_setup(1, GPIO14,
                               GPIO_MODE_INPUT,
                               GPIO_CNF_INPUT_FLOAT)) {
            hw_gpio_release(0, GPIO12 | GPIO13 | GPIO15);
            return -1;
        }
    } else {
        if (!hw_gpio_use_setup(0, GPIO15,
                               GPIO_MODE_OUTPUT_50_MHZ,
                               GPIO_CNF_OUTPUT_ALTFN_PUSHPULL)) {
            return -1;
        }
        if (!hw_gpio_use_setup(1, GPIO5 | GPIO7,
                               GPIO_MODE_OUTPUT_50_MHZ,
                               GPIO_CNF_OUTPUT_ALTFN_PUSHPULL)) {
            hw_gpio_release(0, GPIO15);
            return -1;
        }
        if (!hw_gpio_use_setup(1, GPIO6,
                               GPIO_MODE_INPUT,
                               GPIO_CNF_INPUT_FLOAT)) {
            hw_gpio_release(1, GPIO5 | GPIO7);
            return -1;
        }
    }
    return 0;
}

static void do_recv(hw_spi_t *spi, uint32_t base_reg)
{
    uint8_t dat = SPI_DR(base_reg);

    if (spi->rxcnt >= spi->txmax) {
        cupkee_device_response_push(spi->entry, 1, &dat);
    }

    if (++spi->rxcnt >= spi->rxmax) {
        spi->done = 1;
    }
}

static void do_send(hw_spi_t *spi, uint32_t base_reg)
{
    if (spi->txcnt < spi->txmax) {
        SPI_DR(base_reg) = spi->txbuf[spi->txcnt];
    } else
    if (spi->txcnt < spi->rxmax) {
        SPI_DR(base_reg) = 0;
    }
    ++spi->txcnt;
}

static int device_query(int inst, int want)
{
    hw_spi_t *spi = hw_device(inst);
    int send;

    if (!spi) {
        return -CUPKEE_EINVAL;
    }

    if (spi->flags & HW_FL_BUSY) {
        return -CUPKEE_EBUSY;
    }

    send = cupkee_device_request_len(spi->entry);
    if (send < 0) {
        return -CUPKEE_EINVAL;
    } else
    if (send + want == 0) {
        cupkee_device_response_end(spi->entry);
        return 0;
    } else
    if (send && NULL == (spi->txbuf = cupkee_device_request_ptr(spi->entry))) {
        return -CUPKEE_EINVAL;
    }

    spi->txmax = send;
    spi->rxmax = send + want;
    spi->txcnt = 0;
    spi->rxcnt = 0;

    spi->flags |= HW_FL_BUSY;

    while (SPI_SR(reg_base[inst]) & SPI_SR_BSY) {
        ;
    }

    if (spi->txcnt < spi->txmax) {
        SPI_DR(reg_base[inst]) = spi->txbuf[spi->txcnt++];
    } else {
        SPI_DR(reg_base[inst]) = 0;
    }

    return 0;
}

static int device_poll(int inst)
{
    hw_spi_t *spi = hw_device(inst);

    if (!spi) {
        return -CUPKEE_EINVAL;
    }

    if (spi->flags & HW_FL_BUSY) {
        uint32_t sr = SPI_SR(SPI1);

        if (sr & SPI_SR_RXNE) {
            do_recv(spi, reg_base[inst]);
        }
        if (sr & SPI_SR_TXE) {
            do_send(spi, reg_base[inst]);
        }

        if (spi->done && !(SPI_SR(reg_base[inst]) & SPI_SR_BSY)) {
            spi->flags &= ~HW_FL_BUSY;
            spi->done = 0;

            cupkee_device_response_end(spi->entry);
        }
    }

    return 0;
}

static int device_reset(int inst)
{
    hw_spi_t *spi = hw_device(inst);

    if (!spi) {
        return -CUPKEE_EINVAL;
    }

    hw_reset_pin(inst);

    SPI_CR1(reg_base[inst]) = 0;
    SPI_CR2(reg_base[inst]) = 0;
    rcc_periph_clock_disable(rcc_base[inst]);

    return 0;
}

static int device_setup(int inst, void *entry)
{
    hw_spi_t *spi = hw_device(inst);

    if (!spi) {
        return -CUPKEE_EINVAL;
    }

    rcc_periph_clock_enable(rcc_base[inst]);
    if (hw_setup_pin(inst)) {
        rcc_periph_clock_disable(reg_base[inst]);
        return -CUPKEE_ERESOURCE;
    }

    spi->done = 0;
    spi->entry = entry;

    spi_reset(reg_base[inst]);

    SPI_I2SCFGR(reg_base[inst]) = 0;
    SPI_CR1(reg_base[inst]) = SPI_CR1_SSM |
                              SPI_CR1_SSI |
                              SPI_CR1_MSBFIRST |    // 8Bit MSB
                              SPI_CR1_BAUDRATE_FPCLK_DIV_32 |
                              SPI_CR1_MSTR;
    SPI_CR2(reg_base[inst]) = 0;
    SPI_CR1(reg_base[inst]) |= SPI_CR1_SPE;

    return 0;
}

static int device_request(int inst)
{
    if (inst >= SPI_MAX || spis[inst].flags) {
        return -1;
    }

    spis[inst].flags = HW_FL_USED;
    spis[inst].entry = NULL;
    spis[inst].done = 0;

    return 0;
}

static int device_release(int inst)
{
    hw_spi_t *spi = hw_device(inst);

    if (!spi) {
        return -CUPKEE_EINVAL;
    }

    device_reset(inst);
    spi->flags = 0;

    return 0;
}

static const cupkee_driver_t device_driver = {
    .request = device_request,
    .release = device_release,
    .reset   = device_reset,
    .setup   = device_setup,
    .query   = device_query,
    .poll    = device_poll,
};

static const cupkee_device_desc_t hw_device_spi = {
    .name = "spi",
    .inst_max = SPI_MAX,
    .conf_init = NULL,
    .driver = &device_driver
};

void hw_setup_spi(void)
{
    int i;

    for (i = 0; i < SPI_MAX; i++) {
        spis[i].flags = 0;
    }

    cupkee_device_register(&hw_device_spi);
}

/*
void spi1_isr(void)
{
    uint32_t sr = SPI_SR(SPI1);
    console_log("spi1 isr call\r\n");

    if (sr & SPI_SR_RXNE) {
        do_recv(&hw_spi[0], SPI1);
    }
    if (sr & SPI_SR_TXE) {
        do_send(&hw_spi[0], SPI1);
    }
}

void spi2_isr(void)
{
    uint32_t sr = SPI_SR(SPI2);
    if (sr & SPI_SR_RXNE) {
        do_recv(&hw_spi[1], SPI2);
    }
    if (sr & SPI_SR_TXE) {
        do_send(&hw_spi[1], SPI2);
    }
}

void spi3_isr(void)
{
    uint32_t sr = SPI_SR(SPI3);
    if (sr & SPI_SR_RXNE) {
        do_recv(&hw_spi[2], SPI3);
    }
    if (sr & SPI_SR_TXE) {
        do_send(&hw_spi[2], SPI3);
    }
}
*/
