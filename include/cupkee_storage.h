/*
This file is part of cupkee project.

Copyright (c) 2018 Lixing Ding <ding.lixing@gmail.com>

*/

#ifndef __CUPKEE_STORAGE_INC__
#define __CUPKEE_STORAGE_INC__

#define CUPKEE_STORAGE_BANK_SYS         0
#define CUPKEE_STORAGE_BANK_SYS_BACK    1
#define CUPKEE_STORAGE_BANK_CFG         2
#define CUPKEE_STORAGE_BANK_APP         3

#define CUPKEE_STORAGE_BANK_MAX         4

typedef struct cupkee_storage_info_t {
    uint32_t base; //
    uint32_t size; // size in bytes
    uint16_t sector_bgn;
    uint16_t sector_num;
} cupkee_storage_info_t;


int cupkee_storage_init(uint32_t sector_num);

uint32_t cupkee_storage_base(int bank_id);
uint32_t cupkee_storage_size(int bank_id);
int cupkee_storage_erase(uint32_t bank_id);
int cupkee_storage_query(int bank_id, cupkee_storage_info_t *info);
int cupkee_storage_write(uint32_t band_id, uint32_t offset, uint32_t size, const uint8_t *data);

int cupkee_storage_sector_erase(uint32_t start, uint32_t n);
const void *cupkee_storage_sector_mmap(void *to, uint32_t sector_start, uint32_t n);

int cupkee_storage_block_write(uint32_t base, uint32_t size, const uint8_t *data);
int cupkee_storage_block_read (uint32_t sector, uint32_t block, uint8_t *buf);

#endif  /* __CUPKEE_STORAGE_INC__ */
