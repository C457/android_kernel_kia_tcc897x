/****************************************************************************
linux/arch/arm/mach-tcc893x/include/mach/tcc_viqe_ioctl.h
Description: TCC VIOC h/w block 

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

#define VIQE_DEV_NAME		"viqe"
#define VIQE_DEV_MAJOR		230
#define VIQE_DEV_MINOR		0

#define IOCTL_VIQE_INITIALIZE		0xAF0
#define IOCTL_VIQE_EXCUTE			0xAF1
#define IOCTL_VIQE_DEINITIALIZE		0xAF2
#define IOCTL_VIQE_SETTING			0xAF3
/*****************************************************************************
*
* structures
*
******************************************************************************/
typedef struct
{
	int lcdCtrlNo;
	int scalerCh;
	int onTheFly;
	unsigned int useWMIXER;
	unsigned int srcWidth;
	unsigned int srcHeight;
	unsigned int crop_top, crop_bottom, crop_left, crop_right;
	int deinterlaceM2M;
	int renderCnt;
	int OddFirst;
	int DI_use;
	int multi_hwr;

	unsigned int address[6];
	unsigned int dstAddr;
}VIQE_DI_TYPE;

