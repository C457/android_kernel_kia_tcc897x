/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

               CAMERA    API    M O D U L E

                        EDIT HISTORY FOR MODULE

when        who       what, where, why
--------    ---       -------------------------------------------------------
10/xx/08  Telechips   Created file.
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

/*===========================================================================

                           INCLUDE FILES FOR MODULE

===========================================================================*/

#include <linux/delay.h>
#include <mach/daudio.h>
#include <mach/daudio_debug.h>
#include <linux/gpio.h>

#include "../tcc_cam_i2c.h"

#include <mach/gpio.h>
#include <mach/vioc_vin.h>

#include "tcc_camera_device.h"

#include "../cam_reg.h"

#include "daudio_atv.h"
#include "daudio_lvds.h"

#include "../tcc_cam.h"

#define PRV_W 1920 
#define PRV_H 720
#define CAP_W 1920
#define CAP_H 720

#define MULTI_NUM 1000
#define SATURATION_RANGE   	4 // -X ~ +X
#define CONTRAST_RANGE   	4 // -X ~ +X

int rgb[257] = {0, };
//(hklee)2018.10.04 - IE set with default value
static int _cam_brightness = DEFAULT_LVDS_SVM_BRIGHTNESS;
static int _cam_contrast = DEFAULT_LVDS_SVM_CONTRAST;
static int _cam_gamma = -1;
static int _cam_saturation = DEFAULT_LVDS_SVM_SATURATION;
static int _dvrs_brightness = 127;
static int _dvrs_contrast = 127;
static int _dvrs_gamma = -1;
static int _dvrs_saturation = 127;

static DEFINE_MUTEX(sensor_lock);

static int debug_lvds	= DEBUG_DAUDIO_LVDS;

#define VPRINTK(fmt, args...) if (1) printk(KERN_INFO TAG_DAUDIO_LVDS fmt, ##args)

static int sensor_writeRegister(int reg, int val, struct tcc_camera_device * vdev);
static int sensor_readRegister(int reg, struct tcc_camera_device * vdev);
int lvds_write_ie(int cmd, unsigned char value, struct tcc_camera_device * vdev);

static struct capture_size sensor_size[] = {
	{1920, 720}
};

int get_gpioG14(void)
{
	int gpioG14 = -1;
	gpioG14 = gpio_get_value(TCC_GPG(14));
	return 1;// gpioD17;
}

static void lvds_sensor_get_preview_size(int *width, int *height)
{
	*width = PRV_W;
	*height = PRV_H;
}

static void lvds_sensor_get_capture_size(int *width, int *height)
{
	*width = CAP_W;
	*height = CAP_H;
}

static int write_regs(const struct sensor_reg reglist[])
{
	VPRINTK("%s-\n", __func__);
		return 0;
}

static int sensor_writeRegister(int reg, int val, struct tcc_camera_device * vdev)
{
	int ret;
	unsigned char data[2];
	VPRINTK("%s-\n", __func__);
	
	data[0]= (u8)reg & 0xff;
	data[1]= (u8)val & 0xff;
	ret = DDI_I2C_Write(data, 1, 1,vdev);

	return ret;
}

static int sensor_readRegister(int reg, struct tcc_camera_device * vdev)
{
	char ret;
	DDI_I2C_Read((unsigned short)reg, 1, &ret, 1,vdev);
	return ret;
}

static unsigned int lvds_read(unsigned short reg, unsigned char *data, struct tcc_camera_device * vdev)
{
	int i = I2C_RETRY_COUNT;
	int loop  ;
	int retval; 

	//getChipAddress();

	do {
		retval = DDI_I2C_Read(reg , 1, data, 1,vdev );
		if (retval == 0 )
		{
			//success to I2C.
			return 0;
		}
		i--;
	} while(i > 0);

	return FAIL_I2C;
}

static int lvds_write(unsigned char* data, unsigned char reg_bytes, unsigned char data_bytes, struct tcc_camera_device * vdev)
{
	int i = I2C_RETRY_COUNT;
	do {
		int ret = DDI_I2C_Write(data, reg_bytes, data_bytes,vdev);
		if (ret == 0)
		{
			//success to I2C.
			return SUCCESS_I2C;
		}
		i--;
	} while(i > 0);

	return FAIL_I2C;
}

// mhlee - This is to check Camera Data error (DET_ERR & DET_THR)(2020.10.23)
#define REG_det_err	0x15	// Detected error count
#define REG_det_thr 0x0E	// Detected errors Threshold
#define NUM_ERR_THR		255	// 0 ~ 255


static unsigned int sensor_enable_signal_error_monitoring(unsigned int enable, struct tcc_camera_device * vdev)
{
	unsigned int ret = false;

	//	VPRINTK("%s++\n", __func__);

	//	VPRINTK("%s--\n", __func__);

	return ret;
}

//sensor camera DATA error monitoring
static unsigned int sensor_signal_error_monitoring(struct tcc_camera_device * vdev)
{
	unsigned int error_det = false;
	unsigned char data = 0;
	unsigned int ret = 0;

	//	VPRINTK("%s++\n", __func__);
	mutex_lock(&sensor_lock);
	ret = lvds_read(REG_det_err, &data, vdev);
	if(ret == FAIL_I2C) {
		vdev->cam_err.des_ic_err = 1;
		//printk("%s: I2C error occur\n", __func__);
	}
	else {
		vdev->cam_err.des_ic_err = 0;

		if(data >= NUM_ERR_THR)
		{
			//printk("cam_info: SERDES signal error\n");
			vdev->cam_err.des_signal_err = 1;
		}
		else if(vdev->cam_err.des_signal_err) {
			//printk("cam_info: SERDES signal normal\n");
			vdev->cam_err.des_signal_err = 0;
		}
	}

	//	VPRINTK("%s--\n", __func__);
	mutex_unlock(&sensor_lock);

	return ret;
}

static int lvds_sensor_open(eTW_YSEL yin, struct tcc_camera_device * vdev)
{

	unsigned char id =0 ; 
	unsigned int status =0;
	unsigned char val[2];
	unsigned int ret = 0;

	mutex_lock(&sensor_lock);
	ret	= lvds_read(LVDS_REG_ID, &id, vdev);
	VPRINTK("%s, LVDS_REG_ID = 0x%x(%u)\n", __func__, id, ret);

	if(FAIL_I2C == ret) {
		//printk("%s: I2C error occur\n", __func__);	
	}

	if( id == LVDS_VAL_ID )
	{
		vdev->cam_err.des_ic_err = 0;
		val[0]	= 0x04; //main_config
		lvds_read(val[0], &val[1], vdev);
		VPRINTK("%s, OLD LVDS_REG_DESLOCK = 0x%x \n", __func__, val[1]);

		val[1]	|= (unsigned char)0x03;
		lvds_write(val, 1, 1, vdev);
				
		lvds_read(val[0], &val[1], vdev);
		VPRINTK("%s, NEW LVDS_REG_DESLOCK = 0x%x \n", __func__, val[1]);

		lvds_write_ie(0,0,0);
	}
	else {
		vdev->cam_err.des_ic_err = 1;
	}
	mutex_unlock(&sensor_lock);

	return ret;

}

static int lvds_sensor_close(void)
{
	VPRINTK("%s-\n", __func__);
	return 0;
}

int lvds_read_id(struct tcc_camera_device * vdev)
{
 	unsigned int id =0 ; 
	unsigned int ret =0;

	mutex_lock(&sensor_lock);
	//VPRINTK("%s-----------------------------------------------\n", __func__);
	ret	= lvds_read(LVDS_REG_ID, &id, vdev);
	VPRINTK("%s, LVDS_REG_ID = %d\n", __func__, id	);

	if(ret == FAIL_I2C) {
		//printk("%s: I2C error occur\n", __func__);
	}

	mutex_unlock(&sensor_lock);

	return id;
}

static int lvds_sensor_preview(void)
{
	VPRINTK("%s-\n", __func__);
 	return 0;
}

static int lvds_sensor_resetVideoDecoder(struct tcc_camera_device * vdev)
{
	return 0;
}

void lvds_close_cam(struct tcc_camera_device * vdev)
{
	VPRINTK("%s-\n", __func__);

	unsigned int id =0 ; 
	unsigned int status =0;
	unsigned char val[2];
	unsigned int ret = 0;

	mutex_lock(&sensor_lock);
	lvds_read(LVDS_REG_ID, &id, vdev);
	VPRINTK("%s, LVDS_REG_ID = 0x%x\n", __func__, id);

	if( id == LVDS_VAL_ID )
	{
		val[0]	= 0x04; //main_config
		lvds_read(val[0], &val[1], vdev);
		VPRINTK("%s, OLD LVDS_REG_DESLOCK = 0x%x \n", __func__, val[1]);

		val[1]	&= (unsigned char)(~0x03);
		lvds_write(val, 1, 1, vdev);
				
		lvds_read(val[0], &val[1], vdev);
		VPRINTK("%s, NEW LVDS_REG_DESLOCK = 0x%x \n", __func__, val[1]);
	}

	mutex_unlock(&sensor_lock);
	return;
}

int lvds_init(void)
{
	VPRINTK("%s\n", __func__);

	return 0;
}

int lvds_check_video_signal(struct tcc_camera_device * vdev)
{
	int status = SERDES_LOCK_NORMAL;
	unsigned int i2c_ret = 0;
	unsigned int id = 0;

	mutex_lock(&sensor_lock);

	i2c_ret = lvds_read(LVDS_REG_ID, &id, vdev);
	if(id != LVDS_VAL_ID) {
		vdev->cam_err.des_ic_err = 1;

		status = SERDES_I2C_FAIL;
		VPRINTK("check lock = DES IC error\n");
	}
	else {
		vdev->cam_err.des_ic_err = 0;

		status  = gpio_get_value(TCC_GPB(17));
		if(status == SERDES_LOCK_FAIL) {
			vdev->cam_err.des_lock_fail = 1;
			VPRINTK("check lock = lock error\n");
		}
		else {
			vdev->cam_err.des_lock_fail = 0;
		}
	}

	mutex_unlock(&sensor_lock);
	
	return status;
}

int lvds_display_mute(int mute)
{
	printk(KERN_INFO "%s lvds not support display mute.\n", __func__);

	return FAIL;
}

static int setLUT(int brightness, int contrast, int saturation)
{
	int vR, vG, vB, i;
	int fsaturation, fcontrast;
	VIOC_VIN *pVIN =(VIOC_VIN*)0xf3004000;

	for (i = 0; i < 256; i++)
	{
		//contrast brightness
		if(contrast > 127)
			fcontrast = (1 * MULTI_NUM) + ((contrast -127) * MULTI_NUM * (CONTRAST_RANGE - 1)) / 127;
		else
			fcontrast = (contrast * MULTI_NUM) / 127;

		vR = ((i - 16)*fcontrast)/MULTI_NUM + 16;

		//brightness
		if(vR > 255)    vR = 255;
		if(vR < 0)      vR = 0;
		if(brightness >= 127)
			vR = vR + ((brightness - 127) * 2);
		else
			vR = vR + ((brightness - 127) * 2) - 1;

		//saturation
		if(saturation > 127)
			fsaturation = (1 * MULTI_NUM) + ((saturation - 127) * MULTI_NUM * (SATURATION_RANGE - 1)) / 127;
		else
			fsaturation = (saturation * MULTI_NUM) / 127;

		//fsaturation = (1* MULTI_NUM)+ ((((saturation - 127) * MULTI_NUM) / 127) * SATURATION_RANGE);

		vG = vB = (((i - 128)*fsaturation * fcontrast)/MULTI_NUM)/MULTI_NUM + 128;

		if(vR > 255)    vR = 255;
		if(vR < 0)      vR = 0;
		if(vG > 255)    vG = 255;
		if(vG < 0)      vG = 0;
		if(vB > 255)    vB = 255;
		if(vB < 0)      vB = 0;

		rgb[i] =((vR & 0xff) << 16) | ((vG & 0xff) << 8) | ((vB & 0xff));

		//VPRINTK("%s count: %d rgb:0x%2x\n", __func__, i, rgb[i]);
	}
	VPRINTK("%s brightness:%d  contrast:%d  saturation:%d \n",
						__func__, brightness, contrast, saturation);

//	VIOC_VIN_SetLUT_by_table(pVIN, rgb);
	tccxxx_cif_vin_lut_buffer_update(0, rgb);


	return 0;
}

int lvds_write_ie(int cmd, unsigned char value, struct tcc_camera_device * vdev)
{
	VPRINTK("%s - cmd:%d, value:%d\n", __func__, cmd, value);

	int ret = FAIL;

	if (value < 0 || value > 255)
		value = 127;
	
	switch(cmd)
	{
		case SET_CAM_BRIGHTNESS:
			_cam_brightness = value;
			break;
		case SET_CAM_CONTRAST:
			_cam_contrast = value;
			break;
		case SET_CAM_SATURATION:
			_cam_saturation = value;
			break;
		case SET_DVRS_BRIGHTNESS:
 			_dvrs_brightness = value;
			break;
		case SET_DVRS_CONTRAST:
 			_dvrs_contrast = value;
			break;
		case SET_DVRS_SATURATION:
 			_dvrs_saturation = value;
			break;
		default:
			break;
	}

	if(cmd >= SET_CAM_BRIGHTNESS && cmd <= SET_CAM_SATURATION)
		ret = setLUT(_cam_brightness, _cam_contrast, _cam_saturation);
	else if(cmd >= SET_DVRS_BRIGHTNESS && cmd <= SET_DVRS_SATURATION)
		ret = setLUT(_dvrs_brightness, _dvrs_contrast, _dvrs_saturation);
	
	VPRINTK("%s,ret = %d \n", __func__ ,ret);

	return ret;
}

int lvds_read_ie(int cmd, unsigned char *level, struct tcc_camera_device * vdev)
{
	VPRINTK("%s-\n", __func__);

	switch(cmd)
	{
		case GET_CAM_BRIGHTNESS:
			*level = _cam_brightness;
			break;
		case GET_CAM_CONTRAST:
			*level = _cam_contrast;
			break;
		case GET_CAM_SATURATION:
			*level = _cam_saturation;
			break;
		default:
			break;
	}

	return 0;
}

static int lvds_sensor_capture(void)
{
	VPRINTK("%s-\n", __func__);
	return 0;
}

static int lvds_sensor_capturecfg(int width, int height)
{
	VPRINTK("%s-\n", __func__);
	return 0;
}

static int sensor_zoom(int val,struct tcc_camera_device * vdev)
{
	VPRINTK("%s-\n", __func__);
	return 0;
}

static int sensor_autofocus(int val,struct tcc_camera_device * vdev)
{
	VPRINTK("%s-\n", __func__);
	return 0;
}

static int sensor_effect(int val)
{
	VPRINTK("%s-\n", __func__);
	return 0;
}

static int sensor_flip(int val)
{
	VPRINTK("%s-\n", __func__);
	return 0;
}

static int sensor_iso(int val)
{
	VPRINTK("%s-\n", __func__);
	return 0;
}

static int sensor_me(int val)
{
	VPRINTK("%s-\n", __func__);
	return 0;
}

static int sensor_wb(int val)
{
	VPRINTK("%s-\n", __func__);
	return 0;
}

static int sensor_bright(int val,struct tcc_camera_device * vdev)
{
	VPRINTK("%s-\n", __func__);
	return 0;
}

static int sensor_scene(int val,struct tcc_camera_device * vdev)
{
	VPRINTK("%s-\n", __func__);
	return 0;
}

static int sensor_contrast(int val,struct tcc_camera_device * vdev)
{
	VPRINTK("%s-\n", __func__);
	return 0;
}

static int sensor_saturation(int val,struct tcc_camera_device * vdev)
{
	VPRINTK("%s-\n", __func__);
	return 0;
}

//GT system start
static int sensor_u_gain(int val,struct tcc_camera_device * vdev)
{
	VPRINTK("%s-\n", __func__);
	return 0;
}
static int sensor_v_gain(int val,struct tcc_camera_device * vdev)
{
	VPRINTK("%s-\n", __func__);
	return 0;
}
static int sensor_sharpness(int val,struct tcc_camera_device * vdev)
{
	VPRINTK("%s-\n", __func__);
	return 0;
}
//GT system end

static int sensor_hue(int val,struct tcc_camera_device * vdev)
{
	VPRINTK("%s-\n", __func__);
	return 0;
}

static int sensor_getVideo( struct tcc_camera_device * vdev)
{
	char ret;

	mutex_lock(&sensor_lock);
	DDI_I2C_Read(LVDS_REG_RESET , 1, &ret, 1,vdev);
	if ((ret & 0x80) == 0x00)  {
		ret = 0; // detected video signal
	} else {
		ret = 1; // not detected video signal
	}
	mutex_unlock(&sensor_lock);

	return ret;
}

static int sensor_checkNoise( struct tcc_camera_device * vdev)
{
	char ret;

	mutex_lock(&sensor_lock);
	DDI_I2C_Read(LVDS_REG_RESET , 1, &ret, 1,vdev);
	if ((ret & 0xE8) == 0x68)  {
		ret = 0; // not detected video noise
	} else {
		ret = 1; // detected video noise
	}

	mutex_unlock(&sensor_lock);
	return ret;
}
static int lvds_sensor_setPath(int val,struct tcc_camera_device * vdev)
{
	VPRINTK("%s-\n", __func__);
	return 0;
}

static int lvds_sensor_getVideoFormat(void)
{
	VPRINTK("%s-\n", __func__);
	return 0;
}

static int lvds_sensor_check_esd(int val)
{
	VPRINTK("%s-\n", __func__);
	return 0;
}

static int lvds_sensor_check_luma(int val)
{
	VPRINTK("%s-\n", __func__);
	return 0;
}

void adas_sensor_info_init(TCC_SENSOR_INFO_TYPE *sensor_info)
{
	printk("%s - MAX96706 sensor info\n", __func__);

	sensor_info->preview_w				= 1280;
	sensor_info->preview_h				= 720;
	sensor_info->capture_w				= 1280;
	sensor_info->capture_h				= 720;

	sensor_info->cam_capchg_width			= 1920;
	sensor_info->framerate				= 15;
	//2017.12.22 - LVDS SVM Display Timing Fixed. PCLK(Positive Edge) HS(Active High) VS(Active High)
	//Because of setup time issue, AVN detects PCLK at negative edge
	sensor_info->p_clock_pol			= NEGATIVE_EDGE; //POSITIVE_EDGE;
	sensor_info->v_sync_pol 			= ACT_HIGH;
	sensor_info->h_sync_pol 			= ACT_HIGH;
	sensor_info->de_pol 				= ACT_LOW;
	sensor_info->field_bfield_low			= OFF;
	sensor_info->gen_field_en			= OFF;
	sensor_info->conv_en				= OFF;
	sensor_info->hsde_connect_en			= ON;
	sensor_info->vs_mask				= ON;
	sensor_info->input_fmt				= FMT_YUV422_8BIT;
	sensor_info->data_order 			= ORDER_RGB;
	sensor_info->intl_en				= OFF;
	sensor_info->intpl_en				= OFF;
	sensor_info->format 				= M420_ZERO;
	sensor_info->capture_skip_frame 		= 2;
	sensor_info->sensor_sizes			= sensor_size;
}
void lvds_sensor_info_init(TCC_SENSOR_INFO_TYPE *sensor_info)
{
	printk("%s - MAX96706 sensor info\n", __func__);

	sensor_info->preview_w				= 1920;
	sensor_info->preview_h				= 720;
	sensor_info->capture_w				= 1920;
	sensor_info->capture_h				= 720;

	sensor_info->cam_capchg_width			= 1920;
	sensor_info->framerate				= 15;
	//2017.12.22 - LVDS SVM Display Timing Fixed. PCLK(Positive Edge) HS(Active High) VS(Active High)
	//Because of setup time issue, AVN detects PCLK at negative edge
	sensor_info->p_clock_pol			= NEGATIVE_EDGE; //POSITIVE_EDGE;
	sensor_info->v_sync_pol 			= ACT_HIGH;
	sensor_info->h_sync_pol 			= ACT_HIGH;
	sensor_info->de_pol 				= ACT_LOW;
	sensor_info->field_bfield_low			= OFF;
	sensor_info->gen_field_en			= OFF;
	sensor_info->conv_en				= OFF;
	sensor_info->hsde_connect_en			= ON;
	sensor_info->vs_mask				= ON;
	sensor_info->input_fmt				= FMT_YUV422_8BIT;
	sensor_info->data_order 			= ORDER_RGB;
	sensor_info->intl_en				= OFF;
	sensor_info->intpl_en				= OFF;
	sensor_info->format 				= M420_ZERO;
	sensor_info->capture_skip_frame 		= 2;
	sensor_info->sensor_sizes			= sensor_size;
}

void lvds_sensor_init_fnc(SENSOR_FUNC_TYPE_DAUDIO *sensor_func)
{
	printk("%s - MAX96706 sensor function\n", __func__);

	sensor_func->sensor_open				= lvds_sensor_open;
	sensor_func->sensor_close				= lvds_sensor_close;

	sensor_func->sensor_preview				= lvds_sensor_preview;
	sensor_func->sensor_capture				= lvds_sensor_capture;
	sensor_func->sensor_capturecfg				= lvds_sensor_capturecfg;
	sensor_func->sensor_reset_video_decoder			= lvds_sensor_resetVideoDecoder;
	sensor_func->sensor_init				= lvds_init;
	sensor_func->sensor_read_id				= lvds_read_id;
	sensor_func->sensor_check_video_signal			= lvds_check_video_signal;
	sensor_func->sensor_display_mute			= lvds_display_mute;
	sensor_func->sensor_close_cam				= lvds_close_cam;
	sensor_func->sensor_write_ie				= lvds_write_ie;
	sensor_func->sensor_read_ie				= lvds_read_ie;
	sensor_func->sensor_get_preview_size			= lvds_sensor_get_preview_size;
	sensor_func->sensor_get_capture_size			= lvds_sensor_get_capture_size;
	sensor_func->Set_Zoom					= sensor_zoom;
	sensor_func->Set_AF					= sensor_autofocus;
	sensor_func->Set_Effect					= sensor_effect;
	sensor_func->Set_Flip					= sensor_flip;
	sensor_func->Set_ISO					= sensor_iso;
	sensor_func->Set_ME					= sensor_me;
	sensor_func->Set_WB					= sensor_wb;
	sensor_func->Set_Bright					= sensor_bright;
	sensor_func->Set_Scene					= sensor_scene;
	sensor_func->Set_Contrast				= sensor_contrast;
	sensor_func->Set_Saturation				= sensor_saturation;
	sensor_func->Set_Hue					= sensor_hue;
	sensor_func->Get_Video					= sensor_getVideo;
	sensor_func->Check_Noise				= sensor_checkNoise;

	sensor_func->Set_Path					= lvds_sensor_setPath;
	sensor_func->Get_videoFormat				= lvds_sensor_getVideoFormat;
	sensor_func->Set_writeRegister				= sensor_writeRegister;
	sensor_func->Get_readRegister				= sensor_readRegister;

	sensor_func->Check_ESD					= NULL;
	sensor_func->Check_Luma					= NULL;

	sensor_func->Enable_signal_error_monitoring		= sensor_enable_signal_error_monitoring;
	sensor_func->Signal_error_monitoring				= sensor_signal_error_monitoring;
}
