/* linux/arch/arm/mach-tcc897x/include/mach/tcc_pmap.h
 *
 * Copyright (C) 2010 Telechips, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
*/

#ifndef _TCC_PMAP_H

#ifdef CONFIG_TCC_VPU_DRV_MODULE
#include <mach/tcc_video_private.h>
#endif

#ifndef VPU_BUFFER_MANAGE_COUNT // to access bootloader!!
#define VPU_BUFFER_MANAGE_COUNT 3
#endif

#define _TCC_PMAP_H

#undef SZ_1MB
#define SZ_1MB 						(1024*1024)
#define ARRAY_16MBYTE(x)          	((((x) + ((16*SZ_1MB)-1))>> 24) << 24)
#define ARRAY_MBYTE(x)            	((((x) + (SZ_1MB-1))>> 20) << 20)
#define ARRAY_256KBYTE(x)         	((((x) + ((SZ_1MB/4)-1))>> 18) << 18)

//for HEVC/VPU framebuffer_size Calculation
#define HEVC_CAL_PROCBUFFER(use,w,h,f) (((0x700000 /*6.5MB*/) + (ARRAY_MBYTE(((w*h*15)/10)*(f+7+3))) + (ARRAY_MBYTE(((w*h*161)/100)*(f+7+3))))*use)
#define VPU_CAL_PROCBUFFER(use,w,h,f)  (((0x300000) + ARRAY_MBYTE(((w*h*182)/100)*(f+7)))*use)

#ifdef CONFIG_ARM_TRUSTZONE
#define SUPPORT_SECURED_INBUFER
#else
//#define SUPPORT_SECURED_INBUFER     //for WiFi Display
#endif




//##################################################################################################################################################
//##################################################################################################################################################
/********************* Need to be optimized by customer ******************/

// Caution!! Contact engineer who is in charge of scurity in Telechips
// if you modified anything in this file with enabled CONFIG_ARM_TRUSTZONE Feature.

//#define SUPPORT_ROTATION  // Enable in case supporting the display-device


//***************************** DISPLAY ******************************/
#if defined(CONFIG_LCD_HDMI1920X720)
#define PRIMARY_FRAMEBUFFER_WIDTH			1920
#define PRIMARY_FRAMEBUFFER_HEIGHT			720
#if defined(CONFIG_HWCOMPOSER_OVER_1_1_FOR_MID)
#define SECONDARY_FRAMEBUFFER_WIDTH			1920
#define SECONDARY_FRAMEBUFFER_HEIGHT		720
#else
#define SECONDARY_FRAMEBUFFER_WIDTH			0
#define SECONDARY_FRAMEBUFFER_HEIGHT		0
#endif
#else
#define PRIMARY_FRAMEBUFFER_WIDTH			1280
#define PRIMARY_FRAMEBUFFER_HEIGHT			720
#define SECONDARY_FRAMEBUFFER_WIDTH			0
#define SECONDARY_FRAMEBUFFER_HEIGHT		0
#endif

//Choose more bigger resolution between Primary and Secondary display device.
//and Should sync with hardware/telechips/common/hwcomposer/tc_hwc_overlay.h
#if defined(CONFIG_LCD_HDMI1920X720)
#define PRIMARY_TARGET_WIDTH			1920
#define PRIMARY_TARGET_HEIGHT			720
#else
#define PRIMARY_TARGET_WIDTH			1280
#define PRIMARY_TARGET_HEIGHT			720
#endif
#define SECONDARY_TARGET_WIDTH			0
#define SECONDARY_TARGET_HEIGHT			0

#if ((PRIMARY_TARGET_WIDTH*PRIMARY_TARGET_HEIGHT) > (SECONDARY_TARGET_WIDTH*SECONDARY_TARGET_HEIGHT))
#define SUPPORT_DISPLAY_MAX_WIDTH	PRIMARY_TARGET_WIDTH
#define SUPPORT_DISPLAY_MAX_HEIGHT	PRIMARY_TARGET_HEIGHT
#else
#define SUPPORT_DISPLAY_MAX_WIDTH	SECONDARY_TARGET_WIDTH
#define SUPPORT_DISPLAY_MAX_HEIGHT	SECONDARY_TARGET_HEIGHT
#endif


//****************************** Camera ******************************/
#define CAMERA_INPUT_WIDTH			720
#define CAMERA_INPUT_HEIGHT			480
#define SUPPORT_CAMERA_CAPTURE		0


//***************************** VPU/HEVC ******************************/
//Should sync with hardware/telechips/omx/omx_videodec_interface/include/vdec.h
#if defined(CONFIG_SUPPORT_TCC_HEVC_4K)
#define SUPPORT_VIDEO_MAX_WIDTH		4096
#define SUPPORT_VIDEO_MAX_HEIGHT	2160
#else
#if defined(CONFIG_LCD_HDMI1920X720)
#define SUPPORT_VIDEO_MAX_WIDTH		1920	// 1280 / 1920 / 4096
#define SUPPORT_VIDEO_MAX_HEIGHT	1088	// 720  / 1088 / 2160
#else
#define SUPPORT_VIDEO_MAX_WIDTH		1920	// 860 / 1280 / 1920 / 4096
#define SUPPORT_VIDEO_MAX_HEIGHT	1088	// 560 / 720  / 1088 / 2160
#endif
#endif

// DECODER on VPU
#define NUM_OF_MAIN_INSTANCE 		(2)
#define NUM_OF_SUB_INSTANCE  		(0)

#if defined(CONFIG_LCD_HDMI1920X720)
#define INST_1ST_USE				(1)
#define INST_1ST_IS_HEVC_TYPE		(0)		// please check "CONFIG_SUPPORT_TCC_HEVC" Feature in order to enable HEVC
#define INST_1ST_VIDEO_WIDTH		(1920)	// HEVC: 32x alignment, Other: 16x alignment
#define INST_1ST_VIDEO_HEIGHT		(1088)	// HEVC: 32x alignment, Other: 16x alignment
#define INST_1ST_MAX_FRAMEBUFFERS	(24)	// HEVC: 1~21, Others: 1~24

#if defined(CONFIG_VIDEO_DECODING_MEMORY)	//For video device using CPU decoder.
#define INST_2ND_USE				(1)
#else
#define INST_2ND_USE				(0)
#endif
#define INST_2ND_IS_HEVC_TYPE		(1)		// please check "CONFIG_SUPPORT_TCC_HEVC" Feature in order to enable HEVC
#define INST_2ND_VIDEO_WIDTH		(1280)	// HEVC: 32x alignment, Other: 16x alignment
#define INST_2ND_VIDEO_HEIGHT		(736)	// HEVC: 32x alignment, Other: 16x alignment
#define INST_2ND_MAX_FRAMEBUFFERS	(21)	// HEVC: 1~21, Others: 1~24
#else
#define INST_1ST_USE				(1)
#define INST_1ST_IS_HEVC_TYPE		(0)		// please check "CONFIG_SUPPORT_TCC_HEVC" Feature in order to enable HEVC
#define INST_1ST_VIDEO_WIDTH		(1920)	// HEVC: 32x alignment, Other: 16x alignment
#define INST_1ST_VIDEO_HEIGHT		(1088)	// HEVC: 32x alignment, Other: 16x alignment
#define INST_1ST_MAX_FRAMEBUFFERS	(24)	// HEVC: 1~21, Others: 1~24

#define INST_2ND_USE				(0)
#define INST_2ND_IS_HEVC_TYPE		(0)		// please check "CONFIG_SUPPORT_TCC_HEVC" Feature in order to enable HEVC
#define INST_2ND_VIDEO_WIDTH		(1280)	// HEVC: 32x alignment, Other: 16x alignment
#define INST_2ND_VIDEO_HEIGHT		(736)	// HEVC: 32x alignment, Other: 16x alignment
#define INST_2ND_MAX_FRAMEBUFFERS	(24)	// HEVC: 1~21, Others: 1~24
#endif

#if defined(CONFIG_VDEC_CNT_3) || defined(CONFIG_VDEC_CNT_4)
#define INST_3RD_USE				(0)
#define INST_3RD_IS_HEVC_TYPE		(0)		// please check "CONFIG_SUPPORT_TCC_HEVC" Feature in order to enable HEVC
#define INST_3RD_VIDEO_WIDTH		(0)		// HEVC: 32x alignment, Other: 16x alignment
#define INST_3RD_VIDEO_HEIGHT		(0)		// HEVC: 32x alignment, Other: 16x alignment
#define INST_3RD_MAX_FRAMEBUFFERS	(24)	// HEVC: 1~21, Others: 1~24

#define INST_4TH_USE				(0)
#define INST_4TH_IS_HEVC_TYPE		(0)		// please check "CONFIG_SUPPORT_TCC_HEVC" Feature in order to enable HEVC
#define INST_4TH_VIDEO_WIDTH		(0)		// HEVC: 32x alignment, Other: 16x alignment
#define INST_4TH_VIDEO_HEIGHT		(0)		// HEVC: 32x alignment, Other: 16x alignment
#define INST_4TH_MAX_FRAMEBUFFERS	(24)	// HEVC: 1~21, Others: 1~24
#endif

//##################################################################################################################################################
//##################################################################################################################################################

#if (defined(CONFIG_SUPPORT_TCC_NSK) || defined(CONFIG_TCC_DSP))
#define NSK_REV_MEM_SIZE        	(25*SZ_1MB)
#else
#define NSK_REV_MEM_SIZE       		0
#endif

#ifdef SUPPORT_SECURED_INBUFER
#define SECURED_INBUFF_SIZE			(6*SZ_1MB) // 1.5MB/buffer for compressed input stream!
#define VIDEO_SBACKUP_SIZE			(2*SZ_1MB)
#define VIDEO_THUMB_SIZE			(6*SZ_1MB)
#else
#define SECURED_INBUFF_SIZE			(0)
#define VIDEO_SBACKUP_SIZE			(0)
#define VIDEO_THUMB_SIZE			(0)
#endif

#define OVERLAY_SIZE				ARRAY_MBYTE(SUPPORT_DISPLAY_MAX_WIDTH * SUPPORT_DISPLAY_MAX_HEIGHT * 2 * VPU_BUFFER_MANAGE_COUNT)
// to detect CarPlay Black Screen, YUV420 interleaved format, single buffer
#define OVERLAY1_SIZE				ARRAY_MBYTE((SUPPORT_DISPLAY_MAX_WIDTH * SUPPORT_DISPLAY_MAX_HEIGHT * 3) / 2)

// to rotate video-frame, YUV422 sequential format, double buffer
#ifdef SUPPORT_ROTATION
#define OVERLAY_LOT_SIZE			ARRAY_MBYTE(SUPPORT_DISPLAY_MAX_WIDTH * SUPPORT_DISPLAY_MAX_HEIGHT * 2 * 2)
#else
#define OVERLAY_LOT_SIZE			(0)
#endif

#if (INST_1ST_IS_HEVC_TYPE > 0)
#define INST_1ST_PROC_SIZE 			HEVC_CAL_PROCBUFFER(INST_1ST_USE, INST_1ST_VIDEO_WIDTH, INST_1ST_VIDEO_HEIGHT, INST_1ST_MAX_FRAMEBUFFERS)
#else
#define INST_1ST_PROC_SIZE 			VPU_CAL_PROCBUFFER(INST_1ST_USE, INST_1ST_VIDEO_WIDTH, INST_1ST_VIDEO_HEIGHT, INST_1ST_MAX_FRAMEBUFFERS)
#endif

#if (INST_2ND_IS_HEVC_TYPE > 0)
#define INST_2ND_PROC_SIZE 			HEVC_CAL_PROCBUFFER(INST_2ND_USE, INST_2ND_VIDEO_WIDTH, INST_2ND_VIDEO_HEIGHT, INST_2ND_MAX_FRAMEBUFFERS)
#else
#define INST_2ND_PROC_SIZE 			VPU_CAL_PROCBUFFER(INST_2ND_USE, INST_2ND_VIDEO_WIDTH, INST_2ND_VIDEO_HEIGHT, INST_2ND_MAX_FRAMEBUFFERS)
#endif

#if ((INST_1ST_IS_HEVC_TYPE > 0 && INST_1ST_USE) | (INST_2ND_IS_HEVC_TYPE > 0 && INST_2ND_USE))
#define SUPPORT_HEVC_OF_MAIN_INSTANCE 	(1)
#else
#define SUPPORT_HEVC_OF_MAIN_INSTANCE 	(0)
#endif

#if ((INST_1ST_IS_HEVC_TYPE == 0 && INST_1ST_USE) | (INST_2ND_IS_HEVC_TYPE == 0 && INST_2ND_USE))
#define SUPPORT_VPU_OF_MAIN_INSTANCE 	(1)
#else
#define SUPPORT_VPU_OF_MAIN_INSTANCE 	(0)
#endif


// to support 3rd and 4th decoder with 1st and 2nd at the same time.
#if defined(CONFIG_VDEC_CNT_3) || defined(CONFIG_VDEC_CNT_4)
	#if (INST_3RD_IS_HEVC_TYPE > 0)
	#define INST_3RD_PROC_SIZE 		HEVC_CAL_PROCBUFFER(INST_3RD_USE, INST_3RD_VIDEO_WIDTH, INST_3RD_VIDEO_HEIGHT, INST_3RD_MAX_FRAMEBUFFERS)
	#else
	#define INST_3RD_PROC_SIZE 		VPU_CAL_PROCBUFFER(INST_3RD_USE, INST_3RD_VIDEO_WIDTH, INST_3RD_VIDEO_HEIGHT, INST_3RD_MAX_FRAMEBUFFERS)
	#endif

	#if (INST_4TH_IS_HEVC_TYPE > 0)
	#define INST_4TH_PROC_SIZE 		HEVC_CAL_PROCBUFFER(INST_4TH_USE, INST_4TH_VIDEO_WIDTH, INST_4TH_VIDEO_HEIGHT, INST_4TH_MAX_FRAMEBUFFERS)
	#else
	#define INST_4TH_PROC_SIZE 		VPU_CAL_PROCBUFFER(INST_4TH_USE, INST_4TH_VIDEO_WIDTH, INST_4TH_VIDEO_HEIGHT, INST_4TH_MAX_FRAMEBUFFERS)
	#endif

	#if ((INST_3RD_IS_HEVC_TYPE > 0 && INST_3RD_USE) | (INST_4TH_IS_HEVC_TYPE > 0 && INST_4TH_USE))
	#define SUPPORT_HEVC_OF_SUB_INSTANCE 	(1)
	#else
	#define SUPPORT_HEVC_OF_SUB_INSTANCE 	(0)
	#endif

	#if ((INST_3RD_IS_HEVC_TYPE == 0 && INST_3RD_USE) | (INST_4TH_IS_HEVC_TYPE == 0 && INST_4TH_USE))
	#define SUPPORT_VPU_OF_SUB_INSTANCE 	(1)
	#else
	#define SUPPORT_VPU_OF_SUB_INSTANCE 	(0)
	#endif

#else // defined(CONFIG_VDEC_CNT_3) || defined(CONFIG_VDEC_CNT_4)
	#define INST_3RD_PROC_SIZE 				(0)
	#define INST_4TH_PROC_SIZE 				(0)
	#define SUPPORT_HEVC_OF_SUB_INSTANCE 	(0)
	#define SUPPORT_VPU_OF_SUB_INSTANCE 	(0)
#endif

// default use!!
#define VPU_WORKBUFFER_SIZE				(2*SZ_1MB)

#if ((SUPPORT_HEVC_OF_MAIN_INSTANCE > 0) | (SUPPORT_HEVC_OF_SUB_INSTANCE > 0))
#define HEVC_WORKBUFFER_SIZE			(15*SZ_1MB)
#else
#define HEVC_WORKBUFFER_SIZE			(0)
#endif

#if (NUM_OF_MAIN_INSTANCE > 1)
#define VIDEO_TEMP_SIZE					(VPU_WORKBUFFER_SIZE + HEVC_WORKBUFFER_SIZE + INST_1ST_PROC_SIZE + INST_2ND_PROC_SIZE + (CONFIG_VENC_BUFFER_SIZE*SZ_1MB))
#else
	#if (INST_1ST_PROC_SIZE > INST_2ND_PROC_SIZE)
	#define MAIN_INST_PROC_SIZE 		(INST_1ST_PROC_SIZE)
	#else
	#define MAIN_INST_PROC_SIZE 		(INST_2ND_PROC_SIZE)
	#endif
	#if (SUPPORT_HEVC_OF_MAIN_INSTANCE > 0)
	#define VIDEO_TEMP_SIZE				(VPU_WORKBUFFER_SIZE /* default use!! */ + HEVC_WORKBUFFER_SIZE + MAIN_INST_PROC_SIZE + (CONFIG_VENC_BUFFER_SIZE*SZ_1MB))
	#else
	#define VIDEO_TEMP_SIZE				(VPU_WORKBUFFER_SIZE + MAIN_INST_PROC_SIZE + (CONFIG_VENC_BUFFER_SIZE*SZ_1MB))
	#endif
#endif

#if defined(CONFIG_VDEC_CNT_3) || defined(CONFIG_VDEC_CNT_4)
#define VIDEO_EXT_SIZE					(INST_3RD_PROC_SIZE + INST_4TH_PROC_SIZE)
#else
#define VIDEO_EXT_SIZE					(0)
#endif

#define EXTRA_SIZE						(SECURED_INBUFF_SIZE + VIDEO_SBACKUP_SIZE + OVERLAY_SIZE + OVERLAY1_SIZE + OVERLAY_LOT_SIZE)
#ifdef SUPPORT_SECURED_INBUFER
#define VIDEO_WORK						(VPU_WORKBUFFER_SIZE + HEVC_WORKBUFFER_SIZE)
#define SECURED_SIZE					(ARRAY_16MBYTE(EXTRA_SIZE + VIDEO_EXT_SIZE + VIDEO_TEMP_SIZE - VIDEO_WORK))
#define VIDEO_SIZE						((SECURED_SIZE) - (EXTRA_SIZE + VIDEO_EXT_SIZE) + VIDEO_WORK)
#else //SUPPORT_SECURED_INBUFER
#define SECURED_SIZE					(EXTRA_SIZE + VIDEO_EXT_SIZE + VIDEO_TEMP_SIZE)
	#define VIDEO_SIZE					((SECURED_SIZE) - (EXTRA_SIZE + VIDEO_EXT_SIZE))
#endif

#define UMP_BUFFER_MANAGE_COUNT			12  // 9 by default. 12 for flash/netflix using ACodec
#define UMP_RESERVED_SIZE				(ARRAY_MBYTE((ARRAY_256KBYTE((SUPPORT_VIDEO_MAX_WIDTH * SUPPORT_VIDEO_MAX_HEIGHT * 3) / 2)) * UMP_BUFFER_MANAGE_COUNT) + (1*SZ_1MB)) // 1MB for specific streaming case!!
/*
 * FB_WMIXER_SIZE
 * 1920 * 720 * 2(YUV422) * 2(Buffers) for ExtFrame(LastFrame)
 */
#define FB_WMIXER_SIZE					ARRAY_MBYTE(SUPPORT_DISPLAY_MAX_WIDTH * SUPPORT_DISPLAY_MAX_HEIGHT * 2 * 2)
#define RAM_CONSOLE_SIZE				(0)

#if defined(CONFIG_VIDEO_V4L2_VIDEO_0)
#define V4L2_VIDEO0_SIZE				(24 * SZ_1MB)
#else
#define V4L2_VIDEO0_SIZE				(0 * SZ_1MB)
#endif
#if defined(CONFIG_VIDEO_V4L2_VIDEO_1)
#define V4L2_VIDEO1_SIZE				(24 * SZ_1MB)
#else
#define V4L2_VIDEO1_SIZE				(0 * SZ_1MB)
#endif
#if defined(CONFIG_VIDEO_V4L2_VIDEO_2)
#define V4L2_VIDEO2_SIZE				(24 * SZ_1MB)
#else
#define V4L2_VIDEO2_SIZE				(0 * SZ_1MB)
#endif
#if defined(CONFIG_VIDEO_V4L2_VIDEO_3)
#define V4L2_VIDEO3_SIZE				(24 * SZ_1MB)
#else
#define V4L2_VIDEO3_SIZE				(0 * SZ_1MB)
#endif
#if defined(CONFIG_VIDEO_V4L2_VIDEO_4)
#define V4L2_VIDEO4_SIZE				(24 * SZ_1MB)
#else
#define V4L2_VIDEO4_SIZE				(0 * SZ_1MB)
#endif

#define V4L2_VIDEO_TOTAL_SIZE			(V4L2_VIDEO0_SIZE + V4L2_VIDEO1_SIZE + V4L2_VIDEO2_SIZE + V4L2_VIDEO3_SIZE + V4L2_VIDEO4_SIZE)

// Choose bigger memory to VIQE_SIZE (JPEG or VIDEO VIQE)
//#########################################################################
/******** only reference for JPEG (Not allocated mem) in VIQE mem ********/
#define JPEG_HEADER_SIZE				(1*SZ_1MB)
#define JPEG_STREAM_SIZE				(10*SZ_1MB)
#define JPEG_RAW_SIZE					(10*SZ_1MB)
//#########################################################################

//#########################################################################
// VIDEO VIQE mem
// 1920 * (1080/2) * 4 * 4 * 1.5
//#########################################################################
#define VIQE_SIZE                 	(JPEG_HEADER_SIZE+JPEG_STREAM_SIZE+JPEG_RAW_SIZE)
//#define VIQE_SIZE						ARRAY_MBYTE((SUPPORT_VIDEO_MAX_WIDTH * (SUPPORT_VIDEO_MAX_HEIGHT / 2 ) * 4 * 4 * 3) / 2)

#if defined(CONFIG_USB_VIDEO_CLASS)   // for USB Camera Image
#define EXT_CAMERA_SIZE     			(21*SZ_1MB)
#else
#define EXT_CAMERA_SIZE     			(0)
#endif

#define FB_VIDEO_SIZE				(ARRAY_MBYTE(PRIMARY_FRAMEBUFFER_WIDTH * PRIMARY_FRAMEBUFFER_HEIGHT * 4 * 3) + (ARRAY_MBYTE(SECONDARY_FRAMEBUFFER_WIDTH * SECONDARY_FRAMEBUFFER_HEIGHT * 4 * 3)))
#define FB_SCALE_SIZE				(0)
#define FB_SCALE0_SIZE				(0)
#define FB_SCALE1_SIZE				(0)
#define FB_G2D0_SIZE				(0)
#define FB_G2D1_SIZE				(0)
#define VIDEO_DUAL_SIZE				(0)
#define NAND_MEM_SIZE				(0)
#define JPG_ENC_DXB_SIZE			(0)
#define JPG_RAW_DXB_SIZE			(3*SZ_1MB)
#if defined(CONFIG_TCC_DISPLAY_MODE_USE)|| defined(CONFIG_TCC_DISPLAY_LCD_CVBS)
#define OUTPUT_ATTACH_SIZE			(ARRAY_MBYTE(SECONDARY_FRAMEBUFFER_WIDTH * SECONDARY_FRAMEBUFFER_HEIGHT * 4 * 3))
#else
#define OUTPUT_ATTACH_SIZE			(0)
#endif

#if defined(CONFIG_VIDEO_TCC_VOUT)
#define V4L2_VOUT_SIZE				ARRAY_MBYTE(SUPPORT_DISPLAY_MAX_WIDTH * SUPPORT_DISPLAY_MAX_HEIGHT * 2 * 3)
#define V4L2_SUBTITLE_SIZE			ARRAY_MBYTE(SUPPORT_VIDEO_MAX_WIDTH * SUPPORT_VIDEO_MAX_HEIGHT *4 * 2)
#else
#define V4L2_VOUT_SIZE				(0)
#define V4L2_SUBTITLE_SIZE			(0)
#endif

/*
 * REAR_CAMERA_VIQE_SIZE Calculation.
 * If camera input source has 720x480.
 *
 * w * (h/2) * 4 * 4 * 1.5
 * (720 * (480/2) * 4 * 4 * 1.5) => 4MB
 */
#if defined(CONFIG_VIDEO_TCCXXX_V4L_DEVICE)
#define REAR_CAMERA_VIQE_SIZE		ARRAY_MBYTE((CAMERA_INPUT_WIDTH * (CAMERA_INPUT_HEIGHT / 2 ) * 4 * 4 * 3) / 2)
#define REAR_CAMERA_SIZE			ARRAY_MBYTE(PRIMARY_TARGET_WIDTH * PRIMARY_TARGET_HEIGHT * 4 * (4 + SUPPORT_CAMERA_CAPTURE))
//#define PARKING_GUIDE_SIZE			ARRAY_MBYTE(PRIMARY_TARGET_WIDTH * PRIMARY_TARGET_HEIGHT * 4)
#define PARKING_GUIDE_SIZE			(0)
//#define EARLYCAM_LOG_SIZE		    (3 * SZ_1MB)
#define EARLYCAM_LOG_SIZE		    (1 * SZ_1MB)
#else
#define REAR_CAMERA_VIQE_SIZE		(0)
#define REAR_CAMERA_SIZE			(0)
#define PARKING_GUIDE_SIZE			(0)
#endif

#if defined(CONFIG_TCC_DISPLAY_HDMI_LVDS)
#define FB_DUPLICATE_SIZE       	(6 * SZ_1MB)
#else

#define FB_DUPLICATE_SIZE 			(0)
#endif

#define SECURE_HASH_SIZE			(16 * SZ_1MB)

#if defined(CONFIG_FB1_SUPPORT)
#define FB1_VIDEO_SIZE         		(CONFIG_FB1_MEMORY_SIZE *SZ_1MB)
#else
#define FB1_VIDEO_SIZE         		(0)
#endif//

#define PMAP_BASE				(0xA8000000-FB_VIDEO_SIZE-EARLYCAM_LOG_SIZE-PARKING_GUIDE_SIZE-REAR_CAMERA_SIZE-REAR_CAMERA_VIQE_SIZE-FB_WMIXER_SIZE)
#define REAR_CAMERA_VIQE_BASE	(PMAP_BASE)
#define REAR_CAMERA_BASE		(REAR_CAMERA_VIQE_BASE+REAR_CAMERA_VIQE_SIZE)
#define PARKING_GUIDE_BASE		(REAR_CAMERA_BASE+REAR_CAMERA_SIZE)
#define EARLYCAM_LOG_BASE       (PARKING_GUIDE_BASE+PARKING_GUIDE_SIZE)
#define FB_VIDEO_BASE			(EARLYCAM_LOG_BASE+EARLYCAM_LOG_SIZE)
#define FB_WMIXER_BASE			(FB_VIDEO_BASE+FB_VIDEO_SIZE)

#define NSK_REV_MEM_BASE		(FB_WMIXER_BASE+FB_WMIXER_SIZE) // should be fixed address 0x88000000
#define SECURED_INBUFF_BASE		(NSK_REV_MEM_BASE+NSK_REV_MEM_SIZE)
#define VIDEO_SBACKUP_BASE		(SECURED_INBUFF_BASE+SECURED_INBUFF_SIZE)
#define OVERLAY_BASE			(VIDEO_SBACKUP_BASE+VIDEO_SBACKUP_SIZE)
#define OVERLAY1_BASE			(OVERLAY_BASE+OVERLAY_SIZE)
#define OVERLAY_LOT_BASE		(OVERLAY1_BASE+OVERLAY1_SIZE)
#define VIDEO_EXT_BASE			(OVERLAY_LOT_BASE+OVERLAY_LOT_SIZE)
#define VIDEO_BASE				(VIDEO_EXT_BASE+VIDEO_EXT_SIZE)
#define VIDEO_THUMB_BASE		(VIDEO_BASE+VIDEO_SIZE)
#define UMP_RESERVED_BASE		(VIDEO_THUMB_BASE+VIDEO_THUMB_SIZE)
#define V4L2_VIDEO0_BASE		(UMP_RESERVED_BASE + UMP_RESERVED_SIZE)
#define V4L2_VIDEO1_BASE		(V4L2_VIDEO0_BASE + V4L2_VIDEO0_SIZE)
#define V4L2_VIDEO2_BASE		(V4L2_VIDEO1_BASE + V4L2_VIDEO1_SIZE)
#define V4L2_VIDEO3_BASE		(V4L2_VIDEO2_BASE + V4L2_VIDEO2_SIZE)
#define V4L2_VIDEO4_BASE		(V4L2_VIDEO3_BASE + V4L2_VIDEO3_SIZE)
#define RAM_CONSOLE_BASE		(V4L2_VIDEO4_BASE + V4L2_VIDEO4_SIZE)
#define JPEG_HEADER_BASE		(RAM_CONSOLE_BASE+RAM_CONSOLE_SIZE)
#define VIQE_BASE			(JPEG_HEADER_BASE)
#if 1	//For Camera Capture Driver
#define JPEG_STREAM_BASE		(JPEG_HEADER_BASE+JPEG_HEADER_SIZE)	// not used jpeg encoding
#define JPEG_RAW_BASE			(JPEG_STREAM_BASE+JPEG_STREAM_SIZE)	// not used jpeg encoding
#define EXT_CAMERA_BASE			(JPEG_RAW_BASE+JPEG_RAW_SIZE)		// not used jpeg encoding
#else
#define EXT_CAMERA_BASE			(VIQE_BASE+VIQE_SIZE)
#endif
#define FB_SCALE_BASE			(EXT_CAMERA_BASE+EXT_CAMERA_SIZE)
#define FB_SCALE0_BASE			(FB_SCALE_BASE+FB_SCALE_SIZE)
#define FB_SCALE1_BASE			(FB_SCALE0_BASE+FB_SCALE0_SIZE)
#define FB_G2D0_BASE			(FB_SCALE1_BASE+FB_SCALE1_SIZE)
#define FB_G2D1_BASE			(FB_G2D0_BASE+FB_G2D0_SIZE)
#define VIDEO_DUAL_BASE			(FB_G2D1_BASE+FB_G2D1_SIZE)
#define NAND_MEM_BASE			(VIDEO_DUAL_BASE+VIDEO_DUAL_SIZE)
#define JPG_ENC_DXB_BASE		(NAND_MEM_BASE+NAND_MEM_SIZE)
#define JPG_RAW_DXB_BASE		(JPG_ENC_DXB_BASE+JPG_ENC_DXB_SIZE)
#define OUTPUT_ATTACH_BASE		(JPG_RAW_DXB_BASE+JPG_RAW_DXB_SIZE)

#define V4L2_VOUT_BASE			(OUTPUT_ATTACH_BASE+OUTPUT_ATTACH_SIZE)
#define V4L2_SUBTITLE_BASE		(V4L2_VOUT_BASE+V4L2_VOUT_SIZE)
#define FB_DUPLICATE_BASE		(V4L2_SUBTITLE_BASE+V4L2_SUBTITLE_SIZE)
#define SECURE_HASH_BASE		(FB_DUPLICATE_BASE+FB_DUPLICATE_SIZE)
#define FB1_VIDEO_BASE			(SECURE_HASH_BASE+SECURE_HASH_SIZE)

#define PMAP_TOTAL				((FB1_VIDEO_BASE-PMAP_BASE)+FB1_VIDEO_SIZE)

#endif
