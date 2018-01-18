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

#ifndef __CUPKEE_SHELL_DEVICE_INC__
#define __CUPKEE_SHELL_DEVICE_INC__

#define DEVICE_PIN_CONF_NUM             0
#define DEVICE_PIN_CONF_START           1
#define DEVICE_PIN_CONF_DIR             2
#define DEVICE_PIN_CONF_MAX             3

#define DEVICE_UART_CONF_BAUDRATE       0
#define DEVICE_UART_CONF_DATABITS       1
#define DEVICE_UART_CONF_STOPBITS       2
#define DEVICE_UART_CONF_PARITY         3
#define DEVICE_UART_CONF_MAX            4

#define DEVICE_ADC_CONF_CHANNELS        0
#define DEVICE_ADC_CONF_INTERVAL        1
#define DEVICE_ADC_CONF_MAX             2

#define DEVICE_PWM_CONF_CHANNELS        0
#define DEVICE_PWM_CONF_POLARITY        1
#define DEVICE_PWM_CONF_PERIOD          2
#define DEVICE_PWM_CONF_MAX             3

#define DEVICE_PULSE_CONF_CHANNELS      0
#define DEVICE_PULSE_CONF_POLARITY      1
#define DEVICE_PULSE_CONF_MAX           2

#define DEVICE_TIMER_CONF_CHANNELS      0
#define DEVICE_TIMER_CONF_POLARITY      1
#define DEVICE_TIMER_CONF_MAX           2

#define DEVICE_COUNTER_CONF_CHANNELS    0
#define DEVICE_COUNTER_CONF_POLARITY    1
#define DEVICE_COUNTER_CONF_PERIOD      2
#define DEVICE_COUNTER_CONF_MAX         3

#define DEVICE_I2C_CONF_SPEED           0
#define DEVICE_I2C_CONF_ADDRESS         1

const cupkee_device_desc_t *cupkee_device_query_by_name(const char *name);
const cupkee_device_desc_t *cupkee_device_query_by_type(uint16_t type);
const cupkee_device_desc_t *cupkee_device_query_by_index(int i);

val_t cupkee_device_config_set_one(cupkee_device_t *dev, env_t *env, val_t *which, val_t *val);
val_t cupkee_device_config_get_one(cupkee_device_t *dev, env_t *env, val_t *which);
int   cupkee_device_config_set_all(cupkee_device_t *dev, env_t *env, val_t *settings);
val_t cupkee_device_config_get_all(cupkee_device_t *dev);

void cupkee_shell_init_device(void);

#endif /* __CUPKEE_SHELL_DEVICE_INC__ */

