#ifndef ATMEL_MXT1189T_CFG_H
#define ATMEL_MXT1189T_CFG_H

#if defined(INCLUDE_LCD_TOUCHKEY) //touchkey

//[SPT_USERDATA_T38 INSTANCE 0]
unsigned char cfg_1189T_T38[64] = {'1', '8', '0', '4', '1', '0', '1', '0', 0, 0, };

//[GEN_POWERCONFIG_T7 INSTANCE 0]
unsigned char cfg_1189T_T7[5] = {32, 10, 40, 192, 0};

//[GEN_ACQUISITIONCONFIG_T8 INSTANCE 0]
unsigned char cfg_1189T_T8[15] = {60, 0, 5, 1, 0, 0, 5, 0, 50, 25,
								  1, 1, 1, 1, 0
								 };

//[TOUCH_KEYARRAY_T15 INSTANCE 0]
unsigned char cfg_1189T_T15[11] = {3, 19, 49, 9, 1, 0, 5, 50, 2, 10,
								   0
								  };

//[SPT_SELFTEST_T25 INSTANCE 0]
unsigned char cfg_1189T_T25[21] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				   0, 0, 0, 0, 250, 0, 0, 0, 0, 0, 0 };

//[PROCI_TOUCHSUPPRESSION_T42 INSTANCE 0]
unsigned char cfg_1189T_T42[14] = {3, 60, 100, 35, 225, 255, 0, 5, 3, 5,
								   0, 3, 0, 0
								  };

//[SPT_CTECONFIG_T46 INSTANCE 0]
unsigned char cfg_1189T_T46[21] = {128, 0, 16, 16, 0, 0, 1, 0, 0, 128,
								   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
								   0
								  };

//[PROCI_SHIELDLESS_T56 INSTANCE 0]
unsigned char cfg_1189T_T56[36] = {1, 0, 1, 45, 3, 3, 3, 3, 3, 3,
								   3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
								   3, 3, 3, 0, 0, 0, 0, 0, 0, 0,
								   0, 0, 0, 0, 0, 0
								  };

//[SPT_TIMER_T61 INSTANCE 0]
unsigned char cfg_1189T_T61[15] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
								   0, 0, 0, 0, 0
								  }; // 25000 -> A8 61


//[PROCI_LENSBENDING_T65 INSTANCE 0]
unsigned char cfg_1189T_T65[23] = {1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
								   0, 0, 0, 0, 0, 0, 0, 17, 0, 0,
								   0, 0, 0
								  };

//[SPT_DYNAMICCONFIGURATIONCONTROLLER_T70 INSTANCE 0]
unsigned char cfg_1189T_T70[70] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
								   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
								   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
								   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
								   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
								   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
								   0, 0, 0, 0, 0, 0, 0, 0, 0, 0
								  }; // 13 -> 0D 00

unsigned char cfg_1189T_T71[200] = {1, 190, 254, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
									0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
									0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
									0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
									0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
									0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
									0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
									0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
									0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
									0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
								   };



//[PROCG_NOISESUPPRESSION_T72 INSTANCE 0]
unsigned char cfg_1189T_T72[89] = {253, 0, 0, 1, 0, 1, 5, 1, 1, 3,
								   30, 20, 10, 5, 32, 30, 29, 5, 0, 157,
								   0, 22, 25, 28, 25, 22, 32, 32, 32, 32,
								   32, 32, 32, 32, 32, 32, 0, 0, 20, 0,
								   9, 22, 25, 28, 25, 22, 48, 48, 48, 48,
								   48, 48, 48, 48, 48, 48, 0, 10, 18, 83,
								   9, 22, 25, 28, 25, 22, 63, 63, 63, 63,
								   63, 63, 63, 63, 63, 63, 0, 5, 0, 250,
								   0, 5, 7, 2, 5, 0, 0, 0, 0
								  };

//[PROCI_GLOVEDETECTION_T78 INSTANCE 0]
unsigned char cfg_1189T_T78[12] = {129, 3, 1, 1, 10, 10, 32, 0, 46, 25,
								   5, 5
								  };

//[PROCI_RETRANSMISSIONCOMPENSATION_T80 INSTANCE 0]
unsigned char cfg_1189T_T80[14] = {1, 1, 100, 25, 15, 10, 0, 0, 0, 0,
								   0, 0, 0, 0
								  };

//[TOUCH_MULTITOUCHSCREEN_T100 INSTANCE 0]

unsigned char cfg_1189T_T100[64] = {139, 58, 7, 6, 0, 0, 2, 136, 0, 19, //128
									49, 0, 0, 0xCF, 0x02, 24, 50, 45, 75, 0,  // 719 -> CF 02
									49, 49, 0, 0, 0x7F, 0x7, 18, 24, 8, 5,  // 1919 -> 7F 07
									60, 12, 60, 0, 0, 25, 0, 15, 0, 2,
									2, 2, 0, 15, 66, 220, 45, 15, 0, 2,
									0, 0, 0, 12, 0, 0, 0, 0, 0, 0,
									1, 0, 0, 0
								   };


// X and Y are inverted
unsigned char cfg_1189T_T100_1[64] = {131, 58, 0, 16, 0, 0, 10, 136, 0, 19,
									  49, 0, 0, 0xCF, 0x02, 24, 50, 0, 0, 0,  // 719 -> CF 02
									  49, 49, 0, 0, 0x7F, 0x7, 18, 24, 12, 7,  // 1919 -> 7F 07
									  50, 12, 35, 0, 0, 25, 0, 15, 0, 2,
									  3, 1, 0, 15, 67, 220, 45, 15, 0, 2,
									  0, 30, 30, 10, 0, 0, 0, 0, 0, 0,
									  0, 0, 0, 0
									 };

//[SPT_AUXTOUCHCONFIG_T104 INSTANCE 0]
unsigned char cfg_1189T_T104[11] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
									0
								   };


#else
//[SPT_USERDATA_T38 INSTANCE 0]
unsigned char cfg_1189T_T38[64] = {'1', '7', '0', '4', '2', '1', '1', '0', 0, 0, };

//[GEN_POWERCONFIG_T7 INSTANCE 0]
unsigned char cfg_1189T_T7[5] = {255, 255, 50, 67, 0};

//[GEN_ACQUISITIONCONFIG_T8 INSTANCE 0]
unsigned char cfg_1189T_T8[15] = {30, 0, 15, 5, 0, 0, 5, 0, 50, 20,
								  1, 1, 1, 1, 0
								 };

//[PROCI_TOUCHSUPPRESSION_T42 INSTANCE 0]
unsigned char cfg_1189T_T42[13] = {3, 0, 50, 40, 254, 2, 5, 10, 0, 0,
								   0, 3, 0
								  };

//[SPT_CTECONFIG_T46 INSTANCE 0]
unsigned char cfg_1189T_T46[12] = {4, 0, 48, 48, 0, 0, 0, 0, 0, 128,
								   0, 0
								  };

//[PROCI_SHIELDLESS_T56 INSTANCE 0]
unsigned char cfg_1189T_T56[36] = {1, 0, 1, 28, 0, 0, 0, 0, 0, 0,
								   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
								   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
								   0, 0, 0, 0, 0, 0
								  };

//[PROCI_LENSBENDING_T65 INSTANCE 0]
unsigned char cfg_1189T_T65[23] = {129, 20, 0, 0, 0, 0, 0, 0, 0, 0,
								   0, 0, 0, 0, 0, 2, 0, 0, 0, 0,
								   0, 0, 0
								  };

//[PROCG_NOISESUPPRESSION_T72 INSTANCE 0]
unsigned char cfg_1189T_T72[89] = {253, 0, 0, 1, 0, 5, 5, 1, 1, 6,
								   20, 20, 10, 5, 48, 10, 0, 6, 200, 5,
								   0, 0, 0, 0, 0, 0, 32, 32, 32, 32,
								   32, 32, 32, 32, 32, 32, 0, 0, 10, 0,
								   1, 0, 0, 0, 0, 0, 48, 48, 48, 48,
								   48, 48, 48, 48, 48, 48, 0, 8, 18, 34,
								   1, 0, 0, 0, 0, 0, 63, 63, 63, 63,
								   63, 63, 63, 63, 63, 63, 0, 16, 0, 34,
								   0, 0, 0, 0, 0, 0, 0, 0, 0
								  };

//[PROCI_GLOVEDETECTION_T78 INSTANCE 0]
unsigned char cfg_1189T_T78[12] = {129, 2, 2, 0, 10, 0, 24, 0, 0, 0,
								   0, 0
								  };

//[PROCI_RETRANSMISSIONCOMPENSATION_T80 INSTANCE 0]
unsigned char cfg_1189T_T80[14] = {11, 1, 50, 25, 25, 0, 0, 0, 0, 0,
								   0, 0, 0, 0
								  };

//[TOUCH_MULTITOUCHSCREEN_T100 INSTANCE 0]
unsigned char cfg_1189T_T100[62] = {143, 122, 0, 0, 0, 0, 5, 0, 0, 20,
									55, 9, 9, 0xCF, 0x02, 33, 35, 0, 0, 0,  // 719 -> CF 02
									52, 53, 9, 9, 0x7F, 0x7, 22, 23, 6, 7,  // 1919 -> 7F 07
									45, 11, 12, 0, 0, 15, 20, 10, 0, 2,
									1, 2, 0, 20, 2, 220, 30, 3, 0, 1,
									0, 0, 0, 3, 0, 0, 0, 0, 0, 0,
									0, 0
								   };

// X and Y are inverted
unsigned char cfg_1189T_T100_1[62] = {143, 186, 0, 0, 0, 0, 5, 0, 0, 19,
									  55, 9, 9, 0xCF, 0x02, 33, 35, 0, 0, 0,  // 719 -> CF 02
									  49, 53, 9, 9, 0x7F, 0x7, 22, 23, 6, 7,  // 1919 -> 7F 07
									  45, 11, 12, 0, 0, 15, 20, 10, 0, 2,
									  1, 2, 0, 20, 2, 220, 30, 3, 0, 1,
									  0, 0, 0, 3, 0, 0, 0, 0, 0, 0,
									  0, 0
									 };

//[SPT_AUXTOUCHCONFIG_T104 INSTANCE 0]
unsigned char cfg_1189T_T104[11] = {0, 20, 50, 15, 25, 10, 19, 50, 15, 25,
									10
								   };

#endif
#endif  /* ATMEL_MXT1189T_CFG_H */
