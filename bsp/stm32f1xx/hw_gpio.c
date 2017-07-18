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

#define BANK_MAX                7
#define BANK_MASK               7

#define PIN_MAX                 16
#define PIN_MASK                15

#define PIN_MAP_MAX             16

typedef struct hw_pin_t {
    uint8_t inused;
    uint8_t dev_id;
    uint16_t  data;
    const hw_config_pin_t *config;
} hw_pin_t;

static uint8_t  led_map;
static uint8_t  pin_maps[PIN_MAP_MAX];
static hw_pin_t pin_controls[HW_INSTANCES_PIN];
static uint16_t hw_gpio_used[BANK_MAX];
static const uint32_t hw_gpio_rcc[BANK_MAX] = {
    RCC_GPIOA, RCC_GPIOB, RCC_GPIOC, RCC_GPIOD, RCC_GPIOE, RCC_GPIOF, RCC_GPIOF
};
static const uint32_t hw_gpio_bank[BANK_MAX] = {
    GPIOA, GPIOB, GPIOC, GPIOD, GPIOE, GPIOF, GPIOG
};

static inline int map_is_valid(uint8_t map)
{
    return map != 0xff;
}

static inline uint8_t map_gpio(int bank, int pin)
{
    return (bank << 4) | (pin & PIN_MASK);
}

static inline int map_bank(uint8_t map) {
    return (map >> 4) & BANK_MASK;
}

static inline int map_pin(uint8_t map) {
    return map & PIN_MASK;
}

static inline void map_set_mode(uint8_t map, uint8_t mode, uint8_t cnf) {
    uint32_t bank_base = hw_gpio_bank[map_bank(map)];
    int pins = 1 << map_pin(map);

    gpio_set_mode(bank_base, mode, cnf, pins);
}

static inline void map_write(uint8_t map, int v) {
    uint32_t bank_base = hw_gpio_bank[map_bank(map)];
    int pin = map_pin(map);

    if (v)
        GPIO_BSRR(bank_base) = 1 << pin;
    else
        GPIO_BSRR(bank_base) = 1 << (pin + 16);
}

static inline int map_read(uint8_t map) {
    uint32_t bank_base = hw_gpio_bank[(map >> 4) & BANK_MAX];
    int pin = map & PIN_MASK;

    return (GPIO_IDR(bank_base) & (1 << pin)) != 0;
}

static void maps_write(int start, int n, uint32_t v) {
    int i;

    for (i = 0; i < n && start + i < PIN_MAP_MAX; i++) {
        uint8_t map = pin_maps[start + i];

        map_write(map, v & (1 << i));
    }
}

static uint32_t maps_read(int start, int n) {
    int i;
    uint32_t v = 0;

    for (i = 0; i < n && start + i < PIN_MAP_MAX; i++) {
        uint8_t map = pin_maps[start + i];

        v |= map_read(map) << i;
    }
    return v;
}

static inline void map_toggle(uint8_t map) {
    uint32_t bank_base = hw_gpio_bank[(map >> 4) & BANK_MAX];
    int pin = map & PIN_MASK;

    gpio_toggle(bank_base, 1 << pin);
}

static inline int map_use(uint8_t map) {
    return hw_gpio_use(map_bank(map), 1 << map_pin(map));
}

static inline void map_release(uint8_t map) {
    if (map_is_valid(map)) {
        hw_gpio_release(map_bank(map), 1 << map_pin(map));
    }
}

/******************************************************************
 * GPIO device: PIN
 ******************************************************************/
static void pin_reset(int instance)
{
    hw_pin_t *control = &pin_controls[instance];
    const hw_config_pin_t *config = control->config;
    int i;

    for (i = 0; i < config->num; i++) {
        uint8_t map = pin_maps[config->start + i];
        map_release(map);
    }

    pin_controls[instance].dev_id = DEVICE_ID_INVALID;
    pin_controls[instance].config = NULL;
}

static void pin_release(int instance)
{
    pin_reset(instance);

    pin_controls[instance].inused = 0;
}

static int pin_setup(int instance, uint8_t dev_id, const hw_config_t *conf)
{
    hw_pin_t *control = &pin_controls[instance];
    const hw_config_pin_t *config = (const hw_config_pin_t *) conf;
    int i, n, err;
    uint8_t cnf, mod;

    if (config->num == 0 || config->start + config->num > PIN_MAP_MAX) {
        return -CUPKEE_EINVAL;
    }

    switch(config->dir) {
    case DEVICE_OPT_DIR_IN:
        mod = GPIO_MODE_INPUT;
        cnf = GPIO_CNF_INPUT_PULL_UPDOWN;
        break;
    case DEVICE_OPT_DIR_OUT:
        mod = GPIO_MODE_OUTPUT_2_MHZ;
        cnf = GPIO_CNF_OUTPUT_PUSHPULL;
        break;
    case DEVICE_OPT_DIR_DUAL:
        mod = GPIO_MODE_OUTPUT_2_MHZ;
        cnf = GPIO_CNF_OUTPUT_OPENDRAIN;
        break;
    default:
        return -CUPKEE_EINVAL;
    }

    for (i = 0; i < config->num; i++) {
        uint8_t map = pin_maps[config->start + i];
        n = i;
        if (!map_is_valid(map)) {
            err = CUPKEE_EINVAL;
            goto DO_ERROR;
        }

        if (!map_use(map)) {
            err = CUPKEE_ERESOURCE;
            goto DO_ERROR;
        }
        map_set_mode(map, mod, cnf);
    }

    control->dev_id = dev_id;
    control->config = config;
    if (config->dir == DEVICE_OPT_DIR_OUT) {
        control->data = 0;
    } else {
        control->data = maps_read(config->start, config->num);
    }
    return CUPKEE_OK;

DO_ERROR:
    for (i = 0; i < n; i++) {
        map_release(pin_maps[config->start + i]);
    }

    return -err;
}

static void pin_sync(int instance, uint32_t systicks)
{
    hw_pin_t *control = &pin_controls[instance];
    uint16_t data;

    (void) systicks;

    if (control->config->dir == DEVICE_OPT_DIR_OUT) {
        return;
    }

    data = maps_read(control->config->start, control->config->num);
    if (data != control->data) {
        control->data = data;
        cupkee_event_post_device_data(control->dev_id);
    }
}

static int pin_get(int instance, int off, uint32_t *data)
{
    hw_pin_t *control = &pin_controls[instance];

    if (control->config->dir == DEVICE_OPT_DIR_OUT) {
        return 0;
    }

    if (off < 0) {
        *data = maps_read(control->config->start, control->config->num);
        return 1;
    } else
    if (off < control->config->num) {
        uint8_t map = pin_maps[control->config->start + off];
        *data = map_read(map);
        return 1;
    }

    return 0;
}

static int pin_set(int instance, int off, uint32_t data)
{
    hw_pin_t *control = &pin_controls[instance];

    if (control->config->dir == DEVICE_OPT_DIR_IN) {
        return 0;
    }

    if (off < 0) {
        maps_write(control->config->start, control->config->num, data);
        return 1;
    } else
    if (off < control->config->num) {
        uint8_t map = pin_maps[control->config->start + off];
        map_write(map, data);
        return 1;
    }
    return 0;
}

static int pin_size(int instance)
{
    hw_pin_t *control = &pin_controls[instance];

    return control->config->num;
}

static const hw_driver_t pin_driver = {
    .release = pin_release,
    .reset   = pin_reset,
    .setup   = pin_setup,
    .sync    = pin_sync,

    .get = pin_get,
    .set = pin_set,
    .size = pin_size
};

int hw_setup_gpio(void)
{
    int i;

    /* initial all pin useable */
    for (i = 0; i < BANK_MAX; i++) {
        hw_gpio_used[i] = 0;
    }

    /* initial all pin map invalid */
    for (i = 0; i < PIN_MAP_MAX; i++) {
        pin_maps[i] = 0xff;
    }
    led_map = 0xff;

    /* initial pin device control blocks */
    for (i = 0; i < HW_INSTANCES_PIN; i++) {
        pin_controls[i].inused = 0;
        pin_controls[i].data = 0;
    }

    return 0;
}

int hw_pin_map(int id, int bank, int pin)
{
    if (id < 0 || id >= PIN_MAP_MAX || bank >= BANK_MAX || pin >= PIN_MAX) {
        return -CUPKEE_EINVAL;
    }

    pin_maps[id] = map_gpio(bank, pin);

    return CUPKEE_OK;
}

int hw_led_map(int bank, int pin)
{
    uint8_t map;

    if (bank >= BANK_MAX || pin >= PIN_MAX) {
        return -CUPKEE_EINVAL;
    }
    map = map_gpio(bank, pin);

    if (CUPKEE_FALSE == map_use(map)) {
        return -CUPKEE_ERESOURCE;
    }
    map_set_mode(map, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL);

    map_release(led_map);
    led_map = map;

    return CUPKEE_OK;
}

void hw_led_set(void)
{
    if (map_is_valid(led_map)) {
        map_write(led_map, 1);
    }
}

void hw_led_clear(void)
{
    if (map_is_valid(led_map)) {
        map_write(led_map, 0);
    }
}

void hw_led_toggle(void)
{
    if (map_is_valid(led_map)) {
        map_toggle(led_map);
    }
}

int hw_gpio_use(int bank, uint16_t pins)
{
    if (bank >= BANK_MAX || (hw_gpio_used[bank] & pins)) {
        return CUPKEE_FALSE;
    }

    if (!hw_gpio_used[bank]) {
        rcc_periph_clock_enable(hw_gpio_rcc[bank]);
    }

    hw_gpio_used[bank] |= pins;

    return CUPKEE_TRUE;
}

int hw_gpio_use_setup(int bank, uint16_t pins, uint8_t mode, uint8_t cnf)
{
    uint32_t bank_base = hw_gpio_bank[bank];

    if (hw_gpio_use(bank, pins)) {
        gpio_set_mode(bank_base, mode, cnf, pins);
        return CUPKEE_TRUE;
    }
    return CUPKEE_FALSE;
}

int hw_gpio_release(int bank, uint16_t pins)
{
    if (bank >= BANK_MAX) {
        return 0;
    }

    hw_gpio_used[bank] &= ~pins;
    if (!hw_gpio_used[bank]) {
        rcc_periph_clock_disable(hw_gpio_rcc[bank]);
    }
    return 1;
}

const hw_driver_t *hw_request_pin(int instance)
{
    if (instance >= HW_INSTANCES_PIN || pin_controls[instance].inused) {
        return NULL;
    }

    pin_controls[instance].inused = 1;
    pin_controls[instance].dev_id = DEVICE_ID_INVALID;
    pin_controls[instance].config = NULL;

    return &pin_driver;
}

