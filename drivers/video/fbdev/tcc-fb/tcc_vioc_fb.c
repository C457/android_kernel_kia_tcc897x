/*
 * linux/drivers/video/tcc/tccfb.c
 *
 * Based on:    Based on s3c2410fb.c, sa1100fb.c and others
 * Author:  <linux@telechips.com>
 * Created: June 10, 2008
 * Description: TCC LCD Controller Frame Buffer Driver
 *
 * Copyright (C) 2008-2009 Telechips
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see the file COPYING, or write
 * to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/kthread.h>

#include <linux/fb.h>
#include <linux/init.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/wait.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/cpufreq.h>
#include <linux/gpio.h>
#include <linux/irq.h>

#ifdef CONFIG_DMA_SHARED_BUFFER
#include <linux/dma-buf.h>
#include <linux/memblock.h>
#include <linux/miscdevice.h>
#include <linux/export.h>
#include <linux/rbtree.h>
#include <linux/rtmutex.h>
#include <linux/sched.h>
#include <linux/seq_file.h>
#include <linux/debugfs.h>
#endif

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_device.h>
#include <linux/of_dma.h>
#include <linux/console.h>

#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/div64.h>
#include <asm/mach/map.h>
#include <asm/mach-types.h>
#ifdef CONFIG_PM
#include <linux/pm.h>
#endif
#ifdef CONFIG_PM_RUNTIME
#include <linux/pm_runtime.h>
#endif

#include <asm/tlbflush.h>
#include <asm/cacheflush.h>

#include <soc/tcc/pmap.h>
#ifdef CONFIG_ARCH_TCC897X
#include <mach/bsp.h>

#include <mach/tccfb.h>
#include <mach/tcc_fb.h>
#include <mach/tcc_scaler_ctrl.h>
#include <mach/tccfb_ioctrl.h>
#include <mach/tcc_composite_ioctl.h>
#include <mach/tcc_component_ioctl.h>
#include <mach/TCC_LCD_Interface.h>

#include <mach/vioc_blk.h>
#include <mach/tca_lcdc.h>
#include <mach/vioc_rdma.h>
#include <mach/vioc_disp.h>
#include <mach/vioc_lut.h>
#include <mach/tcc_wmixer_ioctrl.h>
#include <mach/vioc_outcfg.h>

#include <mach/vioc_api.h>
#include <mach/tca_display_config.h>
#include <mach/daudio_info.h>
#else
#include <mach/vioc_blk.h>
#include <video/tcc/tccfb.h>
#include <video/tcc/tcc_fb.h>
#include <video/tcc/tcc_scaler_ctrl.h>
#include <video/tcc/tccfb_ioctrl.h>
#include <video/tcc/tcc_composite_ioctl.h>
#include <video/tcc/tcc_component_ioctl.h>
#include <video/tcc/TCC_LCD_Interface.h>

#include <video/tcc/tca_lcdc.h>
#include <video/tcc/vioc_rdma.h>
#include <video/tcc/vioc_disp.h>
#include <video/tcc/vioc_lut.h>
#include <video/tcc/tcc_wmixer_ioctrl.h>
#include <video/tcc/vioc_outcfg.h>

#include <video/tcc/vioc_api.h>
#include <video/tcc/tca_display_config.h>
#endif

#ifdef TCC_VIDEO_DISPLAY_DEINTERLACE_MODE
#include "tcc_vioc_viqe_interface.h"
#include "viqe.h"
#endif

#if defined(CONFIG_SYNC_FB)
#include <sw_sync.h>
#include <linux/file.h>
#endif

#include "tcc_vsync.h"

#ifdef CONFIG_VIDEO_TCC_VOUT
#include <video/tcc/tcc_vout_v4l2.h>
extern unsigned int tcc_vout_get_status(void);
#endif


/* Debugging stuff */
static int debug = 0;
#define dprintk(msg...)	if (debug) { printk( "tccfb: " msg); }

static int screen_debug = 0;
#define sprintk(msg...)	if (screen_debug) { printk( "tcc92fb scr: " msg); }


#define FB_NUM_BUFFERS 3

#if defined(CONFIG_USE_DISPLAY_FB_LOCK)
#ifdef CONFIG_DAUDIO_KK
unsigned int fb_lock = true;
#else
static unsigned int fb_lock = true;   //TO FORBID UPDATE
#endif
#endif
#if defined(CONFIG_HIBERNATION) 
extern unsigned int do_hibernation;

#endif//CONFIG_HIBERNATION

#ifdef CONFIG_HDMI_DISPLAY_LASTFRAME
extern VIOC_RDMA * pLastFrame_RDMABase;
extern int tcc_video_check_last_frame(struct tcc_lcdc_image_update *ImageInfo);
#endif

extern void tca_vioc_displayblock_powerOn(struct tcc_dp_device *pDisplayInfo);
extern void tca_vioc_displayblock_powerOff(struct tcc_dp_device *pDisplayInfo);
extern void tca_vioc_displayblock_disable(struct tcc_dp_device *pDisplayInfo);
extern void tca_vioc_displayblock_timing_set(unsigned int outDevice, struct tcc_dp_device *pDisplayInfo,  struct lcdc_timimg_parms_t *mode);
extern void tca_vioc_displayblock_ctrl_set(unsigned int outDevice, struct tcc_dp_device *pDisplayInfo, stLTIMING *pstTiming, stLCDCTR *pstCtrl, unsigned int swapbf);
extern int tca_fb_pan_display(struct fb_var_screeninfo *var, struct fb_info *info);
extern void tca_fb_activate_var(unsigned int dma_addr,  struct fb_var_screeninfo *var, struct tcc_dp_device *pdp_data);
extern void tca_scale_display_update(struct tcc_dp_device *pdp_data, struct tcc_lcdc_image_update *ImageInfo);
extern void tccfb1_set_par(struct tccfb_info *fbi,  struct fb_var_screeninfo *var);

extern void tca_fb_mouse_set_icon(tcc_mouse_icon *mouse_icon);
extern int tca_fb_mouse_move(unsigned int width, unsigned int height, tcc_mouse *mouse, struct tcc_dp_device *pdp_data);
extern void tca_fb_resize_set_value(tcc_display_resize resize_value, TCC_OUTPUT_TYPE output);
extern void tca_fb_output_attach_resize_set_value(tcc_display_resize resize_value);
extern int tca_fb_divide_set_mode(struct tcc_dp_device *pdp_data, char enable, char mode);

extern void tca_fb_attach_start(struct tccfb_info *info);
extern void tca_fb_attach_stop(struct tccfb_info *info);
#ifdef CONFIG_PM_RUNTIME
void tca_fb_runtime_suspend(struct tccfb_info *fbi);
void tca_fb_runtime_resume(struct tccfb_info *fbi);
#endif

extern int tca_fb_suspend(struct device *dev);
extern int tca_fb_resume(struct device *dev);
extern int tca_fb_init(struct tccfb_info *dev);
extern void tca_fb_exit(void);
extern int tca_main_interrupt_reg(char SetClear, struct tccfb_info *info);
extern void tca_vsync_enable(struct tccfb_info *dev, int on);

#if defined(CONFIG_FB_TCC_COMPOSITE)
extern int tcc_composite_detect(void);
extern void tcc_composite_update(struct tcc_lcdc_image_update *update);
#endif

#if defined(CONFIG_FB_TCC_COMPONENT)
extern int tcc_component_detect(void);
extern void tcc_component_update(struct tcc_lcdc_image_update *update);
#endif

extern unsigned char hdmi_get_phy_status(void);
extern unsigned char hdmi_get_system_en(void);

extern int tcc_2d_compression_read(void);

#define CONFIG_FB_TCC_DEVS_MAX	3	// do not change!
#if defined(CONFIG_ANDROID) || !defined(CONFIG_FB1_SUPPORT)
#define CONFIG_FB_TCC_DEVS		1
#else
#define CONFIG_FB_TCC_DEVS		2
#endif//


#if (CONFIG_FB_TCC_DEVS > CONFIG_FB_TCC_DEVS_MAX)
	#undef CONFIG_FB_TCC_DEVS
	#define CONFIG_FB_TCC_DEVS	CONFIG_FB_TCC_DEVS_MAX
#endif

#define SCREEN_DEPTH_MAX	32	// 32 or 16
//								 : 32 - 32bpp(alpah+rgb888)
//								 : 16 - 16bpp(rgb565)

const unsigned int default_scn_depth[CONFIG_FB_TCC_DEVS_MAX] =
{
/* fb0, Layer0: bottom */  (32), // 32 or 16
/* fb1, Layer1: middle */  (32), //  "
/* fb2, Layer2: top    */  (16)  //  "
};

#ifdef CONFIG_OF
static struct of_device_id tccfb_of_match[] = {
	{ .compatible = "telechips,vioc-fb" },
	{}
};
MODULE_DEVICE_TABLE(of, tccfb_of_match);
#endif

#ifdef CONFIG_MALI400_UMP
/*./gpu/mali/ump/include/ump/ump_kernel_interface.h
 *  Add MALI UMP interface in fbdev !! because using X display
 */
#include "../../../gpu/arm/mali400/ump/include/ump/ump_kernel_interface.h"
#define GET_UMP_SECURE_ID_BUF(x) _IOWR('m', 311 + (x), unsigned int)
extern ump_dd_handle ump_dd_handle_create_from_phys_blocks(ump_dd_physical_block * blocks, unsigned long  num_blocks);
static ump_dd_handle ump_wrapped_buffer[CONFIG_FB_TCC_DEVS_MAX][3];
#endif

static struct lcd_panel *lcd_panel;
static int lcd_video_started = 0;

TCC_OUTPUT_TYPE	Output_SelectMode =  TCC_OUTPUT_NONE;
TCC_LAYER_MODE Output_LayerMode = TCC_LAYER_SHOW;

static char  HDMI_pause = 0;
static char HDMI_video_mode = 0;

static unsigned int HDMI_video_width = 0;
static unsigned int HDMI_video_height = 0;
unsigned int HDMI_video_hz = 0;


extern void tca_fb_wait_for_vsync(struct tcc_dp_device *pdata);
extern void tca_fb_vsync_activate(struct tcc_dp_device *pdata);

#if defined(CONFIG_SYNC_FB)
static void tcc_fd_fence_wait(struct sync_fence *fence)
{
	int err = sync_fence_wait(fence, 1000);
	if (err >= 0)
		return;

	if (err == -ETIME)
		err = sync_fence_wait(fence, 10 * MSEC_PER_SEC);

}

static void tcc_fb_update_regs(struct tccfb_info *tccfb, struct tcc_fenc_reg_data *regs)
{
	unsigned int BaseAddr;
#ifdef CONFIG_PM_RUNTIME
	pm_runtime_get_sync(tccfb->fb->dev);
#endif
	if ((regs->fence_fd > 0) && (regs->fence))	{
		tcc_fd_fence_wait(regs->fence);
		sync_fence_put(regs->fence);
	}
	#if defined(CONFIG_USE_DISPLAY_FB_LOCK)
	if(!fb_lock){
	#endif
 	BaseAddr = tccfb->map_dma + regs->var.xres * regs->var.yoffset * (regs->var.bits_per_pixel/8);

	if(tccfb->pdata.Mdp_data.FbPowerState)
		tca_fb_activate_var(BaseAddr, &regs->var, &tccfb->pdata.Mdp_data);

	#if !defined(CONFIG_HWCOMPOSER_OVER_1_1_FOR_STB)
	#if !defined(CONFIG_HWCOMPOSER_OVER_1_1_FOR_MID) && !defined(CONFIG_TCC_DISPLAY_MODE_USE)
	if(tccfb->pdata.Sdp_data.FbPowerState)
		tca_fb_activate_var(BaseAddr, &regs->var, &tccfb->pdata.Sdp_data);
	#endif
	#endif
	#if defined(CONFIG_USE_DISPLAY_FB_LOCK)
	}
	#endif

	tca_fb_vsync_activate(&tccfb->pdata.Mdp_data);
//	tccfb->fb->fbops->fb_pan_display(&regs->var, tccfb->fb);
#ifdef CONFIG_PM_RUNTIME
	pm_runtime_put_sync(tccfb->fb->dev);
#endif
	sw_sync_timeline_inc(tccfb->fb_timeline, 1);
}

static void fence_handler(struct kthread_work *work)
{
	struct tcc_fenc_reg_data *data, *next;
	struct list_head saved_list;

	struct tccfb_info *tccfb =
			container_of(work, struct tccfb_info, fb_update_regs_work);

	mutex_lock(&tccfb->fb_timeline_lock);
	saved_list = tccfb->fb_update_regs_list;
	list_replace_init(&tccfb->fb_update_regs_list, &saved_list);

	list_for_each_entry_safe(data, next, &saved_list, list) {
		tcc_fb_update_regs(tccfb, data);
		list_del(&data->list);
		kfree(data);
	}
        mutex_unlock(&tccfb->fb_timeline_lock);
}

static void ext_fence_handler(struct kthread_work *work)
{
	int save_timeline_fb_max = 0, save_timeline_value = 0, i = 0;

	struct tccfb_info *tccfb =
			container_of(work, struct tccfb_info, ext_update_regs_work);

	mutex_lock(&tccfb->ext_timeline_lock);

	save_timeline_fb_max = tccfb->ext_timeline_max;
	save_timeline_value = tccfb->ext_timeline->value;

	mutex_unlock(&tccfb->ext_timeline_lock);
	pr_info("ext_fence_handler");
	tca_fb_vsync_activate(&tccfb->pdata.Sdp_data);
//	TCC_OUTPUT_FB_UpdateSync(Output_SelectMode);
	for(i=save_timeline_value; i<save_timeline_fb_max; i++){
		sw_sync_timeline_inc(tccfb->ext_timeline, 1);
	}
}
#endif

static void send_vsync_event(struct work_struct *work)
{
	char buf[64];
	char *envp[] = { NULL, NULL };
	struct tccfb_info *fbdev = container_of(work, struct tccfb_info, vsync_work);

	snprintf(buf, sizeof(buf), "VSYNC=%llu", ktime_to_ns(fbdev->vsync_timestamp));
	envp[0] = buf;
	kobject_uevent_env(&fbdev->dev->kobj, KOBJ_CHANGE, envp);

	dprintk("%s %s\n",__func__, buf );
}

void tccfb_output_starter(char output_type, char lcdc_num, stLTIMING *pstTiming, stLCDCTR *pstCtrl, unsigned int format)
{
	struct fb_info *info;
	struct tccfb_info *ptccfb_info =NULL;
	struct tcc_dp_device *pdp_data =NULL;
	unsigned int BaseAddr = 0;
	
	info = registered_fb[0];
	ptccfb_info = info->par;

	if(ptccfb_info == NULL)
		goto error_null_pointer;

	if(ptccfb_info->pdata.Mdp_data.DispNum == lcdc_num)
		pdp_data = &ptccfb_info->pdata.Mdp_data;
	else if(ptccfb_info->pdata.Sdp_data.DispNum == lcdc_num)
		pdp_data = &ptccfb_info->pdata.Sdp_data;
	else
		goto error_null_pointer;
	
	switch(output_type)
	{
		case TCC_OUTPUT_HDMI:
			pdp_data->DispDeviceType = TCC_OUTPUT_HDMI;
			pdp_data->FbUpdateType = FB_SC_RDMA_UPDATE;
			#if !defined(CONFIG_ARCH_TCC898X)
			tca_vioc_displayblock_powerOn(pdp_data);
			#endif
			tca_vioc_displayblock_ctrl_set(VIOC_OUTCFG_HDMI, pdp_data, pstTiming, pstCtrl, format);
			break;

		case TCC_OUTPUT_COMPOSITE:
			pdp_data->DispDeviceType = TCC_OUTPUT_COMPOSITE;
			pdp_data->FbUpdateType = FB_SC_RDMA_UPDATE;
			tca_vioc_displayblock_powerOn(pdp_data);
			tca_vioc_displayblock_ctrl_set(VIOC_OUTCFG_SDVENC, pdp_data, pstTiming, pstCtrl, format);
			break;

		case TCC_OUTPUT_COMPONENT:
			pdp_data->DispDeviceType = TCC_OUTPUT_COMPONENT;
			pdp_data->FbUpdateType = FB_SC_RDMA_UPDATE;
			tca_vioc_displayblock_powerOn(pdp_data);
			tca_vioc_displayblock_ctrl_set(VIOC_OUTCFG_HDVENC, pdp_data, pstTiming, pstCtrl, format);
			break;
	}

	BaseAddr = ptccfb_info->map_dma + ptccfb_info->fb->var.xres *  ptccfb_info->fb->var.yoffset * ( ptccfb_info->fb->var.bits_per_pixel/8);
	tca_fb_activate_var(BaseAddr, &ptccfb_info->fb->var, pdp_data);

	#if defined(TCC_VIDEO_DISPLAY_BY_VSYNC_INT)
		tcc_vsync_reset_all();
	#endif

	return;

error_null_pointer:
	pr_err("%s cannot find data struct fbinfo:%p pdata:%p \n", __func__, ptccfb_info, pdp_data);
}

unsigned int tccfb_output_get_mode(void)
{
	return Output_SelectMode;
}

static int tccfb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
	/* validate bpp */
	if (var->bits_per_pixel > 32)
		var->bits_per_pixel = 32;
	else if (var->bits_per_pixel < 16)
		var->bits_per_pixel = 16;

	/* set r/g/b positions */
	if (var->bits_per_pixel == 16) {
		var->red.offset 	= 11;
		var->green.offset	= 5;
		var->blue.offset	= 0;
		var->red.length 	= 5;
		var->green.length	= 6;
		var->blue.length	= 5;
		var->transp.length	= 0;
	} else if (var->bits_per_pixel == 32) {
		var->red.offset 	= 16;
		var->green.offset	= 8;
		var->blue.offset	= 0;
		var->transp.offset	= 24;
		var->red.length 	= 8;
		var->green.length	= 8;
		var->blue.length	= 8;
		var->transp.length	= 8;
	} else {
		var->red.length 	= var->bits_per_pixel;
		var->red.offset 	= 0;
		var->green.length	= var->bits_per_pixel;
		var->green.offset	= 0;
		var->blue.length	= var->bits_per_pixel;
		var->blue.offset	= 0;
		var->transp.length	= 0;
	}
	if (var->yres_virtual < var->yoffset + var->yres)
		var->yres_virtual = var->yoffset + var->yres;

	return 0;
}

/* tccfb_pan_display
 *
 * pandisplay (set) the controller from the given framebuffer
 * information
*/
static int tccfb_pan_display(struct fb_var_screeninfo *var, struct fb_info *info)
{
	unsigned int base_addr = 0;
	struct tccfb_info *fbi =(struct tccfb_info *) info->par;

	if(!fbi->pdata.Mdp_data.FbPowerState) {
		pr_info("%s fbi->pdata.Mdp_data.FbPowerState:%d \n", __func__, fbi->pdata.Mdp_data.FbPowerState);
		return 0;
	}

	sprintk("%s addr:0x%x Yoffset:%d \n", __func__, base_addr, var->yoffset);

	tca_fb_pan_display(var, info);
	
	return 0;
}



/*
 *      tccfb_set_par - Optional function. Alters the hardware state.
 *      @info: frame buffer structure that represents a single frame buffer
 *
 */
static int tccfb_set_par(struct fb_info *info)
{
	struct tccfb_info *fbi = info->par;
	struct fb_var_screeninfo *var = &info->var;

	sprintk("- tccfb_set_par pwr:%d  \n",fbi->pdata.Mdp_data.FbPowerState);

	if (var->bits_per_pixel == 16)
		fbi->fb->fix.visual = FB_VISUAL_TRUECOLOR;
	else if (var->bits_per_pixel == 32)
		fbi->fb->fix.visual = FB_VISUAL_TRUECOLOR;
	else
		fbi->fb->fix.visual = FB_VISUAL_PSEUDOCOLOR;

	fbi->fb->fix.line_length = (var->xres*var->bits_per_pixel)/8;
	if(fbi->fb->var.rotate != 0)	{
		pr_info("fb rotation not support \n");
		return -1;
	}
	
	if(info->node == 1)
		tccfb1_set_par(fbi, var);
	
	return 0;
}

static int tccfb_ioctl(struct fb_info *info, unsigned int cmd,unsigned long arg)
{
	struct tccfb_info *ptccfb_info = info->par;
	int screen_width = 0, screen_height = 0;

#ifdef CONFIG_MALI400_UMP
	u32 __user *psecureid = (u32 __user *) arg;
	ump_secure_id secure_id;
	ump_dd_physical_block ump_memory_description;
#endif

	screen_width = lcd_panel->xres;
	screen_height = lcd_panel->yres;

	if((0 > info->node) ||(info->node >= CONFIG_FB_TCC_DEVS))	{
		pr_err("ioctl: Error - fix.id[%d]\n", info->node);
		return 0;
	}

	switch(cmd)
	{
#ifdef CONFIG_MALI400_UMP
		case GET_UMP_SECURE_ID_BUF(0):
			{
				if (ump_wrapped_buffer[info->node][0] == 0) {
					ump_memory_description.addr = info->fix.smem_start;
					ump_memory_description.size = info->fix.smem_len;
					ump_wrapped_buffer[info->node][0] = ump_dd_handle_create_from_phys_blocks(&ump_memory_description, 1);
				}

				if(ump_wrapped_buffer[info->node][0] != 0){
					secure_id = ump_dd_secure_id_get(ump_wrapped_buffer[info->node][0]);
					return put_user( (unsigned int)secure_id, psecureid );
				}
				else
					return -ENOTSUPP;
			}

		case GET_UMP_SECURE_ID_BUF(1):
			{
				if (ump_wrapped_buffer[info->node][1] == 0) {
					ump_memory_description.addr = info->fix.smem_start;
					ump_memory_description.size = info->fix.smem_len;
					ump_wrapped_buffer[info->node][1] = ump_dd_handle_create_from_phys_blocks(&ump_memory_description, 1);
				}

				if(ump_wrapped_buffer[info->node][1] != 0){
					secure_id = ump_dd_secure_id_get(ump_wrapped_buffer[info->node][1]);
					return put_user( (unsigned int)secure_id, psecureid );
				}
				else
					return -ENOTSUPP;
			}

		case GET_UMP_SECURE_ID_BUF(2):
			{
				if (ump_wrapped_buffer[info->node][2] == 0) {
					ump_memory_description.addr = info->fix.smem_start + (info->var.xres * info->var.yres * (info->var.bits_per_pixel/4));
					ump_memory_description.size = info->fix.smem_len;
					ump_wrapped_buffer[info->node][2] = ump_dd_handle_create_from_phys_blocks( &ump_memory_description, 1);
				}
				if(ump_wrapped_buffer[info->node][2] != 0){
					secure_id = ump_dd_secure_id_get(ump_wrapped_buffer[info->node][2]);
					return put_user( (unsigned int)secure_id, psecureid );
				}
				else
					return -ENOTSUPP;
			}
#endif
		case TCC_LCD_FBIOPUT_VSCREENINFO:
		{
#if defined(CONFIG_SYNC_FB)
			struct fb_var_screeninfo var_info;
			struct tcc_fenc_reg_data *regs;
			struct sync_fence *fence;
			struct sync_pt *pt;
			int ret = 0, fd = 0;

			if (copy_from_user(&var_info,
					   (struct fb_var_screeninfo __user *)arg,
					   sizeof(struct fb_var_screeninfo))) {
				ret = -EFAULT;
				break;
			}
			fd = get_unused_fd();

			if(fd< 0){
				pr_err(" fb fence sync get fd error : %d \n", fd);
				break;
			}

			mutex_lock(&ptccfb_info->output_lock);
		
			if (!ptccfb_info->output_on) {
				ptccfb_info->fb_timeline_max++;
				pt = sw_sync_pt_create(ptccfb_info->fb_timeline, ptccfb_info->fb_timeline_max);
				fence = sync_fence_create("display", pt);
				sync_fence_install(fence, fd);
				var_info.reserved[2] = fd;
				var_info.reserved[3] = 0xf;
				sw_sync_timeline_inc(ptccfb_info->fb_timeline, 1);
				printk("lcd display update on power off state \n");
		
				if (copy_to_user((struct fb_var_screeninfo __user *)arg,
						 &var_info,
						 sizeof(struct fb_var_screeninfo))) {
					ret = -EFAULT;
				}
				mutex_unlock(&ptccfb_info->output_lock);
				break;
			}

			regs = kzalloc(sizeof(struct tcc_fenc_reg_data), GFP_KERNEL);

			if (!regs) {
				pr_err("fb fence sync could not allocate \n");
				mutex_unlock(&ptccfb_info->output_lock);
				return -ENOMEM;
			}				

			mutex_lock(&ptccfb_info->fb_timeline_lock);
			regs->screen_cpu = ptccfb_info->screen_cpu;
			regs->screen_dma= ptccfb_info->screen_dma;
			regs->var = var_info;

			regs->fence_fd = var_info.reserved[2];
			if(regs->fence_fd > 0)
			{
				regs->fence = sync_fence_fdget(regs->fence_fd);
				if (!regs->fence ) {
					printk("failed to import fence fd\n");
				}
			}	
			list_add_tail(&regs->list, &ptccfb_info->fb_update_regs_list);

			ptccfb_info->fb_timeline_max++;
			pt = sw_sync_pt_create(ptccfb_info->fb_timeline, ptccfb_info->fb_timeline_max);
			fence = sync_fence_create("display", pt);
			sync_fence_install(fence, fd);
			var_info.reserved[2] = fd;
			var_info.reserved[3] = 0xf;
			mutex_unlock(&ptccfb_info->fb_timeline_lock);
				
			queue_kthread_work(&ptccfb_info->fb_update_regs_worker,
									&ptccfb_info->fb_update_regs_work);

			mutex_unlock(&ptccfb_info->output_lock);

			if(copy_to_user((struct fb_var_screeninfo __user *)arg, &var_info,	sizeof(struct fb_var_screeninfo))) {
				ret = -EFAULT;
				break;
			}
#endif
		}
		break;

		case TCC_LCDC_SET_OUTPUT_RESIZE_MODE:
			{
				tcc_display_resize resize_value;
				struct tcc_dp_device *pdp_data = NULL;

				if(copy_from_user((void *)&resize_value, (const void *)arg, sizeof(tcc_display_resize)))
					return -EFAULT;

				//printk("%s : TCC_LCDC_SET_OUTPUT_RESIZE_MODE, mode=%d\n", __func__, mode);

				tca_fb_resize_set_value(resize_value, TCC_OUTPUT_MAX);

				if((ptccfb_info->pdata.Mdp_data.DispDeviceType == TCC_OUTPUT_HDMI) 
					|| (ptccfb_info->pdata.Mdp_data.DispDeviceType == TCC_OUTPUT_COMPOSITE)  || (ptccfb_info->pdata.Mdp_data.DispDeviceType == TCC_OUTPUT_COMPONENT))
					pdp_data = &ptccfb_info->pdata.Mdp_data;
				else if((ptccfb_info->pdata.Sdp_data.DispDeviceType == TCC_OUTPUT_HDMI) 
					|| (ptccfb_info->pdata.Sdp_data.DispDeviceType == TCC_OUTPUT_COMPOSITE)  || (ptccfb_info->pdata.Sdp_data.DispDeviceType == TCC_OUTPUT_COMPONENT))
					pdp_data = &ptccfb_info->pdata.Sdp_data;
				//else
				//	pr_err("TCC_LCDC_SET_OUTPUT_RESIZE_MODE Can't find display device , Main:%d, Sub:%d\n", ptccfb_info->pdata.Mdp_data.DispDeviceType, ptccfb_info->pdata.Sdp_data.DispDeviceType);

				if(pdp_data != NULL) {
					if(pdp_data->FbPowerState)	{
						tca_fb_activate_var(pdp_data->FbBaseAddr, &ptccfb_info->fb->var, pdp_data);
					}
				}
			}
			break;

		case TCC_SECONDARY_OUTPUT_RESIZE_MODE_STB:
			{
				tcc_display_resize resize_value;
				#ifdef CONFIG_PRESENTATION_SECONDAY_DISPLAY_RESIZE_STB
				struct tcc_dp_device *pdp_data = NULL;
				#endif

				if(copy_from_user((void *)&resize_value, (const void *)arg, sizeof(tcc_display_resize)))
					return -EFAULT;

				#ifdef CONFIG_PRESENTATION_SECONDAY_DISPLAY_RESIZE_STB

				printk("TCC_SECONDARY_OUTPUT_RESIZE_MODE_STB, mode=left:%d, right:%d, up:%d, down:%d\n",resize_value.resize_left, resize_value.resize_right, resize_value.resize_up, resize_value.resize_down);

				tca_fb_resize_set_value(resize_value, TCC_OUTPUT_COMPONENT);
				
				if(ptccfb_info->pdata.Mdp_data.DispDeviceType == TCC_OUTPUT_COMPONENT)
					pdp_data = &ptccfb_info->pdata.Mdp_data;
				else if(ptccfb_info->pdata.Sdp_data.DispDeviceType == TCC_OUTPUT_COMPONENT)
					pdp_data = &ptccfb_info->pdata.Sdp_data;
				//else
				//	pr_err("TCC_LCDC_SET_OUTPUT_RESIZE_MODE Can't find display device , Main:%d, Sub:%d\n", ptccfb_info->pdata.Mdp_data.DispDeviceType, ptccfb_info->pdata.Sdp_data.DispDeviceType);

				if(pdp_data != NULL) {
					if(pdp_data->FbPowerState)	{
						tca_fb_activate_var(pdp_data->FbBaseAddr, &ptccfb_info->fb->var, pdp_data);
					}
				}
				#else
					pr_info("TCC_SECONDARY_OUTPUT_RESIZE_MODE_STB function Not Use \n");
				#endif//				
			}
			break;
		case TCC_LCDC_SET_OUTPUT_ATTACH_RESIZE_MODE:
			{
				tcc_display_resize resize_value;
				struct tcc_dp_device *pdp_data = NULL;

				pdp_data = &ptccfb_info->pdata.Mdp_data;

				if(copy_from_user((void *)&resize_value, (const void *)arg, sizeof(tcc_display_resize)))
					return -EFAULT;

				//dprintk("%s : TCC_LCDC_SET_OUTPUT_ATTACH_RESIZE_MODE, mode=%d\n", __func__, mode);

				tca_fb_output_attach_resize_set_value(resize_value);	
				if(ptccfb_info){
					tca_fb_attach_stop(ptccfb_info);
					tca_fb_attach_start(ptccfb_info);
				}

				
			}
			break;

		case TCC_LCDC_HDMI_START:
			{
				struct tcc_dp_device *pdp_data = NULL;

				if((ptccfb_info->pdata.Mdp_data.FbPowerState != true) || (ptccfb_info->pdata.Mdp_data.DispDeviceType ==TCC_OUTPUT_HDMI))
					pdp_data = &ptccfb_info->pdata.Mdp_data;
				else if((ptccfb_info->pdata.Sdp_data.FbPowerState != true) || (ptccfb_info->pdata.Sdp_data.DispDeviceType ==TCC_OUTPUT_HDMI))
					pdp_data = &ptccfb_info->pdata.Sdp_data;
				else
					pr_err("hdmi power on  : can't find HDMI voic display block \n");
		
				if(pdp_data)
				{
					if(pdp_data->FbPowerState == true)
					{
						pr_info("HDMI voic display block power off  \n");
						tca_vioc_displayblock_disable(pdp_data);
						tca_vioc_displayblock_powerOff(pdp_data);
					}

					pdp_data->DispDeviceType = TCC_OUTPUT_HDMI;
					tca_vioc_displayblock_powerOn(pdp_data);
				}
			}
			break;

		case TCC_LCDC_HDMI_TIMING:
			{
				struct tcc_dp_device *pdp_data = NULL;
				struct lcdc_timimg_parms_t lcdc_timing;

				dprintk(" TCC_LCDC_HDMI_TIMING: \n");

				if (copy_from_user((void*)&lcdc_timing, (const void*)arg, sizeof(struct lcdc_timimg_parms_t)))
					return -EFAULT;

				if(ptccfb_info->pdata.Mdp_data.DispDeviceType == TCC_OUTPUT_HDMI)
					pdp_data = &ptccfb_info->pdata.Mdp_data;
				else if(ptccfb_info->pdata.Sdp_data.DispDeviceType == TCC_OUTPUT_HDMI)
					pdp_data = &ptccfb_info->pdata.Sdp_data;
				else
					pr_err("hdmi timing setting : can't find HDMI voic display block \n");

				if(pdp_data != NULL) {
					tca_vioc_displayblock_timing_set(VIOC_OUTCFG_HDMI, pdp_data, &lcdc_timing);

					if(pdp_data->FbPowerState)	{
						unsigned int BaseAddr;

						#if defined(CONFIG_HWCOMPOSER_OVER_1_1_FOR_MID)
							BaseAddr = ptccfb_info->map_dma + ptccfb_info->fb->var.xres *  ptccfb_info->fb->var.yres_virtual * ( ptccfb_info->fb->var.bits_per_pixel/8);
							tca_fb_activate_var(BaseAddr, &ptccfb_info->fb->var, pdp_data);
						#else
						 	BaseAddr = ptccfb_info->map_dma + ptccfb_info->fb->var.xres *  ptccfb_info->fb->var.yoffset * ( ptccfb_info->fb->var.bits_per_pixel/8);
							if(pdp_data->FbBaseAddr != (unsigned int)0)
									tca_fb_activate_var(pdp_data->FbBaseAddr, &ptccfb_info->fb->var, pdp_data);
							else
									tca_fb_activate_var(BaseAddr, &ptccfb_info->fb->var, pdp_data);
						#endif//					
						
						#ifdef TCC_VIDEO_DISPLAY_BY_VSYNC_INT
						//Disable video path of main display.
						pdp_data = &ptccfb_info->pdata.Mdp_data;
						#ifdef CONFIG_ANDROID
						tcc_vsync_hdmi_start(pdp_data, &lcd_video_started);
						#else
						if(tcc_vsync_get_isVsyncRunning(0) || tcc_vsync_get_isVsyncRunning(1))
							tcc_vsync_hdmi_start(pdp_data, &lcd_video_started);
						#endif
						#endif
					}
				}
			}
			break;


		case TCC_LCDC_HDMI_END:
			{
				struct tcc_dp_device *pdp_data = NULL;

				if(ptccfb_info->pdata.Mdp_data.DispDeviceType == TCC_OUTPUT_HDMI)
					pdp_data = &ptccfb_info->pdata.Mdp_data;
				else if(ptccfb_info->pdata.Sdp_data.DispDeviceType == TCC_OUTPUT_HDMI)
					pdp_data = &ptccfb_info->pdata.Sdp_data;
				else
					pr_err("TCC_LCDC_HDMI_END : can't find HDMI voic display block \n");

				if(pdp_data != NULL){
					#ifdef CONFIG_VIDEO_TCC_VOUT
					if(tcc_vout_get_status() != TCC_VOUT_RUNNING)
					#endif
					{
						#ifdef TCC_VIDEO_DISPLAY_BY_VSYNC_INT
						#ifdef CONFIG_ANDROID
						tcc_vsync_hdmi_end(pdp_data);
						#else
						if(tcc_vsync_get_isVsyncRunning(0) || tcc_vsync_get_isVsyncRunning(1))
							tcc_vsync_hdmi_end(pdp_data);
						#endif
						#endif
					}

					tca_vioc_displayblock_disable(pdp_data);
					tca_vioc_displayblock_powerOff(pdp_data);
					pdp_data->DispDeviceType = TCC_OUTPUT_NONE;
				}

			}
			break;
			


		case TCC_LCDC_HDMI_DISPLAY:
			{
				struct tcc_lcdc_image_update ImageInfo;
				struct tcc_dp_device *pdp_data = NULL;
				if (copy_from_user((void *)&ImageInfo, (const void *)arg, sizeof(struct tcc_lcdc_image_update))){
					return -EFAULT;
				}

				if(ptccfb_info->pdata.Mdp_data.DispDeviceType == TCC_OUTPUT_HDMI)
					pdp_data = &ptccfb_info->pdata.Mdp_data;
				else	 if(ptccfb_info->pdata.Sdp_data.DispDeviceType == TCC_OUTPUT_HDMI)
					pdp_data = &ptccfb_info->pdata.Sdp_data;
				else
					pr_err("TCC_LCDC_HDMI_DISPLAY Can't find HDMI output , Main:%d, Sub :%d \n", ptccfb_info->pdata.Mdp_data.DispDeviceType, ptccfb_info->pdata.Sdp_data.DispDeviceType);
					
				if(pdp_data)	
				{
		#ifdef TCC_VIDEO_DISPLAY_BY_VSYNC_INT
					// this code prevent a error of transparent on layer0 after stopping v
					if(ImageInfo.Lcdc_layer == 0 && ImageInfo.enable == 0) {
						tcc_vsync_set_output_mode_all(-1);
					}
				#if defined(CONFIG_HDMI_DISPLAY_LASTFRAME)
					if(ImageInfo.enable != 0){
						tcc_vsync_set_output_mode_all(TCC_OUTPUT_HDMI);
						if(0 >= tcc_video_check_last_frame(&ImageInfo)){
							printk("----> skip 2 this frame for last-frame \n");
							return 0;
						}
					}
				#endif
		#endif
					
					if(pdp_data->FbPowerState)
						tca_scale_display_update(pdp_data, (struct tcc_lcdc_image_update *)&ImageInfo);
				}

				return 0;
				
			}
			break;


		case TCC_LCDC_HDMI_CHECK:
			{
				unsigned int ret_mode = 0;

				if(HDMI_pause || ((screen_width < screen_height)&& (!HDMI_video_mode)))
				{
					ret_mode = 1;
					dprintk("\n %d : %d %d  \n ", HDMI_pause, screen_width, screen_height);
				}

				put_user(ret_mode, (unsigned int __user*)arg);
			}
			break;

		case TCC_LCDC_HDMI_MODE_SET:
 			{
				TCC_HDMI_M uiHdmi;

				if(get_user(uiHdmi, (int __user *) arg))
					return -EFAULT;

				dprintk("%s: TCC_LCDC_HDMI_MODE_SET [%d] video_M:%d Output_SelectMode:%d   \n", __func__ , uiHdmi , HDMI_video_mode, Output_SelectMode);
				
				switch(uiHdmi)
				{
					case TCC_HDMI_SUSEPNED:
						HDMI_pause = 1;
						break;
					case TCC_HDMI_RESUME:
						HDMI_pause = 0;
						break;
					case TCC_HDMI_VIDEO_START:
						HDMI_video_mode = 1;
						break;
					case TCC_HDMI_VIDEO_END:
						HDMI_video_mode = 0;
						//TCC_OUTPUT_FB_CaptureVideoImg(0);
		#if defined(CONFIG_HDMI_DISPLAY_LASTFRAME) 
						tcc_video_clear_last_frame(pLastFrame_RDMABase,true);
		#endif
						break;
					default:
						break;
				}
 			}
			break;
			

		case TCC_LCDC_HDMI_GET_SIZE:
			{
				tcc_display_size HdmiSize;
				HdmiSize.width = HDMI_video_width;
				HdmiSize.height = HDMI_video_height;
  				HdmiSize.frame_hz= HDMI_video_hz;

				dprintk("%s: TCC_LCDC_HDMI_GET_SIZE -  HDMI_video_width:%d HDMI_video_height:%d   \n", __func__ , HDMI_video_width, HDMI_video_height);
				if (copy_to_user((tcc_display_size *)arg, &HdmiSize, sizeof(HdmiSize)))		{
					return -EFAULT;
				}
			}
			break;
		case TCC_LCDC_HDMI_SET_SIZE:
			{
				tcc_display_size HdmiSize;
				if (copy_from_user((void *)&HdmiSize, (const void *)arg, sizeof(tcc_display_size)))
					return -EFAULT;

				HDMI_video_width = HdmiSize.width;
				HDMI_video_height = HdmiSize.height;
				HDMI_video_hz = HdmiSize.frame_hz;

				if(HDMI_video_hz == 23)
					HDMI_video_hz = 24;
				else if(HDMI_video_hz == 29)
					HDMI_video_hz = 30;

				dprintk("%s: TCC_LCDC_HDMI_SET_SIZE -  HDMI_video_width:%d HDMI_video_height:%d   \n", __func__ , HDMI_video_width, HDMI_video_height);
			}
			break;


	case TCC_HDMI_FBIOPUT_VSCREENINFO:
		{
			unsigned int BaseAddr = 0;
			external_fbioput_vscreeninfo sc_info;
			struct fb_var_screeninfo var;
			struct tcc_dp_device *pdp_data = NULL;

			if(ptccfb_info->pdata.Mdp_data.DispDeviceType == TCC_OUTPUT_HDMI)
				pdp_data = &ptccfb_info->pdata.Mdp_data;
			else	 if(ptccfb_info->pdata.Sdp_data.DispDeviceType == TCC_OUTPUT_HDMI)
				pdp_data = &ptccfb_info->pdata.Sdp_data;
			else
				pr_err("TCC_LCDC_HDMI_DISPLAY Can't find HDMI output , Main:%d, Sub :%d \n", ptccfb_info->pdata.Mdp_data.DispDeviceType, ptccfb_info->pdata.Sdp_data.DispDeviceType);

					
			if((!ptccfb_info->pdata.Mdp_data.FbPowerState) || (pdp_data == NULL))
				return 0;

			if (copy_from_user((void*)&sc_info, (const void*)arg, sizeof(external_fbioput_vscreeninfo)))
				return -EFAULT;

			var.xres = sc_info.width;
			var.yres = sc_info.height;
			var.bits_per_pixel = sc_info.bits_per_pixel;
			var.yoffset = sc_info.offset;

		 	BaseAddr = ptccfb_info->map_dma + sc_info.offset;
			tca_fb_activate_var(BaseAddr,  &var, pdp_data);
			
			if(pdp_data->FbPowerState)
				tca_fb_vsync_activate(pdp_data);
		
			#if defined(CONFIG_SYNC_FB)
			mutex_lock(&ptccfb_info->ext_timeline_lock);
			{
				struct sync_fence *fence;
				struct sync_pt *pt;
				int fd;
				
				fd = get_unused_fd();

				if(fd< 0){
					pr_err(" fb fence sync get fd error : %d \n", fd);
					break;
				}

				ptccfb_info->ext_timeline_max++;
				pt = sw_sync_pt_create(ptccfb_info->ext_timeline, ptccfb_info->ext_timeline_max);
				fence = sync_fence_create("display_ext", pt);
				sync_fence_install(fence, fd);
				sc_info.fence_fd = fd;
				sw_sync_timeline_inc(ptccfb_info->ext_timeline, 1);

			}
//			queue_kthread_work(&ptccfb_info->ext_update_regs_worker,
//									&ptccfb_info->ext_update_regs_work);		
			mutex_unlock(&ptccfb_info->ext_timeline_lock);
			
			if (copy_to_user((external_fbioput_vscreeninfo *)arg, (void*)&sc_info, sizeof(external_fbioput_vscreeninfo)))
				return -EFAULT;
			
			#endif
		}
		break;

	case TCC_CVBS_FBIOPUT_VSCREENINFO:
		{
			unsigned int BaseAddr = 0;
			external_fbioput_vscreeninfo sc_info;
			struct fb_var_screeninfo var;
			struct tcc_dp_device *pdp_data;

			pdp_data = &ptccfb_info->pdata.Sdp_data;
			pdp_data->DispOrder = 1;

			if((!ptccfb_info->pdata.Mdp_data.FbPowerState))
				return 0;

			if (copy_from_user((void*)&sc_info, (const void*)arg, sizeof(external_fbioput_vscreeninfo)))
				return -EFAULT;

			var.xres = sc_info.width;
			var.yres = sc_info.height;
			var.bits_per_pixel = sc_info.bits_per_pixel;
			var.yoffset = sc_info.offset;

		 	BaseAddr = ptccfb_info->map_dma + sc_info.offset;
			//pr_err(" #####TCC_CVBS_FBIOPUT_VSCREENINFO\n");
			tca_fb_activate_var(BaseAddr,  &var, pdp_data);
			
			if(pdp_data->FbPowerState)
				tca_fb_vsync_activate(pdp_data);
		
			#if defined(CONFIG_SYNC_FB)
			mutex_lock(&ptccfb_info->ext_timeline_lock);
			{
				struct sync_fence *fence;
				struct sync_pt *pt;
				int fd;
				
				fd = get_unused_fd();

				if(fd< 0){
					pr_err(" fb fence sync get fd error : %d \n", fd);
					break;
				}

				ptccfb_info->ext_timeline_max++;
				pt = sw_sync_pt_create(ptccfb_info->ext_timeline, ptccfb_info->ext_timeline_max);
				fence = sync_fence_create("display_ext", pt);
				sync_fence_install(fence, fd);
				sc_info.fence_fd = fd;
				sw_sync_timeline_inc(ptccfb_info->ext_timeline, 1);

			}
//			queue_kthread_work(&ptccfb_info->ext_update_regs_worker,
//									&ptccfb_info->ext_update_regs_work);		
			mutex_unlock(&ptccfb_info->ext_timeline_lock);
			
			if (copy_to_user((external_fbioput_vscreeninfo *)arg, (void*)&sc_info, sizeof(external_fbioput_vscreeninfo)))
				return -EFAULT;
			
			#endif
		}
		break;

		case TCC_COMPONENT_FBIOPUT_VSCREENINFO:
			{
				unsigned int BaseAddr = 0;
				external_fbioput_vscreeninfo sc_info;
				struct fb_var_screeninfo var;
				struct tcc_dp_device *pdp_data;
	
				pdp_data = &ptccfb_info->pdata.Sdp_data;
				pdp_data->DispOrder = 1;

				if((!ptccfb_info->pdata.Mdp_data.FbPowerState))
					return 0;

				if (copy_from_user((void*)&sc_info, (const void*)arg, sizeof(external_fbioput_vscreeninfo)))
					return -EFAULT;
	
				var.xres = sc_info.width;
				var.yres = sc_info.height;
				var.bits_per_pixel = sc_info.bits_per_pixel;
				var.yoffset = sc_info.offset;
	
				BaseAddr = ptccfb_info->map_dma + sc_info.offset;
				//pr_err(" #####TCC_COMPONENT_FBIOPUT_VSCREENINFO\n");
			tca_fb_activate_var(BaseAddr,  &var, pdp_data);
			
			if(pdp_data->FbPowerState)
				tca_fb_vsync_activate(pdp_data);
		
			#if defined(CONFIG_SYNC_FB)
			mutex_lock(&ptccfb_info->ext_timeline_lock);
			{
				struct sync_fence *fence;
				struct sync_pt *pt;
				int fd;
				
				fd = get_unused_fd();

				if(fd< 0){
					pr_err(" fb fence sync get fd error : %d \n", fd);
					break;
				}

				ptccfb_info->ext_timeline_max++;
				pt = sw_sync_pt_create(ptccfb_info->ext_timeline, ptccfb_info->ext_timeline_max);
				fence = sync_fence_create("display_ext", pt);
				sync_fence_install(fence, fd);
				sc_info.fence_fd = fd;
				sw_sync_timeline_inc(ptccfb_info->ext_timeline, 1);

			}
//			queue_kthread_work(&ptccfb_info->ext_update_regs_worker,
//									&ptccfb_info->ext_update_regs_work);		
			mutex_unlock(&ptccfb_info->ext_timeline_lock);
			
			if (copy_to_user((external_fbioput_vscreeninfo *)arg, (void*)&sc_info, sizeof(external_fbioput_vscreeninfo)))
				return -EFAULT;
			
			#endif
		}
		break;

		
		case TCC_LCDC_COMPOSITE_CHECK:
			{
				unsigned int composite_detect = 1;

				#if defined(CONFIG_FB_TCC_COMPOSITE)
					composite_detect = tcc_composite_detect();
				#endif

				if (copy_to_user((void *)arg, &composite_detect, sizeof(unsigned int)))
					return -EFAULT;
			}
			break;
			
		case TCC_LCDC_COMPOSITE_MODE_SET:
			{
				struct tcc_dp_device *pdp_data = NULL;
				LCDC_COMPOSITE_MODE composite_mode;

				if(copy_from_user((void *)&composite_mode, (const void *)arg, sizeof(LCDC_COMPOSITE_MODE))){
					return -EFAULT;
				}

				if(composite_mode == LCDC_COMPOSITE_UI_MODE)
				{
					#if defined(CONFIG_HWCOMPOSER_OVER_1_1_FOR_STB)
						//Don't change output mode.
					#else
					Output_SelectMode = TCC_OUTPUT_COMPOSITE;
					#endif

 					printk("TCC_LCDC_COMPOSITE_MODE_SET : Output_SelectMode = %d , composite_mode = %d\n", Output_SelectMode, composite_mode);

					//TCC_OUTPUT_FB_ClearVideoImg();

					if(ptccfb_info->pdata.Mdp_data.DispDeviceType == TCC_OUTPUT_COMPOSITE)
						pdp_data = &ptccfb_info->pdata.Mdp_data;
					else if(ptccfb_info->pdata.Sdp_data.DispDeviceType == TCC_OUTPUT_COMPOSITE)
						pdp_data = &ptccfb_info->pdata.Sdp_data;

					if(pdp_data != NULL) {
						if(pdp_data->FbPowerState) {
							tca_fb_activate_var(pdp_data->FbBaseAddr, &ptccfb_info->fb->var, pdp_data);
						}
					}

					//TCC_OUTPUT_FB_MouseShow(1, TCC_OUTPUT_COMPOSITE);

					#ifdef TCC_VIDEO_DISPLAY_BY_VSYNC_INT
						tca_vsync_video_display_enable();

						#if defined(CONFIG_TCC_DISPLAY_MODE_USE)|| defined(CONFIG_TCC_DISPLAY_LCD_CVBS)
							tcc_vsync_set_firstFrameFlag_all(1);
						#endif

						tcc_vsync_set_output_mode_all(-1);
					#endif
				}
				else if(composite_mode == LCDC_COMPOSITE_NONE_MODE)
				{
					Output_SelectMode = TCC_OUTPUT_NONE;

					#ifdef TCC_VIDEO_DISPLAY_BY_VSYNC_INT
						tca_vsync_video_display_disable();

						#if !defined(CONFIG_TCC_DISPLAY_MODE_USE)|| defined(CONFIG_TCC_DISPLAY_LCD_CVBS)
							tcc_vsync_set_firstFrameFlag_all(1);
						#endif
					#endif

					#ifdef TCC_VIDEO_DISPLAY_DEINTERLACE_MODE
						if(is_deinterlace_enabled(0))
							TCC_VIQE_DI_DeInit60Hz();
					#endif
				}
			}
			break;

		case TCC_LCDC_COMPONENT_CHECK:
			{
				unsigned int component_detect;

				#if defined(CONFIG_FB_TCC_COMPONENT)
					component_detect = tcc_component_detect();
				#endif

				if (copy_to_user((void *)arg, &component_detect, sizeof(unsigned int)))
					return -EFAULT;
			}
			break;
			
		case TCC_LCDC_COMPONENT_MODE_SET:
			{
				struct tcc_dp_device *pdp_data = NULL;
				LCDC_COMPONENT_MODE component_mode;

				if(copy_from_user((void *)&component_mode, (const void *)arg, sizeof(LCDC_COMPONENT_MODE))){
					return -EFAULT;
				}

				if(component_mode == LCDC_COMPONENT_UI_MODE)
				{
				
				#if defined(CONFIG_HWCOMPOSER_OVER_1_1_FOR_STB)
					//Don't change output mode.
				#else
					Output_SelectMode = TCC_OUTPUT_COMPONENT;
				#endif

 					printk("TCC_LCDC_COMPONENT_MODE_SET : Output_SelectMode = %d , component_mode = %d\n", Output_SelectMode, component_mode);

					//TCC_OUTPUT_FB_ClearVideoImg();

					if(ptccfb_info->pdata.Mdp_data.DispDeviceType == TCC_OUTPUT_COMPONENT)
						pdp_data = &ptccfb_info->pdata.Mdp_data;
					else if(ptccfb_info->pdata.Sdp_data.DispDeviceType == TCC_OUTPUT_COMPONENT)
						pdp_data = &ptccfb_info->pdata.Sdp_data;

					if(pdp_data != NULL) {
						if(pdp_data->FbPowerState) {
							tca_fb_activate_var(pdp_data->FbBaseAddr, &ptccfb_info->fb->var, pdp_data);
						}
					}

					//TCC_OUTPUT_FB_MouseShow(1, TCC_OUTPUT_COMPONENT);

					#ifdef TCC_VIDEO_DISPLAY_BY_VSYNC_INT
						tca_vsync_video_display_enable();

						#if defined(CONFIG_TCC_DISPLAY_MODE_USE)
							tcc_vsync_set_firstFrameFlag_all(1);
						#endif

						tcc_vsync_set_output_mode_all(-1);
					#endif
				}
				else if(component_mode == LCDC_COMPONENT_NONE_MODE)
				{
					Output_SelectMode = TCC_OUTPUT_NONE;

					#ifdef TCC_VIDEO_DISPLAY_BY_VSYNC_INT
						tca_vsync_video_display_disable();

						#if !defined(CONFIG_TCC_DISPLAY_MODE_USE)
							tcc_vsync_set_firstFrameFlag_all(1);
						#endif
					#endif

					#ifdef TCC_VIDEO_DISPLAY_DEINTERLACE_MODE
						if(is_deinterlace_enabled(0))
							TCC_VIQE_DI_DeInit60Hz();
					#endif
				}
			}
			break;

		case TCC_LCDC_FBCHANNEL_ONOFF:
			{
				unsigned int onOff;
				struct tcc_dp_device *pdp_data = NULL;
			
				if(copy_from_user((void *)&onOff, (const void *)arg, sizeof(unsigned int)))
					return -EFAULT;

				if((ptccfb_info->pdata.Mdp_data.DispDeviceType == TCC_OUTPUT_HDMI) 
					|| (ptccfb_info->pdata.Mdp_data.DispDeviceType == TCC_OUTPUT_COMPOSITE)  || (ptccfb_info->pdata.Mdp_data.DispDeviceType == TCC_OUTPUT_COMPONENT) )
					pdp_data = &ptccfb_info->pdata.Mdp_data;
				else	 if((ptccfb_info->pdata.Sdp_data.DispDeviceType == TCC_OUTPUT_HDMI) 
					|| (ptccfb_info->pdata.Sdp_data.DispDeviceType == TCC_OUTPUT_COMPOSITE)  || (ptccfb_info->pdata.Sdp_data.DispDeviceType == TCC_OUTPUT_COMPONENT) )
					pdp_data = &ptccfb_info->pdata.Sdp_data;

				#if defined(CONFIG_MACH_TCC8930ST)
					#if !defined(CONFIG_TCC_DISPLAY_MODE_USE)
						if(pdp_data != NULL)
						{
							printk("TCC_LCDC_FBCHANNEL_ONOFF: onOff=%d, addr=0x%08x\n", onOff, pdp_data->rdma_info[RDMA_FB].virt_addr);

							if(onOff)
                				VIOC_RDMA_SetImageEnable(pdp_data->rdma_info[RDMA_FB].virt_addr);
							else
                				VIOC_RDMA_SetImageDisable(pdp_data->rdma_info[RDMA_FB].virt_addr);
						}
					#endif
				#endif
			}
			break;
			
		case TCC_LCDC_MOUSE_SHOW:
			{
				unsigned int enable;
				struct tcc_dp_device *pdp_data = NULL;

				if((ptccfb_info->pdata.Mdp_data.DispDeviceType == TCC_OUTPUT_HDMI) 
					|| (ptccfb_info->pdata.Mdp_data.DispDeviceType == TCC_OUTPUT_COMPOSITE)  || (ptccfb_info->pdata.Mdp_data.DispDeviceType == TCC_OUTPUT_COMPONENT) )
					pdp_data = &ptccfb_info->pdata.Mdp_data;
				else	 if((ptccfb_info->pdata.Sdp_data.DispDeviceType == TCC_OUTPUT_HDMI) 
					|| (ptccfb_info->pdata.Sdp_data.DispDeviceType == TCC_OUTPUT_COMPOSITE)  || (ptccfb_info->pdata.Sdp_data.DispDeviceType == TCC_OUTPUT_COMPONENT) )
					pdp_data = &ptccfb_info->pdata.Sdp_data;
				else
					pr_err("TCC_LCDC_MOUSE_SHOW Can't find output , Main:%d, Sub :%d \n", ptccfb_info->pdata.Mdp_data.DispDeviceType, ptccfb_info->pdata.Sdp_data.DispDeviceType);
				
				if(copy_from_user((void *)&enable, (const void *)arg, sizeof(unsigned int)))
					return -EFAULT;

				if(pdp_data != NULL)
				{
					if(enable == 0)
						VIOC_RDMA_SetImageDisable(pdp_data->rdma_info[RDMA_MOUSE].virt_addr);
				}
			}
			break;
		case TCC_LCDC_MOUSE_MOVE:
			{
				tcc_mouse mouse;
				struct tcc_dp_device *pdp_data = NULL;

				if (copy_from_user((void *)&mouse, (const void *)arg, sizeof(tcc_mouse)))
					return -EFAULT;

				if((ptccfb_info->pdata.Mdp_data.DispDeviceType == TCC_OUTPUT_HDMI) 
					|| (ptccfb_info->pdata.Mdp_data.DispDeviceType == TCC_OUTPUT_COMPOSITE)  || (ptccfb_info->pdata.Mdp_data.DispDeviceType == TCC_OUTPUT_COMPONENT) )
					pdp_data = &ptccfb_info->pdata.Mdp_data;
				else	 if((ptccfb_info->pdata.Sdp_data.DispDeviceType == TCC_OUTPUT_HDMI) 
					|| (ptccfb_info->pdata.Sdp_data.DispDeviceType == TCC_OUTPUT_COMPOSITE)  || (ptccfb_info->pdata.Sdp_data.DispDeviceType == TCC_OUTPUT_COMPONENT) )
					pdp_data = &ptccfb_info->pdata.Sdp_data;
				else
					pr_err("TCC_LCDC_MOUSE_MOVE Can't find  output , Main:%d, Sub :%d \n", ptccfb_info->pdata.Mdp_data.DispDeviceType, ptccfb_info->pdata.Sdp_data.DispDeviceType);

				if(pdp_data != NULL)
					tca_fb_mouse_move(ptccfb_info->fb->var.xres, ptccfb_info->fb->var.yres, &mouse, pdp_data);
			}
			break;
		case TCC_LCDC_MOUSE_ICON:
			{
				tcc_mouse_icon mouse_icon;
				if (copy_from_user((void *)&mouse_icon, (const void *)arg, sizeof(tcc_mouse_icon)))
					return -EFAULT;
				tca_fb_mouse_set_icon(&mouse_icon);
			}
			break;

		case TCC_LCDC_3D_UI_ENABLE:
			{
				unsigned int mode;
				struct tcc_dp_device *pdp_data = NULL;

				if(copy_from_user((void *)&mode, (const void *)arg, sizeof(unsigned int)))
					return -EFAULT;

				if((ptccfb_info->pdata.Mdp_data.DispDeviceType == TCC_OUTPUT_HDMI) 
					|| (ptccfb_info->pdata.Mdp_data.DispDeviceType == TCC_OUTPUT_COMPOSITE)  || (ptccfb_info->pdata.Mdp_data.DispDeviceType == TCC_OUTPUT_COMPONENT) )
					pdp_data = &ptccfb_info->pdata.Mdp_data;
				else if((ptccfb_info->pdata.Sdp_data.DispDeviceType == TCC_OUTPUT_HDMI) 
					|| (ptccfb_info->pdata.Sdp_data.DispDeviceType == TCC_OUTPUT_COMPOSITE)  || (ptccfb_info->pdata.Sdp_data.DispDeviceType == TCC_OUTPUT_COMPONENT) )
					pdp_data = &ptccfb_info->pdata.Sdp_data;
				else
					pr_err("TCC_LCDC_MOUSE_MOVE Can't find  output , Main:%d, Sub :%d \n", ptccfb_info->pdata.Mdp_data.DispDeviceType, ptccfb_info->pdata.Sdp_data.DispDeviceType);

				if(pdp_data != NULL)
					tca_fb_divide_set_mode(pdp_data, true, mode);
			}
			break;
			
		case TCC_LCDC_3D_UI_DISABLE:
			{
				struct tcc_dp_device *pdp_data = NULL;
				
				if((ptccfb_info->pdata.Mdp_data.DispDeviceType == TCC_OUTPUT_HDMI) 
					|| (ptccfb_info->pdata.Mdp_data.DispDeviceType == TCC_OUTPUT_COMPOSITE)  || (ptccfb_info->pdata.Mdp_data.DispDeviceType == TCC_OUTPUT_COMPONENT) )
					pdp_data = &ptccfb_info->pdata.Mdp_data;
				else if((ptccfb_info->pdata.Sdp_data.DispDeviceType == TCC_OUTPUT_HDMI) 
					|| (ptccfb_info->pdata.Sdp_data.DispDeviceType == TCC_OUTPUT_COMPOSITE)  || (ptccfb_info->pdata.Sdp_data.DispDeviceType == TCC_OUTPUT_COMPONENT) )
					pdp_data = &ptccfb_info->pdata.Sdp_data;
				else
					pr_err("TCC_LCDC_MOUSE_MOVE Can't find  output , Main:%d, Sub :%d \n", ptccfb_info->pdata.Mdp_data.DispDeviceType, ptccfb_info->pdata.Sdp_data.DispDeviceType);

				if(pdp_data != NULL)
					tca_fb_divide_set_mode(pdp_data, false, 0);
			}
			break;
			
			
			
		case TCC_LCDC_GET_DISPLAY_TYPE:
			{
				unsigned int display_type = 0; /* Support HDMI/CVBS/COMPONENT output */

                            #if defined(CONFIG_FB_TCC_COMPOSITE) && defined(CONFIG_FB_TCC_COMPONENT)
                                    display_type = 0; /* Support HDMI/CVBS/COMPONENT output */
                            #endif

                            #if defined(CONFIG_FB_TCC_COMPOSITE) && !defined(CONFIG_FB_TCC_COMPONENT)
                                    display_type = 1; /* Support HDMI/CVBS output */
                            #endif

                            #if !defined(CONFIG_FB_TCC_COMPOSITE) && !defined(CONFIG_FB_TCC_COMPONENT)
                                    display_type = 2; /* Support HDMI output */
                            #endif

				if (copy_to_user((void *)arg, &display_type, sizeof(unsigned int)))
					return -EFAULT;

				//TCC_OUTPUT_FB_DetachOutput(1);
			}
			break;
		case TCC_LCDC_DISPLAY_END:
			{
				struct tcc_lcdc_image_update ImageInfo;
				pr_info(" TCC_LCDC_DISPLAY_END lcd_video_started %d\n", lcd_video_started);

				memset(&ImageInfo, 0x00, sizeof(struct tcc_lcdc_image_update));

				if(info->node == 0)				{
					if(!lcd_video_started)
						return 0;

					ImageInfo.Lcdc_layer = RDMA_VIDEO;
					lcd_video_started = 0;
				}
				else if(info->node == 1)	 {
					ImageInfo.Lcdc_layer = RDMA_FB1;
				}
				else {
					pr_err("ERR fb ioctl : TCC_LCDC_DISPLAY_END can not find fb %d node \n", info->node);
					return -1;
				 }

				ImageInfo.enable = 0;
				tca_scale_display_update(&ptccfb_info->pdata.Mdp_data, (struct tcc_lcdc_image_update *)&ImageInfo);

				lcd_video_started = 0;
			}
			break;

		case TCC_LCDC_DISPLAY_UPDATE:
			{
				struct tcc_lcdc_image_update ImageInfo;
				
				if (copy_from_user((void *)&ImageInfo, (const void *)arg, sizeof(struct tcc_lcdc_image_update))){
					return -EFAULT;
				}

				tca_scale_display_update(&ptccfb_info->pdata.Mdp_data, (struct tcc_lcdc_image_update *)&ImageInfo);
				lcd_video_started = 1;
			}
			break;

#if defined(CONFIG_USE_DISPLAY_FB_LOCK)
			case TCC_FB_UPDATE_LOCK:
			{
				int fblock = 0;
				if (copy_from_user(&fblock, (const void*)arg, sizeof(int))) {
					return -EFAULT;
				}
				fb_lock = fblock;
				printk("!!!!!!!! ioctl: fb_lock - [%d]\n", fb_lock);
			}
			break;
#endif
		case FBIO_DISABLE:
			{
				unsigned int type;
				struct tcc_dp_device *pdp_data = NULL;

				if(fb_lock)
				{
					printk("FBDIO_DISABLE is skipped: fb_lock - [%d]\n", fb_lock);
					return -EFAULT;
				}

				if (copy_from_user((void *)&type, (const void *)arg, sizeof(unsigned int ))) {
					return -EFAULT;
				}

				if(type == DD_MAIN)
					pdp_data = &ptccfb_info->pdata.Mdp_data;
				else
					pdp_data = &ptccfb_info->pdata.Sdp_data;
				#if defined(CONFIG_SYNC_FB)
				if(pdp_data)
				{

					if(pdp_data->rdma_info[RDMA_FB].virt_addr)
					{
						spin_lock_irq(&ptccfb_info->spin_lockDisp);
			#if 1
						VIOC_RDMA_SetImageAlphaEnable(pdp_data->rdma_info[RDMA_FB].virt_addr, 1);
						VIOC_RDMA_SetImageAlphaSelect(pdp_data->rdma_info[RDMA_FB].virt_addr, 0);
						VIOC_RDMA_SetImageAlpha(pdp_data->rdma_info[RDMA_FB].virt_addr, 0x0, 0x0);
			#else //
						{
							int sc_num = 0;
							volatile void __iomem *pscale_addr = NULL;
							sc_num = VIOC_CONFIG_GetScaler_PluginToRDMA(pdp_data->rdma_info[RDMA_FB].blk_num);
							if(sc_num > 0)
								pscale_addr = VIOC_SC_GetAddress(sc_num);

							VIOC_RDMA_SetImageSize(pdp_data->rdma_info[RDMA_FB].virt_addr, 0, 0);

							if(pscale_addr){
								VIOC_SC_SetBypass(pscale_addr, 1);
								VIOC_SC_SetUpdate(pscale_addr);
							}
						}
			#endif//
						VIOC_RDMA_SetImageUpdate(pdp_data->rdma_info[RDMA_FB].virt_addr);
						spin_unlock_irq(&ptccfb_info->spin_lockDisp);
					}
				}
				#endif//
			}
			break;


// VSYNC PART
	case TCC_LCDC_REFER_VSYNC_ENABLE:
		tca_vsync_enable(ptccfb_info, 1);
		break;

	case TCC_LCDC_REFER_VSYNC_DISABLE:
		tca_vsync_enable(ptccfb_info, 0);
		break;

	case FBIO_WAITFORVSYNC:
		tca_fb_wait_for_vsync(&ptccfb_info->pdata.Mdp_data);
		break;



// 2D Compression PART
	case GET_2D_COMPRESSION_FB_INFO:
		{
			pmap_t pmap_fb_video;

			if(info->node == 0)
				pmap_get_info("fb_video", &pmap_fb_video);
			else
				pmap_get_info("fb_video1", &pmap_fb_video);

			if (copy_to_user((void*)arg, &(pmap_fb_video.base), sizeof(unsigned int)*2))
				return -EFAULT;
		}
		break;

	case CHECK_2D_COMPRESSION_EN:
		{
			unsigned int ts_address;
			unsigned int bMark_ffff = 0;
			if (copy_from_user((void *)&ts_address, (const void *)arg, sizeof(unsigned int))){
				return -EFAULT;
			}
#if defined(CONFIG_SUPPORT_2D_COMPRESSION)
			if( 0 == tcc_2d_compression_read() )
				bMark_ffff = 1;

			if( bMark_ffff && CONFIG_ADDITIONAL_TS_BUFFER_LINES != 0 )
			{
				unsigned int *remap_addr;

				remap_addr = (unsigned int *)ioremap_nocache(ts_address, 4096);
				if(remap_addr != NULL){
					remap_addr[0] = 0xffffffff;
					iounmap((void*)remap_addr);
				}
				else{
					printk("CHECK_2D_COMPRESSION_EN :: ioremap error for 0x%x \n", ts_address);
					return -EFAULT;
				}
			}
#else
			bMark_ffff = 1;
#endif
			if( bMark_ffff )
				ts_address = 0xffffffff;

			if (copy_to_user((void*)arg, &(ts_address), sizeof(unsigned int))){
				return -EFAULT;
			}
		}
		break;
	default:
		dprintk("ioctl: Unknown [%d/0x%X]", cmd, cmd);
		break;
	}


	return 0;
}

static void schedule_palette_update(struct tccfb_info *fbi,
				    unsigned int regno, unsigned int val)
{
	unsigned long flags;

	local_irq_save(flags);

	local_irq_restore(flags);
}

/* from pxafb.c */
static inline unsigned int chan_to_field(unsigned int chan, struct fb_bitfield *bf)
{
	chan &= 0xffff;
	chan >>= 16 - bf->length;
	return chan << bf->offset;
}

static int tccfb_setcolreg(unsigned regno,
			       unsigned red, unsigned green, unsigned blue,
			       unsigned transp, struct fb_info *info)
{
	struct tccfb_info *fbi = info->par;
	unsigned int val;

	/* dprintk("setcol: regno=%d, rgb=%d,%d,%d\n", regno, red, green, blue); */

	switch (fbi->fb->fix.visual) {
		case FB_VISUAL_TRUECOLOR:
			/* true-colour, use pseuo-palette */

			if (regno < 16) {
				u32 *pal = fbi->fb->pseudo_palette;

				val  = chan_to_field(red,   &fbi->fb->var.red);
				val |= chan_to_field(green, &fbi->fb->var.green);
				val |= chan_to_field(blue,  &fbi->fb->var.blue);

				pal[regno] = val;
			}
			break;

		case FB_VISUAL_PSEUDOCOLOR:
			if (regno < 256) {
				/* currently assume RGB 5-6-5 mode */

				val  = ((red   >>  0) & 0xf800);
				val |= ((green >>  5) & 0x07e0);
				val |= ((blue  >> 11) & 0x001f);

				//writel(val, S3C2410_TFTPAL(regno));
				schedule_palette_update(fbi, regno, val);
			}
			break;

		default:
			return 1;   /* unknown type */
	}

	return 0;
}


/**
 *      tccfb_blank
 *	@blank_mode: the blank mode we want.
 *	@info: frame buffer structure that represents a single frame buffer
 *
 *	Blank the screen if blank_mode != 0, else unblank. Return 0 if
 *	blanking succeeded, != 0 if un-/blanking failed due to e.g. a
 *	video mode which doesn't support it. Implements VESA suspend
 *	and powerdown modes on hardware that supports disabling hsync/vsync:
 *	blank_mode == 2: suspend vsync
 *	blank_mode == 3: suspend hsync
 *	blank_mode == 4: powerdown
 *
 *	Returns negative errno on error, or zero on success.
 *
 */
#ifdef CONFIG_PM
static void touchscreen_enable(void)
{
		extern int tcc_ts_enabled;
#if defined(CONFIG_TOUCHSCREEN_TCCTS)
		if(tcc_ts_enabled == 1)
		{
				extern void tcc_ts_enable(void);
				tcc_ts_enable();
		}
#endif
#if defined(CONFIG_TOUCHSCREEN_TCC_GT813)
		if(tcc_ts_enabled == 2)
		{
				extern void tcc_ts_gt813_enable(void);
				tcc_ts_gt813_enable();
		}
#endif
#if defined(CONFIG_TOUCHSCREEN_ILITEK_AIMVF)
		if(tcc_ts_enabled == 3)
    {
		  	extern void tcc_ts_ilitek_enable(void);
		  	tcc_ts_ilitek_enable();
    }
#endif
#if defined(CONFIG_TOUCHSCREEN_VTL_CT36X)
		if(tcc_ts_enabled == 4)
    {
       	extern void tcc_ts_ct36x_enable(void);
       	tcc_ts_ct36x_enable();
    }
#endif
#if defined(CONFIG_TOUCHSCREEN_TCC_SSD253X)
    {
        if(tcc_ts_enabled == 5)
        {
          extern void tcc_ts_ssd253x_enable(void);
          tcc_ts_ssd253x_enable();
        }
    }
#endif

	printk("touchscreen_enable done \n");
}

static void touchscreen_disable(void)
{
		extern int tcc_ts_enabled;
#if defined(CONFIG_TOUCHSCREEN_TCCTS)
		if(tcc_ts_enabled == 1)
		{
				extern void tcc_ts_disable(void);
				tcc_ts_disable();
		}
#endif
#if defined(CONFIG_TOUCHSCREEN_TCC_GT813)
		if(tcc_ts_enabled == 2)
		{	
				extern void tcc_ts_gt813_disable(void);
				tcc_ts_gt813_disable();
		}
#endif
#if defined(CONFIG_TOUCHSCREEN_ILITEK_AIMVF)
		if(tcc_ts_enabled == 3)
		{
		  	extern void tcc_ts_ilitek_disable(void);
		  	tcc_ts_ilitek_disable();
		}
#endif
#if defined(CONFIG_TOUCHSCREEN_VTL_CT36X)
		if(tcc_ts_enabled == 4)
    {
        extern void tcc_ts_ct36x_disable(void);
        tcc_ts_ct36x_disable();
    }
#endif
#if defined(CONFIG_TOUCHSCREEN_TCC_SSD253X)
    {
        if(tcc_ts_enabled == 5)
        {
          extern void tcc_ts_ssd253x_disable(void);
          tcc_ts_ssd253x_disable();
        }
    }
#endif
}
#endif

#ifdef CONFIG_PM_RUNTIME
static int tcc_fb_enable(struct fb_info *info)
{		
	int ret = 0;
	struct tccfb_info *fbi =(struct tccfb_info *) info->par;		

	mutex_lock(&fbi->output_lock);
	if(fbi->output_on) {
		ret = -EBUSY;
		goto err;
	}
	
	fbi->output_on = true;
	pm_runtime_get_sync(fbi->dev);

err:
	mutex_unlock(&fbi->output_lock);
	return ret;
}

static int tcc_fb_disable(struct fb_info *info)
{
	int ret = 0;
	struct tccfb_info *fbi =(struct tccfb_info *) info->par;

	mutex_lock(&fbi->output_lock);

#if defined(CONFIG_SYNC_FB)	
	flush_kthread_worker(&fbi->fb_update_regs_worker);
	flush_kthread_worker(&fbi->ext_update_regs_worker);
#endif//
	if(!fbi->output_on) {
		ret = -EBUSY;
		goto err;
	}
	fbi->output_on = false;
	pm_runtime_put_sync(fbi->dev);	
	
err:	
	mutex_unlock(&fbi->output_lock);
	return ret;
}
#endif



static int tccfb_blank(int blank_mode, struct fb_info *info)
{
#ifdef CONFIG_PM_RUNTIME
	int ret = 0;

	struct tccfb_info *fbi =(struct tccfb_info *) info->par;
	#if defined(CONFIG_MALI400_UMP)
	return 0; // For mali400 X display drivers
	#endif

	// not used
	return 0;

	pm_runtime_get_sync(fbi->dev);

	switch(blank_mode)
	{
		case FB_BLANK_POWERDOWN:
		case FB_BLANK_NORMAL:
			touchscreen_disable();
			tcc_fb_disable(info);
			break;
		case FB_BLANK_UNBLANK:
			tcc_fb_enable(info);
			touchscreen_enable();
			break;
		case FB_BLANK_HSYNC_SUSPEND:
		case FB_BLANK_VSYNC_SUSPEND:
		default:
			ret = -EINVAL;
	}
	pm_runtime_put_sync(fbi->dev);

#endif
	printk(KERN_INFO "%s\n", __func__);

	return 0;
}

#ifdef CONFIG_DMA_SHARED_BUFFER
static struct sg_table *fb_ion_map_dma_buf(struct dma_buf_attachment *attachment,
					enum dma_data_direction direction)
{
	struct sg_table *table;
	struct fb_info *info = (struct fb_info *)attachment->dmabuf->priv;
	int err;

	table = kzalloc(sizeof(struct sg_table), GFP_KERNEL);
	if (table == NULL) {
		pr_err("%s: could not allocate table\n", __func__);
		return NULL;
	}

	err = sg_alloc_table(table, 1, GFP_KERNEL);
	if (err) {
		kfree(table);
		return ERR_PTR(err);
	}

	sg_set_page(table->sgl, NULL, info->fix.smem_len, 0);
	sg_dma_address(table->sgl) = info->fix.smem_start;
	debug_dma_map_sg(NULL, table->sgl, table->nents, table->nents, DMA_BIDIRECTIONAL);

	return table;
}

static void fb_ion_unmap_dma_buf(struct dma_buf_attachment *attachment,
			      struct sg_table *table,
			      enum dma_data_direction direction)
{
	debug_dma_unmap_sg(NULL, table->sgl, table->nents, DMA_BIDIRECTIONAL);
	sg_free_table(table);

	kfree(table);
}

static int fb_ion_mmap(struct dma_buf *dmabuf, struct vm_area_struct *vma)
{
	return -ENOSYS;
}

static void fb_ion_dma_buf_release(struct dma_buf *dmabuf)
{
	return;
}

static void *fb_ion_dma_buf_kmap(struct dma_buf *dmabuf, unsigned long offset)
{
	return 0;
}


struct dma_buf_ops fb_dma_buf_ops = {
	.map_dma_buf = fb_ion_map_dma_buf,
	.unmap_dma_buf = fb_ion_unmap_dma_buf,
	.mmap = fb_ion_mmap,
	.release = fb_ion_dma_buf_release,
	.kmap_atomic = fb_ion_dma_buf_kmap,
	.kmap = fb_ion_dma_buf_kmap,
};

struct dma_buf* tccfb_dmabuf_export(struct fb_info *info)
{
	struct dma_buf *buf = dma_buf_export(info, &fb_dma_buf_ops, info->fix.smem_len, O_RDWR, NULL);
	return buf;
}
#endif



#ifdef CONFIG_PM
/* suspend and resume support for the lcd controller */
static int tccfb_suspend(struct device *dev)
{

	tca_fb_suspend(dev);
	return 0;
}

static int tccfb_resume(struct device *dev)
{
	tca_fb_resume(dev);
	return 0;
}

static void tccfb_shutdown(struct platform_device *dev)
{
        printk(" %s \n",__func__);
        tccfb_suspend(&dev->dev);
}

static int tccfb_freeze(struct device *dev)
{
	/* Touch Screen Disable */
	touchscreen_disable();

	//It used to suspend when creating Quickboot Image
	tca_fb_suspend(dev);
	printk(" %s \n",__func__);

	return 0;
}

static int tccfb_thaw(struct device *dev)
{
	//After you create a QuickBoot Image, before restarting
	tca_fb_resume(dev);
	printk(" %s \n",__func__);
	  
	/* Touch Screen Enable */
	touchscreen_enable();

	return 0;
}

static int tccfb_restore(struct device *dev)
{
	// Used for Loading after resume to Quickboot Image
	tca_fb_resume(dev);
	printk(" %s \n",__func__);    

	/* Touch Screen Enable */
	touchscreen_enable();

	return 0;
}
#endif

static struct fb_ops tccfb_ops = {
	.owner			= THIS_MODULE,
	.fb_check_var	= tccfb_check_var,
	.fb_set_par		= tccfb_set_par,
	.fb_blank		= tccfb_blank,
	.fb_setcolreg	= tccfb_setcolreg,
	.fb_fillrect	= cfb_fillrect,
	.fb_copyarea	= cfb_copyarea,
	.fb_imageblit	= cfb_imageblit,
	.fb_ioctl		= tccfb_ioctl,
	.fb_pan_display = tccfb_pan_display,
#ifdef CONFIG_DMA_SHARED_BUFFER
	.fb_dmabuf_export = tccfb_dmabuf_export,
#endif	
};


/*
 * tccfb_map_video_memory():
 *	Allocates the DRAM memory for the frame buffer.  This buffer is
 *	remapped into a non-cached, non-buffered, memory region to
 *	allow palette and pixel writes to occur without flushing the
 *	cache.  Once this area is remapped, all virtual memory
 *	access to the video memory should occur at the new region.
 */

static int __init tccfb_map_video_memory(struct tccfb_info *fbi, int plane)
{
	pmap_t pmap_fb_video;

	switch (plane) {
	case 0:
		pmap_get_info("fb_video", &pmap_fb_video);
		break;
	case 1:
		pmap_get_info("fb1_video", &pmap_fb_video);
		break;
	default:
		pr_err("%s: Invalid plane(%d)\n", __func__, plane);
		return -EFAULT;
	}

	if(pmap_fb_video.size == 0)
	{
		fbi->map_size  = fbi->fb->var.xres_virtual * fbi->fb->var.yres_virtual * (fbi->fb->var.bits_per_pixel/ 8);
		fbi->map_cpu = dma_alloc_writecombine(fbi->dev, fbi->map_size, &fbi->map_dma, GFP_KERNEL);
		printk("map_video_memory (fbi=%p) kernel memory, dma:0x%x map_size:%08x\n", fbi,fbi->map_dma, fbi->map_size);
	}
	else
	{
		fbi->map_dma =  pmap_fb_video.base;
		fbi->map_size = pmap_fb_video.size;
		fbi->map_cpu = ioremap_nocache(fbi->map_dma, fbi->map_size);
		printk("plane: %d map_video_memory (fbi=%p) used map memory, map dma:0x%x cpu:%p size:%08x\n", plane, fbi, fbi->map_dma, fbi->map_cpu, fbi->map_size);
	}

	if (fbi->map_cpu) {
		/* prevent initial garbage on screen */
		dprintk("map_video_memory: clear %p:%08x\n", fbi->map_cpu, fbi->map_size);

		memset(fbi->map_cpu, 0x00, fbi->map_size);

		fbi->screen_dma		= fbi->map_dma;
		fbi->fb->screen_base	= fbi->map_cpu;
		fbi->fb->fix.smem_start  = fbi->screen_dma;
		fbi->fb->fix.smem_len = fbi->map_size;

		printk("map_video_memory: dma=%08x cpu=%p size=%08x\n",
			fbi->map_dma, fbi->map_cpu, fbi->fb->fix.smem_len);
	}

	return fbi->map_cpu ? 0 : -ENOMEM;
}

static inline void tccfb_unmap_video_memory(struct tccfb_info *fbi)
{
	dma_free_writecombine(fbi->dev,fbi->map_size,fbi->map_cpu, fbi->map_dma);
}


static unsigned int tcc_vioc_set_rdma_arbitor(struct device_node *np)
{
	void __iomem *virt_addr=NULL;
	int num_of_arbitor;
	int ret=0;
	int i;
	
	if (of_property_read_u32(np, "arbitor_num", &num_of_arbitor)){
	   pr_err( "could not find num_of_arbitor nubmer\n");
	   ret = -ENODEV;
	   return ret;
	}
	
	for(i = 0; i < num_of_arbitor; i++) {
		virt_addr = of_iomap(np, i);
		*(volatile uint *)virt_addr = ((1<<31) | (0<<28) | (2<<16) | (0<<12) | (10<<0));
	}

	return ret;
}

unsigned int tcc_dp_dt_parse_data(struct tccfb_info *info)
{
	int ret = 0;
	struct device_node *main_np, *extend_np;
	struct device_node *rdma_arbitor_np=NULL;
		
	if (info->dev->of_node) {
		// Get default display number.					 
		if (of_property_read_u32(info->dev->of_node, "telechips,fbdisplay_num", &info->pdata.lcdc_number)){
			pr_err( "could not find  telechips,fbdisplay_nubmer\n");
			ret = -ENODEV;
		}

		info->pdata.lcdc_number = daudio_lcd_type_lvds_check();

		if(info->pdata.lcdc_number) {
			main_np = of_find_node_by_name(info->dev->of_node, "fbdisplay1");
			extend_np = of_find_node_by_name(info->dev->of_node, "fbdisplay0");
		}
		else {
			main_np = of_find_node_by_name(info->dev->of_node, "fbdisplay0");
			extend_np = of_find_node_by_name(info->dev->of_node, "fbdisplay1");
		}

		if (!main_np) {
			pr_err( "could not find fb node number %d\n", info->pdata.lcdc_number);
			return -ENODEV;
		}

		// Default display block
		tcc_vioc_display_dt_parse(main_np, &info->pdata.Mdp_data);
		info->pdata.lcdc_number = info->pdata.Mdp_data.DispNum;

		
		// Sub displayblock
		tcc_vioc_display_dt_parse(extend_np, &info->pdata.Sdp_data);

		// Set rdma arbitor resgister and disable disp emergency flag according to SOC guide for TCC896x
		rdma_arbitor_np = of_parse_phandle(info->dev->of_node, "telechips,rdma_arbitor",0);
		if(rdma_arbitor_np) {
			VIOC_DISP_EmergencyFlagDisable(info->pdata.Mdp_data.ddc_info.virt_addr);
			VIOC_DISP_EmergencyFlagDisable(info->pdata.Sdp_data.ddc_info.virt_addr);
			tcc_vioc_set_rdma_arbitor(rdma_arbitor_np);
		 }

	}
	return ret;

}
	
static char tccfb_driver_name[]="tccfb";
static int tccfb_probe(struct platform_device *pdev)
{
	struct tccfb_info *info;
	struct fb_info *fbinfo;

	int ret = 0 , plane = 0;
	unsigned int screen_width, screen_height;

	if (!lcd_panel) {
		pr_err("tccfb: no LCD panel data\n");
		return -EINVAL;
	}

// 	const struct of_device_id *of_id = of_match_device(tccfb_of_match, &pdev->dev);
		
	pr_info("\x1b[1;38m   LCD panel is %s %s %d x %d \x1b[0m \n", lcd_panel->manufacturer, lcd_panel->name, lcd_panel->xres, lcd_panel->yres);
	
    screen_width      = lcd_panel->xres;
    screen_height     = lcd_panel->yres;

#if defined(CONFIG_TCC_HDMI_UI_SIZE_1280_720)
    if(tcc_display_data.resolution == 1)
    {
        screen_width      = 720;
        screen_height     = 576;
    }
    else if(tcc_display_data.resolution == 2)
    {
        screen_width 	  = 800;
        screen_height 	  = 480;
    }
#endif

	printk("%s, screen_width=%d, screen_height=%d \n", __func__, screen_width, screen_height);


	for(plane = 0; plane < CONFIG_FB_TCC_DEVS; plane++)
	{
		struct tccfb_info *info_reg;
		struct fb_info *fbinfo_reg;
		
		fbinfo_reg = framebuffer_alloc(sizeof(struct tccfb_info), &pdev->dev);
		info_reg = fbinfo_reg->par;
		info_reg->fb = fbinfo_reg;
		info_reg->dev = &pdev->dev;
		
		if(plane ==0){
			info = info_reg;
			fbinfo = fbinfo_reg;
			
		}
		platform_set_drvdata(pdev, info_reg);

		tcc_dp_dt_parse_data(info_reg);


		strcpy(fbinfo_reg->fix.id, tccfb_driver_name);

		fbinfo_reg->fix.type			= FB_TYPE_PACKED_PIXELS;
		fbinfo_reg->fix.type_aux		= 0;
		fbinfo_reg->fix.xpanstep		= 0;
		fbinfo_reg->fix.ypanstep		= 1;
		fbinfo_reg->fix.ywrapstep		= 0;
		fbinfo_reg->fix.accel			= FB_ACCEL_NONE;
		fbinfo_reg->fix.visual = FB_VISUAL_TRUECOLOR;
		fbinfo_reg->fix.type = FB_TYPE_PACKED_PIXELS;
		
		fbinfo_reg->var.nonstd			= 0;
		fbinfo_reg->var.activate		= FB_ACTIVATE_NOW;
		fbinfo_reg->var.accel_flags		= 0;
		fbinfo_reg->var.vmode			= FB_VMODE_NONINTERLACED;

		fbinfo_reg->fbops				= &tccfb_ops;
		fbinfo_reg->flags				= FBINFO_FLAG_DEFAULT;

		fbinfo_reg->var.xres			= screen_width;
		fbinfo_reg->var.xres_virtual	= fbinfo_reg->var.xres;
		fbinfo_reg->var.yres			= screen_height;

		fbinfo_reg->var.yres_virtual	= fbinfo_reg->var.yres * FB_NUM_BUFFERS;
		fbinfo_reg->var.bits_per_pixel	= default_scn_depth[plane];
		fbinfo_reg->fix.line_length 	= fbinfo_reg->var.xres * fbinfo_reg->var.bits_per_pixel/8;


#if defined(CONFIG_ADDITIONAL_TS_BUFFER_LINES)
		fbinfo_reg->var.reserved[0] = CONFIG_ADDITIONAL_TS_BUFFER_LINES;
#endif
#if defined(CONFIG_PLATFORM_AVN) && !defined(CONFIG_ANDROID)
		fbinfo_reg->pseudo_palette          = devm_kzalloc(&pdev->dev, sizeof(u32) * 16, GFP_KERNEL);
		if (!fbinfo_reg->pseudo_palette) {
			printk( KERN_ERR "Failed to allocate pseudo_palette\r\n");
			ret = -ENOMEM;
			goto free_framebuffer;
		}
#endif

		info_reg->output_on = true;
		// have to modify pjj
		// panel init function E??aC?????? AE??a
		info_reg->pdata.Mdp_data.DispOrder = DD_MAIN;
		info_reg->pdata.Sdp_data.DispOrder = DD_SUB;

		info_reg->pdata.Mdp_data.FbPowerState = true;
		info_reg->pdata.Mdp_data.FbUpdateType = FB_RDMA_UPDATE;
		info_reg->pdata.Mdp_data.DispDeviceType = TCC_OUTPUT_LCD;
		
		tccfb_check_var(&fbinfo_reg->var, fbinfo_reg);

		/* Initialize video memory */
		ret = tccfb_map_video_memory(info_reg, plane);
		if (ret  < 0) {
			printk( KERN_ERR "Failed to allocate video RAM: %d\n", ret);
			ret = -ENOMEM;

			#if defined(CONFIG_PLATFORM_AVN) && !defined(CONFIG_ANDROID)
			goto free_palette;
			#else
			framebuffer_release(fbinfo_reg);
			continue;
			#endif
		}

		ret = register_framebuffer(fbinfo_reg);
		if (ret < 0) {
			pr_err(KERN_ERR "Failed to register framebuffer device: %d\n", ret);
			goto free_video_memory;
		}

		if (plane == 0)	{// top layer
			if (fb_prepare_logo(fbinfo_reg, FB_ROTATE_UR)) {
				/* Start display and show logo on boot */
				pr_info("fb_show_logo\n");
				fb_set_cmap(&fbinfo_reg->cmap, fbinfo);
				fb_show_logo(fbinfo_reg, FB_ROTATE_UR);
        	}
       	}
		spin_lock_init(&info_reg->spin_lockDisp);
		pr_info("fb%d: %s frame buffer device info->dev:0x%p  \n", fbinfo->node, fbinfo->fix.id, info->dev);
	}
	
	#ifdef CONFIG_PM_RUNTIME
	pm_runtime_set_active(info->dev);	
	pm_runtime_enable(info->dev);  
	pm_runtime_get_noresume(info->dev);  //increase usage_count 
	#endif		

	mutex_init(&info->output_lock);

	info->pdata.Mdp_data.pandisp_sync.state = 1;
	init_waitqueue_head(&info->pdata.Mdp_data.pandisp_sync.waitq);

	info->pdata.Mdp_data.disp_dd_sync.state = 1;
	init_waitqueue_head(&info->pdata.Mdp_data.disp_dd_sync.waitq);

	info->pdata.Sdp_data.pandisp_sync.state = 1;
	init_waitqueue_head(&info->pdata.Sdp_data.pandisp_sync.waitq);

	info->pdata.Sdp_data.disp_dd_sync.state = 1;
	init_waitqueue_head(&info->pdata.Sdp_data.disp_dd_sync.waitq);	
		
	INIT_WORK(&info->vsync_work, send_vsync_event);
	
#if defined(CONFIG_SYNC_FB)
	INIT_LIST_HEAD(&info->fb_update_regs_list);
	mutex_init(&info->fb_timeline_lock);
	init_kthread_worker(&info->fb_update_regs_worker);

	info->fb_update_regs_thread = kthread_run(kthread_worker_fn,
			&info->fb_update_regs_worker, "tccfb");

	if (IS_ERR(info->fb_update_regs_thread)) {
		int err = PTR_ERR(info->fb_update_regs_thread);
		info->fb_update_regs_thread = NULL;

		pr_err("failed to run update_regs thread\n");
		return err;
	}
	init_kthread_work(&info->fb_update_regs_work, fence_handler);
	info->fb_timeline = sw_sync_timeline_create("tccfb");
	info->fb_timeline_max	=  0;

	INIT_LIST_HEAD(&info->ext_update_regs_list);
	mutex_init(&info->ext_timeline_lock);
	init_kthread_worker(&info->ext_update_regs_worker);

	info->ext_update_regs_thread = kthread_run(kthread_worker_fn,
			&info->ext_update_regs_worker, "tccfb-ext");

	if (IS_ERR(info->ext_update_regs_thread)) {
		int err = PTR_ERR(info->ext_update_regs_thread);
		info->ext_update_regs_thread = NULL;

		pr_err("failed to run update_regs thread\n");
		return err;
	}
	init_kthread_work(&info->ext_update_regs_work, ext_fence_handler);
	info->ext_timeline = sw_sync_timeline_create("tccfb-ext");
	info->ext_timeline_max	=  0;
#endif

	if(lcd_panel->init)
		lcd_panel->init(lcd_panel, &info->pdata.Mdp_data);

	tca_fb_init(info);

	tca_main_interrupt_reg(true, info);

#if !defined(CONFIG_PLATFORM_AVN)
	tccfb_set_par(fbinfo);
#endif//
	return 0;

#if defined(CONFIG_PLATFORM_AVN) && !defined(CONFIG_ANDROID)
free_palette:
        devm_kfree(&pdev->dev, fbinfo->pseudo_palette);
free_framebuffer:
        framebuffer_release(fbinfo);
#endif
		
free_video_memory:
	tccfb_unmap_video_memory(info);
	pr_err("TCC89xx fb init failed.\n");
	return ret;
}

/*
 *  Cleanup
 */
static int tccfb_remove(struct platform_device *pdev)
{
	struct tccfb_info	 *info = platform_get_drvdata(pdev);
	struct fb_info	   *fbinfo = info->fb;

	pr_info(" %s  \n", __func__);

	tca_main_interrupt_reg(false, info);

	tccfb_unmap_video_memory(info);

#ifdef CONFIG_PM_RUNTIME
	pm_runtime_disable(info->dev);
#endif

	clk_put(info->pdata.Mdp_data.vioc_clock);
	clk_put(info->pdata.Mdp_data.ddc_clock);

	unregister_framebuffer(fbinfo);

	return 0;
}

int tccfb_register_panel(struct lcd_panel *panel)
{
	dprintk(" %s  name:%s \n", __func__, panel->name);

	lcd_panel = panel;
	return 1;
}
EXPORT_SYMBOL(tccfb_register_panel);

struct lcd_panel *tccfb_get_panel(void)
{
	return lcd_panel;
}
EXPORT_SYMBOL(tccfb_get_panel);

#ifdef CONFIG_PM_RUNTIME
int tcc_fb_runtime_suspend(struct device *dev)
{
	struct platform_device *fb_device = container_of(dev, struct platform_device, dev);
	struct tccfb_info	   *info = platform_get_drvdata(fb_device);

	info->pdata.Mdp_data.FbPowerState = 0;
	
	if (lcd_panel->set_power)
		lcd_panel->set_power(lcd_panel, 0, &info->pdata.Mdp_data);

	tca_fb_runtime_suspend(info);

	return 0;
}

int tcc_fb_runtime_resume(struct device *dev)
{
	struct platform_device *fb_device = container_of(dev, struct platform_device, dev);
	struct tccfb_info	   *info = platform_get_drvdata(fb_device);

	tca_fb_runtime_resume(info);

	if (lcd_panel->set_power)
		lcd_panel->set_power(lcd_panel, 1, &info->pdata.Mdp_data);

	if(info->pdata.Mdp_data.DispDeviceType == TCC_OUTPUT_LCD)
		info->pdata.Mdp_data.FbPowerState = 1;

	return 0;
}

#endif

static const struct dev_pm_ops tccfb_pm_ops = {
#ifdef CONFIG_PM_RUNTIME
	.runtime_suspend      = tcc_fb_runtime_suspend,
	.runtime_resume       = tcc_fb_runtime_resume,
#endif
	.suspend	= tccfb_suspend,
	.resume = tccfb_resume,
	.freeze = tccfb_freeze,
	.thaw = tccfb_thaw,
	.restore = tccfb_restore,
};


static struct platform_driver tccfb_driver = {
	.probe		= tccfb_probe,
	.remove		= tccfb_remove,
	.shutdown	= tccfb_shutdown,
	.driver		= {
		.name	= "tccfb",
		.owner	= THIS_MODULE,
		.pm		= &tccfb_pm_ops,
		.of_match_table = of_match_ptr(tccfb_of_match),
	},
};

//int tccfb_init(void)
static int __init tccfb_init(void)
{
	printk(KERN_INFO " %s \n", __func__);
	return platform_driver_register(&tccfb_driver);
}

static void __exit tccfb_exit(void)
{
	dprintk(" %s \n", __func__);
	tca_fb_exit();

	platform_driver_unregister(&tccfb_driver);
}


module_init(tccfb_init);
module_exit(tccfb_exit);

MODULE_AUTHOR("linux <linux@telechips.com>");
MODULE_DESCRIPTION("Telechips TCC Framebuffer driver");
MODULE_LICENSE("GPL");
