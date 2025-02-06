/*
 * linux/arch/arm/mach-tcc893x/include/mach/vioc_wdma.h
 * Author:  <linux@telechips.com>
 * Created: June 10, 2008
 * Description: TCC VIOC h/w block 
 *
 * Copyright (C) 2008-2009 Telechips
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see the file COPYING, or write
 * to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#ifndef __VIOC_WDMA_H__
#define	__VIOC_WDMA_H__
#include "vioc_ireq.h"
#include "tcc_video_regs.h"

#define 	VIOC_WDMA_IREQ_UPD_MASK 		0x00000001UL
#define 	VIOC_WDMA_IREQ_SREQ_MASK 		0x00000002UL
#define 	VIOC_WDMA_IREQ_ROLL_MASK 		0x00000004UL
#define 	VIOC_WDMA_IREQ_ENR_MASK 		0x00000008UL
#define 	VIOC_WDMA_IREQ_ENF_MASK 		0x00000010UL
#define 	VIOC_WDMA_IREQ_EOFR_MASK 		0x00000020UL
#define 	VIOC_WDMA_IREQ_EOFF_MASK 		0x00000040UL
#define 	VIOC_WDMA_IREQ_SEOFR_MASK 		0x00000080UL
#define 	VIOC_WDMA_IREQ_SEOFF_MASK 		0x00000100UL
#define 	VIOC_WDMA_IREQ_STSEN_MASK 		0x20000000UL
#define 	VIOC_WDMA_IREQ_STBF_MASK 		0x40000000UL
#define 	VIOC_WDMA_IREQ_STEOF_MASK 		0x80000000UL
#define 	VIOC_WDMA_IREQ_ALL_MASK 		0xFFFFFFFFUL

/******************************************************************************
*
*  WDMA YUV-to-RGB Converter Mode Register
*
*  0 - The Range for RGB is 16 ~ 235,"Studio Color". Normally SDTV
*  1 - The Range for RGB is  0 ~ 255,"Conputer System Color". Normally SDTV
*  2 - The Range for RGB is 16 ~ 235,"Studio Color". Normally HDTV
*  3 - The Range for RGB is  0 ~ 255,"Conputer System Color". Normally HDTV
*  4 - The Range for RGB is 16 ~ 235,"Studio Color". Normally UHDTV
*  5 - The Range for RGB is  0 ~ 255,"Conputer System Color". Normally UHDTV
*
*******************************************************************************/
#define 	R2YMD_SDTV_LR	0
#define		R2YMD_SDTV_FR	1
#define		R2YMD_HDTV_LR	2
#define		R2YMD_HDTV_FR	3
#define		R2YMD_UHDTV_LR	4
#define		R2YMD_UHDTV_FR	5

typedef struct
{
	unsigned int ImgSizeWidth;
	unsigned int ImgSizeHeight;
	unsigned int TargetWidth;
	unsigned int TargetHeight;
	unsigned int ImgFormat;
	unsigned int BaseAddress;
	unsigned int BaseAddress1;
	unsigned int BaseAddress2;
	unsigned int Interlaced;
	unsigned int ContinuousMode;
	unsigned int SyncMode;
	unsigned int AlphaValue;
	unsigned int Hue;
	unsigned int Bright;
	unsigned int Contrast;
} VIOC_WDMA_IMAGE_INFO_Type;

typedef	struct
{
	unsigned	FMT		:  5;
	unsigned	FMT10	:  3;
	unsigned	R2Y 	:  1;
	unsigned	R2YMD	:  3;
	unsigned	SWAP 	:  3;
	unsigned	BR		:  1;
	unsigned	UPD  	:  1;
	unsigned	Y2R		:  1;
	unsigned	Y2RMD	:  3;
	unsigned	RESERVE0:  1;
	unsigned	SREQ	:  1;
	unsigned	CONT	:  1;
	unsigned	DITHEN	:  1;
	unsigned	RESERVE1:  2;
	unsigned	DITHSEL	:  1;
	unsigned	IEN		:  1;
	unsigned	FU		:  1;
	unsigned	RESERVE2:  1;
	unsigned	INTL	:  1;
}	VIOC_WDMA_CTRL;

typedef	union
{
	unsigned long		nREG;
	VIOC_WDMA_CTRL		bREG;
}	VIOC_WDMA_CTRL_u;

typedef	struct
{
	unsigned	SYNCSEL :  8;
	unsigned	SEN		:  1;
	unsigned	SYNCMD_ADDR	:  1;
	unsigned	RESERVE0		:  1;
	unsigned	SYNCMD_SENS	:  1;
	unsigned	RESERVE1		:  4;
	unsigned	MAXRATE		:  8;
	unsigned	RESERVE2		:  7;
	unsigned	REN			:  1;
}	VIOC_WDMA_RATE;

typedef	union
{
	unsigned long		nREG;
	VIOC_WDMA_RATE		bREG;
}	VIOC_WDMA_RATE_u;

typedef	struct
{
	unsigned	WIDTH   	: 13;
	unsigned	RESERVE0	:  3;
	unsigned	HEIGHT   	: 13;
	unsigned	RESERVE1	:  3;
}	VIOC_WDMA_SIZE;

typedef	union
{
	unsigned long		nREG;
	VIOC_WDMA_SIZE		bREG;
}	VIOC_WDMA_SIZE_u;

typedef	struct
{
	unsigned	OFFSET0   	: 16;
	unsigned	OFFSET1   	: 16;
}	VIOC_WDMA_OFFS;

typedef	union
{
	unsigned long		nREG;
	VIOC_WDMA_OFFS		bREG;
}VIOC_WDMA_OFFS_u;

typedef	struct
{
	unsigned	BG0		: 10;
	unsigned	BG1		: 10;
	unsigned	RESERVE0: 12;

	unsigned	BG2		: 10;
	unsigned	BG3		: 10;
	unsigned	RESERVE1: 12;
}VIOC_WDMA_BG;

typedef	union
{
	unsigned long		nREG[2];
	VIOC_WDMA_BG		bREG;
}VIOC_WDMA_BG_u;

typedef	struct
{
	unsigned	PTS		: 16;
	unsigned	RESERVE0    	: 16;
}VIOC_WDMA_PTS;

typedef	union
{
	unsigned long		nREG;
	VIOC_WDMA_PTS		bREG;
}VIOC_WDMA_PTS_u;

typedef	struct
{
	unsigned	DITH00	: 3;
	unsigned	RESERVE0	: 1;
	unsigned	DITH01	: 3;
	unsigned	RESERVE1	: 1;
	unsigned	DITH02	: 3;
	unsigned	RESERVE2	: 1;
	unsigned	DITH03	: 3;
	unsigned	RESERVE3	: 1;
	unsigned	DITH10	: 3;
	unsigned	RESERVE4	: 1;
	unsigned	DITH11	: 3;
	unsigned	RESERVE5	: 1;
	unsigned	DITH12	: 3;
	unsigned	RESERVE6	: 1;
	unsigned	DITH13	: 3;
	unsigned	RESERVE7	: 1;	

	unsigned	DITH20	: 3;
	unsigned	RESERVE8	: 1;
	unsigned	DITH21	: 3;
	unsigned	RESERVE9	: 1;
	unsigned	DITH22	: 3;
	unsigned	RESERVE10	: 1;
	unsigned	DITH23	: 3;
	unsigned	RESERVE11	: 1;
	unsigned	DITH30	: 3;
	unsigned	RESERVE12	: 1;
	unsigned	DITH31	: 3;
	unsigned	RESERVE13	: 1;
	unsigned	DITH32	: 3;
	unsigned	RESERVE14	: 1;
	unsigned	DITH33	: 3;
	unsigned	RESERVE15	: 1;	
}VIOC_WDMA_DMAT;

typedef	union
{
	unsigned long	nREG[2];
	VIOC_WDMA_DMAT	bREG;
}VIOC_WDMA_DMAT_u;

//AnD_TODO: check EHN register that is no exist in the datasheet !!!!
typedef	struct
{
	unsigned	CONTRAST: 8;
	unsigned	BRIGHT	: 8;
	unsigned	HUE		: 8;
	unsigned	RESERVE0  : 8;
}VIOC_WDMA_ENH;

typedef	union
{
	unsigned long		nREG;
	VIOC_WDMA_ENH		bREG;
}VIOC_WDMA_ENH_u;

typedef	struct
{
	unsigned	ROLLCNT   	: 16;
	unsigned	RESERVE0     : 15;
	unsigned	ROLL     	:  1;
}VIOC_WDMA_ROLL;

typedef	union
{
	unsigned long		nREG;
	VIOC_WDMA_ROLL		bREG;
}VIOC_WDMA_ROLL_u;

typedef	struct
{
	unsigned	UPDDONE	:  1;	// Register Update Done
	unsigned	STOPREQ	:  1;	// Stop Request
	unsigned	ROLL 		:  1;	// Roll Interrupt
	unsigned	ENRISE		:  1;	// Frame Synchronized Enable Rising
	unsigned	ENFALL		:  1;	// Frame Synchronized Enable Falling (Disable-Done)
	unsigned	EOFRISE		:  1;	// EOF Rising
	unsigned	EOFFALL	:  1;	// EOF Falling
	unsigned	SEOFRISE	:  1;	// SYNC EOF Rising
	unsigned	SEOFFALL	:  1;	// SYNC EOF Falling
	unsigned	RESERVE0	:  7;
	unsigned	RESERVE1	: 13;
	unsigned	STS_SEN		:  1;	// Synchronized Enable
	unsigned	STS_BFIELD	:  1;	// Bottom field indicator
	unsigned	STS_EOF	:  1;	// EOF
}VIOC_WDMA_STATUS;

typedef	union
{
	unsigned long		nREG;
	VIOC_WDMA_STATUS	bREG;
}VIOC_WDMA_STATUS_u;

typedef	struct
{
	unsigned	UPDDONE	:  1;	// Register Update Done
	unsigned	STOPREQ	:  1;	// Stop Request
	unsigned	ROLL 		:  1;	// Roll Interrupt
	unsigned	ENRISE		:  1;	// Synchronized Enable Rising
	unsigned	ENFALL		:  1;	// Synchronized Enable Falling (Disable-Done)
	unsigned	EOFRISE		:  1;	// EOF Rising
	unsigned	EOFFALL	:  1;	// EOF Falling
	unsigned	SEOFRISE	:  1;	// SYNC EOF Rising
	unsigned	SEOFFALL	:  1;	// SYNC EOF Falling
	unsigned	RESERVE0	:  7;
	unsigned	RESERVE1	: 16;
}VIOC_WDMA_IRQMSK;

typedef	union
{
	unsigned long		nREG;
	VIOC_WDMA_IRQMSK	bREG;
}VIOC_WDMA_IRQMSK_u;

typedef	struct _VIOC_WDMA
{
	volatile VIOC_WDMA_CTRL_u 		uCTRL; 			// 0x00  R/W  0x00000400 WMDA Control Register
	volatile VIOC_WDMA_RATE_u 		uRATE; 			// 0x04  R/W  0x00000000 WDMA Rate Control Register
	volatile VIOC_WDMA_SIZE_u 		uSIZE; 			// 0x08  R/W  0x00000000 WDMA Size Register
	volatile unsigned  int 			uBASE0; 		// 0x0C  R/W  0x00000000 WDMA Base Address 0 Register
	volatile unsigned  int 			uCBASE; 		// 0x10  R/W  0x00000000 WDMA Current Address Register
	volatile unsigned  int 			uBASE1; 		// 0x14  R/W  0x00000000 WDMA Base Address 1 Register
	volatile unsigned  int 			uBASE2; 		// 0x18  R/W  0x00000000 WDMA Base Address 2 Register
	volatile VIOC_WDMA_OFFS_u 		uOFFS; 			// 0x1C  R/W  0x00000000 WDMA Offset Register
	volatile unsigned  int 			reserved0; 		// 0x20
	volatile VIOC_WDMA_BG_u 		uBG; 			// 0x24, 0x28  R/W  0x00000000 WDMA Back Ground Color Register
	volatile VIOC_WDMA_PTS_u 		uPTS; 			// 0x2C  R/W  0x00000000 WDMA PTS Register
	volatile VIOC_WDMA_DMAT_u 		uDMAT; 			// 0x30, 0x34  R/W  0x00000000 WDMA Dither Matrix 0,1 Register
	volatile VIOC_WDMA_ENH_u 		uENH; 			// 0x38  R/W  0x00000000 WDMA Color Enhancement Register
	volatile VIOC_WDMA_ROLL_u 		uROLL; 			// 0x3C  R/W  0x00000000 WDMA Rolling Control Register
	volatile unsigned  int 			uSBASE; 		// 0x40  R/W  0x00000000 WMA Synchronized Base Address Register
	volatile VIOC_WDMA_STATUS_u 	uIRQSTS; 		// 0x44  R/W  0x00000000 WDMA Interrupt Status Register
	volatile VIOC_WDMA_IRQMSK_u 	uIRQMSK; 		// 0x48  R/W  0x000001FF WDMA Interrupt Mask Register
}VIOC_WDMA, *PVIOC_WDMA;

/* Interface APIs. */
extern void VIOC_WDMA_SetWifiDisplayImageConfig(VIOC_WDMA * pWDMA, VIOC_WDMA_IMAGE_INFO_Type * ImageCfg, unsigned int r2yEn);
extern void VIOC_WDMA_SetImageEnable(VIOC_WDMA * pWDMA, unsigned int nContinuous);
extern void VIOC_WDMA_SetImageDisable(VIOC_WDMA * pWDMA);
extern void VIOC_WDMA_SetImageUpdate(VIOC_WDMA * pWDMA);
extern void VIOC_WDMA_SetImageFormat(VIOC_WDMA *pWDMA, unsigned int nFormat);
extern void VIOC_WDMA_SetImageRGBSwapMode(VIOC_WDMA *pWDMA, unsigned int rgb_mode);
extern void VIOC_WDMA_SetImageInterlaced(VIOC_WDMA *pWDMA, unsigned int intl);
extern void VIOC_WDMA_SetImageR2YMode(VIOC_WDMA *pWDMA, unsigned int r2y_mode);
extern void VIOC_WDMA_SetImageR2YEnable(VIOC_WDMA *pWDMA, unsigned int enable);
extern void VIOC_WDMA_SetImageY2RMode(VIOC_WDMA *pWDMA, unsigned int y2r_mode);
extern void VIOC_WDMA_SetImageY2REnable(VIOC_WDMA *pWDMA, unsigned int enable);
extern void VIOC_WDMA_SetImageSyncControl(VIOC_WDMA *pWDMA, unsigned int enable, unsigned int RDMA);
extern void VIOC_WDMA_SetImageDither(VIOC_WDMA *pWDMA, unsigned int nDithEn, unsigned int nDithSel, unsigned char mat[4][4]);
extern void VIOC_WDMA_SetImageEnhancer(VIOC_WDMA *pWDMA, unsigned int nContrast, unsigned int nBright, unsigned int nHue);
extern void VIOC_WDMA_SetImageSize(VIOC_WDMA *pWDMA, unsigned int sw, unsigned int sh);
extern void VIOC_WDMA_SetImageBase(VIOC_WDMA *pWDMA, unsigned int nBase0, unsigned int nBase1, unsigned int nBase2);
extern void VIOC_WDMA_SetImageOffset(VIOC_WDMA *pWDMA, unsigned int imgFmt, unsigned int imgWidth);
extern void VIOC_WDMA_SetImageOffset_withYV12(VIOC_WDMA *pWDMA, unsigned int imgWidth);
extern void VIOC_WDMA_SetIreqMask(VIOC_WDMA * pWDMA, unsigned int mask, unsigned int set);
extern void VIOC_WDMA_SetIreqStatus(VIOC_WDMA * pWDMA, unsigned int mask, unsigned int set);
extern void VIOC_WDMA_ClearEORF(VIOC_WDMA * pWDMA);
extern void VIOC_WDMA_ClearEOFF(VIOC_WDMA * pWDMA);
extern void VIOC_WDMA_IreqHandler(unsigned int vectorID);
extern void VIOC_WDMA_SWReset(PVIOC_IREQ_CONFIG pIrgConfig, unsigned int WDMA);
extern void VIOC_WDMA_GetStatus(VIOC_WDMA *pWDMA, unsigned int *status);

extern void VIOC_WDMA_SetContinuousMode(VIOC_WDMA * pWDMA, unsigned int enable);
extern unsigned int VIOC_WDMA_IsImageEnable(VIOC_WDMA * pWDMA);
extern void VIOC_WDMA_DUMP(VIOC_WDMA *pWDMA);
#endif
