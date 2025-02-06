/*!
 ***********************************************************************
 \par Copyright
 \verbatim
  ________  _____           _____   _____           ____  ____   ____		
     /     /       /       /       /       /     /   /    /   \ /			
    /     /___    /       /___    /       /____ /   /    /____/ \___			
   /     /       /       /       /       /     /   /    /           \		
  /     /_____  /_____  /_____  /_____  /     / _ /_  _/_      _____/ 		
   																				
  Copyright (c) 2009 Telechips Inc.
  Luther Bldg, 7-20 Sincheon-dong, Songpa-gu, Seoul, Korea
 \endverbatim
 ***********************************************************************
 */
/*!
 ***********************************************************************
 *
 * \file
 *		TCC_JPU_C6.h
 * \date
 *		2015/05/28
 * \author
 *		AV algorithm group(AValgorithm@telechips.com) 
 * \brief
 *		main api
 * \version
 *		0.0 : first beta release (2015/05/28)
 ***********************************************************************
 */
#ifndef _TCC_JPU_CODEC_H_
#define _TCC_JPU_CODEC_H_

#define MAX_NUM_INSTANCE		4

#define PA 0	// physical address
#define VA 1	// virtual  address

typedef int codec_handle_t;			//!< handle
typedef int codec_result_t;			//!< return value
typedef unsigned int codec_addr_t;	//!< address

#define COMP_Y 0
#define COMP_U 1
#define COMP_V 2

#define	YUV_FORMAT_420					0
#define	YUV_FORMAT_422					1
#define	YUV_FORMAT_224					2
#define	YUV_FORMAT_444					3
#define	YUV_FORMAT_400					4

// Decoding Status
#define JPU_DEC_SUCCESS								1
#define JPU_DEC_INVALID_INSTANCE					2
#define JPU_DEC_SUCCESS_PARTIAL_MODE				3


// Encoder Op Code
#define JPU_ENC_INIT				0x00	//!< init
#define JPU_ENC_REG_FRAME_BUFFER	0x01	//!< register frame buffer
#define JPU_ENC_ENCODE				0x12	//!< encode
#define JPU_ENC_CLOSE				0x20	//!< close

// Decoder Op Code
#define JPU_DEC_INIT				0x00	//!< init
#define JPU_DEC_SEQ_HEADER			0x01	//!< decode sequence header 
#define JPU_DEC_REG_FRAME_BUFFER	0x03	//!< register frame buffer
#define JPU_DEC_DECODE				0x10	//!< decode
#define JPU_DEC_CLOSE				0x20	//!< close
#define JPU_CODEC_GET_VERSION		0x3000	//!< get JPU version


#define RETCODE_ERR_MIN_RESOLUTION			101
#define RETCODE_ERR_MAX_RESOLUTION			102

#define JPG_RET_SUCCESS						 0
#define JPG_RET_FAILURE						 1
#define JPG_RET_BIT_EMPTY					 2
#define JPG_RET_EOS							 3
#define JPG_RET_INVALID_HANDLE				 4
#define JPG_RET_INVALID_PARAM				 5
#define JPG_RET_INVALID_COMMAND				 6
#define JPG_RET_ROTATOR_OUTPUT_NOT_SET		 7
#define JPG_RET_ROTATOR_STRIDE_NOT_SET		 8
#define JPG_RET_FRAME_NOT_COMPLETE			 9
#define JPG_RET_INVALID_FRAME_BUFFER		10
#define JPG_RET_INSUFFICIENT_FRAME_BUFFERS	11
#define JPG_RET_INVALID_STRIDE				12
#define JPG_RET_WRONG_CALL_SEQUENCE			13
#define JPG_RET_CALLED_BEFORE				14
#define JPG_RET_NOT_INITIALIZED				15
#define JPG_RET_CODEC_EXIT					16
#define JPG_RET_SPEC_OUT					17
#define JPG_RET_INSUFFICIENT_BITSTREAM_BUF	18
#define	JPG_RET_CODEC_FINISH				19 


//-----------------------------------------------------
// data structure to get information necessary to 
// start decoding from the decoder (this is an output parameter)
//-----------------------------------------------------
typedef struct jpu_dec_initial_info_t
{
	int m_iPicWidth;				//!< {(PicX+15)/16} * 16  (this width  will be used while allocating decoder frame buffers. picWidth  is a multiple of 16)
	int m_iPicHeight;				//!< {(PicY+15)/16} * 16  (this height will be used while allocating decoder frame buffers. picHeight is a multiple of 16)
	int m_iSourceFormat;			//!< the minimum number of frame buffers that are required for decoding. application must allocate at least this number of frame buffers.
	int m_iErrorReason;
	int m_iMinFrameBufferCount;
	int m_iMinFrameBufferSize[4];	//!< minimum frame buffer size
									//!< 0 ( Original Size )
									//!< 1 ( 1/2 Scaling Down )
									//!< 2 ( 1/4 Scaling Down )
									//!< 3 ( 1/8 Scaling Down )
	int m_Reserved[23];
} jpu_dec_initial_info_t;

//! data structure for initializing Video unit
typedef struct jpu_dec_init_t 
{
	codec_addr_t m_RegBaseVirtualAddr;	//!< virtual address BIT_BASE
	codec_addr_t m_BitstreamBufAddr[2];	//!< bitstream buf address : multiple of 4
	int m_iBitstreamBufSize;			//!< bitstream buf size	   : multiple of 1024

	int m_iCbCrInterleaveMode;

	unsigned int m_uiDecOptFlags;

	//! Callback Func
	void* (*m_Memcpy ) ( void*, const void*, unsigned int );	//!< memcpy
	void  (*m_Memset ) ( void*, int, unsigned int );			//!< memset
	int   (*m_Interrupt ) ( void );								//!< hw interrupt (return value is always 0)

	int m_Reserved[23];
} jpu_dec_init_t;

typedef struct jpu_dec_input_t
{
	unsigned int m_BitstreamDataAddr[2];	//!< bitstream data address
	int m_iBitstreamDataSize;				//!< bitstream data size
	int m_Reserved[29];
} jpu_dec_input_t;

typedef struct jpu_dec_buffer_t
{
	codec_addr_t m_FrameBufferStartAddr[2];	//!< physical[0] and virtual[1] address of a frame buffer of the decoder.
	int m_iFrameBufferCount;				//!< allocated frame buffer count
	int m_iJPGScaleRatio;					//!< JPEG Scaling Ratio
											//!< 0 ( Original Size )
											//!< 1 ( 1/2 Scaling Down )
											//!< 2 ( 1/4 Scaling Down )
											//!< 3 ( 1/8 Scaling Down )
	int m_Reserved[28];
} jpu_dec_buffer_t;

//-----------------------------------------------------
// data structure to get resulting information from 
// JPU after decoding a frame
//-----------------------------------------------------

typedef struct jpu_dec_output_info_t
{
	int m_iHeight;					//!< Height of input bitstream. In some cases, this value can be different from the height of previous frames.
	int m_iWidth;					//!< Width of input bitstream. In some cases, this value can be different from the height of previous frames.
	int m_iDecodingStatus;
	int m_iDispOutIdx;				//!< index of output frame buffer
	int m_iConsumedBytes;			//!< consumed bytes
	int m_iNumOfErrMBs;				//!< number of error macroblocks
	int m_Reserved[26];
} jpu_dec_output_info_t;

typedef struct jpu_dec_output_t 
{
	jpu_dec_output_info_t m_DecOutInfo;
	unsigned char* m_pCurrOut[2][3];	//! physical[0] and virtual[1] current  address of Y, Cb, Cr component
	int m_Reserved[25];
} jpu_dec_output_t;

/*!
 ***********************************************************************
 * \brief
 *		TCC_JPU_DEC		: main api function of libjpudec
 * \param
 *		[in]Op			: decoder operation 
 * \param
 *		[in,out]pHandle	: libvpudec's handle
 * \param
 *		[in]pParam1		: init or input parameter
 * \param
 *		[in]pParam2		: output or info parameter
 * \return
 *		If successful, TCC_JPU_DEC returns 0 or plus. Otherwise, it returns a minus value.
 ***********************************************************************
 */
codec_result_t
TCC_JPU_DEC( int Op, codec_handle_t* pHandle, void* pParam1, void* pParam2 );


//------------------------------------------------------------------------------
// encode struct and definition
//------------------------------------------------------------------------------

typedef struct jpu_enc_init_t 
{
	codec_addr_t m_RegBaseVirtualAddr;

	//! Encoding Info
	int m_iSourceFormat;				//!< input YUV format for mjpeg encoding
	int m_iPicWidth;					//!< multiple of 16
	int m_iPicHeight;					//!< multiple of 16

	int m_iEncQuality;					//!< jpeg encoding quality

	int m_iCbCrInterleaveMode;			//!< 0 (chroma separate mode   : CbCr data is written in separate frame memories)
										//!< 1 (chroma interleave mode : CbCr data is interleaved in chroma memory)

	codec_addr_t m_BitstreamBufferAddr[2]; //!< physical address : multiple of 4, virtual address : multiple of 4

	int m_iBitstreamBufferSize;			//!< multiple of 1024
	unsigned int m_uiEncOptFlags;

	//! Callback Func
	void* (*m_Memcpy ) ( void*, const void*, unsigned int );	//!< memcpy
	void  (*m_Memset ) ( void*, int, unsigned int );			//!< memset
	int   (*m_Interrupt ) ( void );								//!< hw interrupt (return value is always 0)

	int m_Reserved[19];
} jpu_enc_init_t;

typedef struct jpu_enc_input_t 
{
	codec_addr_t m_PicYAddr;
	codec_addr_t m_PicCbAddr;
	codec_addr_t m_PicCrAddr;

	codec_addr_t m_BitstreamBufferAddr[2]; //!< physical address
	int m_iBitstreamBufferSize;
	int m_Reserved[26];
} jpu_enc_input_t;

typedef struct jpu_enc_output_t 
{
	codec_addr_t m_BitstreamOut[2];
	int m_iBitstreamOutSize;
	int m_iBitstreamHeaderSize;
	int m_Reserved[28];
} jpu_enc_output_t;

/*!
 ***********************************************************************
 * \brief
 *		TCC_JPU_ENC		: main api function of libjpudec
 * \param
 *		[in]Op			: encoder operation 
 * \param
 *		[in,out]pHandle	: libvpudec's handle
 * \param
 *		[in]pParam1		: init or input parameter
 * \param
 *		[in]pParam2		: output or info parameter
 * \return
 *		If successful, TCC_JPU_ENC returns 0 or plus. Otherwise, it returns a minus value.
 ***********************************************************************
 */
codec_result_t
TCC_JPU_ENC( int Op, codec_handle_t* pHandle, void* pParam1, void* pParam2 );

#endif	// _TCC_JPU_CODEC_H_