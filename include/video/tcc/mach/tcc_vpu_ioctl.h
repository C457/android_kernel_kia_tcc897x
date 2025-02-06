/****************************************************************************
 *   FileName    : tcc_vpu_ioctl.h
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

#ifndef _VDEC_IOCTL_H_
#define _VDEC_IOCTL_H_

typedef struct {
	int result;
	codec_handle_t gsVpuDecHandle;
	dec_init_t gsVpuDecInit;
}VDEC_INIT_t;

typedef struct {
	int result;
	unsigned int stream_size;
	dec_initial_info_t gsVpuDecInitialInfo;
}VDEC_SEQ_HEADER_t;

typedef struct {
	int result;
	dec_buffer_t gsVpuDecBuffer;
}VDEC_SET_BUFFER_t;

typedef struct {
	int result;
	dec_input_t gsVpuDecInput;
	dec_output_t gsVpuDecOutput;
}VDEC_DECODE_t;

typedef struct {
	int result;
	dec_ring_buffer_status_out_t gsVpuDecRingStatus;
}VDEC_RINGBUF_GETINFO_t;

typedef struct {
	int result;
	dec_init_t gsVpuDecInit;
	dec_ring_buffer_setting_in_t gsVpuDecRingFeed;
}VDEC_RINGBUF_SETBUF_t;

typedef struct {
	int result;
	int iCopiedSize;
	int iFlushBuf;
}VDEC_RINGBUF_SETBUF_PTRONLY_t;

typedef struct {
	int result;
	char * pszVersion;
	char * pszBuildData;
}VDEC_GET_VERSION_t;

typedef struct {
	unsigned int start_addr_phy;
	int size;
}VDEC_RENDERED_BUFFER_t;

////////////////////////////////////////////////////////////////////////////////////////
/* VPU ENCODER */
#define V_ENC_INIT					10	//!< init
#define V_ENC_REG_FRAME_BUFFER		11	//!< register frame buffer
#define V_ENC_PUT_HEADER			12
#define V_ENC_ENCODE				13	//!< encode
#define V_ENC_CLOSE					14	//!< close

#define V_ENC_ALLOC_MEMORY			16
#define V_ENC_GENERAL_RESULT		17
#define V_ENC_INIT_RESULT			18
#define V_ENC_PUT_HEADER_RESULT		19
#define V_ENC_ENCODE_RESULT			20
#define V_ENC_FREE_MEMORY           21

typedef struct {
	int result;
	codec_handle_t gsVpuEncHandle;
	enc_init_t gsVpuEncInit;
	enc_initial_info_t gsVpuEncInitialInfo;
}VENC_INIT_t;

typedef struct {
	int result;
	enc_buffer_t gsVpuEncBuffer;
}VENC_SET_BUFFER_t;

typedef struct {	
	int result;
	enc_header_t gsVpuEncHeader;
}VENC_PUT_HEADER_t;

typedef struct {
	int result;
	enc_input_t gsVpuEncInput;
	enc_output_t gsVpuEncOutput;
}VENC_ENCODE_t;

#endif
