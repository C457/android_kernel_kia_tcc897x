/*
 * siw_touch_sysfs.c - SiW touch sysfs driver
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
#include <linux/err.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/irqreturn.h>
#include <linux/slab.h>
#include <linux/pm.h>
#include <linux/gpio.h>
#include <linux/string.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_device.h>
#include <asm/page.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/memory.h>

#include "siw_touch.h"
#include "siw_touch_hal.h"
#include "siw_touch_irq.h"
#include "siw_touch_sys.h"

#define FW_PATH_EXTERNAL_10_25 "{root}/res/firmware/touchscreen/siw_10_25.img"
#define FW_PATH_EXTERNAL_12_3 "{root}/res/firmware/touchscreen/siw_12_3.img"
#define FW_PATH_EXTERNAL_USB_10_25 "{root}/storage/usb0/siw_10_25.img"
#define FW_PATH_EXTERNAL_USB_12_3 "{root}/storage/usb0/siw_12_3.img"
int FW_UPDATE_RESULT = 0;



//#define __SYS_USE_LOCKSCREEN
//#define __SYS_USE_IME_STATE

#if defined(__SYS_USE_IME_STATE)
static const char *siw_ime_str[] = {
	"OFF",
	"ON",
	"SWYPE",
};
#endif

#if defined(__SIW_CONFIG_KNOCK)
static const char *siw_mfts_str[] = {
	"NONE",
	"FOLDER",
	"FLAT",
	"CURVED",
};
#endif

#if defined(__SIW_SUPPORT_ASC)
static const char *siw_incoming_call_str[] = {
	"IDLE",
	"RINGING",
	"OFFHOOK",
};

static const char *siw_onhand_str[] = {
	"IN_HAND_ATTN",
	"NOT_IN_HAND",
	"IN_HAND_NO_ATTN",
};
#endif	/* __SIW_SUPPORT_ASC */


#define siw_sysfs_err_invalid_param(_dev)	\
		t_dev_err(_dev, "Invalid param\n");

#define _plat_data_snprintf(_buf, _size, args...)	\
		siw_snprintf(_buf, _size, " %-25s = %d\n", ##args)

static void siw_touch_sysfs_gen_symlink(struct siw_ts *ts, char *name)
{
	struct device *dev = ts->dev;
	struct kobject *kobj = &ts->kobj;
	int ret = 0;

	if (!name || !strlen(name)) {
		return;
	}

	if (strcmp(kobj->name, name) != 0) {
		ret = sysfs_create_link(kobj->parent, kobj, name);
		if (ret >= 0) {
			t_dev_info(dev, "symlink generated, %s\n", name);
		}
	}
}

static void siw_touch_sysfs_del_symlink(struct siw_ts *ts, char *name)
{
	struct kobject *kobj = &ts->kobj;

	if (!name || !strlen(name)) {
		return;
	}

	if (strcmp(kobj->name, name) != 0) {
		sysfs_remove_link(kobj->parent, name);
	}
}


static ssize_t show_do_plat_data(struct device *dev, char *buf)
{
	struct siw_ts *ts = to_touch_core(dev);
	struct touch_device_caps *caps = &ts->caps;
	int i;
	int size = 0;

	size += siw_snprintf(buf, size, "=== Platform Data ===\n");
	size += _plat_data_snprintf(buf, size,
					"reset_pin", touch_reset_pin(ts));
	size += _plat_data_snprintf(buf, size,
					"reset_pin_pol", touch_reset_pin_pol(ts));
	size += _plat_data_snprintf(buf, size,
					"irq_pin", touch_irq_pin(ts));
	size += _plat_data_snprintf(buf, size,
					"maker_id_pin", touch_maker_id_pin(ts));

	size += siw_snprintf(buf, size, "power:\n");
	size += _plat_data_snprintf(buf, size,
					"vdd-gpio", touch_vdd_pin(ts));
	size += _plat_data_snprintf(buf, size,
					"vio-gpio", touch_vio_pin(ts));

	size += siw_snprintf(buf, size, "caps:\n");
	size += _plat_data_snprintf(buf, size,
					"max_x", caps->max_x);
	size += _plat_data_snprintf(buf, size,
					"max_y", caps->max_y);
	size += _plat_data_snprintf(buf, size,
					"max_pressure", caps->max_pressure);
	size += _plat_data_snprintf(buf, size,
					"max_width", caps->max_width);
	size += _plat_data_snprintf(buf, size,
					"max_orientation", caps->max_orientation);
	size += _plat_data_snprintf(buf, size,
					"max_id", caps->max_id);
	size += _plat_data_snprintf(buf, size,
					"hw_reset_delay", caps->hw_reset_delay);
	size += _plat_data_snprintf(buf, size,
					"sw_reset_delay", caps->sw_reset_delay);

	size += siw_snprintf(buf, size, "role:\n");
	size += _plat_data_snprintf(buf, size,
					"use_lpwg", ts->role.use_lpwg);
	size += _plat_data_snprintf(buf, size,
					"use_firmware", ts->role.use_firmware);
	size += _plat_data_snprintf(buf, size,
					"use_fw_upgrade", ts->role.use_fw_upgrade);

	size += siw_snprintf(buf, size, "firmware:\n");
	size += _plat_data_snprintf(buf, size,
					"def_fwcnt", ts->def_fwcnt);
	for (i = 0; i < ts->def_fwcnt; i++)
		size += siw_snprintf(buf, size, " %-25s : [%d] %s\n",
						"def_fwpath", i, ts->def_fwpath[i]);

	return size;
}

static ssize_t show_plat_data(struct device *dev, char *buf)
{
	struct siw_ts *ts = to_touch_core(dev);
	int size = 0;

	mutex_lock(&ts->lock);
	size = show_do_plat_data(dev, buf);
	mutex_unlock(&ts->lock);

	return size;
}

static ssize_t show_do_driver_data(struct device *dev, char *buf)
{
//	struct siw_ts *ts = to_touch_core(dev);
	int size = 0;

	size += siw_snprintf(buf, size, "=== Driver Data ===\n");

	size += siw_snprintf(buf, size,
						"Version : %s\n", SIW_DRV_VERSION);

	return size;
}

static ssize_t show_driver_data(struct device *dev, char *buf)
{
	struct siw_ts *ts = to_touch_core(dev);
	int size = 0;

	mutex_lock(&ts->lock);
	size = show_do_driver_data(dev, buf);
	mutex_unlock(&ts->lock);

	return size;
}

static ssize_t show_update_result(struct device *dev, char *buf)
{
	int ret;
        struct siw_ts *ts = to_touch_core(dev);

	t_dev_info(dev, "%s [Start] %d\n",__func__, FW_UPDATE_RESULT);

	ret = snprintf(buf, 255, "%d\n", FW_UPDATE_RESULT);

        return ret;
}

static ssize_t store_update(struct device *dev,
                                const char *buf, size_t count)
{
        struct siw_ts *ts = to_touch_core(dev);
	int state;

	if(sscanf(buf, "%d", &state) < 0)
		return -EINVAL;

	t_dev_info(dev, "%s [Start] %d\n",__func__, state);

	if((state == 1) || (state == 3))
	{
		t_dev_info(dev, "System F/W upgrade with %s\n", ts->test_fwpath);

		ts->test_fwpath[0] = '\0';

		if(state == 3)
			ts->force_fwup |= FORCE_FWUP_SYS_STORE;
	}
	else if((state == 2) || (state == 4))
	{
		switch(daudio_lcd_inch_check())
		{
			case 10 :
				if(state == 4)
				{
					if(sscanf(FW_PATH_EXTERNAL_10_25, "%255s", ts->test_fwpath) < 0)
						return -EINVAL;
				}
				else
				{
					if(sscanf(FW_PATH_EXTERNAL_USB_10_25, "%255s", ts->test_fwpath) < 0)
						return -EINVAL;
				}
				break;
			case 12 :
				if(state == 4)
				{
					if(sscanf(FW_PATH_EXTERNAL_12_3, "%255s", ts->test_fwpath) < 0)
						return -EINVAL;
				}
				else
				{
					if(sscanf(FW_PATH_EXTERNAL_USB_12_3, "%255s", ts->test_fwpath) < 0)
						return -EINVAL;
				}
				break;
			default :
				return -EINVAL;
		}

		t_dev_info(dev, "USB F/W upgrade with %s\n", ts->test_fwpath);

		ts->force_fwup |= FORCE_FWUP_SYS_STORE;
	}
	else
	{
		t_dev_info(dev, "F/W upgrade end with wrong value %d\n", state);
		return -EINVAL;
	}

        siw_touch_qd_upgrade_work_now(ts);

	t_dev_info(dev, "%s [Done]\n",__func__);

	if(count < INT_MAX)
                return count;
        else
                return INT_MAX;
}

static ssize_t store_upgrade(struct device *dev,
				const char *buf, size_t count)
{
	struct siw_ts *ts = to_touch_core(dev);

	if (sscanf(buf, "%255s", ts->test_fwpath) <= 0) {
		siw_sysfs_err_invalid_param(dev);
		if(count < INT_MAX)
	                return count;
	        else
	                return INT_MAX;
	}

	t_dev_info(dev, "Manual F/W upgrade with %s\n", ts->test_fwpath);

	ts->force_fwup |= FORCE_FWUP_SYS_STORE;

	siw_touch_qd_upgrade_work_now(ts);

	if(count < INT_MAX)
                return count;
        else
                return INT_MAX;
}

static ssize_t show_upgrade(struct device *dev, char *buf)
{
	struct siw_ts *ts = to_touch_core(dev);

	ts->force_fwup |= FORCE_FWUP_SYS_SHOW;
	ts->test_fwpath[0] = '\0';

	siw_touch_qd_upgrade_work_now(ts);

	return 0;
}

#if defined(__SIW_CONFIG_KNOCK)
static int __show_do_lpwg_data(struct device *dev, char *buf)
{
	struct siw_ts *ts = to_touch_core(dev);
	struct lpwg_info *lpwg = &ts->lpwg;
	int i;
	int size = 0;

	for (i = 0; i < MAX_LPWG_CODE; i++) {
		if ((lpwg->code[i].x == -1) && (lpwg->code[i].y == -1)) {
			break;
		}

		size += siw_snprintf(buf, size, "%d %d\n",
				lpwg->code[i].x, lpwg->code[i].y);

		if (!lpwg->code[i].x && !lpwg->code[i].y) {
			break;
		}
	}
	memset(lpwg->code, 0, sizeof(struct point) * MAX_LPWG_CODE);

	return size;
}

static ssize_t _show_lpwg_data(struct device *dev, char *buf)
{
	struct siw_ts *ts = to_touch_core(dev);
	int ret = 0;

	if (siw_ops_is_null(ts, lpwg))
		return ret;

	mutex_lock(&ts->lock);
	ret = __show_do_lpwg_data(dev, buf);
	mutex_unlock(&ts->lock);

	return ret;
}

static ssize_t _store_lpwg_data(struct device *dev,
				const char *buf, size_t count)
{
	struct siw_ts *ts = to_touch_core(dev);
	int reply = 0;

	if (sscanf(buf, "%d", &reply) <= 0) {
		siw_sysfs_err_invalid_param(dev);
		if(count < INT_MAX)
	                return count;
	        else
	                return INT_MAX;
	}

	t_dev_info(dev, "uevent reply %d\n", reply);

	atomic_set(&ts->state.uevent, UEVENT_IDLE);
#if defined(SIW_SUPPORT_WAKE_LOCK)
	wake_unlock(&ts->lpwg_wake_lock);
#endif

	if(count < INT_MAX)
                return count;
        else
                return INT_MAX;
}

static ssize_t _store_lpwg_notify(struct device *dev,
				const char *buf, size_t count)
{
	struct siw_ts *ts = to_touch_core(dev);
	int code = 0;
	int param[4] = {0, };
	int mfts_mode = 0;

	mfts_mode = siw_touch_boot_mode_check(dev);
	if ((mfts_mode >= MINIOS_MFTS_FOLDER) && !ts->role.mfts_lpwg)
	{
		if(count < INT_MAX)
	                return count;
	        else
	                return INT_MAX
	}

	if (sscanf(buf, "%d %d %d %d %d",
			&code, &param[0], &param[1], &param[2], &param[3]) <= 0) {
		siw_sysfs_err_invalid_param(dev);
		if(count < INT_MAX)
	                return count;
	        else
	                return INT_MAX;
	}

	/* only below code notify
		3 active_area
		4 knockcode tap count
		8 knockcode double tap check
		9 update_all
	*/
	switch (code) {
	case LPWG_ENABLE:
	case LPWG_LCD:
	case LPWG_LENGTH_BETWEEN_TAP:
	case LPWG_EARLY_SUSPEND:
	case LPWG_SENSOR_STATUS:
		goto out;
	default:
		if ((code >= LPWG_EXT_TCI_INFO_STORE) &&
			(code < LPWG_EXT_STORE_END)) {
			break;
		}

		if ((code >= LPWG_EXT_TCI_INFO_SHOW) &&
			(code < LPWG_EXT_SHOW_END)) {
			break;
		}

		if (code > LPWG_REPLY) {
			t_dev_err(dev, "unknown code, %d\n", code);
			goto out;
		}
		break;
	}

	mutex_lock(&ts->lock);
	siw_ops_lpwg(ts, code, param);
	mutex_unlock(&ts->lock);

out:
	if(count < INT_MAX)
                return count;
        else
                return INT_MAX;
}

static ssize_t _show_mfts_state(struct device *dev, char *buf)
{
	struct siw_ts *ts = to_touch_core(dev);
	int value = 0;
	int size = 0;

	value = atomic_read(&ts->state.mfts);

	size += siw_snprintf(buf, size, "%s : %s(%d)\n",
				__func__,
				siw_mfts_str[value], value);

	return (ssize_t)size;
}

static ssize_t _store_mfts_state(struct device *dev,
				const char *buf, size_t count)
{
	struct siw_ts *ts = to_touch_core(dev);
	int value = 0;

	if (sscanf(buf, "%d", &value) <= 0) {
		siw_sysfs_err_invalid_param(dev);
		if(count < INT_MAX)
	                return count;
	        else
	                return INT_MAX;
	}

	if (value >= MFTS_NONE && value <= MFTS_CURVED) {
		atomic_set(&ts->state.mfts, value);
		t_dev_info(dev, "MFTS : %s(%d)\n", siw_mfts_str[value], value);
	} else {
		t_dev_info(dev, "MFTS : Unknown state, %d\n", value);
	}

	if(count < INT_MAX)
                return count;
        else
                return INT_MAX;
}

static ssize_t _show_mfts_lpwg(struct device *dev, char *buf)
{
	struct siw_ts *ts = to_touch_core(dev);
	int size = 0;

	size += siw_snprintf(buf, size, "MFTS LPWG : %d\n",
				ts->role.use_lpwg_test);

	return (ssize_t)size;
}

static ssize_t _store_mfts_lpwg(struct device *dev,
				const char *buf, size_t count)
{
	struct siw_ts *ts = to_touch_core(dev);
	int value = 0;

	if (sscanf(buf, "%d", &value) <= 0) {
		siw_sysfs_err_invalid_param(dev);
		if(count < INT_MAX)
	                return count;
	        else
	                return INT_MAX
	}

	ts->role.mfts_lpwg = value;
	t_mfts_lpwg = value;
	t_dev_info(dev, "MFTS LPWG : %d\n", ts->role.mfts_lpwg);

	if(count < INT_MAX)
                return count;
        else
                return INT_MAX;
}
#endif	/* __SIW_CONFIG_KNOCK */

#if defined(__SYS_USE_LOCKSCREEN)
static ssize_t _show_lockscreen_state(struct device *dev, char *buf)
{
	struct siw_ts *ts = to_touch_core(dev);
	int value = 0;
	int size = 0;

	value = atomic_read(&ts->state.lockscreen);

	size += siw_snprintf(buf, size, "Lock-screen : %s(%d)\n",
				value ? "LOCK" : "UNLOCK", value);

	return (ssize_t)size;
}

static ssize_t _store_lockscreen_state(struct device *dev,
				const char *buf, size_t count)
{
	struct siw_ts *ts = to_touch_core(dev);
	int value = 0;

	if (sscanf(buf, "%d", &value) <= 0) {
		siw_sysfs_err_invalid_param(dev);
		if(count < INT_MAX)
	                return count;
	        else
	                return INT_MAX
	}

	if (value == LOCKSCREEN_UNLOCK || value == LOCKSCREEN_LOCK) {
		atomic_set(&ts->state.lockscreen, value);
		t_dev_info(dev, "Lock-screen : %s(%d)\n", value ? "LOCK" : "UNLOCK", value);
	} else {
		t_dev_info(dev, "Lock-screen : Unknown state, %d\n", value);
	}

	if(count < INT_MAX)
                return count;
        else
                return INT_MAX;
}
#endif	/* __SYS_USE_LOCKSCREEN */

#if defined(__SYS_USE_IME_STATE)
static ssize_t _show_ime_state(struct device *dev, char *buf)
{
	struct siw_ts *ts = to_touch_core(dev);
	int value = 0;
	int size = 0;

	value = atomic_read(&ts->state.ime);

	size += siw_snprintf(buf, size, "%s(%d)\n",
				siw_ime_str[value], value);

	return (ssize_t)size;
}

static ssize_t _store_ime_state(struct device *dev,
				const char *buf, size_t count)
{
	struct siw_ts *ts = to_touch_core(dev);
	int value = 0;
	int ret = 0;

	if (sscanf(buf, "%d", &value) <= 0) {
		siw_sysfs_err_invalid_param(dev);
		goto out;
	}

	if ((value < IME_OFF) || (value > IME_SWYPE)) {
		t_dev_warn(dev, "IME : Unknown state, %d\n", value);
		goto out;
	}

	if (atomic_read(&ts->state.ime) == value)
		goto out;

	if (touch_test_quirks(ts, CHIP_QUIRK_NOT_SUPPORT_IME)) {
		t_dev_err(dev, "ime control not supporeted in %s\n",
				touch_chip_name(ts));
		goto out;
	}

	atomic_set(&ts->state.ime, value);
	ret = siw_touch_blocking_notifier_call(NOTIFY_IME_STATE,
			&ts->state.ime);
	t_dev_info(dev, "IME : %s(%d), %d\n",
			siw_ime_str[value], value, ret);

out:
	if(count < INT_MAX)
                return count;
        else
                return INT_MAX;
}
#endif	/* __SYS_USE_IME_STATE */

static ssize_t show_quick_cover_state(struct device *dev, char *buf)
{
	struct siw_ts *ts = to_touch_core(dev);
	int value = 0;
	int size = 0;

	value = atomic_read(&ts->state.quick_cover);

	size += siw_snprintf(buf, size, "Qcover : %s(%d)\n",
				value ? "CLOSE" : "OPEN", value);

	return (ssize_t)size;
}

static ssize_t store_quick_cover_state(struct device *dev,
				const char *buf, size_t count)
{
	int value = 0;
	struct siw_ts *ts = to_touch_core(dev);

	if (sscanf(buf, "%d", &value) <= 0) {
		siw_sysfs_err_invalid_param(dev);
		if(count < INT_MAX)
	                return count;
	        else
	                return INT_MAX;
	}

	if ((value == QUICKCOVER_CLOSE) || (value == QUICKCOVER_OPEN)) {
		atomic_set(&ts->state.quick_cover, value);
		t_dev_info(dev, "Qcover : %s(%d)\n",
			value ? "CLOSE" : "OPEN", value);
	} else {
		t_dev_info(dev, "Qcover : Unknown state, %d\n", value);
	}

	if(count < INT_MAX)
                return count;
        else
                return INT_MAX;
}

static ssize_t show_version_info(struct device *dev, char *buf)
{
	return siw_touch_get(dev, CMD_VERSION, buf);
}

static ssize_t show_atcmd_version_info(struct device *dev, char *buf)
{
	return siw_touch_get(dev, CMD_ATCMD_VERSION, buf);
}

static ssize_t show_sp_link_touch_off(struct device *dev, char *buf)
{
	struct siw_ts *ts = to_touch_core(dev);
	int size = 0;

	size += siw_snprintf(buf, size, "SP link touch status %d\n",
				atomic_read(&ts->state.sp_link));

	return (ssize_t)size;
}

static ssize_t store_sp_link_touch_off(struct device *dev,
				const char *buf, size_t count)
{
	struct siw_ts *ts = to_touch_core(dev);
	int value = 0;

	if (sscanf(buf, "%d", &value) <= 0) {
		siw_sysfs_err_invalid_param(dev);
		if(count < INT_MAX)
	        	return count;
		else
			return INT_MAX;
	}

	mutex_lock(&ts->lock);

	siw_touch_irq_control(ts->dev,
			(value == SP_CONNECT) ?
			INTERRUPT_DISABLE : INTERRUPT_ENABLE);

	t_dev_info(dev, "SP Mirroring %s\n",
		(value == SP_CONNECT) ? "Connected" : "Disconnected");

	atomic_set(&ts->state.sp_link, value);

	mutex_unlock(&ts->lock);

	if(count < INT_MAX)
                return count;
        else
                return INT_MAX;
}

static ssize_t show_irq_state(struct device *dev, char *buf)
{
	struct siw_touch_chip *chip = to_touch_chip(dev);
	struct siw_ts *ts = chip->ts;
	int irq_status = atomic_read(&ts->state.irq_enable);
	int size = 0;

	size = siw_snprintf(buf, size,
				"Irq State : %s\n",
				(irq_status) ? "Enabled" : "Disabled");

	return size;
}

static ssize_t store_irq_state(struct device *dev,
				const char *buf, size_t count)
{
	struct siw_touch_chip *chip = to_touch_chip(dev);
	struct siw_ts *ts = chip->ts;
	int value = 0;

	if (sscanf(buf, "%d", &value) <= 0) {
		siw_sysfs_err_invalid_param(dev);
		if(count < INT_MAX)
			return count;
	        else
			return INT_MAX;
	}

	mutex_lock(&ts->lock);

	siw_touch_irq_control(ts->dev,
			(value) ? INTERRUPT_ENABLE : INTERRUPT_DISABLE);

	mutex_unlock(&ts->lock);

	return (ssize_t)count;
}

static ssize_t show_irq_level(struct device *dev, char *buf)
{
	struct siw_touch_chip *chip = to_touch_chip(dev);
	struct siw_ts *ts = chip->ts;
	int irq_level = gpio_get_value(touch_irq_pin(ts));
	int size = 0;

//	t_dev_dbg_base(dev, "irq_level : %d\n", irq_level);
	size = siw_snprintf(buf, size,
				"%d\n",
				irq_level);

	return size;
}

static ssize_t show_debug_tool_state(struct device *dev, char *buf)
{
	struct siw_ts *ts = to_touch_core(dev);
	int value = 0;
	int size = 0;

	value = atomic_read(&ts->state.debug_tool);

	size += siw_snprintf(buf, size, "Debug tool state : %d\n", value);

	return (ssize_t)size;
}

static ssize_t store_debug_tool_state(struct device *dev,
				const char *buf, size_t count)
{
	struct siw_ts *ts = to_touch_core(dev);
	int data = 0;

	if (sscanf(buf, "%d", &data) <= 0) {
		siw_sysfs_err_invalid_param(dev);
		if(count < INT_MAX)
	                return count;
	        else
	                return INT_MAX;
	}

	if (siw_ops_is_null(ts, notify))
	{
		if(count < INT_MAX)
	                return count;
	        else
	                return INT_MAX;
	}

	if ((data >= DEBUG_TOOL_DISABLE) && (data <= DEBUG_TOOL_ENABLE)) {
		atomic_set(&ts->state.debug_tool, data);
		siw_ops_notify(ts, NOTIFY_DEBUG_TOOL, (void *)&data);
		t_dev_info(dev, "Debug tool state : %s\n",
				(data == DEBUG_TOOL_ENABLE) ?
				"Debug tool Enabled" : "Debug tool Disabled");
	} else {
		t_dev_info(dev, "Debug tool state : Invalid value, %d\n", data);
	}

	if(count < INT_MAX)
                return count;
        else
                return INT_MAX;
}

static ssize_t show_debug_option_state(struct device *dev, char *buf)
{
#if defined(__SIW_SUPPORT_DEBUG_OPTION)
	struct siw_ts *ts = to_touch_core(dev);
	int value = 0;
	int size = 0;

	value = atomic_read(&ts->state.debug_option_mask);

	size += siw_snprintf(buf, size, "Debug option : %d\n", value);

	return (ssize_t)size;
#else
	return 0;
#endif
}

static ssize_t store_debug_option_state(struct device *dev,
				const char *buf, size_t count)
{
#if defined(__SIW_SUPPORT_DEBUG_OPTION)
	struct siw_ts *ts = to_touch_core(dev);
	int new_mask = 0;

	if (sscanf(buf, "%d", &new_mask) <= 0) {
		siw_sysfs_err_invalid_param(dev);
		if(count < INT_MAX)
	                return count;
	        else
	                return INT_MAX;
	}

	if (new_mask >= DEBUG_OPTION_DISABLE
		&& new_mask <= DEBUG_OPTION_1_2_3) {
		atomic_set(&ts->state.debug_option_mask, new_mask);
		t_dev_info(dev, "Debug option : Input masking value = %d",
				new_mask);
	} else {
		t_dev_info(dev, "Debug option : Invalid value, %d\n", new_mask);
	}
#endif
	if(count < INT_MAX)
                return count;
        else
                return INT_MAX;
}

#if defined(__SIW_SUPPORT_ASC)
static ssize_t _show_incoming_call_state(struct device *dev, char *buf)
{
	struct siw_ts *ts = to_touch_core(dev);
	int value = 0;
	int size = 0;

	value = atomic_read(&ts->state.incoming_call);

	size += siw_snprintf(buf, size, "%s : %s(%d)\n",
				__func__,
				siw_incoming_call_str[value], value);

	return (ssize_t)size;
}

static ssize_t _store_incoming_call_state(struct device *dev,
				const char *buf, size_t count)
{
	struct siw_ts *ts = to_touch_core(dev);
	int value = 0;
	int ret = 0;

	if (sscanf(buf, "%d", &value) <= 0) {
		siw_sysfs_err_invalid_param(dev);
		if(count < INT_MAX)
	                return count;
	        else
	                return INT_MAX;
	}

	if (value >= INCOMING_CALL_IDLE && value <= INCOMING_CALL_OFFHOOK) {
		if (atomic_read(&ts->state.incoming_call) == value)
		{
			if(count < INT_MAX)
		                return count;
		        else
		                return INT_MAX;
		}

		atomic_set(&ts->state.incoming_call, value);

		ret = siw_touch_blocking_notifier_call(NOTIFY_CALL_STATE,
					&ts->state.incoming_call);

		if (ts->asc.use_asc == ASC_ON)
			siw_touch_qd_toggle_delta_work_jiffies(ts, 0);

		t_dev_info(dev, "Incoming-call : %s(%d)\n",
				siw_incoming_call_str[value], value);
	} else {
		t_dev_info(dev, "Incoming-call : Unknown %d\n", value);
	}

	if(count < INT_MAX)
                return count;
        else
                return INT_MAX;
}

static ssize_t _show_asc_param(struct device *dev, char *buf)
{
	struct siw_ts *ts = to_touch_core(dev);
	struct asc_info *asc = &(ts->asc);
	int size = 0;

	size += siw_snprintf(buf, size, "use_asc = %d\n",
				asc->use_asc);
	size += siw_snprintf(buf, size, "low_delta_thres = %d\n",
				asc->low_delta_thres);
	size += siw_snprintf(buf, size, "high_delta_thres = %d\n",
				asc->high_delta_thres);

	return (ssize_t)size;
}

static ssize_t _store_asc_param(struct device *dev,
				const char *buf, size_t count)
{
	struct siw_ts *ts = to_touch_core(dev);
	struct asc_info *asc = &(ts->asc);
	unsigned char string[30] = {0, };
	u32 value = 0;

	if (touch_test_quirks(ts, CHIP_QUIRK_NOT_SUPPORT_ASC)) {
		t_dev_err(dev, "asc control not supporeted in %s\n",
				touch_chip_name(ts));
		goto out;
	}

	if (sscanf(buf, "%s %d", string, &value) <= 0) {
		siw_sysfs_err_invalid_param(dev);
		if(count < INT_MAX)
	                return count;
	        else
	                return INT_MAX;
	}

	if (!strcmp(string, "use_asc")) {
		if (value == ASC_OFF) {
			asc->use_delta_chk = DELTA_CHK_OFF;
			siw_touch_change_sensitivity(ts, NORMAL_SENSITIVITY);
			asc->use_asc = ASC_OFF;
		} else if (value == ASC_ON) {
			asc->use_asc = ASC_ON;
			mutex_lock(&ts->lock);
			siw_ops_asc(ts, ASC_GET_FW_SENSITIVITY, 0);
			mutex_unlock(&ts->lock);
			siw_touch_qd_toggle_delta_work_jiffies(ts, 0);
		} else {
			t_dev_info(dev, "ASC : Invalid value, %d\n", value);
		}

	} else if (!strcmp(string, "low_delta_thres")) {
		asc->low_delta_thres = value;
	} else if (!strcmp(string, "high_delta_thres")) {
		asc->high_delta_thres = value;
	}

out:
	if(count < INT_MAX)
                return count;
        else
                return INT_MAX;
}

static ssize_t _show_onhand(struct device *dev, char *buf)
{
	struct siw_ts *ts = to_touch_core(dev);
	int value = 0;
	int size = 0;

	value = atomic_read(&ts->state.onhand);

	t_dev_info(dev, "Hand : %s(%d)\n", siw_onhand_str[value], value);

	size += siw_snprintf(buf, size, "%d\n", value);

	return (ssize_t)size;
}

static ssize_t _store_onhand(struct device *dev,
				const char *buf, size_t count)
{
	struct siw_ts *ts = to_touch_core(dev);
	int value = 0;

	if (sscanf(buf, "%d", &value) <= 0) {
		siw_sysfs_err_invalid_param(dev);
		if(count < INT_MAX)
	                return count;
	        else
	                return INT_MAX;
	}

	if (value >= IN_HAND_ATTN && value <= IN_HAND_NO_ATTN) {
		atomic_set(&ts->state.onhand, value);

		if (ts->asc.use_asc == ASC_ON)
			siw_touch_qd_toggle_delta_work_jiffies(ts, 0);

		t_dev_info(dev, "Hand : %s(%d)\n", siw_onhand_str[value], value);
	} else {
		t_dev_info(dev, "Hand : Invalid value, %d\n", value);
	}

	if(count < INT_MAX)
                return count;
        else
                return INT_MAX;
}
#endif	/* __SIW_SUPPORT_ASC */

static ssize_t show_module_info(struct device *dev, char *buf)
{
	struct siw_ts *ts = to_touch_core(dev);
	int size = 0;

	size += siw_snprintf(buf, size, "%s/%s/%s, %s\n",
			dev_name(dev->parent->parent),
			dev_name(dev->parent),
			dev_name(dev),
			dev_name(&ts->input->dev));

	return (ssize_t)size;
}

u32 __weak t_mon_dbg_mask;	//instead of using extern
u32 __weak t_bus_dbg_mask;	//instead of using extern

static ssize_t show_dbg_mask(struct device *dev, char *buf)
{
//	struct siw_ts *ts = to_touch_core(dev);
	int size = 0;

	size += siw_snprintf(buf, size,
				"t_dev_dbg_mask %08Xh\n", t_dev_dbg_mask);
	size += siw_snprintf(buf, size,
				"t_pr_dbg_mask  %08Xh\n", t_pr_dbg_mask);
	size += siw_snprintf(buf, size,
				"t_mon_dbg_mask %08Xh\n", t_mon_dbg_mask);
	size += siw_snprintf(buf, size,
				"t_bus_dbg_mask %08Xh\n", t_bus_dbg_mask);

	size += siw_snprintf(buf, size,
				"Usage:\n");
	size += siw_snprintf(buf, size,
				" t_dev_dbg_mask : echo 0 {mask_value} > dbg_mask\n");
	size += siw_snprintf(buf, size,
				" t_pr_dbg_mask  : echo 1 {mask_value} > dbg_mask\n");
	size += siw_snprintf(buf, size,
				" t_mon_dbg_mask : echo 2 {mask_value} > dbg_mask\n");
	size += siw_snprintf(buf, size,
				" t_bus_dbg_mask : echo 3 {mask_value} > dbg_mask\n");

	return (ssize_t)size;
}

static void store_dbg_mask_usage(struct device *dev)
{
	t_dev_info(dev, "Usage:\n");
	t_dev_info(dev, " t_dev_dbg_mask : echo 0 {mask_value(hex)} > dbg_mask\n");
	t_dev_info(dev, " t_pr_dbg_mask  : echo 1 {mask_value(hex)} > dbg_mask\n");
	t_dev_info(dev, " t_mon_dbg_mask : echo 2 {mask_value(hex)} > dbg_mask\n");
	t_dev_info(dev, " t_bus_dbg_mask : echo 3 {mask_value(hex)} > dbg_mask\n");
}

static ssize_t store_dbg_mask(struct device *dev,
				const char *buf, size_t count)
{
	struct siw_ts *ts = to_touch_core(dev);
	int type = 0;
	u32 old_value, new_value = 0;

	if (sscanf(buf, "%d %X", &type, &new_value) <= 0) {
		siw_sysfs_err_invalid_param(dev);
		store_dbg_mask_usage(dev);
		if(count < INT_MAX)
	                return count;
	        else
	                return INT_MAX;
	}

	switch (type) {
	case 0 :
		old_value = t_dev_dbg_mask;
		t_dev_dbg_mask = new_value;
		t_dev_info(dev, "t_dev_dbg_mask changed : %08Xh -> %08xh\n",
			old_value, new_value);
		break;
	case 1 :
		old_value = t_pr_dbg_mask;
		t_pr_dbg_mask = new_value;
		t_dev_info(dev, "t_pr_dbg_mask changed : %08Xh -> %08xh\n",
			old_value, new_value);
		break;
	case 2 :
		old_value = t_mon_dbg_mask;
		t_mon_dbg_mask = new_value;
		t_dev_info(dev, "t_mon_dbg_mask changed : %08Xh -> %08xh\n",
			old_value, new_value);
		if (!(touch_flags(ts) & TOUCH_USE_MON_THREAD)) {
			t_dev_info(dev, "(but, mon thread not activated)\n");
		}
		break;
	case 3 :
		old_value = t_bus_dbg_mask;
		t_bus_dbg_mask = new_value;
		t_dev_info(dev, "t_bus_dbg_mask changed : %08Xh -> %08xh\n",
			old_value, new_value);
		break;
	default :
		store_dbg_mask_usage(dev);
		break;
	}
	if(count < INT_MAX)
                return count;
        else
                return INT_MAX;
}

static ssize_t show_dbg_flag(struct device *dev, char *buf)
{
//	struct siw_ts *ts = to_touch_core(dev);
	int size = 0;

	size += siw_snprintf(buf, size,
				"t_dbg_flag %08Xh\n\n",
				t_dbg_flag);

	return (ssize_t)size;
}

static ssize_t store_dbg_flag(struct device *dev,
				const char *buf, size_t count)
{
//	struct siw_ts *ts = to_touch_core(dev);
	u32 old_value, new_value = 0;

	if (sscanf(buf, "%X", &new_value) <= 0) {
		siw_sysfs_err_invalid_param(dev);
		if(count < INT_MAX)
	                return count;
	        else
	                return INT_MAX;
	}

	old_value = t_dbg_flag;
	t_dbg_flag = new_value;
	t_dev_info(dev, "t_dbg_flag changed : %08Xh -> %08xh\n",
		old_value, new_value);

	if(count < INT_MAX)
                return count;
        else
                return INT_MAX;
}

static ssize_t show_irq_flag(struct device *dev, char *buf)
{
	struct siw_ts *ts = to_touch_core(dev);
	int size = 0;

	size += siw_snprintf(buf, size,
				"irq flag : %08Xh\n\n",
				(u32)ts->irqflags_curr);

	return (ssize_t)size;
}


static ssize_t store_irq_flag(struct device *dev,
				const char *buf, size_t count)
{
	struct siw_ts *ts = to_touch_core(dev);
	u32 old_value, new_value = 0;

	if (sscanf(buf, "%X", &new_value) <= 0) {
		siw_sysfs_err_invalid_param(dev);
		if(count < INT_MAX)
	                return count;
	        else
	                return INT_MAX;
	}

	old_value = (u32)ts->irqflags_curr;

	mutex_lock(&ts->lock);

	irq_set_irq_type(ts->irq, new_value);
	ts->irqflags_curr &= ~IRQ_TYPE_SENSE_MASK;
	ts->irqflags_curr |= (new_value & IRQ_TYPE_SENSE_MASK);

	mutex_unlock(&ts->lock);

	t_dev_info(dev, "irq flag changed : %08Xh -> %08xh\n",
		old_value, (int)ts->irqflags_curr);

	if(count < INT_MAX)
                return count;
        else
                return INT_MAX;
}

#if defined(SIW_SUPPORT_PROBE_POST_RETRY)
static ssize_t store_init_late(struct device *dev,
				const char *buf, size_t count)
{
	t_dev_info(dev, "probe_post retry is working\n");

	if(count < INT_MAX)
                return count;
        else
                return INT_MAX;
}
#else	/* SIW_SUPPORT_PROBE_POST_RETRY */
static ssize_t store_init_late(struct device *dev,
				const char *buf, size_t count)
{
//	struct siw_ts *ts = to_touch_core(dev);
	int value = 0;

	if (sscanf(buf, "%X", &value) <= 0) {
		siw_sysfs_err_invalid_param(dev);
		if(count < INT_MAX)
	                return count;
	        else
	                return INT_MAX;
	}

	if (value != 0x55AA) {
		goto out;
	}

	siw_touch_atomic_notifier_call(
		LCD_EVENT_TOUCH_INIT_LATE,
		&value);

out:
	if(count < INT_MAX)
                return count;
        else
                return INT_MAX;
}
#endif	/* SIW_SUPPORT_PROBE_POST_RETRY */

extern int siw_touch_notify(struct siw_ts *ts, unsigned long event, void *data);

static ssize_t store_dbg_notify(struct device *dev,
				const char *buf, size_t count)
{
	struct siw_ts *ts = to_touch_core(dev);
	int magic = 0;
	int event = 0;
	int data = 0;

	if (sscanf(buf, "%X %X %X",
			&magic, &event, &data) <= 0) {
		siw_sysfs_err_invalid_param(dev);
		if(count < INT_MAX)
	                return count;
	        else
	                return INT_MAX;
	}

	if (magic != 0x5A) {	//magic code
		goto out;
	}

	siw_touch_notify(ts,
		(unsigned long)event, (void *)&data);

out:
	if(count < INT_MAX)
                return count;
        else
                return INT_MAX;
}

static ssize_t show_dbg_mon(struct device *dev, char *buf)
{
	struct siw_ts *ts = to_touch_core(dev);
	struct siw_ts_thread *ts_thread = &ts->mon_thread;
	const char *name = NULL;
	int size = 0;

	if (!(touch_flags(ts) & TOUCH_USE_MON_THREAD)) {
		size += siw_snprintf(buf, size, "mon thread not enabled\n");
		return (ssize_t)size;
	}

	name = (atomic_read(&ts_thread->state) == TS_THREAD_ON) ? "resume" : "pause";

	size += siw_snprintf(buf, size, "mon thread is %s state\n", name);

	size += siw_snprintf(buf, size, "\nUsage:\n");
	size += siw_snprintf(buf, size, " pause  : echo 1 > dbg_mon\n");
	size += siw_snprintf(buf, size, " resume : echo 0 > dbg_mon\n");

	return (ssize_t)size;
}

static ssize_t store_dbg_mon(struct device *dev,
				const char *buf, size_t count)
{
	struct siw_ts *ts = to_touch_core(dev);
	struct siw_touch_chip *chip = NULL;
	int pause = 0;

	if (!(touch_flags(ts) & TOUCH_USE_MON_THREAD)) {
		t_dev_info(dev, "mon thread not enabled\n");
		if(count < INT_MAX)
	                return count;
	        else
	                return INT_MAX;
	}

	if (sscanf(buf, "%d", &pause) <= 0) {
		siw_sysfs_err_invalid_param(dev);
		if(count < INT_MAX)
	                return count;
	        else
	                return INT_MAX;
	}

	chip = to_touch_chip(dev);

	if (chip->driving_mode != LCD_MODE_U3) {
		t_dev_info(dev, "can be controlled only in U3\n");
		if(count < INT_MAX)
	                return count;
	        else
	                return INT_MAX;
	}

	if (pause)
		siw_touch_mon_pause(dev);
	else
		siw_touch_mon_resume(dev);

	if(count < INT_MAX)
                return count;
        else
                return INT_MAX;
}

enum {
	SYSFS_DBG_TEST_NONE = 0,
	SYSFS_DBG_TEST_SYMLINK,
};

static ssize_t store_dbg_test(struct device *dev,
				const char *buf, size_t count)
{
	struct siw_ts *ts = to_touch_core(dev);
	char name[16+1] = {0, };
	int magic = 0;
	int code = 0;
	int value = 0;

	if (sscanf(buf, "%X %X %X %16s",
			&magic, &code, &value, name) <= 0) {
		siw_sysfs_err_invalid_param(dev);
		if(count < INT_MAX)
	                return count;
	        else
	                return INT_MAX;
	}

	if (magic != 0x5A) {	//magic code
		goto out;
	}

	switch (code) {
	case SYSFS_DBG_TEST_SYMLINK :
		siw_touch_sysfs_del_symlink(ts, name);
		if (value) {
			siw_touch_sysfs_gen_symlink(ts, name);
		}
		break;
	default :
		break;
	}

out:
	if(count < INT_MAX)
                return count;
        else
                return INT_MAX;
}

static ssize_t show_g_state(struct device *dev, char *buf,
					int value, const char *name)
{
	int size = 0;

	size += siw_snprintf(buf, size, "%s chk: %s(%08Xh)\n",
				name, (value & 0x1) ? "ON" : "OFF", value);

	return (ssize_t)size;
}

static ssize_t store_g_state(struct device *dev,
				const char *buf, size_t count,
				int *val, const char *name)
{
	int value = 0;

	if (sscanf(buf, "%X", &value) <= 0) {
		siw_sysfs_err_invalid_param(dev);
		return -EINVAL;
	}

	t_dev_dbg_base(dev, "%s set: %s(%08Xh)\n",
			name, (value & 0x1) ? "ON" : "OFF", value);

	if (val != NULL)
		*val = value;

	if(count < INT_MAX)
                return count;
        else
                return INT_MAX;
}

static ssize_t show_glove_state(struct device *dev, char *buf)
{
	struct siw_ts *ts = to_touch_core(dev);
	int value = atomic_read(&ts->state.glove);

	return show_g_state(dev, buf, value, "Glove");
}

static ssize_t store_glove_state(struct device *dev,
				const char *buf, size_t count)
{
	struct siw_ts *ts = to_touch_core(dev);
	int value = 0;
	int ret = 0;

	ret = store_g_state(dev, buf, count, &value, "Glove");
	if (ret >= 0) {
		mutex_lock(&ts->lock);
		atomic_set(&ts->state.glove, value);
		siw_ops_tc_con(ts, TCON_GLOVE, NULL);
		mutex_unlock(&ts->lock);
	}

	return (ssize_t)count;
}

static ssize_t show_grab_state(struct device *dev, char *buf)
{
	struct siw_ts *ts = to_touch_core(dev);
	int value = atomic_read(&ts->state.grab);

	return show_g_state(dev, buf, value, "Grab");
}

static ssize_t store_grab_state(struct device *dev,
				const char *buf, size_t count)
{
	struct siw_ts *ts = to_touch_core(dev);
	int value = 0;
	int ret = 0;

	ret = store_g_state(dev, buf, count, &value, "Grab");
	if (ret >= 0) {
		mutex_lock(&ts->lock);
		atomic_set(&ts->state.grab, value);
		siw_ops_tc_con(ts, TCON_GRAB, NULL);
		mutex_unlock(&ts->lock);
	}

	return (ssize_t)count;
}

ssize_t __weak show_sys_con(struct device *dev, char *buf)
{
	return 0;
}

ssize_t __weak store_sys_con(struct device *dev,
				const char *buf, size_t count)
{
	if(count < INT_MAX)
                return count;
        else
                return INT_MAX;
}


#define SIW_TOUCH_ATTR(_name, _show, _store)	\
		TOUCH_ATTR(_name, _show, _store)

#define SIW_TOUCH_ATTR_FW_UPDATE(_name, _show, _store) \
                TOUCH_ATTR_FW_UPDATE(_name, _show, _store)


#define SIW_TOUCH_ATTR_T(_name)	\
		touch_attr_##_name


static SIW_TOUCH_ATTR(platform_data,
						show_plat_data, NULL);
static SIW_TOUCH_ATTR(driver_data,
						show_driver_data, NULL);
static SIW_TOUCH_ATTR(fw_upgrade,
						show_upgrade,
						store_upgrade);
static SIW_TOUCH_ATTR_FW_UPDATE(fw_update, show_update_result, store_update);
#if defined(__SIW_CONFIG_KNOCK)
static SIW_TOUCH_ATTR(lpwg_data,
						_show_lpwg_data,
						_store_lpwg_data);
static SIW_TOUCH_ATTR(lpwg_notify, NULL,
						_store_lpwg_notify);
static SIW_TOUCH_ATTR(mfts,
						_show_mfts_state,
						_store_mfts_state);
static SIW_TOUCH_ATTR(mfts_lpwg,
						_show_mfts_lpwg,
						_store_mfts_lpwg);
#endif	/* __SIW_CONFIG_KNOCK */
#if defined(__SYS_USE_LOCKSCREEN)
static SIW_TOUCH_ATTR(keyguard,
						_show_lockscreen_state,
						_store_lockscreen_state);
#endif	/* __SYS_USE_LOCKSCREEN */
#if defined(__SYS_USE_IME_STATE)
static SIW_TOUCH_ATTR(ime_status,
						_show_ime_state,
						_store_ime_state);
#endif	/* __SYS_USE_LOCKSCREEN */
static SIW_TOUCH_ATTR(quick_cover_status,
						show_quick_cover_state,
						store_quick_cover_state);
static SIW_TOUCH_ATTR(firmware,
						show_version_info, NULL);
static SIW_TOUCH_ATTR(version,
						show_version_info, NULL);
static SIW_TOUCH_ATTR(testmode_ver,
						show_atcmd_version_info, NULL);
static SIW_TOUCH_ATTR(fw_version,
                                                show_atcmd_version_info, NULL);
static SIW_TOUCH_ATTR(sp_link_touch_off,
						show_sp_link_touch_off,
						store_sp_link_touch_off);
static SIW_TOUCH_ATTR(irq_state,
						show_irq_state,
						store_irq_state);
static SIW_TOUCH_ATTR(irq_level,
						show_irq_level, NULL);
static SIW_TOUCH_ATTR(debug_tool,
						show_debug_tool_state,
						store_debug_tool_state);
static SIW_TOUCH_ATTR(debug_tool_t,
						show_debug_tool_state,
						store_debug_tool_state);
static SIW_TOUCH_ATTR(debug_option,
						show_debug_option_state,
						store_debug_option_state);
#if defined(__SIW_SUPPORT_ASC)
static SIW_TOUCH_ATTR(incoming_call,
						_show_incoming_call_state,
						_store_incoming_call_state);
static SIW_TOUCH_ATTR(asc,
						_show_asc_param,
						_store_asc_param);
static SIW_TOUCH_ATTR(onhand,
						_show_onhand,
						_store_onhand);
#endif	/* __SIW_SUPPORT_ASC */
static SIW_TOUCH_ATTR(module_info,
						show_module_info, NULL);
static SIW_TOUCH_ATTR(dbg_mask,
						show_dbg_mask,
						store_dbg_mask);
static SIW_TOUCH_ATTR(dbg_flag,
						show_dbg_flag,
						store_dbg_flag);
static SIW_TOUCH_ATTR(irq_flag,
						show_irq_flag,
						store_irq_flag);
static SIW_TOUCH_ATTR(init_late, NULL,
						store_init_late);
static SIW_TOUCH_ATTR(dbg_notify, NULL,
						store_dbg_notify);
static SIW_TOUCH_ATTR(dbg_mon,
						show_dbg_mon,
						store_dbg_mon);
static SIW_TOUCH_ATTR(dbg_test, NULL,
						store_dbg_test);
static SIW_TOUCH_ATTR(glove_status,
						show_glove_state,
						store_glove_state);
static SIW_TOUCH_ATTR(grab_status,
						show_grab_state,
						store_grab_state);
static SIW_TOUCH_ATTR(sys_con,
						show_sys_con,
						store_sys_con);


static struct attribute *siw_touch_attribute_list[] = {
	&SIW_TOUCH_ATTR_T(platform_data).attr,
	&SIW_TOUCH_ATTR_T(driver_data).attr,
	&SIW_TOUCH_ATTR_T(module_info).attr,
	&SIW_TOUCH_ATTR_T(dbg_mask).attr,
	&SIW_TOUCH_ATTR_T(dbg_flag).attr,
	&SIW_TOUCH_ATTR_T(init_late).attr,
	&SIW_TOUCH_ATTR_T(dbg_notify).attr,
	NULL,
};

static const struct attribute_group siw_touch_attribute_group = {
	.attrs = siw_touch_attribute_list,
};

static struct attribute *siw_touch_attribute_list_normal[] = {
	&SIW_TOUCH_ATTR_T(fw_upgrade).attr,
	&SIW_TOUCH_ATTR_T(fw_update).attr,
#if defined(__SIW_CONFIG_KNOCK)
	&SIW_TOUCH_ATTR_T(lpwg_data).attr,
	&SIW_TOUCH_ATTR_T(lpwg_notify).attr,
	&SIW_TOUCH_ATTR_T(mfts).attr,
	&SIW_TOUCH_ATTR_T(mfts_lpwg).attr,
#endif	/* __SIW_CONFIG_KNOCK */
#if defined(__SYS_USE_LOCKSCREEN)
	&SIW_TOUCH_ATTR_T(keyguard.attr),
#endif	/* __SYS_USE_LOCKSCREEN */
#if defined(__SYS_USE_IME_STATE)
	&SIW_TOUCH_ATTR_T(ime_status).attr,
#endif	/* __SYS_USE_IME_STATE */
	&SIW_TOUCH_ATTR_T(quick_cover_status).attr,
	&SIW_TOUCH_ATTR_T(firmware).attr,
	&SIW_TOUCH_ATTR_T(version).attr,
	&SIW_TOUCH_ATTR_T(testmode_ver).attr,
	&SIW_TOUCH_ATTR_T(fw_version).attr,
	&SIW_TOUCH_ATTR_T(sp_link_touch_off).attr,
	&SIW_TOUCH_ATTR_T(irq_state).attr,
	&SIW_TOUCH_ATTR_T(irq_level).attr,
	&SIW_TOUCH_ATTR_T(irq_flag).attr,
	&SIW_TOUCH_ATTR_T(debug_tool).attr,
	&SIW_TOUCH_ATTR_T(debug_tool_t).attr,
	&SIW_TOUCH_ATTR_T(debug_option).attr,
#if defined(__SIW_SUPPORT_ASC)
	&SIW_TOUCH_ATTR_T(incoming_call).attr,
	&SIW_TOUCH_ATTR_T(asc).attr,
	&SIW_TOUCH_ATTR_T(onhand).attr,
#endif	/* __SIW_SUPPORT_ASC */
	&SIW_TOUCH_ATTR_T(dbg_mon).attr,
	&SIW_TOUCH_ATTR_T(dbg_test).attr,
	&SIW_TOUCH_ATTR_T(glove_status).attr,
	&SIW_TOUCH_ATTR_T(grab_status).attr,
	&SIW_TOUCH_ATTR_T(sys_con).attr,
	NULL,
};

static const struct attribute_group siw_touch_attribute_group_normal = {
	.attrs = siw_touch_attribute_list_normal,
};

static struct attribute *siw_touch_attribute_list_charger[] = {
#if defined(__SIW_CONFIG_KNOCK)
	&SIW_TOUCH_ATTR_T(mfts).attr,
	&SIW_TOUCH_ATTR_T(mfts_lpwg).attr,
#endif
	NULL,
};

static const struct attribute_group siw_touch_attribute_group_charger = {
	.attrs = siw_touch_attribute_list_charger,
};

static ssize_t siw_touch_attr_show(struct kobject *kobj,
		struct attribute *attr, char *buf)
{
	struct siw_ts *ts =
		container_of(kobj, struct siw_ts, kobj);
	struct siw_touch_attribute *priv =
		container_of(attr, struct siw_touch_attribute, attr);
	ssize_t ret = 0;

	if (priv->show)
		ret = priv->show(ts->dev, buf);

	return ret;
}

static ssize_t siw_touch_attr_store(struct kobject *kobj,
		struct attribute *attr, const char *buf, size_t count)
{
	struct siw_ts *ts =
		container_of(kobj, struct siw_ts, kobj);
	struct siw_touch_attribute *priv =
		container_of(attr, struct siw_touch_attribute, attr);
	ssize_t ret = count;

	if (priv->store)
		ret = priv->store(ts->dev, buf, count);

	return ret;
}

/*
 * To reegister SiW's unique sysfs functions
 */
static const struct sysfs_ops siw_touch_sysfs_ops = {
	.show	= siw_touch_attr_show,
	.store	= siw_touch_attr_store,
};

static struct kobj_type siw_touch_kobj_type = {
	.sysfs_ops = &siw_touch_sysfs_ops,
};


int siw_touch_init_sysfs(struct siw_ts *ts)
{
	struct device *dev = ts->dev;
	struct device *idev = &ts->input->dev;
	struct kobject *kobj = &ts->kobj;
	char *name = NULL;
	int ret = 0;

	name = (touch_flags(ts) & TOUCH_USE_DRV_NAME_SYSFS) ?
			touch_drv_name(ts) : touch_idrv_name(ts);
	if (!name) {
		name = SIW_TOUCH_INPUT;
	}

	ret = kobject_init_and_add(kobj, &siw_touch_kobj_type,
			idev->kobj.parent, "%s", name);
	if (ret < 0) {
		t_dev_err(dev, "failed to create sysfs entry\n");
		goto out;
	}

	ret = sysfs_create_group(kobj, &siw_touch_attribute_group);
	if (ret < 0) {
		t_dev_err(dev, "failed to add sysfs(initial)\n");
		goto out_sys;
	}
	
	ret = sysfs_create_link(NULL,kobj, "touch_drv");
	if (ret < 0) {
		t_dev_err(dev, "failed to add link_sysfs(initial)\n");
		goto out_sys;
	}

	return 0;

out_sys:
	kobject_del(kobj);

out:
	return ret;
}

void siw_touch_free_sysfs(struct siw_ts *ts)
{
	struct device *dev = ts->dev;
	struct device *idev = &ts->input->dev;

	if (ts->kobj.parent != idev->kobj.parent) {
		t_dev_warn(dev, "Invalid kobject\n");
		return;
	}

	sysfs_remove_group(&ts->kobj, &siw_touch_attribute_group);
	sysfs_remove_link(NULL, "touch_drv");

	kobject_del(&ts->kobj);
}

int __weak siw_touch_misc_init(struct device *dev)
{
	return 0;
}

void __weak siw_touch_misc_free(struct device *dev)
{

}

static int siw_touch_add_sysfs_normal(struct siw_ts *ts)
{
	struct device *dev = ts->dev;
	struct kobject *kobj = &ts->kobj;
	int ret;

	ret = sysfs_create_group(kobj, &siw_touch_attribute_group_normal);
	if (ret < 0) {
		t_dev_err(dev, "failed to add sysfs(normal)\n");
		goto out;
	}

	ret = siw_ops_sysfs(ts, DRIVER_INIT);
	if (ret < 0) {
		t_dev_err(dev, "failed to add sysfs(ops)\n");
		goto out_ops_sysfs;
	}

	return 0;

out_ops_sysfs:
	sysfs_remove_group(kobj, &siw_touch_attribute_group_normal);

out:
	return ret;
}

static void siw_touch_del_sysfs_normal(struct siw_ts *ts)
{
//	struct device *dev = ts->dev;
	struct kobject *kobj = &ts->kobj;

	siw_ops_sysfs(ts, DRIVER_FREE);

	sysfs_remove_group(kobj, &siw_touch_attribute_group_normal);
}

static int siw_touch_add_sysfs_charger(struct siw_ts *ts)
{
	struct device *dev = ts->dev;
	struct kobject *kobj = &ts->kobj;
	int ret;

	ret = sysfs_create_group(kobj, &siw_touch_attribute_group_charger);
	if (ret < 0) {
		t_dev_err(dev, "failed to add sysfs(charger)\n");
		goto out;
	}

out:
	return ret;
}

static void siw_touch_del_sysfs_charger(struct siw_ts *ts)
{
//	struct device *dev = ts->dev;
	struct kobject *kobj = &ts->kobj;

	sysfs_remove_group(kobj, &siw_touch_attribute_group_charger);
}

static int siw_touch_do_add_sysfs(struct siw_ts *ts)
{
	if (ts->is_charger) {
		return siw_touch_add_sysfs_charger(ts);
	}

	return siw_touch_add_sysfs_normal(ts);
}

static void siw_touch_do_del_sysfs(struct siw_ts *ts)
{
	if (ts->is_charger) {
		siw_touch_del_sysfs_charger(ts);
		return;
	}

	siw_touch_del_sysfs_normal(ts);
}

int siw_touch_add_sysfs(struct siw_ts *ts)
{
	struct device *dev = ts->dev;
	struct device *idev = &ts->input->dev;
	struct kobject *kobj = &ts->kobj;
	int ret = 0;

	if (kobj->parent != idev->kobj.parent) {
		t_dev_err(dev, "Invalid kobject\n");
		return -EINVAL;
	}

	ret = siw_touch_do_add_sysfs(ts);
	if (ret < 0) {
		return ret;
	}

	siw_touch_misc_init(dev);

	return 0;
}

void siw_touch_del_sysfs(struct siw_ts *ts)
{
	struct device *dev = ts->dev;
	struct device *idev = &ts->input->dev;
	struct kobject *kobj = &ts->kobj;

	if (kobj->parent != idev->kobj.parent) {
		t_dev_warn(dev, "Invalid kobject\n");
		return;
	}

	siw_touch_misc_free(dev);

	siw_touch_do_del_sysfs(ts);
}


