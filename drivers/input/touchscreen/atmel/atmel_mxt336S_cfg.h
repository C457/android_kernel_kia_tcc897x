/*
*  drivers/input/touchscreen/atmel_mxt336S_cfg.h
*
*  Copyright (c) 2010 Samsung Electronics Co., LTD.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
*/
#ifndef ATMEL_MXT336S_CFG_H
#define ATMEL_MXT336S_CFG_H

/**                            
 ** @author sjpark@cleinsoft    
 ** @date 2014/11/18
 ** firmware frame len info size
 **/
#define FWFRAME_INFO_LEN (2)  

enum {
	/* use in_kernel mode only for test */
	MXT336S_FIRM_IN_KERNEL = 0,
	MXT336S_FIRM_EXTERNAL,
	MXT336S_FIRM_EXTERNAL_NEW,
};

enum {
	/* use in_kernel mode only for test */
	MXT540E_FIRM_IN_KERNEL = 0,
	MXT540E_FIRM_EXTERNAL,
};

typedef struct {
	uint8_t reset;       /*  Force chip reset             */
	uint8_t backupnv;    /*  Force backup to eeprom/flash */
	uint8_t calibrate;   /*  Force recalibration          */
	uint8_t reportall;   /*  Force all objects to report  */
	uint8_t reserved;
	uint8_t diagnostic;  /*  Controls the diagnostic object */
} __packed gen_commandprocessor_t6_config_t;

typedef struct {
	/*  Idle power mode sleep length in ms           */
	uint8_t idleacqint;
	/*  Active power mode sleep length in ms         */
	uint8_t actvacqint;
	/*  Active to idle power mode delay length in units of 0.2s */
	uint8_t actv2idleto;

	// sjpark : add new field
	uint8_t cfg;
	
} __packed gen_powerconfig_t7_config_t;

typedef struct {
	/*  Charge-transfer dwell time             */
	uint8_t chrgtime;
	/*  reserved                               */
	uint8_t reserved;
	/*  Touch drift compensation period        */
	uint8_t tchdrift;
	/*  Drift suspend time                     */
	uint8_t driftst;
	/*  Touch automatic calibration delay in units of 0.2s*/
	uint8_t tchautocal;
	/*  Measurement synchronisation control    */
	uint8_t sync;
	/*  recalibration suspend time after last detection */
	uint8_t atchcalst;
	/*  Anti-touch calibration suspend threshold */
	uint8_t atchcalsthr;
	/*  Anti-touch force calibration threshold */
	uint8_t atchcalfrcthr;
	/*  Anti-touch force calibration ratio */
	uint8_t atchcalfrcratio;
} __packed gen_acquisitionconfig_t8_config_t;

typedef struct {
	/** Screen Configuration **/
	/*  ACENABLE LCENABLE Main configuration field  */
	uint8_t ctrl;

	/** Physical Configuration **/
	/*  LCMASK ACMASK Object x start position on matrix  */
	uint8_t xorigin;
	/*  LCMASK ACMASK Object y start position on matrix  */
	uint8_t yorigin;
	/*  LCMASK ACMASK Object x size (i.e. width)         */
	uint8_t xsize;
	/*  LCMASK ACMASK Object y size (i.e. height)        */
	uint8_t ysize;

	/** Detection Configuration **/
	/*  Adjacent key suppression config     */
	uint8_t akscfg;
	/*  Sets the gain of the analog circuits in front of the ADC.
	 *  The gain should be set in conjunction with the burst length to
	 *  optimize the signal acquisition. Maximum gain values for a given
	 *  object/burst length can be obtained following a full calibration
	 *  of the system. GAIN has a maximum setting of 4;
	 *  settings above 4 are capped at 4.*/
	uint8_t blen;
	/*  ACMASK Threshold for all object channels   */
	uint8_t tchthr;
	/*  Detect integration config           */
	uint8_t tchdi;

	/*  LCMASK Controls flipping and rotating of touchscreen object */
	uint8_t orient;
	/*  Timeout on how long a touch might ever stay
	 *  merged - units of 0.2s, used to tradeoff power
	 *  consumption against being able to detect a touch
	 *  de-merging early */
	uint8_t mrgtimeout;

	/** Position Filter Configuration **/
	/*  Movement hysteresis setting used after touchdown */
	uint8_t movhysti;
	/*  Movement hysteresis setting used once dragging   */
	uint8_t movhystn;
	/*  Position filter setting controlling the rate of  */
	uint8_t movfilter;

	/** Multitouch Configuration **/
	/*  The number of touches that the screen will attempt to track */
	uint8_t numtouch;
	/*  The hysteresis applied on top of the merge threshold
	 *  to stop oscillation */
	uint8_t mrghyst;
	/*  The threshold for the point when two peaks are
	 *  considered one touch */
	uint8_t mrgthr;

	uint8_t amphyst;          /*  TBD */

	/** Resolution Controls **/
	uint16_t xrange;       /*  LCMASK */
	uint16_t yrange;       /*  LCMASK */
	uint8_t xloclip;       /*  LCMASK */
	uint8_t xhiclip;       /*  LCMASK */
	uint8_t yloclip;       /*  LCMASK */
	uint8_t yhiclip;       /*  LCMASK */
	/* edge correction controls */
	uint8_t xedgectrl;     /*  LCMASK */
	uint8_t xedgedist;     /*  LCMASK */
	uint8_t yedgectrl;     /*  LCMASK */
	uint8_t yedgedist;     /*  LCMASK */
	uint8_t jumplimit;
	uint8_t tchhyst;
	uint8_t xpitch;
	uint8_t ypitch;
	uint8_t nexttchdi;
	uint8_t cfg;
} __packed touch_multitouchscreen_t9_config_t;

typedef struct {
	/** Key Array Configuration **/
	/*  ACENABLE LCENABLE Main configuration field           */
	uint8_t ctrl;

	/** Physical Configuration **/
	/*  ACMASK LCMASK Object x start position on matrix  */
	uint8_t xorigin;
	/*  ACMASK LCMASK Object y start position on matrix  */
	uint8_t yorigin;
	/*  ACMASK LCMASK Object x size (i.e. width)         */
	uint8_t xsize;
	/*  ACMASK LCMASK Object y size (i.e. height)        */
	uint8_t ysize;

	/** Detection Configuration **/
	/*  Adjacent key suppression config     */
	uint8_t akscfg;
	/*  ACMASK Burst length for all object channels*/
	uint8_t blen;
	/*  ACMASK LCMASK Threshold for all object channels   */
	uint8_t tchthr;
	/*  Detect integration config           */
	uint8_t tchdi;
	/*  Spare x2 */
	uint8_t reserved[2];
} __packed touch_keyarray_t15_config_t;

typedef struct {
	uint8_t  ctrl;
	uint8_t  cmd;
} __packed spt_comcconfig_t18_config_t;

/* GPIOPWM Configuration */
typedef struct {
	uint8_t ctrl;             /*  Main configuration field           */
	uint8_t reportmask;       /*  Event mask for generating messages
				      to the host */
	uint8_t dir;              /*  Port DIR register   */
	uint8_t intpullup;        /*  Port pull-up per pin enable register */
	uint8_t out;              /*  Port OUT register*/
	uint8_t wake;             /*  Port wake on change enable register  */
	uint8_t pwm;              /*  Port pwm enable register    */
	uint8_t period;           /*  PWM period (min-max) percentage*/
	uint8_t duty[4];          /*  PWM duty cycles percentage */
	uint8_t trigger[4];       /*  Trigger for GPIO */
} __packed spt_gpiopwm_t19_config_t;

typedef struct {
	uint8_t ctrl;
	uint8_t xlogrip;
	uint8_t xhigrip;
	uint8_t ylogrip;
	uint8_t yhigrip;
	uint8_t maxtchs;
	uint8_t reserved;
	uint8_t szthr1;
	uint8_t szthr2;
	uint8_t shpthr1;
	uint8_t shpthr2;
	uint8_t supextto;
} __packed proci_gripfacesuppression_t20_config_t;


typedef struct {
	uint8_t ctrl;
	uint8_t reserved;
	uint8_t reserved1;
	int16_t gcaful;
	int16_t gcafll;
	uint8_t actvgcafvalid;        /* LCMASK */
	uint8_t noisethr;
	uint8_t reserved2;
	uint8_t freqhopscale;
	uint8_t freq[5u];
	uint8_t idlegcafvalid;        /* LCMASK */
} __packed procg_noisesuppression_t22_config_t;

typedef struct {
	/** Prox Configuration **/
	/*  ACENABLE LCENABLE Main configuration field           */
	uint8_t ctrl;

	/** Physical Configuration **/
	/*  ACMASK LCMASK Object x start position on matrix  */
	uint8_t xorigin;
	/*  ACMASK LCMASK Object y start position on matrix  */
	uint8_t yorigin;

	uint8_t reserved0;
	uint8_t reserved1;

	uint8_t askcfg;

	uint8_t reserved2;

	/*  Fixed detection threshold   */
	uint16_t fxddthr;

	/*  Fixed detection integration  */
	uint8_t fxddi;
	/*  Acquisition cycles to be averaged */
	uint8_t average;
	/*  Movement nulling rate */
	uint16_t mvnullrate;
	/*  Movement detection threshold */
	uint16_t mvdthr;
} __packed touch_proximity_t52_config_t;


typedef struct {
	uint8_t ctrl;
	uint8_t numgest;
	uint16_t gesten;
	uint8_t process;
	uint8_t tapto;
	uint8_t flickto;
	uint8_t dragto;
	uint8_t spressto;
	uint8_t lpressto;
	uint8_t reppressto;
	uint16_t flickthr;
	uint16_t dragthr;
	uint16_t tapthr;
	uint16_t throwthr;
} __packed proci_onetouchgestureprocessor_t24_config_t;

typedef struct {
	uint8_t ctrl;
	uint8_t numgest;
	uint8_t reserved2;
	uint8_t gesten;
	uint8_t rotatethr;
	uint16_t zoomthr;
} __packed proci_twotouchgestureprocessor_t27_config_t;

typedef struct {
	uint16_t upsiglim;              /* LCMASK */
	uint16_t losiglim;              /* LCMASK */
} siglim_t;

/*! = Config Structure = */

typedef struct {
	uint8_t  ctrl;                 /* LCENABLE */
	uint8_t  cmd;
	siglim_t siglim[3];            /* T9, T15, T23 */
} __packed spt_selftest_t25_config_t;

/* firmware 2.1
 * self-test structure */
typedef struct {
	uint8_t  ctrl;                 /* LCENABLE */
	uint8_t  cmd;
	siglim_t siglim[3];            /* T9, T15, T23 */
	uint8_t pindwellus;
} __packed spt_selftest_t25_ver2_config_t;

typedef struct {
	uint8_t ctrl;          /*  Ctrl field reserved for future expansion */
	uint8_t cmd;           /*  Cmd field for sending CTE commands */
	uint8_t mode;          /*  LCMASK CTE mode configuration field */
	/*  LCMASK The global gcaf number of averages when idle */
	uint8_t idlegcafdepth;
	/*  LCMASK The global gcaf number of averages when active */
	uint8_t actvgcafdepth;
	int8_t  voltage;
} __packed spt_cteconfig_t28_config_t;

typedef struct {
	uint8_t data[64];
} __packed spt_userdata_t38_t;

/* MXT540E Added */

typedef struct {
	uint8_t ctrl;          /*  Reserved/ GRIPMODE/ Reserved/ ENABLE */
	uint8_t xlogrip;       /*  Grip suppression X low boundary   */
	uint8_t xhigrip;       /*  Grip suppression X high boundary  */
	uint8_t ylogrip;       /*  Grip suppression Y low boundary   */
	uint8_t yhigrip;       /*  Grip suppression Y high boundary  */
} __packed proci_gripsuppression_t40_config_t;

/* firmware 1.0
 * T42 touch suppression */
typedef struct {
	uint8_t ctrl;            /*  ctrl field reserved for future expansion */
	uint8_t apprthr;         /*  Approach threshold */
	uint8_t maxapprarea;     /*  Maximum approach area threshold */
	uint8_t maxtcharea;      /*  Maximum touch area threshold */
	uint8_t supstrength;     /*  Suppression aggressiveness */
	uint8_t supextto;        /*  Suppression extension timeout */
	uint8_t maxnumtchs;      /*  Maximum touches */
	uint8_t shapestrength;   /*  Shaped-based aggressiveness */
} __packed proci_touchsuppression_t42_config_t;

/* firmware 2.1
 * T42 touch suppression */
typedef struct {
	uint8_t ctrl;            /*  ctrl field reserved for future expansion */
	uint8_t apprthr;         /*  Approach threshold */
	uint8_t maxapprarea;     /*  Maximum approach area threshold */
	uint8_t maxtcharea;      /*  Maximum touch area threshold */
	uint8_t supstrength;     /*  Suppression aggressiveness */
	uint8_t supextto;        /*  Suppression extension timeout */
	uint8_t maxnumtchs;      /*  Maximum touches */
	uint8_t shapestrength;   /*  Shaped-based aggressiveness */
	uint8_t supdist;	 /* add firmware 2.1 suppression distance */
	uint8_t disthyst;	 /* add firmware 2.1 suppression distance
				    hysteresis */
} __packed proci_tsuppression_t42_ver2_config_t;

/* firmware 1.0 T46 */
typedef struct {
	/*  ctrl field reserved for future expansion */
	uint8_t ctrl;
	/*  X line start position   */
	uint8_t mode;
	/*  Number of sets of ADC conversions per X when idle  */
	uint8_t idlesyncsperx;
	/*  Number of sets of ADC conversions per X when active*/
	uint8_t actvsyncsperx;
	/*  Number of ADC conversions per sync edge            */
	uint8_t adcspersync;
	/*  Number of pulses for each ADC conversion           */
	uint8_t pulsesperadc;
	/*  X pulse slew rate                                  */
	uint8_t xslew;
	uint16_t syncdelay;
	uint8_t xvoltage;
} __packed spt_cteconfig_t46_config_t;

/* firmware 2.1 T46 */
typedef struct {
	/*  ctrl field reserved for future expansion */
	uint8_t ctrl;
	/*  X line start position   */
	uint8_t reserved;
	/*  Number of sets of ADC conversions per X when idle  */
	uint8_t idlesyncsperx;
	/*  Number of sets of ADC conversions per X when active*/
	uint8_t actvsyncsperx;
	/*  Number of ADC conversions per sync edge            */
	uint8_t adcspersync;
	/*  Number of pulses for each ADC conversion           */
	uint8_t pulsesperadc;
	/*  X pulse slew rate                                  */
	uint8_t xslew;
	uint16_t syncdelay;
	uint8_t xvoltage;
} __packed spt_cteconfig_t46_ver2_config_t;

typedef struct {
	uint8_t ctrl;          /*  Reserved ENABLE            */
	uint8_t contmin;       /*  Minimum contact diameter   */
	uint8_t contmax;       /*  Maximum contact diameter   */
	uint8_t stability;     /*  Stability                  */
	uint8_t maxtcharea;    /*  Maximum touch are          */
	uint8_t amplthr;       /*  Maximum touch amplitude    */
	uint8_t styshape;      /*  Stylus shape adjustment    */
	uint8_t hoversup;      /*  Hovering finger suppression*/
	uint8_t confthr;       /*  Confidence threshold       */
	uint8_t syncsperx;     /*  ADC sets per X             */
} __packed proci_stylus_t47_config_t;


/* firmware 1.0 T48 */
typedef struct {
	/*  Reserved RPTAPX RPTFREQ RPTEN ENABLE             */
	uint8_t ctrl;
	/*  Reserved GCMODE                                  */
	uint8_t cfg;
	/*  INCRST INCBIAS Reserved FIXFREQ MFEN NLEN        */
	uint8_t calcfg;
	/*  Base sampling frequency                          */
	uint8_t basefreq;
	/*  Frequency Hopping frequency 0                    */
	uint8_t freq_0;
	/*  Frequency Hopping frequency 1                    */
	uint8_t freq_1;
	/*  Frequency Hopping frequency 2                    */
	uint8_t freq_2;
	/*  Frequency Hopping frequency 3                    */
	uint8_t freq_3;
	/*  Median Filter frequency for second filter frame  */
	uint8_t mffreq_2;
	/*  Median Filter frequency for third filter frame   */
	uint8_t mffreq_3;
	/*  GAIN Reserved                                    */
	uint8_t nlgain;
	/*  Noise line threshold                             */
	uint8_t nlthr;
	/*  Grass cut limit                                  */
	uint8_t gclimit;
	/*  Grass cut valid ADCs                             */
	uint8_t gcactvinvldadcs;
	/*  Grass cut valid threshold                        */
	uint8_t gcidleinvldadcs;
	/*  Grass-cutting source threshold                   */
	uint16_t gcinvalidthr;
	/*  Max ADCs per X line                              */
	uint8_t gcmaxadcsperx;
	uint8_t gclimitmin;
	uint8_t gclimitmax;
	uint16_t gccountmintgt;
	uint8_t mfinvlddiffthr;
	uint16_t mfincadcspxthr;
	uint16_t mferrorthr;
	uint8_t selfreqmax;
	uint8_t reserved9;
	uint8_t reserved10;
	uint8_t reserved11;
	uint8_t reserved12;
	uint8_t reserved13;
	uint8_t reserved14;
	uint8_t blen0;
	uint8_t tchthr0;
	uint8_t tchdi0;
	uint8_t movhysti0;
	uint8_t movhystn0;
	uint8_t movfilter0;
	uint8_t numtouch0;
	uint8_t mrghyst0;
	uint8_t mrgthr0;
	uint8_t xloclip0;
	uint8_t xhiclip0;
	uint8_t yloclip0;
	uint8_t yhiclip0;
	uint8_t xedgectrl0;
	uint8_t xedgedist0;
	uint8_t yedgectrl0;
	uint8_t yedgedist0;
	uint8_t jumplimit0;
	uint8_t tchhyst0;
	uint8_t nexttchdi0;
	uint8_t blen1;
	uint8_t tchthr1;
	uint8_t tchdi1;
	uint8_t movhysti1;
	uint8_t movhystn1;
	uint8_t movfilter1;
	uint8_t numtouch1;
	uint8_t mrghyst1;
	uint8_t mrgthr1;
	uint8_t xloclip1;
	uint8_t xhiclip1;
	uint8_t yloclip1;
	uint8_t yhiclip1;
	uint8_t xedgectrl1;
	uint8_t xedgedist1;
	uint8_t yedgectrl1;
	uint8_t yedgedist1;
	uint8_t jumplimit1;
	uint8_t tchhyst1;
	uint8_t nexttchdi1;
} __packed procg_noisesuppression_t48_config_t;

/* change noise suppression(T48) in firmware 2.1 */
typedef struct {
	/*  Reserved RPTAPX RPTFREQ RPTEN ENABLE             */
	uint8_t ctrl;
	/*  Reserved GCMODE                                  */
	uint8_t cfg;
	/*  INCRST INCBIAS Reserved FIXFREQ MFEN NLEN        */
	uint8_t calcfg;
	/*  Base sampling frequency                          */
	uint8_t basefreq;
	/*  Frequency Hopping frequency 0                    */
	uint8_t reserved0;
	/*  Frequency Hopping frequency 1                    */
	uint8_t reserved1;
	/*  Frequency Hopping frequency 2                    */
	uint8_t reserved2;
	/*  Frequency Hopping frequency 3                    */
	uint8_t reserved3;
	/*  Median Filter frequency for second filter frame  */
	uint8_t mffreq_2;
	/*  Median Filter frequency for third filter frame   */
	uint8_t mffreq_3;
	/*  GAIN Reserved                                    */
	uint8_t nlgain;
	/*  Noise line threshold                             */
	uint8_t nlthr;
	/*  Grass cut limit                                  */
	uint8_t reserved4;
	/*  Grass cut valid ADCs                             */
	uint8_t gcactvinvldadcs;
	/*  Grass cut valid threshold                        */
	uint8_t gcidleinvldadcs;
	/*  Grass-cutting source threshold                   */
	uint8_t reserved5;
	uint8_t reserved6;
	/*  Max ADCs per X line                              */
	uint8_t gcmaxadcsperx;
	uint8_t gclimitmin;
	uint8_t gclimitmax;
	uint8_t reserved7;
	uint8_t reserved8;
	uint8_t mfinvlddiffthr;
	uint8_t reserved9;
	uint8_t reserved10;
	uint8_t reserved11;
	uint8_t reserved12;
	uint8_t selfreqmax;
	uint8_t cfg2;
	uint8_t reserved13;
	uint8_t reserved14;
	uint8_t reserved15;
	uint8_t reserved16;
	uint8_t reserved17;
	uint8_t blen0;
	uint8_t tchthr0;
	uint8_t tchdi0;
	uint8_t movhysti0;
	uint8_t movhystn0;
	uint8_t movfilter0;
	uint8_t numtouch0;
	uint8_t mrghyst0;
	uint8_t mrgthr0;
	uint8_t xloclip0;
	uint8_t xhiclip0;
	uint8_t yloclip0;
	uint8_t yhiclip0;
	uint8_t xedgectrl0;
	uint8_t xedgedist0;
	uint8_t yedgectrl0;
	uint8_t yedgedist0;
	uint8_t jumplimit0;
	uint8_t tchhyst0;
	uint8_t nexttchdi0;
	uint8_t blen1;
	uint8_t tchthr1;
	uint8_t tchdi1;
	uint8_t movhysti1;
	uint8_t movhystn1;
	uint8_t movfilter1;
	uint8_t numtouch1;
	uint8_t mrghyst1;
	uint8_t mrgthr1;
	uint8_t xloclip1;
	uint8_t xhiclip1;
	uint8_t yloclip1;
	uint8_t yhiclip1;
	uint8_t xedgectrl1;
	uint8_t xedgedist1;
	uint8_t yedgectrl1;
	uint8_t yedgedist1;
	uint8_t jumplimit1;
	uint8_t tchhyst1;
	uint8_t nexttchdi1;
} __packed procg_nsuppression_t48_ver2_config_t;


/* add firmware 2.1 */
typedef struct {
	uint8_t ctrl;
	uint8_t targetthr;
	uint8_t thradjlim;
	uint8_t resetsteptime;
	uint8_t forcechgdist;
	uint8_t forcechgtime;
	uint8_t lowestthr;
} __packed proci_adaptivethreshold_t55_config_t;

/* add firmware 2.1 */
typedef struct {
	uint8_t ctrl;
	uint8_t command;
	uint8_t optint;
	uint8_t inttime;
	uint8_t intdelay[24];
	uint8_t multicutgc;
	uint8_t reserved;
	uint8_t ncncl;
	uint8_t touchbias;
	uint8_t basescale;
	uint8_t shiftlimit;
	uint16_t ylonoisemul;
	uint16_t ylonoisediv;
	uint16_t yhinoisemul;
	uint16_t yhinoisediv;
} __packed proci_shieldless_t56_config_t;

/* add firmware 2.1 */
typedef struct {
	uint8_t ctrl;
	uint8_t areathr;
	uint8_t areahyst;
} __packed proci_extratouchscreendata_t57_config_t;

/* \brief to save the registers which is for calibration/plam-recovery
 *	in runmode */
struct mxt_runmode_registers_t {
	/* T8 */
	uint8_t t8_atchcalst;
	uint8_t t8_atchcalsthr;
	uint8_t t8_atchfrccalthr;
	uint8_t t8_atchfrccalratio;

	/* T9 */
	/* uint8_t t9_numtouch; */
	/* uint8_t t9_tchthr; */

	/* T42 */
	/* uint8_t t42_maxnumtchs; */

	/* T38 */
	/** palm **/
	uint8_t t38_palm_check_flag;
	uint8_t t38_palm_param[4];
	/** check_chip_calibration **/
	uint8_t t38_cal_thr;
	uint8_t t38_num_of_antitouch;
	uint8_t t38_atchcalst;
	uint8_t t38_atchcalsthr;
	uint8_t t38_atchfrccalthr;
	uint8_t t38_atchfrccalratio;
	/** suppression timer **/
	uint8_t t38_supp_ops;
};


/**
 * @author sjpark@cleinsoft
 * mXT336S_AT T62
 **/
/* firmware 1.0 T62 */
typedef struct {

	uint8_t ctrl;			//T62_CTRL;
	uint8_t calcfg1;		//T62_CALCFG1
    uint8_t calcfg2;		//T62_CALCFG2
	uint8_t calcfg3;		//T62_CALCFG3
	uint8_t cfg;			//T62_CFG1
	uint8_t reserved0;		//T62_RESRVED0
	uint8_t minthradj;		//T62_MINTHRADJ
	uint8_t basefreq;		//T62_BASEFREQ
	uint8_t maxselfreq;		//T62_MAXSELFREQ
	uint8_t freq0;			//T62_FREQ0
	uint8_t freq1;			//T62_FREQ1
	uint8_t freq2;			//T62_FREQ2
	uint8_t freq3;			//T62_FREQ3
	uint8_t freq4;			//T62_FREQ4
	uint8_t hopcnt;			//T62_HOPCNT
	uint8_t reserved1;		//T62_RESERVED1
	uint8_t hopcntper;		//T62_HOPCNTPER
	uint8_t hopevalto;		//T62_HOPEVALTO
	uint8_t hopst;			//T62_HOPST
	uint8_t nlgain;			//T62_NLGAIN
	uint8_t minnlthr;		//T62_MINNLTHR
	uint8_t incnlthr;		//T62_INCNLTHR
	uint8_t adcperxthr;		//T62_ADCPERXTHR
	uint8_t nlthrmargin;	//T62_NLTHRMARGIN
	uint8_t maxadcperx;		//T62_MAXADCPERX
	uint8_t actvadcsvldnod;	//T62_ACTVADCSVLDNOD
	uint8_t idleadcsvldnod;	//T62_IDLEADCSVLDNOD
	uint8_t mingclimit;		//T62_MINGCLIMIT
	uint8_t maxgclimit;		//T62_MAXGCLIMIT
	uint8_t reserved2;		//T62_RESERVED2
	uint8_t reserved3;		//T62_RESERVED3
	uint8_t reserved4;		//T62_RESERVED4
	uint8_t reserved5;		//T62_RESERVED5
	uint8_t reserved6;		//T62_RESERVED6
	uint8_t blen0;			//T62_BLEN
	uint8_t tchthr0;		//T62_TCHTHR
	uint8_t tchdi0;			//T62_TCHDI
	uint8_t movhysti0;		//T62_MOVHYSTI
	uint8_t movhystn0;		//T62_MOVHYSTN
	uint8_t movfilter0;		//T62_MOVFILTER
	uint8_t numtouch0;		//T62_NUMTOUCH
	uint8_t mrghyst0;		//T62_MRGHYST
	uint8_t mrgthr0;		//T62_MRGTHR
	uint8_t xloclip0;		//T62_XLOCLIP
	uint8_t xhiclip0;		//T62_XHICLIP
	uint8_t yloclip0;		//T62_YLOCLIP
	uint8_t yhiclip0;		//T62_YHICLIP
	uint8_t xedgectrl0;		//T62_XEDGECTRL
	uint8_t xedgedist0;		//T62_XEDGEDIST
	uint8_t yedgectrl0;		//T62_YEDGECTRL
	uint8_t yedgedist0;		//T62_YEDGEDIST
	uint8_t jumplimit0;		//T62_JUMPLIMIT
	uint8_t tchhyst0;		//T62_TCHHYST
	uint8_t nexttchdi0;		//T62_NEXTTCHDI

} __packed procg_noisesuppression_t62_config_t;

int mxt_config_settings(struct mxt_data *mxt, u8 *mem, int which, unsigned int touch_type);

int mxt_set_config_file_641t(int object_id,int offset,int width,int data,int instance);

int mxt_set_config_file_641td(int object_id,int offset,int width,int data,int instance);

int mxt_get_object_values(struct mxt_data *mxt, int obj_type);
int mxt_copy_object(struct mxt_data *mxt, u8 *buf, int obj_type);

/* \brief get the object's registers
 *
 * @param mxt struct mxt Pointers
 * @param buf a buffer which is to save the registers
 * @param buf_size @buf's size
 * @param obj_type object number
 */
int mxt_get_objects(struct mxt_data *mxt, u8 *buf, int buf_size, int obj_type);

/* \brief load firmware
 * @param dev struct device pointer
 * @param fn firmware name
 * @flag in-kernel firmware, external firmware or test
 * @return if success 0, otherwise < 0
 */
int mxt_load_firmware(struct device *dev, const char *fn, const char* path, int flag);

/* \brief change i2c address
 * @param mxt struct mxt pointer
 * @to_boot to boot : true, to app : false
 * */
void mxt_change_i2c_addr(struct mxt_data *mxt, bool to_boot);

/* \brief get the registers from file, and apply them
 *
 * @param mxt struct mxt_data pointer
 * @param buf a buffer for some registers
 * @param size @buf's size
 */
int mxt_load_registers(struct mxt_data *mxt, const char *buf, int size);
int mxt_power_config(struct mxt_data *mxt, u8 *mem, int which);
int mxt_multitouch_config(struct mxt_data *mxt, u8 *mem, int which);

/* \brief save some registers for runmode
 *
 * In runmode, as soon as boot-up, read the registers and save them
 *
 * @param mxt struct mxt_data pointer
 */
int mxt_config_save_runmode(struct mxt_data *mxt);

/* \brief get runmeode-registers
 *
 * @param regs struct mxt_runmode_registers_t pointer to save them
 */
void mxt_config_get_runmode_registers(struct mxt_runmode_registers_t *regs);

extern int mxt_noisesuppression_t48_config(struct mxt_data *mxt);
extern int mxt_noisesuppression_t48_config_for_TA(struct mxt_data *mxt);

#define TOMEM	0
#define TODEV	1

#endif  /* ATMEL_MXT336S_CFG_H */
