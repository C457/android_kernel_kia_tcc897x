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

#ifndef TCC_SENSOR_IF_H
#define TCC_SENSOR_IF_H

#include <linux/videodev2.h>

#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>

#if defined(CONFIG_ARCH_TCC898X)
#include <video/tcc/vioc_ireq.h>
#include <video/tcc/vioc_config.h>
#include <video/tcc/vioc_wdma.h>
#include <video/tcc/vioc_wmix.h>
#include <video/tcc/vioc_scaler.h>
#include <video/tcc/vioc_vin.h>
#include <video/tcc/vioc_viqe.h>
#include <video/tcc/vioc_cam_plugin.h>
#else
#include <mach/vioc_ireq.h>
#include <mach/vioc_config.h>
#include <mach/vioc_wdma.h>
#include <mach/vioc_wmix.h>
#include <mach/vioc_scaler.h>
#include <mach/vioc_vin.h>
#include <mach/vioc_viqe.h>
#include <mach/vioc_cam_plugin.h>
#endif

enum sensor_reg_width {
	WORD_LEN,
	BYTE_LEN,
	MS_DELAY
};

struct sensor_reg {
	unsigned short reg;
	unsigned short val;
	enum sensor_reg_width width;
};

struct capture_size {
	unsigned long width;
	unsigned long height;
};

struct sensor_gpio {
	int	pwr_port;
	int	pwd_port;
	int	rst_port;

	enum of_gpio_flags pwr_value;
	enum of_gpio_flags pwd_value;	
	enum of_gpio_flags rst_value;	
};

struct vcontrol {
	struct v4l2_queryctrl qc;
	int current_value;
};

enum image_size { QQXGA, QXGA, UXGA, SXGA, XGA, SVGA, VGA, QVGA, QCIF };
#define NUM_IMAGE_SIZES 9

enum pixel_format { YUV, RGB565, RGB555 , ARGB8888};
#define NUM_PIXEL_FORMATS 4

#define NUM_OVERLAY_FORMATS 2

enum camera_type {
	CAMERA_TYPE_CAMERA	= 0,
	CAMERA_TYPE_AUX,
	CAMERA_TYPE_MAX
};

enum camera_encode {
	CAMERA_ENCODE_NTSC	= 0,
	CAMERA_ENCODE_PAL,
	CAMERA_ENCODE_MAX
};

#define DEF_BRIGHTNESS      	3
#define DEF_ISO			0
#define DEF_AWB			0
#define DEF_EFFECT		0
#define DEF_ZOOM		0
#define DEF_FLIP		0
#define DEF_SCENE		0
#define DEF_METERING_EXPOSURE	0
#define DEF_EXPOSURE		0
#define DEF_FOCUSMODE		0
#define DEF_FLASH		0

struct tcc_camera_device;

typedef struct
{
	int (*Open)(struct tcc_camera_device * vdev, bool bChangeCamera);
	int (*Close)(struct tcc_camera_device * vdev);
	int (*PowerDown)(struct tcc_camera_device * vdev, bool bChangeCamera);

	int (*Set_Preview)(struct tcc_camera_device * vdev);
	int (*Set_Capture)(struct tcc_camera_device * vdev);
	int (*Set_CameraMode)(int camera_type, int camera_encode, struct tcc_camera_device * vdev);
	int (*Set_CaptureCfg)(int width, int height, struct tcc_camera_device * vdev);

	int (*Set_Zoom)(int val, struct tcc_camera_device * vdev);
	int (*Set_AF)(int val, struct tcc_camera_device * vdev);
	int (*Set_Effect)(int val, struct tcc_camera_device * vdev);
	int (*Set_Flip)(int val, struct tcc_camera_device * vdev);
	int (*Set_ISO)(int val, struct tcc_camera_device * vdev);
	int (*Set_ME)(int val, struct tcc_camera_device * vdev);
	int (*Set_WB)(int val, struct tcc_camera_device * vdev);
	int (*Set_Bright)(int val, struct tcc_camera_device * vdev);
	int (*Set_Scene)(int val, struct tcc_camera_device * vdev);
	int (*Set_Exposure)(int val, struct tcc_camera_device * vdev);
	int (*Set_FocusMode)(int val, struct tcc_camera_device * vdev);
	
	int (*Check_ESD)(int val, struct tcc_camera_device * vdev);
	int (*Check_Luma)(int val, struct tcc_camera_device * vdev);
	
	int (*ReadSensorRegister)(int reg, struct tcc_camera_device * vdev);
	int (*CheckCameraModule)(struct tcc_camera_device * vdev);

	unsigned int (*Enable_signal_error_monitoring)(unsigned int enable, struct tcc_camera_device * vdev);
	unsigned int (*Signal_error_monitoring)(struct tcc_camera_device * vdev);
}
SENSOR_FUNC_TYPE;

typedef struct tcc_sensor_info
{
	int p_clock_pol;
	int v_sync_pol;
	int h_sync_pol;
	int de_pol;
	int field_bfield_low;
	int gen_field_en;
	int conv_en;
	int hsde_connect_en;
	int vs_mask;
	int input_fmt;
	int data_order;
	int intl_en;
	int intpl_en;
	int format;
	int preview_w;
	int preview_h;
	int preview_zoom_offset_x;
	int preview_zoom_offset_y;
	int capture_w;
	int capture_h;
	int capture_zoom_offset_x;
	int capture_zoom_offset_y;
	int max_zoom_step;
	int cam_capchg_width;
	int capture_skip_frame;
	int framerate;
	struct capture_size *sensor_sizes;
}TCC_SENSOR_INFO_TYPE;

#if defined(CONFIG_VIDEO_CAMERA_SENSOR_MT9P111)
#define SENSOR_5M
#define USING_HW_I2C
#include "camera/mt9p111_5mp.h"
#endif
#if defined(CONFIG_VIDEO_CAMERA_SENSOR_MT9T111)
#define SENSOR_3M
#define USING_HW_I2C
#include "camera/mt9t111_3mp.h"
#endif
#if defined(CONFIG_VIDEO_CAMERA_SENSOR_MT9T113)
#define SENSOR_3M
#define USING_HW_I2C
#include "camera/mt9t113_3mp.h"
#endif
#if defined(CONFIG_VIDEO_CAMERA_SENSOR_S5K5CAGA)
#define SENSOR_3M
#define USING_HW_I2C
#include "camera/s5k5caga_3mp.h"
#endif
#if defined(CONFIG_VIDEO_CAMERA_SENSOR_MT9D112)
#define SENSOR_2M
#define USING_HW_I2C
#include "camera/mt9d112_2mp.h"
#endif
#if defined(CONFIG_VIDEO_CAMERA_SENSOR_OV7690)
#define SENSOR_VGA
#define USING_HW_I2C
#include "camera/ov7690_vga.h"
#endif
#if defined(CONFIG_VIDEO_CAMERA_SENSOR_SIV100B)
#define SENSOR_VGA
#define USING_HW_I2C
#include "camera/siv100b_vga.h"
#endif
#if defined(CONFIG_VIDEO_CAMERA_SENSOR_MT9M113)
#define SENSOR_1_3M 
#define USING_HW_I2C
#include "camera/mt9m113_1.3mp.h"
#endif
#if defined(CONFIG_VIDEO_CAMERA_SENSOR_SR130PC10)
#define SENSOR_1_3M 
#define USING_HW_I2C
#include "camera/sr130pc10_1.3mp.h"
#endif
#if defined(CONFIG_VIDEO_ATV_SENSOR_TVP5150)
#define USING_HW_I2C
#define SENSOR_BT656
#include "atv/tvp5150.h"
#endif

#if 0//defined(CONFIG_VIDEO_ATV_SENSOR_ADV7182)
#define USING_HW_I2C
#include "atv/adv7182.h"
#endif

#ifdef CONFIG_DAUDIO //mhjung
#define USING_HW_I2C
    #if defined(INCLUDE_BOARD3HW_GPIO)
    #include "atv/tw9990.h"
    #include "atv/daudio_lvds.h"
    #else
    #include "atv/tw9921.h"
    #endif

#endif
#if defined(CONFIG_VIDEO_ATV_SENSOR_DAUDIO)
#define USING_HW_I2C
#define SENSOR_DAUDIO
#include "atv/daudio_atv.h"
#endif

extern int sensor_if_change_mode(struct tcc_camera_device * vdev, unsigned char capture_mode);
extern int sensor_if_change_mode_ex(int camera_type, int camera_encode, struct tcc_camera_device * vdev);
extern int sensor_if_adjust_autofocus(struct tcc_camera_device * vdev);
extern int sensor_if_query_control(struct v4l2_queryctrl *qc);
extern int sensor_if_get_control(struct v4l2_control *vc);
extern int sensor_if_set_control(struct v4l2_control *vc, unsigned char init, struct tcc_camera_device * vdev);
extern void sensor_if_set_current_control(struct tcc_camera_device * vdev);
extern int sensor_if_enum_pixformat(struct v4l2_fmtdesc *fmt);
extern int sensor_if_parse_gpio_dt_data(struct tcc_camera_device * vdev);
extern int sensor_if_init(struct tcc_camera_device * vdev);
extern void sensor_pwr_control(int name, int enable);
extern int sensor_if_get_max_resolution(struct tcc_camera_device * vdev);
extern int sensor_if_get_sensor_framerate(struct tcc_camera_device * vdev, int *nFrameRate);
extern void sensor_if_set(struct tcc_camera_device * vdev,int index);
extern void sensor_if_api_connect(struct tcc_camera_device * vdev, int index);
extern int sensor_isit_change_camera(void);
extern int sensor_if_open(struct tcc_camera_device * vdev);
extern int sensor_if_capture_config(struct tcc_camera_device * vdev, int width, int height);
extern int sensor_if_isESD(struct tcc_camera_device * vdev);
extern int sensor_if_cleanup(struct tcc_camera_device * vdev);
extern void sensor_delay(int ms);
extern void sensor_cifmc_enable(struct tcc_camera_device * vdev);
extern void sensor_cifmc_disable(struct tcc_camera_device * vdev);
extern void sensor_port_enable(int port);
extern void sensor_port_disable(int port);
extern int sensor_get_port(int port);

extern int sensor_if_read_i2c(int reg, struct tcc_camera_device * vdev);
extern int sensor_if_check_camera_module(struct tcc_camera_device * vdev);
extern int sensor_if_check_camera_signal_error(struct tcc_camera_device * vdev);
#endif
