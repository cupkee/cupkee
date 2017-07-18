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

#include "test.h"

static uint8_t *mock_memory_base = NULL;
static size_t   mock_memory_size = 0;
static size_t   mock_memory_off  = 0;

static int mock_device_instance_max = 2;
static struct {
    uint8_t id;
    uint8_t type;
    uint8_t inst;
    const hw_config_t *config;

    size_t  out;
    size_t  want;
    uint8_t data[32];
} mock_device_info;

cupkee_device_t *mock_device_curr(void)
{
    return cupkee_device_block(mock_device_info.id);
}

size_t mock_device_curr_want(void)
{
    return mock_device_info.want;
}

int hw_mock_device_curr_id(void)
{
    return mock_device_info.id;
}

static void hw_mock_release(int inst)
{
    (void) inst;
}

static void hw_mock_reset (int inst)
{
    (void) inst;
}

static int  hw_mock_setup (int inst, uint8_t devid, const hw_config_t *conf)
{
    (void) inst;
    mock_device_info.id = devid;
    mock_device_info.config = conf;

    return 0;
}

static int hw_mock_query(int inst, size_t n, void *data, int want)
{
    (void) inst;

    mock_device_info.want = want;
    mock_device_info.out = n;
    n = n < 32 ? n : 32;

    memcpy(mock_device_info.data, data, n);

    return 0;
}

static void hw_mock_sync  (int inst, uint32_t systicks)
{
    (void) inst;
    (void) systicks;
}

static void hw_mock_poll  (int inst)
{
    (void) inst;
}

static int  hw_mock_get (int inst, int offset, uint32_t*data)
{
    (void) inst;
    (void) offset;
    (void) data;
    return 0;
}

static int  hw_mock_set (int inst, int offset, uint32_t data)
{
    (void) inst;
    (void) offset;
    (void) data;
    return 0;
}

static int  hw_mock_size(int inst)
{
    (void) inst;
    return 0;
}

static int hw_mock_read_req     (int inst, size_t n)
{
    (void) inst;
    (void) n;
    return 0;
}

static int hw_mock_read         (int inst, size_t n, void *buf)
{
    (void) inst;
    (void) n;
    (void) buf;
    return 0;
}

static int hw_mock_write        (int inst, size_t n, const void *buf)
{
    (void) inst;
    (void) n;
    (void) buf;
    return 0;
}

static int hw_mock_read_sync    (int inst, size_t n, void *buf)
{
    (void) inst;
    (void) n;
    (void) buf;
    return 0;
}

static int hw_mock_write_sync   (int inst, size_t n, const void *buf)
{
    (void) inst;
    (void) n;
    (void) buf;
    return 0;
}

// Todo: need a suitable name
static int hw_mock_io_cached (int inst, size_t *in, size_t *out)
{
    (void) inst;
    (void) in;
    (void) out;
    return 0;
}

static hw_driver_t mock_driver = {
    .release = hw_mock_release,
    .reset   = hw_mock_reset,
    .setup   = hw_mock_setup,

    .query   = hw_mock_query,

    .sync    = hw_mock_sync, // deprecate
    .poll    = hw_mock_poll, // deprecate

    .get     = hw_mock_get,  // deprecate
    .set     = hw_mock_set,  // deprecate
    .size    = hw_mock_size, // deprecate

    .read_req = hw_mock_read_req,   // deprecate
    .read     = hw_mock_read,       // deprecate
    .write    = hw_mock_write,      // deprecate
    .read_sync     = hw_mock_read_sync, // deprecate
    .write_sync    = hw_mock_write_sync,// deprecate
    .io_cached     = hw_mock_io_cached, // deprecate
};

void hw_mock_init(size_t mem_size)
{
    if (mock_memory_base) {
        free(mock_memory_base);
    }

    mock_memory_base = malloc(mem_size);
    mock_memory_size = mem_size;
    mock_memory_off = 0;
}

void hw_mock_deinit(void)
{
    if (mock_memory_base) {
        free(mock_memory_base);
        mock_memory_base = NULL;
        mock_memory_size = 0;
        mock_memory_off = 0;
    }
}

void hw_enter_critical(uint32_t *state)
{
    (void) state;
}

void hw_exit_critical(uint32_t state)
{
    (void) state;
}

void *hw_malloc(size_t size, size_t align)
{
    size_t off = CUPKEE_SIZE_ALIGN(mock_memory_off, align);

    if (off + size > mock_memory_size) {
        return NULL;
    }

    mock_memory_off = off + size;

    return mock_memory_base + off;
}

void hw_setup(void)
{}

void _hw_reset(void)
{}

void hw_poll(void)
{}

void hw_halt(void)
{}

void hw_info_get(hw_info_t *info)
{
    (void) info;
}

uint32_t hw_storage_size(int bank)
{
    (void) bank;
    return 0;
}

int hw_storage_erase (int bank)
{
    (void) bank;
    return -1;
}

int hw_storage_update(int bank, uint32_t offset, const uint8_t *data, int len)
{
    (void) bank;
    (void) offset;
    (void) data;
    (void) len;

    return -1;
}

int hw_storage_finish(int bank, uint32_t end)
{
    (void) bank;
    (void) end;
    return -1;
}

uint32_t hw_storage_data_length(int bank)
{
    (void) bank;
    return 0;
}

const char *hw_storage_data_map(int bank)
{
    (void) bank;
    return NULL;
}

void hw_usb_msc_init(const char *vendor, const char *product, const char *version, uint32_t blocks,
                     int (*read_cb)(uint32_t lba, uint8_t *),
                     int (*write_cb)(uint32_t lba, const uint8_t *))
{
    (void) vendor;
    (void) product;
    (void) version;
    (void) blocks;
    (void) read_cb;
    (void) write_cb;
}

/* DEBUG LED */
int  hw_led_map(int port, int pin)
{
    (void) port;
    (void) pin;
    return 0;
}

void hw_led_set(void)
{}

void hw_led_clear(void)
{}

void hw_led_toggle(void)
{}

/* GPIO */
int   hw_pin_map(int id, int port, int pin)
{
    (void) id;
    (void) port;
    (void) pin;
    return 0;
}

/* DEVICE */
const hw_driver_t *hw_device_request(int type, int inst)
{
    mock_device_info.type = type;
    mock_device_info.inst = inst;

    return &mock_driver;
}

int hw_device_instances(int type)
{
    (void) type;
    return mock_device_instance_max;
}

