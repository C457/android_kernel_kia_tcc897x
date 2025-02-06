/*
*  Atmel maXTouch header file
*
*  Copyright (c) 2010 Iiro Valkonen <iiro.valkonen@atmel.com>
*  Copyright (c) 2010 Ulf Samuelsson <ulf.samuelsson@atmel.com>
*
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License version 2 or 3 as
*  published by the Free Software Foundation.
*  See the file "COPYING" in the main directory of this archive
*  for more details.
*
*/

#ifndef ATMEL_MXT336S_H
#define ATMEL_MXT336S_H

#define INCLUDE_LCD_TOUCHKEY

#ifdef CONFIG_DAUDIO_KK
#include <mach/daudio.h>
#include <mach/daudio_info.h>
#include <mach/daudio_pinctl.h>
#include <mach/gpio.h>
#include "serdes.h"
#elif defined(CONFIG_WIDE_PE_COMMON)
#include <mobis/daudio.h>
#include <mobis/daudio_info.h>
#include <mobis/daudio_pinctl.h>
#include <mobis/serdes/serdes.h>
#include <linux/tcc_gpio.h>
#endif

#include <linux/semaphore.h>
#ifdef CONFIG_DAUDIO_KK
#include <linux/earlysuspend.h>
#endif
#include <linux/wakelock.h>
#include <linux/input.h>
/* For Special Test */
//valky #undef MXT_FIRMUP_ENABLE /* for auto firmware upgrade */
#define MXT_FIRMUP_ENABLE /* for auto firmware upgrade */
#undef MXT_SELF_TEST
#if 0
#define MXT_TUNNING_ENABLE  /* for TS tunning using console  */
#endif

#undef VERIFY_OCTAL_PERMISSIONS
#define VERIFY_OCTAL_PERMISSIONS(perms) (perms)

//#define MXT_CONFIG_RAW_FILE
#define MXT_CONFIG_XCFG_FILE

//#define MXT_JIG_DETECT
#define MXT_JIG_DETECT_DISABLE	0
#define MXT_JIG_DETECT_ENABLE	1

/* For Regulator */
/* #define ENABLE_CONTROL_REGULATOR */


/*For Performance*/
//#define MXT_DRIVER_FILTER
/* i'll use only threaded irq
 * i remove this option by #if 0 because of too complicated */
#define MXT_THREADED_IRQ

/*Normal Feature*/
#define MXT_SLEEP_POWEROFF

#if 0
#define MXT_TSP_ENABLE
#endif
/* TSP Key Feature*/
#define NUMOFKEYS	2
#define NUMOF4KEYS	4
#define KEY_PRESS	1
#define KEY_RELEASE	0

/* 2 Touch key array */
#define TOUCH_KEY_NULL		0x00
#define	TOUCH_2KEY_MENU		0x01
#define	TOUCH_2KEY_BACK		0x02

/* 4 Touch key array */
#define	TOUCH_4KEY_MENU		0x01
#define	TOUCH_4KEY_HOME		0x02
#define	TOUCH_4KEY_BACK		0x03
#define	TOUCH_4KEY_SEARCH	0x04

/* 7 Touch key array */
#if defined(INCLUDE_LCD_TOUCHKEY )
#define KEY_INVALID					KEY_CNT
#define	TOUCH_9KEY_VOLUMEDOWN		0x80
#define	TOUCH_9KEY_VOLUMEUP			0x40
#define	TOUCH_9KEY_MAP				0x20
#define	TOUCH_9KEY_NAVI				0x10
#define	TOUCH_9KEY_RADIO			0x08
#define	TOUCH_9KEY_MEDIA			0x04
#define	TOUCH_9KEY_CUSTOM			0x02
#define	TOUCH_9KEY_SETUP			0x01
#define	TOUCH_9KEY_POWER			0x100

#define	KEY9_VOLUMEDOWN				1
#define	KEY9_VOLUMEUP				2
#define	KEY9_MAP					3
#define	KEY9_NAVI					4
#define	KEY9_RADIO					5
#define	KEY9_MEDIA					6
#define	KEY9_CUSTOM					7
#define	KEY9_SETUP					8
#define	KEY9_POWER					9

#else
#define	TOUCH_7KEY_POWER		0x01
#define	TOUCH_7KEY_HOME			0x02
#define	TOUCH_7KEY_VOLUMEDOWN	0x04
#define	TOUCH_7KEY_VOLUMEUP		0x08
#define	TOUCH_7KEY_SEEKDOWN		0x10
#define	TOUCH_7KEY_TRACKUP		0x20

/* 7 key array */
#define	KEY7_POWER			229
#define	KEY7_MYMENU			270
#define	KEY7_HOME			269
#define	KEY7_VOLUMEDOWN		25
#define	KEY7_VOLUMEUP		24
#define	KEY7_SEEKDOWN		223
#define	KEY7_TRACKUP		222
#endif

/* Self Test Range */
#define TOUCH_MIN_RANGE		19500	// X0~X16, Y0~Y29
#define TOUCH_MIN_RANGE_X17 19000	// X17
#define TOUCH_MAX_RANGE 	25000	// X0~X16, Y0~Y29
#define TOUCH_REF_READ_SIZE	130		// One Page Size
#define TOUCH_DIFF_GAP		4000	// X0~X16, Y0~Y29 : diff value

#define MXT_I2C_SPEED_KHZ  400
#define MXT_I2C_MAX_LENGTH 300   /* TO_CHK: Is this a correct value? */

/* S family */
#define MAXTOUCH_FAMILYID_S				0x82

/* T family */
#define MAXTOUCH_FAMILYID				0xA4

/* mxt336s */
#define MXT336S_FIRM_VER1_VARIANTID		0x20
#define MXT336S_FIRM_VER2_VARIANTID		0x20

/* mxt641T */
#define MXT641T_FIRM_VER1_VARIANTID		0x18
#define MXT641T_FIRM_VER2_VARIANTID		0x18

/* mxt641TD */
#define MXT641TD_FIRM_VER1_VARIANTID             0x36
#define MXT641TD_FIRM_VER2_VARIANTID             0x36

/* mxt1189T */
#define MXT1189T_FIRM_VER1_VARIANTID		0x27
#define MXT1189T_FIRM_VER2_VARIANTID		0x27

#define MXT_MAX_X_VAL_12_BIT		4095
#define MXT_MAX_Y_VAL_12_BIT		4095
#define MXT_MAX_X_VAL_10_BIT		1023
#define MXT_MAX_Y_VAL_10_BIT		1023

#define MXT_MAX_REPORTED_WIDTH                          255
#define MXT_MAX_REPORTED_PRESSURE                       255

#define MXT_MAX_TOUCH_SIZE                              255
#define MXT_MAX_NUM_TOUCHES                             2  /* 2 finger */

/* Fixed addresses inside maXTouch device */
#define MXT_ADDR_INFO_BLOCK                             0
#define MXT_ADDR_OBJECT_TABLE                           7
#define MXT_ID_BLOCK_SIZE                               7
#define MXT_OBJECT_TABLE_ELEMENT_SIZE                   6

/* Object types */
#define MXT_GEN_MESSAGEPROCESSOR_T5                     5u
#define MXT_GEN_COMMANDPROCESSOR_T6                     6u
#define MXT_GEN_POWERCONFIG_T7                          7u
#define MXT_GEN_ACQUIRECONFIG_T8                        8u
#define MXT_TOUCH_MULTITOUCHSCREEN_T9                   9u
#define MXT_TOUCH_KEYARRAY_T15                          15u
#define MXT_SPT_COMMSCONFIG_T18                         18u
#define MXT_SPT_GPIOPWM_T19                             19u
#define MXT_TOUCH_PROXIMITY_T23                         23u
#define MXT_PROCI_ONETOUCHGESTUREPROCESSOR_T24          24u
#define MXT_SPT_SELFTEST_T25                            25u
#define MXT_PROCI_TWOTOUCHGESTUREPROCESSOR_T27          27u
#define MXT_DEBUG_DIAGNOSTICS_T37                       37u
#define MXT_USER_INFO_T38                               38u
#define MXT_PROCI_GRIPSUPPRESSION_T40                   40u
#define MXT_PROCI_TOUCHSUPPRESSION_T42                  42u
#define MXT_HID_CONFIG_T43								43u
#define MXT_MESSAGECOUNT_T44							44u
#define MXT_SPT_CTECONFIG_T46                           46u
#define MXT_PROCI_STYLUS_T47                            47u
#define MXT_PROCG_NOISESUPPRESSION_T48                  48u
#define MXT_PROCG_NOISESUPPRESSION_T62                  62u
#define MXT_TOUCH_PROXIMITY_T52                         52u
#define MXT_DATA_SOURCE_T53								53u
#define MXT_ADAPTIVE_THRESHOLD_T55						55u
#define MXT_SHIELDLESS_T56								56u
#define MXT_EXTRA_TOUCH_SCREEN_DATA_T57					57u
#define MXT_SPT_TIMER_T61				61u
#define MXT_SPT_DYNAMICCONFIGURATIONCONTROLLER_T70	70u
#define MXT_SPT_DYNAMICCONFIGURATIONCONTAINER_T71	71u
#define MXT_GEN_INFOBLOCK16BIT_T254                     254u

/* 641T, 1189T */
#define MXT_TOUCH_MULTITOUCHSCREEN_T100					100u
#define MXT_PROCI_TWOTOUCHGESTUREPROCESSOR_T27	27u
#define MXT_PROCG_NOISESUPPRESSION_T72		72u
#define MXT_SPT_SELFCAPGLOBALCONFIG_T109	109u

#define MXT_MAX_OBJECT_TYPES_VER1						200u //200u
#define	MXT_MAX_OBJECT_TYPES_VER2						200u //200u

//#define MXT_MAX_OBJECT_TYPES_VER1						54u
//#define	MXT_MAX_OBJECT_TYPES_VER2			63u


#if (MXT_MAX_OBJECT_TYPES_VER1 > MXT_MAX_OBJECT_TYPES_VER2)
#define MXT_MAX_OBJECT_TYPES	MXT_MAX_OBJECT_TYPES_VER1
#else
#define	MXT_MAX_OBJECT_TYPES	MXT_MAX_OBJECT_TYPES_VER2
#endif

#define MXT_CRC_NUM					3

#define MXT_END_OF_MESSAGES                             255u

/* Configuration Object Adress Fields */
/* GEN_MESSAGEPROCESSOR_T5  Address Definitions         */
/* T5 does not have any configuration */

/* GEN_COMMANDPROCESSOR_T6  Address Definitions         */
#define MXT_ADR_T6_RESET                                0x00
#define MXT_ADR_T6_BACKUPNV                             0x01
#define MXT_ADR_T6_CALIBRATE                            0x02
#define MXT_ADR_T6_REPORTALL                            0x03
#define MXT_ADR_T6_RESERVED                             0x04
#define MXT_ADR_T6_DIAGNOSTICS                          0x05

/* T6 Diagnostics Debug Command */
#define MXT_CMD_T6_PAGE_UP                              0x01
#define MXT_CMD_T6_PAGE_DOWN                            0x02
#define MXT_CMD_T6_DELTAS_MODE                          0x10
#define MXT_CMD_T6_REFERENCES_MODE                      0x11
#define MXT_CMD_T6_CTE_MODE                             0x31

/* GEN_POWERCONFIG_T7 Address Definitions               */
#define MXT_ADR_T7_IDLEACQINT                           0x00
#define MXT_ADR_T7_ACTVACQINT                           0x01
#define MXT_ADR_T7_ACTV2IDLETO                          0x02

/* GEN_ACQUIRECONFIG_T8 Address Definitions             */
#define MXT_ADR_T8_CHRGTIME                             0x00
#define MXT_ADR_T8_RESERVED                             0x01
#define MXT_ADR_T8_TCHDRIFT                             0x02
#define MXT_ADR_T8_DRIFTSTS                             0x03
#define MXT_ADR_T8_TCHAUTOCAL                           0x04
#define MXT_ADR_T8_SYNC                                 0x05
#define MXT_ADR_T8_ATCHCALST                            0x06
#define MXT_ADR_T8_ATCHCALSTHR                          0x07
/* V2.0 added */
#define MXT_ADR_T8_ATCHFRCCALTHR                        0x08
/* V2.0 added */
#define MXT_ADR_T8_ATCHFRCCALRATIO                      0x09

/* TOUCH_MULTITOUCHSCREEN_T9 Address Definitions        */
#define MXT_ADR_T9_CTRL                                 0x00
#define MXT_T9_CFGB_ENABLE(x)                           (((x) >> 0) & 0x01)
#define MXT_T9_CFGB_RPRTEN(x)                           (((x) >> 1) & 0x01)
#define MXT_T9_CFGB_DISAMP(x)                           (((x) >> 2) & 0x01)
#define MXT_T9_CFGB_DISVECT(x)                          (((x) >> 3) & 0x01)
#define MXT_T9_CFGB_DISMOVE(x)                          (((x) >> 4) & 0x01)
#define MXT_T9_CFGB_DISREL(x)                           (((x) >> 5) & 0x01)
#define MXT_T9_CFGB_DISPRSS(x)                          (((x) >> 6) & 0x01)
#define MXT_T9_CFGB_SCANEN(x)                           (((x) >> 7) & 0x01)


#define MXT_T9_ENABLE                                   (0x01)
#define MXT_T9_RPRTEN                                   (0x02)
#define MXT_T9_DISAMP                                   (0x04)
#define MXT_T9_DISVECT                                  (0x08)
#define MXT_T9_DISMOVE                                  (0x10)
#define MXT_T9_DISREL                                   (0x20)
#define MXT_T9_DISPRSS                                  (0x40)
#define MXT_T9_SCANEN                                   (0x80)

#define MXT_ADR_T9_XORIGIN                              0x01
#define MXT_ADR_T9_YORIGIN                              0x02
#define MXT_ADR_T9_XSIZE                                0x03
#define MXT_ADR_T9_YSIZE                                0x04
#define MXT_ADR_T9_AKSCFG                               0x05
#define MXT_ADR_T9_BLEN                                 0x06
#define MXT_T9_CFGBF_BL(x)                              (x & 0x0F)
#define MXT_T9_CFGBF_GAIN(x)                            ((x >> 4) & 0x0F)
#define MXT_ADR_T9_TCHTHR                               0x07
#define MXT_ADR_T9_TCHDI                                0x08
#define MXT_ADR_T9_ORIENT                               0x09
#define MXT_T9_CFGB_SWITCH(x)                           (((x) >> 0) & 0x01)
#define MXT_T9_CFGB_INVERTX(x)                          (((x) >> 1) & 0x01)
#define MXT_T9_CFGB_INVERTY(x)                          (((x) >> 2) & 0x01)
#define MXT_ADR_T9_MRGTIMEOUT                           0x0a
#define MXT_ADR_T9_MOVHYSTI                             0x0b
#define MXT_ADR_T9_MOVHYSTN                             0x0c
#define MXT_ADR_T9_MOVFILTER                            0x0d
#define MXT_T9_CFGBF_ADAPTTHR(x)                        (((x) >> 0) & 0xF)
#define MXT_T9_CFGB_DISABLE(x)                          (((x) >> 7) & 0x01)
#define MXT_ADR_T9_NUMTOUCH                             0x0e
#define MXT_ADR_T9_MRGHYST                              0x0f
#define MXT_ADR_T9_MRGTHR                               0x10
#define MXT_ADR_T9_AMPHYST                              0x11
/* 16 bit */
#define MXT_ADR_T9_XRANGE                               0x12
/* 16 bit */
#define MXT_ADR_T9_YRANGE                               0x14
#define MXT_ADR_T9_XLOCLIP                              0x16
#define MXT_ADR_T9_XHICLIP                              0x17
#define MXT_ADR_T9_YLOCLIP                              0x18
#define MXT_ADR_T9_YHICLIP                              0x19
#define MXT_ADR_T9_XEDGECTRL                            0x1a
#define MXT_ADR_T9_XEDGEDIST                            0x1b
#define MXT_ADR_T9_YEDGECTRL                            0x1c
#define MXT_ADR_T9_YEDGEDIST                            0x1d

// mxt336S_AT new
#define MXT_ADR_T9_JUMPLIMIT							0x1e
#define MXT_ADR_T9_TCHHYST								0x1f
#define MXT_ADR_T9_XPITCH								0x20
#define MXT_ADR_T9_YPITCH								0x21
#define MXT_ADR_T9_NEXTTCHDI							0x22

/* TOUCH_KEYARRAY_T15 Address Definitions               */
#define MXT_ADR_T15_CTRL                                0x00
#define MXT_T15_CFGB_ENABLE(x)                         (((x) >> 0) & 0x01)
#define MXT_T15_CFGB_RPRTEN(x)                         (((x) >> 1) & 0x01)
#define MXT_T15_CFGB_INTAKSEN(x)                       (((x) >> 7) & 0x01)
#define MXT_ADR_T15_XORIGIN                             0x01
#define MXT_ADR_T15_YORIGIN                             0x02
#define MXT_ADR_T15_XSIZE                               0x03
#define MXT_ADR_T15_YSIZE                               0x04
#define MXT_ADR_T15_AKSCFG                              0x05
#define MXT_ADR_T15_BLEN                                0x06
#define MXT_T15_CFGBF_BL(x)                             (x & 0x0F)
#define MXT_T15_CFGBF_GAIN(x)                           ((x >> 4) & 0x0F)
#define MXT_ADR_T15_TCHTHR                              0x07
#define MXT_ADR_T15_TCHDI                               0x08
#define MXT_ADR_T15_RESERVED1                           0x09
#define MXT_ADR_T15_RESERVED2                           0x0a

/* Adress Definitions for SPT_GPIOPWM_T19 Address Definitions           */
#define MXT_ADR_T19_CTRL                                0x00
#define MXT_ADR_T19_REPORTMASK                          0x01
#define MXT_ADR_T19_DIR                                 0x02
#define MXT_ADR_T19_INTPULLUP                           0x03
#define MXT_ADR_T19_OUT                                 0x04
#define MXT_ADR_T19_WAKE                                0x05
#define MXT_ADR_T19_PWM                                 0x06
#define MXT_ADR_T19_PERIOD                              0x07
/* 32 bit */
#define MXT_ADR_T19_DUTY                                0x08

/* PROCI_GRIPFACESUPPRESSION_T20 Address Definitions            */
#define MXT_ADR_T20_CTRL                                0x00
#define MXT_ADR_T20_XLOGRIP                             0x01
#define MXT_ADR_T20_XHIGRIP                             0x02
#define MXT_ADR_T20_YLOGRIP                             0x03
#define MXT_ADR_T20_YHIGRIP                             0x04
#define MXT_ADR_T20_MAXTCHS                             0x05
#define MXT_ADR_T20_RESERVED                            0x06
#define MXT_ADR_T20_SZTHR1                              0x07
#define MXT_ADR_T20_SZTHR2                              0x08
#define MXT_ADR_T20_SHPTHR1                             0x09
#define MXT_ADR_T20_SHPTHR2                             0x0a
#define MXT_ADR_T20_SUPEXTTO                            0x0b

/* PROCG_NOISESUPPRESSION_T22 Address Definitions               */
#define MXT_ADR_T22_CTRL                                0x00
/* 16 bit */
#define MXT_ADR_T22_RESERVED1_2                         0x01
/* 16 bit */
#define MXT_ADR_T22_GCAFUL                              0x03
/* 16 bit */
#define MXT_ADR_T22_GCAFLL                              0x05
#define MXT_ADR_T22_ACTVGCAFVALID                       0x07
#define MXT_ADR_T22_NOISETHR                            0x08
#define MXT_ADR_T22_RESERVED9                           0x09
#define MXT_ADR_T22_FREQHOPSCALE                        0x0A
/* 5 bytes */
#define MXT_ADR_T22_FREQ                                0x0B
#define MXT_ADR_T22_IDLEGCAFVALID                       0x10

/* TOUCH_PROXIMITY_T23 Address Definitions              */
#define MXT_ADR_T23_CTRL                                0x00
#define MXT_ADR_T23_XORIGIN                             0x01
#define MXT_ADR_T23_YORIGIN                             0x02
#define MXT_ADR_T23_XSIZE                               0x03
#define MXT_ADR_T23_YSIZE                               0x04
#define MXT_ADR_T23_RESERVED                            0x05
#define MXT_ADR_T23_BLEN                                0x06
#define MXT_ADR_T23_TCHTHR                              0x07
#define MXT_ADR_T23_TCHDI                               0x09
#define MXT_ADR_T23_AVERAGE                             0x0a
/* 16 bit */
#define MXT_ADR_T23_RATE                                0x0b

/* PROCI_ONETOUCHGESTUREPROCESSOR_T24 Address Definitions               */
#define MXT_ADR_T24_CTRL                                0x00
#define MXT_ADR_T24_NUMGEST                             0x01
/* 16 bit */
#define MXT_ADR_T24_GESTEN                              0x02
#define MXT_ADR_T24_PRESSPROC                           0x04
#define MXT_ADR_T24_TAPTO                               0x05
#define MXT_ADR_T24_FLICKTO                             0x06
#define MXT_ADR_T24_DRAGTO                              0x07
#define MXT_ADR_T24_SPRESSTO                            0x08
#define MXT_ADR_T24_LPRESSTO                            0x09
#define MXT_ADR_T24_REPPRESSTO                          0x0a
/* 16 bit */
#define MXT_ADR_T24_FLICKTHR                            0x0b
/* 16 bit */
#define MXT_ADR_T24_DRAGTHR                             0x0d
/* 16 bit */
#define MXT_ADR_T24_TAPTHR                              0x0f
/* 16 bit */
#define MXT_ADR_T24_THROWTHR                            0x11

/* SPT_SELFTEST_T25 Address Definitions         */
#define MXT_ADR_T25_CTRL                                0x00
#define MXT_ADR_T25_CMD                                 0x01
/* 16 bit */
#define MXT_ADR_T25_HISIGLIM0                           0x02
/* 16 bit */
#define MXT_ADR_T25_LOSIGLIM0                           0x04

/* PROCI_TWOTOUCHGESTUREPROCESSOR_T27 Address Definitions               */
#define MXT_ADR_T27_CTRL                                0x00
#define MXT_ADR_T27_NUMGEST                             0x01
#define MXT_ADR_T27_RESERVED2                           0x02
#define MXT_ADR_T27_GESTEN                              0x03
#define MXT_ADR_T27_ROTATETHR                           0x04

/* 16 bit */
#define MXT_ADR_T27_ZOOMTHR                             0x05

/* SPT_CTECONFIG_T28 Address Definitions                */
#define MXT_ADR_T28_CTRL                                0x00
#define MXT_ADR_T28_CMD                                 0x01
#define MXT_ADR_T28_MODE                                0x02
#define MXT_ADR_T28_IDLEGCAFDEPTH                       0x03
#define MXT_ADR_T28_ACTVGCAFDEPTH                       0x04

/* DEBUG_DIAGNOSTICS_T37 Address Definitions            */
#define MXT_ADR_T37_MODE                                0x00
#define MXT_ADR_T37_PAGE                                0x01
#define MXT_ADR_T37_DATA                                0x02

#define MXT_ADR_T38_USER0                               0
#define MXT_ADR_T38_USER1                               1
#define MXT_ADR_T38_USER2                               2
#define MXT_ADR_T38_USER3                               3
#define MXT_ADR_T38_USER4                               4
#define MXT_ADR_T38_USER5                               5
#define MXT_ADR_T38_USER25                              25
#define MXT_ADR_T38_USER26                              26
#define MXT_ADR_T38_USER27                              27
#define MXT_ADR_T38_USER28                              28
#define MXT_ADR_T38_USER30                              30
#define MXT_ADR_T38_USER31								31
#define MXT_ADR_T38_USER32								32
#define MXT_ADR_T38_USER33								33
#define MXT_ADR_T38_USER34								34
#define MXT_ADR_T38_USER35								35
#define MXT_ADR_T38_USER36								36
#define MXT_ADR_T38_USER46								46

/* PROCI_GRIPSUPPRESSION_T40 */
#define MXT_ADR_T40_CTRL                                0x00
#define MXT_ADR_T40_XLOGRIP                             0x01
#define MXT_ADR_T40_XHIGRIP                             0x02
#define MXT_ADR_T40_YLOGRIP                             0x03
#define MXT_ADR_T40_YHIGRIP                             0x04

/* PROCI_TOUCHSUPPRESSION_T42 */
#define MXT_ADR_T42_CTRL                                0x00
/* 0 (TCHTHR/4), 1 to 255 */
#define MXT_ADR_T42_APPRTHR                             0x01
/* 0 (40ch), 1 to 255 */
#define MXT_ADR_T42_MAXAPPRAREA                         0x02
/* 0 (35ch), 1 to 255 */
#define MXT_ADR_T42_MAXTCHAREA                          0x03
/* 0 (128), 1 to 255 */
#define MXT_ADR_T42_SUPSTRENGTH                         0x04
/* 0 (never expires), 1 to 255 (timeout in cycles) */
#define MXT_ADR_T42_SUPEXTTO                            0x05
/* 0 to 9 (maximum number of touches minus 1) */
#define MXT_ADR_T42_MAXNUMTCHS                          0x06
/* 0 (10), 1 to 31 */
#define MXT_ADR_T42_SHAPESTRENGTH                       0x07
/* add firmware 2.1 */
#define MXT_ADR_T42_SUPDIST								0x08
#define MXT_ADR_T42_DISTHYST							0x09

/* SPT_CTECONFIG_T46  */
#define MXT_ADR_T46_CTRL                                0x00  /* Reserved */
#define MXT_T46_CFGB_ENABLE(x)                         (((x) >> 0) & 0x01)
/* 0: 16X14Y, 1: 17X13Y, 2: 18X12Y, 3: 19X11Y,
 * 4: 20X10Y, 5: 21X15Y, 6: 22X8Y, */
#define MXT_ADR_T46_RESERVED							0x01
#define MXT_ADR_T46_IDLESYNCSPERX                       0x02
#define MXT_ADR_T46_ACTVSYNCSPERX                       0x03
#define MXT_ADR_T46_ADCSPERSYNC                         0x04
/* 0:1  1:2   2:3   3:4 pulses */
#define MXT_ADR_T46_PULSESPERADC                        0x05
/* 1:500nsec,  0:350nsec */
#define MXT_ADR_T46_XSLEW                               0x06

/* PROCI_STYLUS_T47 */
#define MXT_ADR_T47_CTRL                                0x00
#define MXT_ADR_T47_CONTMIN                             0x01
#define MXT_ADR_T47_CONTMAX                             0x02
#define MXT_ADR_T47_STABILITY                           0x03
#define MXT_ADR_T47_MAXTCHAREA                          0x04
#define MXT_ADR_T47_AMPLTHR                             0x05
#define MXT_ADR_T47_STYSHAPE                            0x06
#define MXT_ADR_T47_HOVERSUP                            0x07
#define MXT_ADR_T47_CONFTHR                             0x08
#define MXT_ADR_T47_SYNCSPERX                           0x09

/* MXT_PROCG_NOISESUPPRESSION_T48 Message address fifinitions */
/* add definition for firmware ver0.5  */

#define MXT_ADR_T48_CTRL                                0x00
#define MXT_T48_CFGB_ENABLE(x)                         (((x) >> 0) & 0x01)
#define MXT_T48_CFGB_RPRTEN(x)                         (((x) >> 1) & 0x01)
#define MXT_T48_CFGB_RPTFREQ(x)                        (((x) >> 2) & 0x01)
#define MXT_T48_CFGB_RPTAPX(x)                         (((x) >> 3) & 0x01)

#define MXT_ADR_T48_CFG									0x01

#define MXT_ADR_T48_CALCFG                              0x02
#define MXT_T48_CFGB_MFEN(x)                            (((x) >> 1) & 0x01)
#define MXT_T48_CFGB_MEANEN(x)                          (((x) >> 3) & 0x01)
#define MXT_T48_CFGB_DUALXEN(x)                         (((x) >> 4) & 0x01)
#define MXT_T48_CFGB_CHRGON(x)                          (((x) >> 5) & 0x01)
#define MXT_T48_CFGB_INCBIAS(x)                         (((x) >> 6) & 0x01)
#define MXT_T48_CFGB_INCRST(x)                          (((x) >> 7) & 0x01)

#define MXT_ADR_T48_BASEFREQ				0x03
#define MXT_ADR_T48_RESERVED0				0x04
#define MXT_ADR_T48_RESERVED1				0x05
#define MXT_ADR_T48_RESERVED2				0x06
#define MXT_ADR_T48_RESERVED3				0x07
#define MXT_ADR_T48_MFFREQ_2				0x08
#define MXT_ADR_T48_MFFREQ_3				0x09
#define MXT_ADR_T48_RESERVED4				0x0A
#define MXT_ADR_T48_RESERVED5				0x0B
#define MXT_ADR_T48_RESERVED6				0x0C
#define MXT_ADR_T48_GCACTVINVLDADCS			0x0D
#define MXT_ADR_T48_GCIDLEINVLDADCS			0x0E
#define MXT_ADR_T48_RESERVED7				0x0F
#define MXT_ADR_T48_RESERVED8				0x10
#define MXT_ADR_T48_GCMAXADCSPERX			0x11
#define MXT_ADR_T48_GCLIMITMIN				0x12
#define MXT_ADR_T48_GCLIMITMAX				0x13
#define MXT_ADR_T48_GCCOUNTMINTGT			0x14
#define MXT_ADR_T48_MFINVLDDIFFTHR			0x16
#define MXT_ADR_T48_MFINCADCSPXTHR			0x17
#define MXT_ADR_T48_MFERRORTHR				0x19
#define MXT_ADR_T48_SELFREQMAX				0x1B
#define MXT_ADR_T48_RESERVED9				0x1C
#define MXT_ADR_T48_RESERVED10				0x1D
#define MXT_ADR_T48_RESERVED11				0x1E
#define MXT_ADR_T48_RESERVED12				0x1F
#define MXT_ADR_T48_RESERVED13				0x20
#define MXT_ADR_T48_RESERVED14				0x21

/* T48 T9SETTINGS */
#define MXT_ADR_T48_BLEN					0x22
#define MXT_ADR_T48_TCHTHR					0x23
#define MXT_ADR_T48_TCHDI					0x24
#define MXT_ADR_T48_MOVHYSTI				0x25
#define MXT_ADR_T48_MOVHYSTN				0x26
#define MXT_ADR_T48_MOVFILTER				0x27
#define MXT_ADR_T48_NUMTOUCH				0x28
#define MXT_ADR_T48_MRGHYST					0x29
#define MXT_ADR_T48_MRGTHR					0x2A
#define MXT_ADR_T48_XLOCLIP					0x2B
#define MXT_ADR_T48_XHICLIP					0x2C
#define MXT_ADR_T48_YLOCLIP					0x2D
#define MXT_ADR_T48_YHICLIP					0x2E
#define MXT_ADR_T48_XEDGECTRL				0x2F
#define MXT_ADR_T48_XEDGEDIST				0x30
#define MXT_ADR_T48_YEDGECTRL				0x31
#define MXT_ADR_T48_YEDGEDIST				0x32
#define MXT_ADR_T48_JUMPLIMIT				0x33
#define MXT_ADR_T48_TCHHYST					0x34
#define MXT_ADR_T48_NEXTTCHDI				0x35

/***********************************************************************
 * MXT_PROCG_NOISESUPPRESSION_T48 Message address
 * add firmware2.1
 * *********************************************************************/
#define MXT_MSG_T48_CTRL                                0x00
#define MXT_MSG_T48_CFGB_ENABLE(x)                      (((x) >> 0) & 0x01)
#define MXT_MSG_T48_CFGB_RPRTEN(x)                      (((x) >> 1) & 0x01)
#define MXT_MSG_T48_CFGB_RPTFREQ(x)                     (((x) >> 2) & 0x01)
#define MXT_MSG_T48_CFGB_RPTAPX(x)                      (((x) >> 3) & 0x01)

#define MXT_MSG_T48_CFG									0x01

#define MXT_MSG_T48_CALCFG                              0x02
#define MXT_MSG_T48_CFGB_MFEN(x)                        (((x) >> 1) & 0x01)
#define MXT_MSG_T48_CFGB_MEANEN(x)                      (((x) >> 3) & 0x01)
#define MXT_MSG_T48_CFGB_DUALXEN(x)                     (((x) >> 4) & 0x01)
#define MXT_MSG_T48_CFGB_CHRGON(x)                      (((x) >> 5) & 0x01)
#define MXT_MSG_T48_CFGB_INCBIAS(x)                     (((x) >> 6) & 0x01)
#define MXT_MSG_T48_CFGB_INCRST(x)                      (((x) >> 7) & 0x01)

#define MXT_MSG_T48_BASEFREQ				0x03
#define MXT_MSG_T48_RESERVED0				0x04
#define MXT_MSG_T48_RESERVED1				0x05
#define MXT_MSG_T48_RESERVED2				0x06
#define MXT_MSG_T48_RESERVED3				0x07
#define MXT_MSG_T48_MFFREQ_2				0x08
#define MXT_MSG_T48_MFFREQ_3				0x09
#define MXT_MSG_T48_NLGAIN					0x0A
#define MXT_MSG_T48_NLTHR					0x0B
#define MXT_MSG_T48_RESERVED4				0x0C
#define MXT_MSG_T48_GCACTVINVLDADCS			0x0D
#define MXT_MSG_T48_GCIDLEINVLDADCS			0x0E
#define MXT_MSG_T48_RESERVED5				0x0F
#define MXT_MSG_T48_RESERVED6				0x10
#define MXT_MSG_T48_GCMAXADCSPERX			0x11
#define MXT_MSG_T48_GCLIMITMIN				0x12
#define MXT_MSG_T48_GCLIMITMAX				0x13
#define MXT_MSG_T48_RESERVED7				0x14
#define MXT_MSG_T48_MFINVLDDIFFTHR			0x16
#define MXT_MSG_T48_RESERVED8				0x17
#define MXT_MSG_T48_RESERVED9				0x19
#define MXT_MSG_T48_SELFREQMAX				0x1B
#define MXT_MSG_T48_CFG2					0x1C
#define MXT_MSG_T48_RESERVED10				0x1D
#define MXT_MSG_T48_RESERVED11				0x1E
#define MXT_MSG_T48_RESERVED12				0x1F
#define MXT_MSG_T48_RESERVED13				0x20
#define MXT_MSG_T48_RESERVED14				0x21


/************************************************************************
* MESSAGE OBJECTS ADDRESS FIELDS
*
************************************************************************/
#define MXT_MSG_REPORTID                                0x00


/* MXT_GEN_MESSAGEPROCESSOR_T5 Message address definitions              */
#define MXT_MSG_T5_REPORTID                             0x00
#define MXT_MSG_T5_MESSAGE                              0x01
#define MXT_MSG_T5_CHECKSUM                             0x08

/* MXT_GEN_COMMANDPROCESSOR_T6 Message address definitions              */
#define MXT_MSG_T6_STATUS_NORMAL                        0x00
#define MXT_MSG_T6_STATUS                               0x01
#define MXT_MSGB_T6_COMSERR                             0x04
#define MXT_MSGB_T6_CFGERR                              0x08
#define MXT_MSGB_T6_CAL                                 0x00
#define MXT_MSGB_T6_SIGERR                              0x20
#define MXT_MSGB_T6_OFL                                 0x40
#define MXT_MSGB_T6_RESET                               0x80
/* Three bytes */
#define MXT_MSG_T6_CHECKSUM                             0x02

/* MXT_GEN_POWERCONFIG_T7 NO Message address definitions                */
/* MXT_GEN_ACQUIRECONFIG_T8 Message address definitions                 */
/* MXT_TOUCH_MULTITOUCHSCREEN_T9 Message address definitions            */

#define MXT_MSG_T9_STATUS                               0x01
/* Status bit field */
#define MXT_MSGB_T9_SUPPRESS                            0x02
#define MXT_MSGB_T9_AMP                                 0x04
#define MXT_MSGB_T9_VECTOR                              0x08
#define MXT_MSGB_T9_MOVE                                0x10
#define MXT_MSGB_T9_RELEASE                             0x20
#define MXT_MSGB_T9_PRESS                               0x40
#define MXT_MSGB_T9_DETECT                              0x80

#define MXT_MSG_T9_XPOSMSB                              0x02
#define MXT_MSG_T9_YPOSMSB                              0x03
#define MXT_MSG_T9_XYPOSLSB                             0x04
#define MXT_MSG_T9_TCHAREA                              0x05
#define MXT_MSG_T9_TCHAMPLITUDE                         0x06
#define MXT_MSG_T9_TCHVECTOR                            0x07

/* MXT_TOUCH_KEYARRAY_T15 Message address definitions                   */
#define MXT_MSG_T15_STATUS                              0x01
#define MXT_MSGB_T15_DETECT                             0x80
/* 4 bytes */
#define MXT_MSG_T15_KEYSTATE                            0x02

/* MXT_SPT_GPIOPWM_T19 Message address definitions                      */
#define MXT_MSG_T19_STATUS                              0x01

/* MXT_PROCI_GRIPFACESUPPRESSION_T20 Message address definitions        */
#define MXT_MSG_T20_STATUS                              0x01
#define MXT_MSGB_T20_FACE_SUPPRESS                      0x01
/* MXT_PROCG_NOISESUPPRESSION_T22 Message address definitions           */
#define MXT_MSG_T22_STATUS                              0x01
#define MXT_MSGB_T22_FHCHG                              0x01
#define MXT_MSGB_T22_GCAFERR                            0x04
#define MXT_MSGB_T22_FHERR                              0x08
#define MXT_MSG_T22_GCAFDEPTH                           0x02

/* MXT_TOUCH_PROXIMITY_T23 Message address definitions                  */
#define MXT_MSG_T23_STATUS                              0x01
#define MXT_MSGB_T23_FALL                               0x20
#define MXT_MSGB_T23_RISE                               0x40
#define MXT_MSGB_T23_DETECT                             0x80
/* 16 bit */
#define MXT_MSG_T23_PROXDELTA                           0x02

/* MXT_PROCI_ONETOUCHGESTUREPROCESSOR_T24 Message address definitions   */
#define MXT_MSG_T24_STATUS                              0x01
#define MXT_MSG_T24_XPOSMSB                             0x02
#define MXT_MSG_T24_YPOSMSB                             0x03
#define MXT_MSG_T24_XYPOSLSB                            0x04
#define MXT_MSG_T24_DIR                                 0x05
/* 16 bit */
#define MXT_MSG_T24_DIST                                0x06

/* MXT_SPT_SELFTEST_T25 Message address definitions                     */
#define MXT_MSG_T25_STATUS                              0x01
/* 5 Bytes */
#define MXT_MSGR_T25_OK                                 0xFE
#define MXT_MSGR_T25_INVALID_TEST                       0xFD
#define MXT_MSGR_T25_NOT_TESTED							0xFC
#define MXT_MSGR_T25_AVDD_FAULT							0x01
#define MXT_MSGR_T25_PIN_FAULT                          0x12
#define MXT_MSGR_T25_SIGNAL_LIMIT_FAULT                 0x17
#define MXT_MSGR_T25_GAIN_ERROR                         0x20
#define MXT_MSGR_T25_INFO                               0x02

/* MXT_PROCI_TWOTOUCHGESTUREPROCESSOR_T27 Message address definitions   */
#define MXT_MSG_T27_STATUS                              0x01
#define MXT_MSGB_T27_ROTATEDIR                          0x10
#define MXT_MSGB_T27_PINCH                              0x20
#define MXT_MSGB_T27_ROTATE                             0x40
#define MXT_MSGB_T27_STRETCH                            0x80
#define MXT_MSG_T27_XPOSMSB                             0x02
#define MXT_MSG_T27_YPOSMSB                             0x03
#define MXT_MSG_T27_XYPOSLSB                            0x04
#define MXT_MSG_T27_ANGLE                               0x05

/* 16 bit */
#define MXT_MSG_T27_SEPARATION                          0x06

/* MXT_SPT_CTECONFIG_T28 Message address definitions                    */
#define MXT_MSG_T28_STATUS                              0x01
#define MXT_MSGB_T28_CHKERR                             0x01

/* MXT_PROCI_TOUCHSUPPRESSION_T42 Message address fifinitions */
#define MXT_MSG_T42_STATUS								0x01
#define MXT_MSGB_T42_TCHSUP				 				0x01

/* MXT_SPT_CTECONFIG_T46 Message address fifinitions */
#define MXT_MSG_T46_STATUS                               0x01
#define MXT_MSGB_T46_CHKERR                              0x01

/* MXT_DEBUG_DIAGNOSTICS_T37 NO Message address definitions             */



/* MXT_TOUCH_MULTITOUCHSCREEN_T100 Message address definitions            */

//#define MXT_MSG_T100_STATUS                               0x01
/* Status bit field */
#define MXT_MSGB_T100_SUPPRESS                            0x03
//#define MXT_MSGB_T100_AMP                               0x04
//#define MXT_MSGB_T100_VECTOR                              0x08
#define MXT_MSGB_T100_MOVE                                0x01
#define MXT_MSGB_T100_RELEASE                             0x05
#define MXT_MSGB_T100_PRESS                               0x04
#define MXT_MSGB_T100_DETECT                              0x80
/*
#define MXT_MSG_T100_XPOSMSB                              0x02
#define MXT_MSG_T100_YPOSMSB                              0x03
#define MXT_MSG_T100_XYPOSLSB                             0x04
#define MXT_MSG_T100_TCHAREA                              0x05
#define MXT_MSG_T100_TCHAMPLITUDE                         0x06
#define MXT_MSG_T100_TCHVECTOR                            0x07
*/


/* One Touch Events */
#define MT_GESTURE_RESERVED                             0x00
#define MT_GESTURE_PRESS                                0x01
#define MT_GESTURE_RELEASE                              0x02
#define MT_GESTURE_TAP                                  0x03
#define MT_GESTURE_DOUBLE_TAP                           0x04
#define MT_GESTURE_FLICK                                0x05
#define MT_GESTURE_DRAG                                 0x06
#define MT_GESTURE_SHORT_PRESS                          0x07
#define MT_GESTURE_LONG_PRESS                           0x08
#define MT_GESTURE_REPEAT_PRESS                         0x09
#define MT_GESTURE_TAP_AND_PRESS                        0x0a
#define MT_GESTURE_THROW                                0x0b

/* reset mode */
#define RESET_TO_NORMAL                                 0
#define RESET_TO_BOOTLOADER                             1

/* Bootloader states */
#define WAITING_BOOTLOAD_COMMAND                        0xC0
#define WAITING_FRAME_DATA                              0x80
#define FRAME_CRC_CHECK                                 0x02
#define FRAME_CRC_PASS                                  0x04
#define FRAME_CRC_FAIL                                  0x03
#define APP_CRC_FAIL                                    0x40
#define BOOTLOAD_STATUS_MASK                            0x3f

#define MXT_MAX_FRAME_SIZE                              532

#define MXT_ADR_T38_CFG_CTRL							0x00
#define MXT_CFG_OVERWRITE								0x00
#define MXT_CFG_DEBUG									0xA5

/* Firmware */
#define MXT336S_FIRMWARE "mxt336s.fw"

/* Self Test Result */
#define MXT_SELF_TEST_SUCCESS   0
#define MXT_SELF_TEST_FAIL              1
#define MXT_SELF_TEST_RETRY             2
#define MXT_SELF_TEST_TIMEOUT   3

#if defined (MXT_CONFIG_RAW_FILE) || defined( MXT_CONFIG_XCFG_FILE)

//******************************************************************************
/// \brief Config file types
enum mxt_config_type {
  CONFIG_RAW,
  CONFIG_XCFG
};

//******************************************************************************
/// \brief Return codes
enum mxt_rc {
  MXT_SUCCESS = 0,                           /*!< Success */
  MXT_INTERNAL_ERROR = 1,                    /*!< Internal error/assert */
  MXT_ERROR_IO = 2,                          /*!< Input/output error */
  MXT_ERROR_NO_MEM = 3,                      /*!< Memory allocation failure */
  MXT_ERROR_TIMEOUT = 4,                     /*!< Timeout */
  MXT_ERROR_NO_DEVICE = 5,                   /*!< Could not find a device or device went away */
  MXT_ERROR_ACCESS = 6,                      /*!< Permission denied */
  MXT_ERROR_NOT_SUPPORTED = 7,               /*!< Operation not allowed for this device type */
  MXT_ERROR_INTERRUPTED = 8,                 /*!< Interrupt function call */
  MXT_ERROR_OBJECT_NOT_FOUND = 9,            /*!< Object not available on device */
  MXT_ERROR_NO_MESSAGE = 10,                 /*!< Received unexpected invalid message from message processor */
  MXT_ERROR_SELF_TEST_INVALID = 11,          /*!< Self test invalid test command */
  MXT_ERROR_SELF_TEST_ANALOG = 12,           /*!< Self test AVdd Analog power is not present */
  MXT_ERROR_SELF_TEST_PIN_FAULT = 13,        /*!< Self test Pin fault */
  MXT_ERROR_SELF_TEST_AND_GATE = 14,         /*!< Self test AND Gate Fault */
  MXT_ERROR_SELF_TEST_SIGNAL_LIMIT = 15,     /*!< Self test Signal limit fault */
  MXT_ERROR_SELF_TEST_GAIN = 16,             /*!< Self test Gain error */
  MXT_ERROR_CHECKSUM_MISMATCH = 17,          /*!< Information block or config checksum error */
  MXT_ERROR_BOOTLOADER_UNLOCKED = 18,        /*!< Bootloader already unlocked */
  MXT_ERROR_BOOTLOADER_FRAME_CRC_FAIL = 19,  /*!< Bootloader CRC failure (transmission failure) */
  MXT_ERROR_FILE_FORMAT = 20,                /*!< File format error */
  MXT_FIRMWARE_UPDATE_NOT_REQUIRED = 21,     /*!< Device firmware already required version */
  MXT_ERROR_BOOTLOADER_NO_ADDRESS = 22,      /*!< Could not identify bootloader address */
  MXT_ERROR_FIRMWARE_UPDATE_FAILED = 23,     /*!< Version on device did not match version given after bootloading operation */
  MXT_ERROR_RESET_FAILURE = 24,              /*!< Device did not reset */
  MXT_ERROR_UNEXPECTED_DEVICE_STATE = 25,    /*!< Device in unexpected state */
  MXT_ERROR_BAD_INPUT = 26,                  /*!< Incorrect command line parameters or menu input given */
  MXT_ERROR_PROTOCOL_FAULT = 27,             /*!< Bridge TCP protocol parse error */
  MXT_ERROR_CONNECTION_FAILURE = 28,         /*!< Bridge connection error */
  MXT_ERROR_SERIAL_DATA_FAILURE = 29,        /*!< Serial data download failed */
  MXT_ERROR_NOENT = 30,                      /*!< No such file or directory */
  MXT_ERROR_SELFCAP_TUNE = 31,               /*!< Error processing self cap command */
  MXT_MSG_CONTINUE = 32,                     /*!< Continue processing messages */
  MXT_ERROR_RESERVED = 33,                   /*!< Reserved value */
  MXT_DEVICE_IN_BOOTLOADER = 34,             /*!< Device is in bootloader mode */
  MXT_ERROR_OBJECT_IS_VOLATILE = 35,         /*!< Object is volatile */
  MXT_ERROR_RESERVED2 = 36,                  /*!< Reserved value */
};

//******************************************************************************
/// \brief Configuration data for a single object
struct mxt_object_config {
  uint32_t type;
  uint8_t instance;
  uint32_t size;
  uint16_t start_position;
  uint8_t *data;
  struct mxt_object_config *next;
};

/*! \brief ID Information fields in the Information Block*/
struct mxt_id_info {
  uint8_t family;           /*!< Device family */
  uint8_t variant;          /*!< Device variant */

  uint8_t version;          /*!< Firmware version (Major/minor nibbles) */
  uint8_t build;            /*!< Firmware build number */

  uint8_t matrix_x_size;    /*!< Matrix X Size */
  uint8_t matrix_y_size;    /*!< Matrix Y Size */

  /*! Number of elements in the object table. The actual number of objects
   * can be different if any object has more than one instance. */
  uint8_t num_objects;
} __attribute__((packed));

//******************************************************************************
/// \brief Device configuration
struct mxt_config {
  struct mxt_id_info id;
  struct mxt_object_config *head;
  uint32_t info_crc;
  uint32_t config_crc;
  enum mxt_config_type config_type;
};

#endif


enum {
	MXT_DISABLE,
	MXT_ENABLE
};

#define MIGRATION_TO_NVODM

/* Device Info descriptor */
/* Parsed from maXTouch "Id information" inside device */
struct mxt_device_info {
	u8      family_id;
	u8      variant_id;
	u8      major;
	u8      minor;
	u8      build;
	u8      num_objs;
	u8      x_size;
	u8      y_size;
	u8      family[16];     /* Family name */
	u8      variant[16];    /* Variant name */
	u16     num_nodes;      /* Number of sensor nodes */
};

/* object descriptor table, parsed from maXTouch "object table" */
struct mxt_object {
	u8      type;
	u16     chip_addr;
	u8      size;
	u8      instances;
	u8      num_report_ids;
};

/* Mapping from report id to object type and instance */
struct report_id_map {
	u8  object;
	u8  instance;
	/*
	* This is the first report ID belonging to object. It enables us to
	* find out easily the touch number: each touch has different report
	* ID (which are assigned to touches in increasing order). By
	* subtracting the first report ID from current, we get the touch
	* number.
	*/
	u8  first_rid;
};

#ifdef ENABLE_CHARGER
struct mxt_callbacks {
	void (*inform_charger)(struct mxt_callbacks *, int mode);
};
#endif

struct mxt_platform_data {
	const char *platform_name;
	u8    numtouch; /* Number of touches to report  */
	void  (*init_platform_hw)(void *);
	void  (*exit_platform_hw)(void *);
	void  (*suspend_platform_hw)(void *);
	void  (*resume_platform_hw)(void *);
	int (*check_chg_platform_hw)(void *);

	u32	key_led_en1;
	u32	key_led_en2;
	u32	key_led_en3;
	u32	key_led_en4;

	int   max_x;    /* The default reported X range   */
	int   max_y;    /* The default reported Y range   */
	u8    (*valid_interrupt) (void);
#ifdef ENABLE_CHARGER
	void  (*register_cb)(struct mxt_callbacks *);
#endif

#ifdef ENABLE_CONTROL_REGULATOR
	const char *reg_vdd_name;
	const char *reg_avdd_name;
	const char *reg_vdd_lvsio_name;

	struct regulator *reg_vdd;	/* TSP_VDD_1V8 */
	struct regulator *reg_avdd;	/* TSP_AVDD_3V3 */
	/* TSP_VDD_LVSIO_2V8 (for level shifter) */
	struct regulator *reg_vdd_lvsio;

	int	reg_vdd_level;		/* uV range */
	int	reg_avdd_level;		/* uV range */
	int	reg_vdd_lvsio_level;	/* uV range */
#endif

	u32 irq_gpio;
	u32 irq_jig_gpio;
	u32 irq_jig_num;
	u32 board_rev;	/* for set revision check */

	u32 gpio_reset;
	u32 gpio_scl;
	u32 gpio_sda;
};

#ifdef MXT_SELF_TEST
#define MXT_SELF_TEST_FLAG_OFF	0x00
#define MXT_SELF_TEST_FLAG_ON	0x01
#define MXT_SELF_TEST_FLAG_BAD  0x02
#define MXT_SELF_TEST_FLAG_T1   0x10
#define MXT_SELF_TEST_FLAG_T2   0x20
struct mxt_self_test {
	int flag;
	struct delayed_work dwork;
	struct delayed_work check_dwork;
	struct timer_list timer;
};
#endif

struct mxt_100ms_timer {
	struct work_struct tmr_work;
	bool timer_flag;
	uint8_t timer_ticks;
	struct timer_list ts_timeout_tmr;
	struct timer_list *p_ts_timeout_tmr;
	int timer_limit;
	int error_limit;
};

struct mxt_reg_update {
	int flag;
	int verbose;
	struct timer_list tmr;
	struct delayed_work dwork;
	struct mutex lock;
};

struct mxt_supp_ops {
	struct delayed_work dwork;
};

struct mxt_cal_timer {
	struct delayed_work dwork;
};

#define MXT_CHECK_TOUCH_IC_FLAG_OFF	0x00
#define MXT_CHECK_TOUCH_IC_FLAG_ON	0x01
struct mxt_check_touch_ic {
	int flag;
	struct delayed_work dwork;
};

/* Driver datastructure */
struct mxt_data {
	struct i2c_client *client;
	struct input_dev  *input;
#if defined(INCLUDE_LCD_TOUCHKEY )
	struct input_dev  *input_key; 
#endif	
	struct mxt_platform_data *pdata;
	char              phys_name[32];
#if defined(INCLUDE_LCD_TOUCHKEY )
	char              phys_name_key[32];
#endif	
	int               irq;
	int				  jig_irq;

	u16               last_read_addr;
	bool              new_msgs;
	u8                *last_message;

	wait_queue_head_t msg_queue;
	struct semaphore  msg_sem;

#ifndef MXT_THREADED_IRQ
	int               valid_irq_counter;
	int               invalid_irq_counter;
#endif
	int               irq_counter;
	int               message_counter;
	int               read_fail_counter;


	int               bytes_to_read_ap;

	/* Protect canceling/rescheduling of dwork */
	/* spinlock_t           lock; */
	/* struct delayed_work  dwork; */
	/* struct work_struct ta_work; */
	struct mxt_100ms_timer ts_100ms_tmr;
	struct delayed_work  timer_dwork;
	struct delayed_work  config_dwork; /* palm recovery */
#ifdef MXT_SELF_TEST
	struct mxt_self_test self_test;
#endif
	struct delayed_work  timer_get_delta;

#ifdef MXT_FACTORY_TEST
	struct delayed_work  firmup_dwork;
#endif
	u8                   xpos_format;
	u8                   ypos_format;

	struct mxt_device_info  device_info;

	u32                  info_block_crc;
	u32                  configuration_crc;
	u16                  report_id_count;
	struct report_id_map *rid_map;
	struct mxt_object    *object_table;
	int		object_link[MXT_MAX_OBJECT_TYPES];

	u16                  msg_proc_addr;
	u8                   message_size;

	/* u8 debug_config[32]; */

	struct wake_lock wakelock;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend    early_suspend;
#endif
#if defined(MXT_FACTORY_TEST) || defined(MXT_FIRMUP_ENABLE)
	u8			firm_status_data;
	u8			firm_normal_status_ack;
#endif
	/* u8			check_auto_cal; */
	/* u16			set_mode_for_ta; */
	/* u16			mxt_status; */
#ifdef ENABLE_CHARGER
	struct mxt_callbacks callbacks;
#endif
	/* to show the object registers */
	int objnum_show;

	/* for the register update */
	struct mxt_reg_update reg_update;

	unsigned long mxt_time_point;
	int cal_check_flag;

	struct mutex msg_lock;
	struct mxt_supp_ops supp_ops;

	int check_auto_cal_flag;

	struct mxt_cal_timer cal_timer;
	struct mxt_check_touch_ic check_touch_ic_timer;

#ifdef MXT_JIG_DETECT
    u8 jig_detected;
#endif

	struct{
		struct delayed_work reset_dwork;
		struct mutex lock;
	} serdes;

	bool irq_enabled;
};

#if defined(INCLUDE_LCD_TOUCHKEY )
struct mxt_tsp_key {
	u16 id;
	unsigned int keycode;
};
#endif
/* Returns the start address of object in mXT memory. */
#define MXT_BASE_ADDR(object_type) \
	get_object_address(object_type, 0, mxt->object_table,\
			mxt->device_info.num_objs, mxt->object_link)

#define MXT_BASE_ADDR1(object_type) \
	get_object_address(object_type, 1, mxt->object_table,\
			mxt->device_info.num_objs, mxt->object_link)

/* Returns the size of object in mXT memory. */
#define MXT_GET_SIZE(object_type) \
	get_object_size(object_type, mxt->object_table,\
			mxt->device_info.num_objs, mxt->object_link)


/* Returns the number of object instance in mXT memory. */
#define MXT_GET_NUM_INSTANCE(object_type) \
        get_object_size(object_type, mxt->object_table,\
                        mxt->device_info.num_objs, mxt->object_link)


/* If report_id (rid) == 0, then "mxt->report_id[rid].object" will be 0. */
#define	REPORT_ID_TO_OBJECT(rid) \
(((rid) == 0xff) ? 0 : mxt->rid_map[rid].object)

#define	REPORT_ID_TO_OBJECT_NAME(rid) \
object_type_name[REPORT_ID_TO_OBJECT(rid)]

#define	T6_REG(x) (MXT_BASE_ADDR(MXT_GEN_COMMANDPROCESSOR_T6) + (x))
#define	T37_REG(x) (MXT_BASE_ADDR(MXT_DEBUG_DIAGNOSTICS_T37) +  (x))

/* ADD TRACKING_ID */
#define REPORT_MT(touch_number, x, y, amplitude, size)\
	do {\
	input_report_key(mxt->input, BTN_TOUCH, 1);\
	input_report_abs(mxt->input, ABS_MT_POSITION_X, x);\
	input_report_abs(mxt->input, ABS_MT_POSITION_Y, y);\
	input_report_abs(mxt->input, ABS_MT_TOUCH_MAJOR, amplitude);\
	input_report_abs(mxt->input, ABS_MT_WIDTH_MAJOR, size);\
	input_report_abs(mxt->input, ABS_MT_TRACKING_ID, touch_number);\
	input_mt_sync(mxt->input);\
	} while (0)

/* Routines for memory access within a 16 bit address space */
int mxt_read_byte(struct i2c_client *client, u16 addr, u8 *value);
int mxt_read_block(struct i2c_client *client, u16 addr, u16 length, u8 *value);
int mxt_write_byte(struct i2c_client *client, u16 addr, u8 value);
int mxt_write_block(struct i2c_client *client, u16 addr, u16 length, u8 *value);

extern u16 get_object_address(uint8_t object_type,
		       uint8_t instance,
		       struct mxt_object *object_table,
		       int max_objs, int *object_link);
u16 get_object_size(uint8_t object_type,
		    struct mxt_object *object_table,
		    int max_objs, int *object_link);

u16 get_object_instance(uint8_t object_type,
			struct mxt_object *object_table,
			int max_objs, int *object_link);

int sw_reset_chip(struct mxt_data *mxt, u8 mode);
int try_sw_reset_chip(struct mxt_data *mxt, u8 mode);
void hw_reset_chip(struct mxt_data *mxt);

void mxt_hw_reset(void);

extern int LCD_VER;
int mobis_touch_update_check(void);
void mobis_touch_update_complete(void);
void mobis_serdes_need_log(void);
void mobis_show_need_log_count(void);
void mobis_serdes_status_count_reset(void);
void mobis_serdes_status_count_update(u8 *serdes_check_value_arr, int lock_pin, int line_fault);
void mobis_serdes_status_count_read(char *buffer);
void mobis_serdes_status_count_write(char *buffer);
bool mobis_touch_counter_flag_check(int id);
void mobis_touch_counter_press(char *buf, int id);
void mobis_touch_counter_release(char *buf, int id);
void mobis_touch_counter_print(void);
void mobis_touch_counter_reset(void);
u8 mobis_idtc_check_read(void);
void mobis_idtc_check_write(u8 *serdes_check_value_arr, int lock_pin, int line_fault, bool touch_watchdog, bool touch_crc_err);
#if defined(CONFIG_WIDE_PE_COMMON)||defined(CONFIG_TOUCHSCREEN_ONEBIN)
int mxt336s_init(void);
void mxt336s_cleanup(void);
#endif

#endif
