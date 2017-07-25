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

#include "hardware.h"

typedef struct hw_spi_t {
    uint8_t flags;
    uint8_t devid;
    uint8_t done;
    uint8_t nv;

    uint16_t rxmax;
    uint16_t txmax;
    uint16_t rxcnt;
    uint16_t txcnt;

    uint8_t *txbuf;

    const hw_config_spi_t *conf;
} hw_spi_t;

static void hw_spi_release(int instance);
static void hw_spi_reset(int instance);
static int hw_spi_setup(int instance, uint8_t dev_id, const hw_config_t *conf);
static int hw_spi_query(int instance, size_t send, int want);
static void hw_spi_poll(int instance);

static const uint32_t hw_reg_base[] = {
    SPI1, SPI2, SPI3
};

static const uint32_t hw_reg_rcc[] = {
    RCC_SPI1, RCC_SPI2, RCC_SPI3
};

static hw_spi_t hw_spi[HW_INSTANCES_SPI];

static const hw_driver_t hw_spi_driver = {
    .release = hw_spi_release,
    .reset   = hw_spi_reset,
    .setup   = hw_spi_setup,
    .query   = hw_spi_query,
    .poll    = hw_spi_poll,
};

static inline hw_spi_t *hw_device(int instance)
{
    if (instance >= HW_INSTANCES_SPI || !(hw_spi[instance].flags & HW_FL_USED)) {
        return NULL;
    }

    return &hw_spi[instance];
}

static cupkee_device_t *hw_cupkee_device(int instance)
{
    if (instance >= HW_INSTANCES_SPI || !(hw_spi[instance].flags & HW_FL_USED)) {
        return NULL;
    }

    return cupkee_device_block(hw_spi[instance].devid);
}

static void hw_reset_pin(int instance)
{
    if (instance == 0) {
        hw_gpio_release(0, GPIO4 | GPIO5 | GPIO7);
        hw_gpio_release(0, GPIO6);
    } else
    if (instance == 1) {
        hw_gpio_release(1, GPIO12 | GPIO13 | GPIO15);
        hw_gpio_release(1, GPIO14);
    } else {
        hw_gpio_release(0, GPIO15);
        hw_gpio_release(1, GPIO5 | GPIO7);
        hw_gpio_release(1, GPIO6);
    }
}

static int hw_setup_pin(int instance)
{
    if (instance == 0) {
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
    if (instance == 1) {
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

static void hw_recv(hw_spi_t *spi, uint32_t base_reg)
{
    uint8_t dat = SPI_DR(base_reg);

    if (spi->rxcnt >= spi->txmax) {
        cupkee_device_response_push(cupkee_device_block(spi->devid), 1, &dat);
    }

    if (++spi->rxcnt >= spi->rxmax) {
        spi->done = 1;
    }
}

static void hw_send(hw_spi_t *spi, uint32_t base_reg)
{
    if (spi->txcnt < spi->txmax) {
        SPI_DR(base_reg) = spi->txbuf[spi->txcnt];
    } else
    if (spi->txcnt < spi->rxmax) {
        SPI_DR(base_reg) = 0;
    }
    ++spi->txcnt;
}

static void hw_spi_reset(int instance)
{
    hw_reset_pin(instance);

    SPI_CR1(hw_reg_base[instance]) = 0;
    SPI_CR2(hw_reg_base[instance]) = 0;

    rcc_periph_clock_disable(hw_reg_rcc[instance]);
}

static int hw_spi_setup(int instance, uint8_t dev_id, const hw_config_t *conf)
{
    hw_spi_t *spi = hw_device(instance);

    if (!spi) {
        return -1;
    }

    rcc_periph_clock_enable(hw_reg_rcc[instance]);

    if (hw_setup_pin(instance)) {
        rcc_periph_clock_disable(hw_reg_base[instance]);
        return -1;
    }

    spi->done = 0;
    spi->conf = (const hw_config_spi_t *)conf;
    spi->devid = dev_id;

    spi_reset(hw_reg_base[instance]);

    SPI_I2SCFGR(hw_reg_base[instance]) = 0;

    SPI_CR1(hw_reg_base[instance]) = SPI_CR1_SSM |
                                     SPI_CR1_SSI |
                                     SPI_CR1_MSBFIRST |    // 8Bit MSB
                                     SPI_CR1_BAUDRATE_FPCLK_DIV_32 |
                                     SPI_CR1_MSTR;

    SPI_CR2(hw_reg_base[instance]) = 0;

    SPI_CR1(hw_reg_base[instance]) |= SPI_CR1_SPE;

    return 0;
}

static void hw_spi_release(int instance)
{
    if (instance < HW_INSTANCES_SPI && (hw_spi[instance].flags & HW_FL_USED)) {
        hw_spi_reset(instance);
        hw_spi[instance].flags = 0;
    }
}

static int hw_spi_query(int instance, size_t send, int want)
{
    hw_spi_t *spi = hw_device(instance);
    cupkee_device_t *dev = hw_cupkee_device(instance);

    if (!spi || !dev) {
        return -CUPKEE_EINVAL;
    }

    if (send + want == 0) {
        cupkee_device_response_end(dev);
        return 0;
    }

    if (spi->flags & HW_FL_BUSY) {
        return -CUPKEE_EBUSY;
    }

    if (send && NULL == (spi->txbuf = cupkee_device_request_ptr(dev))) {
        return -CUPKEE_EINVAL;
    }

    spi->txmax = send;
    spi->rxmax = send + want;
    spi->txcnt = 0;
    spi->rxcnt = 0;

    spi->flags |= HW_FL_BUSY;

    while (SPI_SR(hw_reg_base[instance]) & SPI_SR_BSY) {
        ;
    }

    if (spi->txcnt < spi->txmax) {
        SPI_DR(hw_reg_base[instance]) = spi->txbuf[spi->txcnt++];
    } else {
        SPI_DR(hw_reg_base[instance]) = 0;
    }

    return 0;
}

static void hw_spi_poll(int instance)
{
    hw_spi_t *spi = hw_device(instance);

    if (spi->flags & HW_FL_BUSY) {
        if (1) {
            uint32_t sr = SPI_SR(SPI1);

            if (sr & SPI_SR_RXNE) {
                hw_recv(spi, SPI1);
            }
            if (sr & SPI_SR_TXE) {
                hw_send(spi, SPI1);
            }
        }
        if (spi->done && !(SPI_SR(hw_reg_base[instance]) & SPI_SR_BSY)) {
            spi->flags &= ~HW_FL_BUSY;
            spi->done = 0;

            cupkee_device_t *dev = hw_cupkee_device(instance);
            if (dev) {
                cupkee_device_response_end(dev);
            }
        }
    }
}

const hw_driver_t *hw_request_spi(int instance)
{
    if (instance >= HW_INSTANCES_SPI || hw_spi[instance].flags) {
        return NULL;
    }

    hw_spi[instance].flags = HW_FL_USED;
    hw_spi[instance].devid = DEVICE_ID_INVALID;
    hw_spi[instance].conf = NULL;
    hw_spi[instance].done = 0;

    return &hw_spi_driver;
}

void hw_setup_spi(void)
{
    int i;

    for (i = 0; i < HW_INSTANCES_SPI; i++) {
        hw_spi[i].flags = 0;
    }
}

/*
void spi1_isr(void)
{
    uint32_t sr = SPI_SR(SPI1);
    console_log("spi1 isr call\r\n");

    if (sr & SPI_SR_RXNE) {
        hw_recv(&hw_spi[0], SPI1);
    }
    if (sr & SPI_SR_TXE) {
        hw_send(&hw_spi[0], SPI1);
    }
}

void spi2_isr(void)
{
    uint32_t sr = SPI_SR(SPI2);
    if (sr & SPI_SR_RXNE) {
        hw_recv(&hw_spi[1], SPI2);
    }
    if (sr & SPI_SR_TXE) {
        hw_send(&hw_spi[1], SPI2);
    }
}

void spi3_isr(void)
{
    uint32_t sr = SPI_SR(SPI3);
    if (sr & SPI_SR_RXNE) {
        hw_recv(&hw_spi[2], SPI3);
    }
    if (sr & SPI_SR_TXE) {
        hw_send(&hw_spi[2], SPI3);
    }
}
*/
