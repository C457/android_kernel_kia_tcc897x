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
 
#include <generated/autoconf.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/videodev2.h>
#include <linux/fb.h>
#include <linux/clk.h>
#include <linux/jiffies.h>
#include <linux/of.h>
#include <linux/of_device.h>

#include <media/v4l2-common.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-device.h>
#if defined(CONFIG_ARCH_TCC898X)
#include <video/tcc/tcc_cam_ioctrl.h>
#else
#include <mach/tcc_cam_ioctrl.h>
#endif
#include "camera_core.h"
#include "cam_clock.h"

#if defined(CONFIG_VIDEO_ATV_SENSOR_DAUDIO)
#include <mach/daudio.h>
#include "atv/daudio_atv.h"
#include "atv/daudio_lvds.h"
#include <mach/gpio.h>
#endif
#include "cam_reg.h"
#include "sensor_if.h"
#include "tcc_camera_device.h"
#include "tcc_cam.h"
#include "tcc_cam_direct_display_if.h"
#include "tcc_cam_switchmanager.h"
#include <video/tcc/viocmg.h>

#define DRIVER_VERSION "v2.0"
#define DRIVER_AUTHOR  "Telechips.Co.Ltd"
#define DRIVER_DESC    "TCC CAMERA driver"

static int debug	= 0;
#define TAG		"camera_core"
#define dprintk(msg...)	if(debug) { printk( "Camera_core: " msg); }
#define log(msg...)	if(debug) { printk(TAG ": %s - ", __func__); printk(msg); }
#define FUNCTION_IN	log("In\n");
#define FUNCTION_OUT	log("Out\n");

static const int LVDS_SVM_97 = 4;
static const int LVDS_SVM_111 = 6;
static const int DVRS_RVM = 5;
static const int ADAS_PRK = 7;

static int cam_err;

#if defined(CONFIG_HIBERNATION)
extern unsigned int do_hibernation;
extern unsigned int do_hibernate_boot;
#endif

extern int tcc_ctrl_ext_frame(char enable);
extern void tcc_ext_mutex(char lock, char bCamera);
extern int get_camera_type(void);

int tcc_videobuf_inputenum(struct v4l2_input *input)
{
	/* default handler assumes 1 video input (the camera) */
	int index = input->index;

	memset(input, 0, sizeof(*input));
	input->index = index;

	if (index > 0)
		return -EINVAL;

	strlcpy(input->name, "camera", sizeof(input->name));
	input->type = V4L2_INPUT_TYPE_CAMERA;

	return 0;
}

int tcc_videobuf_g_input(unsigned int *input)
{
	*input = 0;
	
	return 0;
}

int tcc_videobuf_s_input(unsigned int *input)
{
	if (*input > 0)
		return -EINVAL;

	return 0;
}

int tcc_videobuf_s_caminfo(unsigned int *input, struct tcc_camera_device * vdev)
{
	int camera_type = 0;

	if (*input < 0)
		return -EINVAL;

	cam_err = 0;
	vdev->cam_err.des_ic_err = 0;
	vdev->cam_err.soc_dma_err = 0;
	vdev->cam_err.des_signal_err = 0;
	vdev->cam_err.des_lock_fail = 0;

	vdev->CameraID = *input;

	if(vdev->CameraID == DAUDIO_CAMERA_REAR)
	{
		camera_type = get_camera_type();

		if(camera_type == LVDS_SVM_97 || camera_type == LVDS_SVM_111 || camera_type == DVRS_RVM) // Digital - 1920*720p, SVM = 97.5MHz(4), 111.8MHz(6) / DVRS RVM(5)
			vdev->data.cam_info = DAUDIO_CAMERA_LVDS;
		else if (camera_type == ADAS_PRK)
			vdev->data.cam_info = DAUDIO_ADAS_PRK; // Digital - 1280*720p, 74.25MHz
		else
			vdev->data.cam_info = DAUDIO_CAMERA_REAR; // Analog(Default) - CVBS, 720*480i
	}
	else if(vdev->CameraID == DAUDIO_CAMERA_LVDS)
		vdev->data.cam_info = DAUDIO_CAMERA_LVDS;
		
	printk("cam_info : %d \n", vdev->data.cam_info);

	cam_clock_clkrate(vdev);

#if defined(CONFIG_VIDEO_ATV_SENSOR_DAUDIO)
	datv_init(vdev);                //  temp 20170215 mhjung
#endif

	/* Init the camera IF */
	tcc_get_sensor_info(vdev, vdev->data.cam_info);

	return 0;
}

int tcc_videobuf_g_param(struct v4l2_streamparm *gparam, struct tcc_camera_device * vdev)
{
	memset(gparam,0x00,sizeof(*gparam));
	gparam->parm.capture.capturemode=vdev->data.cif_cfg.oper_mode;
	
	return 0;
}

int tcc_videobuf_s_param(struct v4l2_streamparm *sparam)
{
	return 0;
}

int tcc_videobuf_enum_fmt(struct v4l2_fmtdesc *fmt)
{	
	return sensor_if_enum_pixformat(fmt);
}

int tcc_videobuf_try_fmt(struct v4l2_format *fmt)
{	
	//return sensor_try_format(&fmt->fmt.pix);

	return 0;
}

int tcc_videobuf_g_fmt(struct v4l2_format *fmt, struct tcc_camera_device * vdev)
{
	/* get the current format */
	memset(&fmt->fmt.pix, 0, sizeof(fmt->fmt.pix));
	fmt->fmt.pix = vdev->pix_format;
	
	return 0;
}

int tcc_videobuf_s_fmt(struct v4l2_format *fmt, struct tcc_camera_device * vdev)
{
	vdev->pix_format.width	= fmt->fmt.pix.width;
	vdev->pix_format.height	= fmt->fmt.pix.height;
	vdev->pix_format.pixelformat = fmt->fmt.pix.pixelformat;

	return tccxxx_cif_set_resolution(vdev, fmt->fmt.pix.pixelformat, fmt->fmt.pix.width, fmt->fmt.pix.height);
}

int tcc_videobuf_querycap(struct v4l2_capability *cap, struct tcc_camera_device * vdev)
{
	memset(cap, 0, sizeof(struct v4l2_capability));
	
	strlcpy(cap->driver, CAM_NAME, sizeof(cap->driver));
	strlcpy(cap->card, vdev->vfd->name, sizeof(cap->card));

	cap->bus_info[0] = '\0';
	cap->version = KERNEL_VERSION(3, 18, 00);
	cap->capabilities = V4L2_CAP_VIDEO_CAPTURE |				
						V4L2_CAP_VIDEO_OVERLAY |
						V4L2_CAP_READWRITE | 
						V4L2_CAP_STREAMING;
	
	return 0;
}

int tcc_videobuf_g_fbuf(struct v4l2_framebuffer *fargbuf, struct tcc_camera_device * vdev)
{
	memcpy(fargbuf, &vdev->fbuf, sizeof(struct v4l2_framebuffer));

	return 0;
}

int tcc_videobuf_s_fbuf(struct v4l2_framebuffer *fargbuf, struct tcc_camera_device * vdev)
{
	vdev->fbuf.base = fargbuf->base;
	vdev->fbuf.fmt = fargbuf->fmt;				

	return 0;
}

int tcc_videobuf_reqbufs(struct v4l2_requestbuffers *req, struct tcc_camera_device * vdev)
{	
	if(req->memory != V4L2_MEMORY_MMAP && req->memory != V4L2_MEMORY_USERPTR \
		&& req->memory != V4L2_MEMORY_OVERLAY) 
	{
		printk("reqbufs: memory type invalid\n");
		return -EINVAL;
	}
	
	if(req->type != V4L2_BUF_TYPE_VIDEO_CAPTURE) 
	{
		printk("reqbufs: video type invalid\n");
		return -EINVAL;
	}

	return tccxxx_cif_buffer_set(vdev, req);
}

int tcc_videobuf_set_camera_addr(struct v4l2_requestbuffers *req, struct tcc_camera_device * vdev)
{
	tccxxx_set_camera_addr(vdev, req->count, req->reserved[0], req->reserved[1]);
	return 0;
}

int tcc_videobuf_querybuf(struct v4l2_buffer *buf, struct tcc_camera_device * vdev)
{
	struct tccxxx_cif_buffer *cif_buf = vdev->data.buf + buf->index;
	int index = buf->index;	

	if(index < 0 || index > vdev->data.cif_cfg.pp_num)
	{
		printk(KERN_WARNING "querybuf error : index : %d / %d", index, vdev->data.cif_cfg.pp_num);
		return -EINVAL;
	}
	
	memset(buf, 0, sizeof(*buf));
	buf->type 					= V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf->index 					= index;
	buf->flags 					= V4L2_BUF_FLAG_MAPPED;

	buf->flags					|= cif_buf->v4lbuf.flags;
	buf->field 					= V4L2_FIELD_NONE;
	buf->timestamp 				= cif_buf->v4lbuf.timestamp;
	buf->sequence 				= cif_buf->v4lbuf.sequence;
	buf->memory 				= V4L2_MEMORY_MMAP;

	if(vdev->data.cif_cfg.fmt == 0/* yuv420 */)
		buf->length = PAGE_ALIGN(vdev->pix_format.width*vdev->pix_format.height*2);		
	else
		buf->length = PAGE_ALIGN(vdev->pix_format.width*vdev->pix_format.height*3/2);	
	
	cif_buf->v4lbuf.length		= buf->length;
	cif_buf->v4lbuf.index		= index;
	buf->m.offset 				= buf->index * buf->length;
	cif_buf->v4lbuf.m.offset	= buf->index * buf->length;

	dprintk("<%d :: [PA]0x%x / flag: 0x%x  >\n", index, (unsigned int)(cif_buf->v4lbuf.m.offset), (unsigned int)(buf->flags));	
	
	return 0;
}

int tcc_videobuf_qbuf(struct v4l2_buffer *buf, struct tcc_camera_device * vdev)
{
	struct tccxxx_cif_buffer *cif_buf = vdev->data.buf + buf->index;

	if(buf->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EAGAIN;

	if((buf->index < 0) || (buf->index > vdev->data.cif_cfg.pp_num))
		return -EAGAIN;

	if(cif_buf->v4lbuf.flags & V4L2_BUF_FLAG_QUEUED)
		cif_buf->v4lbuf.flags |= ~V4L2_BUF_FLAG_QUEUED;

	if(cif_buf->v4lbuf.flags & V4L2_BUF_FLAG_DONE)
		cif_buf->v4lbuf.flags |= ~V4L2_BUF_FLAG_DONE;

	cif_buf->v4lbuf.flags |= V4L2_BUF_FLAG_QUEUED;

	list_add_tail(&cif_buf->buf_list, &(vdev->data.list));

	return 0;			
}

// 20131018 swhwang, for check video frame interrupt
int tcc_interrupt_check(int arg, struct tcc_camera_device * vdev)
{
	return vdev->check_int;
}

int tcc_videobuf_dqbuf(struct v4l2_buffer *buf, struct file *file )
{
	struct tcc_camera_device *vdev = video_drvdata(file);
	struct tccxxx_cif_buffer *cif_buf;

	if(list_empty(&(vdev->data.done_list))) 
	{
		if(file->f_flags & O_NONBLOCK)
		{
			printk("file->f_flags Fail!!\n");
			return -EAGAIN;
		}
			
		vdev->data.wakeup_int = 0;
		
		if(wait_event_interruptible_timeout(vdev->data.frame_wait, vdev->data.wakeup_int == 1, msecs_to_jiffies(500)) <= 0) 
		{
			vdev->cam_err.soc_dma_err = 1;
			printk("wait_event_interruptible_timeout 500ms!!\n");
			// 20131018 swhwang, for check video frame interrupt
			if(vdev->check_int == 1)		vdev->check_int = 0; 
			return -EFAULT;
		}
		else if(vdev->check_int == 0)
		{
			printk("interruptible flag 0->1\n");
			vdev->check_int = 1;
		}
		vdev->cam_err.soc_dma_err = 0;

		/* Should probably recheck !list_empty() here */
		if(list_empty(&(vdev->data.done_list)))
		{
			printk("It needs list_empty!!\n");
			return -ENOMEM;
		}
	}

	cif_buf = list_entry(vdev->data.done_list.next, struct tccxxx_cif_buffer, buf_list);

	list_del(vdev->data.done_list.next);
	cif_buf->v4lbuf.flags &= ~V4L2_BUF_FLAG_DONE;

	memcpy(buf, &(cif_buf->v4lbuf), sizeof(struct v4l2_buffer));

	return 0;
}

int tcc_videobuf_streamon(enum v4l2_buf_type *type, struct tcc_camera_device * vdev)
{
	if(*type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EAGAIN;

	if(!vdev->cam_irq) {
		if((tccxxx_cif_irq_request(vdev)) < 0) {
			printk("FAILED to aquire camera-irq.\n");
		}
		else { 
			vdev->cam_irq = ENABLE;
			dprintk("[%s %d] cif_irq_request success ! \n", __FUNCTION__, __LINE__);
		}
	}
	return tccxxx_cif_start_stream(vdev);
}

//stop capture
int tcc_videobuf_streamoff(enum v4l2_buf_type *type, struct tcc_camera_device * vdev)
{	
	struct tccxxx_cif_buffer *cif_buf;		
	int ret_val;

	if(*type != V4L2_BUF_TYPE_VIDEO_CAPTURE) return -EAGAIN;

	ret_val = tccxxx_cif_stop_stream(vdev);

	if(vdev->cam_irq){
		tccxxx_cif_irq_free(vdev);
	}
	vdev->cam_irq = DISABLE;
	
	if(vdev->data.done_list.next != NULL) {
		while(!list_empty(&(vdev->data.done_list))) {
			cif_buf = list_entry(vdev->data.done_list.next, struct tccxxx_cif_buffer, buf_list);

			list_del(&cif_buf->buf_list);

			cif_buf->v4lbuf.flags &= ~V4L2_BUF_FLAG_DONE;
			cif_buf->v4lbuf.flags |= V4L2_BUF_FLAG_QUEUED;

			list_add_tail(&cif_buf->buf_list, &(vdev->data.list));
		}
	}
	vdev->data.done_list.next = &(vdev->data.done_list);

	return ret_val;
}


int tcc_videobuf_user_jpeg_capture(int * Jpeg_quality, struct tcc_camera_device * vdev)
{	

	struct tccxxx_cif_buffer *cif_buf;			
	int ret_val;

	while(!list_empty(&(vdev->data.done_list)))	{
		cif_buf = list_entry(vdev->data.done_list.next, struct tccxxx_cif_buffer, buf_list);		

		list_del(&cif_buf->buf_list);
	
		cif_buf->v4lbuf.flags &= ~V4L2_BUF_FLAG_DONE;
		cif_buf->v4lbuf.flags |= V4L2_BUF_FLAG_QUEUED;
		
		list_add_tail(&cif_buf->buf_list, &(vdev->data.list));
	}

	vdev->data.done_list.next = &(vdev->data.done_list);
	vdev->data.cif_cfg.cap_status = CAPTURE_NONE;
	vdev->data.cif_cfg.now_frame_num = 0;
	vdev->data.cif_cfg.retry_cnt 	= 0;

	if(!vdev->cam_irq) {
		if((tccxxx_cif_irq_request(vdev)) < 0) {
			printk("FAILED to aquire camera-irq.\n");
		}
		else {
			vdev->cam_irq = ENABLE;
			log("[%s %d] cif_irq_request success ! \n", __FUNCTION__, __LINE__);
		}
	}

	vdev->data.wakeup_int = 0;
	ret_val = tccxxx_cif_capture(*Jpeg_quality, vdev);

	return ret_val;	
}

int tcc_videobuf_user_get_capture_info(TCCXXX_JPEG_ENC_DATA * Jpeg_data, struct tcc_camera_device * vdev)
{	
	unsigned char *pVirt_BaseAddr;

	if(vdev->cam_irq) {
		tccxxx_cif_irq_free(vdev);
		vdev->cam_irq = DISABLE;
	}

	// Clear the memory for the JPEG encoding.
	memset(Jpeg_data, 0x00, sizeof(TCCXXX_JPEG_ENC_DATA));

	Jpeg_data->width 			= vdev->data.cif_cfg.main_set.target_x;
	Jpeg_data->height 			= vdev->data.cif_cfg.main_set.target_y;
	Jpeg_data->target_addr 		= vdev->data.cif_cfg.jpg_info.start_phy_addr;
	Jpeg_data->target_size 		= vdev->data.cif_cfg.jpg_info.header_offset + vdev->data.cif_cfg.jpg_info.header_size;
	
	//header
 	Jpeg_data->header_offset 	= vdev->data.cif_cfg.jpg_info.header_offset;
	Jpeg_data->header_size 		= vdev->data.cif_cfg.jpg_info.header_size;
	//thumb
 	Jpeg_data->thumb_offset 	= vdev->data.cif_cfg.jpg_info.thumb_offset;
	Jpeg_data->thumb_size 		= vdev->data.cif_cfg.jpg_info.thumb_size;
	//stream
	Jpeg_data->bitstream_offset = vdev->data.cif_cfg.jpg_info.bitstream_offset;	
	Jpeg_data->bitstream_size	= vdev->data.cif_cfg.jpg_info.bitstream_size;


	if(Jpeg_data->target_size)
	{
		pVirt_BaseAddr = ioremap_nocache(Jpeg_data->target_addr, Jpeg_data->target_size);

		dprintk("CAM :: 0x%x, 0x%x /// 0x%x, 0x%x \n",						\
				*((unsigned char*)pVirt_BaseAddr),							\
				*((unsigned char*)pVirt_BaseAddr+1),						\
				*((unsigned char*)pVirt_BaseAddr+Jpeg_data->thumb_offset),	\
				*((unsigned char*)pVirt_BaseAddr+Jpeg_data->thumb_offset+1));

		iounmap(pVirt_BaseAddr);
	}
	
 	dprintk("%d x %d => stream: 0x%x - 0x%x, thumb: 0x%x - 0x%x, header: 0x%x - 0x%x \n",
					Jpeg_data->width, Jpeg_data->height,
					Jpeg_data->bitstream_offset, Jpeg_data->bitstream_size,
					Jpeg_data->thumb_offset, Jpeg_data->thumb_size,
					Jpeg_data->header_offset, Jpeg_data->header_size);
	
	return 0;
}

int tcc_videobuf_get_zoom_support(int cameraIndex)
{
	printk("Camera zoom support. \n");
	return 1;
}

static int camera_core_sensor_open(struct tcc_camera_device * vdev)
{
	int ret = 0;
	dprintk("Sensor Device-Init \n");

	if((ret = sensor_if_parse_gpio_dt_data(vdev)) < 0) 
	{
		printk(KERN_ERR "%s: cannot initialize gpio port\n", CAM_NAME);
		goto err;
	}

	if((ret = sensor_if_init(vdev) < 0))
	{
		printk(KERN_ERR "%s: cannot initialize sensor\n", CAM_NAME);
		goto err;
	}	

	dprintk("Sensor Register-Init \n");
	if((ret = tccxxx_cif_init(vdev)) < 0) 
	{
		printk(KERN_ERR "%s: cannot initialize interface hardware\n", CAM_NAME);
		goto err;
	}	

	if((ret = tccxxx_cif_open(vdev)) < 0)
	{
		printk (KERN_ERR "%s: cannot open\n", CAM_NAME);
		goto err;
	}

	return ret;

err:
	sensor_if_cleanup(vdev);
	return ret;	
}

int tcc_videobuf_get_vin_crop(struct v4l2_crop * arg, struct tcc_camera_device * vdev) {
	int ret = 0;

	arg->c.left = vdev->data.cif_cfg.main_set.vin_crop.left;
	arg->c.top = vdev->data.cif_cfg.main_set.vin_crop.top;

	arg->c.width = vdev->data.cif_cfg.main_set.vin_crop.width;
	arg->c.height= vdev->data.cif_cfg.main_set.vin_crop.height;

	return ret;
}

int tcc_videobuf_set_vin_crop(struct v4l2_crop * arg, struct tcc_camera_device * vdev) {
	int ret = 0;
	struct v4l2_rect c_rect = arg->c;
	TCC_SENSOR_INFO_TYPE * i_info = &(vdev->tcc_sensor_info);

	if((c_rect.left < 0 || \
		c_rect.top < 0) || \
		i_info->preview_w < c_rect.width || \
		i_info->preview_h < c_rect.height) {

		pr_err("%s invalid arguments(left: %d top: %d, width: %d height: %d \n", \
			__func__, c_rect.left, c_rect.top, c_rect.width, c_rect.height);

		ret = -EINVAL;
		goto error;
	}

	if(i_info->preview_w < c_rect.width + c_rect.left || \
		i_info->preview_h < c_rect.height + c_rect.top) {

		pr_err("%s invalid arguments(left: %d top: %d, width: %d height: %d \n", \
			__func__, c_rect.left, c_rect.top, c_rect.width, c_rect.height);

		ret = -EINVAL;
		goto error;
	}

	vdev->data.cif_cfg.main_set.vin_crop.left = c_rect.left;
	vdev->data.cif_cfg.main_set.vin_crop.top = c_rect.top;

	vdev->data.cif_cfg.main_set.vin_crop.width = c_rect.width;
	vdev->data.cif_cfg.main_set.vin_crop.height = c_rect.height;

error:
	return ret;
}


static long camera_core_do_ioctl(struct file * file, unsigned int cmd, void * arg) {
	struct tcc_camera_device * vdev = video_drvdata(file);
	int retval = 0;

	switch (cmd) {
		case VIDIOC_ENUMINPUT:
			return tcc_videobuf_inputenum((struct v4l2_input *)arg);
			
		case VIDIOC_G_INPUT:
			return tcc_videobuf_g_input((unsigned int*)arg);

		case VIDIOC_S_INPUT:
			return tcc_videobuf_s_input((unsigned int*)arg);
		
		case VIDIOC_G_PARM:
			return tcc_videobuf_g_param((struct v4l2_streamparm*)arg, vdev);

		case VIDIOC_S_PARM:
			return tcc_videobuf_s_param((struct v4l2_streamparm*)arg);
		
		case VIDIOC_ENUM_FMT:
			return tcc_videobuf_enum_fmt((struct v4l2_fmtdesc*)arg);

		case VIDIOC_TRY_FMT:
			return tcc_videobuf_try_fmt((struct v4l2_format*)arg);

		case VIDIOC_G_FMT:
			return tcc_videobuf_g_fmt((struct v4l2_format*)arg, vdev);

		case VIDIOC_S_FMT:
#if defined(CONFIG_TCC_REAR_CAMERA_DRV)
			vdev->backup_format = *((struct v4l2_format*)arg);
#endif
			return tcc_videobuf_s_fmt((struct v4l2_format*)arg, vdev);

		case VIDIOC_QUERYCTRL:
			return sensor_if_query_control((struct v4l2_queryctrl *)arg);

		case VIDIOC_G_CTRL:
			return sensor_if_get_control((struct v4l2_control *)arg);

		case VIDIOC_S_CTRL:
			return sensor_if_set_control((struct v4l2_control *)arg, 0, vdev);
		
		case VIDIOC_QUERYCAP:
			return tcc_videobuf_querycap((struct v4l2_capability *)arg, vdev);

		case VIDIOC_G_FBUF: /* Get the frame buffer parameters */
			return tcc_videobuf_g_fbuf((struct v4l2_framebuffer *)arg, vdev);

		case VIDIOC_S_FBUF: /* set the frame buffer parameters */
			return tcc_videobuf_s_fbuf((struct v4l2_framebuffer *)arg, vdev);

		case VIDIOC_REQBUFS:
			return tcc_videobuf_reqbufs((struct v4l2_requestbuffers *)arg, vdev);

		case VIDIOC_QUERYBUF:
			return tcc_videobuf_querybuf((struct v4l2_buffer *)arg, vdev);

		case VIDIOC_QBUF:
			return tcc_videobuf_qbuf((struct v4l2_buffer *)arg, vdev);

		case VIDIOC_DQBUF:
			return tcc_videobuf_dqbuf((struct v4l2_buffer *)arg, file);						
			
		case VIDIOC_STREAMON:
			return tcc_videobuf_streamon((enum v4l2_buf_type *)arg, vdev);
		
		case VIDIOC_STREAMOFF:
			return tcc_videobuf_streamoff((enum v4l2_buf_type *)arg, vdev);

		case VIDIOC_G_CROP:
			return tcc_videobuf_get_vin_crop((struct v4l2_crop *) arg, vdev);

		case VIDIOC_S_CROP:
			return tcc_videobuf_set_vin_crop((struct v4l2_crop *) arg, vdev);

		case VIDIOC_USER_JPEG_CAPTURE:
			return tcc_videobuf_user_jpeg_capture((int *)arg, vdev);

		case VIDIOC_USER_GET_CAPTURE_INFO:
			return tcc_videobuf_user_get_capture_info((TCCXXX_JPEG_ENC_DATA *)arg, vdev);

		case VIDIOC_USER_PROC_AUTOFOCUS:
			return sensor_if_adjust_autofocus(vdev);

		case VIDIOC_USER_SET_CAMINFO_TOBEOPEN:
			tcc_videobuf_s_caminfo((unsigned int*)arg, vdev);
			return camera_core_sensor_open(vdev);

		case VIDIOC_USER_GET_MAX_RESOLUTION:
			return sensor_if_get_max_resolution(vdev);

		case VIDIOC_USER_GET_SENSOR_FRAMERATE:
			return sensor_if_get_sensor_framerate(vdev, (int *)arg);

		case VIDIOC_USER_GET_ZOOM_SUPPORT:
			return tcc_videobuf_get_zoom_support((int)arg);

		case VIDIOC_USER_SET_CAMERA_ADDR:
			return tcc_videobuf_set_camera_addr((struct v4l2_requestbuffers *)arg, vdev);

		case VIDIOC_USER_INT_CHECK: // 20131018 swhwang, for check video frame interrupt
			return tcc_interrupt_check((int)arg, vdev);

		case VIDEOC_USER_SET_ZOOM_RECT:
			return tccxxx_cif_set_zoom_rect(arg, vdev);

		case VIDEOC_USER_GET_ZOOM_RECT:
			return tccxxx_cif_get_zoom_rect(arg, vdev);
			
		case VIDIOC_OVERLAY:
		case VIDIOC_ENUMSTD:
		case VIDIOC_G_STD:
		case VIDIOC_S_STD:
		case VIDIOC_QUERYSTD:		
		case VIDIOC_G_AUDIO:
		case VIDIOC_S_AUDIO:
		case VIDIOC_G_AUDOUT:
		case VIDIOC_S_AUDOUT:
		case VIDIOC_G_JPEGCOMP:
		case VIDIOC_S_JPEGCOMP:
		case VIDIOC_G_TUNER:
		case VIDIOC_S_TUNER:
		case VIDIOC_G_MODULATOR:
		case VIDIOC_S_MODULATOR:
		case VIDIOC_G_FREQUENCY:
		case VIDIOC_S_FREQUENCY:
		case VIDIOC_ENUMOUTPUT:
		case VIDIOC_G_OUTPUT:
		case VIDIOC_S_OUTPUT:
			return -EINVAL;

		case VIDIOC_CHECK_CAMERA_MODULE:
			return sensor_if_check_camera_module(vdev);

		case VIDIOC_CHECK_CAMERA_ERROR:	// Incoming Camera DATA Error
			//printk("%s cmd:VIDIOC_CHECK_CAMERA_SIGNAL_ERR\n",__func__);

			// Check i2c errors
			if(vdev->cam_err.des_ic_err) {
				*(unsigned int *)arg = DES_IC_COMMUNICATION_ERR;
				if(cam_err != DES_IC_COMMUNICATION_ERR) {
					printk("CAM_INFO: DES_IC ERROR\n");
				}
				cam_err = DES_IC_COMMUNICATION_ERR;
				return retval;
			}

			// Check the link lock in the DES
			if(vdev->cam_err.des_lock_fail) {
				*(unsigned int *)arg = SERDES_LOCK_ERR;
				if(cam_err != SERDES_LOCK_ERR) {
					printk("CAM_INFO: SERDES LOCK ERROR\n");
				}
				cam_err = SERDES_LOCK_ERR;
				return retval;
			}


			// Check the number of errors in the DES
			if(vdev->cam_err.des_signal_err) {
				*(unsigned int *)arg = SERDES_SIGNAL_ERR;
				if(cam_err != SERDES_SIGNAL_ERR) {
					printk("CAM_INFO: SERDES SIGNAL ERROR\n");
				}
				cam_err = SERDES_SIGNAL_ERR;
				return retval;
			}

			// Check the DMA update on the SoC
			if(vdev->cam_err.soc_dma_err) {
				*(unsigned int *)arg = VIDEO_SIGNAL_ERR;
				if(cam_err != VIDEO_SIGNAL_ERR) {
					printk("CAM_INFO: VIDEO ERROR\n");
				}
				cam_err = VIDEO_SIGNAL_ERR;
				return retval;
			}

			if(cam_err) {
				printk("CAM_INFO: NO ERROR\n");
				cam_err = 0;
			}
			*(unsigned int *)arg = 0;
	
			return retval;

		case VIDIOC_USER_READ_SENSOR_REGISTER:
			return sensor_if_read_i2c(*(int *)arg, vdev);

		case DIRECT_DISPLAY_IF_INITIALIZE:
			tcc_ext_mutex(1, 1);

			retval = direct_display_if_initialize(vdev);

			tcc_ext_mutex(0, 1);
			return retval;

		case DIRECT_DISPLAY_IF_START:
			tcc_ext_mutex(1, 1);

            		switch(get_camera_type())
			{
				case 0:	//CVBS - rearcam
					vdev->rcam_misc.preview_crop_y = 7;
					vdev->rcam_misc.preview_additional_height = 7;
					break;
				case 1:	//SVIDEO - SVM
					vdev->rcam_misc.preview_crop_y = 13;
					vdev->rcam_misc.preview_additional_height = 7;
					break;
				case 4:	//LVDS 97.5MHz
				//hklee 2019.08.23 - get preview size from application. (0,0) will be set as (1920*720)
				//case 5: // DVRS RVM
				/*case 6:	//LVDS 111.8MHz - SVM Camera
					((DIRECT_DISPLAY_IF_PARAMETERS *)arg)->preview_width = 1920;
					((DIRECT_DISPLAY_IF_PARAMETERS *)arg)->preview_height = 720;
					break;
				case 7: //ADAS_PRK
					((DIRECT_DISPLAY_IF_PARAMETERS *)arg)->preview_width = 1280;
					((DIRECT_DISPLAY_IF_PARAMETERS *)arg)->preview_height = 720;*/
				default:
					vdev->rcam_misc.preview_crop_y = 0;
					vdev->rcam_misc.preview_additional_height = 0;
					break;
			}
			retval = direct_display_if_start(*(DIRECT_DISPLAY_IF_PARAMETERS *)arg, vdev);

			tcc_ext_mutex(0, 1);
			return retval;

		case DIRECT_DISPLAY_IF_STOP:
			tcc_ext_mutex(1, 1);

			retval = direct_display_if_stop(vdev);

			tcc_ext_mutex(0, 1);

			return retval;

		case DIRECT_DISPLAY_IF_TERMINATE:
		{
			int alive_extFrame = 0;
			tcc_ext_mutex(1, 1);

			retval = direct_display_if_terminate();

			tcc_ext_mutex(0, 0);
			alive_extFrame = tcc_ctrl_ext_frame(0);
			if(alive_extFrame){
				tcc_ctrl_ext_frame(1);
			}

			return retval;
		}
			
 		case RCAM_GET_STATUS:
			*((unsigned long *)arg) = ((vdev->cam_streaming) && (vdev->preview_method == PREVIEW_DD)) ? 1 : 0;
			break;
		
#if defined(CONFIG_TCC_REAR_CAMERA_DRV)
		case RCAM_STREAMON:
			vdev->is_running_by_gear = 0;
			return tcc_cam_switchmanager_start_preview(vdev);
		
		case RCAM_STREAMOFF:
			return tcc_cam_switchmanager_stop_preview(vdev);
		
		case RCAM_LINEBUF_ADDR_WITH_INDEX:
			printk("[%s] RCAM_LINEBUF_ADDR_WITH_INDEX !!\n", __FUNCTION__);
			rcam_get_line_buf_addr_by_index(vdev, (RCAM_LINE_BUFFER_INFO *)arg);
			break;
		
		case RCAM_LINE_UPDATE:
			printk("[%s] RCAM_LINE_UPDATE !!\n", __FUNCTION__);
			rcam_line_buf_update(vdev, (RCAM_LINE_BUFFER_UPDATE_INFO *)arg);
			break;
#endif
	}
	return 0;	
}

static unsigned int camera_core_poll(struct file * file, struct poll_table_struct * wait) {
	struct tcc_camera_device * vdev = video_drvdata(file);

	poll_wait(file, &(vdev->data.frame_wait), wait);	

	if(vdev->data.cif_cfg.cap_status == CAPTURE_DONE) {
		dprintk("POLL IN ! \r\n");
		dprintk("CAM CLOCK :: initialized to 0 \n");

		return POLLIN;
	} else if(vdev->data.cif_cfg.cap_status == CAPTURE_OVERFLOW) {
		dprintk("POLL ERR ! \r\n");		
		printk("CAM CLOCK :: initialized to 0 \n");

		return POLLERR;
	}

	dprintk("NO_INT ! \r\n"); 
	return 0;
}

extern int range_is_allowed(unsigned long pfn, unsigned long size);
static int camera_core_mmap(struct file * file, struct vm_area_struct * vma) {
	if(range_is_allowed(vma->vm_pgoff, vma->vm_end - vma->vm_start) < 0) {
		printk(KERN_ERR  "camera: this address is not allowed \n");
		return -EAGAIN;
	} 

	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	if(remap_pfn_range(vma,vma->vm_start, vma->vm_pgoff , vma->vm_end - vma->vm_start, vma->vm_page_prot)) {
		return -EAGAIN;
	}

	vma->vm_ops 	= NULL;
	vma->vm_flags 	|= VM_IO;
	vma->vm_flags 	|= VM_DONTEXPAND | VM_DONTDUMP;
	
	return 0;
}

static long camera_core_ioctl(struct file * file, unsigned int cmd, unsigned long arg) {
	return video_usercopy(file, cmd, arg, camera_core_do_ioctl);
}

static int camera_core_open(struct file * file) {
	struct tcc_camera_device * vdev = video_drvdata(file);
	int minor = video_devdata(file)->minor;
	
	FUNCTION_IN

	if(!vdev->vfd || (vdev->vfd->minor != minor))
		return -ENODEV;

	// VIOC Block enable.
	cam_clock_enable(vdev, VIOC_CLOCK);

	FUNCTION_OUT
	return	0;
}

static int camera_core_release(struct file * file) {
	struct tcc_camera_device * vdev = video_drvdata(file);
	
	FUNCTION_IN
	
	if(0 == vdev->cam_streaming) {
		tccxxx_cif_close(vdev);
		
		// VIOC Block Disable.
		cam_clock_disable(vdev, VIOC_CLOCK);

		dprintk("CAM CLOCK :: initialized to 0 \n"); 
	}

	FUNCTION_OUT
	return 0;
}

static struct v4l2_file_operations camera_core_fops = {
	.owner          = THIS_MODULE,
	.poll           = camera_core_poll,
	.unlocked_ioctl = camera_core_ioctl,
	.mmap           = camera_core_mmap,
	.open           = camera_core_open,
	.release        = camera_core_release
};

//#if defined(CONFIG_TCC_REAR_CAMERA_DRV)
struct tcc_camera_device * g_vdev_for_snapshot;
//#endif//defined(CONFIG_TCC_REAR_CAMERA_DRV)

static int camera_core_probe(struct platform_device * pdev) {
	struct tcc_camera_device * vdev;

	vdev = kzalloc(sizeof(struct tcc_camera_device), GFP_KERNEL);
	if(vdev == NULL)
		return -ENOMEM;
	// clear vdev
	memset((void *)vdev, 0, sizeof(struct tcc_camera_device));

	if(pdev->dev.of_node) {
		vdev->camera_np = pdev->dev.of_node;
		dprintk("link device node to camera \n");
		cam_clock_get(vdev);
	} else {
		printk("could not find camera node!! \n");
		return -ENODEV;	
	}
	
	vdev->vfd = video_device_alloc();
	if(!vdev->vfd) {
		printk(KERN_ERR "%s: could not allocate video device struct\n", CAM_NAME);
		return -ENOMEM;
	}

	vdev->vfd->v4l2_dev = kzalloc(sizeof(struct v4l2_device), GFP_KERNEL);
	if(v4l2_device_register(&pdev->dev, vdev->vfd->v4l2_dev) < 0) {
		printk(KERN_ERR "%s : [error] v4l2 register failed\n", pdev->name);
		return -ENODEV;
	}

 	vdev->vfd->release = video_device_release;

 	strlcpy(vdev->vfd->name, CAM_NAME, sizeof(vdev->vfd->name));
 	
 	vdev->vfd->fops = &camera_core_fops;
 	vdev->vfd->minor = -1;

	if(video_register_device(vdev->vfd, VFL_TYPE_GRABBER, vdev->vfd->minor) < 0) {
		printk(KERN_ERR "%s: could not register Video for Linux device\n", CAM_NAME);
		video_device_release(vdev->vfd);		
		return -ENODEV;
	}

	video_set_drvdata(vdev->vfd, vdev);

	spin_lock_init(&vdev->sensor_lock);
	spin_lock_init(&vdev->data.dev_lock);
 	mutex_init(&vdev->data.lock);

    tccxxx_cif_vin_lut_spinlock_init();

	printk(KERN_INFO "%s: registered device video%d [v4l2]\n", CAM_NAME, vdev->vfd->minor);

	g_vdev_for_snapshot = vdev;

#if defined(CONFIG_TCC_REAR_CAMERA_DRV)
	if(viocmg_is_feature_rear_cam_enable()) {
		g_vdev_for_snapshot = vdev;
		rcam_parse_vioc_dt_data(vdev);
		rcam_process_pmap(vdev);
		tcc_cam_switchmanager_probe(vdev);
	}
#endif//defined(CONFIG_TCC_REAR_CAMERA_DRV)
	FUNCTION_OUT

	return 0;
}

static int camera_core_remove(struct platform_device * pdev) {
	struct tcc_camera_device * vdev = platform_get_drvdata(pdev);

	FUNCTION_IN

	// VIOC Block Clock Put.
	cam_clock_put(vdev);
	video_unregister_device(vdev->vfd);
	video_device_release(vdev->vfd);
	kfree(vdev->vfd->v4l2_dev);
	kfree(vdev);
	
	FUNCTION_OUT
	return 0;
}

int camera_core_suspend(struct platform_device * pdev, pm_message_t state) {
	FUNCTION_IN
	
	// DO NOT anything because the context was saved already, so it won't be affected whatever do you.
	// If you print any data in context, you can see strange data. I don't know why.

	FUNCTION_OUT
	return 0;
}

int camera_core_resume(struct platform_device * pdev) {
#if defined(CONFIG_TCC_REAR_CAMERA_DRV)
	struct tcc_camera_device * vdev = platform_get_drvdata(pdev);

	FUNCTION_IN

	if(viocmg_is_feature_rear_cam_enable()) {
		// I don't know why the address that vdev is pointing is changed during suspend and resume.
		// So, I preserve and restore the vdev temporolly here.
		vdev = g_vdev_for_snapshot;
		
		// During snapshot booting, the context before making snapshot will be restored although whatever you do here.
		// So, just stop EarlyCamera only if it's working.
#if defined(CONFIG_HIBERNATION)
        if((do_hibernation == 1) && (do_hibernate_boot == 1)) {
			// stop switching thread
			tcc_cam_switchmanager_stop_monitor(vdev);

			// try to handover
			tcc_cam_switchmanager_handover_handler(vdev);

			// start switching thread again
			tcc_cam_swtichmanager_start_monitor(vdev);
		}
#endif         
	}
	FUNCTION_OUT
#endif//defined(CONFIG_TCC_REAR_CAMERA_DRV)
	struct tcc_camera_device * vdev = platform_get_drvdata(pdev);

	FUNCTION_IN

	// I don't know why the address that vdev is pointing is changed during suspend and resume.
	// So, I preserve and restore the vdev temporolly here.
	vdev = g_vdev_for_snapshot;
	
	// During snapshot booting, the context before making snapshot will be restored although whatever you do here.
	// So, just stop EarlyCamera only if it's working.
#if defined(CONFIG_HIBERNATION) 
	if((do_hibernation == 1) && (do_hibernate_boot == 1)) {

		dprintk("vdev = 0x%x \n", vdev);

		if(pdev->dev.of_node) {
			vdev->camera_np = pdev->dev.of_node;
			dprintk("link device node to camera \n");
			cam_clock_get(vdev);
		} else {
			printk("could not find camera node!! \n");
			return -ENODEV;	
		}
	}
#endif 

	FUNCTION_OUT

	return 0;
}

static struct of_device_id camera_of_match[] = {
	{ .compatible = "telechips-camera" },
	{}
};

static struct platform_driver camera_core_driver = {
	.driver = {
		.name 	= CAM_NAME,
		.owner 	= THIS_MODULE,
		.of_match_table = of_match_ptr(camera_of_match),
	},
	.probe 		= camera_core_probe,
	.remove 		= camera_core_remove,
	.suspend 		= camera_core_suspend,
	.resume 		= camera_core_resume
};

static char banner[] __initdata = KERN_INFO "TCCXXX Camera driver initializing\n";
int __init camera_core_init(void) {
	FUNCTION_IN

	printk(banner);
}

void __exit camera_core_cleanup(void) {
	return;
}

module_init(camera_core_init);
module_exit(camera_core_cleanup);
module_platform_driver(camera_core_driver);
MODULE_DEVICE_TABLE(of, camera_of_match);
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE("GPL");

