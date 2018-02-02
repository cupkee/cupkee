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

#include "cupkee.h"
#include "cupkee_sysdisk.h"

#define APP_HEAD    "/* CUPKEE APP */"
#define CFG_HEAD    "/* CUPKEE CONFIG  */"

#define WBVAL(x) ((x) & 0xFF), (((x) >> 8) & 0xFF)
#define QBVAL(x) ((x) & 0xFF), (((x) >> 8) & 0xFF),\
		 (((x) >> 16) & 0xFF), (((x) >> 24) & 0xFF)

#define SECTOR_SIZE		    512
#define BYTES_PER_SECTOR	512
#define SECTORS_PER_CLUSTER	4
#define BYTES_PER_CLUSTER   (BYTES_PER_SECTOR * SECTORS_PER_CLUSTER)
#define RESERVED_SECTORS	1
#define FAT_COPIES		    2
#define ROOT_ENTRIES		512
#define ROOT_ENTRY_LENGTH	32
#define FILEDATA_START_CLUSTER	2
#define DATA_REGION_SECTOR	(RESERVED_SECTORS + FAT_COPIES + \
			(ROOT_ENTRIES * ROOT_ENTRY_LENGTH) / BYTES_PER_SECTOR)
#define FILEDATA_START_SECTOR	(DATA_REGION_SECTOR + \
			(FILEDATA_START_CLUSTER - 2) * SECTORS_PER_CLUSTER)

#define ROOT_START_SECTOR    3
#define ROOT_END_SECTOR      (ROOT_START_SECTOR + (ROOT_ENTRIES / (SECTOR_SIZE / ROOT_ENTRY_LENGTH)))

#define START_CLUSTER(s)     (((s) - FILEDATA_START_SECTOR) / SECTORS_PER_CLUSTER + 2)
#define COUNT_CLUSTER(s)     (((s) + BYTES_PER_CLUSTER - 1) / BYTES_PER_CLUSTER)

static const char *app_data = NULL;
static const char *cfg_data = NULL;
static uint8_t curr_bank  = 0;
static uint8_t curr_state = 0;
static uint16_t curr_offset = 0;
static uint16_t app_size = 0;
static uint16_t cfg_size = 0;
static uint16_t app_start_sector = 0;
static uint16_t cfg_start_sector = 0;

static const uint8_t boot_sector[] = {
	0xEB, 0x3C, 0x90,				// code to jump to the bootstrap code
	'm', 's', 'd', 'o', 's', 'f', 's', 0x00,		// OEM ID
	WBVAL(BYTES_PER_SECTOR),		// bytes per sector
	SECTORS_PER_CLUSTER,			// sectors per cluster
	WBVAL(RESERVED_SECTORS),		// # of reserved sectors (1 boot sector)
	FAT_COPIES,						// FAT copies (2)
	WBVAL(ROOT_ENTRIES),			// root entries (512)
	WBVAL(CUPKEE_SYSDISK_SECTOR_COUNT),			// total number of sectors
	0xF8,							// media descriptor (0xF8 = Fixed disk)
	0x01, 0x00,						// sectors per FAT (1)
	0x20, 0x00,						// sectors per track (32)
	0x40, 0x00,						// number of heads (64)
	0x00, 0x00, 0x00, 0x00,		    // hidden sectors (0)
	0x00, 0x00, 0x00, 0x00,			// large number of sectors (0)
	0x00,							// drive number (0)
	0x00,							// reserved
	0x29,							// extended boot signature
	0x69, 0x17, 0xAD, 0x53,			// volume serial number
	'C', 'U', 'P', 'D', 'I', 'S', 'K', ' ', ' ', ' ', ' ',	// volume label
	'F', 'A', 'T', '1', '6', ' ', ' ', ' '			// filesystem type
};

static const uint8_t fat_sector[] = {
	0xF8, 0xFF, 0xFF, 0xFF,
};

static const uint8_t dir_templete[] = {
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',	// filename
	' ', ' ', ' ',							// extension
	0x20,									// attribute byte
	0x00,									// reserved for Windows NT
	0x00,									// creation millisecond
	0xCE, 0x01,								// creation time
	0x86, 0x41,								// creation date
	0x86, 0x41,								// last access date
	0x00, 0x00,								// reserved for FAT32
	0xCE, 0x01,								// last write time
	0x86, 0x41,								// last write date
};

static void sysdisk_boot(uint8_t *buf)
{
	memset(buf, 0, SECTOR_SIZE);
	memcpy(buf, boot_sector, sizeof(boot_sector));
	buf[SECTOR_SIZE - 2] = 0x55;
	buf[SECTOR_SIZE - 1] = 0xAA;
}

static void sysddisk_fat_set(uint8_t *fat, uint16_t start, uint16_t size)
{
    uint16_t end;

    start = START_CLUSTER(start);
    end   = COUNT_CLUSTER(size) + start;

    while (start < end && start < 256) {
        if (start == end - 1) {
            fat[start * 2] = 0xFF;
            fat[start * 2 + 1] = 0xFF;
        } else {
            fat[start * 2] = start + 1;
            fat[start * 2 + 1] = 0;
        }
        start ++;
    }
}


static void sysdisk_fat(uint8_t *fat)
{
	memset(fat, 0, SECTOR_SIZE);

	memcpy(fat, fat_sector, sizeof(fat_sector));
    sysddisk_fat_set(fat, app_start_sector, app_size);
    sysddisk_fat_set(fat, cfg_start_sector, cfg_size);
}

static void sysdisk_dir_set(uint8_t *dir, const char *prefix, const char *suffix, uint32_t start, uint32_t size)
{
    int i;

    memcpy(dir, dir_templete, sizeof(dir_templete));
    for (i = 0; i < 8 && prefix[i]; i++) {
        dir[i] = prefix[i];
    }
    for (i = 0; i < 3 && suffix[i]; i++) {
        dir[8 + i] = suffix[i];
    }

    dir[26] = (uint8_t )(start);
    dir[27] = (uint8_t )(start >> 8);

    dir[28] = (uint8_t )(size);
    dir[29] = (uint8_t )(size >> 8);
    dir[30] = (uint8_t )(size >> 16);
    dir[31] = (uint8_t )(size >> 24);
}

static void sysdisk_dir(uint8_t *dir)
{
	memset(dir, 0, SECTOR_SIZE);

    sysdisk_dir_set(dir, "APP", "JS", START_CLUSTER(app_start_sector), app_size);
    sysdisk_dir_set(dir + ROOT_ENTRY_LENGTH, "CONFIG", "JS", START_CLUSTER(cfg_start_sector), cfg_size);
}

static void sysdisk_file_read(uint32_t lba, uint8_t *buf)
{
    int length = 0;

    if (lba >= app_start_sector && (lba - app_start_sector) < 16) {
        int offset = (lba - app_start_sector) * SECTOR_SIZE;

        if (offset < app_size) {
            length = app_size - offset;
            if (length > SECTOR_SIZE) {
                length = SECTOR_SIZE;
            }
            memcpy(buf, app_data + offset, length);
        }
    } else
    if (lba >= cfg_start_sector && (lba - cfg_start_sector) < 16) {
        int offset = (lba - cfg_start_sector) * SECTOR_SIZE;

        if (offset < cfg_size) {
            length = cfg_size - offset;
            if (length > SECTOR_SIZE) {
                length = SECTOR_SIZE;
            }
            memcpy(buf, cfg_data + offset, length);
        }
    }

    if (length < SECTOR_SIZE) {
        memset(buf + length, 0, SECTOR_SIZE - length);
    }
}

static void sysdisk_write_init(const uint8_t *data)
{
    const uint8_t *type;
    int i;

    if (!(data[0] == '#' || (data[0] == '/' && (data[1] == '/' || data[1] == '*')))) {
        return;
    }

    for (i = 2; i < 10; i++) {
        if (data[i] != ' ') {
            break;
        }
    }

    if (memcmp(data + i, "CUPKEE ", 7)) {
        return;
    }
    type = data + i + 7;

    if (!memcmp(type, "APP", 3) || !memcmp(type, "app", 3)) {
        curr_state = 1;
        curr_offset = 0;
        curr_bank = HW_STORAGE_BANK_APP;
        app_data = hw_storage_data_map(HW_STORAGE_BANK_APP);
        hw_storage_erase(curr_bank);
    } else
    if (!memcmp(type, "CONFIG", 6) || !memcmp(type, "config", 6)) {
        curr_state = 1;
        curr_offset = 0;
        curr_bank  = HW_STORAGE_BANK_CFG;
        cfg_data = hw_storage_data_map(HW_STORAGE_BANK_CFG);
        hw_storage_erase(curr_bank);
    }
    return;
}

static void sysdisk_write_finish(const uint8_t *entry)
{
    char *target = NULL;
    uint16_t cluster;
    uint32_t size, old_size, max_size;
    uint8_t  bank;

    if (0 == memcmp(entry, "APP     JS ", 11)) {
        target = "app.js";
        bank = HW_STORAGE_BANK_APP;
    } else
    if (0 == memcmp(entry, "CONFIG  JS ", 11)){
        target = "config.js";
        bank = HW_STORAGE_BANK_CFG;
    } else {
        return;
    }

    cluster = entry[26] + entry[27] * 256;
    size = entry[28] + entry[29] * 256 + entry[30] * 0x10000 + entry[31] * 0x1000000;

    if (bank == HW_STORAGE_BANK_APP) {
        app_start_sector = FILEDATA_START_SECTOR + (cluster - 2) * SECTORS_PER_CLUSTER;
        max_size = hw_storage_size(bank);
        old_size = app_size;
        if (max_size < size) {
            size = max_size;
        }
        app_size = size;
    } else {
        cfg_start_sector = FILEDATA_START_SECTOR + (cluster - 2) * SECTORS_PER_CLUSTER;
        max_size = hw_storage_size(bank);
        old_size = cfg_size;
        if (max_size < size) {
            size = max_size;
        }
        cfg_size = size;
    }

    if (old_size != size) {
        hw_storage_finish(bank, size);
        console_log("update %s: %dbytes\r\n", target, size);
    }
}

static void sysdisk_write_parse(const uint8_t *info)
{
    int pos = 0;

    while (pos < SECTOR_SIZE) {
        sysdisk_write_finish(info + pos);
        pos += ROOT_ENTRY_LENGTH;
    }
}

int cupkee_sysdisk_read(uint32_t lba, uint8_t *copy_to)
{
	switch (lba) {
    case 0: // sector 0 is the boot sector
        sysdisk_boot(copy_to);
        break;
    case 1: // sector 1 is FAT 1st copy
    case 2: // sector 2 is FAT 2nd copy
        sysdisk_fat(copy_to);
        break;
    case 3: // sector 3 is the directory entry
        sysdisk_dir(copy_to);
        break;
    default:
        sysdisk_file_read(lba, copy_to);
        break;
	}

	return 0;
}

int cupkee_sysdisk_write(uint32_t lba, const uint8_t *copy_from)
{
    if (lba >= ROOT_START_SECTOR && lba < ROOT_END_SECTOR) {
        sysdisk_write_parse(copy_from);
        curr_state = 0;
    } else
    if (lba >= FILEDATA_START_SECTOR) {
        if (curr_state == 0) {
            sysdisk_write_init(copy_from);
        }
        if (curr_state == 1) {
            hw_storage_update(curr_bank, curr_offset, copy_from, SECTOR_SIZE);
            curr_offset += SECTOR_SIZE;
        }
    }

	return 0;
}

static void sysdisk_scan(void)
{
    app_size = hw_storage_data_length(HW_STORAGE_BANK_APP);
    if (app_size == 0) {
        app_data = APP_HEAD;
        app_size = strlen(app_data);
    } else {
        app_data = hw_storage_data_map(HW_STORAGE_BANK_APP);
    }
    app_start_sector = FILEDATA_START_SECTOR;

    cfg_size = hw_storage_data_length(HW_STORAGE_BANK_CFG);
    if (cfg_size == 0) {
        cfg_data = CFG_HEAD;
        cfg_size = strlen(cfg_data);
    } else {
        cfg_data = hw_storage_data_map(HW_STORAGE_BANK_CFG);
    }
    cfg_start_sector = app_start_sector + (hw_storage_size(HW_STORAGE_BANK_APP) / BYTES_PER_SECTOR);
}

void cupkee_sysdisk_init(void)
{
    curr_state = 0;
    curr_bank  = 0;
    curr_offset = 0;

    sysdisk_scan();
}

const char *cupkee_sysdisk_app_script(void)
{
    return app_data;
}

const char *cupkee_sysdisk_cfg_script(void)
{
    return cfg_data;
}

