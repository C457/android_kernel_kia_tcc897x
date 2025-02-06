#ifndef __TCC_CAMERA_DEVICE_H__
#define __TCC_CAMERA_DEVICE_H__

#include <media/v4l2-device.h>
#include <linux/clk.h>
#include <linux/of.h>
#include <linux/videodev2.h>
#include <soc/tcc/pmap.h>
#if defined(CONFIG_ARCH_TCC898X)
#include <video/tcc/vioc_intr.h>
#else
#include <mach/vioc_intr.h>
#endif
#include <linux/spinlock.h>

#include "tcc_cam.h"
#include "sensor_if.h"
#include "cam_clock.h"
#include "tcc_cam_direct_display_if.h"


/**********************************************************************

tcc_camera_device info struct

***********************************************************************/
struct tcc_camera_device;

enum PREVIEW_METHOD {
	PREVIEW_V4L2,
	PREVIEW_DD,
};

// VIDIOC_CHECK_CAMERA_ERROR error code
#define CAMERA_NO_ERR               0x0000 // No Error
#define DES_IC_COMMUNICATION_ERR    0x0100 // A090201 - Sensor open error by the i2c communication
#define SERDES_LOCK_ERR             0x0101 // A090101 - SERDES link error (GPIO_G06)
#define SERDES_SIGNAL_ERR           0x0102 // A090101 - More than 255 DES errors occured (0xB0E - GMSL1_E)
#define VIDEO_SIGNAL_ERR            0x0103 // A090301 - SoC DMA error

#define SERDES_LOCK_FAIL 0
#define SERDES_LOCK_NORMAL 1
#define SERDES_I2C_FAIL 2

struct rear_cam_misc {
	// lcd panel
	unsigned int lcd_width;
	unsigned int lcd_height;

	// rear cam preview
	unsigned int preview_x; 						// vin position.x in wmix
	unsigned int preview_y; 						// vin position.y in wmix

	unsigned int preview_crop_x;					// rear cam position.x in scaler
	unsigned int preview_crop_y;					// rear cam position.y in scaler

	unsigned int preview_width; 				
	unsigned int preview_height;
	unsigned int preview_format;

	unsigned int preview_additional_width;
	unsigned int preview_additional_height;

	unsigned int draw_preview_width;
	unsigned int draw_preview_height;

	// rear cam parking line
	unsigned int parking_line_x;
	unsigned int parking_line_y;
	unsigned int parking_line_width;
	unsigned int parking_line_height;
	unsigned int parking_line_format;

	unsigned int draw_parking_line_width;
	unsigned int draw_parking_line_height;
};

struct cam_error {
	unsigned int des_ic_err;
	unsigned int soc_dma_err;
	unsigned int des_signal_err;
	unsigned int des_lock_fail;
};

struct tcc_camera_device{
	struct device_node *		camera_np;
	struct video_device *		vfd;
	struct TCCxxxCIF			data;	
	struct v4l2_pix_format		pix_format;
	struct sensor_gpio			gpio_data;
	struct clock_data 			clk_data;	
	struct TCC_VIOC 			vioc;
	struct tccxxx_cif_buffer *	prev_buf;
	struct v4l2_framebuffer 	fbuf;
	struct cam_error			cam_err;

	TCC_SENSOR_INFO_TYPE tcc_sensor_info;
	SENSOR_FUNC_TYPE sensor_func;

	pmap_t pmap_preview;
	pmap_t pmap_viqe;
	pmap_t pmap_pgl;

	spinlock_t sensor_lock;

	int enabled;
	unsigned int cvbs_input;

	// variable for viqe initialize(3D deinterlacing mode)
//	unsigned int bfield;
//	unsigned int frm_cnt;
//	unsigned int field_cnt;

	unsigned char cam_irq;

	unsigned int CameraID;
	unsigned char sensor_status;

	unsigned char skip_frm;
	unsigned char skipped_frm;
	unsigned char frame_lock;
	unsigned int  prev_num;

	unsigned int gZoomStep;

	int check_int;

	unsigned int deint_plugin_status;
	unsigned int sc_plugin_status;
	
	DIRECT_DISPLAY_IF_PARAMETERS	gParams;
	
	unsigned int			* addrPreview;
	
	struct mutex			dd_lock;
	struct task_struct		* threadSwitching;
	struct task_struct		* threadRecovery;
	
	/**** rcam status variable ****/
	int				is_running_by_gear;
	int				cam_streaming;
	int				preview_method;
	int				need_to_backup;

	/**** rear camera pmap ****/
	unsigned int 	mLineBufAddr[2];
	unsigned int	mLineBufSize;

	/**** common camera info backup ****/
	struct v4l2_format		backup_format;
	
	/**** rear cam gear gpio info ****/
	int gear_port, gear_port_irq;
	enum of_gpio_flags gear_active_level;

	struct mutex tcc_cam_switchmanager_lock;
	
	struct rear_cam_misc rcam_misc;

	unsigned long mLineBufIndex;
};

extern int rgb[257];
#endif
