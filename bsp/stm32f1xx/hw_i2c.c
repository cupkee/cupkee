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


#define I2C_REG_BASE(inst)                          ((inst) == 0 ? I2C1 : I2C2)

#define I2C_EVENT_MASTER_MODE_SELECT                ((uint32_t)0x00030001) //           SR2_BUSY | SR2_MASTER | SR1_SB
#define I2C_EVENT_STOP                              ((uint32_t)0x00000010) //                                   SR1_STOPF

// EV6
#define I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECT    ((uint32_t)0x00070082) // SR2_TRA | SR2_BUSY | SR2_MASTER | SR1_TxE | SR1_ADDR
#define I2C_EVENT_MASTER_RECEIVER_MODE_SELECT       ((uint32_t)0x00030002) //           SR2_BUSY | SR2_MASTER |           SR1_ADDR

// EV7
#define I2C_EVENT_MASTER_RECEIVED                   ((uint32_t)0x00030040) //           SR2_BUSY | SR2_MASTER | SR1_RxNE
#define I2C_EVENT_MASTER_RECEIVE_HOLD               ((uint32_t)0x00030044) //           SR2_BUSY | SR2_MASTER | SR1_RxNE | SR1_BTF

// EV8
#define I2C_EVENT_MASTER_TRANSMITTING               ((uint32_t)0x00070080) // SR2_TRA | SR2_BUSY | SR2_MASTER | SR1_TxE
// EV8_2
#define I2C_EVENT_MASTER_TRANSMITTED                ((uint32_t)0x00070084) // SR2_TRA | SR2_BUSY | SR2_MASTER | SR1_TxE | SR1_BTF

enum {
    I2C_STATE_IDLE = 0,
    I2C_STATE_ERROR,
    I2C_STATE_WAIT_NBUSY,
    I2C_STATE_WAIT_START,
    I2C_STATE_WAIT_RADDR,
    I2C_STATE_WAIT_WADDR,
    I2C_STATE_SEND_DATA,
    I2C_STATE_WAIT_DATA_1,
    I2C_STATE_WAIT_DATA_2,
    I2C_STATE_WAIT_DATA_N,
    I2C_STATE_STOP,
};

enum {
    I2C_PROP_SLAVE = 0,
};

typedef struct hw_i2c_t {
    uint8_t dev_id;
    uint8_t state;

    uint8_t slave_addr;
    uint8_t trans_time;
    uint8_t trans_size;
    uint8_t trans_pos;

    void   *req_buf;
    void   *rcv_buf;
} hw_i2c_t;

#if 0
static uint8_t use_map;

static hw_i2c_t i2c_controls[HW_INSTANCES_I2C];

static inline int hw_i2c_match_event(uint32_t i2c, uint32_t event)
{
    uint32_t status = I2C_SR1(i2c);

    status = status | (I2C_SR2(i2c) << 16);

    return (status & event) == event;
}

static inline hw_i2c_t *hw_i2c_control(int instance)
{
    if (instance < HW_INSTANCES_I2C && (use_map & (1 << instance))) {
        return &i2c_controls[instance];
    } else {
        return NULL;
    }
}

static inline int hw_i2c_setup_pin(int instance)
{
    uint16_t pins;

    if (instance == 0) {
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

static inline void hw_i2c_reset_pin(int instance)
{
    uint16_t pins;

    if (instance == 0) {
        pins = GPIO6 | GPIO7;
    } else {
        pins = GPIO10 | GPIO11;
    }

    hw_gpio_release(1, pins);
}

static int request_read(hw_i2c_t *control, size_t n)
{
    uint8_t req[2];

    if (cupkee_buffer_space(control->req_buf) < 2 ||
        cupkee_buffer_space(control->rcv_buf) < n) {
        return 0;
    }

    req[0] = 1;
    req[1] = n;

    return cupkee_buffer_give(control->req_buf, 2, req);
}

static int request_write(hw_i2c_t *control, size_t n, const void *data)
{
    uint8_t req[2];

    if (cupkee_buffer_space(control->req_buf) < 2 + n) {
        return 0;
    }

    req[0] = 1;
    req[1] = n;

    cupkee_buffer_give(control->req_buf, 2, req);
    return cupkee_buffer_give(control->req_buf, n, data);
}

static int request_go_trans(hw_i2c_t *control)
{
    uint8_t req[2];
    int ret = cupkee_buffer_take(control->req_buf, 2, req);

    if (ret == 2) {
        if (req[0]) {
            control->slave_addr |= 1;  // read
        } else {
            control->slave_addr &= ~1; // write
        }
        control->trans_size = req[1];
        control->trans_time = 0;
        control->trans_pos = 0;

        return 1;
    } else {
        return 0;
    }
}

static void do_error_timeout(uint32_t i2c, hw_i2c_t *control)
{
    if (I2C_SR2(i2c) & I2C_SR2_MSL) {
        I2C_CR1(i2c) |= I2C_CR1_STOP;
    }

    cupkee_device_set_error(control->dev_id, CUPKEE_ETIMEOUT);

    control->state = I2C_STATE_IDLE;

    //Todo: try next req OR clean req queue
}

static void do_try_start(uint32_t i2c, hw_i2c_t *control)
{
    if (control->state == I2C_STATE_IDLE) {
        if (request_go_trans(control)) {
            if (I2C_SR2(i2c) & I2C_SR2_BUSY) {
                control->state = I2C_STATE_WAIT_NBUSY;
            } else {
                I2C_CR1(i2c) |= I2C_CR1_START;
                control->state = I2C_STATE_WAIT_START;
            }
        }
    }
}

static void do_wait_nbusy(uint32_t i2c, hw_i2c_t *control)
{
    if (!(I2C_SR2(i2c) & I2C_SR2_BUSY)) {
        I2C_CR1(i2c) |= I2C_CR1_START;
        control->state = I2C_STATE_WAIT_START;
    }
}

static void do_wait_start(uint32_t i2c, hw_i2c_t *control)
{
    if (hw_i2c_match_event(i2c, I2C_EVENT_MASTER_MODE_SELECT)) {
        I2C_DR(i2c) = control->slave_addr;
        if (control->slave_addr & 1) {
            control->state = I2C_STATE_WAIT_RADDR;
        } else {
            control->state = I2C_STATE_WAIT_WADDR;
        }
        control->trans_pos = 0;
    }
}

static void do_wait_raddr(uint32_t i2c, hw_i2c_t *control)
{
    if (I2C_SR1(i2c) & I2C_SR1_ADDR) {
        uint32_t irq_state;

        switch (control->trans_size) {
        case 1: // one byte read
            I2C_CR1(i2c) &= ~I2C_CR1_ACK; // Nack for DataN

            hw_enter_critical(&irq_state);
            hw_i2c_match_event(i2c, I2C_EVENT_MASTER_RECEIVER_MODE_SELECT); // Clear addr
            I2C_CR1(i2c) |= I2C_CR1_STOP;
            hw_exit_critical(irq_state);
            control->state = I2C_STATE_WAIT_DATA_1;

            break;
        case 2: // two bytes read
            I2C_CR1(i2c) |= I2C_CR1_POS; // Trigger Nack at next shift complete

            hw_enter_critical(&irq_state);
            hw_i2c_match_event(i2c, I2C_EVENT_MASTER_RECEIVER_MODE_SELECT); // Clear addr
            I2C_CR1(i2c) &= ~I2C_CR1_ACK; // Nack
            hw_exit_critical(irq_state);
            control->state = I2C_STATE_WAIT_DATA_2;

            break;
        default: // n bytes read
            hw_i2c_match_event(i2c, I2C_EVENT_MASTER_RECEIVER_MODE_SELECT); // Clear addr
            control->state = I2C_STATE_WAIT_DATA_N;
            break;
        }
    }
}

static void do_wait_data_1(uint32_t i2c, hw_i2c_t *control)
{
    if(hw_i2c_match_event(i2c, I2C_EVENT_MASTER_RECEIVED)) {
        control->trans_pos++;
        cupkee_buffer_push(control->rcv_buf, I2C_DR(i2c));
        control->state = I2C_STATE_STOP;
    }
}

static void do_wait_data_2(uint32_t i2c, hw_i2c_t *control)
{
    if(hw_i2c_match_event(i2c, I2C_EVENT_MASTER_RECEIVE_HOLD)) {
        uint32_t irq_state;

        control->trans_pos += 2;

        hw_enter_critical(&irq_state);
        I2C_CR1(i2c) |= I2C_CR1_STOP;
        cupkee_buffer_push(control->rcv_buf, I2C_DR(i2c));
        hw_exit_critical(irq_state);
        cupkee_buffer_push(control->rcv_buf, I2C_DR(i2c));

        control->state = I2C_STATE_STOP;
    }
}

static void do_wait_data_n(uint32_t i2c, hw_i2c_t *control)
{
    if(hw_i2c_match_event(i2c, I2C_EVENT_MASTER_RECEIVE_HOLD)) {
        unsigned lft = control->trans_size - control->trans_pos;
        if (lft > 3) {
            cupkee_buffer_push(control->rcv_buf, I2C_DR(i2c));
            control->trans_pos++;
        } else
        if (lft == 3) {
            I2C_CR1(i2c) &= ~I2C_CR1_ACK; // Nack to last byte
            cupkee_buffer_push(control->rcv_buf, I2C_DR(i2c));
            control->trans_pos++;
        } else {
            uint32_t irq_state;

            hw_enter_critical(&irq_state);
            I2C_CR1(i2c) |= I2C_CR1_STOP;
            cupkee_buffer_push(control->rcv_buf, I2C_DR(i2c));
            hw_exit_critical(irq_state);
            cupkee_buffer_push(control->rcv_buf, I2C_DR(i2c));

            control->trans_pos += 2;
            control->state = I2C_STATE_STOP;
        }
    }
}

static void do_wait_waddr(uint32_t i2c, hw_i2c_t *control)
{
    if (hw_i2c_match_event(i2c, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECT)) {
        uint8_t data;

        cupkee_buffer_shift(control->req_buf, &data);
        I2C_DR(i2c) = data;

        control->trans_pos = 1;
        control->state = I2C_STATE_SEND_DATA;
    }
}

static void do_send_data(uint32_t i2c, hw_i2c_t *control)
{
    if (hw_i2c_match_event(i2c, I2C_EVENT_MASTER_TRANSMITTING)) {
        if (control->trans_pos < control->trans_size) {
            uint8_t data;

            cupkee_buffer_shift(control->req_buf, &data);
            I2C_DR(i2c) = data;
            control->trans_pos++;
        } else {
            I2C_CR1(i2c) |= I2C_CR1_STOP;
            control->state = I2C_STATE_STOP;
        }
    }
}

static void do_stop(uint32_t i2c, hw_i2c_t *control)
{
    I2C_CR1(i2c) |= I2C_CR1_ACK;
    control->state = I2C_STATE_IDLE;

    if (control->slave_addr & 1) {
        cupkee_event_post_device_data(control->dev_id);
    } else {
        cupkee_event_post_device_drain(control->dev_id);
    }

    do_try_start(i2c, control);
}

static void hw_i2c_reset(int instance)
{
    hw_i2c_t *control = hw_i2c_control(instance);
    uint32_t i2c = I2C_REG_BASE(instance);

    // Reset hardware
    I2C_CR1(i2c) = I2C_CR1_SWRST;

    control->state = I2C_STATE_IDLE;

    // Release pins
    hw_i2c_reset_pin(instance);

    I2C_CR1(i2c) = 0;
    I2C_CR2(i2c) = 0;
    I2C_OAR1(i2c) = 0;
    I2C_OAR2(i2c) = 0;
    I2C_TRISE(i2c) = 0;
    I2C_CCR(i2c) = 0;

    rcc_periph_clock_disable(RCC_I2C1);

    // release resource
    if (control->rcv_buf) {
        cupkee_buffer_release(control->rcv_buf);
        control->rcv_buf = NULL;
    }
    if (control->req_buf) {
        cupkee_buffer_release(control->req_buf);
        control->req_buf = NULL;
    }
}

static void hw_i2c_release(int instance)
{
    hw_i2c_reset(instance);
    hw_release_instance(instance, &use_map);
}

static int hw_i2c_setup(int instance, uint8_t dev_id, const hw_config_t *conf)
{
    hw_i2c_t *control = hw_i2c_control(instance);
    const hw_config_i2c_t *config = (const hw_config_i2c_t *) conf;
    uint32_t i2c, speed = config->speed;
    uint8_t freq;

    if (control == NULL || hw_i2c_setup_pin(instance)) {
        return -CUPKEE_ERESOURCE;
    }

    control->dev_id = dev_id;
    control->state = I2C_STATE_IDLE;
    control->slave_addr = 0;
    control->trans_time = 0;
    control->trans_size = 0;
    control->trans_pos = 0;

    if (NULL == (control->rcv_buf = cupkee_buffer_alloc(I2C_RCV_BUF_SIZE))) {
        hw_i2c_reset_pin(instance);
        return -CUPKEE_ERESOURCE;
    }

    if (NULL == (control->req_buf = cupkee_buffer_alloc(I2C_REQ_BUF_SIZE))) {
        hw_i2c_reset_pin(instance);
        cupkee_buffer_release(control->rcv_buf);
        control->rcv_buf = NULL;
        return -CUPKEE_ERESOURCE;
    }

    if (instance == 0) {
        rcc_periph_clock_enable(RCC_I2C1);
        i2c = I2C1;
    } else {
        rcc_periph_clock_enable(RCC_I2C2);
        i2c = I2C2;
    }
    I2C_CR1(i2c) = I2C_CR1_SWRST;

    if (speed > 400000) {
        speed = 400000;
    } else
    if (speed < 10000) {
        speed = 10000;
    }
    freq = SYS_PCLK / 1000000;

    I2C_CR1(i2c) = 0;
    I2C_CR2(i2c) = freq;
    I2C_OAR1(i2c) = config->addr & 0xFE;

    // Set ccr
    if (speed <= 100000) {
        uint16_t ccr = SYS_PCLK / (speed * 2);
        if (ccr < 4) {
            ccr = 4;
        }
        I2C_TRISE(i2c) = freq + 1;
        I2C_CCR(i2c) = ccr;
    } else {
        uint16_t ccr = SYS_PCLK/ (speed * 3);
        if (ccr < 1) {
            ccr = 1;
        }
        I2C_TRISE(i2c) = freq * 3 / 10 + 1;
        I2C_CCR(i2c) = I2C_CCR_FS | ccr;
    }

    //enable all interrupts
    //I2C_CR2(i2c) |= I2C_CR2_ITBUFEN | I2C_CR2_ITEVTEN | I2C_CR2_ITERREN;
    I2C_CR1(i2c) |= I2C_CR1_ACK | I2C_CR1_PE;

    return 0; // CUPKEE_OK;
}

static void hw_i2c_poll(int instance)
{
    hw_i2c_t *control = hw_i2c_control(instance);
    uint32_t i2c = I2C_REG_BASE(instance);

    switch (control->state) {
    case I2C_STATE_IDLE:
    case I2C_STATE_ERROR: break;
    case I2C_STATE_WAIT_NBUSY: do_wait_nbusy(i2c, control); break;
    case I2C_STATE_WAIT_START: do_wait_start(i2c, control); break;
    case I2C_STATE_WAIT_RADDR: do_wait_raddr(i2c, control); break;
    case I2C_STATE_WAIT_WADDR: do_wait_waddr(i2c, control); break;
    case I2C_STATE_SEND_DATA:  do_send_data(i2c, control); break;
    case I2C_STATE_WAIT_DATA_1:  do_wait_data_1(i2c, control); break;
    case I2C_STATE_WAIT_DATA_2:  do_wait_data_2(i2c, control); break;
    case I2C_STATE_WAIT_DATA_N:  do_wait_data_n(i2c, control); break;
    case I2C_STATE_STOP:  do_stop(i2c, control); break;
    }
}

static void hw_i2c_sync(int instance, uint32_t systick)
{
    hw_i2c_t *control = hw_i2c_control(instance);

    (void) systick;

    if (!control || control->state <= I2C_STATE_ERROR) {
        return;
    }

    control->trans_time++;
    if (control->trans_time > I2C_TOUT_THRESHOLD) {
        do_error_timeout(I2C_REG_BASE(instance), control);
    }
}

static int hw_i2c_read_req(int instance, size_t size)
{
    hw_i2c_t *control = hw_i2c_control(instance);

    if (!control || size < 1) {
        return -CUPKEE_EINVAL;
    }

    if (request_read(control, size) == 0) {
        return -CUPKEE_EFULL;
    }

    do_try_start(I2C_REG_BASE(instance), control);
    return 0;
}

static int hw_i2c_read(int instance, size_t n, void *buf)
{
    hw_i2c_t *control = hw_i2c_control(instance);

    if (!control) {
        return -CUPKEE_EINVAL;
    }

    return cupkee_buffer_take(control->rcv_buf, n, buf);
}

static int hw_i2c_write(int instance, size_t n, const void *data)
{
    hw_i2c_t *control = hw_i2c_control(instance);

    if (!control || n < 1) {
        return -CUPKEE_EINVAL;
    }

    if (request_write(control, n, data) == 0) {
        return -CUPKEE_EFULL;
    }

    do_try_start(I2C_REG_BASE(instance), control);
    return n;
}

#define TOUT_TRY(cond)  while (cond) { \
    if (cupkee_systicks() - begin >= I2C_TOUT_THRESHOLD) return -1; \
}

static int hw_i2c_write_sync(int instance, size_t n, const void *data)
{
    hw_i2c_t *control;
    uint32_t i2c;
    uint8_t addr;
    const uint8_t *byte = data;
    uint32_t begin = cupkee_systicks();
    unsigned i;

    if (n < 1) {
        return 0;
    }

    control = hw_i2c_control(instance);
    i2c  = I2C_REG_BASE(instance);
    addr = control->slave_addr & (~1);

    if (!control) {
        return -CUPKEE_EINVAL;
    }

    // Start
    TOUT_TRY (I2C_SR2(i2c) & I2C_SR2_BUSY);
    I2C_CR1(i2c) |= I2C_CR1_START;

    // Send address
    TOUT_TRY (!hw_i2c_match_event(i2c, I2C_EVENT_MASTER_MODE_SELECT));
    I2C_DR(i2c) = addr;

    // Send data
    for (i = 0; i < n; i++) {
        TOUT_TRY (!hw_i2c_match_event(i2c, I2C_EVENT_MASTER_TRANSMITTING));
        I2C_DR(i2c) = byte[i];
    }
    TOUT_TRY (!hw_i2c_match_event(i2c, I2C_EVENT_MASTER_TRANSMITTED));

    // Stop
    I2C_CR1(i2c) |= I2C_CR1_STOP;
    //while (hw_i2c_match_event(i2c, I2C_EVENT_STOP));

    return i;
}

static int hw_i2c_read_1byte(uint32_t i2c, uint8_t addr, uint8_t *buf)
{
    uint32_t irq_state;
    uint32_t begin = cupkee_systicks();

    // Start
    I2C_CR1(i2c) |= I2C_CR1_START;

    // Send address
    TOUT_TRY (!hw_i2c_match_event(i2c, I2C_EVENT_MASTER_MODE_SELECT));
    I2C_DR(i2c) = addr;

    TOUT_TRY (!(I2C_SR1(i2c) & I2C_SR1_ADDR));
    I2C_CR1(i2c) &= ~I2C_CR1_ACK; // Nack for DataN

    hw_enter_critical(&irq_state);
    hw_i2c_match_event(i2c, I2C_EVENT_MASTER_RECEIVER_MODE_SELECT); // Clear addr
    I2C_CR1(i2c) |= I2C_CR1_STOP;
    hw_exit_critical(irq_state);

    TOUT_TRY (!hw_i2c_match_event(i2c, I2C_EVENT_MASTER_RECEIVED));
    *buf = I2C_DR(i2c);

    // wait stop
    //TOUT_TRY (hw_i2c_match_event(i2c, I2C_EVENT_STOP));
    I2C_CR1(i2c) |= I2C_CR1_ACK;

    return 1;
}

static int hw_i2c_read_2byte(uint32_t i2c, uint8_t addr, uint8_t *buf)
{
    uint32_t irq_state;
    uint32_t begin = cupkee_systicks();

    // trigger start for data receive
    I2C_CR1(i2c) |= I2C_CR1_START;

    TOUT_TRY (!hw_i2c_match_event(i2c, I2C_EVENT_MASTER_MODE_SELECT));
    I2C_DR(i2c) = addr;

    TOUT_TRY (!(I2C_SR1(i2c) & I2C_SR1_ADDR));
    I2C_CR1(i2c) |= I2C_CR1_POS; // Trigger Nack at next shift complete

    hw_enter_critical(&irq_state);
    hw_i2c_match_event(i2c, I2C_EVENT_MASTER_RECEIVER_MODE_SELECT); // Clear addr
    I2C_CR1(i2c) &= ~I2C_CR1_ACK; // Nack
    hw_exit_critical(irq_state);

    // Wait for Data1 and Data2 received
    TOUT_TRY (!hw_i2c_match_event(i2c, I2C_EVENT_MASTER_RECEIVE_HOLD));
    hw_enter_critical(&irq_state);
    I2C_CR1(i2c) |= I2C_CR1_STOP;
    buf[0] = I2C_DR(i2c);
    hw_exit_critical(irq_state);

    buf[1] = I2C_DR(i2c);

    //TOUT_TRY (hw_i2c_match_event(i2c, I2C_EVENT_STOP));
    I2C_CR1(i2c) |= I2C_CR1_ACK;

    return 2;
}

static int hw_i2c_read_Nbyte(uint32_t i2c, uint8_t addr, uint8_t n, uint8_t *buf)
{
    uint8_t i;
    uint32_t irq_state;
    uint32_t begin = cupkee_systicks();

    // trigger start for data receive
    I2C_CR1(i2c) |= I2C_CR1_START;

    TOUT_TRY (!hw_i2c_match_event(i2c, I2C_EVENT_MASTER_MODE_SELECT));
    I2C_DR(i2c) = addr; // Set the BIT0 of address for read

    TOUT_TRY (!hw_i2c_match_event(i2c, I2C_EVENT_MASTER_RECEIVER_MODE_SELECT));
    for (i = 0; i < n - 3; i++) {
        TOUT_TRY (!hw_i2c_match_event(i2c, I2C_EVENT_MASTER_RECEIVED));
        buf[i] = I2C_DR(i2c);
    }

    // Wait for DataN-2 and DataN-1 received
    TOUT_TRY (!hw_i2c_match_event(i2c, I2C_EVENT_MASTER_RECEIVE_HOLD));

    // Nack for DataN
    I2C_CR1(i2c) &= ~I2C_CR1_ACK;

    // load DataN-2, to start the receiveing of DataN
    buf[i++] = I2C_DR(i2c);

    // Wait for DataN received
    TOUT_TRY (!hw_i2c_match_event(i2c, I2C_EVENT_MASTER_RECEIVE_HOLD));

    hw_enter_critical(&irq_state);
    I2C_CR1(i2c) |= I2C_CR1_STOP;
    buf[i++] = I2C_DR(i2c);
    hw_exit_critical(irq_state);
    buf[i++] = I2C_DR(i2c);

    //TOUT_TRY (hw_i2c_match_event(i2c, I2C_EVENT_STOP));
    I2C_CR1(i2c) |= I2C_CR1_ACK;

    return i;
}

static int hw_i2c_read_sync(int instance, size_t n, void *buf)
{
    hw_i2c_t *control;
    uint32_t i2c;
    uint8_t addr;
    uint32_t begin = cupkee_systicks();

    if (n < 1) {
        return 0;
    }

    control = hw_i2c_control(instance);
    i2c  = I2C_REG_BASE(instance);
    addr = control->slave_addr | 1;

    if (!control) {
        return -CUPKEE_EINVAL;
    }

    TOUT_TRY (I2C_SR2(i2c) & I2C_SR2_BUSY);

    if (n == 1) {
        return hw_i2c_read_1byte(i2c, addr, buf);
    } else
    if (n == 2) {
        return hw_i2c_read_2byte(i2c, addr, buf);
    } else {
        return hw_i2c_read_Nbyte(i2c, addr, n, buf);
    }
}

static int hw_i2c_prop_get(int instance, int which, uint32_t *value)
{
    hw_i2c_t *control = hw_i2c_control(instance);

    if (!control || !value) {
        return 0;
    }

    if (which == I2C_PROP_SLAVE) {
        *value = control->slave_addr & (~1);
        return 1;
    }
    return 0;
}

static int hw_i2c_prop_set(int instance, int which, uint32_t value)
{
    hw_i2c_t *control = hw_i2c_control(instance);

    value = value & 0xFE;

    if (!control || (value & 0xfe) == 0) {
        return 0;
    }

    if (which == I2C_PROP_SLAVE) {
        control->slave_addr = (uint8_t) value;
        return 1;
    }
    return 0;
}

static int hw_i2c_io_cached(int instance, size_t *in, size_t *out)
{
    hw_i2c_t *control = hw_i2c_control(instance);

    if (!control) {
        return -CUPKEE_EINVAL;
    }

    if (in) {
        if (control->rcv_buf) {
            *in = cupkee_buffer_length(control->rcv_buf);
        } else {
            *in = 0;
        }
    }
    if (out) {
        if (control->req_buf) {
            *out = cupkee_buffer_length(control->req_buf);
        } else {
            *out = 0;
        }
    }
    return 0;
}

static int hw_i2c_prop_num(int instance)
{
    (void) instance;
    return 1;
}

static const hw_driver_t i2c_driver = {
    .release = hw_i2c_release,
    .reset   = hw_i2c_reset,
    .setup   = hw_i2c_setup,
    .sync    = hw_i2c_sync,
    .poll    = hw_i2c_poll,

    .get     = hw_i2c_prop_get,
    .set     = hw_i2c_prop_set,
    .size    = hw_i2c_prop_num,

    .io_cached = hw_i2c_io_cached,

    .read_req  = hw_i2c_read_req,
    .read      = hw_i2c_read,
    .write     = hw_i2c_write,
    .read_sync = hw_i2c_read_sync,
    .write_sync = hw_i2c_write_sync
};

const hw_driver_t *hw_request_i2c(int instance)
{
    if (instance >= HW_INSTANCES_I2C || 0 == hw_use_instance(instance, &use_map)) {
        return NULL;
    }

    i2c_controls[instance].dev_id = DEVICE_ID_INVALID;
    i2c_controls[instance].state  = I2C_STATE_IDLE;

    i2c_controls[instance].req_buf = NULL;
    i2c_controls[instance].rcv_buf = NULL;

    return &i2c_driver;
}

#endif

void hw_setup_i2c(void)
{
}

