/*
 * touch_sw17700.c - SiW touch driver glue for SW17700
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

#include "LA103WF5_0_09.h"
#include "LA103WF5_0_07.h"
#include "LA123WF7_SL03_0_07.h"

//#define CHIP_SW17700_SPI

#define CHIP_ID						"1770"
#define CHIP_DEVICE_NAME			"SW17700"
#define CHIP_COMPATIBLE_NAME		"siw,sw17700"
#define CHIP_DEVICE_DESC			"SiW Touch SW17700 Driver"

#define CHIP_TYPE					CHIP_SW17700

#define CHIP_MODE_ALLOWED			(0 |	\
									LCD_MODE_BIT_U3 |	\
									LCD_MODE_BIT_STOP |	\
									0)

#define CHIP_FW_SIZE				(64<<10)

#define CHIP_FLAGS					(0 |	\
									TOUCH_SKIP_ESD_EVENT |	\
									TOUCH_USE_FW_BINARY | \
									0)

#define CHIP_IRQFLAGS				(IRQF_TRIGGER_FALLING | IRQF_ONESHOT)

#if defined(CHIP_SW17700_SPI)
#define CHIP_INPUT_ID_BUSTYPE		BUS_SPI
#else
#define CHIP_INPUT_ID_BUSTYPE		BUS_I2C
#endif
#define CHIP_INPUT_ID_VENDOR		0xABCD
#define CHIP_INPUT_ID_PRODUCT		0x9876
#define CHIP_INPUT_ID_VERSION		0x1234

#if defined(CONFIG_ARCH_EXYNOS5)
#define __CHIP_QUIRK_ADD			CHIP_QUIRK_NOT_SUPPORT_XFER
#else
#define __CHIP_QUIRK_ADD			0
#endif

#define CHIP_QUIRKS					(0 |	\
									CHIP_QUIRK_NOT_SUPPORT_ASC |	\
									CHIP_QUIRK_NOT_SUPPORT_LPWG |	\
									CHIP_QUIRK_NOT_SUPPORT_WATCH |	\
									CHIP_QUIRK_NOT_SUPPORT_IME |	\
									__CHIP_QUIRK_ADD |	\
									0)

#if defined(CHIP_SW17700_SPI)
#define CHIP_BUS_TYPE				BUS_IF_SPI
#define	CHIP_BUF_SIZE				0
#define CHIP_SPI_MODE				SPI_MODE_0
#define CHIP_BPW					8
#define CHIP_MAX_FREQ				(1 * 1000* 1000)
#define CHIP_TX_HDR_SZ				SPI_BUS_TX_HDR_SZ
#define CHIP_TX_DUMMY_SZ			SPI_BUS_TX_DUMMY_SZ

#if (CHIP_MAX_FREQ >= (30 * 1000* 1000))
#define CHIP_RX_DUMMY_128BIT
#endif

#if defined(CHIP_RX_DUMMY_128BIT)
#define CHIP_RX_HDR_SZ				SPI_BUS_RX_HDR_SZ_128BIT
#define CHIP_RX_DUMMY_SZ			SPI_BUS_RX_DUMMY_SZ_128BIT
#else
#define CHIP_RX_HDR_SZ				SPI_BUS_RX_HDR_SZ_32BIT
#define CHIP_RX_DUMMY_SZ			SPI_BUS_RX_DUMMY_SZ_32BIT
#endif
#else	/* CHIP_SW42103_SPI */
#define CHIP_BUS_TYPE				BUS_IF_I2C
#define	CHIP_BUF_SIZE				0
#define CHIP_SPI_MODE				-1
#define CHIP_BPW					-1
#define CHIP_MAX_FREQ				-1
#define CHIP_TX_HDR_SZ				I2C_BUS_TX_HDR_SZ
#define CHIP_RX_HDR_SZ				I2C_BUS_RX_HDR_SZ
#define CHIP_TX_DUMMY_SZ			I2C_BUS_TX_DUMMY_SZ
#define CHIP_RX_DUMMY_SZ			I2C_BUS_RX_DUMMY_SZ
#endif	/* CHIP_SW42103_SPI */


//data configuration
struct siw_touch_fw_bin chip_fw_bin = {
	.fw_data = LA103WF5_0_07_img,
	.fw_size = sizeof(LA103WF5_0_07_img),
};

struct siw_touch_fw_bin chip_fw_bin_dis = {
        .fw_data = LA103WF5_0_09_img,
        .fw_size = sizeof(LA103WF5_0_09_img),
};

struct siw_touch_fw_bin chip_fw_bin_dis_12_3 = {
        .fw_data = LA123WF7_SL03_0_07_img,
        .fw_size = sizeof(LA123WF7_SL03_0_07_img),
};

struct siw_touch_fw_bin chip_fw_bin_dis_curved_12_3 = {
        .fw_data = NULL,
        .fw_size = sizeof(NULL),
};

static const struct siw_hal_reg_quirk chip_reg_quirks[] = {
	{ .old_addr = SPR_CHIP_TEST, .new_addr = 0x0BF, },
	{ .old_addr = SPR_CHIP_ID, .new_addr = 0x100, },
	{ .old_addr = SPR_RST_CTL, .new_addr = 0x082, },
	{ .old_addr = SPR_BOOT_CTL, .new_addr = 0x002, },
	{ .old_addr = SPR_SRAM_CTL, .new_addr = 0x003, },
	{ .old_addr = SPR_BOOT_STS, .new_addr = 0x107, },
	{ .old_addr = SPR_SUBDISP_STS, .new_addr = 0x10C, },
	{ .old_addr = INFO_CHIP_VERSION, .new_addr = 0x101, },
	{ .old_addr = TC_CONFDN_BASE_ADDR, .new_addr = 0x24C },
	/* */
	{ .old_addr = CODE_ACCESS_ADDR, .new_addr = 0xFD0, },
	{ .old_addr = DATA_I2CBASE_ADDR, .new_addr = 0xFD1, },
	{ .old_addr = PRD_TCM_BASE_ADDR, .new_addr = 0xFD3, },
	/* */
	{ .old_addr = (1<<31), .new_addr = 0, },	/* switch : don't show log */
	/* */
#if defined(CHIP_SW17700_SPI)
	{ .old_addr = SPR_CODE_OFFSET, .new_addr = 0x024, },
	{ .old_addr = SERIAL_DATA_OFFSET, .new_addr = 0x028, },
#else	/* CHIP_SW17700_SPI */
	{ .old_addr = SPR_CODE_OFFSET, .new_addr = 0x019, },
	{ .old_addr = SERIAL_DATA_OFFSET, .new_addr = 0x01D, },
#endif	/* CHIP_SW17700_SPI */
	{ .old_addr = TC_FLASH_DN_STS, .new_addr = 0x266, },
	/* */
	{ .old_addr = IME_STATE, .new_addr = ADDR_SKIP_MASK, },
	/* */
	{ .old_addr = ~0, .new_addr = ~0 },		// End signal
};

#if defined(SIW_CONFIG_OF)
/*
 * of_device_is_compatible(dev->of_node, CHIP_COMPATIBLE_NAME)
 */
static const struct of_device_id chip_match_ids[] = {
	{ .compatible = CHIP_COMPATIBLE_NAME },
	{ },
};
#else
/* Resolution
 *     [10.25] [12.3]
 * X :	1920    1920
 * Y :	 720     720
 */
enum CHIP_CAPABILITY {
	CHIP_MAX_X			= 1920,
	CHIP_MAX_Y			= 720,
	CHIP_MAX_PRESSURE	= 255,
	CHIP_MAX_WIDTH		= 15,
	CHIP_MAX_ORI		= 1,
	CHIP_MAX_ID			= 10,
	/* */
	CHIP_HW_RST_DELAY	= 210,
	CHIP_SW_RST_DELAY	= 90,
};

#define CHIP_PIN_RESET			0
#define CHIP_PIN_IRQ			0
#define CHIP_PIN_MAKER			-1
#define CHIP_PIN_VDD			-1
#define CHIP_PIN_VIO			-1

#if (CHIP_PIN_RESET == 0) || (CHIP_PIN_IRQ == 0)
//	#error Assign external pin & flag first!!!
#endif
#endif	/* SIW_CONFIG_OF */

/* use eg. cname=arc1 to change name */
static char chip_name[32] = CHIP_DEVICE_NAME;
module_param_string(cname, chip_name, sizeof(chip_name), 0);

/* use eg. dname=arc1 to change name */
static char chip_drv_name[32] = SIW_TOUCH_NAME;
module_param_string(dname, chip_drv_name, sizeof(chip_drv_name), 0);

/* use eg. iname=arc1 to change input name */
static char chip_idrv_name[32] = SIW_TOUCH_INPUT;
module_param_string(iname, chip_idrv_name, sizeof(chip_idrv_name), 0);

static const struct siw_touch_pdata chip_pdata = {
	/* Configuration */
	.chip_id			= CHIP_ID,
	.chip_name			= chip_name,
	.drv_name			= chip_drv_name,
	.idrv_name			= chip_idrv_name,
	.owner				= THIS_MODULE,
	.chip_type			= CHIP_TYPE,
	.mode_allowed		= CHIP_MODE_ALLOWED,
	.fw_size			= CHIP_FW_SIZE,
	.flags				= CHIP_FLAGS,	/* Caution : MSB(bit31) unavailable */
	.irqflags			= CHIP_IRQFLAGS,
	.quirks				= CHIP_QUIRKS,
	/* */
	.bus_info			= {
		.bus_type			= CHIP_BUS_TYPE,
		.buf_size			= CHIP_BUF_SIZE,
		.spi_mode			= CHIP_SPI_MODE,
		.bits_per_word		= CHIP_BPW,
		.max_freq			= CHIP_MAX_FREQ,
		.bus_tx_hdr_size	= CHIP_TX_HDR_SZ,
		.bus_rx_hdr_size	= CHIP_RX_HDR_SZ,
		.bus_tx_dummy_size	= CHIP_TX_DUMMY_SZ,
		.bus_rx_dummy_size	= CHIP_RX_DUMMY_SZ,
	},
#if defined(SIW_CONFIG_OF)
	.of_match_table 	= of_match_ptr(chip_match_ids),
#else
	.pins				= {
		.reset_pin		= CHIP_PIN_RESET,
		.reset_pin_pol	= OF_GPIO_ACTIVE_LOW,
		.irq_pin		= CHIP_PIN_IRQ,
		.maker_id_pin	= CHIP_PIN_MAKER,
		.vdd_pin		= CHIP_PIN_VDD,
		.vio_pin		= CHIP_PIN_VIO,
	},
	.caps				= {
		.max_x			= CHIP_MAX_X,
		.max_y			= CHIP_MAX_Y,
		.max_pressure	= CHIP_MAX_PRESSURE,
		.max_width		= CHIP_MAX_WIDTH,
		.max_orientation = CHIP_MAX_ORI,
		.max_id			= CHIP_MAX_ID,
		.hw_reset_delay	= CHIP_HW_RST_DELAY,
		.sw_reset_delay	= CHIP_SW_RST_DELAY,
	},
#endif
	/* Input Device ID */
	.i_id				= {
		.bustype		= CHIP_INPUT_ID_BUSTYPE,
		.vendor 		= CHIP_INPUT_ID_VENDOR,
		.product 		= CHIP_INPUT_ID_PRODUCT,
		.version 		= CHIP_INPUT_ID_VERSION,
	},
	/* */
	//See 'siw_hal_get_default_ops' [siw_touch_hal.c]
	.ops				= NULL,
	/* */
	//See 'siw_hal_get_tci_info' [siw_touch_hal.c]
	.tci_info			= NULL,
	.tci_qcover_open	= NULL,
	.tci_qcover_close	= NULL,
	//See 'siw_hal_get_swipe_info' [siw_touch_hal.c]
	.swipe_ctrl			= NULL,
	//See 'store_ext_watch_config_font_position' [siw_touch_hal_watch.c]
	.watch_win			= NULL,
	//See 'siw_setup_operations' [siw_touch.c]
	.reg_quirks			= (void *)chip_reg_quirks,
	.fw_bin             = (void *)&chip_fw_bin,
	.fw_bin_dis         = (void *)&chip_fw_bin_dis,
	.fw_bin_dis_12_3    = (void *)&chip_fw_bin_dis_12_3,
	.fw_bin_dis_curved_12_3 = (void *)&chip_fw_bin_dis_curved_12_3,
};

static struct siw_touch_chip_data chip_data = {
        .pdata = &chip_pdata,
        .bus_drv = NULL,
};

#if defined(CONFIG_DAUDIO_KK) && !defined(CONFIG_TOUCHSCREEN_ONEBIN)
int factory_connect_for_lcd(void)
{
        if((!gpio_get_value(TCC_GPB(8)))&&daudio_lcd_version()==DAUDIOKK_LCD_OD_10_25_1920_720_INCELL_LTPS_LG)
                return 1;
        else
                return 0;

        return 0;
}
static int __init sw17700_init(void)
{
        int err;

        int LCD_VER = daudio_lcd_version();

        printk("%s, GPIO_B24 = %d, GPIO_B13 = %d, GPIO_B8 = %d ,  daudio_lcd_version() = %d \n", __func__, gpio_get_value(TCC_GPB(24)), gpio_get_value(TCC_GPB(13)), gpio_get_value(TCC_GPB(8)) ,daudio_lcd_version());

        if(gpio_get_value(TCC_GPB(24))) // OE
                switch(LCD_VER) {
                        case DAUDIOKK_LCD_OD_10_25_1920_720_INCELL_LTPS_LG:
                                if(factory_connect_for_lcd())
                                {
                                        return -ENODEV;
                                        break;
                                }
                        case DAUDIOKK_LCD_OI_10_25_1920_720_INCELL_LTPS_LG:
                                err = siw_touch_bus_add_driver(&chip_data);
                                if (err)
                                        pr_err("Adding sw17700 driver failed2 (errno = %d)\n", err);

                                return err;
                                break;

                        default:
                                return -ENODEV;
                                break;
                }

}
module_init(sw17700_init);

static void __exit sw17700_cleanup(void)
{
        (void)siw_touch_bus_del_driver(&chip_data);
}
module_exit(sw17700_cleanup);
#elif defined(CONFIG_WIDE_PE_COMMON) || defined(CONFIG_TOUCHSCREEN_ONEBIN)
int sw17700_init(void)
{
	int err;

	printk("%s, GPIO_B16 = %d, GPIO_C14 = %d, GPIO_B13 = %d,  daudio_lcd_version() = %d \n", __func__, get_oemtype(), get_montype(), get_factorycheck(), daudio_lcd_version());

	err = siw_touch_bus_add_driver(&chip_data);
	if (err)
		pr_err("Adding sw17700 driver failed2 (errno = %d)\n", err);
	return err;
}
EXPORT_SYMBOL(sw17700_init);

void sw17700_cleanup(void)
{
	(void)siw_touch_bus_del_driver(&chip_data);
}
EXPORT_SYMBOL(sw17700_cleanup);
#endif

/*
siw_chip_module_init(CHIP_DEVICE_NAME,
				chip_data,
				CHIP_DEVICE_DESC,
				"kimhh@siliconworks.co.kr");
*/

MODULE_LICENSE("GPL");

def_siw_setup_str("siw_chip_name=", siw_setup_chip_name, chip_name);
def_siw_setup_str("siw_drv_name=", siw_setup_drv_name, chip_drv_name);
def_siw_setup_str("siw_idrv_name=", siw_setup_idrv_name, chip_idrv_name);



