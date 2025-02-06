/****************************************************************************
 *	 TCC Version 0.6
 *	 Copyright (c) telechips, Inc.
 *	 ALL RIGHTS RESERVED
 *
****************************************************************************/

#ifndef _TCC_MEM_IOCTL_H
#define _TCC_MEM_IOCTL_H

#define TCC_LIMIT_PRESENTATION_WINDOW 		0x10
#define TCC_DRAM_16BIT_USED					0x11
#define TCC_VIDEO_SET_DISPLAYING_IDX  		0x12
#define TCC_VIDEO_GET_DISPLAYING_IDX  		0x13
#define TCC_VIDEO_SET_DISPLAYED_BUFFER_ID	0x14
#define TCC_VIDEO_GET_DISPLAYED_BUFFER_ID	0x15

#define TCC_8925S_IS_XX_CHIP		  		0x20


typedef struct
{
	unsigned int 	istance_index;
	int 	index;
}vbuffer_manager;

#endif //_TCC_MEM_IOCTL_H

