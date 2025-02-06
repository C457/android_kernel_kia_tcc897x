/*
 *  mobis_nvdata_handler.h
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
*/

#ifndef __MOBIS_NVDATA_HANDLER_H
#define __MOBIS_NVDATA_HANDLER_H

// ------------ For Common definition ----------------------------//
#define MOBIS_NVDATA_MMC_SECTOR_SIZE	0x200 // 512 byte

// ------------- For misc NV data partion ----------------------//

// Partion name <- please check GPT
#define MOBIS_NVDATA_MISC_PARTITION			"/dev/block/platform/bdm/by-name/misc"  // <- It will be changed by partition change

// Offset list (Unit : bytes) - please consider sector size
enum mobis_misc_nvdata_offset {
	// Please add offset of data that you want to use
	// Please refer kernel/include/linux/reboot.h
	MOBIS_NVDATA_MISC_REBOOT_REASON_OFFSET		= 0,
	MOBIS_NVDATA_MISC_PANIC_COUNT			= 2048,
	MOBIS_NVDATA_MISC_FAC_RESET_BY_PANIC		= 3072,

	// Please add offset of data that you want to use
	MOBIS_NVDATA_MISC_RGB_COLOR_TUNE		= 4096,
	MOBIS_NVDATA_MISC_TOUCH_UPDATE                  = 5120,
	MOBIS_NVDATA_MISC_SERDES_STATUS                 = 6144,

};

// ------------- For settings NV data partion ----------------------//

// Partion name <- please check GPT
#define MOBIS_NVDATA_SETTINGS_PARTITION			"/dev/block/platform/bdm/by-name/settings"  // <- It will be changed by partition change

// Offset list (Unit : bytes) - please consider sector size
enum mobis_settings_nvdata_offset {
        MOBIS_NVDATA_OFFSET_EM_SETTINGS         = 0,
        MOBIS_NVDATA_TELEMATICS_VARIANT_OFFSET  = 512,
        MOBIS_NVDATA_CPU_VARIANT_OFFSET         = 1024,
        MOBIS_NVDATA_OFFSET_IE_SETTINGS         = 1536,
        MOBIS_NVDATA_OFFSET_BOOT_ACTIVE_SETTINGS= 2048,
        MOBIS_NVDATA_OFFSET_ATTEMPT_WRITESKEY   = 2560,
        MOBIS_NVDATA_OFFSET_MUSIC_STREAMING_FREE_COUNT = 3072,
        // Please add offset of data that you want to use

};

// ------------- API ----------------------------------------//

int mobis_misc_nvdata_read(enum mobis_misc_nvdata_offset offset, char* buf, int size);
int mobis_misc_nvdata_write(enum mobis_misc_nvdata_offset offset, char* buf, int size);

int mobis_settings_nvdata_read(enum mobis_settings_nvdata_offset offset, char* buf, int size);
int mobis_settings_nvdata_write(enum mobis_settings_nvdata_offset offset, char* buf, int size);

#endif
