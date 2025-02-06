/****************************************************************************
FileName    : kernel/arch/arm/mach-tcc893x/include/mach/tcc_component_ioctl.h
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
 
#ifndef _COMPONENT_IOCTL_H_
#define _COMPONENT_IOCTL_H_

#include "tcc_Gre2d_type.h"

#define TCC_COMPONENT_NTSC_WIDTH		720
#define TCC_COMPONENT_NTSC_HEIGHT		480
#define TCC_COMPONENT_PAL_WIDTH			720
#define TCC_COMPONENT_PAL_HEIGHT		576
#define TCC_COMPONENT_720P_WIDTH		1280
#define TCC_COMPONENT_720P_HEIGHT		720
#define TCC_COMPONENT_1080I_WIDTH		1920
#define TCC_COMPONENT_1080I_HEIGHT		1080
#define TCC_COMPONENT_MAX_WIDTH			TCC_COMPONENT_1080I_WIDTH
#define TCC_COMPONENT_MAX_HEIGHT		TCC_COMPONENT_1080I_HEIGHT

#define TCC_COMPONENT_IOCTL_START		10
#define TCC_COMPONENT_IOCTL_UPDATE		20
#define TCC_COMPONENT_IOCTL_END			30
#define TCC_COMPONENT_IOCTL_PROCESS		40
#define TCC_COMPONENT_IOCTL_ATTACH		50
#define TCC_COMPONENT_IOCTL_DETACH		60

#define TCC_COMPONENT_IOCTL_BLANK		70
#define TCC_COPONENT_IOCTL_GET_SUSPEND_STATUS 80

#define TCC_COMPONENT_IOCTL_HPD_SWITCH_STATUS	90


/**
 * COMPONENT_CABLE_OUT indicates COMPONENT cable out.
 */
#define COMPONENT_CABLE_OUT   0

/**
 * COMPONENT_CABLE_IN indicates COMPONENT cable in.
 */
#define COMPONENT_CABLE_IN    1

typedef enum{
	COMPONENT_LCDC_0,
	COMPONENT_LCDC_1,
	COMPONENT_LCDC_MAX
}TCC_COMPONENT_LCDC_TYPE;

typedef enum{
	COMPONENT_NTST_M,
	COMPONENT_PAL,
	COMPONENT_720P,
	COMPONENT_1080I,
	COMPONENT_MAX,
}TCC_COMPONENT_MODE_TYPE;

typedef enum{
	COMPONENT_DISP_LAYER_0,
	COMPONENT_DISP_LAYER_1,
	COMPONENT_DISP_LAYER_2,
	COMPONENT_DISP_LAYER_MAX
}TCC_COMPONENT_LAYER_TYPE;


typedef struct
{
	unsigned char lcdc;
	unsigned char mode;
} TCC_COMPONENT_START_TYPE;


typedef G2D_DATA_FM TCC_COMPONENT_FORMAT_TYPE ;
typedef G2D_OP_MODE TCC_COMPONENT_ROATE_TYPE;

typedef struct
{
	TCC_COMPONENT_LAYER_TYPE	layer;
	char 						enable;
	unsigned int  				frame_pix_width; 	// Source Frame Pixel Size sx
	unsigned int  				frame_pix_height;	// Source Frame Pixel Size sy
	unsigned int  				src_off_sx;			// Source Pixel Offset sx
	unsigned int  				src_off_sy;			// Source Pixel Offset sy
	unsigned int  				img_pix_width;		// Source Image Pixel width
	unsigned int  				img_pix_height;		// Source Image Pixel height
	unsigned int  				win_off_sx;			// Window Pixel Offset sx
	unsigned int  				win_off_sy;  		// Window Pixel Offset sy
	TCC_COMPONENT_ROATE_TYPE 	rotate;
	TCC_COMPONENT_FORMAT_TYPE 	fmt;
	unsigned int				pbuffY;
	unsigned int 				pbuffU;
	unsigned int 				pbuffV;
} TCC_COMPONENT_DISPLAY_TYPE;

#endif 
