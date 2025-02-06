/* linux/arch/arm/video/tcc-msm/irq.c
 *
 * Copyright (C) 2008, 2009 Telechips, Inc.
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

#include "vioc_wdma.h"

 #define WDMA_IOC_MAGIC		'w'

#define TCC_WDMA_IOCTRL 					0x9003
#define TCC_WDMA_START						0x9002
#define TCC_WDMA_END						0x9001


/*
struct VIOC_WDMA_IMAGE_INFO_Type
{
	unsigned int ImgSizeWidth;
	unsigned int ImgSizeHeight;
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
};


#define TCC_WDMA_START		_IO(WDMA_IOC_MAGIC,0)
#define TCC_WDMA_END		_IO(WDMA_IOC_MAGIC,1)

#define TCC_WDMA_IOCTRL		_IOR(WDMA_IOC_MAGIC, 0, VIOC_WDMA_IMAGE_INFO_Type)
//#define TCC_WDMA_IOCTRL		_IOR(WDMA_IOC_MAGIC, 0, unsigned int)
*/

#ifndef ADDRESS_ALIGNED
#define ADDRESS_ALIGNED
#define ALIGN_BIT 							(0x8-1)
#define BIT_0 								3
#define GET_ADDR_YUV42X_spY(Base_addr) 		(((((unsigned int)Base_addr) + ALIGN_BIT)>> BIT_0)<<BIT_0)
#define GET_ADDR_YUV42X_spU(Yaddr, x, y) 	(((((unsigned int)Yaddr+(x*y)) + ALIGN_BIT)>> BIT_0)<<BIT_0)
#define GET_ADDR_YUV422_spV(Uaddr, x, y) 	(((((unsigned int)Uaddr+(x*y/2)) + ALIGN_BIT) >> BIT_0)<<BIT_0)
#define GET_ADDR_YUV420_spV(Uaddr, x, y) 	(((((unsigned int)Uaddr+(x*y/4)) + ALIGN_BIT) >> BIT_0)<<BIT_0)
#endif

typedef enum {
	WDMA_POLLING,
	WDMA_INTERRUPT,
	WDMA_NOWAIT
} WDMA_RESPONSE_TYPE;


