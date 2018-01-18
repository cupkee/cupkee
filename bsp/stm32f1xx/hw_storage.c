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

#include "hardware.h"

#ifndef FLASH_BASE
#define FLASH_BASE      0x08000000
#endif

#define APP_SECTORS     8
#define CFG_SECTORS     8

#define BANK_BASE(s)    (FLASH_BASE + (s) * (1 << _storage_sec_size_shift))
#define BANK_SIZE(s)    ((s) * (1 << _storage_sec_size_shift))

static uint8_t _storage_sec_size_shift = 10;
static uint8_t _storage_sec_base_cfg = 0;
static uint8_t _storage_sec_base_app = 0;

static int  hw_storage_program(uint32_t base, const uint8_t *data, uint32_t len)
{
    uint32_t padding = base % sizeof(uint32_t);
    uint32_t pos = 0;

    flash_unlock();

    if (padding) {
        uint32_t val = *((uint32_t *) (base - padding));
        uint8_t *ptr = (uint8_t *)&val;

        base -= padding;
        while (padding < sizeof(uint32_t)) {
            ptr[padding++] = data[pos++];
        }

        flash_program_word(base, val);
        base += sizeof(uint32_t);
    }

    while (pos + sizeof(uint32_t) < len) {
        uint32_t val;
        uint8_t *ptr = (uint8_t *)&val;

        ptr[0] = data[pos++];
        ptr[1] = data[pos++];
        ptr[2] = data[pos++];
        ptr[3] = data[pos++];
        flash_program_word(base, val);

        base += sizeof(uint32_t);
    }

    if (pos < len) {
        uint32_t val = 0xFFFFFFFF, i = 0;
        uint8_t *ptr = (uint8_t *)&val;
        while (pos < len) {
            ptr[i++] = data[pos++];
        }
        flash_program_word(base, val);
    }

    flash_lock();

    return len;
}

void hw_setup_storage(void)
{
    uint32_t rom = desig_get_flash_size();
    uint32_t sector_size;

    if (rom >= 256) {
        _storage_sec_size_shift = 11;
    } else {
        _storage_sec_size_shift = 10;
    }
    sector_size = 1 << _storage_sec_size_shift;

    rom = (rom * 1024) / sector_size;
    _storage_sec_base_cfg = rom - CFG_SECTORS;
    _storage_sec_base_app = _storage_sec_base_cfg - APP_SECTORS;
}

uint32_t hw_storage_size(int bank)
{
    if (bank == HW_STORAGE_BANK_APP) {
        return BANK_SIZE(APP_SECTORS);
    } else
    if (bank == HW_STORAGE_BANK_CFG) {
        return BANK_SIZE(CFG_SECTORS);
    } else {
        return 0;
    }
}

int hw_storage_erase(int bank)
{
    uint32_t cur, end, sector_size;

    if (bank == HW_STORAGE_BANK_APP) {
        cur = BANK_BASE(_storage_sec_base_app);
        end = cur + BANK_SIZE(APP_SECTORS);
    } else
    if (bank == HW_STORAGE_BANK_CFG) {
        cur = BANK_BASE(_storage_sec_base_cfg);
        end = cur + BANK_SIZE(CFG_SECTORS);
    } else {
        return 0;
    }
    sector_size = 1 << _storage_sec_size_shift;

    flash_unlock();
    while (cur < end) {
        flash_erase_page(cur);
        if (flash_get_status_flags() != FLASH_SR_EOP) {
            return -1;
        }
        cur += sector_size;
    }
    flash_lock();

    return 0;
}

const char *hw_storage_data_map(int bank)
{
    if (bank == HW_STORAGE_BANK_APP) {
        return (const char *)BANK_BASE(_storage_sec_base_app);
    } else
    if (bank == HW_STORAGE_BANK_CFG) {
        return (const char *)BANK_BASE(_storage_sec_base_cfg);
    } else {
        return NULL;
    }
}

uint32_t hw_storage_data_length(int bank)
{
    uint8_t *ptr;
    uint32_t end, i;

    if (bank == HW_STORAGE_BANK_APP) {
        ptr = (uint8_t *)BANK_BASE(_storage_sec_base_app);
        end = BANK_SIZE(APP_SECTORS);
    } else
    if (bank == HW_STORAGE_BANK_CFG) {
        ptr = (uint8_t *)BANK_BASE(_storage_sec_base_cfg);
        end = BANK_SIZE(CFG_SECTORS);
    } else {
        return 0;
    }

    for (i = 0; i < end; i++) {
        uint8_t d = ptr[i];

        if (d == 0 || d == 0xFF) {
            break;
        }
    }
    return i;
}

int hw_storage_update(int bank, uint32_t offset, const uint8_t *data, int len)
{
    uint32_t base, end;

    if (bank == HW_STORAGE_BANK_APP) {
        base = BANK_BASE(_storage_sec_base_app);
        end = base + BANK_SIZE(APP_SECTORS);
    } else
    if (bank == HW_STORAGE_BANK_CFG) {
        base = BANK_BASE(_storage_sec_base_cfg);
        end = base + BANK_SIZE(CFG_SECTORS);
    } else {
        return 0;
    }

    base += offset;
    if (base >= end) {
        return -1;
    }

    if (base + len >= end) {
        len = end - base;
    }

    return hw_storage_program(base, data, len);
}

int hw_storage_finish(int bank, uint32_t offset)
{
    uint32_t base, end;
    uint8_t zero = 0;

    if (bank == HW_STORAGE_BANK_APP) {
        base = BANK_BASE(_storage_sec_base_app);
        end = base + BANK_SIZE(APP_SECTORS);
    } else
    if (bank == HW_STORAGE_BANK_CFG) {
        base = BANK_BASE(_storage_sec_base_cfg);
        end = base + BANK_SIZE(CFG_SECTORS);
    } else {
        return 0;
    }

    base += offset;
    if (base >= end) {
        base = end - 1;
    }

    return hw_storage_program(base, &zero, 1);
}

