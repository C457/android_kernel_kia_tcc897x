#include <asm/io.h>
//#include <asm/system.h>
 
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <mach/daudio.h>
#include <mach/daudio_debug.h>
#include <mach/daudio_info.h>
#include <mach/daudio_pinctl.h>
#include <mach/daudio_settings.h>
#include <mach/hardware.h>

#include "../sensor_if.h"
//#include "../cam.h"
#include "../tcc_cam_i2c.h"
#include "../tcc_cam.h"

#if defined(INCLUDE_BOARD3HW_GPIO)
#include "tw9990.h"
#include "daudio_lvds.h"
#include "daudio_atv.h"
#else
#include "tw9921.h"
#endif

#include "tcc_camera_device.h"

#include "daudio_cmmb.h"

#include <mach/gpio.h>

static int debug = 0;
#define VPRINTK(fmt, args...) if(debug){ printk(KERN_INFO TAG_DAUDIO_ATV fmt, ##args); }

static SENSOR_FUNC_TYPE_DAUDIO sf_daudio[SENSOR_MAX];
static int yin_sel = -1;
static ie_setting_info t_ie_info;

extern struct TCCxxxCIF hardware_data;
extern int get_camera_type(void);

struct capture_size sensor_sizes[] = {
	{720, 480}, // NTSC
	{720, 576}  // PAL
};

static int sensor_readRegister(int reg, struct tcc_camera_device * vdev)
{
	char ret;
	DDI_I2C_Read((unsigned short)reg, 1, &ret, 1,vdev);

	return ret;
}

static int sensor_writeRegister(int reg, int val, struct tcc_camera_device * vdev)
{
	int ret;
	unsigned char data[2];

	data[0]= (u8)reg & 0xff;
	data[1]= (u8)val & 0xff;
	ret = DDI_I2C_Write(data, 1, 1,vdev);

	return ret;
}

static int sensor_open(struct tcc_camera_device * vdev, bool bChangeCamera)
{
	int what = 0;
	int cam_info = vdev->data.cam_info;
	eTW_YSEL yin=TW_YIN1;

	VPRINTK("%s\n", __func__);

	if( cam_info == DAUDIO_CAMERA_REAR )
	{
		what = SENSOR_TW9990;

		if( get_camera_type() == 0 )
			yin = TW_YIN1;	//CVBS
		else
			yin = TW_YIN2;	//SVIDEO
	}

	else if(cam_info == DAUDIO_CAMERA_LVDS || cam_info == DAUDIO_ADAS_PRK || cam_info == DAUDIO_DVRS_RVM)
	{
		what = SENSOR_LVDS;
	}
	else if( cam_info == DAUDIO_CAMERA_CMMB )
	{
		what = SENSOR_CMMB;
	}
	else if( cam_info == DAUDIO_CAMERA_AUX )
	{
		what=SENSOR_TW9990;
		yin=TW_YIN0;
	}

	if(what == SENSOR_TW9990)
		yin_sel = yin;

	if(sf_daudio[what].sensor_open == NULL)
		return FAIL;

	if(sf_daudio[what].sensor_open(yin,vdev)) {
		printk("sensor_open fail\n");
	}

	return 0;
}

static int sensor_close(struct tcc_camera_device * vdev)
{
	int what = 0;
	int cam_info = vdev->data.cam_info;

	VPRINTK("%s\n", __func__);

	if (cam_info == DAUDIO_CAMERA_LVDS || cam_info == DAUDIO_ADAS_PRK || cam_info == DAUDIO_DVRS_RVM) {
		what = SENSOR_LVDS;
    }
	else if (cam_info == DAUDIO_CAMERA_CMMB)
		what = SENSOR_CMMB;
	else
		what = SENSOR_TW9990;

	yin_sel = -1;

	if (sf_daudio[what].sensor_close != NULL){
		update_settings();	//sys_sync call - settings partition update
		return sf_daudio[what].sensor_close(vdev);
	}
	else
		return FAIL;
}
static int sensor_change_mode(int type, int encode,struct tcc_camera_device * vdev) 
{
	int what = 0;
	int cam_info = vdev->data.cam_info;

	VPRINTK("%s\n", __func__);
	
	if(CAMERA_TYPE_MAX <= type || CAMERA_ENCODE_MAX <= encode) {
		printk("%s() - type or encode is wrong.\n", __func__);
		return -1;
	}
	
	if (cam_info == DAUDIO_CAMERA_LVDS || cam_info == DAUDIO_ADAS_PRK || cam_info == DAUDIO_DVRS_RVM)
		what = SENSOR_LVDS;
	else if (cam_info == DAUDIO_CAMERA_CMMB)
		what = SENSOR_CMMB;
	else
		what = SENSOR_TW9990;

	if (sf_daudio[what].Set_CameraMode != NULL)
		return sf_daudio[what].Set_CameraMode(type,encode,vdev);
	else
		return FAIL;
}

static int sensor_capture(struct tcc_camera_device * vdev)
{
	int what = 0;
	int cam_info = vdev->data.cam_info;
	
	VPRINTK("%s\n", __func__);

	if (cam_info == DAUDIO_CAMERA_LVDS || cam_info == DAUDIO_ADAS_PRK || cam_info == DAUDIO_DVRS_RVM)
		what = SENSOR_LVDS;
	else if (cam_info == DAUDIO_CAMERA_CMMB)
		what = SENSOR_CMMB;
	else
		what = SENSOR_TW9990;

	if (sf_daudio[what].sensor_capture != NULL)
		return sf_daudio[what].sensor_capture(vdev);
	else
		return FAIL;
}

static int sensor_capturecfg(int width, int height,struct tcc_camera_device * vdev)
{
	int what = 0;
	int cam_info = vdev->data.cam_info;
	
	VPRINTK("%s\n", __func__);

	if (cam_info == DAUDIO_CAMERA_LVDS || cam_info == DAUDIO_ADAS_PRK || cam_info == DAUDIO_DVRS_RVM)
		what = SENSOR_LVDS;
	else if (cam_info== DAUDIO_CAMERA_CMMB)
		what = SENSOR_CMMB;
	else
		what = SENSOR_TW9990;

	if (sf_daudio[what].sensor_capturecfg != NULL)
		return sf_daudio[what].sensor_capturecfg(width, height,vdev);
	else
		return FAIL;
}

static int sensor_zoom(int val,struct tcc_camera_device * vdev)
{
	return 0;
}

static int sensor_autofocus(int val,struct tcc_camera_device * vdev)
{
	return 0;
}

static int sensor_effect(int val)
{
	return 0;
}

static int sensor_flip(int val)
{
	return 0;
}

static int sensor_iso(int val)
{
	return 0;
}

static int sensor_me(int val)
{
	return 0;
}

static int sensor_wb(int val)
{
	return 0;
}

static int sensor_bright(int val,struct tcc_camera_device * vdev)
{
	return 0;
}

static int sensor_scene(int val,struct tcc_camera_device * vdev)
{
	return 0;
}

static int sensor_contrast(int val,struct tcc_camera_device * vdev)
{
	return 0;
}

static int sensor_saturation(int val,struct tcc_camera_device * vdev)
{
	return 0;
}

static int sensor_hue(int val,struct tcc_camera_device * vdev)
{
	return 0;
}

static int sensor_getVideo(struct tcc_camera_device * vdev)
{
	return 0;
}

static int sensor_checkNoise(struct tcc_camera_device * vdev)
{
	return 0;
}

static int sensor_resetVideoDecoder(struct tcc_camera_device * vdev)
{
	int what = 0;
	int cam_info = vdev->data.cam_info;

	VPRINTK("%s\n", __func__);

	if (cam_info == DAUDIO_CAMERA_LVDS || cam_info == DAUDIO_ADAS_PRK || cam_info == DAUDIO_DVRS_RVM)
		what = SENSOR_LVDS;
	else if (cam_info== DAUDIO_CAMERA_CMMB)
		what = SENSOR_CMMB;
	else
		what = SENSOR_TW9990;

	if (sf_daudio[what].sensor_reset_video_decoder != NULL)
		return sf_daudio[what].sensor_reset_video_decoder(vdev);
	else
		return FAIL;
}

static int sensor_setPath(int val, struct tcc_camera_device * vdev)
{
	int (*pfn)(int);
	int cam_info = vdev->data.cam_info;

	if (cam_info == DAUDIO_CAMERA_LVDS || cam_info == DAUDIO_ADAS_PRK || cam_info == DAUDIO_DVRS_RVM)
		pfn=sf_daudio[SENSOR_LVDS].Set_Path;
	else if( cam_info == DAUDIO_CAMERA_CMMB )
		pfn=sf_daudio[SENSOR_CMMB].Set_Path;
	else
		pfn=sf_daudio[SENSOR_TW9990].Set_Path;

	if( pfn )
		return pfn(val);

	return FAIL;
}

static int sensor_getVideoFormat(void)
{
	return 0;
}

static int sensor_check_esd(int val)
{
	return 0;
}

static int sensor_check_luma(int val)
{
	return 0;
}

static int sensor_preview(struct tcc_camera_device * vdev)
{
	int what = 0;
	int cam_info = vdev->data.cam_info;

	VPRINTK("%s\n", __func__);

	if (cam_info == DAUDIO_CAMERA_LVDS || cam_info == DAUDIO_ADAS_PRK || cam_info == DAUDIO_DVRS_RVM)
		what = SENSOR_LVDS;
	else if (cam_info == DAUDIO_CAMERA_CMMB)
		what = SENSOR_CMMB;
	else
		what = SENSOR_TW9990;

	if (sf_daudio[what].sensor_preview != NULL)
		return sf_daudio[what].sensor_preview(vdev);
	else
		return FAIL;
}

// sensor ENABLE camera DATA error monitoring (2020.10.23)
static int sensor_enable_signal_error_monitoring(unsigned int enable, struct tcc_camera_device * vdev)
{
	int what = 0;
	int cam_info = vdev->data.cam_info;

	VPRINTK("%s\n", __func__);

	if (cam_info == DAUDIO_CAMERA_LVDS || cam_info == DAUDIO_ADAS_PRK || cam_info == DAUDIO_DVRS_RVM)
		what = SENSOR_LVDS;
	else if (cam_info == DAUDIO_CAMERA_CMMB)
		what = SENSOR_CMMB;
	else
		what = SENSOR_TW9990;

	if (sf_daudio[what].Enable_signal_error_monitoring != NULL)
		return sf_daudio[what].Enable_signal_error_monitoring(enable,vdev);
	else
		return FAIL;
}

// sensor camera DATA error monitoring (2020.10.23)
static int sensor_signal_error_monitoring(struct tcc_camera_device * vdev)
{
	int what = 0;
	int cam_info = vdev->data.cam_info;

	VPRINTK("%s\n", __func__);

	if (cam_info == DAUDIO_CAMERA_LVDS || cam_info == DAUDIO_ADAS_PRK || cam_info == DAUDIO_DVRS_RVM)
		what = SENSOR_LVDS;
	else if (cam_info == DAUDIO_CAMERA_CMMB)
		what = SENSOR_CMMB;
	else
		what = SENSOR_TW9990;

	if (sf_daudio[what].Signal_error_monitoring != NULL)
		return sf_daudio[what].Signal_error_monitoring(vdev);
	else
		return FAIL;
}

int datv_write_ie(int cmd, unsigned char value, struct tcc_camera_device * vdev)
{
	int ret = 0;
	static int what = 0;
	VPRINTK("%s\n", __func__);

#if defined(CONFIG_VIDEO_DUAL_CAMERA_SUPPORT)
	if(vdev == NULL)
		return 0;
#endif
	if (cmd)
	{
		if (sf_daudio[what].sensor_write_ie != NULL)
		{
			sf_daudio[what].sensor_write_ie(cmd, value, vdev);

			switch(cmd)
			{
				case SET_CAM_BRIGHTNESS:
					if(t_ie_info.twxxxx_cam_brightness != value)
						t_ie_info.twxxxx_cam_brightness = value;
					else
						ret = FAIL;
					break;
				case SET_CAM_CONTRAST:
					if(t_ie_info.twxxxx_cam_contrast != value)
						t_ie_info.twxxxx_cam_contrast = value;
					else
						ret = FAIL;
					break;
				case SET_CAM_SATURATION:
					if(t_ie_info.twxxxx_cam_saturation != value)
						t_ie_info.twxxxx_cam_saturation = value;
					else
						ret = FAIL;
					break;
				default:
						ret = FAIL;
					break;
			}

			if(ret == FAIL)
				return 0;	//same value, do not write to eMMC

			if(!write_ie_setting(&t_ie_info)){
				printk(KERN_ERR "[GT system] %s , write_ie_setting FAIL~~!!\n", __func__ );
				return -EINVAL;
			}
		}
		else
			return FAIL;
	}
	else
		what = value;

	return 0;
}

int datv_read_ie(int cmd, unsigned char *level, struct tcc_camera_device * vdev)
{
	int camera_type	= 0;
	static int what = 0;
	static int initialized = 0;
	VPRINTK("%s\n", __func__);

#if defined(CONFIG_VIDEO_DUAL_CAMERA_SUPPORT)
	if(vdev == NULL)
		return 0;
#endif
	if (cmd)
	{
		if(!initialized)
		{
			//(hklee)2018.10.04 - IE set with default value
			printk("%s - initial setting from emmc\n",__func__);
			if(read_ie_setting(&t_ie_info))
			{
				if(!t_ie_info.twxxxx_cam_brightness)
					t_ie_info.twxxxx_cam_brightness = 0;
				if(!t_ie_info.twxxxx_cam_contrast)
					t_ie_info.twxxxx_cam_contrast = 0;
				if(!t_ie_info.twxxxx_cam_saturation)
					t_ie_info.twxxxx_cam_saturation = 0;
				initialized = 1;
			}
		}

		switch(cmd)
		{
			case GET_CAM_BRIGHTNESS:
				*level = (unsigned char)t_ie_info.twxxxx_cam_brightness;
				break;
			case GET_CAM_CONTRAST:
				*level = (unsigned char)t_ie_info.twxxxx_cam_contrast;
				break;
			case GET_CAM_SATURATION:
				*level = (unsigned char)t_ie_info.twxxxx_cam_saturation;
				break;
			default:
				*level = 0;
				break;
		}
			//return sf_daudio[what].sensor_read_ie(cmd, level, vdev);
	}
	else
		what = *level;

	return 0;
}

int datv_init(struct tcc_camera_device * vdev)
{
	int what = 0;
	int cam_info = vdev->data.cam_info;

	VPRINTK("%s\n", __func__);

#if defined(INCLUDE_BOARD3HW_GPIO)
	if (vdev->data.cam_info == DAUDIO_CAMERA_LVDS || vdev->data.cam_info == DAUDIO_ADAS_PRK || vdev->data.cam_info == DAUDIO_DVRS_RVM)
	{
		lvds_sensor_init_fnc(&sf_daudio[SENSOR_LVDS]);
		what = SENSOR_LVDS;
	}
	else
	{	
		tw9990_sensor_init_fnc(&sf_daudio[SENSOR_TW9990]);
		what = SENSOR_TW9990;
	}

	datv_write_ie(0, what, NULL);
	datv_read_ie(0, &what, NULL);	
#else
	tw9921_sensor_init_fnc(&sf_daudio[SENSOR_TW9990]);
#endif
/*
	daudio_cmmb_sensor_init_fnc(&sf_daudio[SENSOR_CMMB]);

	if (hardware_data.cam_info == DAUDIO_CAMERA_CMMB)
		what = SENSOR_CMMB;
	else
		what = SENSOR_TW9990;
*/

#if defined(CONFIG_LVDS_CAMERA)||defined(INCLUDE_LVDS_CAMERA)
	setup_timer( &testtimer, lvds_timer_fun, 0 );
	mod_timer( &testtimer, jiffies + msecs_to_jiffies(1000) );
#endif
	if (sf_daudio[what].sensor_init != NULL)
		return sf_daudio[what].sensor_init();
	else
		return FAIL;
}

static int sensor_checkCameraModule(struct tcc_camera_device * vdev)
{
	int what = 0;
	int cam_info = vdev->data.cam_info;
	int ret = FAIL;

	if (cam_info == DAUDIO_CAMERA_LVDS || cam_info == DAUDIO_ADAS_PRK || cam_info == DAUDIO_DVRS_RVM)
		what = SENSOR_LVDS;
	else if (cam_info == DAUDIO_CAMERA_CMMB)
		what = SENSOR_CMMB;
	else
		what = SENSOR_TW9990;

	if (sf_daudio[what].sensor_check_video_signal != NULL)
	{
		if ((vdev->data.cam_info == DAUDIO_CAMERA_AUX) && (yin_sel != TW_YIN0))
			ret = 0;
		else if ((vdev->data.cam_info == DAUDIO_CAMERA_REAR) && (yin_sel == TW_YIN0))
			ret = 0;
		else
			ret = sf_daudio[what].sensor_check_video_signal(vdev);
	}
	return ret;
}

int datv_display_mute(int mute,struct tcc_camera_device * vdev)
{
	int what = 0;
	int cam_info = vdev->data.cam_info;

	if (cam_info == DAUDIO_CAMERA_LVDS || cam_info == DAUDIO_ADAS_PRK || cam_info == DAUDIO_DVRS_RVM)
		what = SENSOR_LVDS;
	else if (cam_info == DAUDIO_CAMERA_CMMB)
		what = SENSOR_CMMB;
	else
		what = SENSOR_TW9990;

	if (sf_daudio[what].sensor_display_mute != NULL)
		return sf_daudio[what].sensor_display_mute(mute);
	else
		return FAIL;
}

void datv_close_cam(struct tcc_camera_device * vdev)
{
	int what = 0;
	int cam_info = vdev->data.cam_info;

	if (cam_info == DAUDIO_CAMERA_LVDS || cam_info == DAUDIO_ADAS_PRK || cam_info == DAUDIO_DVRS_RVM)
		what = SENSOR_LVDS;
	else if (cam_info == DAUDIO_CAMERA_CMMB)
		what = SENSOR_CMMB;
	else
		what = SENSOR_TW9990;

	if (sf_daudio[what].sensor_close_cam != NULL)
		sf_daudio[what].sensor_close_cam(vdev);
}

int datv_get_reset_pin(void)
{
	//if (hardware_data.cam_info == DAUDIO_CAMERA_CMMB)
		//return get_gpio_number(CTL_XM_SIRIUS_RESET);	// CMMB reset
	//else
		//return get_gpio_number(CTL_TW9912_RESET);

	//return FAIL;
}

// @ 2015-07-03: JJongspi
// To select sensor size as analog encode (NTSC or PAL).
//void datv_get_preview_size(int *width, int *height)
void datv_get_preview_size(int *width, int *height, int encode,struct tcc_camera_device * vdev)
{
	int what = 0;
	int cam_info = vdev->data.cam_info;

	if (cam_info == DAUDIO_CAMERA_LVDS || cam_info == DAUDIO_ADAS_PRK || cam_info == DAUDIO_DVRS_RVM)
		what = SENSOR_LVDS;
	else if (cam_info == DAUDIO_CAMERA_CMMB)
		what = SENSOR_CMMB;
	else
		what = SENSOR_TW9990;

	if (sf_daudio[what].sensor_get_preview_size != NULL)
		sf_daudio[what].sensor_get_preview_size(width, height);

	// @ 2015-07-03: JJongspi
	if (cam_info == DAUDIO_CAMERA_AUX)
	{
		*width	= sensor_sizes[encode].width;
		*height	= sensor_sizes[encode].height / 2;
	}
}

void datv_get_capture_size(int *width, int *height,struct tcc_camera_device * vdev)
{
	int what = 0;
	int cam_info = vdev->data.cam_info;

	if (cam_info == DAUDIO_CAMERA_LVDS || cam_info == DAUDIO_ADAS_PRK || cam_info == DAUDIO_DVRS_RVM)
		what = SENSOR_LVDS;
	else if (cam_info == DAUDIO_CAMERA_CMMB)
		what = SENSOR_CMMB;
	else
		what = SENSOR_TW9990;

	if (sf_daudio[what].sensor_get_capture_size != NULL)
		sf_daudio[what].sensor_get_capture_size(width, height);
}

void sensor_init_fnc(SENSOR_FUNC_TYPE *sensor_func)
{
	sensor_func->Open					= sensor_open;
	sensor_func->Close					= sensor_close;

	sensor_func->Set_Preview			= sensor_preview;
	sensor_func->Set_Capture			= sensor_capture;
	sensor_func->Set_CameraMode			= sensor_change_mode;
	sensor_func->Set_CaptureCfg			= sensor_capturecfg;

	sensor_func->Set_Zoom				= sensor_zoom;
	sensor_func->Set_AF					= sensor_autofocus;
	sensor_func->Set_Effect				= sensor_effect;
	sensor_func->Set_Flip				= sensor_flip;
	sensor_func->Set_ISO				= sensor_iso;
	sensor_func->Set_ME					= sensor_me;
	sensor_func->Set_WB					= sensor_wb;
	sensor_func->Set_Bright				= sensor_bright;
	sensor_func->Set_Scene				= sensor_scene;
#if 0	
	sensor_func->Set_Contrast			= sensor_contrast;
	sensor_func->Set_Saturation			= sensor_saturation;
	sensor_func->Set_Hue				= sensor_hue;
	sensor_func->Get_Video				= sensor_getVideo;
	sensor_func->Check_Noise			= sensor_checkNoise;
	sensor_func->Reset_Video_Decoder	       = sensor_resetVideoDecoder;
	sensor_func->Set_Path				= sensor_setPath;
	sensor_func->Get_videoFormat		= sensor_getVideoFormat;
	sensor_func->Set_writeRegister		= sensor_writeRegister;
#endif
	sensor_func->ReadSensorRegister		= sensor_readRegister;
	sensor_func->CheckCameraModule		= sensor_checkCameraModule;

	sensor_func->Check_ESD				= NULL;
	sensor_func->Check_Luma				= NULL;

	sensor_func->Enable_signal_error_monitoring		= sensor_enable_signal_error_monitoring;
	sensor_func->Signal_error_monitoring		= sensor_signal_error_monitoring;
}

#if defined(CONFIG_LVDS_CAMERA)||defined(INCLUDE_LVDS_CAMERA)
struct timer_list testtimer;

void lvds_timer_fun(struct tcc_camera_device * vdev)
{
	unsigned int gpioB22=0, gpioB19=0,gpioB23=0;
	unsigned int value=0;

	gpioB22 =  gpio_get_value(TCC_GPB(22));
	VPRINTK("%s\n", __func__);
	if(1)
	{
		value = sf_daudio[SENSOR_LVDS].sensor_check_video_signal(vdev);
	}
	del_timer( &testtimer );
}
#endif

