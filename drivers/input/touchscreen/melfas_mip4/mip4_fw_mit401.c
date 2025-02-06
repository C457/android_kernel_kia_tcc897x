/*
 * MELFAS MIP4 Touchscreen
 *
 * Copyright (C) 2016 MELFAS Inc.
 *
 *
 * mip4_fw_mit401.c : Firmware update functions for MIT401
 *
 * Version : 2016.12.09
 */

#include "mip4_ts.h"

#define BL_PAGE_SIZE		4096
#define BL_PACKET_SIZE	256

//Firmware Info
#define FW_CHIP_CODE	"T4HA"

#define FW_TYPE_TAIL

/*
* Firmware binary tail info
*/
struct melfas_bin_tail {
	u8 tail_mark[4];
	char chip_name[4];
	u32 bin_start_addr;
	u32 bin_length;

	u16 ver_boot;
	u16 ver_core;
	u16 ver_app;
	u16 ver_param;
	u8 boot_start;
	u8 boot_end;
	u8 core_start;
	u8 core_end;
	u8 app_start;
	u8 app_end;
	u8 param_start;
	u8 param_end;

	u8 checksum_type;
	u8 hw_category;
	u16 param_id;
	u32 param_length;
	u32 build_date;
	u32 build_time;

	u32 reserved1;
	u32 reserved2;
	u16 reserved3;
	u16 tail_size;
	u32 crc;
} __attribute__ ((packed));

#define MELFAS_BIN_TAIL_MARK	{0x4D, 0x42, 0x54, 0x01}	// M B T 0x01
#define MELFAS_BIN_TAIL_SIZE		64

/*
* Read status
*/
static int bl_read_status(struct mip4_ts_info *info)
{
	u8 write_buf[2];
	u8 result = 0;
	int cnt = 3000;
	int ret = 0;

	struct i2c_msg msg[2] = {
		{
			.addr = info->client->addr,
			.flags = 0,
			.buf = write_buf,
			.len = 2,
		}, {
			.addr = info->client->addr,
			.flags = I2C_M_RD,
			.buf = &result,
			.len = 1,
		},
	};
#if USE_INT_DBG
	dev_dbg(&info->client->dev, "%s [START]\n", __func__);
#endif
	write_buf[0] = MIP4_R0_BOOT;
	write_buf[1] = MIP4_R1_BOOT_STATUS;

	do {
		if (i2c_transfer(info->client->adapter, msg, ARRAY_SIZE(msg)) != ARRAY_SIZE(msg)) {
			dev_err(&info->client->dev, "%s [ERROR] i2c_transfer\n", __func__);
			ret = -1;
			goto MIP4_ERROR;
		}

		if (result == MIP4_BOOT_STATUS_DONE) {
#if USE_INT_DBG
			dev_dbg(&info->client->dev, "%s - Done\n", __func__);
#endif
			ret = 0;
			break;
		} else if (result == MIP4_BOOT_STATUS_BUSY) {
#if USE_INT_DBG
			dev_dbg(&info->client->dev, "%s - Busy\n", __func__);
#endif
			ret = -1;
			usleep_range(500, 500);
		} else if (result == MIP4_BOOT_STATUS_ERROR) {
			dev_dbg(&info->client->dev, "%s - Error\n", __func__);
			ret = -1;
			goto MIP4_ERROR;
		} else {
			dev_err(&info->client->dev, "%s [ERROR] wrong value [0x%02X]\n", __func__, result);
			ret = -1;
			usleep_range(500, 500);
		}
	} while (--cnt);

	if (!cnt) {
		dev_err(&info->client->dev, "%s [ERROR] count overflow - cnt [%d] status [0x%02X]\n", __func__, cnt, result);
		goto MIP4_ERROR;
	}
#if USE_INT_DBG
	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);
#endif
	return ret;

MIP4_ERROR:
	dev_err(&info->client->dev, "%s [ERROR]\n", __func__);
	return ret;
}

/*
* Change mode
*/
static int bl_change_mode(struct mip4_ts_info *info, u8 mode)
{
	u8 write_buf[8];
	u8 read_buf[8];
	int cnt = 10;
	int ret = 0;

	struct i2c_msg msg[2] = {
		{
			.addr = info->client->addr,
			.flags = 0,
			.buf = write_buf,
			.len = 2,
		}, {
			.addr = info->client->addr,
			.flags = I2C_M_RD,
			.buf = read_buf,
			.len = 1,
		},
	};

	dev_dbg(&info->client->dev, "%s [START]\n", __func__);

	do {
		//Write
		write_buf[0] = MIP4_R0_BOOT;
		write_buf[1] = MIP4_R1_BOOT_MODE;
		write_buf[2] = mode;
		if (i2c_master_send(info->client, write_buf, 3) != 3) {
			dev_err(&info->client->dev, "%s [ERROR] i2c_master_send\n", __func__);
			goto MIP4_ERROR;
		}
		dev_dbg(&info->client->dev, "%s - Write : Mode [%d]\n", __func__, mode);

		//Wait
		msleep(1000);

		//Read
		write_buf[0] = MIP4_R0_BOOT;
		write_buf[1] = MIP4_R1_BOOT_MODE;
		if (i2c_transfer(info->client->adapter, msg, ARRAY_SIZE(msg)) != ARRAY_SIZE(msg)) {
			dev_err(&info->client->dev, "%s [ERROR] i2c_transfer\n", __func__);
			ret = -1;
			goto MIP4_ERROR;
		}
		dev_dbg(&info->client->dev, "%s - Read : Mode [%d]\n", __func__, read_buf[0]);

		if (read_buf[0] == mode) {
			break;
		}
	} while (--cnt);

	if (!cnt) {
		dev_err(&info->client->dev, "%s [ERROR] count overflow - cnt [%d]\n", __func__, cnt);
		goto MIP4_ERROR;
	}

	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);
	return ret;

MIP4_ERROR:
	dev_err(&info->client->dev, "%s [ERROR]\n", __func__);
	return ret;
}

/*
* Read info
*/
static int bl_read_info(struct mip4_ts_info *info, u16 *buf_addr)
{
	u8 write_buf[8];
	u8 read_buf[8];
	int ret = 0;

	struct i2c_msg msg[2] = {
		{
			.addr = info->client->addr,
			.flags = 0,
			.buf = write_buf,
			.len = 2,
		}, {
			.addr = info->client->addr,
			.flags = I2C_M_RD,
			.buf = read_buf,
			.len = 2,
		},
	};

	dev_dbg(&info->client->dev, "%s [START]\n", __func__);

	write_buf[0] = MIP4_R0_BOOT;
	write_buf[1] = MIP4_R1_BOOT_BUF_ADDR;
	if (i2c_transfer(info->client->adapter, msg, ARRAY_SIZE(msg)) != ARRAY_SIZE(msg)) {
		dev_err(&info->client->dev, "%s [ERROR] i2c_transfer\n", __func__);
		ret = -1;
		goto MIP4_ERROR;
	}

	*buf_addr = (u16)((read_buf[1] << 8) | read_buf[0]);
	dev_dbg(&info->client->dev, "%s - Buf Addr [0x%04X]\n", __func__, *buf_addr);

	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);
	return ret;

MIP4_ERROR:
	dev_err(&info->client->dev, "%s [ERROR]\n", __func__);
	return ret;
}

/*
* Erase Mass
*/
static int bl_erase_mass(struct mip4_ts_info *info)
{
	u8 write_buf[3];

	dev_dbg(&info->client->dev, "%s [START]\n", __func__);

	//Command
	write_buf[0] = MIP4_R0_BOOT;
	write_buf[1] = MIP4_R1_BOOT_CMD;
	write_buf[2] = MIP4_BOOT_CMD_MASS_ERASE;
	if (i2c_master_send(info->client, write_buf, 3) != 3) {
		dev_err(&info->client->dev, "%s [ERROR] i2c_master_send\n", __func__);
		goto MIP4_ERROR;
	}

	//Status
	if (bl_read_status(info) != 0) {
		goto MIP4_ERROR;
	}

	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);
	return 0;

MIP4_ERROR:
	dev_err(&info->client->dev, "%s [ERROR]\n", __func__);
	return -1;
}

/*
* Write Page
*/
static int bl_write_page(struct mip4_ts_info *info, int offset, const u8 *data, int length, int buf_addr)
{
	u8 write_buf[2 + BL_PACKET_SIZE];
	int buf_offset = 0;
#if USE_INT_DBG
	dev_dbg(&info->client->dev, "%s [START]\n", __func__);
#endif

	if (length > BL_PACKET_SIZE) {
		dev_err(&info->client->dev, "%s [ERROR] packet length overflow\n", __func__);
		goto MIP4_ERROR;
	}

	//Addr
	write_buf[0] = MIP4_R0_BOOT;
	write_buf[1] = MIP4_R1_BOOT_TARGET_ADDR;
	write_buf[2] = (u8)(offset & 0xFF);
	write_buf[3] = (u8)((offset >> 8) & 0xFF);
	write_buf[4] = (u8)((offset >> 16) & 0xFF);
	write_buf[5] = (u8)((offset >> 24) & 0xFF);
	if (i2c_master_send(info->client, write_buf, 6) != 6) {
		dev_err(&info->client->dev, "%s [ERROR] i2c_master_send\n", __func__);
		goto MIP4_ERROR;
	}
#if USE_INT_DBG
	dev_dbg(&info->client->dev, "%s - Addr [0x%06X]\n", __func__, offset);
#endif

	//Size
	write_buf[0] = MIP4_R0_BOOT;
	write_buf[1] = MIP4_R1_BOOT_SIZE;
	write_buf[2] = (u8)(length & 0xFF);
	write_buf[3] = (u8)((length >> 8) & 0xFF);
	write_buf[4] = (u8)((length >> 16) & 0xFF);
	write_buf[5] = (u8)((length >> 24) & 0xFF);
	if (i2c_master_send(info->client, write_buf, 6) != 6) {
		dev_err(&info->client->dev, "%s [ERROR] i2c_master_send\n", __func__);
		goto MIP4_ERROR;
	}
#if USE_INT_DBG
	dev_dbg(&info->client->dev, "%s - Size [%d]\n", __func__, length);
#endif

	//Data
	for (buf_offset = 0; buf_offset < length; buf_offset += BL_PACKET_SIZE) {
		write_buf[0] = (u8)(((buf_addr + buf_offset) >> 8) & 0xFF);
		write_buf[1] = (u8)((buf_addr + buf_offset) & 0xFF);
		memcpy(&write_buf[2], &data[buf_offset], BL_PACKET_SIZE);
		if (i2c_master_send(info->client, write_buf, (2 + BL_PACKET_SIZE)) != (2 + BL_PACKET_SIZE)) {
			dev_err(&info->client->dev, "%s [ERROR] i2c_master_send\n", __func__);
			goto MIP4_ERROR;
		}
#if USE_INT_DBG
		dev_dbg(&info->client->dev, "%s - PacketSize [%d] BufOffset [0x%04X]\n", __func__, BL_PACKET_SIZE, buf_offset);
#endif
	}

	//Command
	write_buf[0] = MIP4_R0_BOOT;
	write_buf[1] = MIP4_R1_BOOT_CMD;
	write_buf[2] = MIP4_BOOT_CMD_WRITE;
	if (i2c_master_send(info->client, write_buf, 3) != 3) {
		dev_err(&info->client->dev, "%s [ERROR] i2c_master_send\n", __func__);
		goto MIP4_ERROR;
	}

	//Status
	if (bl_read_status(info) != 0) {
		goto MIP4_ERROR;
	}
#if USE_INT_DBG
	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);
#endif
	return 0;

MIP4_ERROR:
	dev_err(&info->client->dev, "%s [ERROR]\n", __func__);
	return -1;
}

/*
* Read Page
*/
static int bl_read_page(struct mip4_ts_info *info, int offset, u8 *data, int length, int buf_addr)
{
	u8 write_buf[8];
	u8 read_buf[BL_PACKET_SIZE];
	int buf_offset = 0;

	struct i2c_msg msg[2] = {
		{
			.addr = info->client->addr,
			.flags = 0,
			.buf = write_buf,
			.len = 2,
		}, {
			.addr = info->client->addr,
			.flags = I2C_M_RD,
			.buf = read_buf,
			.len = BL_PACKET_SIZE,
		},
	};
#if USE_INT_DBG
	dev_dbg(&info->client->dev, "%s [START]\n", __func__);
#endif
	//Addr
	write_buf[0] = MIP4_R0_BOOT;
	write_buf[1] = MIP4_R1_BOOT_TARGET_ADDR;
	write_buf[2] = (u8)(offset & 0xFF);
	write_buf[3] = (u8)((offset >> 8) & 0xFF);
	write_buf[4] = (u8)((offset >> 16) & 0xFF);
	write_buf[5] = (u8)((offset >> 24) & 0xFF);
	if (i2c_master_send(info->client, write_buf, 6) != 6) {
		dev_err(&info->client->dev, "%s [ERROR] i2c_master_send\n", __func__);
		goto MIP4_ERROR;
	}
#if USE_INT_DBG
	dev_dbg(&info->client->dev, "%s - Addr [0x%06X]\n", __func__, offset);
#endif
	//Size
	write_buf[0] = MIP4_R0_BOOT;
	write_buf[1] = MIP4_R1_BOOT_SIZE;
	write_buf[2] = (u8)(length & 0xFF);
	write_buf[3] = (u8)((length >> 8) & 0xFF);
	write_buf[4] = (u8)((length >> 16) & 0xFF);
	write_buf[5] = (u8)((length >> 24) & 0xFF);
	if (i2c_master_send(info->client, write_buf, 6) != 6) {
		dev_err(&info->client->dev, "%s [ERROR] i2c_master_send\n", __func__);
		goto MIP4_ERROR;
	}
#if USE_INT_DBG
	dev_dbg(&info->client->dev, "%s - Size [%d]\n", __func__, length);
#endif
	//Command
	write_buf[0] = MIP4_R0_BOOT;
	write_buf[1] = MIP4_R1_BOOT_CMD;
	write_buf[2] = MIP4_BOOT_CMD_READ;
	if (i2c_master_send(info->client, write_buf, 3) != 3) {
		dev_err(&info->client->dev, "%s [ERROR] i2c_master_send\n", __func__);
		goto MIP4_ERROR;
	}

	//Status
	if (bl_read_status(info) != 0) {
		goto MIP4_ERROR;
	}

	//Read
	for (buf_offset = 0; buf_offset < length; buf_offset += BL_PACKET_SIZE) {
		write_buf[0] = (u8)(((buf_addr + buf_offset) >> 8) & 0xFF);
		write_buf[1] = (u8)((buf_addr + buf_offset) & 0xFF);
		if (i2c_transfer(info->client->adapter, msg, ARRAY_SIZE(msg)) != ARRAY_SIZE(msg)) {
			dev_err(&info->client->dev, "%s [ERROR] i2c_transfer\n", __func__);
			goto MIP4_ERROR;
		}
		memcpy(&data[buf_offset], read_buf, BL_PACKET_SIZE);
#if USE_INT_DBG
		dev_dbg(&info->client->dev, "%s - PacketSize [%d] BufOffset [0x%04X]\n", __func__, BL_PACKET_SIZE, buf_offset);
#endif
	}
#if USE_INT_DBG
	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);
#endif
	return 0;

MIP4_ERROR:
	dev_err(&info->client->dev, "%s [ERROR]\n", __func__);
	return -1;
}

/*
* Earse Page
*/
static int bl_erase_page(struct mip4_ts_info *info, unsigned int addr)
{
	u8 write_buf[6];

	dev_dbg(&info->client->dev, "%s [START]\n", __func__);

	//Addr
	write_buf[0] = MIP4_R0_BOOT;
	write_buf[1] = MIP4_R1_BOOT_TARGET_ADDR;
	write_buf[2] = (u8)(addr & 0xFF);
	write_buf[3] = (u8)((addr >> 8) & 0xFF);
	write_buf[4] = (u8)((addr >> 16) & 0xFF);
	write_buf[5] = (u8)((addr >> 24) & 0xFF);
	if (i2c_master_send(info->client, write_buf, 6) != 6) {
		dev_err(&info->client->dev, "%s [ERROR] i2c_master_send\n", __func__);
		goto MIP4_ERROR;
	}
	dev_dbg(&info->client->dev, "%s - Addr [0x%06X]\n", __func__, addr);

	//Size
	write_buf[0] = MIP4_R0_BOOT;
	write_buf[1] = MIP4_R1_BOOT_SIZE;
	write_buf[2] = (u8)(BL_PAGE_SIZE & 0xFF);
	write_buf[3] = (u8)((BL_PAGE_SIZE >> 8) & 0xFF);
	write_buf[4] = (u8)((BL_PAGE_SIZE >> 16) & 0xFF);
	write_buf[5] = (u8)((BL_PAGE_SIZE >> 24) & 0xFF);
	if (i2c_master_send(info->client, write_buf, 6) != 6) {
		dev_err(&info->client->dev, "%s [ERROR] i2c_master_send\n", __func__);
		goto MIP4_ERROR;
	}
	dev_dbg(&info->client->dev, "%s - Size [%d]\n", __func__, BL_PAGE_SIZE);

	//Command
	write_buf[0] = MIP4_R0_BOOT;
	write_buf[1] = MIP4_R1_BOOT_CMD;
	write_buf[2] = MIP4_BOOT_CMD_ERASE;
	if (i2c_master_send(info->client, write_buf, 3) != 3) {
		dev_err(&info->client->dev, "%s [ERROR] i2c_master_send\n", __func__);
		goto MIP4_ERROR;
	}

	//Status
	if (bl_read_status(info) != 0) {
		goto MIP4_ERROR;
	}

	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);
	return 0;

MIP4_ERROR:
	dev_err(&info->client->dev, "%s [ERROR]\n", __func__);
	return -1;
}

/*
* Start bootloader mode
*/
static int bl_enter(struct mip4_ts_info *info)
{
	dev_dbg(&info->client->dev, "%s [START]\n", __func__);

	if (bl_change_mode(info, MIP4_BOOT_MODE_BOOT) != 0) {
		goto MIP4_ERROR;
	}

	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);
	return 0;

MIP4_ERROR:
	dev_err(&info->client->dev, "%s [ERROR]\n", __func__);
	return -1;
}

/*
* Exit bootloader mode
*/
static int bl_exit(struct mip4_ts_info *info)
{
	dev_dbg(&info->client->dev, "%s [START]\n", __func__);

	if (bl_change_mode(info, MIP4_BOOT_MODE_APP) != 0) {
		goto MIP4_ERROR;
	}

	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);
	return 0;

MIP4_ERROR:
	dev_err(&info->client->dev, "%s [ERROR]\n", __func__);
	return -1;
}

/*
* Flash chip firmware (main function)
*/
int mip4_ts_flash_fw(struct mip4_ts_info *info, const u8 *fw_data, size_t fw_size, bool force, bool section)
{
	struct i2c_client *client = info->client;
	struct melfas_bin_tail *bin_info;
	int ret = 0;
	int retry = 3;
	u8 rbuf[BL_PACKET_SIZE];
	u32 flash_addr = 0;
	u32 flash_addr_start = 0;
	u32 flash_addr_end = 0;
	u32 flash_addr_offset = 0;
	u32 bin_addr = 0;
	u32 bin_addr_start = 0;
	u32 bin_addr_end = 0;
	int bin_size = 0;
	u8 *bin_data;
	u16 tail_size = 0;
	u8 tail_mark[4] = MELFAS_BIN_TAIL_MARK;
	u16 ver_chip[FW_MAX_SECT_NUM];
	u16 buf_addr = 0;
	u8 bin_type = 0;

	/* Check tail size */
	tail_size = (fw_data[fw_size - 5] << 8) | fw_data[fw_size - 6];
	if (tail_size != MELFAS_BIN_TAIL_SIZE) {
		dev_err(&client->dev, "%s [ERROR] wrong tail size [%d]\n", __func__, tail_size);
		ret = fw_err_file_type;
		goto error_file;
	}

	/* Check bin format */
	if (memcmp(&fw_data[fw_size - tail_size], tail_mark, 4)) {
		dev_err(&client->dev, "%s [ERROR] wrong tail mark\n", __func__);
		ret = fw_err_file_type;
		goto error_file;
	}

	/* Read bin info */
	bin_info = (struct melfas_bin_tail *)&fw_data[fw_size - tail_size];

#if FW_UPDATE_DEBUG
	print_hex_dump(KERN_ERR, MIP4_TS_DEVICE_NAME " Bin Info : ", DUMP_PREFIX_OFFSET, 16, 1, bin_info, tail_size, false);
#endif

	/* Check bin size */
	if ((bin_info->bin_start_addr + bin_info->bin_length) == fw_size) {
		bin_size = bin_info->bin_start_addr + bin_info->bin_length;
		bin_addr_start = bin_info->bin_start_addr;
		bin_addr_end = bin_size;
		flash_addr_offset = 0;
	} else if (bin_info->bin_length == fw_size) {
		bin_size = bin_info->bin_length;
		bin_addr_start = 0;
		bin_addr_end = bin_size;
		flash_addr_offset = bin_info->bin_start_addr;
	} else {
		dev_err(&client->dev, "%s [ERROR] wrong bin size\n", __func__);
		ret = fw_err_file_type;
		goto error_file;
	}

	flash_addr_start = bin_addr_start + flash_addr_offset;
	flash_addr_end = bin_addr_end + flash_addr_offset;

	dev_dbg(&client->dev, "%s - Bin : Size[%d] Start[0x%04X] End[0x%04X]\n", __func__, bin_size, bin_addr_start, bin_addr_end);
	dev_dbg(&client->dev, "%s - Flash : Size[%d] Start[0x%04X] End[0x%04X] Offset[0x%04X]\n", __func__, bin_size, flash_addr_start, flash_addr_end, flash_addr_offset);

	/* Check F/W version */
	dev_info(&client->dev, "F/W file version [0x%04X 0x%04X 0x%04X 0x%04X]\n", bin_info->ver_boot, bin_info->ver_core, bin_info->ver_app, bin_info->ver_param);

	/* Check F/W type */
	if ((bin_info->ver_boot != 0xEEEE) && (bin_info->ver_boot != 0xFFFF) && (bin_info->ver_core == 0xEEEE) && (bin_info->ver_app == 0xEEEE) && (bin_info->ver_param == 0xEEEE)) {
		bin_type = 0;
		dev_info(&client->dev, "F/W type [Bootloader]\n");
	} else if ((bin_info->ver_boot == 0xEEEE) && (bin_info->ver_core != 0xEEEE) && (bin_info->ver_core != 0xFFFF) && (bin_info->ver_app != 0xEEEE) && (bin_info->ver_app != 0xFFFF) && (bin_info->ver_param != 0xEEEE) && (bin_info->ver_param != 0xFFFF)) {
		bin_type = 1;
		dev_info(&client->dev, "F/W type [Main]\n");
	} else {
		dev_err(&client->dev,
			"%s wrong file type\n", __func__);
		ret = fw_err_file_type;
		goto error_file;
	}

	if (force == true) {
		/* Force update */
		dev_info(&client->dev, "Skip chip firmware version check\n");
	} else {
		/* Read firmware version from chip */
		retry = 2;
		while (retry--) {
			if (mip4_ts_get_fw_version_u16(info, ver_chip)) {
				mip4_ts_reset(info);
			} else {
				if((ver_chip[3] != 0) && (ver_chip[2] != 0))
					break;
			}
		}

		if (retry < 0) {
			dev_err(&client->dev, "%s [ERROR] Unknown chip firmware version\n", __func__);
		} else {
			dev_info(&client->dev, "Chip firmware version [0x%04X 0x%04X 0x%04X 0x%04X]\n", ver_chip[0], ver_chip[1], ver_chip[2], ver_chip[3]);

			/* Compare version */
			if (bin_type == 0) {
			        if (ver_chip[0] >= bin_info->ver_boot) {
				        dev_info(&client->dev, "Bootloader is already up-to-date\n");
 	  	        		ret = fw_err_uptodate;
					goto uptodate;
				}
			} else if (bin_type == 1) {
				if ((ver_chip[1] >= bin_info->ver_core) && (ver_chip[2] >= bin_info->ver_app) && (ver_chip[3] >= bin_info->ver_param)) {
					dev_info(&client->dev, "Firmware is already up-to-date\n");
					ret = fw_err_uptodate;
					goto uptodate;
				}
			} else {
				if ((ver_chip[0] >= bin_info->ver_boot) && (ver_chip[1] >= bin_info->ver_core) && (ver_chip[2] >= bin_info->ver_app) && (ver_chip[3] >= bin_info->ver_param)) {
					dev_info(&client->dev, "Chip firmware is already up-to-date\n");
					ret = fw_err_uptodate;
					goto uptodate;
				}
			}
		}
	}

	/* Read bin data */
	bin_data = kzalloc(sizeof(u8) * (bin_size), GFP_KERNEL);
	memcpy(bin_data, fw_data, bin_size);

	/* Enter bootloader mode */
	dev_dbg(&client->dev, "%s - Enter bootloader mode\n", __func__);

	retry = 3;
	while (retry--) {
		if (bl_enter(info) != 0) {
			dev_err(&client->dev, "%s [ERROR] bl_enter\n", __func__);
		} else {
			break;
		}
	}
	if (retry < 0) {
		dev_err(&client->dev, "%s [ERROR] bl_enter retry limit\n", __func__);
		ret = fw_err_download;
		goto error_update;
	}

	/* Read info */
	if (bl_read_info(info, &buf_addr)) {
		dev_err(&client->dev, "%s [ERROR] bl_read_info\n", __func__);
		ret = fw_err_download;
		goto error_update;
	}
	dev_dbg(&client->dev, "%s - Buffer Addr [0x%04X]\n", __func__, buf_addr);

	/* Erase */
	if (bin_type == 0) {
		for (flash_addr = flash_addr_start; flash_addr < flash_addr_end; flash_addr += BL_PAGE_SIZE) {
			dev_dbg(&client->dev, "%s - Erase page : addr[0x%08X]\n", __func__, flash_addr);
			if (bl_erase_page(info, flash_addr)) {
				dev_err(&client->dev, "%s [ERROR] bl_erase_page : addr[0x%08X]\n", __func__, flash_addr);
				ret = fw_err_download;
				goto error_update;
			}
		}
	} else {
		dev_dbg(&client->dev, "%s - Erase mass\n", __func__);
		if (bl_erase_mass(info)) {
			dev_err(&client->dev, "%s [ERROR] bl_erase_mass\n", __func__);
			ret = fw_err_download;
			goto error_update;
		}
	}

	/* Write & Verify */
	dev_dbg(&client->dev, "%s - Write & Verify\n", __func__);

	dev_dbg(&client->dev, "%s - Size : Page[%d] Packet[%d]\n", __func__, BL_PAGE_SIZE, BL_PACKET_SIZE);

	for (flash_addr = flash_addr_start; flash_addr < flash_addr_end; flash_addr += BL_PACKET_SIZE) {
		bin_addr = flash_addr - flash_addr_offset;

		/* Write */
		//dev_dbg(&client->dev, "%s - Write : addr - bin[0x%08X] flash[0x%08X]\n", __func__, bin_addr, flash_addr);

		if (bl_write_page(info, flash_addr, &bin_data[bin_addr], BL_PACKET_SIZE, buf_addr)) {
			dev_err(&client->dev, "%s [ERROR] bl_write_page : addr - bin[0x%08X] flash[0x%08X]\n", __func__, bin_addr, flash_addr);
			ret = fw_err_download;
			goto error_update;
		}

		/* Verify */
		//dev_dbg(&client->dev, "%s - Read : addr - bin[0x%08X] flash[0x%08X]\n", __func__, bin_addr, flash_addr);
		if (bl_read_page(info, flash_addr, rbuf, BL_PACKET_SIZE, buf_addr)) {
			dev_err(&client->dev, "%s [ERROR] bl_read_page : addr - bin[0x%08X] flash[0x%08X]\n", __func__, bin_addr, flash_addr);
			ret = fw_err_download;
			goto error_update;
		}

#if FW_UPDATE_DEBUG
		print_hex_dump(KERN_ERR, MIP4_TS_DEVICE_NAME " F/W File : ", DUMP_PREFIX_OFFSET, 16, 1, &bin_data[bin_addr], BL_PACKET_SIZE, false);
		print_hex_dump(KERN_ERR, MIP4_TS_DEVICE_NAME " F/W Chip : ", DUMP_PREFIX_OFFSET, 16, 1, rbuf, BL_PACKET_SIZE, false);
#endif

		if (memcmp(rbuf, &bin_data[bin_addr], BL_PACKET_SIZE)) {
			dev_err(&client->dev, "%s [ERROR] Verify failed : addr - bin[0x%08X] flash[0x%08X]\n", __func__, bin_addr, flash_addr);
			ret = fw_err_download;
			goto error_update;
		}

	}

	/* Exit bootloader mode */
	dev_dbg(&client->dev, "%s - Exit bootloader mode\n", __func__);

	if (bl_exit(info) != 0) {
		dev_err(&client->dev, "%s [ERROR] bl_exit\n", __func__);
		ret = fw_err_download;
		goto error_update;
	}

	/* Reset chip */
	mip4_ts_reset(info);

	/* Check chip firmware version */
	if (mip4_ts_get_fw_version_u16(info, ver_chip)) {
		dev_err(&client->dev, "%s [ERROR] Unknown chip firmware version\n", __func__);
		ret = fw_err_download;
		goto error_update;
	} else {
		if (bin_type == 0) {
			if (ver_chip[0] == bin_info->ver_boot) {
				dev_dbg(&client->dev, "%s - Version check OK\n", __func__);
			} else {
				dev_err(&client->dev, "Version mismatch after flash.\n");
				dev_err(&client->dev, "Chip[0x%04X 0xEEEE 0xEEEE 0xEEEE]\n", ver_chip[0]);
				dev_err(&client->dev, "File[0x%04X 0xEEEE 0xEEEE 0xEEEE]\n", bin_info->ver_boot);
				ret = fw_err_download;
				goto error_update;
			}
		} else if (bin_type == 1) {
			if ((ver_chip[1] == bin_info->ver_core) && (ver_chip[2] == bin_info->ver_app) && (ver_chip[3] == bin_info->ver_param)) {
				dev_dbg(&client->dev, "%s - Version check OK\n", __func__);
			} else {
				dev_err(&client->dev, "Version mismatch after flash.\n");
				dev_err(&client->dev, "Chip[0xEEEE 0x%04X 0x%04X 0x%04X]\n", ver_chip[1], ver_chip[2], ver_chip[3]);
				dev_err(&client->dev, "File[0xEEEE 0x%04X 0x%04X 0x%04X]\n", bin_info->ver_core, bin_info->ver_app, bin_info->ver_param);
				ret = fw_err_download;
				goto error_update;
			}
		}
	}

	kfree(bin_data);

uptodate:
	goto exit;

error_update:
	kfree(bin_data);

	/* Reset chip */
	mip4_ts_reset(info);

error_file:
	dev_err(&client->dev, "%s [ERROR]\n", __func__);

exit:
	return ret;
}

/*
* Get version of F/W bin file
*/
int mip4_ts_bin_fw_version(struct mip4_ts_info *info, const u8 *fw_data, size_t fw_size, u8 *ver_buf)
{
	struct melfas_bin_tail *bin_info;
	u16 tail_size = 0;
	u8 tail_mark[4] = MELFAS_BIN_TAIL_MARK;

	dev_dbg(&info->client->dev,"%s [START]\n", __func__);

	//Check tail size
	tail_size = (fw_data[fw_size - 5] << 8) | fw_data[fw_size - 6];
	if (tail_size != MELFAS_BIN_TAIL_SIZE) {
		dev_err(&info->client->dev, "%s [ERROR] wrong tail size [%d]\n", __func__, tail_size);
		goto MIP4_ERROR;
	}

	//Check bin format
	if (memcmp(&fw_data[fw_size - tail_size], tail_mark, 4)) {
		dev_err(&info->client->dev, "%s [ERROR] wrong tail mark\n", __func__);
		goto MIP4_ERROR;
	}

	//Read bin info
	bin_info = (struct melfas_bin_tail *)&fw_data[fw_size - tail_size];

	//F/W version
	ver_buf[0] = (bin_info->ver_boot >> 8) & 0xFF;
	ver_buf[1] = (bin_info->ver_boot) & 0xFF;
	ver_buf[2] = (bin_info->ver_core >> 8) & 0xFF;
	ver_buf[3] = (bin_info->ver_core) & 0xFF;
	ver_buf[4] = (bin_info->ver_app >> 8) & 0xFF;
	ver_buf[5] = (bin_info->ver_app) & 0xFF;
	ver_buf[6] = (bin_info->ver_param >> 8) & 0xFF;
	ver_buf[7] = (bin_info->ver_param) & 0xFF;

	dev_dbg(&info->client->dev,"%s [DONE]\n", __func__);
	return 0;

MIP4_ERROR:
	dev_err(&info->client->dev,"%s [ERROR]\n", __func__);
	return 1;
}

