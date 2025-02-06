
/**********************************************************

  DEVICE   : mxT336S  0.5.6
  CUSTOMER : MOBIS
  PROJECT  : N1
  X SIZE   : X18
  Y SIZE   : Y11
  CHRGTIME : 2.59us
  X Pitch  :
  Y Pitch  :
***********************************************************/

#ifndef MXT336S_CONFIG__
#define MXT336S_CONFIG__

#define TOUCH_TYPE_MAX 12
#define TOUCH_TYPE_7_IN 0
#define TOUCH_TYPE_8_IN 1

extern unsigned int touch_type;

#if defined(CONFIG_DAUDIO_ECO) || defined(CONFIG_DAUDIO_KK)

/* SPT_USERDATA_T38 INSTANCE 0 */
static uint8_t t38_userdata0[TOUCH_TYPE_MAX] = {'1','1','1','1','1','1','1','1','1','1','1','1'};
static uint8_t t38_userdata1[TOUCH_TYPE_MAX] = {'6','6','6','6','6','6','6','6','6','6','6','6'};
static uint8_t t38_userdata2[TOUCH_TYPE_MAX] = {'0','0','0','0','0','0','0','0','0','0','0','0'};
static uint8_t t38_userdata3[TOUCH_TYPE_MAX] = {'5','5','5','5','5','5','5','5','5','5','5','5'};
static uint8_t t38_userdata4[TOUCH_TYPE_MAX] = {'1','1','1','1','1','1','1','1','1','1','1','1'};
static uint8_t t38_userdata5[TOUCH_TYPE_MAX] = {'1','1','1','1','1','1','1','1','1','1','1','1'};
static uint8_t t38_userdata6[TOUCH_TYPE_MAX] = {'0','0','0','0','0','0','0','0','0','0','0','0'};
static uint8_t t38_userdata7[TOUCH_TYPE_MAX] = {'7','7','7','7','7','7','7','9','7','7','7','7'};

/* GEN_POWERCONFIG_T7 INSTANCE 0 */
static uint8_t t7_idleacqint[TOUCH_TYPE_MAX]   = {255,255,255,255,255,255,255,255,255,255,255,255};
static uint8_t t7_actvacqint[TOUCH_TYPE_MAX]  = {255,255,255,255,255,255,255,255,255,255,255,255};

static uint8_t t7_actv2idleto[TOUCH_TYPE_MAX]  = {255,255,255,255,255,255,255,255,255,255,255,255};
static uint8_t t7_cfg[TOUCH_TYPE_MAX]              = {0,0,0,0,0,0,0,0,0,0,0,0};

/* _GEN_ACQUISITIONCONFIG_T8 INSTANCE 0 */
static uint8_t t8_chrgtime[TOUCH_TYPE_MAX]        = {27,27,27,27,27,27,27,27,27,27,27,27};	/* 6 - 60  * 83 ns */
static uint8_t t8_reserved[TOUCH_TYPE_MAX]        = {0,0,0,0,0,0,0,0,0,0,0,0};
static uint8_t t8_tchdrift[TOUCH_TYPE_MAX]        = {5,5,5,5,5,5,5,5,5,5,5,5};
static uint8_t t8_driftst[TOUCH_TYPE_MAX]         = {1,1,1,1,1,1,1,1,1,1,1,1};
static uint8_t t8_tchautocal[TOUCH_TYPE_MAX]      = {25,25,25,25,25,25,25,25,25,25,25,25};
static uint8_t t8_sync[TOUCH_TYPE_MAX]            = {0,0,0,0,0,0,0,0,0,0,0,0};
static uint8_t t8_atchcalst[TOUCH_TYPE_MAX]       = {5,5,5,5,5,5,5,5,5,5,5,5};
static uint8_t t8_atchcalsthr[TOUCH_TYPE_MAX]     = {0,0,0,0,0,0,0,0,0,0,0,0};
static uint8_t t8_atchcalfrcthr[TOUCH_TYPE_MAX]   = {50,50,50,50,50,50,50,50,50,50,50,50};	/* V2.0 added */
static uint8_t t8_atchcalfrcratio[TOUCH_TYPE_MAX] = {25,25,25,25,25,25,25,25,25,25,25,25};	/* V2.0 added */

/* TOUCH_MULTITOUCHSCREEN_T9 INSTANCE 0 */
static uint8_t t9_ctrl[TOUCH_TYPE_MAX]       = {139,139,139,139,139,139,139,139,139,139,139,139};
static uint8_t t9_xorigin[TOUCH_TYPE_MAX]    = {0,0,0,0,0,0,0,0,0,0,0,0};
static uint8_t t9_yorigin[TOUCH_TYPE_MAX]    = {0,0,0,0,0,0,0,0,0,0,0,0};
static uint8_t t9_xsize[TOUCH_TYPE_MAX]      = {24,24,24,24,24,24,24,24,24,24,24,24};
static uint8_t t9_ysize[TOUCH_TYPE_MAX]      = {14,14,14,14,14,14,14,14,14,14,14,14};
static uint8_t t9_akscfg[TOUCH_TYPE_MAX]     = {0,0,0,0,0,0,0,0,0,0,0,0};
static uint8_t t9_blen[TOUCH_TYPE_MAX]       = {64,64,64,64,48,48,48,48,64,64,48,64};
static uint8_t t9_tchthr[TOUCH_TYPE_MAX]     = {40,40,40,40,40,40,40,40,40,40,40,40};
static uint8_t t9_tchdi[TOUCH_TYPE_MAX] 	 = {2,2,2,2,2,2,2,2,2,2,2,2};

/* touch_type 6,7 orient = 0 */
static uint8_t t9_orient[TOUCH_TYPE_MAX]     = {2,2,2,2,2,2,0,0,2,0,0,2};

static uint8_t t9_mrgtimeout[TOUCH_TYPE_MAX] = {10,10,10,10,10,10,10,10,10,10,10,10};
static uint8_t t9_movhysti[TOUCH_TYPE_MAX]   = {5,5,5,5,5,5,5,5,5,5,5,5};
static uint8_t t9_movhystn[TOUCH_TYPE_MAX]   = {5,5,5,5,5,5,5,5,5,5,5,5};
static uint8_t t9_movfilter[TOUCH_TYPE_MAX]  = {0,0,0,0,0,0,0,0,0,0,0,0};
static uint8_t t9_numtouch[TOUCH_TYPE_MAX]   = {2,2,2,2,2,2,2,2,2,2,2,2};
static uint8_t t9_mrghyst[TOUCH_TYPE_MAX]      = {5,5,5,5,5,5,5,5,5,5,5,5};
static uint8_t t9_mrgthr[TOUCH_TYPE_MAX]        = {5,5,5,5,5,5,5,5,5,5,5,5};
static uint8_t t9_amphyst[TOUCH_TYPE_MAX]    = {20,20,20,20,20,20,20,20,20,20,20,20};

/* touch_type 6,7(QXI) 1280X720*/
static uint16_t t9_xrange[TOUCH_TYPE_MAX] 	 = {799,799,799,799,799,799,1279,1279,799,799,799,799};
static uint16_t t9_yrange[TOUCH_TYPE_MAX] 	 = {479,479,479,479,479,479,719,719,479,479,479,479};

static uint8_t t9_xloclip[TOUCH_TYPE_MAX] 	 = {0,0,0,0,0,0,0,0,0,0,0,0};
static uint8_t t9_xhiclip[TOUCH_TYPE_MAX] 	 = {0,0,0,0,0,0,0,0,0,0,0,0};
static uint8_t t9_yloclip[TOUCH_TYPE_MAX] 	 = {0,0,0,0,0,0,0,0,0,0,0,0};
static uint8_t t9_yhiclip[TOUCH_TYPE_MAX]    = {0,0,0,0,0,0,0,0,0,0,0,0};
static uint8_t t9_xedgectrl[TOUCH_TYPE_MAX]  = {192,192,192,192,192,192,192,192,192,192,192,192};
static uint8_t t9_xedgedist[TOUCH_TYPE_MAX]  = {0,0,0,0,0,0,0,0,0,0,0,0};
static uint8_t t9_yedgectrl[TOUCH_TYPE_MAX]  = {192,192,192,192,192,192,192,192,192,192,192,192};
static uint8_t t9_yedgedist[TOUCH_TYPE_MAX]  = {0,0,0,0,0,0,0,0,0,0,0,0};
static uint8_t t9_jumplimit[TOUCH_TYPE_MAX]  = {30,30,30,30,30,30,30,30,30,30,30,30};
static uint8_t t9_tchhyst[TOUCH_TYPE_MAX] 	 = {10,10,10,10,10,10,10,10,10,10,10,10};	/* V2.0 or MXT224E added */
static uint8_t t9_xpitch[TOUCH_TYPE_MAX] 	 = {65,65,65,65,65,65,65,65,65,65,65,65};	/* MXT224E added */
static uint8_t t9_ypitch[TOUCH_TYPE_MAX]     = {65,65,65,65,65,65,65,65,65,65,65,65};	/* MXT224E added */
static uint8_t t9_nexttchdi[TOUCH_TYPE_MAX]  = {1,1,1,1,1,1,1,1,1,1,1,1};
static uint8_t t9_cfg[TOUCH_TYPE_MAX]        = {0,0,0,0,0,0,0,0,0,0,0,0};

/* PROCI_TOUCHSUPPRESSION_T42 INSTANCE 0 */
static uint8_t t42_ctrl[TOUCH_TYPE_MAX]          = {3,3,3,3,3,3,3,3,3,3,3,3};
static uint8_t t42_apprthr[TOUCH_TYPE_MAX]       = {32,32,32,32,32,32,32,32,32,32,32,32};	/* 0 (TCHTHR/4), 1 to 255 */
static uint8_t t42_maxapprarea[TOUCH_TYPE_MAX]   = {40,40,40,40,40,40,40,40,40,40,40,40};	/* 0 (40ch), 1 to 255 */
static uint8_t t42_maxtcharea[TOUCH_TYPE_MAX]    = {35,35,35,35,35,35,35,35,35,35,35,35};	/* 0 (35ch), 1 to 255 */
static uint8_t t42_supstrength[TOUCH_TYPE_MAX]   = {0,0,0,0,0,0,0,0,0,0,0,0};	/* 0 (128), 1 to 255 */
/* 0 (never expires), 1 to 255 (timeout in cycles) */
static uint8_t t42_supextto[TOUCH_TYPE_MAX] 	 = {0,0,0,0,0,0,0,0,0,0,0,0};
/* 0 to 9 (maximum number of touches minus 1) */
static uint8_t t42_maxnumtchs[TOUCH_TYPE_MAX]    = {4,4,4,4,4,4,4,4,4,4,4,4};
static uint8_t t42_shapestrength[TOUCH_TYPE_MAX] = {0,0,0,0,0,0,0,0,0,0,0,0};	/* 0 (10), 1 to 31 */
static uint8_t t42_supdist[TOUCH_TYPE_MAX]       = {0,0,0,0,0,0,0,0,0,0,0,0};
static uint8_t t42_disthyst[TOUCH_TYPE_MAX]      = {0,0,0,0,0,0,0,0,0,0,0,0};

/* SPT_CTECONFIG_T46 INSTANCE 0 */
static uint8_t t46_ctrl[TOUCH_TYPE_MAX] 	     = {0,0,0,0,0,0,0,0,0,0,0,0};	/*Reserved */
/*0: 16X14Y, 1: 17X13Y, 2: 18X12Y, 3: 19X11Y, 4: 20X10Y, 5: 21X15Y, 6: 22X8Y */
static uint8_t t46_mod[TOUCH_TYPE_MAX] 		     = {0,0,0,0,0,0,0,0,0,0,0,0};
static uint8_t t46_idlesyncsperx[TOUCH_TYPE_MAX] = {16,16,16,16,16,16,16,16,16,16,16,16};
static uint8_t t46_actvsyncsperx[TOUCH_TYPE_MAX] = {16,16,16,16,16,16,16,16,16,16,16,16};
static uint8_t t46_adcspersync[TOUCH_TYPE_MAX]   = {0,0,0,0,0,0,0,0,0,0,0,0};
static uint8_t t46_pulsesperadc[TOUCH_TYPE_MAX]  = {0,0,0,0,0,0,0,0,0,0,0,0};	/*0:1  1:2   2:3   3:4 pulses */
static uint8_t t46_xslew[TOUCH_TYPE_MAX]         = {1,1,1,1,1,1,1,1,1,1,1,1};	/*0:500nsec,1:350nsec,2:250nsec(firm2.1)*/
static uint8_t t46_syncdelay[TOUCH_TYPE_MAX]     = {0,0,0,0,0,0,0,0,0,0,0,0};
/* add firmware2.1 */
static uint8_t t46_xvoltage[TOUCH_TYPE_MAX]      = {1,1,1,1,1,1,1,1,1,1,1,1};

/* PROCI_SHIELDLESS_T56 INSTANCE 0 */
static uint8_t t56_ctrl[TOUCH_TYPE_MAX] 		= {3,3,3,3,3,3,3,3,3,3,3,3};
static uint8_t t56_command[TOUCH_TYPE_MAX] 	    = {0,0,0,0,0,0,0,0,0,0,0,0};
static uint8_t t56_optint[TOUCH_TYPE_MAX]       = {1,1,1,1,1,1,1,1,1,1,1,1};

static uint8_t t56_inttime[TOUCH_TYPE_MAX]      = {53,53,53,53,57,57,57,57,53,53,57,53};

static uint8_t t56_intdelay0[TOUCH_TYPE_MAX] 	= {24,24,24,24,28,28,28,28,24,24,28,24};
static uint8_t t56_intdelay1[TOUCH_TYPE_MAX] 	= {24,24,24,24,28,28,28,28,24,24,28,24};
static uint8_t t56_intdelay2[TOUCH_TYPE_MAX] 	= {24,24,24,24,28,28,28,28,24,24,28,24};
static uint8_t t56_intdelay3[TOUCH_TYPE_MAX] 	= {24,24,24,24,28,28,28,28,24,24,28,24};
static uint8_t t56_intdelay4[TOUCH_TYPE_MAX] 	= {24,24,24,24,32,32,32,32,24,24,32,24};
static uint8_t t56_intdelay5[TOUCH_TYPE_MAX] 	= {24,24,24,24,32,32,32,32,24,24,32,24};
static uint8_t t56_intdelay6[TOUCH_TYPE_MAX] 	= {24,24,24,24,32,32,32,32,24,24,32,24};
static uint8_t t56_intdelay7[TOUCH_TYPE_MAX] 	= {28,28,28,28,32,32,32,32,28,28,32,28};
static uint8_t t56_intdelay8[TOUCH_TYPE_MAX] 	= {28,28,28,28,32,32,32,32,28,28,32,28};
static uint8_t t56_intdelay9[TOUCH_TYPE_MAX] 	= {28,28,28,28,32,32,32,32,28,28,32,28};
static uint8_t t56_intdelay10[TOUCH_TYPE_MAX] 	= {28,28,28,28,32,32,32,32,28,28,32,28};
static uint8_t t56_intdelay11[TOUCH_TYPE_MAX] 	= {28,28,28,28,32,32,32,32,28,28,32,28};
static uint8_t t56_intdelay12[TOUCH_TYPE_MAX] 	= {28,28,28,28,32,32,32,32,28,28,32,28};
static uint8_t t56_intdelay13[TOUCH_TYPE_MAX] 	= {28,28,28,28,32,32,32,32,28,28,32,28};
static uint8_t t56_intdelay14[TOUCH_TYPE_MAX] 	= {28,28,28,28,32,32,32,32,28,28,32,28};
static uint8_t t56_intdelay15[TOUCH_TYPE_MAX] 	= {28,28,28,28,32,32,32,32,28,28,32,28};
static uint8_t t56_intdelay16[TOUCH_TYPE_MAX] 	= {28,28,28,28,32,32,32,32,28,28,32,28};
static uint8_t t56_intdelay17[TOUCH_TYPE_MAX] 	= {28,28,28,28,32,32,32,32,28,28,32,28};
static uint8_t t56_intdelay18[TOUCH_TYPE_MAX] 	= {28,28,28,28,32,32,32,32,28,28,32,28};
static uint8_t t56_intdelay19[TOUCH_TYPE_MAX] 	= {28,28,28,28,28,28,28,28,28,28,28,28};
static uint8_t t56_intdelay20[TOUCH_TYPE_MAX] 	= {28,28,28,28,28,28,28,28,28,28,28,28};
static uint8_t t56_intdelay21[TOUCH_TYPE_MAX] 	= {28,28,28,28,28,28,28,28,28,28,28,28};
static uint8_t t56_intdelay22[TOUCH_TYPE_MAX] 	= {28,28,28,28,28,28,28,28,28,28,28,28};
static uint8_t t56_intdelay23[TOUCH_TYPE_MAX] 	= {24,24,24,24,28,28,28,28,24,24,28,24};
static uint8_t t56_multicutgc[TOUCH_TYPE_MAX] 	= {0,0,0,0,0,0,0,0,0,0,0,0};	//2014.11.22
static uint8_t t56_reserved1[TOUCH_TYPE_MAX]    = {0,0,0,0,0,0,0,0,0,0,0,0};	//2014.11.22
static uint8_t t56_ncncl[TOUCH_TYPE_MAX] 		= {1,1,1,1,1,1,1,1,1,1,1,1};
static uint8_t t56_touchbias[TOUCH_TYPE_MAX]    = {0,0,0,0,0,0,0,0,0,0,0,0};
static uint8_t t56_basescale[TOUCH_TYPE_MAX]    = {20,20,20,20,20,20,20,20,20,20,20,20};	//2014.11.22
static uint8_t t56_shiftlimit[TOUCH_TYPE_MAX]   = {4,4,4,4,4,4,4,4,4,4,4,4};	//2014.11.22
static uint8_t t56_ylonoisemul[TOUCH_TYPE_MAX]  = {0,0,0,0,0,0,0,0,0,0,0,0};
static uint8_t t56_ylonoisediv[TOUCH_TYPE_MAX]  = {0,0,0,0,0,0,0,0,0,0,0,0};
static uint8_t t56_yhinoisemul[TOUCH_TYPE_MAX]  = {0,0,0,0,0,0,0,0,0,0,0,0};
static uint8_t t56_yhinoisediv[TOUCH_TYPE_MAX]  = {0,0,0,0,0,0,0,0,0,0,0,0};
static uint8_t t56_reserved2[TOUCH_TYPE_MAX]    = {0,0,0,0,0,0,0,0,0,0,0,0};

/* PROCG_NOISESUPPRESSION_T62 INSTANCE 0 */
static uint8_t t62_ctrl[TOUCH_TYPE_MAX]           = {3,3,3,3,3,3,3,3,3,3,3,3};
static uint8_t t62_calcfg1[TOUCH_TYPE_MAX]        = {11,11,11,11,11,11,11,11,11,11,11,11};
static uint8_t t62_calcfg2[TOUCH_TYPE_MAX]        = {0,0,0,0,0,0,0,0,0,0,0,0};
static uint8_t t62_calcfg3[TOUCH_TYPE_MAX]        = {7,7,7,7,7,7,7,7,7,7,7,7};
static uint8_t t62_cfg[TOUCH_TYPE_MAX]            = {0,0,0,0,0,0,0,0,0,0,0,0};
static uint8_t t62_reserved0[TOUCH_TYPE_MAX]      = {0,0,0,0,0,0,0,0,0,0,0,0};
static uint8_t t62_minthradj[TOUCH_TYPE_MAX]      = {32,32,32,32,32,32,32,32,32,32,32,32};
static uint8_t t62_basefreq[TOUCH_TYPE_MAX]       = {0,0,0,0,0,0,0,0,0,0,0,0};
static uint8_t t62_maxselfreq[TOUCH_TYPE_MAX]     = {25,25,25,25,25,25,25,25,25,25,25,25};
static uint8_t t62_freq0[TOUCH_TYPE_MAX]          = {5,5,5,5,5,5,5,5,5,5,5,5};
static uint8_t t62_freq1[TOUCH_TYPE_MAX]          = {10,10,10,10,10,10,10,10,10,10,10,10};
static uint8_t t62_freq2[TOUCH_TYPE_MAX]          = {15,15,15,15,15,15,15,15,15,15,15,15};
static uint8_t t62_freq3[TOUCH_TYPE_MAX]          = {20,20,20,20,20,20,20,20,20,20,20,20};
static uint8_t t62_freq4[TOUCH_TYPE_MAX]          = {24,24,24,24,24,24,24,24,24,24,24,24};
static uint8_t t62_hopcnt[TOUCH_TYPE_MAX] 		  = {5,5,5,5,5,5,5,5,5,5,5,5};
static uint8_t t62_reserved1[TOUCH_TYPE_MAX]      = {0,0,0,0,0,0,0,0,0,0,0,0};
static uint8_t t62_hopcntper[TOUCH_TYPE_MAX]      = {20,20,20,20,20,20,20,20,20,20,20,20};
static uint8_t t62_hopevalto[TOUCH_TYPE_MAX]      = {5,5,5,5,5,5,5,5,5,5,5,5};
static uint8_t t62_hopst[TOUCH_TYPE_MAX]          = {5,5,5,5,5,5,5,5,5,5,5,5};
static uint8_t t62_nlgain[TOUCH_TYPE_MAX] 	      = {70,70,70,70,70,70,70,70,70,70,70,70};
static uint8_t t62_minnlthr[TOUCH_TYPE_MAX] 	  = {20,20,20,20,20,20,20,20,20,20,20,20};
static uint8_t t62_incnlthr[TOUCH_TYPE_MAX]       = {20,20,20,20,20,20,20,20,20,20,20,20};
static uint8_t t62_adcperxthr[TOUCH_TYPE_MAX] 	  = {15,15,15,15,15,15,15,15,15,15,15,15};
static uint8_t t62_nlthrmargin[TOUCH_TYPE_MAX]    = {30,30,30,30,30,30,30,30,30,30,30,30};
static uint8_t t62_maxadcperx[TOUCH_TYPE_MAX]     = {63,63,63,63,63,63,63,63,63,63,63,63};
static uint8_t t62_actvadcsvldnod[TOUCH_TYPE_MAX] = {6,6,6,6,6,6,6,6,6,6,6,6};
static uint8_t t62_idleadcsvldnod[TOUCH_TYPE_MAX] = {6,6,6,6,6,6,6,6,6,6,6,6};
static uint8_t t62_mingclimit[TOUCH_TYPE_MAX] 	  = {10,10,10,10,10,10,10,10,10,10,10,10};
static uint8_t t62_maxgclimit[TOUCH_TYPE_MAX]     = {64,64,64,64,64,64,64,64,64,64,64,64};
static uint8_t t62_reserved2[TOUCH_TYPE_MAX]      = {0,0,0,0,0,0,0,0,0,0,0,0};
static uint8_t t62_reserved3[TOUCH_TYPE_MAX]      = {0,0,0,0,0,0,0,0,0,0,0,0};
static uint8_t t62_reserved4[TOUCH_TYPE_MAX]      = {0,0,0,0,0,0,0,0,0,0,0,0};
static uint8_t t62_reserved5[TOUCH_TYPE_MAX]      ={0,0,0,0,0,0,0,0,0,0,0,0};
static uint8_t t62_reserved6[TOUCH_TYPE_MAX]      = {0,0,0,0,0,0,0,0,0,0,0,0};
static uint8_t t62_blen0[TOUCH_TYPE_MAX] 		  = {48,48,48,48,48,48,48,48,48,48,48,48};
static uint8_t t62_tchthr0[TOUCH_TYPE_MAX] 	      = {40,40,40,40,40,40,40,40,40,40,40,40};
static uint8_t t62_tchdi0[TOUCH_TYPE_MAX]         = {2,2,2,2,2,2,2,2,2,2,2,2};
static uint8_t t62_movhysti0[TOUCH_TYPE_MAX]      = {1,1,1,1,1,1,1,1,1,1,1,1};
static uint8_t t62_movhystn0[TOUCH_TYPE_MAX]      = {1,1,1,1,1,1,1,1,1,1,1,1};
static uint8_t t62_movfilter0[TOUCH_TYPE_MAX]     = {49,49,49,49,49,49,49,49,49,49,49,49};
static uint8_t t62_numtouch0[TOUCH_TYPE_MAX]      = {2,2,2,2,2,2,2,2,2,2,2,2};
static uint8_t t62_mrghyst0[TOUCH_TYPE_MAX]       = {10,10,10,10,10,10,10,10,10,10,10,10};
static uint8_t t62_mrgthr0[TOUCH_TYPE_MAX]        = {10,10,10,10,10,10,10,10,10,10,10,10};
static uint8_t t62_xloclip0[TOUCH_TYPE_MAX]       = {7,7,7,7,5,5,5,5,7,7,5,7};
static uint8_t t62_xhiclip0[TOUCH_TYPE_MAX]       = {7,7,7,7,5,5,5,5,7,7,5,7};
static uint8_t t62_yloclip0[TOUCH_TYPE_MAX]       = {8,8,8,8,13,13,13,13,8,8,13,8};
static uint8_t t62_yhiclip0[TOUCH_TYPE_MAX]       = {8,8,8,8,13,13,13,13,8,8,13,8};
static uint8_t t62_xedgectrl0[TOUCH_TYPE_MAX] 	  = {237,237,237,237,222,222,222,222,237,237,222,237};
static uint8_t t62_xedgedist0[TOUCH_TYPE_MAX]     = {58,58,58,58,63,63,63,63,58,58,63,58};
static uint8_t t62_yedgectrl0[TOUCH_TYPE_MAX]     = {170,170,170,170,162,162,162,162,170,170,162,170};
static uint8_t t62_yedgedist0[TOUCH_TYPE_MAX]     = {54,54,54,54,54,54,54,54,54,54,54,54};
static uint8_t t62_jumplimit0[TOUCH_TYPE_MAX]     = {30,30,30,30,30,30,30,30,30,30,30,30};
static uint8_t t62_tchhyst0[TOUCH_TYPE_MAX] 	  = {10,10,10,10,10,10,10,10,10,10,10,10};
static uint8_t t62_nexttchdi0[TOUCH_TYPE_MAX]     = {1,1,1,1,1,1,1,1,1,1,1,1};

#else

/* SPT_USERDATA_T38 INSTANCE 0 */
static uint8_t t38_userdata0[TOUCH_TYPE_MAX] = {'1','1'};
static uint8_t t38_userdata1[TOUCH_TYPE_MAX] = {'6','6'};
static uint8_t t38_userdata2[TOUCH_TYPE_MAX] = {'0','0'};
static uint8_t t38_userdata3[TOUCH_TYPE_MAX] = {'3','2'};
static uint8_t t38_userdata4[TOUCH_TYPE_MAX] = {'1','0'};
static uint8_t t38_userdata5[TOUCH_TYPE_MAX] = {'1','5'};
static uint8_t t38_userdata6[TOUCH_TYPE_MAX] = {'0','0'};
#if defined(CONFIG_DAUDIOECO_HIGH_RESOLUTION)
static uint8_t t38_userdata7[TOUCH_TYPE_MAX] = {'1','1'};
#elif defined(CONFIG_DAUDIOECO_LOWCOST_LCD_7)
static uint8_t t38_userdata7[TOUCH_TYPE_MAX] = {'2','2'};
#else
static uint8_t t38_userdata7[TOUCH_TYPE_MAX] = {'0','0'};
#endif

/* GEN_POWERCONFIG_T7 INSTANCE 0 */
static uint8_t t7_idleacqint[TOUCH_TYPE_MAX]	= {255,255};
static uint8_t t7_actvacqint[TOUCH_TYPE_MAX] 	= {255,255};
static uint8_t t7_actv2idleto[TOUCH_TYPE_MAX]   = {255,255};
static uint8_t t7_cfg[TOUCH_TYPE_MAX] 			= {0,0};

/* _GEN_ACQUISITIONCONFIG_T8 INSTANCE 0 */
static uint8_t t8_chrgtime[TOUCH_TYPE_MAX]        = {27,27};	/* 6 - 60  * 83 ns */
static uint8_t t8_reserved[TOUCH_TYPE_MAX]        = {0,0};
static uint8_t t8_tchdrift[TOUCH_TYPE_MAX]        = {5,5};
static uint8_t t8_driftst[TOUCH_TYPE_MAX]         = {1,1};
static uint8_t t8_tchautocal[TOUCH_TYPE_MAX]      = {25,25};
static uint8_t t8_sync[TOUCH_TYPE_MAX]            = {0,0};
static uint8_t t8_atchcalst[TOUCH_TYPE_MAX]       = {5,5};
static uint8_t t8_atchcalsthr[TOUCH_TYPE_MAX]     = {0,0};
static uint8_t t8_atchcalfrcthr[TOUCH_TYPE_MAX]   = {50,50};	/* V2.0 added */
static uint8_t t8_atchcalfrcratio[TOUCH_TYPE_MAX] = {25,25};	/* V2.0 added */

/* TOUCH_MULTITOUCHSCREEN_T9 INSTANCE 0 */
static uint8_t t9_ctrl[TOUCH_TYPE_MAX]       = {139,139};
static uint8_t t9_xorigin[TOUCH_TYPE_MAX]    = {0,0};
static uint8_t t9_yorigin[TOUCH_TYPE_MAX]    = {0,0};
static uint8_t t9_xsize[TOUCH_TYPE_MAX]      = {24,24};
static uint8_t t9_ysize[TOUCH_TYPE_MAX]      = {14,14};
static uint8_t t9_akscfg[TOUCH_TYPE_MAX]     = {0,0};
static uint8_t t9_blen[TOUCH_TYPE_MAX]       = {64,48};
static uint8_t t9_tchthr[TOUCH_TYPE_MAX]     = {40,40};
static uint8_t t9_tchdi[TOUCH_TYPE_MAX] 	 = {2,2};
#if defined(CONFIG_DAUDIOECO_HIGH_RESOLUTION)
static uint8_t t9_orient[TOUCH_TYPE_MAX]     = {0,0};
#elif defined(CONFIG_DAUDIOECO_LOWCOST_LCD_7)
static uint8_t t9_orient[TOUCH_TYPE_MAX]     = {0,0};
#else
static uint8_t t9_orient[TOUCH_TYPE_MAX]     = {2,2};
#endif
static uint8_t t9_mrgtimeout[TOUCH_TYPE_MAX] = {10,10};
static uint8_t t9_movhysti[TOUCH_TYPE_MAX]   = {5,5};
static uint8_t t9_movhystn[TOUCH_TYPE_MAX]   = {5,5};
static uint8_t t9_movfilter[TOUCH_TYPE_MAX]  = {0,0};
static uint8_t t9_numtouch[TOUCH_TYPE_MAX]   = {2,2};
static uint8_t t9_mrghyst[TOUCH_TYPE_MAX]    = {5,5};
static uint8_t t9_mrgthr[TOUCH_TYPE_MAX]     = {5,5};
static uint8_t t9_amphyst[TOUCH_TYPE_MAX]    = {20,20};
static uint16_t t9_xrange[TOUCH_TYPE_MAX] 	 = {799,799};
static uint16_t t9_yrange[TOUCH_TYPE_MAX] 	 = {479,479};
static uint8_t t9_xloclip[TOUCH_TYPE_MAX] 	 = {0,0};
static uint8_t t9_xhiclip[TOUCH_TYPE_MAX] 	 = {0,0};
static uint8_t t9_yloclip[TOUCH_TYPE_MAX] 	 = {0,0};
static uint8_t t9_yhiclip[TOUCH_TYPE_MAX]    = {0,0};
static uint8_t t9_xedgectrl[TOUCH_TYPE_MAX]  = {192,192};
static uint8_t t9_xedgedist[TOUCH_TYPE_MAX]  = {0,0};
static uint8_t t9_yedgectrl[TOUCH_TYPE_MAX]  = {192,192};
static uint8_t t9_yedgedist[TOUCH_TYPE_MAX]  = {0,0};
static uint8_t t9_jumplimit[TOUCH_TYPE_MAX]  = {30,30};
static uint8_t t9_tchhyst[TOUCH_TYPE_MAX] 	 = {10,10};	/* V2.0 or MXT224E added */
static uint8_t t9_xpitch[TOUCH_TYPE_MAX] 	 = {65,65};	/* MXT224E added */
static uint8_t t9_ypitch[TOUCH_TYPE_MAX]     = {65,65};	/* MXT224E added */
static uint8_t t9_nexttchdi[TOUCH_TYPE_MAX]  = {1,1};
static uint8_t t9_cfg[TOUCH_TYPE_MAX]        = {0,0};

/* PROCI_TOUCHSUPPRESSION_T42 INSTANCE 0 */
static uint8_t t42_ctrl[TOUCH_TYPE_MAX]          = {3,3};
static uint8_t t42_apprthr[TOUCH_TYPE_MAX]       = {32,32};	/* 0 (TCHTHR/4), 1 to 255 */
static uint8_t t42_maxapprarea[TOUCH_TYPE_MAX]   = {40,40};	/* 0 (40ch), 1 to 255 */
static uint8_t t42_maxtcharea[TOUCH_TYPE_MAX]    = {35,35};	/* 0 (35ch), 1 to 255 */
static uint8_t t42_supstrength[TOUCH_TYPE_MAX]   = {0,0};	/* 0 (128), 1 to 255 */
/* 0 (never expires), 1 to 255 (timeout in cycles) */
static uint8_t t42_supextto[TOUCH_TYPE_MAX] 	 = {0,0};
/* 0 to 9 (maximum number of touches minus 1) */
static uint8_t t42_maxnumtchs[TOUCH_TYPE_MAX]    = {4,4};
static uint8_t t42_shapestrength[TOUCH_TYPE_MAX] = {0,0};	/* 0 (10), 1 to 31 */
static uint8_t t42_supdist[TOUCH_TYPE_MAX]       = {0,0};
static uint8_t t42_disthyst[TOUCH_TYPE_MAX]      = {0,0};

/* SPT_CTECONFIG_T46 INSTANCE 0 */
static uint8_t t46_ctrl[TOUCH_TYPE_MAX] 	     = {0,0};	/*Reserved */
/*0: 16X14Y, 1: 17X13Y, 2: 18X12Y, 3: 19X11Y, 4: 20X10Y, 5: 21X15Y, 6: 22X8Y */
static uint8_t t46_mod[TOUCH_TYPE_MAX] 		     = {0,0};
static uint8_t t46_idlesyncsperx[TOUCH_TYPE_MAX] = {16,16};
static uint8_t t46_actvsyncsperx[TOUCH_TYPE_MAX] = {16,16};
static uint8_t t46_adcspersync[TOUCH_TYPE_MAX]   = {0,0};
static uint8_t t46_pulsesperadc[TOUCH_TYPE_MAX]  = {0,0};	/*0:1  1:2   2:3   3:4 pulses */
static uint8_t t46_xslew[TOUCH_TYPE_MAX]         = {1,1};	/*0:500nsec,1:350nsec,2:250nsec(firm2.1)*/
static uint8_t t46_syncdelay[TOUCH_TYPE_MAX]     = {0,0};
/* add firmware2.1 */
static uint8_t t46_xvoltage[TOUCH_TYPE_MAX]      = {1,1};

/* PROCI_SHIELDLESS_T56 INSTANCE 0 */
static uint8_t t56_ctrl[TOUCH_TYPE_MAX] 		= {3,3};
static uint8_t t56_command[TOUCH_TYPE_MAX] 	    = {0,0};
static uint8_t t56_optint[TOUCH_TYPE_MAX]       = {1,1};
static uint8_t t56_inttime[TOUCH_TYPE_MAX]      = {53,57};
static uint8_t t56_intdelay0[TOUCH_TYPE_MAX] 	= {24,28};
static uint8_t t56_intdelay1[TOUCH_TYPE_MAX] 	= {24,28};
static uint8_t t56_intdelay2[TOUCH_TYPE_MAX] 	= {24,28};
static uint8_t t56_intdelay3[TOUCH_TYPE_MAX] 	= {24,28};
static uint8_t t56_intdelay4[TOUCH_TYPE_MAX] 	= {24,32};
static uint8_t t56_intdelay5[TOUCH_TYPE_MAX] 	= {24,32};
static uint8_t t56_intdelay6[TOUCH_TYPE_MAX] 	= {24,32};
static uint8_t t56_intdelay7[TOUCH_TYPE_MAX] 	= {28,32};
static uint8_t t56_intdelay8[TOUCH_TYPE_MAX] 	= {28,32};
static uint8_t t56_intdelay9[TOUCH_TYPE_MAX] 	= {28,32};
static uint8_t t56_intdelay10[TOUCH_TYPE_MAX] 	= {28,32};
static uint8_t t56_intdelay11[TOUCH_TYPE_MAX] 	= {28,32};
static uint8_t t56_intdelay12[TOUCH_TYPE_MAX] 	= {28,32};
static uint8_t t56_intdelay13[TOUCH_TYPE_MAX] 	= {28,32};
static uint8_t t56_intdelay14[TOUCH_TYPE_MAX] 	= {28,32};
static uint8_t t56_intdelay15[TOUCH_TYPE_MAX] 	= {28,32};
static uint8_t t56_intdelay16[TOUCH_TYPE_MAX] 	= {28,32};
static uint8_t t56_intdelay17[TOUCH_TYPE_MAX] 	= {28,32};
static uint8_t t56_intdelay18[TOUCH_TYPE_MAX] 	= {28,32};
static uint8_t t56_intdelay19[TOUCH_TYPE_MAX] 	= {28,28};
static uint8_t t56_intdelay20[TOUCH_TYPE_MAX] 	= {28,28};
static uint8_t t56_intdelay21[TOUCH_TYPE_MAX] 	= {28,28};
static uint8_t t56_intdelay22[TOUCH_TYPE_MAX] 	= {28,28};
static uint8_t t56_intdelay23[TOUCH_TYPE_MAX] 	= {24,28};
static uint8_t t56_multicutgc[TOUCH_TYPE_MAX] 	= {0,0};	//2014.11.22
static uint8_t t56_reserved1[TOUCH_TYPE_MAX]    = {0,0};	//2014.11.22
static uint8_t t56_ncncl[TOUCH_TYPE_MAX] 		= {1,1};
static uint8_t t56_touchbias[TOUCH_TYPE_MAX]    = {0,0};
static uint8_t t56_basescale[TOUCH_TYPE_MAX]    = {20,20};	//2014.11.22
static uint8_t t56_shiftlimit[TOUCH_TYPE_MAX]   = {4,4};	//2014.11.22
static uint8_t t56_ylonoisemul[TOUCH_TYPE_MAX]  = {0,0};
static uint8_t t56_ylonoisediv[TOUCH_TYPE_MAX]  = {0,0};
static uint8_t t56_yhinoisemul[TOUCH_TYPE_MAX]  = {0,0};
static uint8_t t56_yhinoisediv[TOUCH_TYPE_MAX]  = {0,0};
static uint8_t t56_reserved2[TOUCH_TYPE_MAX]    = {0,0};

/* PROCG_NOISESUPPRESSION_T62 INSTANCE 0 */
static uint8_t t62_ctrl[TOUCH_TYPE_MAX]           = {3,3};
static uint8_t t62_calcfg1[TOUCH_TYPE_MAX]        = {11,11};
static uint8_t t62_calcfg2[TOUCH_TYPE_MAX]        = {0,0};
static uint8_t t62_calcfg3[TOUCH_TYPE_MAX]        = {7,7};
static uint8_t t62_cfg[TOUCH_TYPE_MAX]            = {0,0};
static uint8_t t62_reserved0[TOUCH_TYPE_MAX]      = {0,0};
static uint8_t t62_minthradj[TOUCH_TYPE_MAX]      = {32,32};
static uint8_t t62_basefreq[TOUCH_TYPE_MAX]       = {0,0};
static uint8_t t62_maxselfreq[TOUCH_TYPE_MAX]     = {25,25};
static uint8_t t62_freq0[TOUCH_TYPE_MAX]          = {5,5};
static uint8_t t62_freq1[TOUCH_TYPE_MAX]          = {10,10};
static uint8_t t62_freq2[TOUCH_TYPE_MAX]          = {15,15};
static uint8_t t62_freq3[TOUCH_TYPE_MAX]          = {20,20};
static uint8_t t62_freq4[TOUCH_TYPE_MAX]          = {24,24};
static uint8_t t62_hopcnt[TOUCH_TYPE_MAX] 		  = {5,5};
static uint8_t t62_reserved1[TOUCH_TYPE_MAX]      = {0,0};
static uint8_t t62_hopcntper[TOUCH_TYPE_MAX]      = {20,20};
static uint8_t t62_hopevalto[TOUCH_TYPE_MAX]      = {5,5};
static uint8_t t62_hopst[TOUCH_TYPE_MAX]          = {5,5};
static uint8_t t62_nlgain[TOUCH_TYPE_MAX] 	      = {70,70};
static uint8_t t62_minnlthr[TOUCH_TYPE_MAX] 	  = {20,20};
static uint8_t t62_incnlthr[TOUCH_TYPE_MAX]       = {20,20};
static uint8_t t62_adcperxthr[TOUCH_TYPE_MAX] 	  = {15,15};
static uint8_t t62_nlthrmargin[TOUCH_TYPE_MAX]    = {30,30};
static uint8_t t62_maxadcperx[TOUCH_TYPE_MAX]     = {63,63};
static uint8_t t62_actvadcsvldnod[TOUCH_TYPE_MAX] = {6,6};
static uint8_t t62_idleadcsvldnod[TOUCH_TYPE_MAX] = {6,6};
static uint8_t t62_mingclimit[TOUCH_TYPE_MAX] 	  = {10,10};
static uint8_t t62_maxgclimit[TOUCH_TYPE_MAX]     = {64,64};
static uint8_t t62_reserved2[TOUCH_TYPE_MAX]      = {0,0};
static uint8_t t62_reserved3[TOUCH_TYPE_MAX]      = {0,0};
static uint8_t t62_reserved4[TOUCH_TYPE_MAX]      = {0,0};
static uint8_t t62_reserved5[TOUCH_TYPE_MAX]      = {0,0};
static uint8_t t62_reserved6[TOUCH_TYPE_MAX]      = {0,0};
static uint8_t t62_blen0[TOUCH_TYPE_MAX] 		  = {48,48};
static uint8_t t62_tchthr0[TOUCH_TYPE_MAX] 	      = {40,40};
static uint8_t t62_tchdi0[TOUCH_TYPE_MAX]         = {2,2};
static uint8_t t62_movhysti0[TOUCH_TYPE_MAX]      = {1,1};
static uint8_t t62_movhystn0[TOUCH_TYPE_MAX]      = {1,1};
static uint8_t t62_movfilter0[TOUCH_TYPE_MAX]     = {49,49};
static uint8_t t62_numtouch0[TOUCH_TYPE_MAX]      = {2,2};
static uint8_t t62_mrghyst0[TOUCH_TYPE_MAX]       = {10,10};
static uint8_t t62_mrgthr0[TOUCH_TYPE_MAX]        = {10,10};
static uint8_t t62_xloclip0[TOUCH_TYPE_MAX]       = {7,5};
static uint8_t t62_xhiclip0[TOUCH_TYPE_MAX]       = {7,5};
static uint8_t t62_yloclip0[TOUCH_TYPE_MAX]       = {8,13};
static uint8_t t62_yhiclip0[TOUCH_TYPE_MAX]       = {8,13};
static uint8_t t62_xedgectrl0[TOUCH_TYPE_MAX] 	  = {237,222};
static uint8_t t62_xedgedist0[TOUCH_TYPE_MAX]     = {58,63};
static uint8_t t62_yedgectrl0[TOUCH_TYPE_MAX]     = {170,162};
static uint8_t t62_yedgedist0[TOUCH_TYPE_MAX]     = {54,54};
static uint8_t t62_jumplimit0[TOUCH_TYPE_MAX]     = {30,30};
static uint8_t t62_tchhyst0[TOUCH_TYPE_MAX] 	  = {10,10};
static uint8_t t62_nexttchdi0[TOUCH_TYPE_MAX]     = {1,1};

#endif

#endif

