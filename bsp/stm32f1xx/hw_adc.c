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

#define ADC_SETUP       0
#define ADC_START       1
#define ADC_CONVERT     2
#define ADC_BUSY        3      // work in process
#define ADC_RESUME      4
#define ADC_INVALID     0xffff

static uint8_t adc_state;
static uint8_t adc_num;
static uint8_t adc_cur;
static uint8_t adc_seq[16];
static uint16_t adc_chn_data[16];
static uint16_t adc_chn_state;

static int hw_adc_map(uint8_t bank, uint8_t port)
{
    if (bank == 0 && port < 8) {
        return port;
    } else
    if (bank == 1 && port < 2) {
        return port + 8;
    } else
    if (bank == 2 && port < 6) {
        return port + 10;
    }
    return -1;
}

static uint8_t hw_adc_chn_scan(uint8_t *seq)
{
    int i, n;

    for (i = 0, n = 0; i < 16; ++i) {
        if (adc_chn_state & (1 << i)) {
            seq[n++] = i;
        }
    }

    return n;
}

static void hw_adc_setup(void)
{
    int i;

    rcc_periph_clock_enable(RCC_ADC1);

    adc_power_off(ADC1);
    adc_disable_scan_mode(ADC1);
    adc_disable_external_trigger_regular(ADC1);
    adc_set_right_aligned(ADC1);
    adc_set_single_conversion_mode(ADC1);
    adc_set_sample_time_on_all_channels(ADC1, ADC_SMPR_SMP_28DOT5CYC);

    adc_power_on(ADC1);

    for (i = 0; i < 16; ++i) {
        adc_chn_data[i] = 0;
    }

    adc_state = ADC_SETUP;
}

static void hw_adc_resume(void)
{
    adc_power_off(ADC1);
    adc_disable_scan_mode(ADC1);
    adc_disable_external_trigger_regular(ADC1);
    adc_set_right_aligned(ADC1);
    adc_set_single_conversion_mode(ADC1);
    adc_set_sample_time_on_all_channels(ADC1, ADC_SMPR_SMP_28DOT5CYC);

    adc_power_on(ADC1);
    adc_state = ADC_RESUME;
}

static void hw_adc_reset(void)
{
    adc_power_off(ADC1);
    rcc_periph_clock_disable(RCC_ADC1);

    adc_state = ADC_SETUP;
}

void hw_setup_adc(void)
{
    int i;

    adc_state = ADC_SETUP;
    adc_chn_state = 0;
    for (i = 0; i < 16; ++i) {
        adc_chn_data[i] = 0;
    }
}

int hw_adc_start(uint8_t bank, uint8_t port)
{
    int chn = hw_adc_map(bank, port);

    if (chn < 0) {
        return -CUPKEE_EINVAL;
    }

    if (!adc_chn_state) {
        hw_adc_setup();
    }

    if (!(adc_chn_state & (1 << chn))) {
        hw_gpio_setup(port, 1 << port, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, 0);
        adc_chn_state |= 1 << chn;
    } else {
        // Already start
    }

    return 0;
}

int hw_adc_stop(uint8_t bank, uint8_t port)
{
    int chn = hw_adc_map(bank, port);

    if (chn < 0) {
        return -CUPKEE_EINVAL;
    }

    if (adc_chn_state & (1 << chn)) {
        hw_gpio_setup(port, 1 << port, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, 0);
        adc_chn_state &= ~(1 << chn);
    } else {
        // Already start
    }

    if (adc_chn_state == 0) {
        hw_adc_reset();
    }

    return 0;
}

int hw_adc_get(uint8_t bank, uint8_t port, float *v)
{
    int x = hw_adc_map(bank, port);

    if (x < 0) {
        return -CUPKEE_EINVAL;
    }

    if (0 == (adc_chn_state & (1 << x))) {
        return -CUPKEE_EINVAL;
    }

    if (v) {
        *v = adc_chn_data[x] * (1.0 / 0xFFF);
    }
    return 0;
}

void hw_poll_adc(void)
{
    if (adc_chn_state) {
        switch(adc_state) {
        case ADC_SETUP:
            adc_reset_calibration(ADC1);
            adc_calibrate(ADC1);
            adc_state = ADC_START;
            break;
        case ADC_START:
            adc_num = hw_adc_chn_scan(adc_seq);
            adc_set_regular_sequence(ADC1, adc_num, adc_seq);
            adc_cur = 0;
            adc_state = ADC_CONVERT;
            break;
        case ADC_CONVERT:
            adc_start_conversion_direct(ADC1);
            adc_state = ADC_BUSY;
            break;
        case ADC_BUSY:
            if (adc_eoc(ADC1)) {
                uint8_t x = adc_seq[adc_cur++];

                if (x < 16) {
                    adc_chn_data[x] = adc_read_regular(ADC1);
                    if (adc_cur >= adc_num) {
                        hw_adc_resume();
                    } else {
                        adc_state = ADC_CONVERT;
                    }
                } else {
                    hw_adc_reset();
                }
            }
            break;
        case ADC_RESUME:
            adc_state = ADC_SETUP;
            break;
        default:
            // Todo: error process here
            hw_adc_reset();
            break;
        }
    }
}
