/*
 * siw_touch_sys.c - SiW touch system interface
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

#if defined(CONFIG_PLAT_SAMSUNG)
#include <plat/cpu.h>
#include <plat/gpio-core.h>
#include <plat/gpio-cfg.h>
#include <plat/gpio-cfg-helpers.h>
#include <plat/pm.h>
#endif

#include "siw_touch.h"
#include "siw_touch_gpio.h"
#include "siw_touch_sys.h"

#if defined(CONFIG_TOUCHSCREEN_SIW_LG4895_F650) ||	\
	defined(CONFIG_TOUCHSCREEN_SIW_LG4946_F700)
#include <soc/qcom/lge/board_lge.h>
#endif


/*
 * depends on AP bus
 */
int siw_touch_sys_bus_use_dma(struct device *dev)
{
	struct siw_ts *ts = to_touch_core(dev);

	if (touch_bus_type(ts) != BUS_IF_SPI) {
		return 0;
	}

#if defined(__If_the_bus_of_your_chipset_needs_dma_control__)
	{
		return 1;
	}
#elif defined(CONFIG_ARCH_EXYNOS5)
	{
		return 0;
	}
#endif

	return 0;
}

int siw_touch_get_boot_mode(void)
{
#if defined(CONFIG_SIW_GET_BOOT_MODE)
	if (sys_get_boot_mode() == BOOT_MODE_CHARGERLOGO) {
		return SIW_TOUCH_CHARGER_MODE;
	}
#elif defined(CONFIG_TOUCHSCREEN_SIW_LG4895_F650) ||	\
	defined(CONFIG_TOUCHSCREEN_SIW_LG4946_F700)
	if (lge_get_boot_mode() == LGE_BOOT_MODE_CHARGERLOGO) {
		return SIW_TOUCH_CHARGER_MODE;
	}
#endif

	return 0;
}

int siw_touch_boot_mode_check(struct device *dev)
{
	struct siw_ts *ts = to_touch_core(dev);
	int ret = NORMAL_BOOT;

#if defined(CONFIG_SIW_GET_FACTORY_MODE)
	ret = sys_get_factory_boot();
#elif defined(CONFIG_TOUCHSCREEN_SIW_LG4895_F650) ||	\
	defined(CONFIG_TOUCHSCREEN_SIW_LG4946_F700)
	ret = lge_get_factory_boot();
#endif

	if (ret != NORMAL_BOOT) {
		switch (atomic_read(&ts->state.mfts)) {
			case MFTS_NONE :
				ret = MINIOS_AAT;
				break;
			case MFTS_FOLDER :
				ret = MINIOS_MFTS_FOLDER;
				break;
			case MFTS_FLAT :
				ret = MINIOS_MFTS_FLAT;
				break;
			case MFTS_CURVED :
				ret = MINIOS_MFTS_CURVED;
				break;
			default :
				ret = MINIOS_AAT;
				break;
		}
	}

	return ret;
}

int siw_touch_boot_mode_tc_check(struct device *dev)
{
	int ret = 0;

#if defined(CONFIG_SIW_GET_BOOT_MODE)
	if (sys_get_boot_mode() == BOOT_MODE_CHARGERLOGO) {
		return 1;
	}
#elif defined(CONFIG_TOUCHSCREEN_SIW_LG4895_F650)
	if ((lge_get_boot_mode() == LGE_BOOT_MODE_QEM_910K) ||
		(lge_get_boot_mode() == LGE_BOOT_MODE_PIF_910K)) {
		return 1;
	}
#endif

	return ret;
}

int siw_touch_sys_gpio_set_pull(int pin, int value)
{
	int ret = 0;

#if defined(CONFIG_PLAT_SAMSUNG)
	{
		int pull_val;

		switch (value) {
		case GPIO_PULL_UP:
			pull_val = S3C_GPIO_PULL_UP;
			break;
		case GPIO_PULL_DOWN:
			pull_val = S3C_GPIO_PULL_DOWN;
			break;
		default:
			pull_val = S3C_GPIO_PULL_NONE;
			break;
		}
		ret = s3c_gpio_setpull(pin, pull_val);
	}
#endif

	return ret;
}

int siw_touch_sys_panel_reset(struct device *dev)
{
	return 0;
}

int siw_touch_sys_get_panel_bl(struct device *dev)
{
	return 0;
}

int siw_touch_sys_set_panel_bl(struct device *dev, int level)
{
	if (level == 0){
		t_dev_info(dev, "BLU control OFF\n");
		/* To Do : screen off */
	} else {
		t_dev_info(dev, "BLU control ON (level:%d)\n", level);
		/* To Do : screen on with level */
	}
	return 0;
}

int siw_touch_sys_osc(struct device *dev, int onoff)
{
	return 0;
}

//#if defined(CONFIG_TOUCHSCREEN_SIW_SW1828) || defined(CONFIG_TOUCHSCREEN_SIW_SW1828_MODULE)
/*
 * Sensitivity level control
 *
 * [read]
 * # cat sys_con
 * 1 ~ 3	: Valid level
 * 0		: Not supported
 *
 * [write]
 * # echo 0x1 > sys_con		//set level 1
 * # echo 0x81 > sys_con	//set level 1 then save
 *
 */
#include "siw_touch_hal.h"

#define LEVEL_MIN		1
#define LEVEL_MAX		3
#define LEVEL_ERR		0

#define LEVEL_ADDR		0x270
#define LEVEL_WR_CMD	((0xC657<<16) | (1<<2))
#define LEVEL_WR_STS	0xD8
#define LEVEL_WR_DLY	2
#define LEVEL_WR_CNT	50

static int check_sys_con(struct device *dev, int *level)
{
	int var_level;
	int ret = 0;

	ret = siw_hal_read_value(dev, LEVEL_ADDR, &var_level);
	if (ret < 0) {
		t_dev_err(dev, "failed to get level\n");
		goto out;
	}

	if ((var_level < LEVEL_MIN) || (var_level > LEVEL_MAX)) {
		t_dev_err(dev,
			"The current FW doesn't support this, %d\n", var_level);
		ret = -ESRCH;
		goto out;
	}

	if (level != NULL)
		*level = var_level;

out:
	return ret;
}

ssize_t show_do_sys_con(struct device *dev, char *buf)
{
	int level = 0;
	int ret = 0;

	ret = check_sys_con(dev, &level);
	if (ret < 0) {
		level = LEVEL_ERR;
		goto out;
	}

	t_dev_info(dev, "current level is %d\n", level);

out:
	return siw_snprintf(buf, 0, "%d\n", level);
}

ssize_t show_sys_con(struct device *dev, char *buf)
{
	struct siw_ts *ts = to_touch_core(dev);
	int ret = 0;

	mutex_lock(&ts->lock);

	ret = show_do_sys_con(dev, buf);

	mutex_unlock(&ts->lock);

	return ret;
}

ssize_t store_do_sys_con(struct device *dev,
				const char *buf, size_t count)
{
	int level = 0;
	int save = 0;
	int ret = 0;

	if (sscanf(buf, "%X", &level) <= 0) {
		t_dev_err(dev, "Invalid param\n");
		if(count < INT_MAX)
	                return count;
	        else
	                return INT_MAX;
	}

	ret = check_sys_con(dev, NULL);
	if (ret < 0) {
		if(count < INT_MAX)
	                return count;
	        else
	                return INT_MAX;
	}

	save = level & 0x80;

	level &= 0x7F;
	if ((level < LEVEL_MIN) || (level > LEVEL_MAX)) {
		t_dev_err(dev, "invalid level, %d\n", level);
		if(count < INT_MAX)
	                return count;
	        else
	                return INT_MAX;
	}

	ret = siw_hal_write_value(dev, LEVEL_ADDR, level);
	if (ret < 0) {
		t_dev_err(dev,
			"failed to set level(%d), %d\n",
			level, ret);
		if(count < INT_MAX)
	                return count;
	        else
	                return INT_MAX;
	}
	t_dev_info(dev, "new level is %d\n", level);

	if (save) {
		struct siw_touch_chip *chip = to_touch_chip(dev);
		struct siw_hal_reg *reg = chip->reg;
		u32 wr_cmd, chk_resp = 0;
		int wr_failed = 1;
		int retry = LEVEL_WR_CNT;

		t_dev_info(dev, "saving level(%d) begins\n", level);

		wr_cmd = LEVEL_WR_CMD;
		ret = siw_hal_write_value(dev, reg->tc_flash_dn_ctl, wr_cmd);
		if (ret < 0) {
			t_dev_err(dev, "failed to set wr_cmd(%Xh), %d\n",
				wr_cmd, ret);
			if(count < INT_MAX)
		                return count;
		        else
	        	        return INT_MAX;
		}

		do {
			touch_msleep(LEVEL_WR_DLY);

			ret = siw_hal_read_value(dev, reg->tc_flash_dn_status, &chk_resp);
			if (ret >= 0) {
				if (chk_resp == LEVEL_WR_STS) {
					t_dev_info(dev, "saving level(%d) done\n", level);
					wr_failed = 0;
					break;
				}
			}
		} while (--retry);

		if (wr_failed) {
			t_dev_err(dev,
				"save fail: addr[%04Xh] data[%08Xh], expect[%08Xh]\n",
				reg->tc_flash_dn_status, chk_resp, LEVEL_WR_STS);
		}
	}

	if(count < INT_MAX)
                return count;
        else
                return INT_MAX;
}

ssize_t store_sys_con(struct device *dev,
				const char *buf, size_t count)
{
	struct siw_ts *ts = to_touch_core(dev);
	int ret = 0;

	mutex_lock(&ts->lock);

	ret = store_do_sys_con(dev, buf, count);

	mutex_unlock(&ts->lock);

	return ret;
}
//#endif
