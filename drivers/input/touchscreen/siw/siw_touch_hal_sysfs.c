/*
 * siw_touch_hal_sysfs.c - SiW touch hal driver
 *
 * Copyright (C) 2016 Silicon Works - http://www.siliconworks.co.kr
 * Author: Hyunho Kim <kimhh@siliconworks.co.kr>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/async.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/irqreturn.h>
#include <linux/slab.h>
#include <linux/pm.h>
#include <linux/gpio.h>
#include <linux/string.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_device.h>
#include <linux/firmware.h>
#include <asm/page.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/memory.h>

#include "siw_touch.h"
#include "siw_touch_hal.h"
#include "siw_touch_bus.h"
#include "siw_touch_event.h"
#include "siw_touch_gpio.h"
#include "siw_touch_irq.h"
#include "siw_touch_sys.h"

#define DES_ADDRESS 0x48
#define DES_DELAY 0x00
#define SER_ADDRESS 0x40


//#define __SIW_USE_BUS_TEST


#define siw_hal_sysfs_err_invalid_param(_dev)	\
		t_dev_err(_dev, "Invalid param\n");

#define _reg_snprintf(_buf, _size, _reg, _element)	\
		siw_snprintf(_buf, _size, "# 0x%04X [%s]\n", _reg->_element, #_element)

int siw_debug;
int siw_touch_cnt = 5;

static int ori_show_reg_list(struct device *dev, char *buf, int size)
{
	struct siw_touch_chip *chip = to_touch_chip(dev);
//	struct siw_ts *ts = chip->ts;
	struct siw_hal_reg *reg = chip->reg;

	size += siw_snprintf(buf, size, "# Reg. Map List\n");

	size += _reg_snprintf(buf, size, reg, spr_chip_id);
	size += _reg_snprintf(buf, size, reg, spr_rst_ctl);
	size += _reg_snprintf(buf, size, reg, spr_boot_ctl);
	size += _reg_snprintf(buf, size, reg, spr_sram_ctl);
	size += _reg_snprintf(buf, size, reg, spr_boot_status);
	size += _reg_snprintf(buf, size, reg, spr_subdisp_status);
	size += _reg_snprintf(buf, size, reg, spr_code_offset);
	size += _reg_snprintf(buf, size, reg, tc_ic_status);
	size += _reg_snprintf(buf, size, reg, tc_status);
	size += _reg_snprintf(buf, size, reg, tc_version);
	size += _reg_snprintf(buf, size, reg, tc_product_id1);
	size += _reg_snprintf(buf, size, reg, tc_product_id2);
	size += _reg_snprintf(buf, size, reg, tc_version_ext);
	size += _reg_snprintf(buf, size, reg, info_chip_version);
	size += _reg_snprintf(buf, size, reg, info_lot_num);
	size += _reg_snprintf(buf, size, reg, info_serial_num);
	size += _reg_snprintf(buf, size, reg, info_date);
	size += _reg_snprintf(buf, size, reg, info_time);
	size += _reg_snprintf(buf, size, reg, cmd_abt_loc_x_start_read);
	size += _reg_snprintf(buf, size, reg, cmd_abt_loc_x_end_read);
	size += _reg_snprintf(buf, size, reg, cmd_abt_loc_y_start_read);
	size += _reg_snprintf(buf, size, reg, cmd_abt_loc_y_end_read);
	size += _reg_snprintf(buf, size, reg, code_access_addr);
	size += _reg_snprintf(buf, size, reg, data_i2cbase_addr);
	size += _reg_snprintf(buf, size, reg, prd_tcm_base_addr);
	size += _reg_snprintf(buf, size, reg, tc_device_ctl);
	size += _reg_snprintf(buf, size, reg, tc_interrupt_ctl);
	size += _reg_snprintf(buf, size, reg, tc_interrupt_status);
	size += _reg_snprintf(buf, size, reg, tc_drive_ctl);
#if defined(__SIW_CONFIG_KNOCK)
	size += _reg_snprintf(buf, size, reg, tci_enable_w);
	size += _reg_snprintf(buf, size, reg, tap_count_w);
	size += _reg_snprintf(buf, size, reg, min_intertap_w);
	size += _reg_snprintf(buf, size, reg, max_intertap_w);
	size += _reg_snprintf(buf, size, reg, touch_slop_w);
	size += _reg_snprintf(buf, size, reg, tap_distance_w);
	size += _reg_snprintf(buf, size, reg, int_delay_w);
	size += _reg_snprintf(buf, size, reg, act_area_x1_w);
	size += _reg_snprintf(buf, size, reg, act_area_y1_w);
	size += _reg_snprintf(buf, size, reg, act_area_x2_w);
	size += _reg_snprintf(buf, size, reg, act_area_y2_w);
	size += _reg_snprintf(buf, size, reg, tci_debug_fail_ctrl);
	size += _reg_snprintf(buf, size, reg, tci_debug_fail_buffer);
	size += _reg_snprintf(buf, size, reg, tci_debug_fail_status);
#endif
#if defined(__SIW_CONFIG_SWIPE)
	size += _reg_snprintf(buf, size, reg, swipe_enable_w);
	size += _reg_snprintf(buf, size, reg, swipe_dist_w);
	size += _reg_snprintf(buf, size, reg, swipe_ratio_thr_w);
	size += _reg_snprintf(buf, size, reg, swipe_ratio_period_w);
	size += _reg_snprintf(buf, size, reg, swipe_ratio_dist_w);
	size += _reg_snprintf(buf, size, reg, swipe_time_min_w);
	size += _reg_snprintf(buf, size, reg, swipe_time_max_w);
	size += _reg_snprintf(buf, size, reg, swipe_act_area_x1_w);
	size += _reg_snprintf(buf, size, reg, swipe_act_area_y1_w);
	size += _reg_snprintf(buf, size, reg, swipe_act_area_x2_w);
	size += _reg_snprintf(buf, size, reg, swipe_act_area_y2_w);
	size += _reg_snprintf(buf, size, reg, swipe_fail_debug_r);
	size += _reg_snprintf(buf, size, reg, swipe_debug_r);
#endif
	size += _reg_snprintf(buf, size, reg, cmd_raw_data_report_mode_read);
	size += _reg_snprintf(buf, size, reg, cmd_raw_data_compress_write);
	size += _reg_snprintf(buf, size, reg, cmd_raw_data_report_mode_write);
	size += _reg_snprintf(buf, size, reg, spr_charger_status);
	size += _reg_snprintf(buf, size, reg, ime_state);
#if defined(__SIW_SUPPORT_ASC)
	size += _reg_snprintf(buf, size, reg, max_delta);
	size += _reg_snprintf(buf, size, reg, touch_max_w);
	size += _reg_snprintf(buf, size, reg, touch_max_r);
	size += _reg_snprintf(buf, size, reg, call_state);
#endif
	size += _reg_snprintf(buf, size, reg, tc_tsp_test_ctl);
	size += _reg_snprintf(buf, size, reg, tc_tsp_test_status);
	size += _reg_snprintf(buf, size, reg, tc_tsp_test_pf_result);
	size += _reg_snprintf(buf, size, reg, tc_tsp_test_off_info);
	size += _reg_snprintf(buf, size, reg, tc_flash_dn_status);
	size += _reg_snprintf(buf, size, reg, tc_confdn_base_addr);
	size += _reg_snprintf(buf, size, reg, tc_flash_dn_ctl);
	size += _reg_snprintf(buf, size, reg, raw_data_ctl_read);
	size += _reg_snprintf(buf, size, reg, raw_data_ctl_write);
	size += _reg_snprintf(buf, size, reg, serial_data_offset);

#if defined(__SIW_SUPPORT_WATCH)
	if (!touch_test_quirks(chip->ts, CHIP_QUIRK_NOT_SUPPORT_WATCH)) {
		size += _reg_snprintf(buf, size, reg, ext_watch_font_offset);
		size += _reg_snprintf(buf, size, reg, ext_watch_font_addr);
		size += _reg_snprintf(buf, size, reg, ext_watch_font_dn_addr_info);
		size += _reg_snprintf(buf, size, reg, ext_watch_font_crc);
		size += _reg_snprintf(buf, size, reg, ext_watch_dcs_ctrl);
		size += _reg_snprintf(buf, size, reg, ext_watch_mem_ctrl);
		size += _reg_snprintf(buf, size, reg, ext_watch_ctrl);
		size += _reg_snprintf(buf, size, reg, ext_watch_area_x);
		size += _reg_snprintf(buf, size, reg, ext_watch_area_y);
		size += _reg_snprintf(buf, size, reg, ext_watch_blink_area);

		switch (touch_chip_type(chip->ts)) {
		case CHIP_LG4895:
			/* SKIP : reg->ext_watch_lut not available */
			break;
		default:
			size += _reg_snprintf(buf, size, reg, ext_watch_lut);
			break;
		}

		size += _reg_snprintf(buf, size, reg, ext_watch_display_on);
		size += _reg_snprintf(buf, size, reg, ext_watch_display_status);
		size += _reg_snprintf(buf, size, reg, ext_watch_rtc_sct);
		size += _reg_snprintf(buf, size, reg, ext_watch_rtc_sctcnt);
		size += _reg_snprintf(buf, size, reg, ext_watch_rtc_capture);
		size += _reg_snprintf(buf, size, reg, ext_watch_rtc_ctst);
		size += _reg_snprintf(buf, size, reg, ext_watch_rtc_ecnt);
		size += _reg_snprintf(buf, size, reg, ext_watch_hour_disp);
		size += _reg_snprintf(buf, size, reg, ext_watch_blink_prd);
		size += _reg_snprintf(buf, size, reg, ext_watch_rtc_run);
		size += _reg_snprintf(buf, size, reg, ext_watch_position);
		size += _reg_snprintf(buf, size, reg, ext_watch_position_r);
		size += _reg_snprintf(buf, size, reg, ext_watch_state);
		size += _reg_snprintf(buf, size, reg, sys_dispmode_status);
	}
#endif	/* __SIW_SUPPORT_WATCH */

	/* SIW_SUPPORT_PRD */
	size += _reg_snprintf(buf, size, reg, prd_serial_tcm_offset);
	size += _reg_snprintf(buf, size, reg, prd_tc_mem_sel);
	size += _reg_snprintf(buf, size, reg, prd_tc_test_mode_ctl);
	size += _reg_snprintf(buf, size, reg, prd_m1_m2_raw_offset);
	size += _reg_snprintf(buf, size, reg, prd_tune_result_offset);
	size += _reg_snprintf(buf, size, reg, prd_open3_short_offset);
	size += _reg_snprintf(buf, size, reg, prd_ic_ait_start_reg);
	size += _reg_snprintf(buf, size, reg, prd_ic_ait_data_readystatus);

	return size;
}

static ssize_t show_reg_list(struct device *dev, char *buf)
{
	struct siw_touch_chip *chip = to_touch_chip(dev);
//	struct siw_ts *ts = chip->ts;
//	struct siw_hal_reg *reg = chip->reg;
	struct siw_hal_fw_info *fw = &chip->fw;
	u32 bootmode = 0;
	u32 boot_chk_offset = 0;
	int size = 0;
	int ret = 0;

	ret = siw_hal_get_boot_status(dev, &bootmode);
	if (ret < 0) {
		return (ssize_t)ret;
	}

	size += ori_show_reg_list(dev, buf, size);

	size += siw_snprintf(buf, size, "\n");

	if (fw->version_ext) {
		size += siw_snprintf(buf, size,
					">> version    : %08X, chip : %u, protocol : %u\n",
					fw->version_ext,
					fw->v.version.chip,
					fw->v.version.protocol);
	} else {
		size += siw_snprintf(buf, size,
					">> version    : v%u.%02u, chip : %u, protocol : %u\n",
					fw->v.version.major,
					fw->v.version.minor,
					fw->v.version.chip,
					fw->v.version.protocol);
	}

	size += siw_snprintf(buf, size,
				">> revision   : %d\n",
				fw->revision);

	size += siw_snprintf(buf, size,
				">> product id : %s\n",
				fw->product_id);

	boot_chk_offset = siw_hal_boot_sts_pos_busy(chip);
	size += siw_snprintf(buf, size,
				">> flash boot : %s(%s), crc %s  (0x%08X)\n",
				((bootmode >> (boot_chk_offset)) & 0x1) ? "BUSY" : "idle",
				((bootmode >> (boot_chk_offset + 1)) & 0x1) ? "done" : "booting",
				((bootmode >> (boot_chk_offset + 2)) & 0x1) ? "ERROR" : "ok",
				bootmode);

	size += siw_snprintf(buf, size,
				">> status     : type %d[%08Xh, %08Xh, %08Xh]\n",
				chip->status_type,
				chip->status_mask_normal,
				chip->status_mask_logging,
				chip->status_mask_reset);

	size += siw_snprintf(buf, size, "\n");

	return (ssize_t)size;
}

#define REG_BURST_MAX			512
#define REG_BURST_COL_PWR		4

static int ori_show_reg_ctrl_log_history(struct device *dev, char *buf)
{
	struct siw_touch_chip *chip = to_touch_chip(dev);
//	struct siw_ts *ts = chip->ts;
	struct siw_hal_reg_log *reg_log = chip->reg_log;
	char *dir_name;
	int reg_burst;
	int reg_err;
	int i;
	int size = 0;

	size += siw_snprintf(buf, size, "[Test History]\n");
	for (i = 0; i < REG_LOG_MAX; i++) {
		if (reg_log->dir) {
			dir_name = !!((reg_log->dir & REG_DIR_MASK) == REG_DIR_WR) ?
						"wr" : "rd";
			reg_burst = !!(reg_log->dir & (REG_DIR_ERR<<1));
			reg_err = !!(reg_log->dir & REG_DIR_ERR);
		} else {
			dir_name = "__";
			reg_burst = 0;
			reg_err = 0;
		}

		if (reg_burst) {
			size += siw_snprintf(buf, size, " %s: reg[0x%04X] = (burst) %s\n",
						dir_name, reg_log->addr,
						(reg_err) ? "(err)" : "");
		} else {
			size += siw_snprintf(buf, size, " %s: reg[0x%04X] = 0x%08X %s\n",
						dir_name, reg_log->addr, reg_log->data,
						(reg_err) ? "(err)" : "");
		}

		reg_log++;
	}

	return size;
}

static ssize_t show_reg_ctrl(struct device *dev, char *buf)
{
//	struct siw_touch_chip *chip = to_touch_chip(dev);
//	struct siw_ts *ts = chip->ts;
	int size = 0;

	size += ori_show_reg_ctrl_log_history(dev, buf);

	size += siw_snprintf(buf, size, "\n[Usage]\n");
	size += siw_snprintf(buf, size, " echo wr 0x1234 {value} > reg_ctrl\n");
	size += siw_snprintf(buf, size, " echo rd 0x1234 > reg_ctrl\n");
	size += siw_snprintf(buf, size, " (burst access)\n");
	size += siw_snprintf(buf, size, " echo rd 0x1234 0x111 > reg_ctrl, 0x111 is size(max 0x%X)\n",
		REG_BURST_MAX);

	return (ssize_t)size;
}

static void ori_store_reg_ctrl_log_add(struct device *dev,
				struct siw_hal_reg_log *new_log)

{
	struct siw_touch_chip *chip = to_touch_chip(dev);
	struct siw_hal_reg_log *reg_log = chip->reg_log;

	memmove(&reg_log[1], reg_log, sizeof(*reg_log) * (REG_LOG_MAX - 1));
	memcpy(reg_log, new_log, sizeof(*reg_log));
}

static inline void ori_store_reg_ctrl_rd_burst_log(struct device *dev,
					u8 *row_buf, int row, int col)
{
	if (col)
		t_dev_info(dev, "rd: [%3Xh] %*ph\n", row, col, row_buf);
}

static int ori_store_reg_ctrl_rd_burst(struct device *dev, u32 addr, int size)
{
	u8 *rd_buf, *row_buf;
	int col_power = REG_BURST_COL_PWR;
	int col_width = (1<<col_power);
	int row_curr, col_curr;
	int ret = 0;

	size = min(size, REG_BURST_MAX);

	rd_buf = (u8 *)kzalloc(size, GFP_KERNEL);
	if (rd_buf == NULL) {
		t_dev_err(dev, "failed to allocate rd_buf\n");
		return -ENOMEM;
	}

	ret = siw_hal_reg_read(dev, addr, rd_buf, size);
	if (ret < 0) {
		goto out;
	}

	t_dev_info(dev, "rd: addr %04Xh, size %Xh\n", addr, size);

	row_buf = rd_buf;
	row_curr = 0;
	while (size) {
		col_curr = min(col_width, size);

		ori_store_reg_ctrl_rd_burst_log(dev, row_buf, row_curr, col_curr);

		row_buf += col_curr;
		row_curr += col_curr;
		size -= col_curr;
	}

out:
	kfree(rd_buf);

	return ret;
}

static ssize_t store_reg_ctrl(struct device *dev,
				const char *buf, size_t count)
{
//	struct siw_ts *ts = to_touch_core(dev);
	struct siw_hal_reg_log reg_log;
	char command[6] = {0};
	u32 reg = 0;
	u32 data = 1;
	u32 reg_addr;
	int wr = -1;
	int value = 0;
	int ret = 0;

	if (sscanf(buf, "%5s %X %X", command, &reg, &value) <= 0) {
		siw_hal_sysfs_err_invalid_param(dev);
		if(count < INT_MAX)
	                return count;
	        else
	                return INT_MAX;
	}

	if (!strcmp(command, "wr") ||
		!strcmp(command, "write")) {
		wr = 1;
	}
	if (!strcmp(command, "rd") ||
		!strcmp(command, "read")) {
		wr = 0;
	}

	reg_addr = reg;
	if (wr == 1) {
		data = value;
		ret = siw_hal_write_value(dev,
					reg_addr,
					data);
		reg_log.dir = REG_DIR_WR;
		reg_log.addr = reg_addr;
		reg_log.data = data;
		if (ret >= 0) {
			t_dev_info(dev,
				"wr: reg[0x%04X] = 0x%08X\n",
				reg_addr, data);
		} else {
			reg_log.dir |= REG_DIR_ERR;
		}
		ori_store_reg_ctrl_log_add(dev, &reg_log);
		goto out;
	} else if (!wr) {
		reg_log.dir = REG_DIR_RD;
		reg_log.addr = reg_addr;
		if (value <= 4) {
			ret = siw_hal_read_value(dev,
						reg_addr,
						&data);
			reg_log.data = data;
			if (ret >= 0) {
				t_dev_info(dev,
					"rd: reg[0x%04X] = 0x%08X\n",
					reg_addr, data);
			}
		} else {
			reg_log.dir |= (REG_DIR_ERR<<1);
			ret = ori_store_reg_ctrl_rd_burst(dev, reg_addr, value);
		}
		if (ret < 0) {
			reg_log.dir |= REG_DIR_ERR;
		}
		ori_store_reg_ctrl_log_add(dev, &reg_log);
		goto out;
	}

	t_dev_info(dev, "[Usage]\n");
	t_dev_info(dev, " echo wr 0x1234 {value} > reg_ctrl\n");
	t_dev_info(dev, " echo rd 0x1234 > reg_ctrl\n");
	t_dev_info(dev, " (burst access)\n");
	t_dev_info(dev, " echo rd 0x1234 0x111 > reg_ctrl, 0x111 is size(max 0x%X)\n",
		REG_BURST_MAX);

out:
	if(count < INT_MAX)
                return count;
        else
                return INT_MAX;
}

#if defined(__SIW_CONFIG_KNOCK)
static ssize_t _show_tci_debug(struct device *dev, char *buf)
{
	struct siw_touch_chip *chip = to_touch_chip(dev);
//	struct siw_ts *ts = chip->ts;
	struct siw_hal_reg *reg = chip->reg;
	u32 rdata = 0;
	int size = 0;
	int ret = 0;

	if (!chip->opt.t_chk_tci_debug) {
		t_dev_info(dev, "This chipset doesn't support tci debugging\n");
		goto out;
	}

	ret = siw_hal_read_value(dev,
			reg->tci_debug_fail_ctrl,
			&rdata);
	if (ret < 0) {
		t_dev_err(dev, "failed to get tci_debug_fail_ctrl, %d\n", ret);
		goto out;
	}

	size += siw_snprintf(buf, size,
				"TCI Debug Status = DRV %s, IC %s\n",
				(chip->tci_debug_type) ? "Enabled" : "Disabled",
				(rdata & 0x01) ? "Enabled" : "Disabled");

out:
	return size;
}

static ssize_t _store_tci_debug(struct device *dev,
				const char *buf, size_t count)
{
	struct siw_touch_chip *chip = to_touch_chip(dev);
	int value = 0;

	if (!chip->opt.t_chk_tci_debug) {
		t_dev_info(dev, "This chipset doesn't support tci debugging\n");
		if(count < INT_MAX)
	                return count;
	        else
	                return INT_MAX;
	}

	if (sscanf(buf, "%d", &value) <= 0) {
		siw_hal_sysfs_err_invalid_param(dev);
		if(count < INT_MAX)
	                return count;
	        else
	                return INT_MAX;
	}

	chip->tci_debug_type = (u8)!!value;
	t_dev_info(dev, "TCI-Debug %s\n", (value) ? "Enabled" : "Disabled");

	if(count < INT_MAX)
                return count;
        else
                return INT_MAX
}

static ssize_t _show_lcd_mode(struct device *dev, char *buf)
{
	struct siw_touch_chip *chip = to_touch_chip(dev);
	int size = 0;

	size += siw_snprintf(buf, size, "current driving mode is %s\n",
				siw_lcd_driving_mode_str(chip->lcd_mode));

	return size;
}

static ssize_t _store_lcd_mode(struct device *dev,
				const char *buf, size_t count)
{
	struct siw_touch_chip *chip = to_touch_chip(dev);
	struct siw_ts *ts = chip->ts;
	int value = 0;

	if (sscanf(buf, "%d", &value) <= 0) {
		siw_hal_sysfs_err_invalid_param(dev);
		if(count < INT_MAX)
	                return count;
	        else
	                return INT_MAX;
	}

	t_dev_info(dev, "try to change driving mode: %s -> %s\n",
			siw_lcd_driving_mode_str(chip->lcd_mode),
			siw_lcd_driving_mode_str(value));
	siw_ops_notify(ts, LCD_EVENT_LCD_MODE, &value);

	if(count < INT_MAX)
                return count;
        else
                return INT_MAX;
}

#endif	/* __SIW_CONFIG_KNOCK */

#if defined(__SIW_CONFIG_SWIPE)
static const char *siw_hal_debug_type_str[] = {
	"Disable Type",
	"Buffer Type",
	"Always Report Type"
};

static ssize_t _show_swipe_debug(struct device *dev, char *buf)
{
	struct siw_touch_chip *chip = to_touch_chip(dev);
//	struct siw_ts *ts = chip->ts;
	struct siw_hal_reg *reg = chip->reg;
	u32 rdata = -1;
	int size = 0;
	int ret = 0;

	ret = siw_hal_read_value(dev,
				reg->swipe_fail_debug_r,
				&rdata);
	if (ret < 0) {
		t_dev_info(dev, "Fail to Read SWIPE Debug reason type\n");
		return (ssize_t)ret;
	}

	size += siw_snprintf(buf, size,
				"Read SWIPE Debug reason type[IC] = %s\n",
				siw_hal_debug_type_str[rdata]);
	size += siw_snprintf(buf, size,
				"Read SWIPE Debug reason type[Driver] = %s\n",
				siw_hal_debug_type_str[chip->swipe_debug_type]);

	return (ssize_t)size;
}

static ssize_t _store_swipe_debug(struct device *dev,
				const char *buf, size_t count)
{
	struct siw_touch_chip *chip = to_touch_chip(dev);
	int value = 0;

	if (sscanf(buf, "%d", &value) <= 0) {
		siw_hal_sysfs_err_invalid_param(dev);
		if(count < INT_MAX )
	                return count;
	        else
	                return INT_MAX;
	}

	if (value > 2 || value < 0) {
		t_dev_err(dev, "SET SWIPE debug reason wrong, 0, 1, 2 only\n");
		if(count < INT_MAX)
	                return count;
	        else
	                return INT_MAX;
	}

	chip->swipe_debug_type = (u8)value;
	t_dev_info(dev, "Write SWIPE Debug reason type = %s\n",
			siw_hal_debug_type_str[value]);

	if(count < INT_MAX)
                return count;
        else
                return INT_MAX;
}
#endif	/* __SIW_CONFIG_SWIPE */

static ssize_t show_reset_ctrl(struct device *dev, char *buf)
{
	int size = 0;

	size += siw_snprintf(buf, size, "Reset Control Usage\n");
	size += siw_snprintf(buf, size,
				" SW Reset        : echo %d > hal_reset_ctrl\n",
				SW_RESET);
	size += siw_snprintf(buf, size,
				" HW Reset(Async) : echo %d > hal_reset_ctrl\n",
				HW_RESET_ASYNC);
	size += siw_snprintf(buf, size,
				" HW Reset(Sync)  : echo %d > hal_reset_ctrl\n",
				HW_RESET_SYNC);

	size += siw_snprintf(buf, size,
				" HW Reset(Cond)  : echo 0x%X > hal_reset_ctrl\n",
				HW_RESET_COND);

	return size;
}

static ssize_t store_reset_xxx(struct device *dev, int type)
{
	struct siw_touch_chip *chip = to_touch_chip(dev);
	struct siw_ts *ts = chip->ts;

	if (type == HW_RESET_COND) {
		siw_ops_mon_handler(ts, MON_FLAG_RST_ONLY);
		goto out;
	}

	siw_ops_reset(ts, type);

out:
	return 0;
}

static ssize_t store_reset_ctrl(struct device *dev,
				const char *buf, size_t count)
{
	int value = 0;

	if (sscanf(buf, "%X", &value) <= 0) {
		siw_hal_sysfs_err_invalid_param(dev);
		if(count < INT_MAX)
	                return count;
	        else
	                return INT_MAX;
	}

	store_reset_xxx(dev, value);

	if(count < INT_MAX)
                return count;
        else
                return INT_MAX;
}

#if defined(SIW_ATTR_RST_BY_READ)
static ssize_t show_reset_sw(struct device *dev, char *buf)
{
	return store_reset_xxx(dev, SW_RESET);
}

static ssize_t show_reset_hw(struct device *dev, char *buf)
{
	return store_reset_xxx(dev, HW_RESET_SYNC);
}
#endif

#if defined(__SIW_USE_BUS_TEST)
u32 t_dbg_bus_cnt = 10000;
module_param_named(dbg_bus_cnt, t_dbg_bus_cnt, uint, S_IRUGO|S_IWUSR|S_IWGRP);

static int _show_debug_bus_test(struct device *dev, int tcnt, int *lcnt)
{
	struct siw_touch_chip *chip = to_touch_chip(dev);
	struct siw_hal_reg *reg = chip->reg;
	struct siw_hal_fw_info *fw = &chip->fw;
	u32 chip_id;
	u32 data;
	int log_cnt = 10;
	int i;
	int last = 0;
	int ret = 0;

	if (tcnt >= 100000) {
		log_cnt = 10000;
	} else if (tcnt >= 10000) {
		log_cnt = 1000;
	} else if (tcnt >= 1000) {
		log_cnt = 100;
	}

	for (i = 0 ; i < tcnt ; i++) {
		data = i & 0xFFFF;
		data |= (i & 0x01) ? 0x5A5A0000 : 0xA5A50000;

		ret = siw_hal_ic_test_unit(dev, data);
		if (ret < 0) {
			break;
		}

		if (!i)
			continue;

		last = (i == (tcnt - 1));

		if (last || !(i % log_cnt)) {
			t_dev_info(dev, "bus testing... (%d)\n", i);
		}

		if (last || !(i % SIW_TOUCH_MAX_BUF_IDX)) {
			ret = siw_hal_read_value(dev,
					reg->spr_chip_id,
					&chip_id);
			if (ret < 0) {
				break;
			}

			if (fw->chip_id_raw != chip_id) {
				ret = -EFAULT;
				break;
			}
		}
	}

	if (lcnt != NULL) {
		*lcnt = i;
	}

	return ret;
}

static ssize_t _show_debug_bus(struct device *dev, char *buf)
{
	struct siw_touch_chip *chip = to_touch_chip(dev);
	struct siw_ts *ts = chip->ts;
	char *bus_str = NULL;
	u32 bus_type = touch_bus_type(ts);
	u32 bus_freq = 0;
	int size = 0;
	int tcnt = t_dbg_bus_cnt;
	int lcnt = 0;
	int ret = 0;

	switch (bus_type) {
	case BUS_IF_I2C:
		bus_str = "I2C";
		break;
	case BUS_IF_SPI:
		bus_str = "SPI";
		break;
	default:
		t_dev_err(dev, "unknown bus type, %d\n", bus_type);
		goto out;
	}

	t_dev_info(dev, "%s bus test(%d) begins\n", bus_str, tcnt);

	if (bus_type == BUS_IF_SPI) {
		bus_freq = touch_max_freq(ts);
		if ((bus_freq == ~0) || (bus_freq < 1000000)) {
			t_dev_err(dev, "wrong frequency, %d\n", bus_freq);
			goto out;
		}

		t_dev_info(dev, "SPI bus freq: %d MHz\n", bus_freq / 1000000);
	}

	siw_touch_mon_pause(dev);

#if defined(SIW_SUPPORT_WAKE_LOCK)
	wake_lock_timeout(&ts->lpwg_wake_lock, msecs_to_jiffies(1000));
#endif

	touch_msleep(10);

	mutex_lock(&ts->lock);

	siw_touch_irq_control(dev, INTERRUPT_DISABLE);

	ret = _show_debug_bus_test(dev, tcnt, &lcnt);

	siw_touch_irq_control(dev, INTERRUPT_ENABLE);

	mutex_unlock(&ts->lock);

	siw_touch_mon_resume(dev);

	t_dev_info(dev, "%s bus test(%d) %s, %d\n",
		bus_str, tcnt, (ret < 0) ? "failed" : "done", lcnt);

	size += siw_snprintf(buf, size,
			 "%s bus test(%d) %s, %d\n",
			 bus_str, tcnt, (ret < 0) ? "failed" : "done", lcnt);

out:
	return (ssize_t)size;
}
#endif	/* __SIW_USE_BUS_TEST */

static const char *debug_hal_delay_str[] = {
	[HAL_DBG_DLY_TC_DRIVING_0]	= "(Group - TC Driving)",
	[HAL_DBG_DLY_FW_0]			= "(Group - FW)",
	[HAL_DBG_DLY_HW_RST_0]		= "(Group - Reset)",
	[HAL_DBG_DLY_NOTIFY]		= "(Group - Notify)",
	[HAL_DBG_DLY_LPWG]			= "(Group - LPWG)",
};

static ssize_t show_debug_hal(struct device *dev, char *buf)
{
	struct siw_touch_chip *chip = to_touch_chip(dev);
//	struct siw_ts *ts = chip->ts;
	struct siw_hal_debug *dbg = &chip->dbg;
	char *str;
	int size = 0;
	int i;

	for (i = 0; i < HAL_DBG_DLY_MAX; i++) {
		switch (i) {
		case HAL_DBG_DLY_TC_DRIVING_0:
		case HAL_DBG_DLY_FW_0:
		case HAL_DBG_DLY_HW_RST_0:
		case HAL_DBG_DLY_NOTIFY:
		case HAL_DBG_DLY_LPWG:
			str = (char *)debug_hal_delay_str[i];
			break;
		default:
			str = "";
		}
		size += siw_snprintf(buf, size,
			 "debug_hal: delay[%d] = %Xh %s\n",
			 i, dbg->delay[i], str);
	}

	return (ssize_t)size;
}

static int store_debug_hal_delay(struct device *dev, int sel, int val, int opt)
{
	struct siw_touch_chip *chip = to_touch_chip(dev);
	struct siw_hal_debug *dbg = &chip->dbg;

	if (sel < HAL_DBG_DLY_MAX) {
		u32 *delay = &dbg->delay[sel];

		if ((*delay) != val) {
			t_dev_info(dev, "debug_hal: delay[%d] changed: %Xh -> %Xh\n",
				sel, (*delay), val);
			(*delay) = val;
		}
	}

	return 0;
}

static ssize_t store_debug_hal(struct device *dev,
				const char *buf, size_t count)
{
//	struct siw_touch_chip *chip = to_touch_chip(dev);
//	struct siw_ts *ts = chip->ts;
	int code = 0;
	int sel = 0;
	int val = 0;
	int opt = 0;
	int ret;

	if (sscanf(buf, "%X %X %X %X", &code, &sel, &val, &opt) <= 0) {
		siw_hal_sysfs_err_invalid_param(dev);
		if(count < INT_MAX)
	                return count;
	        else
	                return INT_MAX;
	}

	t_dev_info(dev, "debug_hal: code %Xh, sel %Xh, val %Xh, opt %Xh\n",
			code, sel, val, opt);

	switch (code) {
	case HAL_DBG_GRP_0:
		ret = store_debug_hal_delay(dev, sel, val, opt);
		break;
	default:
		break;
	}

	if(count < INT_MAX)
                return count;
        else
                return INT_MAX;
}

#if defined(SIW_SUPPORT_ALIVE_DETECTION)
u32 __weak t_alive_dbg_mask;	//instead of using extern

int __weak siw_hal_alive_level_get(struct device *dev)
{
	return 0;
}

int __weak siw_hal_alive_level_set(struct device *dev, int new_level)
{
	return 0;
}

void __weak siw_hal_irq_count_clr(struct device *dev)
{

}

static ssize_t show_alive_debug(struct device *dev, char *buf)
{
//	struct siw_ts *ts = to_touch_core(dev);
	int size = 0;

	size += siw_snprintf(buf, size,
				"t_alive_dbg_mask %08Xh\n", t_alive_dbg_mask);

	size += siw_snprintf(buf, size,
				"Usage:\n");
	size += siw_snprintf(buf, size,
				" echo {mask_value} > alive_debug\n");

	return (ssize_t)size;
}

static ssize_t store_alive_debug(struct device *dev,
				const char *buf, size_t count)
{
//	struct siw_ts *ts = to_touch_core(dev);
	u32 old_value, new_value = 0;

	if (sscanf(buf, "%X", &new_value) <= 0) {
		siw_hal_sysfs_err_invalid_param(dev);
		if(count < INT_MAX)
	                return count;
	        else
	                return INT_MAX;
	}

	old_value = t_alive_dbg_mask;
	t_alive_dbg_mask = new_value;
	t_dev_info(dev, "t_alive_dbg_mask changed : %08Xh -> %08Xh\n",
		old_value, new_value);

	if(count < INT_MAX)
                return count;
        else
                return INT_MAX;
}

static ssize_t show_alive_count(struct device *dev, char *buf)
{
	struct siw_touch_chip *chip = to_touch_chip(dev);
	int size = 0;

	size += siw_snprintf(buf, size,
				"irq_count : alive %d, report %d, others %d\n",
				chip->irq_count_alive,
				chip->irq_count_report,
				chip->irq_count_others);

	return (ssize_t)size;
}

static ssize_t store_alive_count(struct device *dev,
				const char *buf, size_t count)
{
//	struct siw_ts *ts = to_touch_core(dev);
	char string[32] = {0, };

	if (sscanf(buf, "%s", string) <= 0) {
		siw_hal_sysfs_err_invalid_param(dev);
		if(count < INT_MAX)
	                return count;
	        else
	                return INT_MAX;
	}

	if (!strcmp(string, "reset")) {
		siw_hal_irq_count_clr(dev);
	}

	if(count < INT_MAX)
                return count;
        else
                return INT_MAX;
}

static ssize_t show_alive_level(struct device *dev, char *buf)
{
	int size = 0;
	int level = siw_hal_alive_level_get(dev);

	size += siw_snprintf(buf, size,
				"alive_level : %s\n", siw_alive_level_str(level));
	return size;
}

static ssize_t store_alive_level(struct device *dev,
			const char *buf, size_t count)
{
	int level = 0;

	if (sscanf(buf, "%d", &level) <= 0) {
		siw_hal_sysfs_err_invalid_param(dev);
		if(count < INT_MAX)
	                return count;
	        else
	                return INT_MAX;
	}

	siw_hal_alive_level_set(dev, level);

	if(count < INT_MAX)
                return count;
        else
                return INT_MAX;
}

static ssize_t show_power_off(struct device *dev,char *buf)
{
	siw_hal_set_gpio_reset(dev, GPIO_OUT_ZERO);

	return 0;
}

static ssize_t show_power_on(struct device *dev,char *buf)
{
	siw_hal_set_gpio_reset(dev, GPIO_OUT_ONE);

	return 0;
}

static ssize_t show_recovery_off(struct device *dev,char *buf)
{
	struct siw_touch_chip *chip = to_touch_chip(dev);
	printk("%s [Start]\n",__func__);
	siw_serdes_reset_dwork_stop(chip->ts);

	return 0;
}

static ssize_t show_recovery_on(struct device *dev,char *buf)
{
	struct siw_touch_chip *chip = to_touch_chip(dev);
	printk("%s [Start]\n",__func__);
	siw_serdes_reset_dwork_start(chip->ts);

	return 0;
}

static ssize_t show_debug(struct device *dev,char *buf)
{
        return snprintf(buf, PAGE_SIZE, "CURRENT DEBUG MODE : %d\n"
                                        "0:NONE\n" "1:INFO\n"
                                        "2:MESSAGES\n" "3:TRACE\n \t4:INTURRUPT\n", siw_debug);

	return 0;
}

static ssize_t set_debug(struct device *dev, const char *buf, size_t count)
{
        int state;

        if(sscanf(buf, "%d", &state) < 0)
		return -EINVAL;

        if ((state >= DEBUG_NONE) && (state <= DEBUG_INT)) {
		
		switch(state){
			case 0 :
			case 1 : 
				t_dev_dbg_mask = state;
				t_alive_dbg_mask = state;
				break;
			case 2 :
				t_dev_dbg_mask = 0x10;
                                t_alive_dbg_mask = 0;
                                break;
			case 3 :
				t_dev_dbg_mask = 0x10;
                                t_alive_dbg_mask = 1;
				break;
			case 4 :
				t_dev_dbg_mask = 0;
				t_alive_dbg_mask = 1;
				break;
			default:
				break;
		}
		siw_debug = state;
                
                pr_info("\tCURRENT DEBUG MODE : %d\n"
                                "\t0:NONE\n" "\t1:INFO\n"
                                "\t2:MESSAGES\n" "\t3:TRACE\n \t4:INTURRUPT\n", siw_debug);
        }
        else {
                return -EINVAL;
        }

	if(count < INT_MAX)
                return count;
        else
                return INT_MAX;
}

static ssize_t show_serdes_setting(struct device *dev, const char *buf)
{
        struct siw_touch_chip *chip = to_touch_chip(dev);
        struct siw_ts *ts = chip->ts;
	struct i2c_client *client = to_i2c_client(dev);

	siw_serdes_reset_dwork_stop(chip->ts);
	mutex_lock(&ts->serdes.lock);
	mutex_lock(&ts->lock);

        serdes_i2c_show(client);

	mutex_unlock(&ts->lock);
        mutex_unlock(&ts->serdes.lock);
	siw_serdes_reset_dwork_start(chip->ts);

        return 0;
}

static ssize_t set_serdes_setting(struct device *dev, const char *buf, size_t count)
{
        struct siw_touch_chip *chip = to_touch_chip(dev);
        struct siw_ts *ts = chip->ts;
        struct i2c_client *client = to_i2c_client(dev);
        int ser_bitrate = 0;
	int des_bitrate = 0;

	siw_serdes_reset_dwork_stop(chip->ts);
	mutex_lock(&ts->serdes.lock);
	mutex_lock(&ts->lock);

        ser_bitrate = ser_i2c_bitrate_check(client);
	des_bitrate = des_i2c_bitrate_check(client);

	printk("Ser_Bitrate : 0x%x Des_Bitrate : 0x%x\n",ser_bitrate,des_bitrate);

        serdes_i2c_reset(client, false);

	mutex_unlock(&ts->lock);
        mutex_unlock(&ts->serdes.lock);
	siw_serdes_reset_dwork_start(chip->ts);

	if(count < INT_MAX)
                return count;
        else
                return INT_MAX;
}

static ssize_t set_des_setting(struct device *dev, const char *buf, size_t count)
{
	struct siw_touch_chip *chip = to_touch_chip(dev);
        struct siw_ts *ts = chip->ts;
        struct i2c_client *client = to_i2c_client(dev);
        int ser_bitrate = 0;
        int des_bitrate = 0;

	siw_serdes_reset_dwork_stop(chip->ts);
	mutex_lock(&ts->serdes.lock);
	mutex_lock(&ts->lock);

        ser_bitrate = ser_i2c_bitrate_check(client);
        des_bitrate = des_i2c_bitrate_check(client);

        printk("Ser_Bitrate : 0x%x Des_Bitrate : 0x%x\n",ser_bitrate,des_bitrate);

	serdes_i2c_reset(client,true);

	mutex_unlock(&ts->lock);
        mutex_unlock(&ts->serdes.lock);
	siw_serdes_reset_dwork_start(chip->ts);

	if(count < INT_MAX)
                return count;
        else
                return INT_MAX;
}

static ssize_t set_serdes_6gbps(struct device *dev, const char *buf, size_t count)
{
        struct siw_touch_chip *chip = to_touch_chip(dev);
        struct siw_ts *ts = chip->ts;
        struct i2c_client *client = to_i2c_client(dev);
        int ser_bitrate = 0;
        int des_bitrate = 0;

	siw_serdes_reset_dwork_stop(chip->ts);
	mutex_lock(&ts->serdes.lock);
	mutex_lock(&ts->lock);

        ser_bitrate = ser_i2c_bitrate_check(client);
        des_bitrate = des_i2c_bitrate_check(client);

        printk("Before Ser_Bitrate : 0x%x Des_Bitrate : 0x%x\n",ser_bitrate,des_bitrate);

        des_i2c_bitrate_3gbps_to_6gbps(client);
        ser_i2c_bitrate_3gbps_to_6gbps(client);

        ser_bitrate = ser_i2c_bitrate_check(client);
        des_bitrate = des_i2c_bitrate_check(client);

        printk("Now Ser_Bitrate : 0x%x Des_Bitrate : 0x%x\n",ser_bitrate,des_bitrate);

	mutex_unlock(&ts->lock);
        mutex_unlock(&ts->serdes.lock);
	siw_serdes_reset_dwork_start(chip->ts);

	if(count < INT_MAX)
                return count;
        else
                return INT_MAX;
}

static ssize_t set_serdes_3gbps(struct device *dev, const char *buf, size_t count)
{
        struct siw_touch_chip *chip = to_touch_chip(dev);
        struct siw_ts *ts = chip->ts;
        struct i2c_client *client = to_i2c_client(dev);
        int ser_bitrate = 0;
        int des_bitrate = 0;

	siw_serdes_reset_dwork_stop(chip->ts);
	mutex_lock(&ts->serdes.lock);
	mutex_lock(&ts->lock);

        ser_bitrate = ser_i2c_bitrate_check(client);
        des_bitrate = des_i2c_bitrate_check(client);

        printk("Before Ser_Bitrate : 0x%x Des_Bitrate : 0x%x\n",ser_bitrate,des_bitrate);

        serdes_i2c_bitrate_6gbps_to_3gbps(client);
        mdelay(100);

        ser_bitrate = ser_i2c_bitrate_check(client);
        des_bitrate = des_i2c_bitrate_check(client);

        printk("Now Ser_Bitrate : 0x%x Des_Bitrate : 0x%x\n",ser_bitrate,des_bitrate);

	mutex_unlock(&ts->lock);
        mutex_unlock(&ts->serdes.lock);
	siw_serdes_reset_dwork_start(chip->ts);

	if(count < INT_MAX)
                return count;
        else
                return INT_MAX;
}

static ssize_t show_read_register(struct device *dev, const char *buf)
{
	int ret, i;
        struct siw_touch_chip *chip = to_touch_chip(dev);
        struct siw_ts *ts = chip->ts;
	struct i2c_client *client = to_i2c_client(dev);
	u8 buf1[3];
	u8 buf2[3];
	unsigned short addr;

	siw_serdes_reset_dwork_stop(chip->ts);
	mutex_lock(&ts->serdes.lock);
	mutex_lock(&ts->lock);

        addr = client->addr;

        printk("-----------%s\n", __func__);

	serdes_video_signal_check(client, 0);

	client->addr = serdes_line_fault_config[0][0];
        buf1[0] = (serdes_line_fault_config[0][1] >> 8) & 0xff;
        buf1[1] = serdes_line_fault_config[0][1] & 0xff;
        buf1[2] = serdes_line_fault_config[0][2];

        ret = i2c_master_send(client, buf1, 3);

        mdelay(20);

        for(i = 0; i < 6 ;i++) {

                client->addr = serdes_read[i][0];
                buf1[0] = (serdes_read[i][1] >> 8) & 0xff;
                buf1[1] = serdes_read[i][1] & 0xff;
                buf1[2] = serdes_read[i][2];

		ret = i2c_master_send(client, buf1, 2);
                ret = i2c_master_recv(client, buf2, 1);

		printk("0x%04X 0x%04X write:0x%02X read:0x%02X\n", serdes_read[i][0], serdes_read[i][1], serdes_read[i][2], buf2[0]);
        }

	client->addr = serdes_line_fault_config[1][0];
        buf1[0] = (serdes_line_fault_config[1][1] >> 8) & 0xff;
        buf1[1] = serdes_line_fault_config[1][1] & 0xff;
        buf1[2] = serdes_line_fault_config[1][2];

        ret = i2c_master_send(client, buf1, 3);

        mdelay(100);

        client->addr = serdes_line_fault_config[2][0];
        buf1[0] = (serdes_line_fault_config[2][1] >> 8) & 0xff;
        buf1[1] = serdes_line_fault_config[2][1] & 0xff;

        ret = i2c_master_send(client, buf1, 2);
        ret = i2c_master_recv(client, buf2, 1);

        printk("%s err 0x%02x\n",__func__, buf2[0]);

	printk("-----------%s Success\n", __func__);

	client->addr = addr;

	mutex_unlock(&ts->lock);
        mutex_unlock(&ts->serdes.lock);
	siw_serdes_reset_dwork_start(chip->ts);

        return 0;
}

static ssize_t set_adm(struct device *dev, const char *buf, size_t count)
{
	int state;
        struct siw_touch_chip *chip = to_touch_chip(dev);
        struct siw_ts *ts = chip->ts;
	u8 buf1[3];
        unsigned short addr;
        struct i2c_client *client = to_i2c_client(dev);

	if(sscanf(buf, "%d", &state) < 0)
		return -EINVAL;

	printk("%s state %d\n",__func__, state);
        if (state == 1) {
		printk("%s Set ADM Mode\n",__func__);
		siw_serdes_reset_dwork_stop(chip->ts);
		siw_hal_set_gpio_reset(dev, GPIO_OUT_ZERO);

		mdelay(20);

		printk("%s Set LVDS off Mode\n",__func__);

		addr = client->addr;
	        client->addr = 0x48;
	        buf1[0] = 0x01;
	        buf1[1] = 0xCF;
	        buf1[2] = 0xC7;

	        i2c_master_send(client, buf1, 3);

		printk("%s Set Mute off Mode\n",__func__);

		buf1[0] = 0x02;
		buf1[1] = 0x21;
		buf1[2] = 0x80;

		i2c_master_send(client, buf1, 3);

	        client->addr = addr;
	}
	else{
		return -EINVAL;
	}
	if(count < INT_MAX)
                return count;
        else
                return INT_MAX;
}

static ssize_t set_test(struct device *dev, const char *buf, size_t count)
{
        int state;
        struct siw_touch_chip *chip = to_touch_chip(dev);
        struct siw_ts *ts = chip->ts;
        u8 buf1[3];
        unsigned short addr;
        struct i2c_client *client = to_i2c_client(dev);
	int i;

        if(sscanf(buf, "%d", &state) < 0)
		return -EINVAL;

        printk("%s state %d\n",__func__, state);

	siw_serdes_reset_dwork_stop(chip->ts);
        mutex_lock(&ts->serdes.lock);

	siw_serdes_line_fault_check(client);

	if(state == 1)
	{
		printk("interrupt off\n");
		siw_touch_irq_control(dev, INTERRUPT_DISABLE);
	}
	else if(state >= 2)
	{
		printk("mutex lock\n");
		mutex_lock(&ts->lock);
	}

	for(i=0;i < (10-state);i++)
	{
		printk("mdelay(5)\n");
		mdelay(5);
	}


	if(state == 1)
	{
		printk("interrupt on\n");
		siw_touch_irq_control(dev, INTERRUPT_ENABLE);
	}
	else if(state >= 2)
	{
		siw_serdes_i2c_connect(client);
		printk("mutex unlock\n");
		mutex_unlock(&ts->lock);
	}

        mutex_unlock(&ts->serdes.lock);
        siw_serdes_reset_dwork_start(chip->ts);

	if(count < INT_MAX)
                return count;
        else
                return INT_MAX;
}

static ssize_t show_mute_on(struct device *dev, const char *buf)
{
        struct siw_touch_chip *chip = to_touch_chip(dev);
        struct siw_ts *ts = chip->ts;
        unsigned short addr;
        struct i2c_client *client = to_i2c_client(dev);
        u8 buf1[3];

        client->addr = DES_ADDRESS;
        buf1[0] = 0x02;
        buf1[1] = 0x21;
        buf1[2] = 0x90;

        i2c_master_send(client, buf1, 3);

        client->addr = addr;

        printk("%s [Done]\n",__func__);

        return 0;
}

static ssize_t show_mute_off(struct device *dev, const char *buf)
{
        struct siw_touch_chip *chip = to_touch_chip(dev);
        struct siw_ts *ts = chip->ts;
        unsigned short addr;
        struct i2c_client *client = to_i2c_client(dev);
        u8 buf1[3];

        client->addr = DES_ADDRESS;
        buf1[0] = 0x02;
        buf1[1] = 0x21;
        buf1[2] = 0x80;

        i2c_master_send(client, buf1, 3);

        client->addr = addr;

        printk("%s [Done]\n",__func__);

        return 0;
}

static ssize_t set_int_on(struct device *dev, char *buf)
{
        int ret;
	struct siw_touch_chip *chip = to_touch_chip(dev);
        struct siw_ts *ts = chip->ts;
        struct i2c_client *client = to_i2c_client(dev);

        printk("%s [Start]\n",__func__);

	siw_touch_irq_control(dev, INTERRUPT_ENABLE);

	return 0;
}

static ssize_t set_int_off(struct device *dev, char *buf)
{
        int ret;
	struct siw_touch_chip *chip = to_touch_chip(dev);
        struct siw_ts *ts = chip->ts;
        struct i2c_client *client = to_i2c_client(dev);

        printk("%s [Start]\n",__func__);

	siw_touch_irq_control(dev, INTERRUPT_DISABLE);

	return 0;
}

static ssize_t set_lvds_con(struct device *dev, const char *buf, size_t count)
{
        int state;
	struct siw_touch_chip *chip = to_touch_chip(dev);
        struct siw_ts *ts = chip->ts;
        struct i2c_client *client = to_i2c_client(dev);
        u8 buf1[3];
        unsigned short addr;

        if(sscanf(buf, "%d", &state) < 0)
		return -EINVAL;

        printk("%s Start %d\n",__func__,state);

	siw_serdes_reset_dwork_stop(chip->ts);
	mutex_lock(&ts->serdes.lock);
	mutex_lock(&ts->lock);

	if(state)
	{
		addr = client->addr;

                client->addr = 0x48;
                buf1[0] = 0x01;
                buf1[1] = 0xCF;
                buf1[2] = 0x01;

                i2c_master_send(client, buf1, 3);
                client->addr = addr;
	}
	else
	{
		addr = client->addr;

                client->addr = 0x48;
                buf1[0] = 0x01;
                buf1[1] = 0xCF;
                buf1[2] = 0xC7;

                i2c_master_send(client, buf1, 3);
                client->addr = addr;
	}

	mutex_unlock(&ts->lock);
        mutex_unlock(&ts->serdes.lock);
	siw_serdes_reset_dwork_start(chip->ts);

	if(count < INT_MAX)
                return count;
        else
                return INT_MAX;
}

static ssize_t show_blu_reg(struct device *dev, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct siw_touch_chip *chip = to_touch_chip(dev);
        struct siw_ts *ts = chip->ts;
        u8 buf1[3];
	u8 buf2[3];
        unsigned short addr;

	printk("%s [Start]\n",__func__);

	siw_serdes_reset_dwork_stop(chip->ts);
	mutex_lock(&ts->serdes.lock);
	mutex_lock(&ts->lock);

	addr = client->addr;

	client->addr = 0x40;
	buf1[0] = 0x02;
	buf1[1] = 0x06;

	i2c_master_send(client, buf1, 2);
        i2c_master_recv(client, buf2, 1);

	printk("SER 0x0206 : 0x%02x\n",buf2[0]);

	client->addr = 0x48;
        buf1[0] = 0x02;
        buf1[1] = 0x12;

        i2c_master_send(client, buf1, 2);
        i2c_master_recv(client, buf2, 1);

        printk("DES 0x0212 : 0x%02x\n",buf2[0]);

	client->addr = 0x40;
        buf1[0] = 0x02;
        buf1[1] = 0x15;

        i2c_master_send(client, buf1, 2);
        i2c_master_recv(client, buf2, 1);

        printk("SER 0x0215 : 0x%02x\n",buf2[0]);
	if((buf2[0]&0x08)==0x08)
		printk("Micom BLU HIGH\n");
	else
		printk("Micom BLU LOW\n");

        client->addr = 0x48;
        buf1[0] = 0x02;
        buf1[1] = 0x15;

        i2c_master_send(client, buf1, 2);
        i2c_master_recv(client, buf2, 1);

        printk("DES 0x0215 : 0x%02x\n",buf2[0]);

	if((buf2[0]&0x06)==0x04)
		printk("DES BLU Default");
	else if((buf2[0]&0x06)==0x02)
		printk("DES BLU Forced");
	else
		printk("DES BLU Unknown");

	if((buf2[0]&0x08)==0x08)
		printk(" HIGH\n");
	else
		printk(" LOW\n");

	client->addr = addr;

	mutex_unlock(&ts->lock);
        mutex_unlock(&ts->serdes.lock);
	siw_serdes_reset_dwork_start(chip->ts);

	return 0;
}

static ssize_t set_blu_con(struct device *dev,  const char *buf, size_t count)
{
        int state;
        struct i2c_client *client = to_i2c_client(dev);
	struct siw_touch_chip *chip = to_touch_chip(dev);
        struct siw_ts *ts = chip->ts;
        u8 buf1[3];
        unsigned short addr;

        if(sscanf(buf, "%d", &state) < 0)
		return -EINVAL;

        printk("%s Start %d\n",__func__,state);

	siw_serdes_reset_dwork_stop(chip->ts);
	mutex_lock(&ts->serdes.lock);
	mutex_lock(&ts->lock);

        if(state == 1) //default to micom
        {
                addr = client->addr;

		client->addr = 0x48;
                buf1[0] = 0x02;
                buf1[1] = 0x15;
                buf1[2] = 0x24;

                i2c_master_send(client, buf1, 3);

		client->addr = 0x48;
                buf1[0] = 0x00;
                buf1[1] = 0x10;
                buf1[2] = 0x31;

                i2c_master_send(client, buf1, 3);

                client->addr = addr;
        }
	else if(state == 2) //forced on
	{
		addr = client->addr;

                client->addr = 0x48;
                buf1[0] = 0x02;
                buf1[1] = 0x15;
                buf1[2] = 0x32;

                i2c_master_send(client, buf1, 3);

                client->addr = addr;
	}
        else //forced off
        {
                addr = client->addr;

		client->addr = 0x48;
                buf1[0] = 0x02;
                buf1[1] = 0x15;
                buf1[2] = 0x22;

                i2c_master_send(client, buf1, 3);

                client->addr = addr;
        }

	mutex_unlock(&ts->lock);
        mutex_unlock(&ts->serdes.lock);
	siw_serdes_reset_dwork_start(chip->ts);

	if(count < INT_MAX)
                return count;
        else
                return INT_MAX;
}

static ssize_t set_touch_log(struct device *dev, char *buf)
{
        int ret;
        struct i2c_client *client = to_i2c_client(dev);
	struct siw_touch_chip *chip = to_touch_chip(dev);
        struct siw_ts *ts = chip->ts;
	int ser_bitrate, des_bitrate;
	u8 buf1[3];
        u8 buf2[3];
	unsigned short addr;
	int i;

	printk("\n----------------------------------------------------\n\n");
        printk("%s [Start]\n\n",__func__);

	siw_serdes_reset_dwork_stop(chip->ts);
	mutex_lock(&ts->serdes.lock);

	siw_touch_get(dev, CMD_ATCMD_VERSION, buf);
	printk("F/W Version : %s\n", buf);

	mobis_touch_counter_print();

	mutex_lock(&ts->lock);

	if(!get_montype())
	{
		mobis_show_need_log_count();

		ser_bitrate = ser_i2c_bitrate_check(client);
	        des_bitrate = des_i2c_bitrate_check(client);

		printk("Ser_bitrate : ");
		if(ser_bitrate == 0x88)
			printk("6Gbps");
		else if(ser_bitrate == 0x84)
			printk("3Gbps");
		else
			printk("Wrong");

		printk(" Des_bitrate : ");
                if(des_bitrate == 0x02)
                        printk("6Gbps");
                else if(des_bitrate == 0x01)
                        printk("3Gbps");
		else if(des_bitrate == 0x81)
			printk("3Gbps 175mV");
                else
                        printk("Wrong");
                printk("\n\n");


		addr = client->addr;

		client->addr = 0x40;
	        buf1[0] = 0x02;
	        buf1[1] = 0x15;

	        i2c_master_send(client, buf1, 2);
	        i2c_master_recv(client, buf2, 1);

	        printk("SER 0x0215 read:0x%02x\n",buf2[0]);
	        if((buf2[0]&0x08)==0x08)
	                printk("Micom BLU HIGH\n");
	        else
	                printk("Micom BLU LOW\n");

	        client->addr = 0x48;
	        buf1[0] = 0x02;
	        buf1[1] = 0x15;

	        i2c_master_send(client, buf1, 2);
	        i2c_master_recv(client, buf2, 1);

	        printk("DES 0x0215 read:0x%02x\n",buf2[0]);

	        if((buf2[0]&0x06)==0x04)
	                printk("DES BLU Default");
	        else if((buf2[0]&0x06)==0x02)
	                printk("DES BLU Forced");
	        else
	                printk("DES BLU Unknown");

	        if((buf2[0]&0x08)==0x08)
	                printk(" HIGH");
	        else
	                printk(" LOW");

	        client->addr = addr;

		printk("\n\n");

		serdes_video_signal_check(client, 0);

		printk("\n");

		addr = client->addr;
		for(i = 0; i < 6 ;i++) {
			client->addr = serdes_read[i][0];
			buf1[0] = (serdes_read[i][1] >> 8) & 0xff;
			buf1[1] = serdes_read[i][1] & 0xff;
			buf1[2] = serdes_read[i][2];

			if(client->addr != DES_DELAY)
			{
				i2c_master_send(client, buf1, 2);
				i2c_master_recv(client, buf2, 1);
			}

			if(client->addr == SER_ADDRESS)
				printk("SER ");
			else if(client->addr == DES_ADDRESS)
                                printk("DES ");
			printk("0x%04X write:0x%02X read:0x%02X\n",serdes_read[i][1], serdes_read[i][2], buf2[0]);
		}
		client->addr = addr;

		printk("\n");

		serdes_i2c_show(client);

                printk("\n");
	}

	siw_touch_cnt = 5;

	mobis_touch_counter_reset();

	printk("----------------------------------------------------\n");

	mutex_unlock(&ts->lock);
        mutex_unlock(&ts->serdes.lock);
	siw_serdes_reset_dwork_start(chip->ts);

	return 0;
}

static ssize_t set_touch_test(struct device *dev, const char *buf, size_t count)
{
        int state;
        struct siw_touch_chip *chip = to_touch_chip(dev);
        struct siw_ts *ts = chip->ts;
        u8 buf1[3];
        unsigned short addr;
        struct i2c_client *client = to_i2c_client(dev);

        struct input_dev *input = ts->input;

        printk("%s Start\n",__func__);

        input_report_key(input, BTN_TOUCH, 1);

        input_report_key(input, BTN_TOOL_FINGER, 1);

        input_report_abs(input, ABS_MT_TRACKING_ID, 0);

        input_report_abs(input, ABS_MT_POSITION_X, 0x630);

        input_report_abs(input, ABS_MT_POSITION_Y, 0x17e);

        input_report_abs(input, ABS_MT_PRESSURE, 0x7);

        input_sync(input);

        mdelay(16);

	input_report_abs(input, ABS_MT_TRACKING_ID, -1);

        input_sync(input);

	if(count < INT_MAX)
                return count;
        else
                return INT_MAX;
}

ssize_t siw_set_adm(struct device *dev, const char *buf, size_t count)
{
        return set_adm(dev, buf, count);
}
EXPORT_SYMBOL(siw_set_adm);

static ssize_t set_des_bitrate_change(struct device *dev, const char *buf)
{
	struct siw_touch_chip *chip = to_touch_chip(dev);
        struct siw_ts *ts = chip->ts;
        struct i2c_client *client = to_i2c_client(dev);

	printk("%s [Start]\n",__func__);

	siw_serdes_reset_dwork_stop(chip->ts);
        mutex_lock(&ts->serdes.lock);
        mutex_lock(&ts->lock);

	des_i2c_bitrate_3gbps_to_6gbps(client);

	mutex_unlock(&ts->lock);
        mutex_unlock(&ts->serdes.lock);
        siw_serdes_reset_dwork_start(chip->ts);

        return 0;
}	

static ssize_t show_serdes_log(struct device *dev, const char *buf)
{
        struct siw_touch_chip *chip = to_touch_chip(dev);
        struct siw_ts *ts = chip->ts;
        struct i2c_client *client = to_i2c_client(dev);

	unsigned short addr;
	u8 data[255];

        int i;
        int ret;

        char serdes_log[10];
        char buffer[4];
        int sizeofbuffer, sizeofserdes_log;
        int lock_pin, line_fault;
        memset(serdes_log, 0x00, sizeof(serdes_log));

	if(get_montype())
        {
                printk("%s This function support only departed lcd model which is include Serdes chip\n",__func__);
                return 0;
        }

        printk("%s [Start]\n",__func__);

        siw_serdes_reset_dwork_stop(chip->ts);
        mutex_lock(&ts->serdes.lock);
	mutex_lock(&ts->lock);

	serdes_log[0] = 0x62;
        serdes_log[1] = 0xF1;
        serdes_log[2] = 0x20;

        lock_pin = siw_serdes_i2c_connect(client);
        serdes_video_signal_check(client, lock_pin);
        line_fault = siw_serdes_line_fault_check(client);

        if(siw_serdes_check_value[1] != 0xE1)
                serdes_log[3] = 0x01;

        if((line_fault == 0x2E) && ((lock_pin&0xfa)!=0xda))
                serdes_log[4] = 0x01;

        if((lock_pin&0xfa)!=0xda)
        {
                serdes_log[5] = 0x01;
        }
        else if((siw_serdes_check_value[0] >= 0x20) || (siw_serdes_check_value[4] >= 0x20))
        {
                serdes_log[5] = 0x01;
        }

        mobis_serdes_status_count_read(buffer);

	sizeofbuffer = sizeof(buffer) / sizeof(char);
        sizeofserdes_log = sizeof(serdes_log) / sizeof(char);

        for(i = 0;i < sizeofbuffer ;i++)
                serdes_log[sizeofserdes_log - sizeofbuffer + i] = buffer[i];

        for(i = 0; i < sizeofserdes_log ; i++)
        {
                printk("Serdes Byte[%d] : %02X\n",i+1,serdes_log[i]);
        }

        snprintf(data, sizeof(data), "%02X%02X%02X%02X%02X%02X%02X\n", serdes_log[3], serdes_log[4], serdes_log[5], serdes_log[6], serdes_log[7], serdes_log[8], serdes_log[9]);

        ret = snprintf(buf, 255, "%s\n", data);

	mobis_serdes_status_count_reset();

	mutex_unlock(&ts->lock);
        mutex_unlock(&ts->serdes.lock);
        siw_serdes_reset_dwork_start(chip->ts);

	return ret;
}

#endif	/* SIW_SUPPORT_ALIVE_DETECTION */


#define SIW_TOUCH_HAL_ATTR(_name, _show, _store)	\
		TOUCH_ATTR(_name, _show, _store)

#define _SIW_TOUCH_HAL_ATTR_T(_name)	\
		touch_attr_##_name

static SIW_TOUCH_HAL_ATTR(reg_list, show_reg_list, NULL);
static SIW_TOUCH_HAL_ATTR(reg_ctrl, show_reg_ctrl, store_reg_ctrl);
#if defined(__SIW_CONFIG_KNOCK)
static SIW_TOUCH_HAL_ATTR(tci_debug, _show_tci_debug, _store_tci_debug);
static SIW_TOUCH_HAL_ATTR(lcd_mode, _show_lcd_mode, _store_lcd_mode);
#endif
#if defined(__SIW_CONFIG_SWIPE)
static SIW_TOUCH_HAL_ATTR(swipe_debug, _show_swipe_debug, _store_swipe_debug);
#endif
static SIW_TOUCH_HAL_ATTR(reset_ctrl, show_reset_ctrl, store_reset_ctrl);
#if defined(SIW_ATTR_RST_BY_READ)
static SIW_TOUCH_HAL_ATTR(reset_sw, show_reset_sw, NULL);
static SIW_TOUCH_HAL_ATTR(reset_hw, show_reset_hw, NULL);
#endif
#if defined(__SIW_USE_BUS_TEST)
static SIW_TOUCH_HAL_ATTR(debug_bus, _show_debug_bus, NULL);
#endif
static SIW_TOUCH_HAL_ATTR(debug_hal, show_debug_hal, store_debug_hal);
#if defined(SIW_SUPPORT_ALIVE_DETECTION)
static SIW_TOUCH_HAL_ATTR(alive_debug, show_alive_debug, store_alive_debug);
static SIW_TOUCH_HAL_ATTR(alive_count, show_alive_count, store_alive_count);
static SIW_TOUCH_HAL_ATTR(alive_level, show_alive_level, store_alive_level);
#endif
static SIW_TOUCH_HAL_ATTR(8_debug, show_debug, set_debug);
static SIW_TOUCH_HAL_ATTR(power_off, show_power_off, NULL);
static SIW_TOUCH_HAL_ATTR(power_on, show_power_on, NULL);
static SIW_TOUCH_HAL_ATTR(serdes_setting, show_serdes_setting, set_serdes_setting);
static SIW_TOUCH_HAL_ATTR(des_setting, NULL, set_des_setting);
static SIW_TOUCH_HAL_ATTR(recovery_off, show_recovery_off, NULL);
static SIW_TOUCH_HAL_ATTR(recovery_on, show_recovery_on, NULL);
static SIW_TOUCH_HAL_ATTR(read_register, show_read_register, NULL);
static SIW_TOUCH_HAL_ATTR(adm, NULL, set_adm);
static SIW_TOUCH_HAL_ATTR(test, NULL, set_test);
static SIW_TOUCH_HAL_ATTR(mute_on, show_mute_on, NULL);
static SIW_TOUCH_HAL_ATTR(mute_off, show_mute_off, NULL);
static SIW_TOUCH_HAL_ATTR(int_on, set_int_on, NULL);
static SIW_TOUCH_HAL_ATTR(int_off, set_int_off, NULL);
static SIW_TOUCH_HAL_ATTR(set_serdes_6gbps, NULL, set_serdes_6gbps);
static SIW_TOUCH_HAL_ATTR(set_serdes_3gbps, NULL, set_serdes_3gbps);
static SIW_TOUCH_HAL_ATTR(linklock_disconnect, set_des_bitrate_change, NULL);
static SIW_TOUCH_HAL_ATTR(touch_test, NULL, set_touch_test);
static SIW_TOUCH_HAL_ATTR(lvds_con, NULL, set_lvds_con);
static SIW_TOUCH_HAL_ATTR(blu_con, show_blu_reg, set_blu_con);
static SIW_TOUCH_HAL_ATTR(touch_log, set_touch_log, NULL);
static SIW_TOUCH_HAL_ATTR(serdes_log, show_serdes_log, NULL);

static struct attribute *siw_hal_attribute_list[] = {
	&_SIW_TOUCH_HAL_ATTR_T(reg_list).attr,
	&_SIW_TOUCH_HAL_ATTR_T(reg_ctrl).attr,
#if defined(__SIW_CONFIG_KNOCK)
	&_SIW_TOUCH_HAL_ATTR_T(tci_debug).attr,
	&_SIW_TOUCH_HAL_ATTR_T(lcd_mode).attr,
#endif
#if defined(__SIW_CONFIG_SWIPE)
	&_SIW_TOUCH_HAL_ATTR_T(swipe_debug).attr,
#endif
	&_SIW_TOUCH_HAL_ATTR_T(reset_ctrl).attr,
#if defined(SIW_ATTR_RST_BY_READ)
	&_SIW_TOUCH_HAL_ATTR_T(reset_sw).attr,
	&_SIW_TOUCH_HAL_ATTR_T(reset_hw).attr,
#endif
#if defined(__SIW_USE_BUS_TEST)
	&_SIW_TOUCH_HAL_ATTR_T(debug_bus).attr,
#endif
	&_SIW_TOUCH_HAL_ATTR_T(debug_hal).attr,
#if defined(SIW_SUPPORT_ALIVE_DETECTION)
	&_SIW_TOUCH_HAL_ATTR_T(alive_debug).attr,
	&_SIW_TOUCH_HAL_ATTR_T(alive_count).attr,
	&_SIW_TOUCH_HAL_ATTR_T(alive_level).attr,
#endif
	&_SIW_TOUCH_HAL_ATTR_T(8_debug).attr,
	&_SIW_TOUCH_HAL_ATTR_T(power_off).attr,
	&_SIW_TOUCH_HAL_ATTR_T(power_on).attr,
	&_SIW_TOUCH_HAL_ATTR_T(serdes_setting).attr,
	&_SIW_TOUCH_HAL_ATTR_T(des_setting).attr,
	&_SIW_TOUCH_HAL_ATTR_T(recovery_off).attr,
        &_SIW_TOUCH_HAL_ATTR_T(recovery_on).attr,
	&_SIW_TOUCH_HAL_ATTR_T(read_register).attr,
	&_SIW_TOUCH_HAL_ATTR_T(adm).attr,
	&_SIW_TOUCH_HAL_ATTR_T(test).attr,
	&_SIW_TOUCH_HAL_ATTR_T(mute_on).attr,
        &_SIW_TOUCH_HAL_ATTR_T(mute_off).attr,
	&_SIW_TOUCH_HAL_ATTR_T(int_on).attr,
        &_SIW_TOUCH_HAL_ATTR_T(int_off).attr,
        &_SIW_TOUCH_HAL_ATTR_T(set_serdes_6gbps).attr,
        &_SIW_TOUCH_HAL_ATTR_T(set_serdes_3gbps).attr,
	&_SIW_TOUCH_HAL_ATTR_T(linklock_disconnect).attr,
	&_SIW_TOUCH_HAL_ATTR_T(touch_test).attr,
	&_SIW_TOUCH_HAL_ATTR_T(lvds_con).attr,
	&_SIW_TOUCH_HAL_ATTR_T(blu_con).attr,
	&_SIW_TOUCH_HAL_ATTR_T(touch_log).attr,
	&_SIW_TOUCH_HAL_ATTR_T(serdes_log).attr,
	NULL,
};

static const struct attribute_group siw_hal_attribute_group = {
	.attrs = siw_hal_attribute_list,
};

static int __siw_hal_sysfs_add_abt(struct device *dev, int on_off)
{
	struct siw_touch_chip *chip = to_touch_chip(dev);
	struct siw_ts *ts = chip->ts;
	int ret = 0;

	ret = siw_ops_abt_sysfs(ts, on_off);
	if ((on_off == DRIVER_INIT) && (ret < 0)) {
		t_dev_err(dev, "%s abt sysfs register failed\n",
			touch_chip_name(ts));
	}

	return ret;
}

static int __siw_hal_sysfs_add_prd(struct device *dev, int on_off)
{
	struct siw_touch_chip *chip = to_touch_chip(dev);
	struct siw_ts *ts = chip->ts;
	int ret = 0;

	ret = siw_ops_prd_sysfs(ts, on_off);
	if ((on_off == DRIVER_INIT) && (ret < 0)) {
		t_dev_err(dev, "%s prd sysfs register failed\n",
			touch_chip_name(ts));
	}

	return ret;
}

static int __siw_hal_sysfs_add_watch(struct device *dev, int on_off)
{
	struct siw_touch_chip *chip = to_touch_chip(dev);
	struct siw_ts *ts = chip->ts;
	int ret = 0;

	ret = siw_ops_watch_sysfs(ts, on_off);
	if ((on_off == DRIVER_INIT) && (ret < 0)) {
		t_dev_err(dev, "%s watch sysfs register failed\n",
			touch_chip_name(ts));
	}

	return ret;
}


static int siw_hal_sysfs_add(struct device *dev, int on_off)
{
	int ret = 0;

	if (on_off == DRIVER_INIT) {
		ret = __siw_hal_sysfs_add_abt(dev, DRIVER_INIT);
		if (ret < 0) {
			goto out;
		}

		ret = __siw_hal_sysfs_add_prd(dev, DRIVER_INIT);
		if (ret < 0) {
			goto out_prd;
		}

		ret = __siw_hal_sysfs_add_watch(dev, DRIVER_INIT);
		if (ret < 0) {
			goto out_watch;
		}
		return 0;
	}

	__siw_hal_sysfs_add_watch(dev, DRIVER_FREE);

out_watch:
	__siw_hal_sysfs_add_prd(dev, DRIVER_FREE);

out_prd:
	__siw_hal_sysfs_add_abt(dev, DRIVER_FREE);

out:
	return ret;
}

static int siw_hal_create_sysfs(struct device *dev)
{
	struct siw_touch_chip *chip = to_touch_chip(dev);
	struct siw_ts *ts = chip->ts;
	struct kobject *kobj = &ts->kobj;
	int ret = 0;

	ret = sysfs_create_group(kobj, &siw_hal_attribute_group);
	if (ret < 0) {
		t_dev_err(dev, "%s sysfs register failed\n",
				touch_chip_name(ts));
		goto out;
	}

	ret = siw_hal_sysfs_add(dev, DRIVER_INIT);
	if (ret < 0) {
		goto out_add;
	}

	t_dev_dbg_base(dev, "%s sysfs registered\n",
			touch_chip_name(ts));

	return 0;

out_add:
	sysfs_remove_group(kobj, &siw_hal_attribute_group);

out:

	return ret;
}

static void siw_hal_remove_sysfs(struct device *dev)
{
	struct siw_touch_chip *chip = to_touch_chip(dev);
	struct siw_ts *ts = chip->ts;

	siw_hal_sysfs_add(dev, DRIVER_FREE);

	sysfs_remove_group(&ts->kobj, &siw_hal_attribute_group);

	t_dev_dbg_base(dev, "%s sysfs unregistered\n",
			touch_chip_name(ts));
}

int siw_hal_sysfs(struct device *dev, int on_off)
{
	if (on_off == DRIVER_INIT) {
		return siw_hal_create_sysfs(dev);
	}

	siw_hal_remove_sysfs(dev);
	return 0;
}


