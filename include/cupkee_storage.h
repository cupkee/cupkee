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

intptr_t cupkee_storage_base(int bank_id);
uint32_t cupkee_storage_size(int bank_id);
int cupkee_storage_erase(uint32_t bank_id);
int cupkee_storage_query(int bank_id, cupkee_storage_info_t *info);
int cupkee_storage_write(uint32_t band_id, uint32_t offset, uint32_t size, const uint8_t *data);

int cupkee_storage_sector_erase(uint32_t start, uint32_t n);
const void *cupkee_storage_sector_mmap(void *to, uint32_t sector_start, uint32_t n);

int cupkee_storage_block_write(uint32_t base, uint32_t size, const uint8_t *data);
int cupkee_storage_block_read (uint32_t sector, uint32_t block, uint8_t *buf);

#endif  /* __CUPKEE_STORAGE_INC__ */
