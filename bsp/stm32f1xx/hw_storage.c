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

void hw_setup_storage(void)
{
}

static inline uint32_t hw_storage_size(void)
{
    return desig_get_flash_size() * 1024;
}

intptr_t hw_storage_base(void)
{
    return FLASH_BASE;
}

int hw_storage_erase(uint32_t base, uint32_t size)
{
    uint32_t flash_size = hw_storage_size();
    uint32_t flash_sector_size = (flash_size) >= 256 * 1024 ? (1 << 11) : (1 << 10);
    uint32_t flash_sector_mask = flash_sector_size - 1;

    uint32_t end = base + size;

    if ((base & flash_sector_mask) || base < FLASH_BASE ||
        (size & flash_sector_mask) || end > FLASH_BASE + flash_size) {
        return -CUPKEE_EINVAL;
    }

    flash_unlock();
    while (base < end) {
        flash_erase_page(base);
        if (flash_get_status_flags() != FLASH_SR_EOP) {
            return -1;
        }
        base += flash_sector_size;
    }
    flash_lock();

    return 0;
}

int hw_storage_program(uint32_t base, uint32_t size, const uint8_t *data)
{
    uint32_t flash_tail = FLASH_BASE + hw_storage_size();
    uint32_t padding = base % sizeof(uint32_t);
    uint32_t pos = 0;

    if (base < FLASH_BASE || base + size > flash_tail) {
        return -CUPKEE_EINVAL;
    }

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

    while (pos + sizeof(uint32_t) < size) {
        uint32_t val;
        uint8_t *ptr = (uint8_t *)&val;

        ptr[0] = data[pos++];
        ptr[1] = data[pos++];
        ptr[2] = data[pos++];
        ptr[3] = data[pos++];
        flash_program_word(base, val);

        base += sizeof(uint32_t);
    }

    if (pos < size) {
        uint32_t val = 0xFFFFFFFF, i = 0;
        uint8_t *ptr = (uint8_t *)&val;
        while (pos < size) {
            ptr[i++] = data[pos++];
        }
        flash_program_word(base, val);
    }

    flash_lock();

    return size;
}

