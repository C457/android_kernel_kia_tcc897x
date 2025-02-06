#include <linux/string.h>
#include <linux/device.h>
#include "atmel_mxt336S.h"
#include "atmel_mxt336S_parse.h"

enum {
	COM_AND_VAL_RET = 0,
	COM_AND_VAL_NEXT = 1,
	COM_AND_VAL_ERR = -1,
};

enum {
	GET_DATA_ERR_NONE = 0,
	GET_DATA_ERR_SECTION,
	GET_DATA_ERR_EOF,
	GET_DATA_ERR_OVERFLOW,
	GET_DATA_ERR_INPARAM,
};

const struct mxt_object_info_t object_info[] = {
	/* {"[GEN_DATASOURCE_T53 INSTANCE 0]", MXT_DATA_SOURCE_T53, 0}, */
	{"[SPT_USERDATA_T38 INSTANCE 0]", MXT_USER_INFO_T38, 0},
	{"[GEN_POWERCONFIG_T7 INSTANCE 0]", MXT_GEN_POWERCONFIG_T7, 0},
	{"[GEN_ACQUISITIONCONFIG_T8 INSTANCE 0]", MXT_GEN_ACQUIRECONFIG_T8, 0},
	{"[TOUCH_MULTITOUCHSCREEN_T9 INSTANCE 0]",
		MXT_TOUCH_MULTITOUCHSCREEN_T9, 0},
	{"[TOUCH_MULTITOUCHSCREEN_T9 INSTANCE 1]",
		MXT_TOUCH_MULTITOUCHSCREEN_T9, 1},
	{"[TOUCH_KEYARRAY_T15 INSTANCE 0]", MXT_TOUCH_KEYARRAY_T15, 0},
	{"[TOUCH_KEYARRAY_T15 INSTANCE 1]", MXT_TOUCH_KEYARRAY_T15, 1},
	{"[SPT_COMMSCONFIG_T18 INSTANCE 0]", MXT_SPT_COMMSCONFIG_T18, 0},
	{"[SPT_GPIOPWM_T19 INSTANCE 0]", MXT_SPT_GPIOPWM_T19, 0},
	{"[PROCI_ONETOUCHGESTUREPROCESSOR_T24 INSTANCE 0]",
		MXT_PROCI_ONETOUCHGESTUREPROCESSOR_T24, 0},
	{"[PROCI_ONETOUCHGESTUREPROCESSOR_T24 INSTANCE 1]",
		MXT_PROCI_ONETOUCHGESTUREPROCESSOR_T24, 1},
	{"[SPT_SELFTEST_T25 INSTANCE 0]", MXT_SPT_SELFTEST_T25, 0},
	{"[PROCI_TWOTOUCHGESTUREPROCESSOR_T27 INSTANCE 0]",
		MXT_PROCI_TWOTOUCHGESTUREPROCESSOR_T27, 0},
	{"[PROCI_TWOTOUCHGESTUREPROCESSOR_T27 INSTANCE 1]",
		MXT_PROCI_TWOTOUCHGESTUREPROCESSOR_T27, 1},
	{"[PROCI_GRIPSUPPRESSION_T40 INSTANCE 0]",
		MXT_PROCI_GRIPSUPPRESSION_T40, 0},
	{"[PROCI_GRIPSUPPRESSION_T40 INSTANCE 1]",
		MXT_PROCI_GRIPSUPPRESSION_T40, 1},
	{"[PROCI_TOUCHSUPPRESSION_T42 INSTANCE 0]",
		MXT_PROCI_TOUCHSUPPRESSION_T42, 0},
	{"[PROCI_TOUCHSUPPRESSION_T42 INSTANCE 1]",
		MXT_PROCI_TOUCHSUPPRESSION_T42, 1},
	/* {"[SPT_DIGITIZER_T43 INSTANCE 0]", MXT_HID_CONFIG_T43, 0}, */
	{"[SPT_CTECONFIG_T46 INSTANCE 0]", MXT_SPT_CTECONFIG_T46, 0},
	{"[PROCI_STYLUS_T47 INSTANCE 0]", MXT_PROCI_STYLUS_T47, 0},
	{"[PROCI_STYLUS_T47 INSTANCE 1]", MXT_PROCI_STYLUS_T47, 1},
	{"[PROCG_NOISESUPPRESSION_T48 INSTANCE 0]",
		MXT_PROCG_NOISESUPPRESSION_T48, 0},
	{"[TOUCH_PROXKEY_T52 INSTANCE 0]", MXT_TOUCH_PROXIMITY_T52, 0},
	{"[TOUCH_PROXKEY_T52 INSTANCE 1]", MXT_TOUCH_PROXIMITY_T52, 1},
	{"[PROCI_ADAPTIVE_THRESHOLD_T55 INSTANCE 0]",
		MXT_ADAPTIVE_THRESHOLD_T55, 0},
	{"[PROCI_ADAPTIVE_THRESHOLD_T55 INSTANCE 1]",
		MXT_ADAPTIVE_THRESHOLD_T55, 1},
	{"[PROCI_SHIELDLESS_T56 INSTANCE_0]", MXT_SHIELDLESS_T56},
	{"[PROCI_EXTRA_TOUCH_SCREEN_DATA_T57 INSTANCE 0]",
		MXT_EXTRA_TOUCH_SCREEN_DATA_T57, 0},
	{"[PROCI_EXTRA_TOUCH_SCREEN_DATA_T57 INSTANCE 1]",
		MXT_EXTRA_TOUCH_SCREEN_DATA_T57, 1},
};

/**** function declaration ****/

/* \brief compare @org @cmp and get value
 *
 * @param org string
 * @param cmp compare string
 * @param the res result value
 * @return return status
 */
static int compare_and_getvalue(const char *org, const char *cmp, int *res);

/* \brief trim the whitespace(\r or \n) at the end of string
 *
 * @param str string
 */
static void mxt_trim_endws(char *str);

/* \brief check whether @str is a section or not
 *
 * @param str string
 * @return : if this string is a section return 1, otherwise 0
 */
static int mxt_check_section(char *str);

/* \brief get section's data
 *
 * @param input_string the left whole string
 * @param string string repository
 * @param str_size string size
 * @return
 *	if success return GET_DATA_ERR_NONE,
 *	otherwise
 *		GET_DATA_ERR_SECTION,
 *		GET_DATA_ERR_EOF,
 *		GET_DATA_ERR_OVERFLOW,
 */
static int mxt_get_data(char **input_string, char *mxt_string, int str_size);

/* \brief print result
 *
 * @param pconfig result of paring
 */
static void print_result(struct mxt_config_t *pconfig);

/* \brief parse one section data
 *
 * @param dev struct device pointer
 * @param string one-section string
 * @param pconfig struct mxt_confit_t pointer for saving result
 * @return if success return 0, otherwise < 0
 */
static int parse_object_data(struct device *dev,
		char *mxt_string, struct mxt_config_t *pconfig);

/**** internal functions ****/

static void print_result(struct mxt_config_t *pconfig)
{
	int i = 0;

	pr_info("object_num:%d, instance_num:%d",
			pconfig->obj_info.object_num,
			pconfig->obj_info.instance_num);
	if (pconfig->obj_info.object_num == MXT_USER_INFO_T38) {
		pr_info("[SPT_USERDATA_T38 INSTANCE 0]");
		for (i = 0; i < 64; i++)
			pr_info("DATA[%d]=%d", i,
					pconfig->config.userdata_t38.data[i]);
	}

	if (pconfig->obj_info.object_num == MXT_GEN_POWERCONFIG_T7) {
		pr_info("[GEN_POWERCONFIG_T7 INSTANCE 0]");
#define T7	(pconfig->config.power_t7)
		pr_info("IDLEACQINT=%d", T7.idleacqint);
		pr_info("ACTVACQINT=%d", T7.actvacqint);
		pr_info("ACTV2IDLETO=%d", T7.actv2idleto);
#undef T7
	}

	if (pconfig->obj_info.object_num == MXT_GEN_ACQUIRECONFIG_T8) {
#define T8	(pconfig->config.acquisition_t8)
		pr_info("[GEN_ACQUISITIONCONFIG_T8 INSTANCE 0]");
		pr_info("CHRGTIME=%d", T8.chrgtime);
		pr_info("RESERVED=%d", T8.reserved);
		pr_info("TCHDRIFT=%d", T8.tchdrift);
		pr_info("DRIFTST=%d", T8.driftst);
		pr_info("TCHAUTOCAL=%d", T8.tchautocal);
		pr_info("SYNC=%d", T8.sync);
		pr_info("ATCHCALST=%d", T8.atchcalst);
		pr_info("ATCHCALSTHR=%d", T8.atchcalsthr);
		pr_info("ATCHFRCCALTHR=%d", T8.atchcalfrcthr);
		pr_info("ATCHFRCCALRATIO=%d", T8.atchcalfrcratio);
#undef T8
	}

	if (pconfig->obj_info.object_num == MXT_TOUCH_MULTITOUCHSCREEN_T9) {
		for (i = 0; i < 2; i++) {
			touch_multitouchscreen_t9_config_t *ts_t9 = NULL;

			if (pconfig->obj_info.instance_num == i) {
				if (0 == i) {
					ts_t9 = &pconfig->config.\
						touchscreen_t9;
					pr_info("[TOUCH_MULTITOUCHSCREEN_T9 "
							"INSTANCE 0]");
				} else {
					ts_t9 = &pconfig->config.\
						touchscreen1_t9;
					pr_info("[TOUCH_MULTITOUCHSCREEN_T9 "
							"INSTANCE 1]");
				}

				pr_info("CTRL=%d",	ts_t9->ctrl);
				pr_info("XORIGIN=%d",	ts_t9->xorigin);
				pr_info("YORIGIN=%d",	ts_t9->yorigin);
				pr_info("XSIZE=%d",	ts_t9->xsize);
				pr_info("YSIZE=%d",	ts_t9->ysize);
				pr_info("AKSCFG=%d",	ts_t9->akscfg);
				pr_info("BLEN=%d",	ts_t9->blen);
				pr_info("TCHTHR=%d",	ts_t9->tchthr);
				pr_info("TCHDI=%d",	ts_t9->tchdi);
				pr_info("ORIENT=%d",	ts_t9->orient);
				pr_info("MRGTIMEOUT=%d", ts_t9->mrgtimeout);
				pr_info("MOVHYSTI=%d",	ts_t9->movhysti);
				pr_info("MOVHYSTN=%d",	ts_t9->movhystn);
				pr_info("MOVFILTER=%d",	ts_t9->movfilter);
				pr_info("NUMTOUCH=%d",	ts_t9->numtouch);
				pr_info("MRGHYST=%d",	ts_t9->mrghyst);
				pr_info("MRGTHR=%d",	ts_t9->mrgthr);
				pr_info("AMPHYST=%d",	ts_t9->amphyst);
				pr_info("XRANGE=%d",	ts_t9->xrange);
				pr_info("YRANGE=%d",	ts_t9->yrange);
				pr_info("XLOCLIP=%d",	ts_t9->xloclip);
				pr_info("XHICLIP=%d",	ts_t9->xhiclip);
				pr_info("YLOCLIP=%d",	ts_t9->yloclip);
				pr_info("YHICLIP=%d",	ts_t9->yhiclip);
				pr_info("XEDGECTRL=%d",	ts_t9->xedgectrl);
				pr_info("XEDGEDIST=%d",	ts_t9->xedgedist);
				pr_info("YEDGECTRL=%d",	ts_t9->yedgectrl);
				pr_info("YEDGEDIST=%d",	ts_t9->yedgedist);
				pr_info("JUMPLIMIT=%d",	ts_t9->jumplimit);
				pr_info("TCHHYST=%d",	ts_t9->tchhyst);
				pr_info("XPITCH=%d",	ts_t9->xpitch);
				pr_info("YPITCH=%d",	ts_t9->ypitch);
				pr_info("NEXTTCHDI=%d",	ts_t9->nexttchdi);
			}
		}
	}

	if (pconfig->obj_info.object_num == MXT_TOUCH_KEYARRAY_T15) {
		for (i = 0; i < 2; i++) {
			touch_keyarray_t15_config_t *tk_t15 = NULL;
			if (pconfig->obj_info.instance_num == i) {
				if (0 == i) {
					tk_t15 = &pconfig->config.keyarray_t15;
					pr_info("[TOUCH_KEYARRAY_T15 INSTANCE 0]");
				} else {
					tk_t15 = &pconfig->config.keyarray1_t15;
					pr_info("[TOUCH_KEYARRAY_T15 INSTANCE 1]");
				}

				pr_info("CTRL=%d", tk_t15->ctrl);
				pr_info("XORIGIN=%d", tk_t15->xorigin);
				pr_info("YORIGIN=%d", tk_t15->yorigin);
				pr_info("XSIZE=%d", tk_t15->xsize);
				pr_info("YSIZE=%d", tk_t15->ysize);
				pr_info("AKSCFG=%d", tk_t15->akscfg);
				pr_info("BLEN=%d", tk_t15->blen);
				pr_info("TCHTHR=%d", tk_t15->tchthr);
				pr_info("TCHDI=%d", tk_t15->tchdi);
				pr_info("RESERVED[0]=%d", tk_t15->reserved[0]);
				pr_info("RESERVED[1]=%d", tk_t15->reserved[1]);
			}
		}
	}

	if (pconfig->obj_info.object_num == MXT_SPT_COMMSCONFIG_T18) {
		pr_info("[SPT_COMMSCONFIG_T18 INSTANCE 0]");
		pr_info("CTRL=%d",	pconfig->config.comc_t18.ctrl);
		pr_info("COMMAND=%d",	pconfig->config.comc_t18.cmd);
	}

	if (pconfig->obj_info.object_num == MXT_SPT_GPIOPWM_T19) {
#define T19 (pconfig->config.gpiopwm_t19)
		pr_info("[SPT_GPIOPWM_T19 INSTANCE 0]");
		pr_info("CTRL=%d", T19.ctrl);
		pr_info("REPORTMASK=%d", T19.reportmask);
		pr_info("DIR=%d", T19.dir);
		pr_info("INTPULLUP=%d", T19.intpullup);
		pr_info("OUT=%d", T19.out);
		pr_info("WAKE=%d", T19.wake);
		pr_info("PWM=%d", T19.pwm);
		pr_info("PERIOD=%d", T19.period);
		pr_info("DUTY[0]=%d", T19.duty[0]);
		pr_info("DUTY[1]=%d", T19.duty[1]);
		pr_info("DUTY[2]=%d", T19.duty[2]);
		pr_info("DUTY[3]=%d", T19.duty[3]);
		pr_info("TRIGGER[0]=%d", T19.trigger[0]);
		pr_info("TRIGGER[1]=%d", T19.trigger[1]);
		pr_info("TRIGGER[2]=%d", T19.trigger[2]);
		pr_info("TRIGGER[3]=%d", T19.trigger[3]);
#undef T19
	}

	if (pconfig->obj_info.object_num ==
			MXT_PROCI_ONETOUCHGESTUREPROCESSOR_T24) {
		for (i = 0; i < 2; i++) {
			proci_onetouchgestureprocessor_t24_config_t *pog_t24
				= NULL;
			if (pconfig->obj_info.instance_num == i) {
				if (0 == i) {
					pog_t24 = &pconfig->config.\
						  onegesture_t24;
					pr_info("[PROCI_ONETOUCHGESTUREPROCE"
							"SSOR_T24 INSTANCE 0]");
				} else {
					pog_t24 = &pconfig->config.\
						  onegesture1_t24;
					pr_info("[PROCI_ONETOUCHGESTUREPROCE"
							"SSOR_T24 INSTANCE 1]");
				}

				pr_info("CTRL=%d", pog_t24->ctrl);
				pr_info("NUMGEST=%d", pog_t24->numgest);
				pr_info("GESTEN=%d", pog_t24->gesten);
				pr_info("PROCESS=%d", pog_t24->process);
				pr_info("TAPTO=%d", pog_t24->tapto);
				pr_info("FLICKTO=%d", pog_t24->flickto);
				pr_info("DRAGTO=%d", pog_t24->dragto);
				pr_info("SPRESSTO=%d", pog_t24->spressto);
				pr_info("LPRESSTO=%d", pog_t24->lpressto);
				pr_info("REPPRESSTO=%d", pog_t24->reppressto);
				pr_info("FLICKTHR=%d", pog_t24->flickthr);
				pr_info("DRAGTHR=%d", pog_t24->dragthr);
				pr_info("TAPTHR=%d", pog_t24->tapthr);
				pr_info("THROWTHR=%d", pog_t24->throwthr);
			}
		}
	}

	if (pconfig->obj_info.object_num == MXT_SPT_SELFTEST_T25) {
		if (pconfig->version <= 10) {
#define T25 (pconfig->config.selftest_t25)
			pr_info("[SPT_SELFTEST_T25 INSTANCE 0]");
			pr_info("CTRL=%d", T25.ctrl);
			pr_info("CMD=%d", T25.cmd);
			pr_info("SIGLIM[0].UPSIGLIM=%d",
					T25.siglim[0].upsiglim);
			pr_info("SIGLIM[0].LOSIGLIM=%d",
					T25.siglim[0].losiglim);
			pr_info("SIGLIM[1].UPSIGLIM=%d",
					T25.siglim[1].upsiglim);
			pr_info("SIGLIM[1].LOSIGLIM=%d",
					T25.siglim[1].losiglim);
			pr_info("SIGLIM[2].UPSIGLIM=%d",
					T25.siglim[2].upsiglim);
			pr_info("SIGLIM[2].LOSIGLIM=%d",
					T25.siglim[2].losiglim);
#undef T25
		} else if (pconfig->version >= 21) {
#define VER2_T25 (pconfig->config.selftest_ver2_t25)
			pr_info("[SPT_SELFTEST_T25 INSTANCE 0]");
			pr_info("CTRL=%d", VER2_T25.ctrl);
			pr_info("CMD=%d", VER2_T25.cmd);
			pr_info("SIGLIM[0].UPSIGLIM=%d",
					VER2_T25.siglim[0].upsiglim);
			pr_info("SIGLIM[0].LOSIGLIM=%d",
					VER2_T25.siglim[0].losiglim);
			pr_info("SIGLIM[1].UPSIGLIM=%d",
					VER2_T25.siglim[1].upsiglim);
			pr_info("SIGLIM[1].LOSIGLIM=%d",
					VER2_T25.siglim[1].losiglim);
			pr_info("SIGLIM[2].UPSIGLIM=%d",
					VER2_T25.siglim[2].upsiglim);
			pr_info("SIGLIM[2].LOSIGLIM=%d",
					VER2_T25.siglim[2].losiglim);
			pr_info("PINDWELLUS=%d", VER2_T25.pindwellus);
#undef VER2_T25
		}
	}

	if (pconfig->obj_info.object_num ==
			MXT_PROCI_TWOTOUCHGESTUREPROCESSOR_T27) {
		for (i = 0; i < 2; i++) {
			proci_twotouchgestureprocessor_t27_config_t *ptg_t27
				= NULL;
			if (pconfig->obj_info.instance_num == i) {
				if (0 == i) {
					ptg_t27 = &pconfig->config.\
						  twogesture_t27;
					pr_info("[PROCI_TWOTOUCHGESTUREPROCE"
							"SSOR_T27 INSTANCE 0]");
				} else {
					ptg_t27 = &pconfig->config.\
						 twogesture1_t27;
					pr_info("[PROCI_TWOTOUCHGESTUREPROCE"
							"SSOR_T27 INSTANCE 1]");
				}

				pr_info("CTRL=%d", ptg_t27->ctrl);
				pr_info("NUMGEST=%d", ptg_t27->numgest);
				pr_info("RESERVED[0]=%d", ptg_t27->reserved2);
				pr_info("GESTEN=%d", ptg_t27->gesten);
				pr_info("ROTATETHR=%d",	ptg_t27->rotatethr);
				pr_info("ZOOMTHR=%d", ptg_t27->zoomthr);
			}
		}
	}

	if (pconfig->obj_info.object_num == MXT_PROCI_GRIPSUPPRESSION_T40) {
		for (i = 0; i < 2; i++) {
			proci_gripsuppression_t40_config_t *pgs_t40 = NULL;
			if (pconfig->obj_info.instance_num == i) {
				if (0 == i) {
					pgs_t40 = &pconfig->config.\
						  gripsuppression_t40;
					pr_info("[PROCI_GRIPSUPPRESSION_T40 "
							"INSTANCE 0]");
				} else {
					pgs_t40 = &pconfig->config.\
						  gripsuppression1_t40;
					pr_info("[PROCI_GRIPSUPPRESSION_T40 "
							"INSTANCE 1]");
				}

				pr_info("CTRL=%d", pgs_t40->ctrl);
				pr_info("XLOGRIP=%d", pgs_t40->xlogrip);
				pr_info("XHIGRIP=%d", pgs_t40->xhigrip);
				pr_info("YLOGRIP=%d", pgs_t40->ylogrip);
				pr_info("YHIGRIP=%d", pgs_t40->yhigrip);
			}
		}
	}

	if (pconfig->obj_info.object_num == MXT_PROCI_TOUCHSUPPRESSION_T42) {
		for (i = 0; i < 2; i++) {
			if (pconfig->version <= 10) {
				proci_touchsuppression_t42_config_t \
					*pts_t42 = NULL;
				if (pconfig->obj_info.instance_num == i) {
					if (0 == i) {
						pts_t42 = &pconfig->config.\
							  touchsuppression_t42;
						pr_info("[PROCI_TOUCH "
							"SUPPRESSION_T42 "
							"INSTANCE 0]");
					} else {
						pts_t42 = &pconfig->config.\
							  touchsuppression1_t42;
						pr_info("[PROCI_TOUCH "
							"SUPPRESSION_T42 "
							"INSTANCE 1]");
					}

					pr_info("CTRL=%d",
						pts_t42->ctrl);
					pr_info("APPRTHR=%d",
						pts_t42->apprthr);
					pr_info("MAXAPPRAREA=%d",
						pts_t42->maxapprarea);
					pr_info("MAXTCHAREA=%d",
						pts_t42->maxtcharea);
					pr_info("SUPSTRENGTH=%d",
						pts_t42->supstrength);
					pr_info("SUPEXTTO=%d",
						pts_t42->supextto);
					pr_info("MAXNUMTCHS=%d",
						pts_t42->maxnumtchs);
					pr_info("SHAPESTRENGTH=%d",
						pts_t42->shapestrength);
				}
			} else if (pconfig->version >= 21) {
				proci_tsuppression_t42_ver2_config_t \
					*pts_t42 = NULL;
				if (pconfig->obj_info.instance_num == i) {
					if (0 == i) {
						pts_t42 = &pconfig->config.\
							tsuppression_ver2_t42;
						pr_info("[PROCI_TOUCH "
							"SUPPRESSION_T42 "
							"INSTANCE 0]");
					} else {
						pts_t42 = &pconfig->config.\
							tsuppression1_ver2_t42;
						pr_info("[PROCI_TOUCH "
							"SUPPRESSION_T42 "
							"INSTANCE 1]");
					}

					pr_info("CTRL=%d",
						pts_t42->ctrl);
					pr_info("APPRTHR=%d",
						pts_t42->apprthr);
					pr_info("MAXAPPRAREA=%d",
						pts_t42->maxapprarea);
					pr_info("MAXTCHAREA=%d",
						pts_t42->maxtcharea);
					pr_info("SUPSTRENGTH=%d",
						pts_t42->supstrength);
					pr_info("SUPEXTTO=%d",
						pts_t42->supextto);
					pr_info("MAXNUMTCHS=%d",
						pts_t42->maxnumtchs);
					pr_info("SHAPESTRENGTH=%d",
						pts_t42->shapestrength);
					pr_info("SUPDIST=%d",
						pts_t42->supdist);
					pr_info("DISTHYST=%d",
						pts_t42->disthyst);
				}
			}
		}
	}

	if (pconfig->obj_info.object_num == MXT_SPT_CTECONFIG_T46) {
		if (pconfig->version <= 10) {
#define T46	(pconfig->config.cteconfig_t46)
			pr_info("[SPT_CTECONFIG_T46 INSTANCE 0]");
			pr_info("CTRL=%d", T46.ctrl);
			pr_info("RESERVED=%d", T46.mode);
			pr_info("IDLESYNCSPERX=%d", T46.idlesyncsperx);
			pr_info("ACTVSYNCSPERX=%d", T46.actvsyncsperx);
			pr_info("ADCSPERSYNC=%d", T46.adcspersync);
			pr_info("PULSESPERADC=%d", T46.pulsesperadc);
			pr_info("XSLEW=%d", T46.xslew);
			pr_info("SYNCDELAY=%d", T46.syncdelay);
#undef T46
		} else if (pconfig->version >= 21) {
#define VER2_T46 (pconfig->config.cteconfig_ver2_t46)
			pr_info("[SPT_CTECONFIG_T46 INSTANCE 0]");
			pr_info("CTRL=%d", VER2_T46.ctrl);
			pr_info("RESERVED=%d", VER2_T46.reserved);
			pr_info("IDLESYNCSPERX=%d", VER2_T46.idlesyncsperx);
			pr_info("ACTVSYNCSPERX=%d", VER2_T46.actvsyncsperx);
			pr_info("ADCSPERSYNC=%d", VER2_T46.adcspersync);
			pr_info("PULSESPERADC=%d", VER2_T46.pulsesperadc);
			pr_info("XSLEW=%d", VER2_T46.xslew);
			pr_info("SYNCDELAY=%d", VER2_T46.syncdelay);
			pr_info("XVOLTAGE=%d", VER2_T46.xvoltage);
#undef VER2_T46
		}
	}

	if (pconfig->obj_info.object_num == MXT_PROCI_STYLUS_T47) {
		for (i = 0; i < 2; i++) {
			proci_stylus_t47_config_t *ps_t47 = NULL;
			if (pconfig->obj_info.instance_num == i) {
				if (0 == i) {
					ps_t47 = &pconfig->config.stylus_t47;
					pr_info("[PROCI_STYLUS_T47 INSTANCE 0]");
				} else {
					ps_t47 = &pconfig->config.stylus1_t47;
					pr_info("[PROCI_STYLUS_T47 INSTANCE 1]");
				}

				pr_info("CTRL=%d", ps_t47->ctrl);
				pr_info("CONTMIN=%d", ps_t47->contmin);
				pr_info("CONTMAX=%d", ps_t47->contmax);
				pr_info("STABILITY=%d",	ps_t47->stability);
				pr_info("MAXTCHAREA=%d", ps_t47->maxtcharea);
				pr_info("AMPLTHR=%d", ps_t47->amplthr);
				pr_info("STYSHAPE=%d", ps_t47->styshape);
				pr_info("HOVERSUP=%d", ps_t47->hoversup);
				pr_info("CONFTHR=%d", ps_t47->confthr);
				pr_info("SYNCSPERX=%d",	ps_t47->syncsperx);
			}
		}
	}

	if (pconfig->obj_info.object_num == MXT_PROCG_NOISESUPPRESSION_T48) {
		if (pconfig->version <= 10) {
#define T48	(pconfig->config.noisesuppression_t48)
			pr_info("[PROCG_NOISESUPPRESSION_T48 INSTANCE 0]");
			pr_info("CTRL=%d", T48.ctrl);
			pr_info("CFG=%d", T48.cfg);
			pr_info("CALCFG=%d", T48.calcfg);
			pr_info("BASEFREQ=%d", T48.basefreq);
			pr_info("MFFREQ[0]=%d", T48.mffreq_2);
			pr_info("MFFREQ[1]=%d", T48.mffreq_3);
			pr_info("GCACTVINVLDADCS=%d", T48.gcactvinvldadcs);
			pr_info("GCIDLEINVLDADCS=%d", T48.gcidleinvldadcs);
			pr_info("GCMAXADCSPERX=%d", T48.gcmaxadcsperx);
			pr_info("GCLIMITMIN=%d", T48.gclimitmin);
			pr_info("GCLIMITMAX=%d", T48.gclimitmax);
			pr_info("GCCOUNTMINTGT=%d", T48.gccountmintgt);
			pr_info("MFINVLDDIFFTHR=%d", T48.mfinvlddiffthr);
			pr_info("MFINCADCSPXTHR=%d", T48.mfincadcspxthr);
			pr_info("MFERRORTHR=%d", T48.mferrorthr);
			pr_info("SELFREQMAX=%d", T48.selfreqmax);
			/* T9 settings */
			pr_info("BLEN[0]=%d", T48.blen0);
			pr_info("TCHTHR[0]=%d", T48.tchthr0);
			pr_info("TCHDI[0]=%d", T48.tchdi0);
			pr_info("MOVHYSTI[0]=%d", T48.movhysti0);
			pr_info("MOVHYSTN[0]=%d", T48.movhystn0);
			pr_info("MOVFILTER[0]=%d", T48.movfilter0);
			pr_info("NUMTOUCH[0]=%d", T48.numtouch0);
			pr_info("MRGHYST[0]=%d", T48.mrghyst0);
			pr_info("MRGTHR[0]=%d", T48.mrgthr0);
			pr_info("XLOCLIP[0]=%d", T48.xloclip0);
			pr_info("XHICLIP[0]=%d", T48.xhiclip0);
			pr_info("YLOCLIP[0]=%d", T48.yloclip0);
			pr_info("YHICLIP[0]=%d", T48.yhiclip0);
			pr_info("XEDGECTRL[0]=%d", T48.xedgectrl0);
			pr_info("XEDGEDIST[0]=%d", T48.xedgedist0);
			pr_info("YEDGECTRL[0]=%d", T48.yedgectrl0);
			pr_info("YEDGEDIST[0]=%d", T48.yedgedist0);
			pr_info("JUMPLIMIT[0]=%d", T48.jumplimit0);
			pr_info("TCHHYST[0]=%d", T48.tchhyst0);
			pr_info("NEXTTCHDI[0]=%d", T48.nexttchdi0);
			pr_info("BLEN[1]=%d", T48.blen1);
			pr_info("TCHTHR[1]=%d", T48.tchthr1);
			pr_info("TCHDI[1]=%d", T48.tchdi1);
			pr_info("MOVHYSTI[1]=%d", T48.movhysti1);
			pr_info("MOVHYSTN[1]=%d", T48.movhystn1);
			pr_info("MOVFILTER[1]=%d", T48.movfilter1);
			pr_info("NUMTOUCH[1]=%d", T48.numtouch1);
			pr_info("MRGHYST[1]=%d", T48.mrghyst1);
			pr_info("MRGTHR[1]=%d", T48.mrgthr1);
			pr_info("XLOCLIP[1]=%d", T48.xloclip1);
			pr_info("XHICLIP[1]=%d", T48.xhiclip1);
			pr_info("YLOCLIP[1]=%d", T48.yloclip1);
			pr_info("YHICLIP[1]=%d", T48.yhiclip1);
			pr_info("XEDGECTRL[1]=%d", T48.xedgectrl1);
			pr_info("XEDGEDIST[1]=%d", T48.xedgedist1);
			pr_info("YEDGECTRL[1]=%d", T48.yedgectrl1);
			pr_info("YEDGEDIST[1]=%d", T48.yedgedist1);
			pr_info("JUMPLIMIT[1]=%d", T48.jumplimit1);
			pr_info("TCHHYST[1]=%d", T48.tchhyst1);
			pr_info("NEXTTCHDI[1]=%d", T48.nexttchdi1);
#undef T48
		} else if (pconfig->version >= 21) {
#define VER2_T48	(pconfig->config.nsuppression_ver2_t48)
			pr_info("[PROCG_NOISESUPPRESSION_T48 INSTANCE 0]");
			pr_info("CTRL=%d", VER2_T48.ctrl);
			pr_info("CFG=%d", VER2_T48.cfg);
			pr_info("CALCFG=%d", VER2_T48.calcfg);
			pr_info("BASEFREQ=%d", VER2_T48.basefreq);
			pr_info("RESERVED[0]=%d", VER2_T48.reserved0);
			pr_info("RESERVED[1]=%d", VER2_T48.reserved1);
			pr_info("RESERVED[2]=%d", VER2_T48.reserved2);
			pr_info("RESERVED[3]=%d", VER2_T48.reserved3);
			pr_info("MFFREQ[0]=%d", VER2_T48.mffreq_2);
			pr_info("MFFREQ[1]=%d", VER2_T48.mffreq_3);
			pr_info("NLGAIN=%d", VER2_T48.nlgain);
			pr_info("NLTHR=%d", VER2_T48.nlthr);
			pr_info("RESERVED=%d", VER2_T48.reserved4);
			pr_info("GCACTVINVLDADCS=%d", VER2_T48.gcactvinvldadcs);
			pr_info("GCIDLEINVLDADCS=%d", VER2_T48.gcidleinvldadcs);
			pr_info("RESERVED[0]=%d", VER2_T48.reserved5);
			pr_info("RESERVED[1]=%d", VER2_T48.reserved6);
			pr_info("GCMAXADCSPERX=%d", VER2_T48.gcmaxadcsperx);
			pr_info("GCLIMITMIN=%d", VER2_T48.gclimitmin);
			pr_info("GCLIMITMAX=%d", VER2_T48.gclimitmax);
			pr_info("RESERVED[0]=%d", VER2_T48.reserved7);
			pr_info("RESERVED[1]=%d", VER2_T48.reserved8);
			pr_info("MFINVLDDIFFTHR=%d", VER2_T48.mfinvlddiffthr);
			pr_info("RESERVED[0]=%d", VER2_T48.reserved9);
			pr_info("RESERVED[1]=%d", VER2_T48.reserved10);
			pr_info("RESERVED[2]=%d", VER2_T48.reserved11);
			pr_info("RESERVED[3]=%d", VER2_T48.reserved12);
			pr_info("SELFREQMAX=%d", VER2_T48.selfreqmax);
			pr_info("CFG2=%d", VER2_T48.cfg2);
			pr_info("RESERVED[0]=%d", VER2_T48.reserved13);
			pr_info("RESERVED[1]=%d", VER2_T48.reserved14);
			pr_info("RESERVED[2]=%d", VER2_T48.reserved15);
			pr_info("RESERVED[3]=%d", VER2_T48.reserved16);
			/* T9 settings */
			pr_info("BLEN[0]=%d", VER2_T48.blen0);
			pr_info("TCHTHR[0]=%d", VER2_T48.tchthr0);
			pr_info("TCHDI[0]=%d", VER2_T48.tchdi0);
			pr_info("MOVHYSTI[0]=%d", VER2_T48.movhysti0);
			pr_info("MOVHYSTN[0]=%d", VER2_T48.movhystn0);
			pr_info("MOVFILTER[0]=%d", VER2_T48.movfilter0);
			pr_info("NUMTOUCH[0]=%d", VER2_T48.numtouch0);
			pr_info("MRGHYST[0]=%d", VER2_T48.mrghyst0);
			pr_info("MRGTHR[0]=%d", VER2_T48.mrgthr0);
			pr_info("XLOCLIP[0]=%d", VER2_T48.xloclip0);
			pr_info("XHICLIP[0]=%d", VER2_T48.xhiclip0);
			pr_info("YLOCLIP[0]=%d", VER2_T48.yloclip0);
			pr_info("YHICLIP[0]=%d", VER2_T48.yhiclip0);
			pr_info("XEDGECTRL[0]=%d", VER2_T48.xedgectrl0);
			pr_info("XEDGEDIST[0]=%d", VER2_T48.xedgedist0);
			pr_info("YEDGECTRL[0]=%d", VER2_T48.yedgectrl0);
			pr_info("YEDGEDIST[0]=%d", VER2_T48.yedgedist0);
			pr_info("JUMPLIMIT[0]=%d", VER2_T48.jumplimit0);
			pr_info("TCHHYST[0]=%d", VER2_T48.tchhyst0);
			pr_info("NEXTTCHDI[0]=%d", VER2_T48.nexttchdi0);
			pr_info("BLEN[1]=%d", VER2_T48.blen1);
			pr_info("TCHTHR[1]=%d", VER2_T48.tchthr1);
			pr_info("TCHDI[1]=%d", VER2_T48.tchdi1);
			pr_info("MOVHYSTI[1]=%d", VER2_T48.movhysti1);
			pr_info("MOVHYSTN[1]=%d", VER2_T48.movhystn1);
			pr_info("MOVFILTER[1]=%d", VER2_T48.movfilter1);
			pr_info("NUMTOUCH[1]=%d", VER2_T48.numtouch1);
			pr_info("MRGHYST[1]=%d", VER2_T48.mrghyst1);
			pr_info("MRGTHR[1]=%d", VER2_T48.mrgthr1);
			pr_info("XLOCLIP[1]=%d", VER2_T48.xloclip1);
			pr_info("XHICLIP[1]=%d", VER2_T48.xhiclip1);
			pr_info("YLOCLIP[1]=%d", VER2_T48.yloclip1);
			pr_info("YHICLIP[1]=%d", VER2_T48.yhiclip1);
			pr_info("XEDGECTRL[1]=%d", VER2_T48.xedgectrl1);
			pr_info("XEDGEDIST[1]=%d", VER2_T48.xedgedist1);
			pr_info("YEDGECTRL[1]=%d", VER2_T48.yedgectrl1);
			pr_info("YEDGEDIST[1]=%d", VER2_T48.yedgedist1);
			pr_info("JUMPLIMIT[1]=%d", VER2_T48.jumplimit1);
			pr_info("TCHHYST[1]=%d", VER2_T48.tchhyst1);
			pr_info("NEXTTCHDI[1]=%d", VER2_T48.nexttchdi1);
#undef VER2_T48
		}
	}

	if (pconfig->obj_info.object_num == MXT_TOUCH_PROXIMITY_T52) {
		for (i = 0; i < 2; i++) {
			touch_proximity_t52_config_t *tpx_t52 = NULL;
			if (pconfig->obj_info.instance_num == i) {
				if (0 == i) {
					tpx_t52 =
						&pconfig->config.proximity_t52;
					pr_info("[TOUCH_PROXKEY_T52 "
							"INSTANCE 0]");
				} else {
					tpx_t52 =
						&pconfig->config.proximity1_t52;
					pr_info("[TOUCH_PROXKEY_T52 "
							"INSTANCE 1]");
				}

				pr_info("CTRL=%d", tpx_t52->ctrl);
				pr_info("XORIGIN=%d", tpx_t52->xorigin);
				pr_info("YORIGIN=%d",	tpx_t52->yorigin);
				pr_info("RESERVED[0]=%d", tpx_t52->reserved0);
				pr_info("RESERVED[1]=%d", tpx_t52->reserved1);
				pr_info("AKSCFG=%d",	tpx_t52->askcfg);
				pr_info("RESERVED[2]=%d", tpx_t52->reserved2);
				pr_info("FXDDTHR=%d",	tpx_t52->fxddthr);
				pr_info("FXDDI=%d", tpx_t52->fxddi);
				pr_info("AVERAGE=%d",	tpx_t52->average);
				pr_info("MVNULLRATE=%d", tpx_t52->mvnullrate);
				pr_info("MVDTHR=%d", tpx_t52->mvdthr);
			}
		}
	}

	if (pconfig->obj_info.object_num == MXT_ADAPTIVE_THRESHOLD_T55) {
		for (i = 0; i < 2; i++) {
			proci_adaptivethreshold_t55_config_t *tpx_t55 = NULL;
			if (pconfig->obj_info.instance_num == i) {
				if (i == 0) {
					tpx_t55 = &pconfig->config.
						adaptive_threshold_t55;
					pr_info("[ADAPTIVE_THRESHOLD_T55 "
							"INSTANCE 0]");
				} else {
					tpx_t55 = &pconfig->config.
						adaptive_threshold1_t55;
					pr_info("[ADAPTIVE_THRESHOLD_T55 "
							"INSTANCE 1]");
				}

				pr_info("CTRL=%d", tpx_t55->ctrl);
				pr_info("TARGETTHR=%d", tpx_t55->targetthr);
				pr_info("THRADJLIM=%d", tpx_t55->thradjlim);
				pr_info("RESETSTEPTIME=%d",
						tpx_t55->resetsteptime);
				pr_info("FORCECHGDIST=%d",
						tpx_t55->forcechgdist);
				pr_info("FORCECHGTIME=%d",
						tpx_t55->forcechgtime);
				pr_info("LOWESTTHR=%d", tpx_t55->lowestthr);
			}
		}
	}

	if (pconfig->obj_info.object_num == MXT_SHIELDLESS_T56) {
#define T56	(pconfig->config.shieldless_t56)
		pr_info("CTRL=%d", T56.ctrl);
		pr_info("COMMAND=%d", T56.command);
		pr_info("OPTINT=%d", T56.optint);
		pr_info("INTTIME=%d", T56.inttime);
		pr_info("INTDELAY[0]=%d", T56.intdelay[0]);
		pr_info("INTDELAY[1]=%d", T56.intdelay[1]);
		pr_info("INTDELAY[2]=%d", T56.intdelay[2]);
		pr_info("INTDELAY[3]=%d", T56.intdelay[3]);
		pr_info("INTDELAY[4]=%d", T56.intdelay[4]);
		pr_info("INTDELAY[5]=%d", T56.intdelay[5]);
		pr_info("INTDELAY[6]=%d", T56.intdelay[6]);
		pr_info("INTDELAY[7]=%d", T56.intdelay[7]);
		pr_info("INTDELAY[8]=%d", T56.intdelay[8]);
		pr_info("INTDELAY[9]=%d", T56.intdelay[9]);
		pr_info("INTDELAY[10]=%d", T56.intdelay[10]);
		pr_info("INTDELAY[11]=%d", T56.intdelay[11]);
		pr_info("INTDELAY[12]=%d", T56.intdelay[12]);
		pr_info("INTDELAY[13]=%d", T56.intdelay[13]);
		pr_info("INTDELAY[14]=%d", T56.intdelay[14]);
		pr_info("INTDELAY[15]=%d", T56.intdelay[15]);
		pr_info("INTDELAY[16]=%d", T56.intdelay[16]);
		pr_info("INTDELAY[17]=%d", T56.intdelay[17]);
		pr_info("MULTICUTGC=%d", T56.multicutgc);
		pr_info("RESERVED=%d", T56.reserved);
		pr_info("NCNCL=%d", T56.ncncl);
		pr_info("TOUCHBIAS=%d", T56.touchbias);
		pr_info("BASESCALE=%d", T56.basescale);
		pr_info("SHIFTLIMIT=%d", T56.shiftlimit);
#undef T56
	}

	if (pconfig->obj_info.object_num == MXT_EXTRA_TOUCH_SCREEN_DATA_T57) {
		for (i = 0; i < 2; i++) {
			proci_extratouchscreendata_t57_config_t *tpx_t57 = NULL;
			if (pconfig->obj_info.instance_num == i) {
				if (i == 0) {
					tpx_t57 = &pconfig->config.
						exttouchscreendata_t57;
					pr_info("[EXTRA_TOUCH_SCREEN_DATA_T57 "
							"INSTANCE 0]");
				} else {
					tpx_t57 = &pconfig->config.
						exttouchscreendata1_t57;
					pr_info("[EXTRA_TOUCH_SCREEN_DATA_T57 "
							"INSTANCE 1]");
				}

				pr_info("CTRL=%d", tpx_t57->ctrl);
				pr_info("AREATHR=%d", tpx_t57->areathr);
				pr_info("AREAHYST=%d", tpx_t57->areahyst);
			}
		}
	}
}

static int parse_object_data(struct device *dev,
		char *mxt_string, struct mxt_config_t *pconfig)
{
	int ret = 0;
	int value = 0;
	int object_num, instance_num;
	int f_version;
#define STRING_SIZE 256
	char temp[STRING_SIZE] = {0, };

#define GOTO_ERR(num)							\
	do {								\
		num++;							\
		ret = num;						\
		goto parse_object_data_out;				\
	} while (0);							\

#define COMP_AND_GET(ORG, CMP_STR, RET, VAL)				\
	do {								\
		int ret;						\
		ret = compare_and_getvalue(ORG, CMP_STR, &RET);		\
		if (COM_AND_VAL_ERR == ret) {				\
			GOTO_ERR(object_num);				\
		} else  {						\
			if (COM_AND_VAL_NEXT == ret) {			\
				VAL = RET;				\
				continue;				\
			}						\
		}							\
	} while (0);

#define COMP_AND_GET2(CMP_STR, VAL)					\
	COMP_AND_GET(temp, CMP_STR, value, pconfig->config.VAL)

#define COMP_AND_GET3(CMP_STR, VAL)					\
	COMP_AND_GET(temp, CMP_STR, value, VAL)

	object_num = pconfig->obj_info.object_num;
	instance_num = pconfig->obj_info.instance_num;
	f_version = pconfig->version;

	if (object_num == MXT_USER_INFO_T38) {
		print_debug("<USERDATA T38>\n");
		do {
			int offset, value;
			char c;

			if (mxt_get_data(&mxt_string, temp, STRING_SIZE) < 0)
				break;
			if (0 == sscanf(temp, "DATA[%d]=%d", &offset, &value))
				GOTO_ERR(pconfig->obj_info.object_num);
			if ((offset >= MXT_ADR_T38_USER5) &&
					(offset < MXT_ADR_T38_USER25))  {
				if (0 == sscanf(temp, "DATA[%d]='%c'",
							&offset, &c))
					GOTO_ERR(pconfig->obj_info.object_num);
				value = (int)c;
			}
			if (offset <= 63)
				pconfig->config.userdata_t38.data[offset]
					= value;

			if (offset >= 63)
				break;
		} while (1);
	} else if (object_num == MXT_GEN_POWERCONFIG_T7) {
		print_debug("<POWER CONFIG T7>\n");
		do {
#define T7	power_t7
			if (mxt_get_data(&mxt_string, temp, STRING_SIZE) < 0)
				break;
			COMP_AND_GET2("IDLEACQINT", T7.idleacqint);
			COMP_AND_GET2("ACTVACQINT", T7.actvacqint);
			COMP_AND_GET2("ACTV2IDLETO", T7.actv2idleto);
		} while (1);
	} else if (object_num == MXT_GEN_ACQUIRECONFIG_T8) {
		print_debug("<ACQUISITIONCONFIG_CONFIG T8>\n");
		do {
#define T8	acquisition_t8
			if (mxt_get_data(&mxt_string, temp, STRING_SIZE) < 0)
				break;

			COMP_AND_GET2("CHRGTIME", T8.chrgtime);
			COMP_AND_GET2("RESERVED", T8.reserved);
			COMP_AND_GET2("TCHDRIFT", T8.tchdrift);
			COMP_AND_GET2("DRIFTST", T8.driftst);
			COMP_AND_GET2("TCHAUTOCAL", T8.tchautocal);
			COMP_AND_GET2("SYNC", T8.sync);
			COMP_AND_GET2("ATCHCALST", T8.atchcalst);
			COMP_AND_GET2("ATCHCALSTHR", T8.atchcalsthr);
			COMP_AND_GET2("ATCHFRCCALTHR", T8.atchcalfrcthr);
			COMP_AND_GET2("ATCHFRCCALRATIO", T8.atchcalfrcratio);
		} while (1);
	} else if (object_num == MXT_TOUCH_MULTITOUCHSCREEN_T9) {
		touch_multitouchscreen_t9_config_t *ts_t9 = NULL;
		print_debug("<TOUCH_MULTITOUCHSCREEN_T9_INSTANCE_0>\n");
		if (0 == instance_num)
			ts_t9 = &pconfig->config.touchscreen_t9;
		else
			ts_t9 = &pconfig->config.touchscreen1_t9;

		do {
			if (mxt_get_data(&mxt_string, temp, STRING_SIZE) < 0)
				break;

			COMP_AND_GET3("CTRL", ts_t9->ctrl);
			COMP_AND_GET3("XORIGIN", ts_t9->xorigin);
			COMP_AND_GET3("YORIGIN", ts_t9->yorigin);
			COMP_AND_GET3("XSIZE", ts_t9->xsize);
			COMP_AND_GET3("YSIZE", ts_t9->ysize);
			COMP_AND_GET3("AKSCFG", ts_t9->akscfg);
			COMP_AND_GET3("BLEN", ts_t9->blen);
			COMP_AND_GET3("TCHTHR", ts_t9->tchthr);
			COMP_AND_GET3("TCHDI", ts_t9->tchdi);
			COMP_AND_GET3("ORIENT", ts_t9->orient);
			COMP_AND_GET3("MRGTIMEOUT", ts_t9->mrgtimeout);
			COMP_AND_GET3("MOVHYSTI", ts_t9->movhysti);
			COMP_AND_GET3("MOVHYSTN", ts_t9->movhystn);
			COMP_AND_GET3("MOVFILTER", ts_t9->movfilter);
			COMP_AND_GET3("NUMTOUCH", ts_t9->numtouch);
			COMP_AND_GET3("MRGHYST", ts_t9->mrghyst);
			COMP_AND_GET3("MRGTHR", ts_t9->mrgthr);
			COMP_AND_GET3("AMPHYST", ts_t9->amphyst);
			COMP_AND_GET3("XRANGE", ts_t9->xrange);
			COMP_AND_GET3("YRANGE", ts_t9->yrange);
			COMP_AND_GET3("XLOCLIP", ts_t9->xloclip);
			COMP_AND_GET3("XHICLIP", ts_t9->xhiclip);
			COMP_AND_GET3("YLOCLIP", ts_t9->yloclip);
			COMP_AND_GET3("YHICLIP", ts_t9->yhiclip);
			COMP_AND_GET3("XEDGECTRL", ts_t9->xedgectrl);
			COMP_AND_GET3("XEDGEDIST", ts_t9->xedgedist);
			COMP_AND_GET3("YEDGECTRL", ts_t9->yedgectrl);
			COMP_AND_GET3("YEDGEDIST", ts_t9->yedgedist);
			COMP_AND_GET3("JUMPLIMIT", ts_t9->jumplimit);
			COMP_AND_GET3("TCHHYST", ts_t9->tchhyst);
			COMP_AND_GET3("XPITCH", ts_t9->xpitch);
			COMP_AND_GET3("YPITCH", ts_t9->ypitch);
			COMP_AND_GET3("NEXTTCHDI", ts_t9->nexttchdi);
		} while (1);
	} else if (object_num == MXT_TOUCH_KEYARRAY_T15) {
		touch_keyarray_t15_config_t *tk_t15 = NULL;
		print_debug("TOUCH_KEYARRAY_T15_INSTANCE_0\n");
		if (0 == instance_num)
			tk_t15 = &pconfig->config.keyarray_t15;
		else
			tk_t15 = &pconfig->config.keyarray1_t15;

		do {
			if (mxt_get_data(&mxt_string, temp, STRING_SIZE) < 0)
				break;

			COMP_AND_GET3("CTRL", tk_t15->ctrl);
			COMP_AND_GET3("XORIGIN", tk_t15->xorigin);
			COMP_AND_GET3("YORIGIN", tk_t15->yorigin);
			COMP_AND_GET3("XSIZE", tk_t15->xsize);
			COMP_AND_GET3("YSIZE", tk_t15->ysize);
			COMP_AND_GET3("AKSCFG", tk_t15->akscfg);
			COMP_AND_GET3("BLEN", tk_t15->blen);
			COMP_AND_GET3("TCHTHR", tk_t15->tchthr);
			COMP_AND_GET3("TCHDI", tk_t15->tchdi);
			COMP_AND_GET3("RESERVED[0]", tk_t15->reserved[0]);
			COMP_AND_GET3("RESERVED[1]", tk_t15->reserved[1]);
		} while (1);
	} else if (object_num == MXT_SPT_COMMSCONFIG_T18) {
		print_debug("SPT_COMMSCONFIG_T18_INSTANCE_0\n");
		do {
#define T18	comc_t18
			if (mxt_get_data(&mxt_string, temp, STRING_SIZE) < 0)
				break;

			COMP_AND_GET2("CTRL", T18.ctrl);
			COMP_AND_GET2("COMMAND", T18.cmd);
		} while (1);
	} else if (object_num == MXT_SPT_GPIOPWM_T19) {
		print_debug("SPT_GPIOPWM_T19_INSTANCE_0\n");
		do {
#define T19	gpiopwm_t19
			if (mxt_get_data(&mxt_string, temp, STRING_SIZE) < 0)
				break;

			COMP_AND_GET2("CTRL", T19.ctrl);
			COMP_AND_GET2("REPORTMASK", T19.reportmask);
			COMP_AND_GET2("DIR", T19.dir);
			COMP_AND_GET2("INTPULLUP", T19.intpullup);
			COMP_AND_GET2("OUT", T19.out);
			COMP_AND_GET2("WAKE", T19.wake);
			COMP_AND_GET2("PWM", T19.pwm);
			COMP_AND_GET2("PERIOD", T19.period);
			COMP_AND_GET2("DUTY[0]", T19.duty[0]);
			COMP_AND_GET2("DUTY[1]", T19.duty[1]);
			COMP_AND_GET2("DUTY[2]", T19.duty[2]);
			COMP_AND_GET2("DUTY[3]", T19.duty[3]);
			COMP_AND_GET2("TRIGGER[0]", T19.trigger[0]);
			COMP_AND_GET2("TRIGGER[1]", T19.trigger[1]);
			COMP_AND_GET2("TRIGGER[2]", T19.trigger[2]);
			COMP_AND_GET2("TRIGGER[3]", T19.trigger[3]);
		} while (1);
	} else if (object_num == MXT_PROCI_ONETOUCHGESTUREPROCESSOR_T24) {
		proci_onetouchgestureprocessor_t24_config_t *pog_t24 = NULL;
		print_debug("PROCI_ONETOUCHGESTUREPROCESSOR_T24_INSTANCE_0\n");
		if (0 == instance_num)
			pog_t24 = &pconfig->config.onegesture_t24;
		else
			pog_t24 = &pconfig->config.onegesture1_t24;

		do {
			if (mxt_get_data(&mxt_string, temp, STRING_SIZE) < 0)
				break;

			COMP_AND_GET3("CTRL", pog_t24->ctrl);
			COMP_AND_GET3("NUMGEST", pog_t24->numgest);
			COMP_AND_GET3("GESTEN", pog_t24->gesten);
			COMP_AND_GET3("PROCESS", pog_t24->process);
			COMP_AND_GET3("TAPTO", pog_t24->tapto);
			COMP_AND_GET3("FLICKTO", pog_t24->flickto);
			COMP_AND_GET3("DRAGTO", pog_t24->dragto);
			COMP_AND_GET3("SPRESSTO", pog_t24->spressto);
			COMP_AND_GET3("LPRESSTO", pog_t24->lpressto);
			COMP_AND_GET3("REPPRESSTO", pog_t24->reppressto);
			COMP_AND_GET3("FLICKTHR", pog_t24->flickthr);
			COMP_AND_GET3("DRAGTHR", pog_t24->dragthr);
			COMP_AND_GET3("TAPTHR", pog_t24->tapthr);
			COMP_AND_GET3("THROWTHR", pog_t24->throwthr);
		} while (1);
	} else if (object_num == MXT_SPT_SELFTEST_T25) {
		print_debug("SPT_SELFTEST_T25_INSTANCE_0\n");
		do {
			if (mxt_get_data(&mxt_string, temp, STRING_SIZE) < 0)
				break;

			if (f_version <= 10) {
#define T25	selftest_t25
				COMP_AND_GET2("CTRL", T25.ctrl);
				COMP_AND_GET2("CMD", T25.cmd);
				COMP_AND_GET2("SIGLIM[0].UPSIGLIM",
						T25.siglim[0].upsiglim);
				COMP_AND_GET2("SIGLIM[0].LOSIGLIM",
						T25.siglim[0].losiglim);
				COMP_AND_GET2("SIGLIM[1].UPSIGLIM",
						T25.siglim[1].upsiglim);
				COMP_AND_GET2("SIGLIM[1].LOSIGLIM",
						T25.siglim[1].losiglim);
				COMP_AND_GET2("SIGLIM[2].UPSIGLIM",
						T25.siglim[2].upsiglim);
				COMP_AND_GET2("SIGLIM[2].LOSIGLIM",
						T25.siglim[2].losiglim);
			} else if (f_version >= 21) {
#define VER2_T25 selftest_ver2_t25
				COMP_AND_GET2("CTRL", VER2_T25.ctrl);
				COMP_AND_GET2("CMD", VER2_T25.cmd);
				COMP_AND_GET2("SIGLIM[0].UPSIGLIM",
						VER2_T25.siglim[0].upsiglim);
				COMP_AND_GET2("SIGLIM[0].LOSIGLIM",
						VER2_T25.siglim[0].losiglim);
				COMP_AND_GET2("SIGLIM[1].UPSIGLIM",
						VER2_T25.siglim[1].upsiglim);
				COMP_AND_GET2("SIGLIM[1].LOSIGLIM",
						VER2_T25.siglim[1].losiglim);
				COMP_AND_GET2("SIGLIM[2].UPSIGLIM",
						VER2_T25.siglim[2].upsiglim);
				COMP_AND_GET2("SIGLIM[2].LOSIGLIM",
						VER2_T25.siglim[2].losiglim);
				COMP_AND_GET2("PINDWELLUS",
						VER2_T25.pindwellus);
			}
		} while (1);
	} else if (object_num == MXT_PROCI_TWOTOUCHGESTUREPROCESSOR_T27) {
		proci_twotouchgestureprocessor_t27_config_t *ptg_t27 = NULL;
		print_debug("PROCI_TWOTOUCHGESTUREPROCESSOR_T27_INSTANCE_0\n");
		if (0 == instance_num)
			ptg_t27 = &pconfig->config.twogesture_t27;
		else
			ptg_t27 = &pconfig->config.twogesture1_t27;

		do {
			if (mxt_get_data(&mxt_string, temp, STRING_SIZE) < 0)
				break;

			COMP_AND_GET3("CTRL", ptg_t27->ctrl);
			COMP_AND_GET3("NUMGEST", ptg_t27->numgest);
			COMP_AND_GET3("RESERVED[0]", ptg_t27->reserved2);
			COMP_AND_GET3("GESTEN", ptg_t27->gesten);
			COMP_AND_GET3("ROTATETHR", ptg_t27->rotatethr);
			COMP_AND_GET3("ZOOMTHR", ptg_t27->zoomthr);
		} while (1);
	} else if (object_num == MXT_PROCI_GRIPSUPPRESSION_T40) {
		proci_gripsuppression_t40_config_t *pgs_t40 = NULL;
		print_debug("PROCI_GRIPSUPPRESSION_T40_INSTANCE_0\n");

		if (0 == instance_num)
			pgs_t40 = &pconfig->config.gripsuppression_t40;
		else
			pgs_t40 = &pconfig->config.gripsuppression1_t40;

		do {
			if (mxt_get_data(&mxt_string, temp, STRING_SIZE) < 0)
				break;

			COMP_AND_GET3("CTRL", pgs_t40->ctrl);
			COMP_AND_GET3("XLOGRIP", pgs_t40->xlogrip);
			COMP_AND_GET3("XHIGRIP", pgs_t40->xhigrip);
			COMP_AND_GET3("YLOGRIP", pgs_t40->ylogrip);
			COMP_AND_GET3("YHIGRIP", pgs_t40->yhigrip);
		} while (1);
	} else if (object_num == MXT_PROCI_TOUCHSUPPRESSION_T42) {
		if (f_version <= 10) {
			proci_touchsuppression_t42_config_t *pts_t42 = NULL;
			print_debug("PROCI_TOUCHSUPPRESSION_T42_INSTANCE_0\n");
			if (0 == instance_num)
				pts_t42 = &pconfig->config.touchsuppression_t42;
			else
				pts_t42 = &pconfig->config.\
					  touchsuppression1_t42;

			do {
				if (mxt_get_data(&mxt_string, \
							temp, STRING_SIZE) < 0)
					break;

				COMP_AND_GET3("CTRL",
						pts_t42->ctrl);
				COMP_AND_GET3("APPRTHR",
						pts_t42->apprthr);
				COMP_AND_GET3("MAXAPPRAREA",
						pts_t42->maxapprarea);
				COMP_AND_GET3("MAXTCHAREA",
						pts_t42->maxtcharea);
				COMP_AND_GET3("SUPSTRENGTH",
						pts_t42->supstrength);
				COMP_AND_GET3("SUPEXTTO",
						pts_t42->supextto);
				COMP_AND_GET3("MAXNUMTCHS",
						pts_t42->maxnumtchs);
				COMP_AND_GET3("SHAPESTRENGTH",
						pts_t42->shapestrength);
			} while (1);
		} else if (f_version >= 21) {
			proci_tsuppression_t42_ver2_config_t *pts_t42 = NULL;
			print_debug("PROCI_TOUCHSUPPRESSION_T42_INSTANCE_0\n");
			if (0 == instance_num)
				pts_t42 = &pconfig->config.\
					  tsuppression_ver2_t42;
			else
				pts_t42 = &pconfig->config.\
					  tsuppression1_ver2_t42;

			do {
				if (mxt_get_data(&mxt_string, \
							temp, STRING_SIZE) < 0)
					break;

				COMP_AND_GET3("CTRL",
						pts_t42->ctrl);
				COMP_AND_GET3("APPRTHR",
						pts_t42->apprthr);
				COMP_AND_GET3("MAXAPPRAREA",
						pts_t42->maxapprarea);
				COMP_AND_GET3("MAXTCHAREA",
						pts_t42->maxtcharea);
				COMP_AND_GET3("SUPSTRENGTH",
						pts_t42->supstrength);
				COMP_AND_GET3("SUPEXTTO",
						pts_t42->supextto);
				COMP_AND_GET3("MAXNUMTCHS",
						pts_t42->maxnumtchs);
				COMP_AND_GET3("SHAPESTRENGTH",
						pts_t42->shapestrength);
				COMP_AND_GET3("SUPDIST",
						pts_t42->supdist);
				COMP_AND_GET3("DISTHYST",
						pts_t42->disthyst);
			} while (1);
		}
	} else if (object_num == MXT_SPT_CTECONFIG_T46) {
		print_debug("SPT_CTECONFIG_T46_INSTANCE_0\n");
		do {
			if (mxt_get_data(&mxt_string, temp, STRING_SIZE) < 0)
				break;

			if (f_version <= 10) {
#define T46	cteconfig_t46
				COMP_AND_GET2("CTRL", T46.ctrl);
				COMP_AND_GET2("RESERVED", T46.mode);
				COMP_AND_GET2("IDLESYNCSPERX",
						T46.idlesyncsperx);
				COMP_AND_GET2("ACTVSYNCSPERX",
						T46.actvsyncsperx);
				COMP_AND_GET2("ADCSPERSYNC",
						T46.adcspersync);
				COMP_AND_GET2("PULSESPERADC",
						T46.pulsesperadc);
				COMP_AND_GET2("XSLEW", T46.xslew);
				COMP_AND_GET2("SYNCDELAY",
						T46.syncdelay);
			}

			else if (f_version >= 21) {
#define VER2_T46 cteconfig_ver2_t46
				COMP_AND_GET2("CTRL",
						VER2_T46.ctrl);
				COMP_AND_GET2("RESERVED",
						VER2_T46.reserved);
				COMP_AND_GET2("IDLESYNCSPERX",
						VER2_T46.idlesyncsperx);
				COMP_AND_GET2("ACTVSYNCSPERX",
						VER2_T46.actvsyncsperx);
				COMP_AND_GET2("ADCSPERSYNC",
						VER2_T46.adcspersync);
				COMP_AND_GET2("PULSESPERADC",
						VER2_T46.pulsesperadc);
				COMP_AND_GET2("XSLEW", VER2_T46.xslew);
				COMP_AND_GET2("SYNCDELAY",
						VER2_T46.syncdelay);
				COMP_AND_GET2("XVOLTAGE",
						VER2_T46.xvoltage);
			}
		} while (1);
	} else if (object_num == MXT_PROCI_STYLUS_T47) {
		proci_stylus_t47_config_t *ps_t47 = NULL;
		print_debug("PROCI_STYLUS_T47_INSTANCE_0\n");
		if (0 == instance_num)
			ps_t47 = &pconfig->config.stylus_t47;
		else
			ps_t47 = &pconfig->config.stylus1_t47;
		do {
			if (mxt_get_data(&mxt_string, temp, STRING_SIZE) < 0)
				break;

			COMP_AND_GET3("CTRL", ps_t47->ctrl);
			COMP_AND_GET3("CONTMIN", ps_t47->contmin);
			COMP_AND_GET3("CONTMAX", ps_t47->contmax);
			COMP_AND_GET3("STABILITY", ps_t47->stability);
			COMP_AND_GET3("MAXTCHAREA", ps_t47->maxtcharea);
			COMP_AND_GET3("AMPLTHR", ps_t47->amplthr);
			COMP_AND_GET3("STYSHAPE", ps_t47->styshape);
			COMP_AND_GET3("HOVERSUP", ps_t47->hoversup);
			COMP_AND_GET3("CONFTHR", ps_t47->confthr);
			COMP_AND_GET3("SYNCSPERX", ps_t47->syncsperx);
		} while (1);
	} else if (object_num == MXT_PROCG_NOISESUPPRESSION_T48) {
		print_debug("PROCG_NOISESUPPRESSION_T48_INSTANCE_0\n");
		do {
			if (mxt_get_data(&mxt_string, temp, STRING_SIZE) < 0)
				break;

			if (f_version <= 10) {
#define T48	noisesuppression_t48
				COMP_AND_GET2("CTRL", T48.ctrl);
				COMP_AND_GET2("CFG", T48.cfg);
				COMP_AND_GET2("CALCFG", T48.calcfg);
				COMP_AND_GET2("BASEFREQ", T48.basefreq);
				COMP_AND_GET2("MFFREQ[0]", T48.mffreq_2);
				COMP_AND_GET2("MFFREQ[1]", T48.mffreq_3);
				COMP_AND_GET2("GCACTVINVLDADCS",
						T48.gcactvinvldadcs);
				COMP_AND_GET2("GCIDLEINVLDADCS",
						T48.gcidleinvldadcs);
				COMP_AND_GET2("GCMAXADCSPERX",
						T48.gcmaxadcsperx);
				COMP_AND_GET2("GCLIMITMIN", T48.gclimitmin);
				COMP_AND_GET2("GCLIMITMAX", T48.gclimitmax);
				COMP_AND_GET2("GCCOUNTMINTGT",
						T48.gccountmintgt);
				COMP_AND_GET2("MFINVLDDIFFTHR",
						T48.mfinvlddiffthr);
				COMP_AND_GET2("MFINCADCSPXTHR",
						T48.mfincadcspxthr);
				COMP_AND_GET2("MFERRORTHR", T48.mferrorthr);
				COMP_AND_GET2("SELFREQMAX", T48.selfreqmax);
				/* T9 settings */
				COMP_AND_GET2("BLEN[0]", T48.blen0);
				COMP_AND_GET2("TCHTHR[0]", T48.tchthr0);
				COMP_AND_GET2("TCHDI[0]", T48.tchdi0);
				COMP_AND_GET2("MOVHYSTI[0]", T48.movhysti0);
				COMP_AND_GET2("MOVHYSTN[0]", T48.movhystn0);
				COMP_AND_GET2("MOVFILTER[0]", T48.movfilter0);
				COMP_AND_GET2("NUMTOUCH[0]", T48.numtouch0);
				COMP_AND_GET2("MRGHYST[0]", T48.mrghyst0);
				COMP_AND_GET2("MRGTHR[0]", T48.mrgthr0);
				COMP_AND_GET2("XLOCLIP[0]", T48.xloclip0);
				COMP_AND_GET2("XHICLIP[0]", T48.xhiclip0);
				COMP_AND_GET2("YLOCLIP[0]", T48.yloclip0);
				COMP_AND_GET2("YHICLIP[0]", T48.yhiclip0);
				COMP_AND_GET2("XEDGECTRL[0]", T48.xedgectrl0);
				COMP_AND_GET2("XEDGEDIST[0]", T48.xedgedist0);
				COMP_AND_GET2("YEDGECTRL[0]", T48.yedgectrl0);
				COMP_AND_GET2("YEDGEDIST[0]", T48.yedgedist0);
				COMP_AND_GET2("JUMPLIMIT[0]", T48.jumplimit0);
				COMP_AND_GET2("TCHHYST[0]", T48.tchhyst0);
				COMP_AND_GET2("NEXTTCHDI[0]", T48.nexttchdi0);
				COMP_AND_GET2("BLEN[1]", T48.blen1);
				COMP_AND_GET2("TCHTHR[1]", T48.tchthr1);
				COMP_AND_GET2("TCHDI[1]", T48.tchdi1);
				COMP_AND_GET2("MOVHYSTI[1]", T48.movhysti1);
				COMP_AND_GET2("MOVHYSTN[1]", T48.movhystn1);
				COMP_AND_GET2("MOVFILTER[1]", T48.movfilter1);
				COMP_AND_GET2("NUMTOUCH[1]", T48.numtouch1);
				COMP_AND_GET2("MRGHYST[1]", T48.mrghyst1);
				COMP_AND_GET2("MRGTHR[1]", T48.mrgthr1);
				COMP_AND_GET2("XLOCLIP[1]", T48.xloclip1);
				COMP_AND_GET2("XHICLIP[1]", T48.xhiclip1);
				COMP_AND_GET2("YLOCLIP[1]", T48.yloclip1);
				COMP_AND_GET2("YHICLIP[1]", T48.yhiclip1);
				COMP_AND_GET2("XEDGECTRL[1]", T48.xedgectrl1);
				COMP_AND_GET2("XEDGEDIST[1]", T48.xedgedist1);
				COMP_AND_GET2("YEDGECTRL[1]", T48.yedgectrl1);
				COMP_AND_GET2("YEDGEDIST[1]", T48.yedgedist1);
				COMP_AND_GET2("JUMPLIMIT[1]", T48.jumplimit1);
				COMP_AND_GET2("TCHHYST[1]", T48.tchhyst1);
				COMP_AND_GET2("NEXTTCHDI[1]", T48.nexttchdi1);
			}

			else if (f_version >= 21) {
#define VER2_T48	nsuppression_ver2_t48
				COMP_AND_GET2("CTRL", VER2_T48.ctrl);
				COMP_AND_GET2("CFG", VER2_T48.cfg);
				COMP_AND_GET2("CALCFG", VER2_T48.calcfg);
				COMP_AND_GET2("BASEFREQ", VER2_T48.basefreq);
				COMP_AND_GET2("RESERVED[0]", VER2_T48.reserved0);
				COMP_AND_GET2("RESERVED[1]", VER2_T48.reserved1);
				COMP_AND_GET2("RESERVED[2]", VER2_T48.reserved2);
				COMP_AND_GET2("RESERVED[3]", VER2_T48.reserved3);
				COMP_AND_GET2("MFFREQ[0]", VER2_T48.mffreq_2);
				COMP_AND_GET2("MFFREQ[1]", VER2_T48.mffreq_3);
				COMP_AND_GET2("NLGAIN", VER2_T48.nlgain);
				COMP_AND_GET2("NLTHR", VER2_T48.nlthr);
				COMP_AND_GET2("RESERVED", VER2_T48.reserved4);
				COMP_AND_GET2("GCACTVINVLDADCS",
						VER2_T48.gcactvinvldadcs);
				COMP_AND_GET2("GCIDLEINVLDADCS",
						VER2_T48.gcidleinvldadcs);
				COMP_AND_GET2("RESERVED[0]", VER2_T48.reserved5);
				COMP_AND_GET2("RESERVED[1]", VER2_T48.reserved6);
				COMP_AND_GET2("GCMAXADCSPERX",
						VER2_T48.gcmaxadcsperx);
				COMP_AND_GET2("GCLIMITMIN", VER2_T48.gclimitmin);
				COMP_AND_GET2("GCLIMITMAX", VER2_T48.gclimitmax);
				COMP_AND_GET2("RESERVED[0]", VER2_T48.reserved7);
				COMP_AND_GET2("RESERVED[1]", VER2_T48.reserved8);
				COMP_AND_GET2("MFINVLDDIFFTHR",
						VER2_T48.mfinvlddiffthr);
				COMP_AND_GET2("RESERVED[0]", VER2_T48.reserved9);
				COMP_AND_GET2("RESERVED[1]", VER2_T48.reserved10);
				COMP_AND_GET2("RESERVED[2]", VER2_T48.reserved11);
				COMP_AND_GET2("RESERVED[3]", VER2_T48.reserved12);
				COMP_AND_GET2("SELFREQMAX", VER2_T48.selfreqmax);
				COMP_AND_GET2("CFG2", VER2_T48.cfg2);
				COMP_AND_GET2("RESERVED[0]", VER2_T48.reserved13);
				COMP_AND_GET2("RESERVED[1]", VER2_T48.reserved14);
				COMP_AND_GET2("RESERVED[2]", VER2_T48.reserved15);
				COMP_AND_GET2("RESERVED[3]", VER2_T48.reserved16);
				/* T9 settings */
				COMP_AND_GET2("BLEN[0]", T48.blen0);
				COMP_AND_GET2("TCHTHR[0]", T48.tchthr0);
				COMP_AND_GET2("TCHDI[0]", T48.tchdi0);
				COMP_AND_GET2("MOVHYSTI[0]", T48.movhysti0);
				COMP_AND_GET2("MOVHYSTN[0]", T48.movhystn0);
				COMP_AND_GET2("MOVFILTER[0]", T48.movfilter0);
				COMP_AND_GET2("NUMTOUCH[0]", T48.numtouch0);
				COMP_AND_GET2("MRGHYST[0]", T48.mrghyst0);
				COMP_AND_GET2("MRGTHR[0]", T48.mrgthr0);
				COMP_AND_GET2("XLOCLIP[0]", T48.xloclip0);
				COMP_AND_GET2("XHICLIP[0]", T48.xhiclip0);
				COMP_AND_GET2("YLOCLIP[0]", T48.yloclip0);
				COMP_AND_GET2("YHICLIP[0]", T48.yhiclip0);
				COMP_AND_GET2("XEDGECTRL[0]", T48.xedgectrl0);
				COMP_AND_GET2("XEDGEDIST[0]", T48.xedgedist0);
				COMP_AND_GET2("YEDGECTRL[0]", T48.yedgectrl0);
				COMP_AND_GET2("YEDGEDIST[0]", T48.yedgedist0);
				COMP_AND_GET2("JUMPLIMIT[0]", T48.jumplimit0);
				COMP_AND_GET2("TCHHYST[0]", T48.tchhyst0);
				COMP_AND_GET2("NEXTTCHDI[0]", T48.nexttchdi0);
				COMP_AND_GET2("BLEN[1]", T48.blen1);
				COMP_AND_GET2("TCHTHR[1]", T48.tchthr1);
				COMP_AND_GET2("TCHDI[1]", T48.tchdi1);
				COMP_AND_GET2("MOVHYSTI[1]", T48.movhysti1);
				COMP_AND_GET2("MOVHYSTN[1]", T48.movhystn1);
				COMP_AND_GET2("MOVFILTER[1]", T48.movfilter1);
				COMP_AND_GET2("NUMTOUCH[1]", T48.numtouch1);
				COMP_AND_GET2("MRGHYST[1]", T48.mrghyst1);
				COMP_AND_GET2("MRGTHR[1]", T48.mrgthr1);
				COMP_AND_GET2("XLOCLIP[1]", T48.xloclip1);
				COMP_AND_GET2("XHICLIP[1]", T48.xhiclip1);
				COMP_AND_GET2("YLOCLIP[1]", T48.yloclip1);
				COMP_AND_GET2("YHICLIP[1]", T48.yhiclip1);
				COMP_AND_GET2("XEDGECTRL[1]", T48.xedgectrl1);
				COMP_AND_GET2("XEDGEDIST[1]", T48.xedgedist1);
				COMP_AND_GET2("YEDGECTRL[1]", T48.yedgectrl1);
				COMP_AND_GET2("YEDGEDIST[1]", T48.yedgedist1);
				COMP_AND_GET2("JUMPLIMIT[1]", T48.jumplimit1);
				COMP_AND_GET2("TCHHYST[1]", T48.tchhyst1);
				COMP_AND_GET2("NEXTTCHDI[1]", T48.nexttchdi1);
			}
		} while (1);
	} else if (object_num == MXT_TOUCH_PROXIMITY_T52) {
		touch_proximity_t52_config_t *tpx_t52 = NULL;
		print_debug("TOUCH_PROXKEY_T52_INSTANCE_0\n");
		if (0 == instance_num)
			tpx_t52 = &pconfig->config.proximity_t52;
		else
			tpx_t52 = &pconfig->config.proximity1_t52;
		do {
			if (mxt_get_data(&mxt_string, temp, STRING_SIZE) < 0)
				break;

			COMP_AND_GET3("CTRL", tpx_t52->ctrl);
			COMP_AND_GET3("XORIGIN", tpx_t52->xorigin);
			COMP_AND_GET3("YORIGIN", tpx_t52->yorigin);
			COMP_AND_GET3("YORIGIN", tpx_t52->yorigin);
			COMP_AND_GET3("RESERVED[0]", tpx_t52->reserved0);
			COMP_AND_GET3("RESERVED[1]", tpx_t52->reserved1);
			COMP_AND_GET3("AKSCFG", tpx_t52->askcfg);
			COMP_AND_GET3("RESERVED[2]", tpx_t52->reserved2);
			COMP_AND_GET3("FXDDTHR", tpx_t52->fxddthr);
			COMP_AND_GET3("FXDDI", tpx_t52->fxddi);
			COMP_AND_GET3("AVERAGE", tpx_t52->average);
			COMP_AND_GET3("MVNULLRATE", tpx_t52->mvnullrate);
			COMP_AND_GET3("MVDTHR", tpx_t52->mvdthr);
		} while (1);
	} else if (object_num == MXT_ADAPTIVE_THRESHOLD_T55) {
		proci_adaptivethreshold_t55_config_t *tpx_t55 = NULL;
		print_debug("ADAPTIVE_THRESHOLD_T55_INSTANCE_0\n");
		if (instance_num == 0)
			tpx_t55 = &pconfig->config.adaptive_threshold_t55;
		else
			tpx_t55 = &pconfig->config.adaptive_threshold1_t55;
		do {
			if (mxt_get_data(&mxt_string, temp, STRING_SIZE) < 0)
				break;

			COMP_AND_GET3("CTRL", tpx_t55->ctrl);
			COMP_AND_GET3("TARGETTHR", tpx_t55->targetthr);
			COMP_AND_GET3("THRADJLIM", tpx_t55->thradjlim);
			COMP_AND_GET3("RESETSTEPTIME", tpx_t55->resetsteptime);
			COMP_AND_GET3("FORCECHGDIST", tpx_t55->forcechgdist);
			COMP_AND_GET3("FORCECHGTIME", tpx_t55->forcechgtime);
			COMP_AND_GET3("LOWESTTHR", tpx_t55->lowestthr);
		} while (1);
	} else if (object_num == MXT_SHIELDLESS_T56) {
		print_debug("PROCI_SHIELDLESS_T56_INSTANCE_0\n");
		do {
#define T56	shieldless_t56
			if (mxt_get_data(&mxt_string, temp, STRING_SIZE) < 0)
				break;

			COMP_AND_GET2("CTRL", T56.ctrl);
			COMP_AND_GET2("COMMAND", T56.command);
			COMP_AND_GET2("OPTINT", T56.optint);
			COMP_AND_GET2("INTTIME", T56.inttime);
			COMP_AND_GET2("INTDELAY[0]", T56.intdelay[0]);
			COMP_AND_GET2("INTDELAY[1]", T56.intdelay[1]);
			COMP_AND_GET2("INTDELAY[2]", T56.intdelay[2]);
			COMP_AND_GET2("INTDELAY[3]", T56.intdelay[3]);
			COMP_AND_GET2("INTDELAY[4]", T56.intdelay[4]);
			COMP_AND_GET2("INTDELAY[5]", T56.intdelay[5]);
			COMP_AND_GET2("INTDELAY[6]", T56.intdelay[6]);
			COMP_AND_GET2("INTDELAY[7]", T56.intdelay[7]);
			COMP_AND_GET2("INTDELAY[8]", T56.intdelay[8]);
			COMP_AND_GET2("INTDELAY[9]", T56.intdelay[9]);
			COMP_AND_GET2("INTDELAY[10]", T56.intdelay[10]);
			COMP_AND_GET2("INTDELAY[11]", T56.intdelay[11]);
			COMP_AND_GET2("INTDELAY[12]", T56.intdelay[12]);
			COMP_AND_GET2("INTDELAY[13]", T56.intdelay[13]);
			COMP_AND_GET2("INTDELAY[14]", T56.intdelay[14]);
			COMP_AND_GET2("INTDELAY[15]", T56.intdelay[15]);
			COMP_AND_GET2("INTDELAY[16]", T56.intdelay[16]);
			COMP_AND_GET2("INTDELAY[17]", T56.intdelay[17]);
			COMP_AND_GET2("MULTICUTGC", T56.multicutgc);
			COMP_AND_GET2("RESERVED", T56.reserved);
			COMP_AND_GET2("NCNCL", T56.ncncl);
			COMP_AND_GET2("TOUCHBIAS", T56.touchbias);
			COMP_AND_GET2("BASESCALE", T56.basescale);
			COMP_AND_GET2("SHIFTLIMIT", T56.shiftlimit);
		} while (1);
	} else if (object_num == MXT_EXTRA_TOUCH_SCREEN_DATA_T57) {
		proci_extratouchscreendata_t57_config_t *tpx_t57 = NULL;
		print_debug("EXTRA_TOUCH_SCREEN_DATA_T57_INSTANCE_0\n");
		if (instance_num == 0)
			tpx_t57 = &pconfig->config.exttouchscreendata_t57;
		else
			tpx_t57 = &pconfig->config.exttouchscreendata_t57;
		do {
			if (mxt_get_data(&mxt_string, temp, STRING_SIZE) < 0)
				break;

			COMP_AND_GET3("CTRL", tpx_t57->ctrl);
			COMP_AND_GET3("AREATHR", tpx_t57->areathr);
			COMP_AND_GET3("AREAHYST", tpx_t57->areahyst);
		} while (1);
	}

parse_object_data_out:
	if (ret > 0) {
		ret = 0;
		dev_dbg(dev, "parsing number is %d", ret);
	} else if (ret < 0) {
		if (ret == -GET_DATA_ERR_SECTION)
			dev_err(dev, "invalid section");
		else if (ret == -GET_DATA_ERR_OVERFLOW)
			dev_err(dev, "line is overflow");
		else if (ret == -GET_DATA_ERR_INPARAM)
			dev_err(dev, "invalid parameters");
		else if (ret == -GET_DATA_ERR_EOF)
			ret = 0; /* this is not an error */
	}

	return ret;
}

static int compare_and_getvalue(const char *org, const char *cmp, int *res)
{
	char *p = NULL;
	if (!org || !cmp || !res)
		return COM_AND_VAL_ERR;

	p = strchr(org, '=');
	if (NULL == p)
		return COM_AND_VAL_ERR;

	/* It's to check where @org has @cmp string correctly */
	if (!strncmp(org, cmp, p - org)) {
		*res = (int)simple_strtoul(p + 1, NULL, 10);
		return COM_AND_VAL_NEXT;
	}

	return COM_AND_VAL_RET;
}

static void mxt_trim_endws(char *str)
{
	int size = 0;

	if (str) {
		size = strlen(str);
		if ((size >= 1) && ((str[size - 1] == '\r') ||
				(str[size - 1] == '\n')))
			str[size - 1] = '\0';
		if ((size >= 2) && ((str[size - 2] == '\r') ||
				(str[size - 2] == '\n')))
			str[size - 2] = '\0';
	}
}

static int mxt_check_section(char *str)
{
	int size, section;

	size = strlen(str);
	if ((size >= 3) && ((str[0] == '[') &&
			(str[size - 1] == ']')))
		section = 1;
	else
		section = 0;

	return section;
}

static int mxt_get_data(char **org, char *mxt_string, int str_size)
{
	char *line_str;

	if (!org)
		return -GET_DATA_ERR_EOF;

	if (!mxt_string || (str_size <= 0))
		return -GET_DATA_ERR_INPARAM;

	line_str = strsep(org, "\n");
	if (NULL == line_str) {
		line_str = strsep(org, "\0");
		if (NULL == line_str)
			return -GET_DATA_ERR_EOF;
	}

	if (strlen(line_str) > str_size)
		return -GET_DATA_ERR_OVERFLOW;

	mxt_trim_endws(line_str);
	if (mxt_check_section(line_str))
		return -GET_DATA_ERR_SECTION;

	strcpy(mxt_string, line_str);

	return GET_DATA_ERR_NONE;
}

/**** external functions ****/

int request_parse(struct device *dev,
		char *data, struct mxt_config_t *pconfig, bool verbose)
{
	int ret = 0, i;
	int section = 0;
	int object_max = 0;
	char *line_str;

	/* check the parameters */
	if (!dev || !data || !pconfig) {
		pr_err("invalid parameter, dev:%p, data:%p, pconfig:%p",
				dev, data, pconfig);
		return -1;
	}

	memset(pconfig, 0, sizeof(struct mxt_config_t));
	object_max = sizeof(object_info)/sizeof(object_info[0]);

	/* get a section line */
	line_str = strsep(&data, "\n");
	if (NULL == line_str) {
		dev_err(dev, "invalid format:%s", line_str);
		return -1;
	}
	mxt_trim_endws(line_str);
	section = mxt_check_section(line_str);
	if (0 == section) {
		dev_err(dev, "the first line should be a section\n");
		return -1;
	}

	for (i = 0; i < object_max; i++) {
		if (!strcmp(line_str, object_info[i].name)) {
			memcpy(&pconfig->obj_info, &object_info[i],
					sizeof(struct mxt_object_info_t));
			break;
		}
	}

	if (i < object_max) {
		ret = parse_object_data(dev, data, pconfig);
		if (ret == 0) {
			if (verbose)
				print_result(pconfig);
		} else
			dev_err(dev, "failed to parse data");
	} else {
		dev_dbg(dev, "invalid section name :%s\n", line_str);
	}

	return ret;
}
