/*
MIT License

This file is part of cupkee project.

Copyright (c) 2016-2017 Lixing Ding <ding.lixing@gmail.com>

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

#include <cupkee.h>
#include "cupkee_shell_misc.h"
#include "cupkee_shell_device.h"

static const char * const device_pin_conf_names[] = {
    "pinNum", "pinStart", "dir"
};

static const char * const device_adc_conf_names[] = {
    "channel", "interval"
};

static const char * const device_pwm_pulse_timer_counter_conf_names[] = {
    "channel", "polarity", "period"
};

static const char * const device_uart_conf_names[] = {
    "baudrate", "dataBits", "stopBits", "parity"
};

static const char *device_i2c_conf_names[] = {
    "speed", "address", "mode"
};

static const char *device_spi_conf_names[] = {
    "speed", "mode"
};

static const char *device_opt_dir[] = {
    "in", "out", "duplex"
};

static const char *device_opt_polarity[] = {
    "positive", "negative", "edge"
};

static const char *device_opt_parity[] = {
    "none", "odd", "even",
};

static const char *device_opt_stopbits[] = {
    "1bit", "2bit", "0.5bit", "1.5bit"
};

static const cupkee_device_desc_t device_pin = {
    .name = "pin",
    .type = DEVICE_TYPE_PIN,
    .category = DEVICE_CATEGORY_MAP,
    .conf_num = 3,
    .conf_names = device_pin_conf_names
};

static const cupkee_device_desc_t device_adc = {
    .name = "adc",
    .type = DEVICE_TYPE_ADC,
    .category = DEVICE_CATEGORY_MAP,
    .conf_num = 2,
    .conf_names = device_adc_conf_names,
};

static const cupkee_device_desc_t device_pwm = {
    .name = "pwm",
    .type = DEVICE_TYPE_PWM,
    .category = DEVICE_CATEGORY_MAP,
    .conf_num = 3,
    .conf_names = device_pwm_pulse_timer_counter_conf_names,
};

static const cupkee_device_desc_t device_pulse = {
    .name = "pulse",
    .type = DEVICE_TYPE_PULSE,
    .category = DEVICE_CATEGORY_MAP,
    .conf_num = 2,
    .conf_names = device_pwm_pulse_timer_counter_conf_names,
};

static const cupkee_device_desc_t device_timer = {
    .name = "timer",
    .type = DEVICE_TYPE_TIMER,
    .category = DEVICE_CATEGORY_MAP,
    .conf_num = 2,
    .conf_names = device_pwm_pulse_timer_counter_conf_names,
};

static const cupkee_device_desc_t device_counter = {
    .name = "counter",
    .type = DEVICE_TYPE_COUNTER,
    .category = DEVICE_CATEGORY_MAP,
    .conf_num = 3,
    .conf_names = device_pwm_pulse_timer_counter_conf_names,
};

static const cupkee_device_desc_t device_uart = {
    .name = "uart",
    .type = DEVICE_TYPE_UART,
    .category = DEVICE_CATEGORY_STREAM,
    .conf_num = 4,
    .conf_names = device_uart_conf_names,
};

static const cupkee_device_desc_t device_i2c = {
    .name = "i2c",
    .type = DEVICE_TYPE_I2C,
    .category = DEVICE_CATEGORY_BLOCK,
    .conf_num = 2,
    .conf_names = device_i2c_conf_names,
};

static const cupkee_device_desc_t device_spi = {
    .name = "spi",
    .type = DEVICE_TYPE_SPI,
    .category = DEVICE_CATEGORY_BLOCK,
    .conf_num = 2,
    .conf_names = device_spi_conf_names,
};

static const cupkee_device_desc_t device_usb_cdc = {
    .name = "usb-cdc",
    .type = DEVICE_TYPE_USB_CDC,
    .category = DEVICE_CATEGORY_STREAM,
    .conf_num = 0,
};

static const cupkee_device_desc_t *device_entrys[] = {
    &device_pin,
    &device_adc,
    &device_pwm,
    &device_pulse,
    &device_timer,
    &device_counter,
    &device_uart,
    &device_i2c,
    &device_spi,
    &device_usb_cdc,
    NULL
};

static void device_config_get_option(val_t *opt, int i, int max, const char **opt_list)
{
    if (i >= max) {
        i = 0;
    }

    val_set_foreign_string(opt, (intptr_t) opt_list[i]);
}

static void device_config_get_sequence(env_t *env, val_t *val, uint8_t n, uint8_t *seq)
{
    array_t *a;
    int i;

    a = _array_create(env, n);
    if (a) {
        for (i = 0; i < n; i++) {
            val_set_number(_array_elem(a, i), seq[i]);
        }

        val_set_array(val, (intptr_t) a);
    }
}

static int device_config_set_uint8(val_t *val, uint8_t *conf)
{
    if (val_is_number(val)) {
        *conf = val_2_integer(val);
        return CUPKEE_OK;
    }
    return -CUPKEE_EINVAL;
}

static int device_config_set_uint16(val_t *val, uint16_t *conf)
{
    if (val_is_number(val)) {
        *conf = val_2_integer(val);
        return CUPKEE_OK;
    }
    return -CUPKEE_EINVAL;
}

static int device_config_set_uint32(val_t *val, uint32_t *conf)
{
    if (val_is_number(val)) {
        *conf = val_2_integer(val);
        return CUPKEE_OK;
    }
    return -CUPKEE_EINVAL;
}

static int device_config_set_option(val_t *val, uint8_t *conf, int max, const char **opt_list)
{
    int opt = shell_val_id(val, max, opt_list);

    if (opt >= 0) {
        *conf = opt;
        return CUPKEE_OK;
    }
    return -CUPKEE_EINVAL;
}

static int device_config_set_sequence(val_t *val, int max, uint8_t *n, uint8_t *seq)
{
    int i, len;
    val_t *elems;

    if (val_is_array(val)) {
        array_t *array = (array_t *)val_2_intptr(val);

        len   = array_len(array);
        if (len > max) {
            return -CUPKEE_EINVAL;
        }
        elems = array_values(array);
    } else
    if (val_is_number(val)) {
        len = 1;
        elems = val;
    } else{
        return -CUPKEE_EINVAL;
    }

    for (i = 0; i < len; i++) {
        val_t *cur = elems + i;
        int    num;

        if (!val_is_number(cur)) {
            return -CUPKEE_EINVAL;
        }

        num = val_2_integer(cur);
        if (num > 255) {
            return -CUPKEE_EINVAL;
        }
        seq[i] = num;
    }

    if (len) {
        *n = len;
        return CUPKEE_OK;
    } else {
        return -CUPKEE_EINVAL;
    }
}

static int device_pin_config_get(env_t *env, hw_config_t *conf, int which, val_t *val)
{
    hw_config_pin_t *pin = (hw_config_pin_t *) conf;

    (void) env;

    switch (which) {
    case DEVICE_PIN_CONF_NUM:   val_set_number(val, pin->num);   break;
    case DEVICE_PIN_CONF_START: val_set_number(val, pin->start); break;
    case DEVICE_PIN_CONF_DIR:   device_config_get_option(val, pin->dir, DEVICE_OPT_DIR_MAX, device_opt_dir); break;
    default:                    return -CUPKEE_EINVAL;
    }

    return CUPKEE_OK;
}

static int device_pin_config_set(env_t *env, hw_config_t *conf, int which, val_t *val)
{
    hw_config_pin_t *pin = (hw_config_pin_t *) conf;

    (void) env;

    switch (which) {
    case DEVICE_PIN_CONF_NUM:   return device_config_set_uint8(val, &pin->num);
    case DEVICE_PIN_CONF_START: return device_config_set_uint8(val, &pin->start);
    case DEVICE_PIN_CONF_DIR:   return device_config_set_option(val, &pin->dir, DEVICE_OPT_DIR_MAX, device_opt_dir);
    default:                    return -CUPKEE_EINVAL;
    }
}

static int device_adc_config_get(env_t *env, hw_config_t *conf, int which, val_t *val)
{
    hw_config_adc_t *adc = (hw_config_adc_t *) conf;

    switch (which) {
    case DEVICE_ADC_CONF_CHANNELS: device_config_get_sequence(env, val, adc->chn_num, adc->chn_seq);   break;
    case DEVICE_ADC_CONF_INTERVAL: val_set_number(val, adc->interval); break;
    default:                       return -CUPKEE_EINVAL;
    }

    return CUPKEE_OK;
}

static int device_adc_config_set(env_t *env, hw_config_t *conf, int which, val_t *val)
{
    hw_config_adc_t *adc = (hw_config_adc_t *) conf;

    (void) env;

    switch (which) {
    case DEVICE_ADC_CONF_CHANNELS: return device_config_set_sequence(val, HW_CHN_MAX_ADC, &adc->chn_num, adc->chn_seq);
    case DEVICE_ADC_CONF_INTERVAL: return device_config_set_uint16(val, &adc->interval);
    default:                       return -CUPKEE_EINVAL;
    }
}

static int device_timer_config_get(env_t *env, hw_config_t *conf, int which, val_t *val)
{
    hw_config_timer_t *timer = (hw_config_timer_t *) conf;

    switch (which) {
    case DEVICE_TIMER_CONF_CHANNELS: device_config_get_sequence(env, val, timer->chn_num, timer->chn_seq);   break;
    case DEVICE_TIMER_CONF_POLARITY: device_config_get_option(val, timer->polarity, DEVICE_OPT_POLARITY_MAX, device_opt_polarity); break;
    default:                       return -CUPKEE_EINVAL;
    }
    return CUPKEE_OK;
}

static int device_timer_config_set(env_t *env, hw_config_t *conf, int which, val_t *val)
{
    hw_config_timer_t *timer = (hw_config_timer_t *) conf;

    (void) env;
    switch (which) {
    case DEVICE_TIMER_CONF_CHANNELS: return device_config_set_sequence(val, HW_CHN_MAX_TIMER, &timer->chn_num, timer->chn_seq);
    case DEVICE_TIMER_CONF_POLARITY: return device_config_set_option(val, &timer->polarity, DEVICE_OPT_POLARITY_MAX, device_opt_polarity); break;
    default:                       return -CUPKEE_EINVAL;
    }
}

static int device_pwm_config_get(env_t *env, hw_config_t *conf, int which, val_t *val)
{
    hw_config_pwm_t *pwm = (hw_config_pwm_t *) conf;

    switch (which) {
    case DEVICE_PWM_CONF_CHANNELS: device_config_get_sequence(env, val, pwm->chn_num, pwm->chn_seq);   break;
    case DEVICE_PWM_CONF_POLARITY: device_config_get_option(val, pwm->polarity, DEVICE_OPT_POLARITY_MAX, device_opt_polarity); break;
    case DEVICE_PWM_CONF_PERIOD:   val_set_number(val, pwm->period);   break;
    default:                       return -CUPKEE_EINVAL;
    }
    return CUPKEE_OK;
}

static int device_pwm_config_set(env_t *env, hw_config_t *conf, int which, val_t *val)
{
    hw_config_pwm_t *pwm = (hw_config_pwm_t *) conf;

    (void) env;
    switch (which) {
    case DEVICE_PWM_CONF_CHANNELS: return device_config_set_sequence(val, HW_CHN_MAX_PWM, &pwm->chn_num, pwm->chn_seq);
    case DEVICE_PWM_CONF_POLARITY: return device_config_set_option(val, &pwm->polarity, DEVICE_OPT_POLARITY_MAX, device_opt_polarity); break;
    case DEVICE_PWM_CONF_PERIOD:   return device_config_set_uint16(val, &pwm->period);   break;
    default:                       return -CUPKEE_EINVAL;
    }
}

static int device_pulse_config_get(env_t *env, hw_config_t *conf, int which, val_t *val)
{
    hw_config_pulse_t *pulse= (hw_config_pulse_t *) conf;

    switch (which) {
    case DEVICE_PULSE_CONF_CHANNELS: device_config_get_sequence(env, val, pulse->chn_num, pulse->chn_seq);   break;
    case DEVICE_PULSE_CONF_POLARITY: device_config_get_option(val, pulse->polarity, DEVICE_OPT_POLARITY_MAX - 1, device_opt_polarity); break;
    default:                         return -CUPKEE_EINVAL;
    }
    return CUPKEE_OK;
}

static int device_pulse_config_set(env_t *env, hw_config_t *conf, int which, val_t *val)
{
    hw_config_pulse_t *pulse= (hw_config_pulse_t *) conf;

    (void) env;

    switch (which) {
    case DEVICE_PULSE_CONF_CHANNELS: return device_config_set_sequence(val, HW_CHN_MAX_PULSE, &pulse->chn_num, pulse->chn_seq);
    case DEVICE_PULSE_CONF_POLARITY: return device_config_set_option(val, &pulse->polarity, DEVICE_OPT_POLARITY_MAX - 1, device_opt_polarity);
    default:                         return -CUPKEE_EINVAL;
    }
    return CUPKEE_OK;
}

static int device_counter_config_get(env_t *env, hw_config_t *conf, int which, val_t *val)
{
    hw_config_counter_t *counter = (hw_config_counter_t *) conf;

    switch (which) {
    case DEVICE_COUNTER_CONF_CHANNELS: device_config_get_sequence(env, val, counter->chn_num, counter->chn_seq);   break;
    case DEVICE_COUNTER_CONF_POLARITY: device_config_get_option(val, counter->polarity, DEVICE_OPT_POLARITY_MAX, device_opt_polarity); break;
    case DEVICE_COUNTER_CONF_PERIOD:   val_set_number(val, counter->period);   break;
    default:                       return -CUPKEE_EINVAL;
    }
    return CUPKEE_OK;
}

static int device_counter_config_set(env_t *env, hw_config_t *conf, int which, val_t *val)
{
    hw_config_counter_t *counter = (hw_config_counter_t *) conf;

    (void) env;
    switch (which) {
    case DEVICE_COUNTER_CONF_CHANNELS: return device_config_set_sequence(val, HW_CHN_MAX_COUNTER, &counter->chn_num, counter->chn_seq);
    case DEVICE_COUNTER_CONF_POLARITY: return device_config_set_option(val, &counter->polarity, DEVICE_OPT_POLARITY_MAX, device_opt_polarity); break;
    case DEVICE_COUNTER_CONF_PERIOD:   return device_config_set_uint16(val, &counter->period);   break;
    default:                       return -CUPKEE_EINVAL;
    }
}

static int device_uart_config_get(env_t *env, hw_config_t *conf, int which, val_t *val)
{
    hw_config_uart_t *uart = (hw_config_uart_t *) conf;

    (void) env;

    switch (which) {
    case DEVICE_UART_CONF_BAUDRATE: val_set_number(val, uart->baudrate);  break;
    case DEVICE_UART_CONF_DATABITS: val_set_number(val, uart->data_bits); break;
    case DEVICE_UART_CONF_STOPBITS: device_config_get_option(val, uart->stop_bits,
                                            DEVICE_OPT_STOPBITS_MAX, device_opt_stopbits); break;
    case DEVICE_UART_CONF_PARITY:   device_config_get_option(val, uart->parity,
                                            DEVICE_OPT_PARITY_MAX, device_opt_parity); break;
    default: return -CUPKEE_EINVAL;
    }

    return CUPKEE_OK;
}

static int device_uart_config_set(env_t *env, hw_config_t *conf, int which, val_t *val)
{
    hw_config_uart_t *uart = (hw_config_uart_t *) conf;

    (void) env;

    switch (which) {
    case DEVICE_UART_CONF_BAUDRATE: return device_config_set_uint32(val, &uart->baudrate);  break;
    case DEVICE_UART_CONF_DATABITS: return device_config_set_uint8(val, &uart->data_bits); break;
    case DEVICE_UART_CONF_STOPBITS:
        return device_config_set_option(val, &uart->stop_bits, DEVICE_OPT_STOPBITS_MAX, device_opt_stopbits);
    case DEVICE_UART_CONF_PARITY:
        return device_config_set_option(val, &uart->parity, DEVICE_OPT_PARITY_MAX, device_opt_parity);
    default: break;
    }

    return -CUPKEE_EINVAL;
}

static int device_i2c_config_get(env_t *env, hw_config_t *conf, int which, val_t *val)
{
    hw_config_i2c_t *i2c= (hw_config_i2c_t *) conf;

    (void) env;

    switch (which) {
    case DEVICE_I2C_CONF_SPEED:   val_set_number(val, i2c->speed); break;
    case DEVICE_I2C_CONF_ADDRESS: val_set_number(val, i2c->addr);  break;
    default: return -CUPKEE_EINVAL;
    }

    return CUPKEE_OK;
}

static int device_i2c_config_set(env_t *env, hw_config_t *conf, int which, val_t *val)
{
    hw_config_i2c_t *i2c= (hw_config_i2c_t *) conf;

    (void) env;

    switch (which) {
    case DEVICE_I2C_CONF_SPEED: return device_config_set_uint32(val, &i2c->speed);  break;
    case DEVICE_I2C_CONF_ADDRESS: return device_config_set_uint8(val, &i2c->addr); break;
    default: break;
    }

    return -CUPKEE_EINVAL;
}

static int device_config_set(cupkee_device_t *dev, env_t *env, int index, val_t *val)
{
    if (index >= 0 && index < dev->desc->conf_num) {
        switch (dev->desc->type) {
        case DEVICE_TYPE_PIN:      return device_pin_config_set     (env, &dev->config, index, val);
        case DEVICE_TYPE_ADC:      return device_adc_config_set     (env, &dev->config, index, val);
        case DEVICE_TYPE_DAC:      break;
        case DEVICE_TYPE_PWM:      return device_pwm_config_set     (env, &dev->config, index, val);
        case DEVICE_TYPE_PULSE:    return device_pulse_config_set   (env, &dev->config, index, val);
        case DEVICE_TYPE_TIMER:    return device_timer_config_set   (env, &dev->config, index, val);
        case DEVICE_TYPE_COUNTER:  return device_counter_config_set (env, &dev->config, index, val);
        case DEVICE_TYPE_UART:     return device_uart_config_set    (env, &dev->config, index, val);
        case DEVICE_TYPE_USART:
        case DEVICE_TYPE_SPI:      return -CUPKEE_EIMPLEMENT;
        case DEVICE_TYPE_I2C:      return device_i2c_config_set     (env, &dev->config, index, val);
        case DEVICE_TYPE_USB_CDC:
        default: break;
        }
        return -CUPKEE_EIMPLEMENT;
    }

    return -CUPKEE_EINVAL;
}

static int device_config_get(cupkee_device_t *dev, env_t *env, int index, val_t *val)
{
    if (index >= 0 && index < dev->desc->conf_num) {
        switch (dev->desc->type) {
        case DEVICE_TYPE_PIN:      return device_pin_config_get     (env, &dev->config, index, val);
        case DEVICE_TYPE_ADC:      return device_adc_config_get     (env, &dev->config, index, val);
        case DEVICE_TYPE_DAC:      return -CUPKEE_EIMPLEMENT;
        case DEVICE_TYPE_PWM:      return device_pwm_config_get     (env, &dev->config, index, val);
        case DEVICE_TYPE_PULSE:    return device_pulse_config_get   (env, &dev->config, index, val);
        case DEVICE_TYPE_TIMER:    return device_timer_config_get   (env, &dev->config, index, val);
        case DEVICE_TYPE_COUNTER:  return device_counter_config_get (env, &dev->config, index, val);
        case DEVICE_TYPE_UART:     return device_uart_config_get    (env, &dev->config, index, val);
        case DEVICE_TYPE_USART:
        case DEVICE_TYPE_SPI:      return -CUPKEE_EIMPLEMENT;
        case DEVICE_TYPE_I2C:      return device_i2c_config_get     (env, &dev->config, index, val);
        case DEVICE_TYPE_USB_CDC:
        default: return -CUPKEE_EIMPLEMENT;
        }
    }

    return -CUPKEE_EINVAL;
}

val_t cupkee_device_config_set_one(cupkee_device_t *dev, env_t *env, val_t *which, val_t *val)
{
    int index = shell_val_id(which, dev->desc->conf_num, dev->desc->conf_names);

    return device_config_set(dev, env, index, val) == CUPKEE_OK ? VAL_TRUE : VAL_FALSE;
}

val_t cupkee_device_config_get_one(cupkee_device_t *dev, env_t *env, val_t *which)
{
    int index = shell_val_id(which, dev->desc->conf_num, dev->desc->conf_names);
    val_t val;

    if (CUPKEE_OK == device_config_get(dev, env, index, &val)) {
        return val;
    } else {
        return VAL_UNDEFINED;
    }
}

int cupkee_device_config_set_all(cupkee_device_t *dev, env_t *env, val_t *settings)
{
    object_iter_t it;
    const char *key;
    val_t *val;

    if (object_iter_init(&it, settings)) {
        return -CUPKEE_EINVAL;
    }

    while (object_iter_next(&it, &key, &val)) {
        int index = shell_str_id(key, dev->desc->conf_num, dev->desc->conf_names);

        if (device_config_set(dev, env, index, val)) {
            return -CUPKEE_EINVAL;
        }
    }

    return CUPKEE_OK;
}

val_t cupkee_device_config_get_all(cupkee_device_t *dev)
{
    (void) dev;
    return VAL_UNDEFINED;
}

const cupkee_device_desc_t *cupkee_device_query_by_index(int i)
{
    unsigned index = i;
    if (index >= sizeof(device_entrys) / sizeof(cupkee_device_desc_t *)) {
        return NULL;
    }
    return device_entrys[index];
}

const cupkee_device_desc_t *cupkee_device_query_by_name(const char *name)
{
    unsigned i;

    for (i = 0; device_entrys[i]; i++) {
        if (strcmp(name, device_entrys[i]->name) == 0) {
            return device_entrys[i];
        }
    }
    return NULL;
}

const cupkee_device_desc_t *cupkee_device_query_by_type(uint16_t type)
{
    unsigned i;

    for (i = 0; device_entrys[i]; i++) {
        if (device_entrys[i]->type == type) {
            return device_entrys[i];
        }
    }
    return NULL;
}

