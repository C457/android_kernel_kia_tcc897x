/****************************************************************************
 *   FileName    : tcc_hevc_ioctl.h
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
#include "tcc_video_common.h"

#ifndef _HEVC_IOCTL_H_
#define _HEVC_IOCTL_H_

#define HEVC_MAX (VPU_ENC)

typedef struct {
	int result;
	codec_handle_t gsHevcDecHandle;
	hevc_dec_init_t gsHevcDecInit;
}HEVC_INIT_t;

typedef struct {
	int result;
	unsigned int stream_size;
	hevc_dec_initial_info_t gsHevcDecInitialInfo;
}HEVC_SEQ_HEADER_t;

typedef struct {
	int result;
	hevc_dec_buffer_t gsHevcDecBuffer;
}HEVC_SET_BUFFER_t;

typedef struct {
	int result;
	hevc_dec_input_t gsHevcDecInput;
	hevc_dec_output_t gsHevcDecOutput;
}HEVC_DECODE_t;

typedef struct {
	int result;
	hevc_dec_ring_buffer_status_out_t gsHevcDecRingStatus;
}HEVC_RINGBUF_GETINFO_t;

typedef struct {
	int result;
	hevc_dec_init_t gsHevcDecInit;
	hevc_dec_ring_buffer_setting_in_t gsHevcDecRingFeed;
}HEVC_RINGBUF_SETBUF_t;

typedef struct {
	int result;
	int iCopiedSize;
	int iFlushBuf;
}HEVC_RINGBUF_SETBUF_PTRONLY_t;

typedef struct {
	int result;
	char * pszVersion;
	char * pszBuildData;
}HEVC_GET_VERSION_t;
#endif
