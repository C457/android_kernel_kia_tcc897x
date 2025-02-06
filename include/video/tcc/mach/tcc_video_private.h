/****************************************************************************
 *   FileName    : tcc_video_private.h
 *   Description : 
 ****************************************************************************
 *
 *   TCC Version 1.0
 *   Copyright (c) Telechips Inc.
 *   All rights reserved 
 
This source code contains confidential information of Telechips.
Any unauthorized use without a written permission of Telechips including not limited to re-distribution in source or binary form is strictly prohibited.
This source code is provided "AS IS" and nothing contained in this source code shall constitute any express or implied warranty of any kind, including without limitation, any warranty of merchantability, fitness for a particular purpose or non-infringement of any patent, copyright or other third party intellectual property right. No warranty is made, express or implied, regarding the information's accuracy, completeness, or performance. 
In no event shall Telechips be liable for any claim, damages or other liability arising from, out of or in connection with this source code or the use in the source code. 
This source code is provided subject to the terms of a Mutual Non-Disclosure Agreement between Telechips and Company.
*
****************************************************************************/

#ifndef VIDEO_PRIVATE_H_
#define VIDEO_PRIVATE_H_

#include "TCC_HEVCDEC.h"

#define VPU_BUFFER_MANAGE_COUNT 6

typedef struct TCC_PLATFORM_PRIVATE_PMEM_INFO
{
	unsigned int width;
	unsigned int height;
	unsigned int format;
	unsigned int offset[3];
	unsigned int optional_info[20];
/*
	0 : Buffer width
	1 : Buffer height
	2 : Frame Width
	3 : Frame Height
	4 : buffer_id
	5 : timeStamp
	6 : curTime
	7 : flags
	8 : framerate
	9 : display out index for MVC
	10 : MVC Base addr0
	11 : MVC Base addr1
	12 : MVC Base addr2
	13 : Vsync enable field
	14 : display out index of VPU
	15 : vpu instance-index.
	16 : mapConv_info.
	17 : local playback : 1, streaming: 0
	18 : Player Index for PIP
	19 : not reserved
*/
	unsigned char name[6];
	unsigned int unique_addr;
	unsigned int copied; //to check if decoded data is copied into gralloc buffer
	hevc_dec_MapConv_info_t mapConv_info;
} TCC_PLATFORM_PRIVATE_PMEM_INFO;
#endif
