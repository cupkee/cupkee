/*
This file is part of cupkee project.

Copyright (c) 2017 Lixing Ding <ding.lixing@gmail.com>

*/

#include <cupkee.h>

static uint32_t sector_total_num;
static uint8_t sector_bgn[CUPKEE_STORAGE_BANK_MAX];
static uint8_t sector_num[CUPKEE_STORAGE_BANK_MAX];

int cupkee_storage_init(uint32_t sectors)
{
    sector_total_num = sectors;

    sector_bgn[CUPKEE_STORAGE_BANK_APP] = sectors - 1;
    sector_bgn[CUPKEE_STORAGE_BANK_CFG] = sector_bgn[CUPKEE_STORAGE_BANK_APP] - 1;
    sector_bgn[CUPKEE_STORAGE_BANK_SYS_BACK] = sector_bgn[CUPKEE_STORAGE_BANK_CFG] / 2;
    sector_bgn[CUPKEE_STORAGE_BANK_SYS] = 0;

    sector_num[CUPKEE_STORAGE_BANK_APP] = 1;
    sector_num[CUPKEE_STORAGE_BANK_CFG] = 1;
    sector_num[CUPKEE_STORAGE_BANK_SYS_BACK] = sector_bgn[CUPKEE_STORAGE_BANK_CFG]
                                             - sector_bgn[CUPKEE_STORAGE_BANK_SYS_BACK];
    sector_num[CUPKEE_STORAGE_BANK_SYS] = sector_bgn[CUPKEE_STORAGE_BANK_SYS_BACK];

    return 0;
}

intptr_t cupkee_storage_base(int bank_id)
{
    if (bank_id < CUPKEE_STORAGE_BANK_MAX) {
        return hw_storage_base() + sector_bgn[bank_id] * CUPKEE_SECTOR_SIZE;
    }

    return 0;
}

uint32_t cupkee_storage_size(int bank_id)
{
    if (bank_id < CUPKEE_STORAGE_BANK_MAX) {
        return sector_num[bank_id] * CUPKEE_SECTOR_SIZE;
    }

    return 0;
}

int cupkee_storage_query(int bank_id, cupkee_storage_info_t *info)
{
    if (bank_id < CUPKEE_STORAGE_BANK_MAX) {
        if (info) {
            info->sector_bgn = sector_bgn[bank_id];
            info->sector_num = sector_num[bank_id];
            info->base = hw_storage_base() + sector_bgn[bank_id] * CUPKEE_SECTOR_SIZE;
            info->size = sector_num[bank_id] * CUPKEE_SECTOR_SIZE;
        }

        return 0;
    }

    return -CUPKEE_EINVAL;
}

int cupkee_storage_erase(uint32_t bank_id)
{
    if (bank_id < CUPKEE_STORAGE_BANK_MAX) {
        intptr_t base = hw_storage_base() + sector_bgn[bank_id] * CUPKEE_SECTOR_SIZE;
        uint32_t size = sector_num[bank_id] * CUPKEE_SECTOR_SIZE;

        return hw_storage_erase(base, size);
    }
    return -CUPKEE_EINVAL;
}

int cupkee_storage_write(uint32_t bank_id, uint32_t offset, uint32_t size, const uint8_t *data)
{
    if (bank_id < CUPKEE_STORAGE_BANK_MAX) {
        intptr_t base = cupkee_storage_base(bank_id) + offset;

        return hw_storage_program(base, size, data);
    }
    return -CUPKEE_EINVAL;
}

int cupkee_storage_sector_erase(uint32_t sector_start, uint32_t n)
{
    intptr_t base = hw_storage_base() + sector_start * CUPKEE_SECTOR_SIZE;
    uint32_t size = n * CUPKEE_SECTOR_SIZE;

    //console_log("erase sector: %x, %x\r\n", base, size);
    return hw_storage_erase(base, size);
}

int cupkee_storage_block_write(uint32_t sector, uint32_t block, const uint8_t *data)
{
    intptr_t base = hw_storage_base() + sector * CUPKEE_SECTOR_SIZE;

    base += block * CUPKEE_BLOCK_SIZE;

    //console_log("write sector: %x, %x\r\n", base, CUPKEE_BLOCK_SIZE);
    return hw_storage_program(base, CUPKEE_BLOCK_SIZE, data);
}

int cupkee_storage_block_read(uint32_t sector, uint32_t block, uint8_t *buf)
{
    if (sector < sector_total_num) {
        intptr_t base = hw_storage_base() + sector * CUPKEE_SECTOR_SIZE;
        const uint8_t *p = (const uint8_t *)base + block * CUPKEE_BLOCK_SIZE;
        memcpy(buf, p, CUPKEE_BLOCK_SIZE);
        return 0;
    } else {
        return -CUPKEE_EINVAL;
    }
}

const void *cupkee_storage_sector_mmap(void *to, uint32_t sector_start, uint32_t n)
{
    (void) to;
    if (sector_start + n <= sector_total_num) {
        intptr_t base = hw_storage_base() + sector_start * CUPKEE_SECTOR_SIZE;

        return (const void *)base;
    } else {
        return NULL;
    }
}


