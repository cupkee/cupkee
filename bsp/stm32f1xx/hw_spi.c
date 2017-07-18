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
    uint8_t query_reply_expect;
    uint8_t query_reply_received;

    const hw_config_spi_t *config;
} hw_spi_t;

static void hw_spi_release(int instance);
static void hw_spi_reset(int instance);
static int hw_spi_setup(int instance, uint8_t dev_id, const hw_config_t *conf);
static int hw_spi_query(int instance, size_t n, void *data, int want);
static void hw_spi_poll(int instance);

static const uint32_t hw_spi_base[] = {
    SPI1, SPI2, SPI3
};

static const uint32_t hw_spi_rcc[] = {
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

static void hw_spi_release(int instance)
{
    (void) instance;
}

static void hw_spi_reset(int instance)
{
    (void) instance;
}

static int hw_spi_setup(int instance, uint8_t dev_id, const hw_config_t *conf)
{
    (void) instance;
    (void) dev_id;
    (void) conf;

    return -1;
}

static int hw_spi_query(int instance, size_t n, void *data, int want)
{
    (void) instance;
    (void) n;
    (void) data;
    (void) want;

    return -1;
}

static void hw_spi_poll(int instance)
{
    (void) instance;
}

const hw_driver_t *hw_request_spi(int instance)
{
    if (instance >= HW_INSTANCES_SPI || hw_spi[instance].flags) {
        return NULL;
    }

    hw_spi[instance].flags = HW_FL_USED;
    hw_spi[instance].devid = DEVICE_ID_INVALID;
    hw_spi[instance].config = NULL;

    return &hw_spi_driver;
}

void hw_setup_spi(void)
{
    int i;

    for (i = 0; i < HW_INSTANCES_SPI; i++) {
        hw_spi[i].flags = 0;
    }
}

