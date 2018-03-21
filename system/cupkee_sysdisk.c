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

#include "cupkee.h"
#include "cupkee_sysdisk.h"

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

#define APP_HEAD    "/* CUPKEE APP */"

static const char *app_head = APP_HEAD;
static const char *app_data = NULL;
static uint16_t app_size = 0;
static uint16_t write_start_sector = 0;
static uint8_t  write_state = 0;
static uint16_t write_offset = 0;

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
    sysddisk_fat_set(fat, write_start_sector, app_size);
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

    sysdisk_dir_set(dir, "APP", "JS", START_CLUSTER(write_start_sector), app_size);
}

static void sysdisk_file_read(uint32_t lba, uint8_t *buf)
{
    int length = 0;

    if (lba >= write_start_sector) {
        uint32_t offset = (lba - write_start_sector) * SECTOR_SIZE;

        if (offset < app_size) {
            length = app_size - offset;
            if (length > SECTOR_SIZE) {
                length = SECTOR_SIZE;
            }
            memcpy(buf, app_data + offset, length);
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
        write_state = 1;
        write_offset = 0;
        cupkee_storage_erase(CUPKEE_STORAGE_BANK_APP);
    }
    return;
}

static int sysdisk_write_finish(const uint8_t *entry)
{
    uint16_t cluster;
    uint32_t max_size;

    if (memcmp(entry, "APP     JS ", 11)) {
        return 0;
    }

    cluster = entry[26] + entry[27] * 256;
    app_size = entry[28] + entry[29] * 256 + entry[30] * 0x10000 + entry[31] * 0x1000000;


    write_start_sector = FILEDATA_START_SECTOR + (cluster - 2) * SECTORS_PER_CLUSTER;
    max_size = cupkee_storage_size(CUPKEE_STORAGE_BANK_APP) - 1;
    if (max_size < app_size) {
        app_size = max_size;
    } else {
        uint8_t zero = 0;
        cupkee_storage_write(CUPKEE_STORAGE_BANK_APP, app_size, 1, &zero);
    }

    return 1; // done
}

static void sysdisk_write_parse(const uint8_t *info)
{
    int pos = 0;

    while (pos < SECTOR_SIZE) {
        sysdisk_write_finish(info + pos);
        pos += ROOT_ENTRY_LENGTH;
    }
}

static uint32_t sysdisk_app_scan(intptr_t base, uint32_t end)
{
    uint8_t *ptr = (uint8_t *) base;
    uint32_t i;

    for (i = 0; i < end; i++) {
        uint8_t d = ptr[i];

        if (d == 0 || d == 0xFF) {
            break;
        }
    }

    return i;
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
        write_state = 0;
    } else
    if (lba >= FILEDATA_START_SECTOR) {
        if (write_state == 0) {
            sysdisk_write_init(copy_from);
        }

        if (write_state == 1) {
            cupkee_storage_write(CUPKEE_STORAGE_BANK_APP,
                    write_offset, SECTOR_SIZE, copy_from);
            write_offset += SECTOR_SIZE;
        }
    }

	return 0;
}

void cupkee_sysdisk_init(void)
{
    intptr_t base = cupkee_storage_base(CUPKEE_STORAGE_BANK_APP);
    uint32_t size = cupkee_storage_size(CUPKEE_STORAGE_BANK_APP);

    app_size = sysdisk_app_scan(base, size);
    if (app_size == 0) {
        app_data = app_head;
        app_size = strlen(app_data);
    } else {
        app_data = (void *)base;
    }

    write_state = 0;
    write_offset = 0;
    write_start_sector = FILEDATA_START_SECTOR;
}

const char *cupkee_sysdisk_app(void)
{
    return ((intptr_t)app_data == (intptr_t) (app_head)) ? NULL : app_data;
}

