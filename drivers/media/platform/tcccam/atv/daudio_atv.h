#ifndef __DAUDIO_ATV_H__
#define __DAUDIO_ATV_H__

#define REG_TERM 0xFF
#define VAL_TERM 0xFF


#define FRAMESKIP_COUNT_FOR_CAPTURE 1
#define SENSOR_FRAMERATE			15

#define FEATURE_TV_STANDARD_NTSC

#define PRV_ZOFFX 0
#define PRV_ZOFFY 0
#define CAP_ZOFFX 0
#define CAP_ZOFFY 0

#define CAM_2XMAX_ZOOM_STEP 25
#define CAM_CAPCHG_WIDTH	720

#define I2C_RETRY_COUNT		3

//(hklee)2018.10.04 - IE Default Value Define
#define DEFAULT_LVDS_SVM_BRIGHTNESS	122
#define DEFAULT_LVDS_SVM_CONTRAST	122
#define DEFAULT_LVDS_SVM_SATURATION	122

#define DEFAULT_TW9990_CAM_BRIGHTNESS	123
#define DEFAULT_TW9990_CAM_CONTRAST	95
#define DEFAULT_TW9990_CAM_SATURATION	128

enum daudio_atvs {
	SENSOR_TW9990 = 0,
	SENSOR_CMMB,
	SENSOR_LVDS,	
	SENSOR_MAX,
};

/**
 * vdloss
 * 1 = Video not present. (sync is not detected in number of consecutive line periods specified by MISSCNT register)
 * 0 = Video detected.
 *
 * hlock
 * 1 = Horizontal sync PLL is locked to the incoming video source. 0 = Horizontal sync PLL is not locked.
 *
 * vlock
 * 1 = Vertical logic is locked to the incoming video source. 0 = Vertical logic is not locked.
 *
 * slock
 * 1 = Sub-carrier PLL is locked to the incoming video source. 0 = Sub-carrier PLL is not locked.
 **/

typedef struct {
	int vdloss;
	int hlock;
	int vlock;
	int slock;
	int mono;
} tw_cstatus;

#define VDLOSS	0x80
#define HLOCK	0x40
#define SLOCK	0x20
#define VLOCK	0x08
#define MONO	0x02


/**
-* @author arc@cleinsoft
-* @date 2014/12/19
-* TW9912 input video selection parameter added
-**/

typedef enum {
	TW_YIN0=0,
	TW_YIN1,
	TW_YIN2,
} eTW_YSEL;

typedef enum {
	TW_NTSC=0,
	TW_PAL
} eTW_STANDARD;


typedef struct SENSOR_FUNC_TYPE_DAUDIO_ {
	int (*sensor_open)(eTW_YSEL yin,struct tcc_camera_device * vdev);
	int (*sensor_close)(struct tcc_camera_device * vdev);
	int (*PowerDown)(bool bChangeCamera);

	int (*sensor_preview)(struct tcc_camera_device * vdev);
	int (*Set_CameraMode)(int camera_type, int camera_encode,struct tcc_camera_device * vdev);
	int (*sensor_capture)(struct tcc_camera_device * vdev);
	int (*sensor_capturecfg)(int width, int height,struct tcc_camera_device * vdev);
	int (*sensor_reset_video_decoder)(struct tcc_camera_device * vdev);
	int (*sensor_init)(void);
	int (*sensor_read_id)(void);
	int (*sensor_check_video_signal)(struct tcc_camera_device * vdev);
	int (*sensor_display_mute)(int mute);
	void (*sensor_close_cam)(struct tcc_camera_device * vdev);
	int (*sensor_write_ie)(int cmd, unsigned char value,struct tcc_camera_device * vdev);
	int (*sensor_read_ie)(int cmd, unsigned char *level,struct tcc_camera_device * vdev);
	void (*sensor_get_preview_size)(int *width, int *heith);
	void (*sensor_get_capture_size)(int *width, int *height);

	int (*Set_Zoom)(int val);
	int (*Set_AF)(int val);
	int (*Set_Effect)(int val);
	int (*Set_Flip)(int val);
	int (*Set_ISO)(int val);
	int (*Set_ME)(int val);
	int (*Set_WB)(int val);
	int (*Set_Bright)(int val,struct tcc_camera_device * vdev);
	int (*Set_Scene)(int val,struct tcc_camera_device * vdev);
	int (*Set_Exposure)(int val);
	int (*Set_FocusMode)(int val);
	int (*Set_Contrast)(int val,struct tcc_camera_device * vdev);
	int (*Set_Saturation)(int val,struct tcc_camera_device * vdev);
	
	int (*Set_UGain)(int val,struct tcc_camera_device * vdev);     //GT system start
	int (*Set_VGain)(int val,struct tcc_camera_device * vdev);
	int (*Set_Sharpness)(int val,struct tcc_camera_device * vdev); //GT system end

	int (*Set_Hue)(int val,struct tcc_camera_device * vdev);
	int (*Get_Video)(void);
	int (*Check_Noise)(void);

	int (*Set_Path)(int val,struct tcc_camera_device * vdev);
	int (*Get_videoFormat)(void);
	int (*Set_writeRegister)(int reg, int val, struct tcc_camera_device * vdev);
	int (*Get_readRegister)(int reg, struct tcc_camera_device * vdev);

	int (*Check_ESD)(int val);
	int (*Check_Luma)(int val);

	unsigned int (*Enable_signal_error_monitoring)(unsigned int enable, struct tcc_camera_device * vdev);
	unsigned int (*Signal_error_monitoring)(struct tcc_camera_device * vdev);
} SENSOR_FUNC_TYPE_DAUDIO;


extern struct capture_size sensor_sizes[];
extern struct sensor_reg *sensor_reg_common[];
extern void sensor_info_init(TCC_SENSOR_INFO_TYPE *sensor_info);
extern void sensor_init_fnc(SENSOR_FUNC_TYPE *sensor_func);
#if defined(INCLUDE_BOARD3HW_GPIO)
extern void tw9990_sensor_init_fnc(SENSOR_FUNC_TYPE_DAUDIO *sensor_func);
extern void lvds_sensor_init_fnc(SENSOR_FUNC_TYPE_DAUDIO *sensor_func);
#else
extern void tw9921_sensor_init_fnc(SENSOR_FUNC_TYPE_DAUDIO *sensor_func);
#endif
extern void daudio_cmmb_sensor_init_fnc(SENSOR_FUNC_TYPE_DAUDIO *sensor_func);

int datv_init(struct tcc_camera_device * vdev);
int datv_read_id(void);
int datv_display_mute(int mute, struct tcc_camera_device * vdev);
void datv_close_cam(struct tcc_camera_device * vdev);
int datv_get_reset_pin(void);

// @ 2015-07-10: JJongspi
//void datv_get_preview_size(int *width, int *height);
void datv_get_preview_size(int *width, int *height, int encode,struct tcc_camera_device * vdev);
void datv_get_capture_size(int *width, int *height, struct tcc_camera_device * vdev);

//D-Audio Image Enhancement
int datv_write_ie(int cmd, unsigned char value,struct tcc_camera_device * vdev);
int datv_read_ie(int cmd, unsigned char *level, struct tcc_camera_device * vdev);

#endif
