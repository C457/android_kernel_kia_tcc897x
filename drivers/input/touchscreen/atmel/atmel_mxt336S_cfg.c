/*
*  atmel_maxtouch.c - Atmel maXTouch Touchscreen Controller
*
*  Version 0.2a
*
*  An early alpha version of the maXTouch Linux driver.
*
*
*  Copyright (C) 2010 Iiro Valkonen <iiro.valkonen@atmel.com>
*  Copyright (C) 2009 Ulf Samuelsson <ulf.samuelsson@atmel.com>
*  Copyright (C) 2009 Raphael Derosso Pereira <raphaelpereira@gmail.com>
*
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program; if not, write to the Free Software
*  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/string.h>
#include <linux/proc_fs.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/input.h>
#include <linux/module.h>
#include <linux/device.h>
//#include <linux/earlysuspend.h>
#include <linux/firmware.h>
#include <linux/wakelock.h>

#include <linux/delay.h>
#include <linux/i2c.h>

#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/syscalls.h>
#include <linux/slab.h>

#include "atmel_mxt336S.h"
#include "atmel_mxt336S_cfg.h"
#include "atmel_mxt336S_parse.h"
#include "device_config_mxt336S.h"
#include "atmel_mxt1189T_cfg.h"
#include "atmel_mxt641T_cfg.h"
#include "atmel_mxt641TD_cfg.h"

u8 firmware_latest[] = {
	/* only for test */
	/* #include "mxt_firmware.h" */
};

/* mxt ICs has two slave address,
 * one is the application mode, the other is the boot mode
 * */
static struct mxt_i2c_addr {
	u8 app;		/* application */
	u8 boot;	/* Bootloader */
} mxt_i2c_addr_list[4] = {
	{0x4A, 0x24},
	{0x4B, 0x25},
	{0x5A, 0x34},
	{0x5B, 0x35},
};

/******************************************************************************
*
*
*       Object table init
*
*
* *****************************************************************************/
/** General Object **/
/* Power config settings. */
static gen_powerconfig_t7_config_t power_config = {0, };
/* Acquisition config. */
gen_acquisitionconfig_t8_config_t acquisition_config_mxt336s = {0, };
EXPORT_SYMBOL(acquisition_config_mxt336s);

/** Touch Object **/
/* Multitouch screen config. */
static touch_multitouchscreen_t9_config_t touchscreen_config = {0, };
/* static touch_multitouchscreen_t9_config_t touchscreen_config1 = {0, }; */
/* Key array config. */
/* static touch_keyarray_t15_config_t keyarray_config = {0, }; */

/** Signal Processing Objects **/
/* Proximity Config */
/* static touch_proximity_t52_config_t proximity_config = {0, }; */
/* One-touch gesture config. */
/*static
proci_onetouchgestureprocessor_t24_config_t onetouch_gesture_config = {0, }; */
/*
static
proci_twotouchgestureprocessor_t27_config_t twotouch_gesture_config = {0, };*/

/** Support Objects **/
/* GPIO/PWM config */
/* static spt_gpiopwm_t19_config_t	gpiopwm_config = {0, }; */

/* for registers in runtime mode */
struct mxt_runmode_registers_t runmode_regs = {0, };

/* MXT540E Objects */
/* static proci_gripsuppression_t40_config_t gripsuppression_t40_config = {0, }; */
static proci_touchsuppression_t42_config_t \
touchsuppression_t42_config = {0, };
/* firmware2.1 */
static proci_tsuppression_t42_ver2_config_t \
tsuppression_t42_ver2_config = {0, };
static spt_cteconfig_t46_config_t cte_t46_config = {0, };
/* firmware2.1 */
static spt_cteconfig_t46_ver2_config_t cte_t46_ver2_config = {0, };

/* static proci_stylus_t47_config_t stylus_t47_config = {0, }; */
static procg_noisesuppression_t48_config_t \
noisesuppression_t48_config = {0, };

/* firmware2.1 T48 */
static procg_nsuppression_t48_ver2_config_t \
nsuppression_t48_ver2_config = {0, };

static spt_userdata_t38_t	userdata_t38 = {{0}, };

/* add firmware 2.1 */
static proci_shieldless_t56_config_t shieldless_t56_config = {0, };

// sjpark@cleinsoft
/* static proci_stylus_t47_config_t stylus_t47_config = {0, }; */
static procg_noisesuppression_t62_config_t \
noisesuppression_t62_config = {0, };

#if defined (MXT_CONFIG_RAW_FILE) || defined(MXT_CONFIG_XCFG_FILE)
int mxt_set_config_file_641t(int object_id,int offset,int width,int data, int instance)
{
	if(offset < 0)
		return -1;

        switch (object_id)
        {
                case 7:
                cfg_641T_T7[offset] = data & 0xFF;
                if(width == 2)
                {
                        cfg_641T_T7[offset+1] =  ((data >> 8) & 0xFF);
                }
                else if(width == 4)
                {
                        cfg_641T_T7[offset + 1] = ((data >> 8) & 0xFF);
                        cfg_641T_T7[offset + 2] = ((data >> 16) & 0xFF);
                        cfg_641T_T7[offset + 3] = ((data >> 24) & 0xFF);
                }
                //printk("[%s:%d::::::::object_id=T%d:offset=%d:width=%d:data=%d] \r\n",__func__,__LINE__,object_id,offset,width,data);
                break;

                case 8:
                cfg_641T_T8[offset] = data & 0xFF;
                if(width == 2)
                {
                        cfg_641T_T8[offset+1] =  ((data >> 8) & 0xFF);
                }
                else if(width == 4)
                {
                        cfg_641T_T8[offset + 1] = ((data >> 8) & 0xFF);
                        cfg_641T_T8[offset + 2] = ((data >> 16) & 0xFF);
                        cfg_641T_T8[offset + 3] = ((data >> 24) & 0xFF);
                }
                //printk("[%s:%d::::::::object_id=T%d:offset=%d:width=%d:data=%d] \r\n",__func__,__LINE__,object_id,offset,width,data);
                break;

                case 38:
                cfg_641T_T38[offset] = data & 0xFF;
                if(width == 2)
                {
                        cfg_641T_T38[offset+1] =  ((data >> 8) & 0xFF);
                }
                else if(width == 4)
                {
                        cfg_641T_T38[offset + 1] = ((data >> 8) & 0xFF);
                        cfg_641T_T38[offset + 2] = ((data >> 16) & 0xFF);
                        cfg_641T_T38[offset + 3] = ((data >> 24) & 0xFF);
                }
                //printk("[%s:%d::::::::object_id=T%d:offset=%d:width=%d:data=%d] \r\n",__func__,__LINE__,object_id,offset,width,data);
                break;

                case 25:
                cfg_641T_T25[offset] = data & 0xFF;
                if(width == 2)
                {
                        cfg_641T_T25[offset+1] =  ((data >> 8) & 0xFF);
                }
                else if(width == 4)
                {
                        cfg_641T_T25[offset + 1] = ((data >> 8) & 0xFF);
                        cfg_641T_T25[offset + 2] = ((data >> 16) & 0xFF);
                        cfg_641T_T25[offset + 3] = ((data >> 24) & 0xFF);
                }
                //printk("[%s:%d::::::::object_id=T%d:offset=%d:width=%d:data=%d] \r\n",__func__,__LINE__,object_id,offset,width,data);
                break;

                case 37:
                cfg_641T_T37[offset] = data & 0xFF;
                if(width == 2)
                {
                        cfg_641T_T37[offset+1] =  ((data >> 8) & 0xFF);
                }
                else if(width == 4)
                {
                        cfg_641T_T37[offset + 1] = ((data >> 8) & 0xFF);
                        cfg_641T_T37[offset + 2] = ((data >> 16) & 0xFF);
                        cfg_641T_T37[offset + 3] = ((data >> 24) & 0xFF);
                }
                //printk("[%s:%d::::::::object_id=T%d:offset=%d:width=%d:data=%d] \r\n",__func__,__LINE__,object_id,offset,width,data);
                break;

                case 42:
                cfg_641T_T42[offset] = data & 0xFF;
                if(width == 2)
                {
                        cfg_641T_T42[offset+1] =  ((data >> 8) & 0xFF);
                }
                else if(width == 4)
                {
                        cfg_641T_T42[offset + 1] = ((data >> 8) & 0xFF);
                        cfg_641T_T42[offset + 2] = ((data >> 16) & 0xFF);
                        cfg_641T_T42[offset + 3] = ((data >> 24) & 0xFF);
                }
                //printk("[%s:%d::::::::object_id=T%d:offset=%d:width=%d:data=%d] \r\n",__func__,__LINE__,object_id,offset,width,data);
                break;

                case 46:
                cfg_641T_T46[offset] = data & 0xFF;
                if(width == 2)
                {
                        cfg_641T_T46[offset+1] =  ((data >> 8) & 0xFF);
                }
                else if(width == 4)
                {
                        cfg_641T_T46[offset + 1] = ((data >> 8) & 0xFF);
                        cfg_641T_T46[offset + 2] = ((data >> 16) & 0xFF);
                        cfg_641T_T46[offset + 3] = ((data >> 24) & 0xFF);
                }
                //printk("[%s:%d::::::::object_id=T%d:offset=%d:width=%d:data=%d] \r\n",__func__,__LINE__,object_id,offset,width,data);
                break;
		
		case 56:
                cfg_641T_T56[offset] = data & 0xFF;
                if(width == 2)
                {
                        cfg_641T_T56[offset+1] =  ((data >> 8) & 0xFF);
                }
                else if(width == 4)
                {
                        cfg_641T_T56[offset + 1] = ((data >> 8) & 0xFF);
                        cfg_641T_T56[offset + 2] = ((data >> 16) & 0xFF);
                        cfg_641T_T56[offset + 3] = ((data >> 24) & 0xFF);
                }
                //printk("[%s:%d::::::::object_id=T%d:offset=%d:width=%d:data=%d] \r\n",__func__,__LINE__,object_id,offset,width,data);
                break;

                case 65:
                cfg_641T_T65[instance][offset] = data & 0xFF;
                if(width == 2)
                {
                        cfg_641T_T65[instance][offset+1] =  ((data >> 8) & 0xFF);
                }
                else if(width == 4)
                {
                        cfg_641T_T65[instance][offset + 1] = ((data >> 8) & 0xFF);
                        cfg_641T_T65[instance][offset + 2] = ((data >> 16) & 0xFF);
                        cfg_641T_T65[instance][offset + 3] = ((data >> 24) & 0xFF);
                }
                //printk("[%s:%d::::::::instance=%d:object_id=T%d:offset=%d:width=%d:data=%d] \r\n",__func__,__LINE__,instance,object_id,offset,width,data);
                break;

                case 72:
                cfg_641T_T72[offset] = data & 0xFF;
                if(width == 2)
                {
                        cfg_641T_T72[offset+1] =  ((data >> 8) & 0xFF);
                }
                else if(width == 4)
                {
                        cfg_641T_T72[offset + 1] = ((data >> 8) & 0xFF);
                        cfg_641T_T72[offset + 2] = ((data >> 16) & 0xFF);
                        cfg_641T_T72[offset + 3] = ((data >> 24) & 0xFF);
                }
                //printk("[%s:%d::::::::object_id=T%d:offset=%d:width=%d:data=%d] \r\n",__func__,__LINE__,object_id,offset,width,data);
                break;

		case 78:
                cfg_641T_T78[offset] = data & 0xFF;
                if(width == 2)
                {
                        cfg_641T_T78[offset+1] =  ((data >> 8) & 0xFF);
                }
                else if(width == 4)
                {
                        cfg_641T_T78[offset + 1] = ((data >> 8) & 0xFF);
                        cfg_641T_T78[offset + 2] = ((data >> 16) & 0xFF);
                        cfg_641T_T78[offset + 3] = ((data >> 24) & 0xFF);
                }
                //printk("[%s:%d::::::::object_id=T%d:offset=%d:width=%d:data=%d] \r\n",__func__,__LINE__,object_id,offset,width,data);
                break;

                case 100:
                cfg_641T_T100[offset] = data & 0xFF;
                if(width == 2)
                {
                        cfg_641T_T100[offset+1] =  ((data >> 8) & 0xFF);
                }
                else if(width == 4)
                {
                        cfg_641T_T100[offset + 1] = ((data >> 8) & 0xFF);
                        cfg_641T_T100[offset + 2] = ((data >> 16) & 0xFF);
                        cfg_641T_T100[offset + 3] = ((data >> 24) & 0xFF);
                }
                //printk("[%s:%d::::::::object_id=T%d:offset=%d:width=%d:data=%d] \r\n",__func__,__LINE__,object_id,offset,width,data);
                break;

                default:
                //printk( "mxt_set_config_file Skip T%d\r\n",width);
                return -1;
        }

	return 0;
}
int mxt_set_config_file_641td(int object_id,int offset,int width,int data,int instance)
{
	if(offset < 0)
		return -1;

        switch (object_id)
        {
                case 7:
                cfg_641TD_T7[offset] = data & 0xFF;
                if(width == 2)
                {
                        cfg_641TD_T7[offset+1] =  ((data >> 8) & 0xFF);
                }
                else if(width == 4)
                {
                        cfg_641TD_T7[offset + 1] = ((data >> 8) & 0xFF);
                        cfg_641TD_T7[offset + 2] = ((data >> 16) & 0xFF);
                        cfg_641TD_T7[offset + 3] = ((data >> 24) & 0xFF);
                }
                //printk("[%s:%d::::::::object_id=T%d:offset=%d:width=%d:data=%d] \r\n",__func__,__LINE__,object_id,offset,width,data);
                break;

                case 8:
                cfg_641TD_T8[offset] = data & 0xFF;
                if(width == 2)
                {
                        cfg_641TD_T8[offset+1] =  ((data >> 8) & 0xFF);
                }
                else if(width == 4)
                {
                        cfg_641TD_T8[offset + 1] = ((data >> 8) & 0xFF);
                        cfg_641TD_T8[offset + 2] = ((data >> 16) & 0xFF);
                        cfg_641TD_T8[offset + 3] = ((data >> 24) & 0xFF);
                }
                //printk("[%s:%d::::::::object_id=T%d:offset=%d:width=%d:data=%d] \r\n",__func__,__LINE__,object_id,offset,width,data);
                break;

                case 38:
                cfg_641TD_T38[offset] = data & 0xFF;
                if(width == 2)
                {
                        cfg_641TD_T38[offset+1] =  ((data >> 8) & 0xFF);
                }
                else if(width == 4)
                {
                        cfg_641TD_T38[offset + 1] = ((data >> 8) & 0xFF);
                        cfg_641TD_T38[offset + 2] = ((data >> 16) & 0xFF);
                        cfg_641TD_T38[offset + 3] = ((data >> 24) & 0xFF);
                }
                //printk("[%s:%d::::::::object_id=T%d:offset=%d:width=%d:data=%d] \r\n",__func__,__LINE__,object_id,offset,width,data);
                break;

                case 25:
                cfg_641TD_T25[offset] = data & 0xFF;
                if(width == 2)
                {
                        cfg_641TD_T25[offset+1] =  ((data >> 8) & 0xFF);
                }
                else if(width == 4)
                {
                        cfg_641TD_T25[offset + 1] = ((data >> 8) & 0xFF);
                        cfg_641TD_T25[offset + 2] = ((data >> 16) & 0xFF);
                        cfg_641TD_T25[offset + 3] = ((data >> 24) & 0xFF);
                }
                //printk("[%s:%d::::::::object_id=T%d:offset=%d:width=%d:data=%d] \r\n",__func__,__LINE__,object_id,offset,width,data);
                break;

                case 37:
                cfg_641TD_T37[offset] = data & 0xFF;
                if(width == 2)
                {
                        cfg_641TD_T37[offset+1] =  ((data >> 8) & 0xFF);
                }
                else if(width == 4)
                {
                        cfg_641TD_T37[offset + 1] = ((data >> 8) & 0xFF);
                        cfg_641TD_T37[offset + 2] = ((data >> 16) & 0xFF);
                        cfg_641TD_T37[offset + 3] = ((data >> 24) & 0xFF);
                }
                //printk("[%s:%d::::::::object_id=T%d:offset=%d:width=%d:data=%d] \r\n",__func__,__LINE__,object_id,offset,width,data);
                break;

                case 42:
                cfg_641TD_T42[offset] = data & 0xFF;
                if(width == 2)
                {
                        cfg_641TD_T42[offset+1] =  ((data >> 8) & 0xFF);
                }
                else if(width == 4)
                {
                        cfg_641TD_T42[offset + 1] = ((data >> 8) & 0xFF);
                        cfg_641TD_T42[offset + 2] = ((data >> 16) & 0xFF);
                        cfg_641TD_T42[offset + 3] = ((data >> 24) & 0xFF);
                }
                //printk("[%s:%d::::::::object_id=T%d:offset=%d:width=%d:data=%d] \r\n",__func__,__LINE__,object_id,offset,width,data);
                break;

                case 46:
                cfg_641TD_T46[offset] = data & 0xFF;
                if(width == 2)
                {
                        cfg_641TD_T46[offset+1] =  ((data >> 8) & 0xFF);
                }
                else if(width == 4)
                {
                        cfg_641TD_T46[offset + 1] = ((data >> 8) & 0xFF);
                        cfg_641TD_T46[offset + 2] = ((data >> 16) & 0xFF);
                        cfg_641TD_T46[offset + 3] = ((data >> 24) & 0xFF);
                }
                //printk("[%s:%d::::::::object_id=T%d:offset=%d:width=%d:data=%d] \r\n",__func__,__LINE__,object_id,offset,width,data);
                break;
		
		case 56:
                cfg_641TD_T56[offset] = data & 0xFF;
                if(width == 2)
                {
                        cfg_641TD_T56[offset+1] =  ((data >> 8) & 0xFF);
                }
                else if(width == 4)
                {
                        cfg_641TD_T56[offset + 1] = ((data >> 8) & 0xFF);
                        cfg_641TD_T56[offset + 2] = ((data >> 16) & 0xFF);
                        cfg_641TD_T56[offset + 3] = ((data >> 24) & 0xFF);
                }
                //printk("[%s:%d::::::::object_id=T%d:offset=%d:width=%d:data=%d] \r\n",__func__,__LINE__,object_id,offset,width,data);
                break;

                case 65:
                cfg_641TD_T65[instance][offset] = data & 0xFF;
                if(width == 2)
                {
                        cfg_641TD_T65[instance][offset+1] =  ((data >> 8) & 0xFF);
                }
                else if(width == 4)
                {
                        cfg_641TD_T65[instance][offset + 1] = ((data >> 8) & 0xFF);
                        cfg_641TD_T65[instance][offset + 2] = ((data >> 16) & 0xFF);
                        cfg_641TD_T65[instance][offset + 3] = ((data >> 24) & 0xFF);
                }
                //printk("[%s:%d::::::::instance=%d:object_id=T%d:offset=%d:width=%d:data=%d] \r\n",__func__,__LINE__,instance,object_id,offset,width,data);
                break;

                case 72:
                cfg_641TD_T72[offset] = data & 0xFF;
                if(width == 2)
                {
                        cfg_641TD_T72[offset+1] =  ((data >> 8) & 0xFF);
                }
                else if(width == 4)
                {
                        cfg_641TD_T72[offset + 1] = ((data >> 8) & 0xFF);
                        cfg_641TD_T72[offset + 2] = ((data >> 16) & 0xFF);
                        cfg_641TD_T72[offset + 3] = ((data >> 24) & 0xFF);
                }
                //printk("[%s:%d::::::::object_id=T%d:offset=%d:width=%d:data=%d] \r\n",__func__,__LINE__,object_id,offset,width,data);
                break;

		case 78:
                cfg_641TD_T78[offset] = data & 0xFF;
                if(width == 2)
                {
                        cfg_641TD_T78[offset+1] =  ((data >> 8) & 0xFF);
                }
                else if(width == 4)
                {
                        cfg_641TD_T78[offset + 1] = ((data >> 8) & 0xFF);
                        cfg_641TD_T78[offset + 2] = ((data >> 16) & 0xFF);
                        cfg_641TD_T78[offset + 3] = ((data >> 24) & 0xFF);
                }
                //printk("[%s:%d::::::::object_id=T%d:offset=%d:width=%d:data=%d] \r\n",__func__,__LINE__,object_id,offset,width,data);
                break;

		case 80:
                cfg_641TD_T80[offset] = data & 0xFF;
                if(width == 2)
                {
                        cfg_641TD_T80[offset+1] =  ((data >> 8) & 0xFF);
                }
                else if(width == 4)
                {
                        cfg_641TD_T80[offset + 1] = ((data >> 8) & 0xFF);
                        cfg_641TD_T80[offset + 2] = ((data >> 16) & 0xFF);
                        cfg_641TD_T80[offset + 3] = ((data >> 24) & 0xFF);
                }
                //printk("[%s:%d::::::::object_id=T%d:offset=%d:width=%d:data=%d] \r\n",__func__,__LINE__,object_id,offset,width,data);
                break;

                case 100:
                cfg_641TD_T100[offset] = data & 0xFF;
                if(width == 2)
                {
                        cfg_641TD_T100[offset+1] =  ((data >> 8) & 0xFF);
                }
                else if(width == 4)
                {
                        cfg_641TD_T100[offset + 1] = ((data >> 8) & 0xFF);
                        cfg_641TD_T100[offset + 2] = ((data >> 16) & 0xFF);
                        cfg_641TD_T100[offset + 3] = ((data >> 24) & 0xFF);
                }
                //printk("[%s:%d::::::::object_id=T%d:offset=%d:width=%d:data=%d] \r\n",__func__,__LINE__,object_id,offset,width,data);
                break;

		case 104:
                cfg_641TD_T104[offset] = data & 0xFF;
                if(width == 2)
                {
                        cfg_641TD_T104[offset+1] =  ((data >> 8) & 0xFF);
                }
                else if(width == 4)
                {
                        cfg_641TD_T104[offset + 1] = ((data >> 8) & 0xFF);
                        cfg_641TD_T104[offset + 2] = ((data >> 16) & 0xFF);
                        cfg_641TD_T104[offset + 3] = ((data >> 24) & 0xFF);
                }
                //printk("[%s:%d::::::::object_id=T%d:offset=%d:width=%d:data=%d] \r\n",__func__,__LINE__,object_id,offset,width,data);
                break;

		case 109:
                cfg_641TD_T109[offset] = data & 0xFF;
                if(width == 2)
                {
                        cfg_641TD_T109[offset+1] =  ((data >> 8) & 0xFF);
                }
                else if(width == 4)
                {
                        cfg_641TD_T109[offset + 1] = ((data >> 8) & 0xFF);
                        cfg_641TD_T109[offset + 2] = ((data >> 16) & 0xFF);
                        cfg_641TD_T109[offset + 3] = ((data >> 24) & 0xFF);
                }
                //printk("[%s:%d::::::::object_id=T%d:offset=%d:width=%d:data=%d] \r\n",__func__,__LINE__,object_id,offset,width,data);
                break;

		case 110:
		cfg_641TD_T110[instance][offset] = data & 0xFF;
                if(width == 2)
                {
                        cfg_641TD_T110[instance][offset+1] =  ((data >> 8) & 0xFF);
                }
                else if(width == 4)
                {
                        cfg_641TD_T110[instance][offset + 1] = ((data >> 8) & 0xFF);
                        cfg_641TD_T110[instance][offset + 2] = ((data >> 16) & 0xFF);
                        cfg_641TD_T110[instance][offset + 3] = ((data >> 24) & 0xFF);
                }
                //printk("[%s:%d::::::::instance=%d:object_id=T%d:offset=%d:width=%d:data=%d] \r\n",__func__,__LINE__,instance,object_id,offset,width,data);
                break;

		case 111:
                cfg_641TD_T111[instance][offset] = data & 0xFF;
                if(width == 2)
                {
                        cfg_641TD_T111[instance][offset+1] =  ((data >> 8) & 0xFF);
                }
                else if(width == 4)
                {
                        cfg_641TD_T111[instance][offset + 1] = ((data >> 8) & 0xFF);
                        cfg_641TD_T111[instance][offset + 2] = ((data >> 16) & 0xFF);
                        cfg_641TD_T111[instance][offset + 3] = ((data >> 24) & 0xFF);
                }
                //printk("[%s:%d::::::::instance=%d:object_id=T%d:offset=%d:width=%d:data=%d] \r\n",__func__,__LINE__,instance,object_id,offset,width,data);
                break;

		case 113:
                cfg_641TD_T113[offset] = data & 0xFF;
                if(width == 2)
                {
                        cfg_641TD_T113[offset+1] =  ((data >> 8) & 0xFF);
                }
                else if(width == 4)
                {
                        cfg_641TD_T113[offset + 1] = ((data >> 8) & 0xFF);
                        cfg_641TD_T113[offset + 2] = ((data >> 16) & 0xFF);
                        cfg_641TD_T113[offset + 3] = ((data >> 24) & 0xFF);
                }
                //printk("[%s:%d::::::::object_id=T%d:offset=%d:width=%d:data=%d] \r\n",__func__,__LINE__,object_id,offset,width,data);
                break;

		case 133:
		cfg_641TD_T133[offset] = data & 0xFF;
                if(width == 2)
                {
                        cfg_641TD_T133[offset+1] =  ((data >> 8) & 0xFF);
                }
                else if(width == 4)
                {
                        cfg_641TD_T133[offset + 1] = ((data >> 8) & 0xFF);
                        cfg_641TD_T133[offset + 2] = ((data >> 16) & 0xFF);
                        cfg_641TD_T133[offset + 3] = ((data >> 24) & 0xFF);
                }
                //printk("[%s:%d::::::::object_id=T%d:offset=%d:width=%d:data=%d] \r\n",__func__,__LINE__,object_id,offset,width,data);
                break;

                default:
                //printk( "mxt_set_config_file Skip T%d:offset=%d:width=%d:data=%d\r\n",object_id,offset,width,data);
                return -1;
        }

	return 0;
}
#endif


int mxt_get_objects(struct mxt_data *mxt, u8 *buf, int buf_size, int obj_type)
{
	u16 obj_size = 0;
	u16 obj_addr = 0;
	int error;

	if (!mxt || !buf) {
		pr_err("[TSP] invalid parameter (mxt:%p, buf:%p)\n", mxt, buf);
		return -1;
	}

	obj_addr = MXT_BASE_ADDR(obj_type);
	obj_size = MXT_GET_SIZE(obj_type);

	if ((obj_addr == 0) || (obj_size == 0) || (obj_size > buf_size)) {
		dev_err(&mxt->client->dev, "[TSP] Invalid object(addr:0x%04X, "
				"size:%d, object_type:%d)\n",
				obj_addr, obj_size, obj_type);
		return -1;
	}

	error = mxt_read_block(mxt->client, obj_addr, obj_size, buf);
	if (error < 0) {
		dev_err(&mxt->client->dev, "[TSP] mxt_read_block failed! "
				"(%s, %d)\n", __func__, __LINE__);
		return -EIO;
	}

	return 0;
}

int mxt_get_object_values(struct mxt_data *mxt, int obj_type)
{
	struct i2c_client *client = mxt->client;


	u8 *obj = NULL;
	u16 obj_size = 0;
	u16 obj_addr = 0;
	int error;
	int version;
	version = (mxt->device_info.major * 10) + (mxt->device_info.minor);

	switch (obj_type) {
	case MXT_GEN_POWERCONFIG_T7:
		obj = (u8 *)&power_config;
		break;

	case MXT_GEN_ACQUIRECONFIG_T8:
		obj = (u8 *)&acquisition_config_mxt336s;
		break;

	case MXT_TOUCH_MULTITOUCHSCREEN_T9:
		obj = (u8 *)&touchscreen_config;
		break;

	/*case MXT_PROCI_GRIPSUPPRESSION_T40:
		obj = (u8 *)&gripsuppression_t40_config;
		break;*/

	case MXT_PROCI_TOUCHSUPPRESSION_T42:
		obj = (u8 *)&touchsuppression_t42_config;
		break;

	case MXT_SPT_CTECONFIG_T46:
		obj = (u8 *)&cte_t46_config;
		break;

	/*case MXT_PROCI_STYLUS_T47:
		obj = (u8 *)&stylus_t47_config;
		break;*/

	case MXT_PROCG_NOISESUPPRESSION_T48:
		if (version <= 10) {
			obj = (u8 *)&noisesuppression_t48_config;
			break;
		}
		else if (version >= 21) {
			obj = (u8 *)&nsuppression_t48_ver2_config;
			break;
		}
	case MXT_USER_INFO_T38:
		obj = (u8 *)&userdata_t38;
		break;

	default:
		dev_err(&mxt->client->dev, "[TSP] Not supporting object type "
				"(object type: %d)", obj_type);
		return -1;
	}

	obj_addr = MXT_BASE_ADDR(obj_type);
	obj_size = MXT_GET_SIZE(obj_type);

	if ((obj_addr == 0) || (obj_size == 0)) {
		dev_err(&mxt->client->dev, "[TSP] Not supporting object type "
				"(object type: %d)\n", obj_type);
		return -1;
	}

	error = mxt_read_block(client, obj_addr, obj_size, obj);
	if (error < 0) {
		dev_err(&client->dev, "[TSP] mxt_read_block failed! "
				"(%s, %d)\n", __func__, __LINE__);
		return -EIO;
	}

	return 0;
}

int mxt_copy_object(struct mxt_data *mxt, u8 *buf, int obj_type)
{
	u8 *obj = NULL;
	u16 obj_size = 0;
	int version;
	version = (mxt->device_info.major * 10) + (mxt->device_info.minor);

	switch (obj_type) {
	case MXT_GEN_POWERCONFIG_T7:
		obj = (u8 *)&power_config;
		obj_size = MXT_GET_SIZE(obj_type);
		break;

	case MXT_GEN_ACQUIRECONFIG_T8:
		obj = (u8 *)&acquisition_config_mxt336s;
		obj_size = MXT_GET_SIZE(obj_type);
		break;

	case MXT_TOUCH_MULTITOUCHSCREEN_T9:
		obj = (u8 *)&touchscreen_config;
		obj_size = MXT_GET_SIZE(obj_type);
		break;

	/*case MXT_PROCI_GRIPSUPPRESSION_T40:
		obj = (u8 *)&gripsuppression_t40_config;
		obj_size = MXT_GET_SIZE(obj_type);
		break;*/

	case MXT_PROCI_TOUCHSUPPRESSION_T42:
		obj = (u8 *)&touchsuppression_t42_config;
		obj_size = MXT_GET_SIZE(obj_type);
		break;

	case MXT_SPT_CTECONFIG_T46:
		obj = (u8 *)&cte_t46_config;
		obj_size = MXT_GET_SIZE(obj_type);
		break;

	/*case MXT_PROCI_STYLUS_T47:
		obj = (u8 *)&stylus_t47_config;
		obj_size = MXT_GET_SIZE(obj_type);
		break;*/

	case MXT_PROCG_NOISESUPPRESSION_T48:
		if (version <= 10) {
			obj = (u8 *)&noisesuppression_t48_config;
			obj_size = MXT_GET_SIZE(obj_type);
			break;
		}
		else if (version >= 21) {
			obj = (u8 *)&nsuppression_t48_ver2_config;
			obj_size = MXT_GET_SIZE(obj_type);
			break;
		}

	case MXT_USER_INFO_T38:
		obj = (u8 *)&userdata_t38;
		obj_size = MXT_GET_SIZE(obj_type);
		break;

	default:
		dev_err(&mxt->client->dev, "[TSP] Not supporting object type "
				"(object type: %d)\n", obj_type);
		return -1;
	}

	dev_info(&mxt->client->dev, "[TSP] obj type: %d, obj size: %d\n",
			obj_type, obj_size);

	if (memcpy(buf, obj, obj_size) == NULL) {
		dev_err(&mxt->client->dev, "[TSP] memcpy failed! (%s, %d)\n",
				__func__, __LINE__);
		return -1;
	}

	return 0;
}

int mxt_userdata_config(struct mxt_data *mxt)
{
	struct i2c_client *client = mxt->client;
	u16 obj_addr, obj_size;
	int error;

	obj_addr = MXT_BASE_ADDR(MXT_USER_INFO_T38);
	obj_size = MXT_GET_SIZE(MXT_USER_INFO_T38);

    userdata_t38.data[0] = t38_userdata0[touch_type];
    userdata_t38.data[1] = t38_userdata1[touch_type];
    userdata_t38.data[2] = t38_userdata2[touch_type];
    userdata_t38.data[3] = t38_userdata3[touch_type];
    userdata_t38.data[4] = t38_userdata4[touch_type];
    userdata_t38.data[5] = t38_userdata5[touch_type];
    userdata_t38.data[6] = t38_userdata6[touch_type];
    userdata_t38.data[7] = t38_userdata7[touch_type];

	error = mxt_write_block(client,
			obj_addr, obj_size, (u8 *)&userdata_t38);
	if (error < 0) {
		dev_err(&client->dev, "[TSP] mxt_write_block failed! "
				"(%s, %d)\n", __func__, __LINE__);
		return -EIO;
	}

	return 0;
}

int mxt_power_config(struct mxt_data *mxt, u8 *mem, int which)
{
	struct i2c_client *client = mxt->client;

	u16 obj_addr, obj_size;
	int error;

	if((which == TOMEM) && (mem == NULL))
	{
		return -EINVAL;
	}

	obj_addr = MXT_BASE_ADDR(MXT_GEN_POWERCONFIG_T7);
	obj_size = MXT_GET_SIZE(MXT_GEN_POWERCONFIG_T7);

	/* Set Idle Acquisition Interval to 32 ms. */
	power_config.idleacqint = t7_idleacqint[touch_type];
	/* Set Active Acquisition Interval to 16 ms. */
	power_config.actvacqint = t7_actvacqint[touch_type];
	/* Set Active to Idle Timeout to 4 s (one unit = 200ms). */
	power_config.actv2idleto = t7_actv2idleto[touch_type];
	// sjpark@cleinsoft : modified
	power_config.cfg = t7_cfg[touch_type];
	
	if(which == TOMEM)
	{
		memcpy(mem+obj_addr, &power_config, sizeof(power_config));
		return 0;
	}

	error = mxt_write_block(client,
			obj_addr, obj_size, (u8 *)&power_config);
	if (error < 0) {
		dev_err(&client->dev, "[TSP] mxt_write_block failed! "
				"(%s, %d)\n", __func__, __LINE__);
		return -EIO;
	}
	return 0;
}

/* extern bool mxt_reconfig_flag; */
int mxt336s_acquisition_config(struct mxt_data *mxt, u8 *mem, int which)
{
	struct i2c_client *client = mxt->client;
	u16 obj_addr, obj_size;
	int error;

	if((which == TOMEM) && (mem == NULL))
	{
		return -EINVAL;
	}

	obj_addr = MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8);
	obj_size = MXT_GET_SIZE(MXT_GEN_ACQUIRECONFIG_T8);

    acquisition_config_mxt336s.chrgtime     = t8_chrgtime[touch_type];
    acquisition_config_mxt336s.reserved     = t8_reserved[touch_type];
    acquisition_config_mxt336s.tchdrift     = t8_tchdrift[touch_type];
    acquisition_config_mxt336s.driftst      = t8_driftst[touch_type];
    acquisition_config_mxt336s.tchautocal   = t8_tchautocal[touch_type];
    acquisition_config_mxt336s.sync         = t8_sync[touch_type];
    acquisition_config_mxt336s.atchcalst    = t8_atchcalst[touch_type];
    acquisition_config_mxt336s.atchcalsthr  = t8_atchcalsthr[touch_type];
    /*!< Anti-touch force calibration threshold */
    acquisition_config_mxt336s.atchcalfrcthr = t8_atchcalfrcthr[touch_type];
    /*!< Anti-touch force calibration ratio */
    acquisition_config_mxt336s.atchcalfrcratio = t8_atchcalfrcratio[touch_type];

	if(which == TOMEM)
	{
		memcpy(mem+obj_addr, &acquisition_config_mxt336s, sizeof(acquisition_config_mxt336s));
		return 0;
	}

	error = mxt_write_block(client,
			obj_addr, obj_size, (u8 *)&acquisition_config_mxt336s);
	if (error < 0) {
		dev_err(&client->dev, "[TSP] mxt_write_block failed! "
				"(%s, %d)\n", __func__, __LINE__);
		return -EIO;
	}

	return 0;
}
EXPORT_SYMBOL(mxt336s_acquisition_config);

int mxt_multitouch_config(struct mxt_data *mxt, u8 *mem, int which)
{
	struct i2c_client *client = mxt->client;
	u16 obj_addr, obj_size;
	int error;

	if((which == TOMEM) && (mem == NULL))
	{
		return -EINVAL;
	}

	obj_addr = MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9);
	obj_size = MXT_GET_SIZE(MXT_TOUCH_MULTITOUCHSCREEN_T9);

    touchscreen_config.ctrl         = t9_ctrl[touch_type];
    touchscreen_config.xorigin      = t9_xorigin[touch_type];
    touchscreen_config.yorigin      = t9_yorigin[touch_type];
    touchscreen_config.xsize        = t9_xsize[touch_type];
    touchscreen_config.ysize        = t9_ysize[touch_type];
    touchscreen_config.akscfg       = t9_akscfg[touch_type];
    touchscreen_config.blen         = t9_blen[touch_type];

    touchscreen_config.tchthr       = t9_tchthr[touch_type];

    touchscreen_config.tchdi        = t9_tchdi[touch_type];
    touchscreen_config.orient       = t9_orient[touch_type];
    touchscreen_config.mrgtimeout   = t9_mrgtimeout[touch_type];
    touchscreen_config.movhysti     = t9_movhysti[touch_type];
    touchscreen_config.movhystn     = t9_movhystn[touch_type];
    touchscreen_config.movfilter    = t9_movfilter[touch_type];
    touchscreen_config.numtouch     = t9_numtouch[touch_type];
    touchscreen_config.mrghyst      = t9_mrghyst[touch_type];
    touchscreen_config.mrgthr       = t9_mrgthr[touch_type];
    touchscreen_config.amphyst      = t9_amphyst[touch_type];
    touchscreen_config.xrange       = t9_xrange[touch_type];
    touchscreen_config.yrange       = t9_yrange[touch_type];
    touchscreen_config.xloclip      = t9_xloclip[touch_type];
    touchscreen_config.xhiclip      = t9_xhiclip[touch_type];
    touchscreen_config.yloclip      = t9_yloclip[touch_type];
    touchscreen_config.yhiclip      = t9_yhiclip[touch_type];
    touchscreen_config.xedgectrl    = t9_xedgectrl[touch_type];
    touchscreen_config.xedgedist    = t9_xedgedist[touch_type];
    touchscreen_config.yedgectrl    = t9_yedgectrl[touch_type];
    touchscreen_config.yedgedist    = t9_yedgedist[touch_type];
    touchscreen_config.jumplimit    = t9_jumplimit[touch_type];

    touchscreen_config.tchhyst      = t9_tchhyst[touch_type];
    touchscreen_config.xpitch       = t9_xpitch[touch_type];
    touchscreen_config.ypitch       = t9_ypitch[touch_type];

    touchscreen_config.nexttchdi    = t9_nexttchdi[touch_type];
    touchscreen_config.cfg          = t9_cfg[touch_type];

	if(which == TOMEM)
	{
		memcpy(mem+obj_addr, &touchscreen_config, sizeof(touchscreen_config));
		return 0;
	}

	error = mxt_write_block(client, obj_addr,
			obj_size, (u8 *)&touchscreen_config);
	if (error < 0) {
		dev_err(&client->dev, "[TSP] mxt_write_block failed! "
				"(%s, %d)\n", __func__, __LINE__);
		return -EIO;
	}
	return 0;
}

int mxt_touch_suppression_t42_config(struct mxt_data *mxt, u8 *mem, int which)
{
	struct i2c_client *client = mxt->client;
	u16 obj_addr, obj_size;
	int error;
	int version;

	if((which == TOMEM) && (mem == NULL))
	{
		return -EINVAL;
	}

	version = (mxt->device_info.major * 10) + (mxt->device_info.minor);

	obj_addr = MXT_BASE_ADDR(MXT_PROCI_TOUCHSUPPRESSION_T42);
	obj_size = MXT_GET_SIZE(MXT_PROCI_TOUCHSUPPRESSION_T42);

	if (1) {
        touchsuppression_t42_config.ctrl = t42_ctrl[touch_type];
        touchsuppression_t42_config.apprthr = t42_apprthr[touch_type];
        touchsuppression_t42_config.maxapprarea = t42_maxapprarea[touch_type];
        touchsuppression_t42_config.maxtcharea  = t42_maxtcharea[touch_type];
        touchsuppression_t42_config.supstrength = t42_supstrength[touch_type];
        touchsuppression_t42_config.supextto    = t42_supextto[touch_type];
        touchsuppression_t42_config.maxnumtchs  = t42_maxnumtchs[touch_type];
        touchsuppression_t42_config.shapestrength = t42_shapestrength[touch_type];

		if(which == TOMEM)
		{
			memcpy(mem+obj_addr, &touchsuppression_t42_config, sizeof(touchsuppression_t42_config));
			return 0;
		}

		error = mxt_write_block(client, obj_addr,
				obj_size, (u8 *)&touchsuppression_t42_config);
		if (error < 0) {
			dev_err(&client->dev, "[TSP] mxt_write_block failed! "
					"(%s, %d)\n", __func__, __LINE__);
			return -EIO;
		}
	}
	else if (version >= 21) {
		tsuppression_t42_ver2_config.ctrl = t42_ctrl[touch_type];
		tsuppression_t42_ver2_config.apprthr = t42_apprthr[touch_type];
		tsuppression_t42_ver2_config.maxapprarea = t42_maxapprarea[touch_type];
		tsuppression_t42_ver2_config.maxtcharea = t42_maxtcharea[touch_type];
		tsuppression_t42_ver2_config.supstrength = t42_supstrength[touch_type];
		tsuppression_t42_ver2_config.supextto = t42_supextto[touch_type];
		tsuppression_t42_ver2_config.maxnumtchs = t42_maxnumtchs[touch_type];
		tsuppression_t42_ver2_config.shapestrength = t42_shapestrength[touch_type];
		tsuppression_t42_ver2_config.supdist = t42_supdist[touch_type];
		tsuppression_t42_ver2_config.disthyst = t42_disthyst[touch_type];

		if(which == TOMEM)
		{
			memcpy(mem+obj_addr, &tsuppression_t42_ver2_config, sizeof(tsuppression_t42_ver2_config));
			return 0;
		}

		error = mxt_write_block(client, obj_addr,
				obj_size, (u8 *)&tsuppression_t42_ver2_config);
		if (error < 0) {
			dev_err(&client->dev, "[TSP] mxt_write_block failed! "
					"(%s, %d)\n", __func__, __LINE__);
			return -EIO;
		}
	}
	return 0;
}

int mxt_cte_t46_config(struct mxt_data *mxt, u8 *mem, int which)
{
	struct i2c_client *client = mxt->client;
	u16 obj_addr, obj_size;
	int error = 0;
	int version;

	if((which == TOMEM) && (mem == NULL))
	{
		return -EINVAL;
	}

	version = (mxt->device_info.major * 10) + (mxt->device_info.minor);

	obj_addr = MXT_BASE_ADDR(MXT_SPT_CTECONFIG_T46);
	obj_size = MXT_GET_SIZE(MXT_SPT_CTECONFIG_T46);

	/* Set CTE config */
	//valky@cleinsoft if (version <= 10) {
	if (1) {
        cte_t46_config.ctrl = t46_ctrl[touch_type];
        cte_t46_config.mode = t46_mod[touch_type];
        cte_t46_config.idlesyncsperx = t46_idlesyncsperx[touch_type];
        cte_t46_config.actvsyncsperx = t46_actvsyncsperx[touch_type];
        cte_t46_config.adcspersync  = t46_adcspersync[touch_type];
        cte_t46_config.pulsesperadc = t46_pulsesperadc[touch_type];
        cte_t46_config.xslew = t46_xslew[touch_type];
        cte_t46_config.syncdelay = t46_syncdelay[touch_type];
        cte_t46_config.xvoltage = t46_xvoltage[touch_type];

		if(which == TOMEM)
		{
			memcpy(mem+obj_addr, &cte_t46_config, sizeof(cte_t46_config));
			return 0;
		}

		/* Write CTE config to chip. */
		error = mxt_write_block(client, obj_addr, obj_size,
				(u8 *)&cte_t46_config);
		if (error < 0) {
			dev_err(&client->dev, "[TSP] mxt_write_block failed! "
					"(%s, %d)\n", __func__, __LINE__);
			return -EIO;
		}
	}
	else if (version >= 21) {
		cte_t46_ver2_config.ctrl = t46_ctrl[touch_type];
		cte_t46_ver2_config.reserved = t46_mod[touch_type];
		cte_t46_ver2_config.idlesyncsperx = t46_idlesyncsperx[touch_type];
		cte_t46_ver2_config.actvsyncsperx = t46_actvsyncsperx[touch_type];
		cte_t46_ver2_config.adcspersync	= t46_adcspersync[touch_type];
		cte_t46_ver2_config.pulsesperadc = t46_pulsesperadc[touch_type];
		cte_t46_ver2_config.xslew = t46_xslew[touch_type];
		cte_t46_ver2_config.syncdelay = t46_syncdelay[touch_type];
		cte_t46_ver2_config.xvoltage = t46_xvoltage[touch_type];

		if(which == TOMEM)
		{
			memcpy(mem+obj_addr, &cte_t46_ver2_config, sizeof(cte_t46_ver2_config));
			return 0;
		}

		/* Write CTE config to chip. */
		error = mxt_write_block(client, obj_addr, obj_size,
				(u8 *)&cte_t46_ver2_config);
		if (error < 0) {
			dev_err(&client->dev, "[TSP] mxt_write_block failed! "
					"(%s, %d)\n", __func__, __LINE__);
			return -EIO;
		}
	}

	return error;
}

int mxt336s_noisesuppression_t48_config(struct mxt_data *mxt)
{
	struct i2c_client *client = mxt->client;
	u16 obj_addr, obj_size, size;
	int error;

	obj_addr = MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T48);
	obj_size = MXT_GET_SIZE(MXT_PROCG_NOISESUPPRESSION_T48);

	size = sizeof(noisesuppression_t48_config);
	if (obj_size > size) {
		char *buff = NULL;
		buff = kzalloc(obj_size, GFP_KERNEL);
		if (!buff) {
			dev_err(&client->dev, "[TSP] failed to alloc mem\n");
			return -ENOMEM;
		}
		memcpy(buff, &noisesuppression_t48_config, size);
		error = mxt_write_block(client,
				obj_addr, obj_size, buff);
		kfree(buff);
	} else {
		error = mxt_write_block(client,
				obj_addr, obj_size,
				(u8 *)&noisesuppression_t48_config);
	}
	if (error < 0) {
		dev_err(&client->dev, "[TSP] mxt_write_block failed! "
				"(%s, %d)\n", __func__, __LINE__);
		return -EIO;
	}

	return 0;
}
EXPORT_SYMBOL(mxt336s_noisesuppression_t48_config);

int mxt336s_noisesuppression_t48_ver2_config(struct mxt_data *mxt)
{
	return 0;
}

EXPORT_SYMBOL(mxt336s_noisesuppression_t48_ver2_config);

int mxt_proximity_config(struct mxt_data *mxt)
{
	return 1;
}

int mxt_proximity_config1(struct mxt_data *mxt)
{
	return 1;
}

int mxt_shieldless_t56_config(struct mxt_data *mxt, u8 *mem, int which)
{
	struct i2c_client *client = mxt->client;
	u16 obj_addr, obj_size;
	int error = 0;

	if((which == TOMEM) && (mem == NULL))
	{
		return -EINVAL;
	}

	obj_addr = MXT_BASE_ADDR(MXT_SHIELDLESS_T56);
	obj_size = MXT_GET_SIZE(MXT_SHIELDLESS_T56);

    shieldless_t56_config.ctrl = t56_ctrl[touch_type];
    shieldless_t56_config.command = t56_command[touch_type];
    shieldless_t56_config.optint = t56_optint[touch_type];
    shieldless_t56_config.inttime = t56_inttime[touch_type];
    shieldless_t56_config.intdelay[0] = t56_intdelay0[touch_type];
    shieldless_t56_config.intdelay[1] = t56_intdelay1[touch_type];
    shieldless_t56_config.intdelay[2] = t56_intdelay2[touch_type];
    shieldless_t56_config.intdelay[3] = t56_intdelay3[touch_type];
    shieldless_t56_config.intdelay[4] = t56_intdelay4[touch_type];
    shieldless_t56_config.intdelay[5] = t56_intdelay5[touch_type];
    shieldless_t56_config.intdelay[6] = t56_intdelay6[touch_type];
    shieldless_t56_config.intdelay[7] = t56_intdelay7[touch_type];
    shieldless_t56_config.intdelay[8] = t56_intdelay8[touch_type];
    shieldless_t56_config.intdelay[9] = t56_intdelay9[touch_type];
    shieldless_t56_config.intdelay[10] = t56_intdelay10[touch_type];
    shieldless_t56_config.intdelay[11] = t56_intdelay11[touch_type];
    shieldless_t56_config.intdelay[12] = t56_intdelay12[touch_type];
    shieldless_t56_config.intdelay[13] = t56_intdelay13[touch_type];
    shieldless_t56_config.intdelay[14] = t56_intdelay14[touch_type];
    shieldless_t56_config.intdelay[15] = t56_intdelay15[touch_type];
    shieldless_t56_config.intdelay[16] = t56_intdelay16[touch_type];
    shieldless_t56_config.intdelay[17] = t56_intdelay17[touch_type];
    shieldless_t56_config.intdelay[18] = t56_intdelay18[touch_type];
    shieldless_t56_config.intdelay[19] = t56_intdelay19[touch_type];
    shieldless_t56_config.intdelay[20] = t56_intdelay20[touch_type];
    shieldless_t56_config.intdelay[21] = t56_intdelay21[touch_type];
    shieldless_t56_config.intdelay[22] = t56_intdelay22[touch_type];
    shieldless_t56_config.intdelay[23] = t56_intdelay23[touch_type];
    shieldless_t56_config.multicutgc = t56_multicutgc[touch_type];
	shieldless_t56_config.reserved = t56_reserved1[touch_type];
    shieldless_t56_config.ncncl = t56_ncncl[touch_type];
    shieldless_t56_config.touchbias = t56_touchbias[touch_type];
    shieldless_t56_config.basescale = t56_basescale[touch_type];
    shieldless_t56_config.shiftlimit = t56_shiftlimit[touch_type];
    shieldless_t56_config.ylonoisemul = t56_ylonoisemul[touch_type];
    shieldless_t56_config.ylonoisediv = t56_ylonoisediv[touch_type];
    shieldless_t56_config.yhinoisemul = t56_yhinoisemul[touch_type];
    shieldless_t56_config.yhinoisediv = t56_yhinoisediv[touch_type];
    shieldless_t56_config.reserved = t56_reserved2[touch_type];

	if(which == TOMEM)
	{
		memcpy(mem+obj_addr, &shieldless_t56_config, sizeof(shieldless_t56_config));
		return 0;
	}

	error = mxt_write_block(client, obj_addr,
			obj_size, (u8 *)&shieldless_t56_config);
	if (error < 0) {
		dev_err(&client->dev, "[TSP] mxt_write_block failed! "
				"(%s, %d)\n", __func__, __LINE__);
		return -EIO;
	}
	return 0;
}

int mxt_extra_touchscreen_data_t57_config(struct mxt_data *mxt)
{
	return 0;
}

int mxt_extra_touchscreen_data_t57_config1(struct mxt_data *mxt)
{
	return 0;
}

/**
 * @author sjpark@cleinsoft
 * @date 2013/11/27
 * mXT336S_AT new fuction for T62
 **/
int mxt336s_noisesuppression_t62_config(struct mxt_data *mxt, u8 *mem, int which)
{
	struct i2c_client *client = mxt->client;
	u16 obj_addr, obj_size;
	int error;

	if((which == TOMEM) && (mem == NULL))
	{
		return -EINVAL;
	}

	obj_addr = MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T62);
	obj_size = MXT_GET_SIZE(MXT_PROCG_NOISESUPPRESSION_T62);

    noisesuppression_t62_config.ctrl            = t62_ctrl[touch_type];
    noisesuppression_t62_config.calcfg1         = t62_calcfg1[touch_type];
    noisesuppression_t62_config.calcfg2         = t62_calcfg2[touch_type];
    noisesuppression_t62_config.calcfg3         = t62_calcfg3[touch_type];
    noisesuppression_t62_config.cfg             = t62_cfg[touch_type];
    noisesuppression_t62_config.reserved0       = t62_reserved0[touch_type];
    noisesuppression_t62_config.minthradj       = t62_minthradj[touch_type];
    noisesuppression_t62_config.basefreq        = t62_basefreq[touch_type];
    noisesuppression_t62_config.maxselfreq      = t62_maxselfreq[touch_type];
    noisesuppression_t62_config.freq0           = t62_freq0[touch_type];
    noisesuppression_t62_config.freq1           = t62_freq1[touch_type];
    noisesuppression_t62_config.freq2           = t62_freq2[touch_type];
    noisesuppression_t62_config.freq3           = t62_freq3[touch_type];
    noisesuppression_t62_config.freq4           = t62_freq4[touch_type];
    noisesuppression_t62_config.hopcnt          = t62_hopcnt[touch_type];
    noisesuppression_t62_config.reserved1       = t62_reserved1[touch_type];
    noisesuppression_t62_config.hopcntper       = t62_hopcntper[touch_type];
    noisesuppression_t62_config.hopevalto       = t62_hopevalto[touch_type];
    noisesuppression_t62_config.hopst           = t62_hopst[touch_type];
    noisesuppression_t62_config.nlgain          = t62_nlgain[touch_type];
    noisesuppression_t62_config.minnlthr        = t62_minnlthr[touch_type];
    noisesuppression_t62_config.incnlthr        = t62_incnlthr[touch_type];
    noisesuppression_t62_config.adcperxthr      = t62_adcperxthr[touch_type];
    noisesuppression_t62_config.nlthrmargin     = t62_nlthrmargin[touch_type];
    noisesuppression_t62_config.maxadcperx      = t62_maxadcperx[touch_type];
    noisesuppression_t62_config.actvadcsvldnod  = t62_actvadcsvldnod[touch_type];
    noisesuppression_t62_config.idleadcsvldnod  = t62_idleadcsvldnod[touch_type];
    noisesuppression_t62_config.mingclimit      = t62_mingclimit[touch_type];
    noisesuppression_t62_config.maxgclimit      = t62_maxgclimit[touch_type];
    noisesuppression_t62_config.reserved2       = t62_reserved2[touch_type];
    noisesuppression_t62_config.reserved3       = t62_reserved3[touch_type];
    noisesuppression_t62_config.reserved4       = t62_reserved4[touch_type];
    noisesuppression_t62_config.reserved5       = t62_reserved5[touch_type];
    noisesuppression_t62_config.reserved6       = t62_reserved6[touch_type];
    noisesuppression_t62_config.blen0           = t62_blen0[touch_type];
    noisesuppression_t62_config.tchthr0         = t62_tchthr0[touch_type];
    noisesuppression_t62_config.tchdi0          = t62_tchdi0[touch_type];
    noisesuppression_t62_config.movhysti0       = t62_movhysti0[touch_type];
    noisesuppression_t62_config.movhystn0       = t62_movhystn0[touch_type];
    noisesuppression_t62_config.movfilter0      = t62_movfilter0[touch_type];
    noisesuppression_t62_config.numtouch0       = t62_numtouch0[touch_type];
    noisesuppression_t62_config.mrghyst0        = t62_mrghyst0[touch_type];
    noisesuppression_t62_config.mrgthr0         = t62_mrgthr0[touch_type];
    noisesuppression_t62_config.xloclip0        = t62_xloclip0[touch_type];
    noisesuppression_t62_config.xhiclip0        = t62_xhiclip0[touch_type];
    noisesuppression_t62_config.yloclip0        = t62_yloclip0[touch_type];
    noisesuppression_t62_config.yhiclip0        = t62_yhiclip0[touch_type];
    noisesuppression_t62_config.xedgectrl0      = t62_xedgectrl0[touch_type];
    noisesuppression_t62_config.xedgedist0      = t62_xedgedist0[touch_type];
    noisesuppression_t62_config.yedgectrl0      = t62_yedgectrl0[touch_type];
    noisesuppression_t62_config.yedgedist0      = t62_yedgedist0[touch_type];
    noisesuppression_t62_config.jumplimit0      = t62_jumplimit0[touch_type];
    noisesuppression_t62_config.tchhyst0        = t62_tchhyst0[touch_type];
    noisesuppression_t62_config.nexttchdi0      = t62_nexttchdi0[touch_type];

	if(which == TOMEM)
	{
		memcpy(mem+obj_addr, &noisesuppression_t62_config, sizeof(noisesuppression_t62_config));
		return 0;
	}

	error = mxt_write_block(client, obj_addr, obj_size,
                (u8 *)&noisesuppression_t62_config);
	if (error < 0) {
		dev_err(&client->dev, "[TSP] noisesuppression_t62_config error "
				"(%s, %d)\n", __func__, __LINE__);
		return -EIO;
	}

	return 0;
}
EXPORT_SYMBOL(mxt336s_noisesuppression_t62_config);

void mxt_config_get_runmode_registers(struct mxt_runmode_registers_t *regs)
{
	memcpy(regs, &runmode_regs,
			sizeof(struct mxt_runmode_registers_t));
}

int mxt_config_save_runmode(struct mxt_data *mxt)
{
	int ret = 0;
	int retry = 3;
	u8 t38_buff[64] = {0, };
	u16 t38_addr;
	struct i2c_client *client = mxt->client;

	t38_addr = MXT_BASE_ADDR(MXT_USER_INFO_T38);
retry_runmode:
	ret = mxt_get_object_values(mxt, MXT_GEN_ACQUIRECONFIG_T8);
	/* ret |= mxt_get_object_values(mxt, MXT_TOUCH_MULTITOUCHSCREEN_T9); */
	/* ret |= mxt_get_object_values(mxt, MXT_PROCI_TOUCHSUPPRESSION_T42); */
	ret |= mxt_read_block(client, t38_addr + MXT_ADR_T38_USER1,
			4, &t38_buff[MXT_ADR_T38_USER1]);
	ret |= mxt_read_block(client, t38_addr + MXT_ADR_T38_USER25,
			10, &t38_buff[MXT_ADR_T38_USER25]);

	if (ret == 0) {
		/* T8 */
		/* The below registers will be reloaded after calibration */
		runmode_regs.t8_atchcalst = acquisition_config_mxt336s.atchcalst;
		runmode_regs.t8_atchcalsthr = acquisition_config_mxt336s.atchcalsthr;
		runmode_regs.t8_atchfrccalthr =
			acquisition_config_mxt336s.atchcalfrcthr;
		runmode_regs.t8_atchfrccalratio =
			acquisition_config_mxt336s.atchcalfrcratio;

		runmode_regs.t38_atchcalst = t38_buff[MXT_ADR_T38_USER1];
		runmode_regs.t38_atchcalsthr = t38_buff[MXT_ADR_T38_USER2];
		runmode_regs.t38_atchfrccalthr = t38_buff[MXT_ADR_T38_USER3];
		runmode_regs.t38_atchfrccalratio = t38_buff[MXT_ADR_T38_USER4];
		runmode_regs.t38_palm_check_flag = t38_buff[MXT_ADR_T38_USER26];
		memcpy(&runmode_regs.t38_palm_param,
				&t38_buff[MXT_ADR_T38_USER27], 4);
		runmode_regs.t38_cal_thr = t38_buff[MXT_ADR_T38_USER32];
		runmode_regs.t38_num_of_antitouch = t38_buff[MXT_ADR_T38_USER33];
		runmode_regs.t38_supp_ops = t38_buff[MXT_ADR_T38_USER34];
	} else {
		if (retry--) {
			dev_warn(&client->dev,
					"%s() retry:%d\n", __func__, retry);
			goto retry_runmode;
		} else {
			dev_warn(&client->dev, "%s() failed to"
					"get runmode configs.\n"
					"set the default value\n", __func__);
			runmode_regs.t8_atchcalst = t8_atchcalst[touch_type];
			runmode_regs.t8_atchcalsthr = t8_atchcalsthr[touch_type];
			runmode_regs.t8_atchfrccalthr = t8_atchcalfrcthr[touch_type];
			runmode_regs.t8_atchfrccalratio = t8_atchcalfrcratio[touch_type];

			runmode_regs.t38_palm_check_flag = 1;
			/* 2012_0510 : ATMEL */
			runmode_regs.t38_palm_param[0] = 100;
			runmode_regs.t38_palm_param[1] = 90;
			runmode_regs.t38_palm_param[2] = 100;
			runmode_regs.t38_palm_param[3] = 10;
			/* 2012_0510 : ATEML */
			runmode_regs.t38_cal_thr = 10;
			runmode_regs.t38_num_of_antitouch = 2;

			runmode_regs.t38_atchcalst = t8_atchcalst[touch_type];
			runmode_regs.t38_atchcalsthr = t8_atchcalsthr[touch_type];
			runmode_regs.t38_atchfrccalthr = t8_atchcalfrcthr[touch_type];
			runmode_regs.t38_atchfrccalratio = t8_atchcalfrcratio[touch_type];

			/* 2012_0521 :
			 *  - kykim : 5sec = 5000ms */
			runmode_regs.t38_supp_ops = 81;
		}
	}

	return ret;
}

void qt_mem_clear(struct mxt_data *mxt)
{
	uint16_t object_address;
	uint8_t tmp[255] = {0,};
	struct i2c_client *client = mxt->client;
	uint8_t i, j, k;
	uint8_t object_size;
	uint8_t instance;

	for(i=0; i<mxt->device_info.num_objs; i++)
	{
		/* Get object address of instance 0 */
		object_address = mxt->object_table[i].chip_addr;
		if(object_address != 0)
		{
			object_size = mxt->object_table[i].size;
			instance = mxt->object_table[i].instances;
			
			for(j=0; j<instance; j++)
			{
				mxt_write_block(client, object_address, object_size, tmp);
				object_address += object_size;
			}
		}
	}
}

int mxt_userdata_config_1189T(struct mxt_data *mxt)
{
	struct i2c_client *client = mxt->client;
	u16 obj_addr, obj_size;
	int error;

	obj_addr = MXT_BASE_ADDR(MXT_USER_INFO_T38);
	obj_size = MXT_GET_SIZE(MXT_USER_INFO_T38);
	error = mxt_write_block(client, obj_addr, obj_size, (u8 *)cfg_1189T_T38);
	if (error < 0) {
		dev_err(&client->dev, "[TSP] mxt_write_block failed! "
				"(%s, %d)\n", __func__, __LINE__);
		return -EIO;
	}

	return 0;
}

int mxt_config_write_1189T(struct mxt_data *mxt, u8 *mem, int which, unsigned int touch_type)
{
	struct i2c_client *client = mxt->client;

	u16 obj_addr, obj_size, obj_num;
	int error;


	if((which == TOMEM) && (mem == NULL))
	{
		return -EINVAL;
	}

	obj_addr = MXT_BASE_ADDR(MXT_GEN_POWERCONFIG_T7);
	obj_size = MXT_GET_SIZE(MXT_GEN_POWERCONFIG_T7);
	if(which == TOMEM)
	{
		memcpy(mem+obj_addr, cfg_1189T_T7, sizeof(cfg_1189T_T7));
	}
	else
	{
		error = mxt_write_block(client, obj_addr, obj_size, (u8 *)cfg_1189T_T7);
		if (error < 0) {
			dev_err(&client->dev, "[TSP] mxt_write_block failed! "
					"(%s, %d)\n", __func__, __LINE__);
			return -EIO;
		}
	}

	obj_addr = MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8);
	obj_size = MXT_GET_SIZE(MXT_GEN_ACQUIRECONFIG_T8);
	if(which == TOMEM)
	{
		memcpy(mem+obj_addr, cfg_1189T_T8, sizeof(cfg_1189T_T8));
	}
	else
	{
		error = mxt_write_block(client, obj_addr, obj_size, (u8 *)cfg_1189T_T8);
		if (error < 0) {
			dev_err(&client->dev, "[TSP] mxt_write_block failed! "
					"(%s, %d)\n", __func__, __LINE__);
			return -EIO;
		}
	}

#if defined(INCLUDE_LCD_TOUCHKEY)
    obj_addr = MXT_BASE_ADDR(MXT_TOUCH_KEYARRAY_T15);
    obj_size = MXT_GET_SIZE(MXT_TOUCH_KEYARRAY_T15);
    if(which == TOMEM)
    {
        memcpy(mem+obj_addr, cfg_1189T_T15, sizeof(cfg_1189T_T15));
    }
    else
    {
        error = mxt_write_block(client, obj_addr, obj_size, (u8 *)cfg_1189T_T15);
        if (error < 0) {
            dev_err(&client->dev, "[TSP] mxt_write_block failed! "
                    "(%s, %d)\n", __func__, __LINE__);
            return -EIO;
        }
    }
#endif
	obj_addr = MXT_BASE_ADDR(MXT_PROCI_TOUCHSUPPRESSION_T42);
	obj_size = MXT_GET_SIZE(MXT_PROCI_TOUCHSUPPRESSION_T42);
	if(which == TOMEM)
	{
		memcpy(mem+obj_addr, cfg_1189T_T42, sizeof(cfg_1189T_T42));
	}
	else
	{
		error = mxt_write_block(client, obj_addr, obj_size, (u8 *)cfg_1189T_T42);
		if (error < 0) {
			dev_err(&client->dev, "[TSP] mxt_write_block failed! "
					"(%s, %d)\n", __func__, __LINE__);
			return -EIO;
		}
	}

	obj_addr = MXT_BASE_ADDR(MXT_SPT_CTECONFIG_T46);
	obj_size = MXT_GET_SIZE(MXT_SPT_CTECONFIG_T46);
	if(which == TOMEM)
	{
		memcpy(mem+obj_addr, cfg_1189T_T46, sizeof(cfg_1189T_T46));
	}
	else
	{
		error = mxt_write_block(client, obj_addr, obj_size, (u8 *)cfg_1189T_T46);
		if (error < 0) {
			dev_err(&client->dev, "[TSP] mxt_write_block failed! "
					"(%s, %d)\n", __func__, __LINE__);
			return -EIO;
		}
	}

	obj_addr = MXT_BASE_ADDR(MXT_SHIELDLESS_T56);
	obj_size = MXT_GET_SIZE(MXT_SHIELDLESS_T56);
	if(which == TOMEM)
	{
		memcpy(mem+obj_addr, cfg_1189T_T56, sizeof(cfg_1189T_T56));
	}
	else
	{
		error = mxt_write_block(client, obj_addr, obj_size, (u8 *)cfg_1189T_T56);
		if (error < 0) {
			dev_err(&client->dev, "[TSP] mxt_write_block failed! "
					"(%s, %d)\n", __func__, __LINE__);
			return -EIO;
		}
	}
#if defined(INCLUDE_LCD_TOUCHKEY)
	obj_addr = MXT_BASE_ADDR(MXT_SPT_TIMER_T61);
        obj_size = MXT_GET_SIZE(MXT_SPT_TIMER_T61);
        if(which == TOMEM)
        {
                memcpy(mem+obj_addr, cfg_1189T_T61, sizeof(cfg_1189T_T61));
        }
        else
        {
                error = mxt_write_block(client, obj_addr, obj_size*3 , (u8 *)cfg_1189T_T61);
                if (error < 0) {
                        dev_err(&client->dev, "[TSP] mxt_write_block failed! "
                                        "(%s, %d)\n", __func__, __LINE__);
                        return -EIO;
                }
        }
#endif

	obj_addr = MXT_BASE_ADDR(65u);
	obj_size = MXT_GET_SIZE(65u);
	if(which == TOMEM)
	{
		memcpy(mem+obj_addr, cfg_1189T_T65, sizeof(cfg_1189T_T65));
	}
	else
	{
		error = mxt_write_block(client, obj_addr, obj_size, (u8 *)cfg_1189T_T65);
		if (error < 0) {
			dev_err(&client->dev, "[TSP] mxt_write_block failed! "
					"(%s, %d)\n", __func__, __LINE__);
			return -EIO;
		}
	}

#if defined(INCLUDE_LCD_TOUCHKEY) 
	obj_addr = MXT_BASE_ADDR(MXT_SPT_DYNAMICCONFIGURATIONCONTROLLER_T70);
        obj_size = MXT_GET_SIZE(MXT_SPT_DYNAMICCONFIGURATIONCONTROLLER_T70);
	if(which == TOMEM)
        {
                memcpy(mem+obj_addr, cfg_1189T_T70, sizeof(cfg_1189T_T70));
        }
        else
        {
                error = mxt_write_block(client, obj_addr, obj_size*7, (u8 *)cfg_1189T_T70);
                if (error < 0) {
                        dev_err(&client->dev, "[TSP] mxt_write_block failed! "
                                        "(%s, %d)\n", __func__, __LINE__);
                        return -EIO;
                }
        }

	

	obj_addr = MXT_BASE_ADDR(MXT_SPT_DYNAMICCONFIGURATIONCONTAINER_T71);
        obj_size = MXT_GET_SIZE(MXT_SPT_DYNAMICCONFIGURATIONCONTAINER_T71);
        if(which == TOMEM)
        {
                memcpy(mem+obj_addr, cfg_1189T_T71, sizeof(cfg_1189T_T71));
        }
        else
        {
                error = mxt_write_block(client, obj_addr, obj_size, (u8 *)cfg_1189T_T71);
                if (error < 0) {
                        dev_err(&client->dev, "[TSP] mxt_write_block failed! "
                                        "(%s, %d)\n", __func__, __LINE__);
                        return -EIO;
                }
        }

#endif
	obj_addr = MXT_BASE_ADDR(72u);
	obj_size = MXT_GET_SIZE(72u);
	if(which == TOMEM)
	{
		memcpy(mem+obj_addr, cfg_1189T_T72, sizeof(cfg_1189T_T72));
	}
	else
	{
		error = mxt_write_block(client, obj_addr, obj_size, (u8 *)cfg_1189T_T72);
		if (error < 0) {
			dev_err(&client->dev, "[TSP] mxt_write_block failed! "
					"(%s, %d)\n", __func__, __LINE__);
			return -EIO;
		}
	}

	obj_addr = MXT_BASE_ADDR(78u);
	obj_size = MXT_GET_SIZE(78u);
	if(which == TOMEM)
	{
		memcpy(mem+obj_addr, cfg_1189T_T78, sizeof(cfg_1189T_T78));
	}
	else
	{
		error = mxt_write_block(client, obj_addr, obj_size, (u8 *)cfg_1189T_T78);
		if (error < 0) {
			dev_err(&client->dev, "[TSP] mxt_write_block failed! "
					"(%s, %d)\n", __func__, __LINE__);
			return -EIO;
		}
	}

	obj_addr = MXT_BASE_ADDR(80u);
	obj_size = MXT_GET_SIZE(80u);
	if(which == TOMEM)
	{
		memcpy(mem+obj_addr, cfg_1189T_T80, sizeof(cfg_1189T_T80));
	}
	else
	{
		error = mxt_write_block(client, obj_addr, obj_size, (u8 *)cfg_1189T_T80);
		if (error < 0) {
			dev_err(&client->dev, "[TSP] mxt_write_block failed! "
					"(%s, %d)\n", __func__, __LINE__);
			return -EIO;
		}
	}

	obj_addr = MXT_BASE_ADDR(100u);
	obj_size = MXT_GET_SIZE(100u);
	if(touch_type == 1)
	{
		if(which == TOMEM)
		{
			memcpy(mem+obj_addr, cfg_1189T_T100_1, sizeof(cfg_1189T_T100_1));
		}
		else
		{
			error = mxt_write_block(client, obj_addr, obj_size, (u8 *)cfg_1189T_T100_1);
		}
	}
	else
	{
		if(which == TOMEM)
		{
			memcpy(mem+obj_addr, cfg_1189T_T100, sizeof(cfg_1189T_T100));
		}
		else
		{
			error = mxt_write_block(client, obj_addr, obj_size, (u8 *)cfg_1189T_T100);
		}
	}
	if (error < 0) {
		dev_err(&client->dev, "[TSP] mxt_write_block failed! "
				"(%s, %d)\n", __func__, __LINE__);
		return -EIO;
	}

	obj_addr = MXT_BASE_ADDR(104u);
	obj_size = MXT_GET_SIZE(104u);
	if(which == TOMEM)
	{
		memcpy(mem+obj_addr, cfg_1189T_T104, sizeof(cfg_1189T_T104));
	}
	else
	{
		error = mxt_write_block(client, obj_addr, obj_size, (u8 *)cfg_1189T_T104);
		if (error < 0) {
			dev_err(&client->dev, "[TSP] mxt_write_block failed! "
					"(%s, %d)\n", __func__, __LINE__);
			return -EIO;
		}
	}

	return 0;
}

int mxt_userdata_config_641T(struct mxt_data *mxt)
{
        struct i2c_client *client = mxt->client;
        u16 obj_addr, obj_size;
        int error;

        obj_addr = MXT_BASE_ADDR(MXT_USER_INFO_T38);
        obj_size = MXT_GET_SIZE(MXT_USER_INFO_T38);
        error = mxt_write_block(client, obj_addr, obj_size, (u8 *)cfg_641T_T38);
        if (error < 0) {
                dev_err(&client->dev, "[TSP] mxt_write_block failed! "
                                "(%s, %d)\n", __func__, __LINE__);
                return -EIO;
        }

        return 0;
}


int mxt_config_write_641T(struct mxt_data *mxt, u8 *mem, int which)
{
	struct i2c_client *client = mxt->client;

	u16 obj_addr, obj_size, i;
	int error;

	if((which == TOMEM) && (mem == NULL))
	{
		return -EINVAL;
	}

	obj_addr = MXT_BASE_ADDR(MXT_GEN_POWERCONFIG_T7);
	obj_size = MXT_GET_SIZE(MXT_GEN_POWERCONFIG_T7);
	if(which == TOMEM)
	{
		memcpy(mem+obj_addr, cfg_641T_T7, sizeof(cfg_641T_T7));
	}
	else
	{
		error = mxt_write_block(client, obj_addr, obj_size, (u8 *)cfg_641T_T7);
		if (error < 0) {
			dev_err(&client->dev, "[TSP] mxt_write_block failed! "
					"(%s, %d)\n", __func__, __LINE__);
			return -EIO;
		}
	}

	obj_addr = MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8);
	obj_size = MXT_GET_SIZE(MXT_GEN_ACQUIRECONFIG_T8);
	if(which == TOMEM)
	{
		memcpy(mem+obj_addr, cfg_641T_T8, sizeof(cfg_641T_T8));
	}
	else
	{
		error = mxt_write_block(client, obj_addr, obj_size, (u8 *)cfg_641T_T8);
		if (error < 0) {
			dev_err(&client->dev, "[TSP] mxt_write_block failed! "
					"(%s, %d)\n", __func__, __LINE__);
			return -EIO;
		}
	}

#if 0
	obj_addr = MXT_BASE_ADDR(24u);
	obj_size = MXT_GET_SIZE(24u);
	if(which == TOMEM)
	{
		memcpy(mem+obj_addr, cfg_641T_T24, sizeof(cfg_641T_T24));
	}
	else
	{
		error = mxt_write_block(client, obj_addr, obj_size, (u8 *)cfg_641T_T24);
		if (error < 0) {
			dev_err(&client->dev, "[TSP] mxt_write_block failed! "
					"(%s, %d)\n", __func__, __LINE__);
			return -EIO;
		}
	}
#endif

	obj_addr = MXT_BASE_ADDR(25u);
	obj_size = MXT_GET_SIZE(25u);
	if(which == TOMEM)
	{
		memcpy(mem+obj_addr, cfg_641T_T25, sizeof(cfg_641T_T25));
	}
	else
	{
		error = mxt_write_block(client, obj_addr, obj_size, (u8 *)cfg_641T_T25);
		if (error < 0) {
			dev_err(&client->dev, "[TSP] mxt_write_block failed! "
					"(%s, %d)\n", __func__, __LINE__);
			return -EIO;
		}
	}

#if 0
	obj_addr = MXT_BASE_ADDR(27u);
	obj_size = MXT_GET_SIZE(27u);
	if(which == TOMEM)
	{
		memcpy(mem+obj_addr, cfg_641T_T27, sizeof(cfg_641T_T27));
	}
	else
	{
		error = mxt_write_block(client, obj_addr, obj_size, (u8 *)cfg_641T_T27);
		if (error < 0) {
			dev_err(&client->dev, "[TSP] mxt_write_block failed! "
					"(%s, %d)\n", __func__, __LINE__);
			return -EIO;
		}
	}
#endif

	obj_addr = MXT_BASE_ADDR(37u);
	obj_size = MXT_GET_SIZE(37u);
	if(which == TOMEM)
	{
		memcpy(mem+obj_addr, cfg_641T_T37, sizeof(cfg_641T_T37));
	}
	else
	{
		error = mxt_write_block(client, obj_addr, obj_size, (u8 *)cfg_641T_T37);
		if (error < 0) {
			dev_err(&client->dev, "[TSP] mxt_write_block failed! "
					"(%s, %d)\n", __func__, __LINE__);
			return -EIO;
		}
	}

	obj_addr = MXT_BASE_ADDR(MXT_PROCI_TOUCHSUPPRESSION_T42);
	obj_size = MXT_GET_SIZE(MXT_PROCI_TOUCHSUPPRESSION_T42);
	if(which == TOMEM)
	{
		memcpy(mem+obj_addr, cfg_641T_T42, sizeof(cfg_641T_T42));
	}
	else
	{
		error = mxt_write_block(client, obj_addr, obj_size, (u8 *)cfg_641T_T42);
		if (error < 0) {
			dev_err(&client->dev, "[TSP] mxt_write_block failed! "
					"(%s, %d)\n", __func__, __LINE__);
			return -EIO;
		}
	}

	obj_addr = MXT_BASE_ADDR(MXT_SPT_CTECONFIG_T46);
	obj_size = MXT_GET_SIZE(MXT_SPT_CTECONFIG_T46);
	if(which == TOMEM)
	{
		memcpy(mem+obj_addr, cfg_641T_T46, sizeof(cfg_641T_T46));
	}
	else
	{
		error = mxt_write_block(client, obj_addr, obj_size, (u8 *)cfg_641T_T46);
		if (error < 0) {
			dev_err(&client->dev, "[TSP] mxt_write_block failed! "
					"(%s, %d)\n", __func__, __LINE__);
			return -EIO;
		}
	}

	obj_addr = MXT_BASE_ADDR(MXT_SHIELDLESS_T56);
	obj_size = MXT_GET_SIZE(MXT_SHIELDLESS_T56);
	if(which == TOMEM)
	{
		memcpy(mem+obj_addr, cfg_641T_T56, sizeof(cfg_641T_T56));
	}
	else
	{
		error = mxt_write_block(client, obj_addr, obj_size, (u8 *)cfg_641T_T56);
		if (error < 0) {
			dev_err(&client->dev, "[TSP] mxt_write_block failed! "
					"(%s, %d)\n", __func__, __LINE__);
			return -EIO;
		}
	}

	obj_addr = MXT_BASE_ADDR(65u);
        obj_size = MXT_GET_SIZE(65u);
        if(which == TOMEM)
        {
                for(i=0; i<=2; i++)
                {
                        memcpy(mem+obj_addr+(i*obj_size), cfg_641T_T65[i], sizeof(cfg_641T_T65[0]));
                }
        }
        else
        {
                for(i=0; i<=2; i++)
                {
                        error = mxt_write_block(client, obj_addr+(i*obj_size), obj_size, (u8 *)cfg_641T_T65[i]);
                        if (error < 0) {
                                dev_err(&client->dev, "[TSP] mxt_write_block failed! "
                                                "(%s, %d)\n", __func__, __LINE__);
                                return -EIO;
                        }
                }
        }

	obj_addr = MXT_BASE_ADDR(72u);
	obj_size = MXT_GET_SIZE(72u);
	if(which == TOMEM)
	{
		memcpy(mem+obj_addr, cfg_641T_T72, sizeof(cfg_641T_T72));
	}
	else
	{
		error = mxt_write_block(client, obj_addr, obj_size, (u8 *)cfg_641T_T72);
		if (error < 0) {
			dev_err(&client->dev, "[TSP] mxt_write_block failed! "
					"(%s, %d)\n", __func__, __LINE__);
			return -EIO;
		}
	}


	obj_addr = MXT_BASE_ADDR(78u);
	obj_size = MXT_GET_SIZE(78u);
	if(which == TOMEM)
	{
		memcpy(mem+obj_addr, cfg_641T_T78, sizeof(cfg_641T_T78));
	}
	else
	{
		error = mxt_write_block(client, obj_addr, obj_size, (u8 *)cfg_641T_T78);
		if (error < 0) {
			dev_err(&client->dev, "[TSP] mxt_write_block failed! "
					"(%s, %d)\n", __func__, __LINE__);
			return -EIO;
		}
	}

#if 0
	obj_addr = MXT_BASE_ADDR(80u);
	obj_size = MXT_GET_SIZE(80u);
	if(which == TOMEM)
	{
		memcpy(mem+obj_addr, cfg_641T_T80, sizeof(cfg_641T_T80));
	}
	else
	{
		error = mxt_write_block(client, obj_addr, obj_size, (u8 *)cfg_641T_T80);
		if (error < 0) {
			dev_err(&client->dev, "[TSP] mxt_write_block failed! "
					"(%s, %d)\n", __func__, __LINE__);
			return -EIO;
		}
	}
#endif

	obj_addr = MXT_BASE_ADDR(100u);
	obj_size = MXT_GET_SIZE(100u);
	if(which == TOMEM)
	{
		memcpy(mem+obj_addr, cfg_641T_T100, sizeof(cfg_641T_T100));
	}
	else
	{
		error = mxt_write_block(client, obj_addr, obj_size, (u8 *)cfg_641T_T100);
		if (error < 0) {
			dev_err(&client->dev, "[TSP] mxt_write_block failed! "
					"(%s, %d)\n", __func__, __LINE__);
			return -EIO;
		}
	}

#if 0
	obj_addr = MXT_BASE_ADDR(104u);
	obj_size = MXT_GET_SIZE(104u);
	if(which == TOMEM)
	{
		memcpy(mem+obj_addr, cfg_641T_T104, sizeof(cfg_641T_T104));
	}
	else
	{
		error = mxt_write_block(client, obj_addr, obj_size, (u8 *)cfg_641T_T104);
		if (error < 0) {
			dev_err(&client->dev, "[TSP] mxt_write_block failed! "
					"(%s, %d)\n", __func__, __LINE__);
			return -EIO;
		}
	}

	obj_addr = MXT_BASE_ADDR(109u);
	obj_size = MXT_GET_SIZE(109u);
	if(which == TOMEM)
	{
		memcpy(mem+obj_addr, cfg_641T_T109, sizeof(cfg_641T_T109));
	}
	else
	{
		error = mxt_write_block(client, obj_addr, obj_size, (u8 *)cfg_641T_T109);
		if (error < 0) {
			dev_err(&client->dev, "[TSP] mxt_write_block failed! "
					"(%s, %d)\n", __func__, __LINE__);
			return -EIO;
		}
	}

	obj_addr = MXT_BASE_ADDR(110u);
	obj_size = MXT_GET_SIZE(110u);
	if(which == TOMEM)
	{
		for(i=0; i<=8; i++)
		{
			memcpy(mem+obj_addr+i*obj_size, cfg_641T_T110[i], sizeof(cfg_641T_T110[0]));
		}
	}
	else
	{
		for(i=0; i<=8; i++)
		{
			error = mxt_write_block(client, obj_addr+i*obj_size, obj_size, (u8 *)cfg_641T_T110[i]);
			if (error < 0) {
				dev_err(&client->dev, "[TSP] mxt_write_block failed! "
						"(%s, %d)\n", __func__, __LINE__);
				return -EIO;
			}
		}
	}

	obj_addr = MXT_BASE_ADDR(111u);
	obj_size = MXT_GET_SIZE(111u);
	if(which == TOMEM)
	{
		for(i=0; i<=2; i++)
		{
			memcpy(mem+obj_addr+i*obj_size, cfg_641T_T111[i], sizeof(cfg_641T_T111[0]));
		}
	}
	else
	{
		for(i=0; i<=2; i++)
		{
			error = mxt_write_block(client, obj_addr+i*obj_size, obj_size, (u8 *)cfg_641T_T111[i]);
			if (error < 0) {
				dev_err(&client->dev, "[TSP] mxt_write_block failed! "
						"(%s, %d)\n", __func__, __LINE__);
				return -EIO;
			}
		}
	}

	obj_addr = MXT_BASE_ADDR(113u);
	obj_size = MXT_GET_SIZE(113u);
	if(which == TOMEM)
	{
		memcpy(mem+obj_addr, cfg_641T_T113, sizeof(cfg_641T_T113));
	}
	else
	{
		error = mxt_write_block(client, obj_addr, obj_size, (u8 *)cfg_641T_T113);
		if (error < 0) {
			dev_err(&client->dev, "[TSP] mxt_write_block failed! "
					"(%s, %d)\n", __func__, __LINE__);
			return -EIO;
		}
	}
#endif

	return 0;
}


int mxt_userdata_config_641TD(struct mxt_data *mxt)
{
        struct i2c_client *client = mxt->client;
        u16 obj_addr, obj_size;
        int error;

        obj_addr = MXT_BASE_ADDR(MXT_USER_INFO_T38);
        obj_size = MXT_GET_SIZE(MXT_USER_INFO_T38);
        error = mxt_write_block(client, obj_addr, obj_size, (u8 *)cfg_641TD_T38);
        if (error < 0) {
                dev_err(&client->dev, "[TSP] mxt_write_block failed! "
                                "(%s, %d)\n", __func__, __LINE__);
                return -EIO;
        }

        return 0;
}


int mxt_config_write_641TD(struct mxt_data *mxt, u8 *mem, int which)
{
	struct i2c_client *client = mxt->client;

	u16 obj_addr, obj_size, i;
	int error;

	if((which == TOMEM) && (mem == NULL))
	{
		return -EINVAL;
	}

	obj_addr = MXT_BASE_ADDR(MXT_GEN_POWERCONFIG_T7);
	obj_size = MXT_GET_SIZE(MXT_GEN_POWERCONFIG_T7);
	if(which == TOMEM)
	{
		memcpy(mem+obj_addr, cfg_641TD_T7, sizeof(cfg_641TD_T7));
	}
	else
	{
		error = mxt_write_block(client, obj_addr, obj_size, (u8 *)cfg_641TD_T7);
		if (error < 0) {
			dev_err(&client->dev, "[TSP] mxt_write_block failed! "
					"(%s, %d)\n", __func__, __LINE__);
			return -EIO;
		}
	}

	obj_addr = MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8);
	obj_size = MXT_GET_SIZE(MXT_GEN_ACQUIRECONFIG_T8);
	if(which == TOMEM)
	{
		memcpy(mem+obj_addr, cfg_641TD_T8, sizeof(cfg_641TD_T8));
	}
	else
	{
		error = mxt_write_block(client, obj_addr, obj_size, (u8 *)cfg_641TD_T8);
		if (error < 0) {
			dev_err(&client->dev, "[TSP] mxt_write_block failed! "
					"(%s, %d)\n", __func__, __LINE__);
			return -EIO;
		}
	}

#if 0
	obj_addr = MXT_BASE_ADDR(24u);
	obj_size = MXT_GET_SIZE(24u);
	if(which == TOMEM)
	{
		memcpy(mem+obj_addr, cfg_641TD_T24, sizeof(cfg_641TD_T24));
	}
	else
	{
		error = mxt_write_block(client, obj_addr, obj_size, (u8 *)cfg_641TD_T24);
		if (error < 0) {
			dev_err(&client->dev, "[TSP] mxt_write_block failed! "
					"(%s, %d)\n", __func__, __LINE__);
			return -EIO;
		}
	}
#endif

	obj_addr = MXT_BASE_ADDR(25u);
	obj_size = MXT_GET_SIZE(25u);
	if(which == TOMEM)
	{
		memcpy(mem+obj_addr, cfg_641TD_T25, sizeof(cfg_641TD_T25));
	}
	else
	{
		error = mxt_write_block(client, obj_addr, obj_size, (u8 *)cfg_641TD_T25);
		if (error < 0) {
			dev_err(&client->dev, "[TSP] mxt_write_block failed! "
					"(%s, %d)\n", __func__, __LINE__);
			return -EIO;
		}
	}

#if 0
	obj_addr = MXT_BASE_ADDR(27u);
	obj_size = MXT_GET_SIZE(27u);
	if(which == TOMEM)
	{
		memcpy(mem+obj_addr, cfg_641TD_T27, sizeof(cfg_641TD_T27));
	}
	else
	{
		error = mxt_write_block(client, obj_addr, obj_size, (u8 *)cfg_641TD_T27);
		if (error < 0) {
			dev_err(&client->dev, "[TSP] mxt_write_block failed! "
					"(%s, %d)\n", __func__, __LINE__);
			return -EIO;
		}
	}
#endif

	obj_addr = MXT_BASE_ADDR(37u);
	obj_size = MXT_GET_SIZE(37u);
	if(which == TOMEM)
	{
		memcpy(mem+obj_addr, cfg_641TD_T37, sizeof(cfg_641TD_T37));
	}
	else
	{
		error = mxt_write_block(client, obj_addr, obj_size, (u8 *)cfg_641TD_T37);
		if (error < 0) {
			dev_err(&client->dev, "[TSP] mxt_write_block failed! "
					"(%s, %d)\n", __func__, __LINE__);
			return -EIO;
		}
	}

	obj_addr = MXT_BASE_ADDR(MXT_PROCI_TOUCHSUPPRESSION_T42);
	obj_size = MXT_GET_SIZE(MXT_PROCI_TOUCHSUPPRESSION_T42);
	if(which == TOMEM)
	{
		memcpy(mem+obj_addr, cfg_641TD_T42, sizeof(cfg_641TD_T42));
	}
	else
	{
		error = mxt_write_block(client, obj_addr, obj_size, (u8 *)cfg_641TD_T42);
		if (error < 0) {
			dev_err(&client->dev, "[TSP] mxt_write_block failed! "
					"(%s, %d)\n", __func__, __LINE__);
			return -EIO;
		}
	}

	obj_addr = MXT_BASE_ADDR(MXT_SPT_CTECONFIG_T46);
	obj_size = MXT_GET_SIZE(MXT_SPT_CTECONFIG_T46);
	if(which == TOMEM)
	{
		memcpy(mem+obj_addr, cfg_641TD_T46, sizeof(cfg_641TD_T46));
	}
	else
	{
		error = mxt_write_block(client, obj_addr, obj_size, (u8 *)cfg_641TD_T46);
		if (error < 0) {
			dev_err(&client->dev, "[TSP] mxt_write_block failed! "
					"(%s, %d)\n", __func__, __LINE__);
			return -EIO;
		}
	}

	obj_addr = MXT_BASE_ADDR(MXT_SHIELDLESS_T56);
	obj_size = MXT_GET_SIZE(MXT_SHIELDLESS_T56);
	if(which == TOMEM)
	{
		memcpy(mem+obj_addr, cfg_641TD_T56, sizeof(cfg_641TD_T56));
	}
	else
	{
		error = mxt_write_block(client, obj_addr, obj_size, (u8 *)cfg_641TD_T56);
		if (error < 0) {
			dev_err(&client->dev, "[TSP] mxt_write_block failed! "
					"(%s, %d)\n", __func__, __LINE__);
			return -EIO;
		}
	}

	obj_addr = MXT_BASE_ADDR(65u);
        obj_size = MXT_GET_SIZE(65u);
        if(which == TOMEM)
        {
                for(i=0; i<=2; i++)
                {
                        memcpy(mem+obj_addr+i*obj_size, cfg_641TD_T65[i], sizeof(cfg_641TD_T65[0]));
                }
        }
        else
        {
                for(i=0; i<=2; i++)
                {
                        error = mxt_write_block(client, obj_addr+i*obj_size, obj_size, (u8 *)cfg_641TD_T65[i]);
                        if (error < 0) {
                                dev_err(&client->dev, "[TSP] mxt_write_block failed! "
                                                "(%s, %d)\n", __func__, __LINE__);
                                return -EIO;
                        }
                }
        }

	obj_addr = MXT_BASE_ADDR(72u);
	obj_size = MXT_GET_SIZE(72u);
	if(which == TOMEM)
	{
		memcpy(mem+obj_addr, cfg_641TD_T72, sizeof(cfg_641TD_T72));
	}
	else
	{
		error = mxt_write_block(client, obj_addr, obj_size, (u8 *)cfg_641TD_T72);
		if (error < 0) {
			dev_err(&client->dev, "[TSP] mxt_write_block failed! "
					"(%s, %d)\n", __func__, __LINE__);
			return -EIO;
		}
	}

	obj_addr = MXT_BASE_ADDR(78u);
	obj_size = MXT_GET_SIZE(78u);
	if(which == TOMEM)
	{
		memcpy(mem+obj_addr, cfg_641TD_T78, sizeof(cfg_641TD_T78));
	}
	else
	{
		error = mxt_write_block(client, obj_addr, obj_size, (u8 *)cfg_641TD_T78);
		if (error < 0) {
			dev_err(&client->dev, "[TSP] mxt_write_block failed! "
					"(%s, %d)\n", __func__, __LINE__);
			return -EIO;
		}
	}

	obj_addr = MXT_BASE_ADDR(80u);
	obj_size = MXT_GET_SIZE(80u);
	if(which == TOMEM)
	{
		memcpy(mem+obj_addr, cfg_641TD_T80, sizeof(cfg_641TD_T80));
	}
	else
	{
		error = mxt_write_block(client, obj_addr, obj_size, (u8 *)cfg_641TD_T80);
		if (error < 0) {
			dev_err(&client->dev, "[TSP] mxt_write_block failed! "
					"(%s, %d)\n", __func__, __LINE__);
			return -EIO;
		}
	}


	obj_addr = MXT_BASE_ADDR(100u);
	obj_size = MXT_GET_SIZE(100u);
	if(which == TOMEM)
	{
		memcpy(mem+obj_addr, cfg_641TD_T100, sizeof(cfg_641TD_T100));
	}
	else
	{
		error = mxt_write_block(client, obj_addr, obj_size, (u8 *)cfg_641TD_T100);
		if (error < 0) {
			dev_err(&client->dev, "[TSP] mxt_write_block failed! "
					"(%s, %d)\n", __func__, __LINE__);
			return -EIO;
		}
	}

	obj_addr = MXT_BASE_ADDR(104u);
	obj_size = MXT_GET_SIZE(104u);
	if(which == TOMEM)
	{
		memcpy(mem+obj_addr, cfg_641TD_T104, sizeof(cfg_641TD_T104));
	}
	else
	{
		error = mxt_write_block(client, obj_addr, obj_size, (u8 *)cfg_641TD_T104);
		if (error < 0) {
			dev_err(&client->dev, "[TSP] mxt_write_block failed! "
					"(%s, %d)\n", __func__, __LINE__);
			return -EIO;
		}
	}

	obj_addr = MXT_BASE_ADDR(109u);
	obj_size = MXT_GET_SIZE(109u);
	if(which == TOMEM)
	{
		memcpy(mem+obj_addr, cfg_641TD_T109, sizeof(cfg_641TD_T109));
	}
	else
	{
		error = mxt_write_block(client, obj_addr, obj_size, (u8 *)cfg_641TD_T109);
		if (error < 0) {
			dev_err(&client->dev, "[TSP] mxt_write_block failed! "
					"(%s, %d)\n", __func__, __LINE__);
			return -EIO;
		}
	}

	obj_addr = MXT_BASE_ADDR(110u);
	obj_size = MXT_GET_SIZE(110u);
	if(which == TOMEM)
	{
		for(i=0; i<=8; i++)
		{
			memcpy(mem+obj_addr+i*obj_size, cfg_641TD_T110[i], sizeof(cfg_641TD_T110[0]));
		}
	}
	else
	{
		for(i=0; i<=8; i++)
		{
			error = mxt_write_block(client, obj_addr+(i*obj_size), obj_size, (u8 *)cfg_641TD_T110[i]);
			if (error < 0) {
				dev_err(&client->dev, "[TSP] mxt_write_block failed! "
						"(%s, %d)\n", __func__, __LINE__);
				return -EIO;
			}
		}
	}

	obj_addr = MXT_BASE_ADDR(111u);
	obj_size = MXT_GET_SIZE(111u);
	if(which == TOMEM)
	{
		for(i=0; i<=2; i++)
		{
			memcpy(mem+obj_addr+(i*obj_size), cfg_641TD_T111[i], sizeof(cfg_641TD_T111[0]));
		}
	}
	else
	{
		for(i=0; i<=2; i++)
		{
			error = mxt_write_block(client, obj_addr+i*obj_size, obj_size, (u8 *)cfg_641TD_T111[i]);
			if (error < 0) {
				dev_err(&client->dev, "[TSP] mxt_write_block failed! "
						"(%s, %d)\n", __func__, __LINE__);
				return -EIO;
			}
		}
	}

	obj_addr = MXT_BASE_ADDR(113u);
	obj_size = MXT_GET_SIZE(113u);
	if(which == TOMEM)
	{
		memcpy(mem+obj_addr, cfg_641TD_T113, sizeof(cfg_641TD_T113));
	}
	else
	{
		error = mxt_write_block(client, obj_addr, obj_size, (u8 *)cfg_641TD_T113);
		if (error < 0) {
			dev_err(&client->dev, "[TSP] mxt_write_block failed! "
					"(%s, %d)\n", __func__, __LINE__);
			return -EIO;
		}
	}
	
	obj_addr = MXT_BASE_ADDR(133u);
        obj_size = MXT_GET_SIZE(133u);
        if(which == TOMEM)
        {
                memcpy(mem+obj_addr, cfg_641TD_T133, sizeof(cfg_641TD_T133));
        }
        else
        {
                error = mxt_write_block(client, obj_addr, obj_size, (u8 *)cfg_641TD_T133);
                if (error < 0) {
                        dev_err(&client->dev, "[TSP] mxt_write_block failed! "
                                        "(%s, %d)\n", __func__, __LINE__);
                        return -EIO;
                }
        }

	return 0;
}


int mxt_config_settings(struct mxt_data *mxt, u8 *mem, int which, unsigned int touch_type)
{
	int version;

	version = (mxt->device_info.major * 10) + (mxt->device_info.minor);

	printk("%s : %s\n", __func__, which ? "TODEV" : "TOMEM");
	dev_dbg(&mxt->client->dev, "[TSP] mxt_config_settings");

	/* Clear whole configuration memory */
	if(which != TOMEM)
		qt_mem_clear(mxt);

	switch(touch_type){
		case 5:
			if (which != TOMEM)
			{
				if (mxt_userdata_config_1189T(mxt) < 0) /* T38 */
					return -1;
			}

		        if (mxt_config_write_1189T(mxt, mem, which, touch_type) < 0)            /* ALL */
		                return -1;
			break;
		case 6:
			if (which != TOMEM)
			{
				if (mxt_userdata_config_641T(mxt) < 0) /* T38 */
			                return -1;
			}

		        if (mxt_config_write_641T(mxt, mem, which) < 0)            /* ALL */
		                return -1;
			break;
		case 7:
			if (which != TOMEM)
			{
				if (mxt_userdata_config(mxt) < 0)       /* T38 */
			                return -1;
			}

		        if (mxt_power_config(mxt, mem, which) < 0)              /* T7 */
		                return -1;
	
		        if (mxt336s_acquisition_config(mxt, mem, which) < 0)    /* T8 */
				return -1;

		        if (mxt_multitouch_config(mxt, mem, which) < 0) /* T9 */
		                return -1;

		        if (mxt_touch_suppression_t42_config(mxt, mem, which) < 0) /* T42 */
		                return -1;

		        if (mxt_cte_t46_config(mxt, mem, which) < 0)               /* T46 */
		                return -1;

		        if (mxt336s_noisesuppression_t62_config(mxt, mem, which) < 0)               /* T62 */
		                return -1;

		        if (mxt_shieldless_t56_config(mxt, mem, which) < 0)
		                return -1;	
			break;
		case 8:
			if (which != TOMEM)
			{
	                        if (mxt_userdata_config_641TD(mxt) < 0) /* T38 */
	                                return -1;
			}

                        if (mxt_config_write_641TD(mxt, mem, which) < 0)            /* ALL */
                                return -1;
                        break;
		default :
			printk("%s Wrong touch type %d\n",__func__,touch_type);
			break;
	}

	return 0;
}

/*
* Bootloader functions
*/

static void bootloader_status(struct i2c_client *client, u8 value)
{
	u8 *str = NULL;

	switch (value) {
	case 0xC0:
		str = "WAITING_BOOTLOAD_CMD"; break;
	case 0x80:
		str = "WAITING_FRAME_DATA"; break;
	case 0x40:
		str = "APP_CRC_FAIL"; break;
	case 0x02:
		str = "FRAME_CRC_CHECK"; break;
	case 0x03:
		str = "FRAME_CRC_FAIL"; break;
	case 0x04:
		str = "FRAME_CRC_PASS"; break;
	default:
		str = "Unknown Status"; break;
	}

	dev_dbg(&client->dev, "[TSP] bootloader status: %s (0x%02X)\n",
			str, value);
}

static int mxt_wait_change_signal(struct mxt_data *mxt)
{
	int ret = 0;

	if (mxt->pdata->check_chg_platform_hw) {
		int retry = 0;

		do {
			/* wait 100ms */
			if (retry > 100)
				return -EIO;
			ret = mxt->pdata->check_chg_platform_hw(NULL);
			if (ret == 0)
				break;
			mdelay(1);
			dev_info(&mxt->client->dev, "wait CHG, ret = %d[%d]\n",
					ret, retry);
		} while (retry++);
	}

	return ret;
}

static int check_bootloader(struct mxt_data *mxt, unsigned int status)
{
	u8 val = 0;
	u16 retry = 0;
	struct i2c_client *client = mxt->client;

	msleep(10);  /* recommendation from ATMEL */

recheck:
	if (retry++ >= 10) {
		dev_err(&client->dev, "[TSP] failed to check the request\n");
		return -EINVAL;
	}

	if ((status == WAITING_BOOTLOAD_COMMAND) ||
			(status == FRAME_CRC_PASS)) {
		int ret;
		/* in this case, we should check CHG line */
		/* I think that msleep(10) is enough, but keep the datasheet */
		ret = mxt_wait_change_signal(mxt);
		if (ret < 0) {
			dev_err(&client->dev, "[TSP] CHG didn't go down\n");
			return -EIO;
		}
	}

	if (i2c_master_recv(client, &val, 1) != 1) {
		dev_err(&client->dev, "[TSP] i2c recv failed\n");
		return -EIO;
	}

	switch (status) {
	case WAITING_FRAME_DATA:
	case WAITING_BOOTLOAD_COMMAND:
		val &= ~BOOTLOAD_STATUS_MASK;
		bootloader_status(client, val);
		if (val == APP_CRC_FAIL) {
			dev_info(&client->dev, "[TSP] We've got a APP_CRC_FAIL,"
					" so try again (count=%d)\n", retry);
			goto recheck;
		}
		break;

	case FRAME_CRC_PASS:
		bootloader_status(client, val);
		/* this checks FRAME_CRC_CHECK and FRAME_CRC_PASS */
		if (val == FRAME_CRC_CHECK)
			goto recheck;
		break;

	default:
		return -EINVAL;
	}

	if (val != status) {
		dev_err(&client->dev, "[TSP] Invalid status: 0x%02X\n", val);
		return -EINVAL;
	}

	return 0;
}

static int unlock_bootloader(struct i2c_client *client)
{
	u8 cmd[2] = { 0};

	cmd[0] = 0xdc;  /* MXT_CMD_UNLOCK_BL_LSB */
	cmd[1] = 0xaa;  /*MXT_CMD_UNLOCK_BL_MSB */

	return i2c_master_send(client, cmd, 2);
}

void mxt_change_i2c_addr(struct mxt_data *mxt, bool to_boot)
{
	int i, size;
	u8 to, from;

	size = sizeof(mxt_i2c_addr_list) / sizeof(mxt_i2c_addr_list[0]);
	to = from = mxt->client->addr;

	for (i = 0; i < size; i++) {
		if (to_boot) {
			/* application to bootloader */
			if (from == mxt_i2c_addr_list[i].app) {
				to = mxt_i2c_addr_list[i].boot;
				break;
			}
		} else {
			/* bootloader to application */
			if (from == mxt_i2c_addr_list[i].boot) {
				to = mxt_i2c_addr_list[i].app;
				break;
			}
		}
	}

	if (likely(i < size)) {
		mxt->client->addr = to;
	} else {
		dev_info(&mxt->client->dev,
				"%s() invalid i2c slave addr : %02X\n",
				__func__, mxt->client->addr);
		/* set the default mx336s address */
		if (to_boot)
		{
			mxt->client->addr = to = 0x24;
	                if(get_oemtype())
			{
	                        if((LCD_VER == DAUDIOKK_LCD_OD_08_00_1280_720_OGS_Si_BOE) || (LCD_VER == DAUDIOKK_LCD_OI_08_00_1280_720_OGS_Si_BOE))
	                                mxt->client->addr = to = 0x25;
			}
		}
		else
		{
			mxt->client->addr = to = 0x4A;
			if(get_oemtype())
                        {
                                if((LCD_VER == DAUDIOKK_LCD_OD_08_00_1280_720_OGS_Si_BOE) || (LCD_VER == DAUDIOKK_LCD_OI_08_00_1280_720_OGS_Si_BOE))
                                        mxt->client->addr = to = 0x4B;
                        }
		}
	}
	dev_info(&mxt->client->dev,
			"[TSP] I2C address: 0x%02X --> 0x%02X\n", from, to);
}

/**
 * @author sjpark@cleinsoft
 * @date 2014/11/18
 * Release firmware structure 
 **/
int release_mxt_firmware(struct firmware *fw)
{
    if(fw==NULL)
        return -EINVAL;
    if((fw->size > 0) && fw->data)
        kfree(fw->data);
    kfree(fw);

    return 0;
}

/**
 * @author sjpark@cleinsoft
 * @date 2014/11/18
 * Read firmware packet frame data.
 **/
int mxt_read_firmware_fs(struct firmware *fw, const char *path, loff_t *pos)
{
	struct file *fd;
    int ret = 0, frame_size = 0;
    char frame_bytes[2]={0};
    mm_segment_t old_fs;       

    old_fs = get_fs();
    set_fs(get_ds());
    fd = filp_open(path, O_RDONLY, 0);

    if ((fd != NULL) && (fd >= 0)){
        if(vfs_llseek(fd, *pos, SEEK_SET) != *pos){
            pr_err("[TSP] FW file seeking err (%s)\n", __func__); 
            goto out;
        }
        if((ret = vfs_read(fd, frame_bytes, FWFRAME_INFO_LEN, pos)) != FWFRAME_INFO_LEN){
            pr_err("[TSP] Reading touch firmware frame size err  : %d\n", ret);
            goto out;
        }
		frame_size = (frame_bytes[0] << 8) | frame_bytes[1];
        if((frame_size >= fw->size) || (frame_size <= 0)){
            pr_err("[TSP] Touch firmware frame size err : %d\n", frame_size);
            goto out;
        }
        fw->data = kmalloc(frame_size+FWFRAME_INFO_LEN, GFP_KERNEL);
        if(fw->data == NULL){
            pr_err("[TSP] Frame memory alloc err (%s)\n", __func__); 
            ret = -ENOMEM;
            goto out;
        }
        if(vfs_llseek(fd, *pos, SEEK_SET) != *pos){
            pr_err("[TSP] FW file seeking err (%s)\n", __func__); 
            goto out;
        }
        ret = vfs_read(fd, (char*)fw->data, frame_size+FWFRAME_INFO_LEN, pos);
        if(ret<0){
            pr_err("[TSP] Reading touch firmware file err: %d\n", ret);
        }
    }
    else{
        pr_err("[TSP] Cannot open firmware file: %d\n", fd);
    }
out:
    if((fd != NULL) && (fd >= 0)) 
        filp_close(fd, NULL);

    set_fs(old_fs);

    return ret;
}

/**
 * @author sjpark@cleinsoft
 * @date 2014/11/18
 * Creating firmware object & get file size for updating
 **/
int mxt_init_firmware_fs(const struct firmware **firmware_p, const char *name, 
        const char* path, struct device *device)
{ 
	struct file* fd = NULL;
    int ret = 0, sz = 0;
    //unsigned char *data;
    struct firmware *firmware;
    mm_segment_t old_fs = KERNEL_DS;

    if(path==NULL)
        return -EINVAL;

    *firmware_p = firmware = kzalloc(sizeof(*firmware), GFP_KERNEL);
    if (firmware==NULL) {           
        dev_err(device, "[TSP] %s: kmalloc(struct firmware) failed\n", __func__);         
        ret = -ENOMEM;      
        goto out;              
    }

    old_fs = get_fs();         
    set_fs(get_ds());
    fd = filp_open(path, O_RDONLY, 0);
    if (!IS_ERR(fd)){
        sz = vfs_llseek(fd, 0L, SEEK_END);
        if(sz <= 0){
            ret = -EINVAL;
            goto out;
        }
        firmware->size = sz;
    }else{
        dev_err(device, "[TSP] Firmware file open failed: %s\n", path);
        ret = PTR_ERR(fd);
    }
out:
    if(!IS_ERR(fd))
        filp_close(fd, NULL);
    set_fs(old_fs);            

    return ret;
} 

int mxt_load_firmware(struct device *dev, const char *fn, const char* path, int flag)
{
	int ret;
	int frame_size;
	loff_t pos = 0;
	unsigned int retry;
	struct firmware *fw = NULL;
	struct mxt_data *mxt = dev_get_drvdata(dev);

	if (MXT336S_FIRM_IN_KERNEL == flag) {
		fw = kzalloc(sizeof(struct firmware), GFP_KERNEL);
		if (NULL == fw) {
			dev_err(dev, "[TSP] failed to kzalloc for fw\n");
			return -ENOMEM;
		}
		fw->data = firmware_latest;
		fw->size = sizeof(firmware_latest);
	} else if (MXT336S_FIRM_EXTERNAL == flag) {
		ret = request_firmware((const struct firmware **)&fw, fn, dev);
		if (ret < 0) {
			dev_err(dev, "[TSP] Unable to open firmware %s\n", fn);
			return -ENOMEM;
		}
	} 
    else if (MXT336S_FIRM_EXTERNAL_NEW == flag) {
        ret = mxt_init_firmware_fs((const struct firmware **)&fw, fn, path, dev);
		if (ret < 0) {
			dev_err(dev, "[TSP] Unable to load firmware %s\n", path);
            release_mxt_firmware(fw);
			return -ENOMEM;
		}
    }
    else{
		dev_err(dev, "unknown firmware flag\n");
        return -EINVAL;
    }

	/* set resets into bootloader mode */
	ret = sw_reset_chip(mxt, RESET_TO_BOOTLOADER);
	if (ret < 0) {
		dev_warn(dev, "[TSP] couldn't reset by soft(%d), "
				"try to reset by h/w\n", ret);
		hw_reset_chip(mxt);
	}
	msleep(250);

	/* change to slave address of bootloader */
	mxt_change_i2c_addr(mxt, true);
	ret = check_bootloader(mxt, WAITING_BOOTLOAD_COMMAND);
	if (ret < 0) {
		dev_err(dev, "[TSP] ... Waiting bootloader command: Failed");
		goto err_fw;
	}

	/* unlock bootloader */
	unlock_bootloader(mxt->client);
	msleep(200);

	dev_info(dev, "Updating progress ( %d bytes)\n", fw->size);

	while (pos < fw->size) {
		retry = 0;

		ret = check_bootloader(mxt, WAITING_FRAME_DATA);
		if (ret < 0) {
			dev_err(dev, "... Waiting frame data: Failed\n");
			goto err_fw;
		}

        if (MXT336S_FIRM_EXTERNAL_NEW == flag) {
            frame_size = mxt_read_firmware_fs(fw, path, &pos);
        }else{
            frame_size = ((*(fw->data + pos) << 8) | *(fw->data + pos + 1));
        }
        if(frame_size <= 0){
            dev_err(dev, "[TSP] Read firmware frame data failed : %d\n", frame_size);
            goto err_fw;
        }

		/* We should add 2 at frame size as the the firmware data is not
		* included the CRC bytes.
		*/
        if (MXT336S_FIRM_EXTERNAL_NEW != flag)
            frame_size += 2;

		if ((pos + frame_size) > fw->size) {
			dev_err(dev, "[TSP] invalid file, file size:%d, "
					"pos : %d, frame_size : %d\n",
					fw->size, (int)pos, frame_size);
			ret = -EINVAL;
			goto err_fw;
		}

		/* write one frame to device */
try_to_resend_the_last_frame:
        if (MXT336S_FIRM_EXTERNAL_NEW == flag)
            i2c_master_send(mxt->client,(u8 *)(fw->data), frame_size);
        else
            i2c_master_send(mxt->client,(u8 *)(fw->data+pos), frame_size);

	#if defined(CONFIG_DISASSEMBLED_MONITOR)
		udelay(100);
	#endif

		ret = check_bootloader(mxt, FRAME_CRC_PASS);
		if (ret < 0) {
			if (++retry < 10) {
				/* recommendation from ATMEL */
				check_bootloader(mxt, WAITING_FRAME_DATA);
				dev_info(dev, "[TSP] We've got a "
						"FRAME_CRC_FAIL, so try again "
						"up to 10 times (count=%d)\n",
						retry);
				goto try_to_resend_the_last_frame;
			}
			dev_err(dev, "... CRC on the frame failed after "
					"10 trials!\n");
			goto err_fw;
		}

		pos += frame_size;

		dev_info(dev, "%zd / %zd (bytes) updated...", (int)pos, fw->size);
        if (MXT336S_FIRM_EXTERNAL_NEW == flag){
            if(fw->data){
                kfree(fw->data);
                fw->data = NULL;
            }
        }
	}
	dev_info(dev, "\n[TSP] Updating firmware completed!\n");
	dev_info(dev, "[TSP] note: You may need to reset this target.\n");

err_fw:
	/* change to slave address of application */
	mxt_change_i2c_addr(mxt, false);
	if (MXT336S_FIRM_IN_KERNEL == flag) 
		kfree(fw);
	else if (MXT336S_FIRM_EXTERNAL == flag)
		release_firmware(fw);
    else if (MXT336S_FIRM_EXTERNAL_NEW == flag)
        release_mxt_firmware(fw);

	return ret;
}

int mxt_load_registers(struct mxt_data *mxt, const char *buf, int size)
{
	int ret = -1;
	char *dup = NULL;
	struct mxt_config_t mxt_cfg;
	int version;

	dup = kstrdup(buf, GFP_KERNEL);
	if (!dup) {
		dev_err(&mxt->client->dev, "failed to duplicate string\n");
		return ret;
	}

	version = (mxt->device_info.major * 10) + mxt->device_info.minor;
	mxt_cfg.version = version;

	ret = request_parse(&mxt->client->dev,
			dup, &mxt_cfg, mxt->reg_update.verbose);
	if (ret == 0) {
		int error;
		u16 obj_addr = 0, obj_size = 0;
		u8 *data = NULL;

		data = (u8 *)&mxt_cfg.config;

		if (data && (mxt_cfg.obj_info.object_num != 0)) {
			obj_addr = get_object_address(
					mxt_cfg.obj_info.object_num,
					mxt_cfg.obj_info.instance_num,
					mxt->object_table,
					mxt->device_info.num_objs,
					mxt->object_link);
			obj_size = MXT_GET_SIZE(mxt_cfg.obj_info.object_num);
			error = mxt_write_block(mxt->client,
					obj_addr, obj_size, data);
			if (error < 0) {
				dev_err(&mxt->client->dev, "[TSP] "
						"mxt_write_block failed! "
						"(%s, %d)\n",
						__func__, __LINE__);
				ret = -1;
			}
		}
	}

	kfree(dup);
	return ret;
}
