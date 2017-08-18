/*
MIT License

This file is part of cupkee project.

Copyright (c) 2016,2017 Lixing Ding <ding.lixing@gmail.com>

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

#ifndef __CUPKEE_BSP_INC__
#define __CUPKEE_BSP_INC__

#include <stdint.h>

/****************************************************************/
/* hardware defines                                             */
/****************************************************************/
#define SYSTEM_TICKS_PRE_SEC                1000
#define SYSTEM_STACK_SIZE                   (8 * 1024)

#define HW_STORAGE_BANK_BIN                 0
#define HW_STORAGE_BANK_BIN2                1
#define HW_STORAGE_BANK_APP                 2
#define HW_STORAGE_BANK_CFG                 3

#define HW_PIN_MAX                          16
#define HW_CHN_MAX_ADC                      4
#define HW_CHN_MAX_PWM                      4
#define HW_CHN_MAX_PULSE                    4
#define HW_CHN_MAX_TIMER                    2
#define HW_CHN_MAX_COUNTER                  4

#define HW_BUF_SIZE                         32

/****************************************************************/
/* system define                                                */
/****************************************************************/
#define DEVICE_ID_INVALID                   0xff

#define DEVICE_EVENT_ERR                    0
#define DEVICE_EVENT_DATA                   1
#define DEVICE_EVENT_DRAIN                  2
#define DEVICE_EVENT_READY                  3
#define DEVICE_EVENT_MAX                    4

#define DEVICE_CATEGORY_MAP                 0
#define DEVICE_CATEGORY_STREAM              1
#define DEVICE_CATEGORY_BLOCK               2
#define DEVICE_CATEGORY_MAX                 3

enum DEVICE_TYPE {
    DEVICE_TYPE_PIN = 0,
    DEVICE_TYPE_ADC,
    DEVICE_TYPE_DAC,
    DEVICE_TYPE_PWM,
    DEVICE_TYPE_PULSE,
    DEVICE_TYPE_TIMER,
    DEVICE_TYPE_COUNTER,
    DEVICE_TYPE_UART,
    DEVICE_TYPE_I2C,
    DEVICE_TYPE_SPI,
    DEVICE_TYPE_USART,
    DEVICE_TYPE_USB_CDC,
    DEVICE_TYPE_DUMMY
};

#define DEVICE_OPT_DIR_IN                   0   // default
#define DEVICE_OPT_DIR_OUT                  1
#define DEVICE_OPT_DIR_DUAL                 2
#define DEVICE_OPT_DIR_MAX                  3

#define DEVICE_OPT_POLARITY_POSITIVE        0   // default
#define DEVICE_OPT_POLARITY_NEGATIVE        1
#define DEVICE_OPT_POLARITY_EDGE            2
#define DEVICE_OPT_POLARITY_MAX             3

#define DEVICE_OPT_PARITY_NONE              0   // default
#define DEVICE_OPT_PARITY_ODD               1
#define DEVICE_OPT_PARITY_EVEN              2
#define DEVICE_OPT_PARITY_MAX               3

#define DEVICE_OPT_STOPBITS_1               0   // default
#define DEVICE_OPT_STOPBITS_2               1
#define DEVICE_OPT_STOPBITS_0_5             2
#define DEVICE_OPT_STOPBITS_1_5             3
#define DEVICE_OPT_STOPBITS_MAX             4

typedef struct hw_info_t {
    int ram_sz;
    int rom_sz;
    void *ram_base;
    void *rom_base;
    uint32_t sys_freq;
} hw_info_t;

typedef struct hw_config_pin_t {
    uint8_t num;
    uint8_t start;
    uint8_t dir;         // DEVICE_OPT_DIR
} hw_config_pin_t;

typedef struct hw_config_adc_t {
    uint16_t interval;
    uint8_t chn_num;
    uint8_t chn_seq[HW_CHN_MAX_ADC];
} hw_config_adc_t;

typedef struct hw_config_pwm_t {
    uint16_t period;     // ms
    uint8_t  polarity;   // DEVICE_OPT_POLARITY
    uint8_t  chn_num;
    uint8_t  chn_seq[HW_CHN_MAX_PWM];
} hw_config_pwm_t;

typedef struct hw_config_pulse_t {
    uint8_t polarity;   // DEVICE_OPT_POLARITY
    uint8_t chn_num;
    uint8_t chn_seq[HW_CHN_MAX_PULSE];
} hw_config_pulse_t;

typedef struct hw_config_timer_t {
    uint8_t polarity;   // DEVICE_OPT_POLARITY
    uint8_t chn_num;
    uint8_t chn_seq[HW_CHN_MAX_TIMER];
} hw_config_timer_t;

typedef struct hw_config_counter_t {
    uint16_t period;     // us
    uint8_t  polarity;   // DEVICE_OPT_POLARITY
    uint8_t  chn_num;
    uint8_t  chn_seq[HW_CHN_MAX_COUNTER];
} hw_config_counter_t;

typedef struct hw_config_uart_t {
    uint32_t baudrate;
    uint8_t  data_bits;
    uint8_t  stop_bits;  // DEVICE_OPT_STOPBITS
    uint8_t  parity;     // DEVICE_OPT_PARITY
} hw_config_uart_t;

typedef struct hw_config_i2c_t {
    uint8_t  addr;       // self-address
    uint8_t  mode;       // master or slave or multi-master
    uint8_t  order;      // MSB or LSB
    uint32_t speed;
} hw_config_i2c_t;

typedef struct hw_config_spi_t {
    uint8_t  mode;       // master(default) or slave or multi-master
    uint8_t  order;      // MSB(default) or LSB
    uint8_t  dir;        // readonly or writonly or duplex(default)
    uint32_t speed;      // 1,200,000 default
} hw_config_spi_t;


/****************************************************************/
/* system utils                                                 */
/****************************************************************/

/****************************************************************/
/* hardware implements                                          */
/****************************************************************/
void hw_setup(void);
void _hw_reset(void);

void hw_poll(void);
void hw_halt(void);

void hw_enter_critical(uint32_t *state);
void hw_exit_critical(uint32_t state);

void hw_info_get(hw_info_t *);

void *hw_malloc(size_t size, size_t align);

void  *hw_boot_memory_alloc(size_t size, size_t align);
size_t hw_boot_memory_size(void);

size_t hw_memory_left(void);
size_t hw_malloc_all(void **p, size_t align);

uint32_t hw_storage_size(int bank);
int hw_storage_erase (int bank);
int hw_storage_update(int bank, uint32_t offset, const uint8_t *data, int len);
int hw_storage_finish(int bank, uint32_t end);
uint32_t hw_storage_data_length(int bank);
const char *hw_storage_data_map(int bank);

/* DEBUG LED */
int  hw_led_map(int port, int pin);
void hw_led_set(void);
void hw_led_clear(void);
void hw_led_toggle(void);

/* GPIO */
enum HW_DIR {
    HW_DIR_IN,
    HW_DIR_OUT,
    HW_DIR_DUPLEX
};

int  hw_pin_map(int id, uint8_t port, uint8_t pin, uint8_t dir);
int  hw_pin_get(int id);
void hw_pin_set(int id, int v);
void hw_pin_toggle(int id);


/* TIMER */
int  hw_timer_alloc(void);
void hw_timer_release(int inst);

int hw_timer_start(int inst, int id, int us);
int hw_timer_stop(int inst);
int hw_timer_update(int inst, int us);
int hw_timer_duration_get(int inst);

/* DEVICE */
int   hw_device_setup(void);

#endif /* __CUPKEE_BSP_INC__ */

