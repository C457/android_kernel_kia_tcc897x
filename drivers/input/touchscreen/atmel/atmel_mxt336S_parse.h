#ifndef ATMEL_MXT336S_PARSE_H_
#define ATMEL_MXT336S_PARSE_H_

#include <linux/types.h>
#include <linux/device.h>
#include "atmel_mxt336S_cfg.h"

/* \brief parse the atmel generated file
 *
 * parse only one section block string
 *
 * example)
 *  [TOUCH_MULTITOUCHSCREEN_T9 INSTANCE 0] --> section
 *  CTRL=139 -->data
 *  XORIGIN=0
 *  YORIGIN=0
 *  XSIZE=18
 *  YSIZE=30
 *  AKSCFG=0
 *  ..
 */

/* \brief structure for atmel multitouch object infomation
 *
 * @param name object name for human readable
 * @param object_num object number
 * @param instance_num object's instance num
 */
struct mxt_object_info_t {
	char *name;
	int object_num;
	int instance_num;
};

/* \brief atmel multitouch configuration structure
 *
 * @param for detail, see the manual
 */
struct mxt_config_t {
	union {
		/* datasource t53 */
		spt_userdata_t38_t			userdata_t38;
		gen_powerconfig_t7_config_t		power_t7;
		gen_acquisitionconfig_t8_config_t	acquisition_t8;
		touch_multitouchscreen_t9_config_t	touchscreen_t9;
		touch_multitouchscreen_t9_config_t	touchscreen1_t9;
		touch_keyarray_t15_config_t		keyarray_t15;
		touch_keyarray_t15_config_t		keyarray1_t15;
		spt_comcconfig_t18_config_t		comc_t18;
		spt_gpiopwm_t19_config_t		gpiopwm_t19;
		proci_onetouchgestureprocessor_t24_config_t onegesture_t24;
		proci_onetouchgestureprocessor_t24_config_t onegesture1_t24;
		spt_selftest_t25_config_t		selftest_t25;
		/* multi-touch firmware 2.1
		 * T25 self test */
		spt_selftest_t25_ver2_config_t		selftest_ver2_t25;
		proci_twotouchgestureprocessor_t27_config_t twogesture_t27;
		proci_twotouchgestureprocessor_t27_config_t twogesture1_t27;
		proci_gripsuppression_t40_config_t	gripsuppression_t40;
		proci_gripsuppression_t40_config_t	gripsuppression1_t40;
		proci_touchsuppression_t42_config_t	touchsuppression_t42;
		proci_touchsuppression_t42_config_t	touchsuppression1_t42;
		/* multi-touch firmware 2.1
		 * T42 Touch suppression */
		proci_tsuppression_t42_ver2_config_t	tsuppression_ver2_t42;
		proci_tsuppression_t42_ver2_config_t	tsuppression1_ver2_t42;
		spt_cteconfig_t46_config_t		cteconfig_t46;
		/* multi-touch firmware 2.1
		 * T46 CTE Configuration */
		spt_cteconfig_t46_ver2_config_t		cteconfig_ver2_t46;
		proci_stylus_t47_config_t		stylus_t47;
		proci_stylus_t47_config_t		stylus1_t47;
		procg_noisesuppression_t48_config_t	noisesuppression_t48;
		/* multi-touch firmware 2.1
		 * T48 noise suppression */
		procg_nsuppression_t48_ver2_config_t	nsuppression_ver2_t48;
		touch_proximity_t52_config_t		proximity_t52;
		touch_proximity_t52_config_t		proximity1_t52;
		proci_adaptivethreshold_t55_config_t	adaptive_threshold_t55;
		proci_adaptivethreshold_t55_config_t	adaptive_threshold1_t55;
		proci_shieldless_t56_config_t		shieldless_t56;
		proci_extratouchscreendata_t57_config_t	exttouchscreendata_t57;
		proci_extratouchscreendata_t57_config_t	exttouchscreendata1_t57;
	} config;

	struct mxt_object_info_t obj_info;
	int version;
};

/* \breif request to parse the text which is generated by atmel program
 *
 * this function parse only one-section not whole text
 *
 * @dev struct device pointer for this device
 * @param data one-section string
 * @param struct mxt_config_t pointer
 * @param verbose if ture, print the config info
 * @return if success return 0, otherwise < 0
 */
extern int request_parse(struct device *dev,
		char *data, struct mxt_config_t *pconfig, bool verbose);

/* if you want to see the parsing sequence, recomment the below definition */
/* #define ENABLE_PRINT_DEBUG */

#ifdef ENABLE_PRINT_DEBUG
#define print_debug(fmt, ...)	pr_info(fmt, ##__VA_ARGS__)
#else
#define print_debug(fmt, ...)	{}
#endif

#endif
