/****************************************************************************
FileName    : kernel/arch/arm/mach-tcc893x/include/mach/tcc_ccfb_ioctl.h
Description : 

Copyright (C) 2013 Telechips Inc.

This program is free software; you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation;
either version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 59 Temple Place,
Suite 330, Boston, MA 02111-1307 USA
****************************************************************************/

#ifndef _TCC_CCFB_H
#define _TCC_CCFB_H

#include "autoconf.h"

/* ioctl enum */
#define CCFB_OPEN			0x100
#define CCFB_CLOSE			0x101
#define CCFB_GET_CONFIG	0x102
#define CCFB_SET_CONFIG	0x103
#define CCFB_DISP_UPDATE	0x104
#define CCFB_DISP_ENABLE	0x105
#define CCFB_DISP_DISABLE	0x106

typedef enum
{
	CCFB_LCDC_NONE,
	CCFB_LCDC_0,
	CCFB_LCDC_1,
	CCFB_LCDC_MAX
}ccfb_lcdc_type;

typedef enum
{
	CCFB_IMG_FMT_RGB565,
	CCFB_IMG_FMT_ARGB4444,	// AnD
	CCFB_IMG_FMT_ARGB888,	// android
	CCFB_IMG_FMT_ARGB8888,	// AnD
	CCFB_IMG_FMT_MAX
}ccfb_img_fmt;

typedef struct
{
	uint32_t disp_fmt;
	uint32_t disp_x;
	uint32_t disp_y;
	uint32_t disp_w;
	uint32_t disp_h;
	uint32_t disp_m;	/* refer to LISC register for the value */

	uint32_t mem_w;
	uint32_t mem_h;

	uint32_t chroma_en;
	uint32_t chroma_R;
	uint32_t chroma_G;
	uint32_t chroma_B;

}ccfb_res_config_t;

typedef struct
{
	uint32_t			curLcdc;
	ccfb_res_config_t	res;
} ccfb_config_t;

#endif
