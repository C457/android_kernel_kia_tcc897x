/****************************************************************************
One line to give the program's name and a brief idea of what it does.
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

#ifndef _TCC_CAMERA_IOCTL_H_
#define _TCC_CAMERA_IOCTL_H_

//#define ATAG_CAMERA     0x5441000c

#if defined(CONFIG_TCC_REAR_CAMERA_DRV)
typedef struct RCAM_LINE_POSITION_INFO_ {
	unsigned long line_x;
	unsigned long line_y;
}RCAM_LINE_POSITION_INFO, *PRCAM_LINE_POSITION_INFO;

typedef struct RCAM_LINE_SIZE_INFO_ {
	unsigned long width;
	unsigned long height;
}RCAM_LINE_SIZE_INFO, *PRCAM_LINE_SIZE_INFO;

typedef struct RCAM_LINE_BUFFER_INFO_ {
	unsigned long index;
	unsigned long rear_linebuf_addr;
	unsigned long buffer_size;
}RCAM_LINE_BUFFER_INFO, *PRCAM_LINE_BUFFER_INFO;

typedef struct RCAM_LINE_BUFFER_UPDATE_INFO_ {
	unsigned long mBufnum;

	RCAM_LINE_SIZE_INFO mSize;
	RCAM_LINE_POSITION_INFO mPos;
}RCAM_LINE_BUFFER_UPDATE_INFO, *PRCAM_LINE_UFFER_UPDATE_INFO;
#endif

typedef struct {
	int x;
	int y;
	int max_x;
	int max_y;
}pinch_info;

/* Our own specific controls */
#define V4L2_CID_ISO				V4L2_CID_PRIVATE_BASE+0
#define V4L2_CID_EFFECT				V4L2_CID_PRIVATE_BASE+1
#define V4L2_CID_ZOOM				V4L2_CID_PRIVATE_BASE+2
#define V4L2_CID_FLIP				V4L2_CID_PRIVATE_BASE+3
#define V4L2_CID_SCENE				V4L2_CID_PRIVATE_BASE+4
#define V4L2_CID_METERING_EXPOSURE		V4L2_CID_PRIVATE_BASE+5
#define V4L2_CID_FLASH				V4L2_CID_PRIVATE_BASE+6
#define V4L2_CID_FOCUS_MODE			V4L2_CID_PRIVATE_BASE+7
#define V4L2_CID_LAST_PRIV			V4L2_CID_FLASH
#define V4L2_CID_MAX				V4L2_CID_LAST_PRIV+1

#define VIDIOC_USER_JPEG_CAPTURE		_IOWR ('V', BASE_VIDIOC_PRIVATE+1, int)
#define VIDIOC_USER_GET_CAPTURE_INFO		_IOWR ('V', BASE_VIDIOC_PRIVATE+2, TCCXXX_JPEG_ENC_DATA)
#define VIDIOC_USER_PROC_AUTOFOCUS		_IOWR ('V', BASE_VIDIOC_PRIVATE+3, int)
#define VIDIOC_USER_SET_CAMINFO_TOBEOPEN	_IOWR ('V', BASE_VIDIOC_PRIVATE+4, int)
#define VIDIOC_USER_GET_MAX_RESOLUTION		_IOWR ('V', BASE_VIDIOC_PRIVATE+5, int)
#define VIDIOC_USER_GET_SENSOR_FRAMERATE	_IOWR ('V', BASE_VIDIOC_PRIVATE+6, int)
#define VIDIOC_USER_GET_ZOOM_SUPPORT		_IOWR ('V', BASE_VIDIOC_PRIVATE+7, int)
#define VIDIOC_USER_SET_CAMERA_ADDR		_IOWR ('V', BASE_VIDIOC_PRIVATE+8, struct v4l2_requestbuffers)
#define VIDIOC_USER_INT_CHECK			_IOWR ('V', BASE_VIDIOC_PRIVATE+9, int)

#define VIDEOC_USER_SET_ZOOM_RECT				_IOWR ('V', BASE_VIDIOC_PRIVATE+11, pinch_info)
#define VIDEOC_USER_GET_ZOOM_RECT				_IOWR ('V', BASE_VIDIOC_PRIVATE+12, pinch_info)
#define RCAM_GET_STATUS						_IOR  ('V', BASE_VIDIOC_PRIVATE+30, unsigned long)
#if defined(CONFIG_TCC_REAR_CAMERA_DRV)
#define RCAM_STREAMON						_IO   ('V', BASE_VIDIOC_PRIVATE+31)
#define RCAM_STREAMOFF						_IO   ('V', BASE_VIDIOC_PRIVATE+32)
#define RCAM_LINEBUF_ADDR_WITH_INDEX		_IOWR ('V', BASE_VIDIOC_PRIVATE+33, RCAM_LINE_BUFFER_INFO)
#define RCAM_LINE_UPDATE					_IOW  ('V', BASE_VIDIOC_PRIVATE+34, RCAM_LINE_BUFFER_UPDATE_INFO)
#endif

#define VIDIOC_USER_READ_SENSOR_REGISTER	_IOWR ('V', BASE_VIDIOC_PRIVATE+40, int)
#define VIDIOC_CHECK_CAMERA_MODULE		_IO ('V', BASE_VIDIOC_PRIVATE+41)
#define VIDIOC_CHECK_CAMERA_ERROR      _IOR ('V', BASE_VIDIOC_PRIVATE+42, int)

#define DIRECT_DISPLAY_IF_INITIALIZE		_IOWR ('V', BASE_VIDIOC_PRIVATE+50, int)
#define DIRECT_DISPLAY_IF_START			_IOWR ('V', BASE_VIDIOC_PRIVATE+51, DIRECT_DISPLAY_IF_PARAMETERS)
#define DIRECT_DISPLAY_IF_STOP			_IOWR ('V', BASE_VIDIOC_PRIVATE+52, int)
#define DIRECT_DISPLAY_IF_TERMINATE		_IOWR ('V', BASE_VIDIOC_PRIVATE+53, int)

#endif//_TCC_CAMERA_IOCTL_H_
