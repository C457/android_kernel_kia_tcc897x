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
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/ctype.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/sched.h>
#include <linux/irq.h>
#include <linux/module.h>

#include <linux/of.h>
#include <linux/of_irq.h>

#include <linux/of_device.h>
#include <linux/of_address.h>

#include <mach/irqs.h>
#include <mach/hardware.h>
#include <asm/io.h>

#include <mach/bsp.h>
#include <mach/gpio.h>

#include <asm/scatterlist.h>
#include <asm/mach-types.h>

#include "sensor_if.h"
#include "tcc_cam.h"
#include "camera_core.h"
#include "cam_reg.h"
#include <soc/tcc/pmap.h>
#include "tcc_cam_i2c.h"
#if defined(CONFIG_ARCH_TCC898X)
#include <video/tcc/vioc_rdma.h>
#include <video/tcc/vioc_ireq.h>
#include <video/tcc/vioc_config.h>
#include <video/tcc/vioc_wdma.h>
#include <video/tcc/vioc_wmix.h>
#include <video/tcc/vioc_scaler.h>
#include <video/tcc/vioc_vin.h>
#include <video/tcc/vioc_viqe.h>
#include <video/tcc/vioc_disp.h>
#include <video/tcc/vioc_deintls.h>
#include <video/tcc/vioc_fifo.h>
#else
#include <mach/vioc_rdma.h>
#include <mach/vioc_ireq.h>
#include <mach/vioc_config.h>
#include <mach/vioc_wdma.h>
#include <mach/vioc_wmix.h>
#include <mach/vioc_scaler.h>
#include <mach/vioc_vin.h>
#include <mach/vioc_viqe.h>
#include <mach/vioc_disp.h>
#include <mach/vioc_deintls.h>
#include <mach/vioc_fifo.h>
#endif
#if defined(CONFIG_TCC_REAR_CAMERA_DRV)
#include <video/tcc/viocmg.h>
#endif//defined(CONFIG_TCC_REAR_CAMERA_DRV)

#include "tcc_cam_direct_display_if.h"
#include "tcc_camera_device.h"

#include <tcc_cam_cm_control.h>
#include <mach/daudio_info.h>

#define PREVIEW_BUFFER_NUMBER	4

//#define FEATURE_USE_DEINTLS_REAR_CAMERA
//#define PREVIEW_RATIO_KEEP

static int debug	= 1;
#define log(msg...)	if(debug) { printk("%s - ", __func__); printk(msg); }
#define FUNCTION_IN	log("START\n");
#define FUNCTION_OUT	log("FINISH\n");

void test_registers(struct tcc_camera_device * vdev) { 
	struct reg_test { 
	unsigned int * reg; 
	unsigned int cnt; 
	};
	 
	VIOC_VIN * pVINBase = (VIOC_VIN *)vdev->vioc.vin.address;
#if defined(CONFIG_TCC_REAR_CAMERA_DRV)
	VIOC_RDMA * pRDMA_PGL = (VIOC_RDMA *)vdev->vioc.pgl.address;
#endif
#ifdef FEATURE_USE_DEINTLS_REAR_CAMERA
	unsigned int * pDeintls = (unsigned int *)vdev->vioc.deintls.address;
#else
	VIQE * pVIQE = (VIQE *)vdev->vioc.viqe.address;
#endif
	VIOC_SC * pSC = (VIOC_SC *)vdev->vioc.scaler.address;
	VIOC_WMIX * pWMIX = (VIOC_WMIX *)vdev->vioc.wmixer.address;
	VIOC_WDMA * pWDMA = (VIOC_WDMA *)vdev->vioc.wdma.address;
	VIOC_RDMA * pRDMA_disp = (VIOC_RDMA *)vdev->vioc.rdma.address;
	 
	int i = 0;
	 
	struct reg_test regList[] = { 
	{ (unsigned int *)pVINBase+10,  4 },
#if defined(CONFIG_TCC_REAR_CAMERA_DRV)
//	{ (unsigned int *)pRDMA_PGL, 12 }, 
#endif
#ifdef FEATURE_USE_DEINTLS_REAR_CAMERA
//	{ (unsigned int *)pDeintls, 4 },
#else
//	{ (unsigned int *)pVIQE, 4 },
#endif
//	{ (unsigned int *)pSC, 8 },
//	{ (unsigned int *)pWMIX, 28 }, 
	{ (unsigned int *)pWDMA+4, 1 }, 
//	{ (unsigned int *)pRDMA_disp, 12 }, 
//	{ (unsigned int *)VIOC_CONFIG_GetPathStruct(VIOC_SC0 + vdev->vioc.scaler.index),  1 },
#ifdef FEATURE_USE_DEINTLS_REAR_CAMERA
//	{ (unsigned int *)VIOC_CONFIG_GetPathStruct(VIOC_DEINTLS), 1 },
#else
//	{ (unsigned int *)VIOC_CONFIG_GetPathStruct(VIOC_VIQE), 1 },
#endif
	}; 
	unsigned int * addr; 
	unsigned int reg, idxLoop, nReg, idxReg; 
	 
	for(i=0; i<3; i++) {
		for(idxLoop=0; idxLoop<sizeof(regList)/sizeof(regList[0]); idxLoop++) { 
			addr = regList[idxLoop].reg;
			nReg = regList[idxLoop].cnt;

			for(idxReg=0; idxReg<nReg; idxReg++) {
				if((idxReg%4) == 0)
					printk("\n%08x: ", (unsigned int)(addr + idxReg));


				// REGREAD((unsigned int)(addr + idxReg), reg); 
				printk("%08x ", *(addr + idxReg));
			}
		}

		if(vdev->data.cam_info == DAUDIO_CAMERA_REAR) {
			printk("\n[TW9990 Register Check]\n");
			printk("reg : 0x01, val : 0x%x\n", sensor_if_read_i2c(0x01,vdev));
		}
	}
	if(vdev->data.cam_info == DAUDIO_CAMERA_REAR) {
		printk("reg : 0x02, val : 0x%x\n", sensor_if_read_i2c(0x02,vdev));
		printk("reg : 0x03, val : 0x%x\n", sensor_if_read_i2c(0x03,vdev));
	}
}

int direct_display_parse_device_tree(struct tcc_camera_device * vdev) {
	struct device_node * camera_np = vdev->camera_np, * handler_np = NULL;
	struct device_node * np_fb, * np_fb_1st = NULL, * np_fb_child = NULL;
	unsigned int lut_offset;

	FUNCTION_IN
	
	if(!camera_np) {
		printk("%s - FAILED: to find camera platform node.\n", __func__);
		return -ENODEV;
	}
	
	// Camera Config
	if(!(handler_np = of_parse_phandle(camera_np, "camera_config", 0))) {
		printk("%s - FAILED: to find camera irqe node.\n", __func__);
                return -ENODEV;
	}
	of_property_read_u32_index(camera_np, "camera_config", 1, &vdev->vioc.config.index);
	vdev->vioc.config.address = (unsigned int *)of_iomap(handler_np, vdev->vioc.config.index);
	log("config[%d] address: %p\n", vdev->vioc.config.index, vdev->vioc.config.address);

	// PGL
	if(!(handler_np = of_parse_phandle(camera_np, "camera_pgl", 0))) {
		printk("%s - FAILED: to find camera pgl node.\n", __func__);
                return -ENODEV;
	}
	of_property_read_u32_index(camera_np, "camera_pgl", 1, &vdev->vioc.pgl.index);
	vdev->vioc.pgl.address = (unsigned int *)of_iomap(handler_np, vdev->vioc.pgl.index);
	log("pgl[%d] address: %p\n", vdev->vioc.pgl.index, vdev->vioc.pgl.address);

	// VIN
 	if(!(handler_np = of_parse_phandle(camera_np, "camera_videoin", 0))) {
		printk("%s - FAILED: to find video input node.\n", __func__);
                return -ENODEV;
 	}
	of_property_read_u32_index(camera_np, "camera_videoin", 2, &vdev->vioc.vin.index);
	vdev->vioc.vin.address = (unsigned int *)of_iomap(handler_np, vdev->vioc.vin.index);
	log("vin[%d] address: %p\n", vdev->vioc.vin.index, vdev->vioc.vin.address);
	
	of_property_read_u32_index(camera_np, "camera_videoin", 1, &lut_offset);			
	vdev->vioc.lut.address = (unsigned int *)of_iomap(handler_np, vdev->vioc.vin.index + lut_offset);
	log("lut address: %p\n", vdev->vioc.lut.address);
	
#ifdef FEATURE_USE_DEINTLS_REAR_CAMERA
	// DEINTL_S device tree
	if(!(handler_np = of_parse_phandle(camera_np, "camera_deintls", 0))) {
		printk("%s - FAILED: to find camera deintls node.\n", __func__);
                return -ENODEV;
	}
	of_property_read_u32_index(camera_np, "camera_deintls", 1, &vdev->vioc.deintls.index);
	vdev->vioc.deintls.address = (unsigned int *)of_iomap(handler_np, vdev->vioc.deintls.index);
	log("deintl_s[%d] address: %p\n", vdev->vioc.deintls.index, vdev->vioc.deintls.address);
#else
	// VIQE device tree
	if(!(handler_np = of_parse_phandle(camera_np, "camera_viqe", 0))) {
		printk("%s - FAILED: to find camera viqe node.\n", __func__);
                return -ENODEV;
	}
	of_property_read_u32_index(camera_np, "camera_viqe", 1, &vdev->vioc.viqe.index);
	vdev->vioc.viqe.address = (unsigned int *)of_iomap(handler_np, vdev->vioc.viqe.index);
	log("viqe[%d] address: %p\n", vdev->vioc.viqe.index, vdev->vioc.viqe.address);
#endif
	
	// SCaler
	if(!(handler_np = of_parse_phandle(camera_np, "camera_scaler", 0))) {
		printk("%s - FAILED: to find camera scaler node.\n", __func__);
                return -ENODEV;
	}
	of_property_read_u32_index(camera_np, "camera_scaler", 1, &vdev->vioc.scaler.index);
	vdev->vioc.scaler.address = (unsigned int *)of_iomap(handler_np, vdev->vioc.scaler.index);
	log("scaler[%d] address: %p\n", vdev->vioc.scaler.index, vdev->vioc.scaler.address);

	// WMIXer
	if(!(handler_np = of_parse_phandle(camera_np, "camera_wmixer", 0))) {
		printk("%s - FAILED: to find camera wmixer node.\n", __func__);
                return -ENODEV;
	}
	of_property_read_u32_index(camera_np,"camera_wmixer", 1, &vdev->vioc.wmixer.index);
	vdev->vioc.wmixer.address = (unsigned int *)of_iomap(handler_np, vdev->vioc.wmixer.index);
	log("wmixer[%d] address: %p\n", vdev->vioc.wmixer.index, vdev->vioc.wmixer.address);

	// WDMA
	if(!(handler_np = of_parse_phandle(camera_np, "camera_wdma", 0))) {
		printk("%s - FAILED: to find camera wdma node.\n", __func__);
                return -ENODEV;
	}
	of_property_read_u32_index(camera_np,"camera_wdma", 1, &vdev->vioc.wdma.index);
	vdev->vioc.wdma.address = (unsigned int *)of_iomap(handler_np, vdev->vioc.wdma.index);
	log("wdma[%d] address: %p\n", vdev->vioc.wdma.index, vdev->vioc.wdma.address);
	
	/* get the information of vioc-fb device node */
	np_fb = of_find_compatible_node(NULL, NULL, "telechips,vioc-fb");
	if(of_property_read_u32(np_fb, "telechips,fbdisplay_num", &vdev->vioc.disp.index)) {
		printk("%s - FAILED: to find fbdisplay_num node.\n", __func__);
                return -ENODEV;
	}

	vdev->vioc.disp.index = daudio_lcd_type_lvds_check();

	/* get register address for main output */
	np_fb_1st = of_find_node_by_name(np_fb, ((vdev->vioc.disp.index) ? "fbdisplay1" : "fbdisplay0"));
	if(!(np_fb_child = of_parse_phandle(np_fb_1st, "telechips,disp", 0))) {
		printk("%s - FAILED: to find disp node.\n", __func__);
                return -ENODEV;
	}
	vdev->vioc.disp.address = (unsigned int *)of_iomap(np_fb_child, 0);
	log("disp[%d] address: %p\n", vdev->vioc.disp.index, vdev->vioc.disp.address);

	if(!(np_fb_child = of_parse_phandle(np_fb_1st,"telechips,wmixer", 0))) {
		printk("%s - FAILED: to find wmixer node.\n", __func__);
                return -ENODEV;
	}
	of_property_read_u32_index(np_fb_1st, "telechips,wmixer", 1, &vdev->vioc.wmixer_out.index);
	vdev->vioc.wmixer_out.address = (unsigned int *)of_iomap(np_fb_child, vdev->vioc.wmixer_out.index);
	log("wmixer_out[%d] address: %p\n", vdev->vioc.wmixer_out.index, vdev->vioc.wmixer_out.address);

	if(!(np_fb_child = of_parse_phandle(np_fb_1st,"telechips,rdma", 0))) {
		printk("%s - FAILED: to find rdma node.\n", __func__);
                return -ENODEV;
	}
#if defined(CONFIG_TCC_REAR_CAMERA_DRV)
	// Linux
	of_property_read_u32_index(np_fb_1st, "telechips,rdma", 1+2, &vdev->vioc.rdma.index);
#else
	// Android
	of_property_read_u32_index(np_fb_1st, "telechips,rdma", 1+1, &vdev->vioc.rdma.index);
#endif//defined(CONFIG_TCC_REAR_CAMERA_DRV)
	vdev->vioc.rdma.address = (unsigned int *)of_iomap(np_fb_child, vdev->vioc.rdma.index);
	log("rdma[%d] address: %p\n", vdev->vioc.rdma.index, vdev->vioc.rdma.address);

	// FIFO
	if(!(handler_np = of_parse_phandle(camera_np, "camera_fifo", 0))) {
		printk("%s - FAILED: to find camera fifo node.\n", __func__);
                return -ENODEV;
	}
	vdev->vioc.fifo.address = (unsigned int *)of_iomap(handler_np, vdev->vioc.fifo.index);
	log("fifo[%d] address: %p\n", vdev->vioc.fifo.index, vdev->vioc.fifo.address);
	
	FUNCTION_OUT
	return 0;
}

int direct_display_reset_vioc_path(struct tcc_camera_device * vdev) {
	VIOC_IREQ_CONFIG	* pVIOCConfig	= (VIOC_IREQ_CONFIG	*)vdev->vioc.config.address;
	
	VIOC_CONFIG_SWReset(pVIOCConfig, VIOC_CONFIG_FIFO,	vdev->vioc.fifo.index,		VIOC_CONFIG_RESET);
	VIOC_CONFIG_SWReset(pVIOCConfig, VIOC_CONFIG_RDMA,	vdev->vioc.rdma.index,		VIOC_CONFIG_RESET);
	VIOC_CONFIG_SWReset(pVIOCConfig, VIOC_CONFIG_WDMA,	vdev->vioc.wdma.index,		VIOC_CONFIG_RESET);
	VIOC_CONFIG_SWReset(pVIOCConfig, VIOC_CONFIG_WMIXER,	vdev->vioc.wmixer.index,	VIOC_CONFIG_RESET);
	VIOC_CONFIG_SWReset(pVIOCConfig, VIOC_CONFIG_SCALER,	vdev->vioc.scaler.index,	VIOC_CONFIG_RESET);
	if(vdev->tcc_sensor_info.intl_en) {
#ifdef FEATURE_USE_DEINTLS_REAR_CAMERA
	VIOC_CONFIG_SWReset(pVIOCConfig, VIOC_CONFIG_DEINTS,	vdev->vioc.deintls.index,	VIOC_CONFIG_RESET);
#else
	VIOC_CONFIG_SWReset(pVIOCConfig, VIOC_CONFIG_VIQE,	vdev->vioc.viqe.index,		VIOC_CONFIG_RESET);
#endif
	}
	VIOC_CONFIG_SWReset(pVIOCConfig, VIOC_CONFIG_VIN,	vdev->vioc.vin.index,		VIOC_CONFIG_RESET);
#if defined(CONFIG_TCC_REAR_CAMERA_DRV)
	VIOC_CONFIG_SWReset(pVIOCConfig, VIOC_CONFIG_VIN,	vdev->vioc.pgl.index,		VIOC_CONFIG_RESET);
#endif//defined(CONFIG_TCC_REAR_CAMERA_DRV)
	mdelay(1);
#if defined(CONFIG_TCC_REAR_CAMERA_DRV)
	VIOC_CONFIG_SWReset(pVIOCConfig, VIOC_CONFIG_VIN,	vdev->vioc.pgl.index,		VIOC_CONFIG_CLEAR);
#endif//defined(CONFIG_TCC_REAR_CAMERA_DRV)
	VIOC_CONFIG_SWReset(pVIOCConfig, VIOC_CONFIG_VIN,	vdev->vioc.vin.index,		VIOC_CONFIG_CLEAR);
	if(vdev->tcc_sensor_info.intl_en) {
#ifdef FEATURE_USE_DEINTLS_REAR_CAMERA
	VIOC_CONFIG_SWReset(pVIOCConfig, VIOC_CONFIG_DEINTS,	vdev->vioc.deintls.index,	VIOC_CONFIG_CLEAR);
#else
	VIOC_CONFIG_SWReset(pVIOCConfig, VIOC_CONFIG_VIQE,	vdev->vioc.viqe.index,		VIOC_CONFIG_CLEAR);
#endif
	}
	VIOC_CONFIG_SWReset(pVIOCConfig, VIOC_CONFIG_SCALER,	vdev->vioc.scaler.index,	VIOC_CONFIG_CLEAR);
	VIOC_CONFIG_SWReset(pVIOCConfig, VIOC_CONFIG_WMIXER,	vdev->vioc.wmixer.index,	VIOC_CONFIG_CLEAR);
	VIOC_CONFIG_SWReset(pVIOCConfig, VIOC_CONFIG_WDMA,	vdev->vioc.wdma.index,		VIOC_CONFIG_CLEAR);
	VIOC_CONFIG_SWReset(pVIOCConfig, VIOC_CONFIG_RDMA,	vdev->vioc.rdma.index,		VIOC_CONFIG_CLEAR);
	VIOC_CONFIG_SWReset(pVIOCConfig, VIOC_CONFIG_FIFO,	vdev->vioc.fifo.index,		VIOC_CONFIG_CLEAR);
	
	return 0;
}

int direct_display_init_parameters(struct tcc_camera_device * vdev) {
	struct cif_c_t * data = (struct cif_c_t *)&vdev->data.cif_cfg;
	
	// Set all the parameters.
	data->polarity_pclk	= vdev->tcc_sensor_info.p_clock_pol;
	data->polarity_vsync	= vdev->tcc_sensor_info.v_sync_pol;
	data->polarity_href	= vdev->tcc_sensor_info.h_sync_pol;
	data->polarity_de	= vdev->tcc_sensor_info.de_pol;
	data->field_bfield_low	= vdev->tcc_sensor_info.field_bfield_low;
	data->gen_field_en	= vdev->tcc_sensor_info.gen_field_en;
	data->conv_en		= vdev->tcc_sensor_info.conv_en;
	data->hsde_connect_en	= vdev->tcc_sensor_info.hsde_connect_en;
	data->vs_mask		= vdev->tcc_sensor_info.vs_mask;
	data->input_fmt		= vdev->tcc_sensor_info.input_fmt;
	data->data_order	= vdev->tcc_sensor_info.data_order;
	data->intl_en		= vdev->tcc_sensor_info.intl_en;
	data->intpl_en		= vdev->tcc_sensor_info.intpl_en;
	data->fmt		= vdev->tcc_sensor_info.format;
	data->main_set.vin_crop.width	= vdev->tcc_sensor_info.preview_w;
	data->main_set.vin_crop.height	= vdev->tcc_sensor_info.preview_h;
	data->main_set.vin_crop.left = 0;
	data->main_set.vin_crop.top = 0;

	log("polarity_pclk	= 0x%x\n", data->polarity_pclk);
	log("polarity_vsync	= 0x%x\n", data->polarity_vsync);
	log("polarity_href	= 0x%x\n", data->polarity_href);
	log("polarity_de	= 0x%x\n", data->polarity_de);
	log("field_bfield_low	= 0x%x\n", data->field_bfield_low);
	log("gen_field_en	= 0x%x\n", data->gen_field_en);
	log("conv_en		= 0x%x\n", data->conv_en);
	log("hsde_connect_en	= 0x%x\n", data->hsde_connect_en);
	log("vs_mask		= 0x%x\n", data->vs_mask);
	log("input_fmt		= 0x%x\n", data->input_fmt);
	log("data_order		= 0x%x\n", data->data_order);
	log("intl_en		= 0x%x\n", data->intl_en);
	log("ntpl_en		= 0x%x\n", data->intpl_en);
	log("fmt		= 0x%x\n", data->fmt);
	log("main_set.vin_crop.width	= 0x%x\n", data->main_set.vin_crop.width);
	log("main_set.vin_crop.height	= 0x%x\n", data->main_set.vin_crop.height);
	log("main_set.vin_crop.left	= 0x%x\n", data->main_set.vin_crop.left);
	log("main_set.vin_crop.top	= 0x%x\n", data->main_set.vin_crop.top);

	return 0;
}

int direct_display_set_port(struct tcc_camera_device * vdev) {
	struct device_node	* camera_np	= vdev->camera_np;
	struct device_node	* port_np	= NULL;
	
	unsigned int	* cifport_addr;
	unsigned int	nUsingPort;
	
	// Camera Port
	if(vdev->data.cam_info < DAUDIO_CAMERA_LVDS) 
		of_property_read_u32_index(camera_np, "camera_port", 1, &nUsingPort);
	else
		of_property_read_u32_index(camera_np, "camera_port", 3, &nUsingPort);
	port_np = of_parse_phandle(camera_np, "camera_port", 0);
	cifport_addr = of_iomap(port_np, 0);
	
	printk("cifport register = 0x%x\n", *cifport_addr);
	
	BITCSET(*cifport_addr, 0x00077777, nUsingPort << (vdev->vioc.vin.index * 4));
	
	return 0;
}
#if defined(CONFIG_TCC_REAR_CAMERA_DRV)
int direct_display_set_pgl(struct tcc_camera_device * vdev, unsigned int addrY, unsigned int width, unsigned int height, unsigned int fmt) {
	VIOC_RDMA * pRDMA = (VIOC_RDMA *)vdev->vioc.pgl.address;
	
	log("RDMA[%d] = 0x%08x, size = %d * %d, FMT = %d\n", vdev->vioc.pgl.index, (unsigned int)pRDMA, width, height, fmt);
	
	VIOC_RDMA_SetImageFormat(pRDMA, fmt);
	VIOC_RDMA_SetImageSize(pRDMA, width, height);
	VIOC_RDMA_SetImageOffset(pRDMA, fmt, width);
	VIOC_RDMA_SetImageBase(pRDMA, addrY, 0, 0);
	VIOC_RDMA_SetImageAlphaEnable(pRDMA, ON);
	VIOC_RDMA_SetImageAlpha(pRDMA, 0xff, 0xff);
	VIOC_RDMA_SetImageEnable(pRDMA);
	
	return 0;
}
#endif//defined(CONFIG_TCC_REAR_CAMERA_DRV)
int direct_display_set_vin(struct tcc_camera_device * vdev) {
	VIOC_VIN * pVIN = (VIOC_VIN *)vdev->vioc.vin.address;
	struct cif_c_t * data = (struct cif_c_t *)&vdev->data.cif_cfg;

	log("VIN[%d] = 0x%08x, size = %d * %d, crop(x, y(w * h), %d, %d(%d * %d), De-Interlace = %d\n", \
		vdev->vioc.vin.index, (unsigned int)vdev->vioc.vin.address,	\
		vdev->tcc_sensor_info.preview_w, vdev->tcc_sensor_info.preview_h, \
		data->main_set.vin_crop.left, data->main_set.vin_crop.top, \
		data->main_set.vin_crop.width, data->main_set.vin_crop.height, \
		vdev->tcc_sensor_info.intl_en);

	VIOC_VIN_SetSyncPolarity(pVIN, !(data->polarity_href), !(data->polarity_vsync), \
			data->field_bfield_low, data->polarity_de, data->gen_field_en,!(data->polarity_pclk));
	VIOC_VIN_SetCtrl(pVIN, data->conv_en, data->hsde_connect_en, data->vs_mask, data->input_fmt, data->data_order);
	VIOC_VIN_SetInterlaceMode(pVIN, data->intl_en, data->intpl_en);
	VIOC_VIN_SetCaptureModeEnable(pVIN, OFF);
	
	VIOC_VIN_SetImageSize(pVIN, \
		vdev->tcc_sensor_info.preview_w, vdev->tcc_sensor_info.preview_h);

	//2017.12.22 - LVDS SVM Display Timing Fixed. Horizontal Blank(128) Vertical Blank(4)
	//2018.06.15 - LVDS SVM PCLK Changed(2208 -> 2528). Horizontal Blank(128+320 = 448)
	if(vdev->data.cam_info == DAUDIO_CAMERA_LVDS || vdev->data.cam_info == DAUDIO_DVRS_RVM)
	{
//		if(gpio_get_value(TCC_GPB(19)))	//b110
			VIOC_VIN_SetImageOffset(pVIN, 448, 4, 0);
//		else				//b100
//			VIOC_VIN_SetImageOffset(pVIN, 128, 4, 0);
	} else if (vdev->data.cam_info == DAUDIO_ADAS_PRK) 
	{
		VIOC_VIN_SetImageOffset(pVIN, 288, 0, 0);
	}
	else
		VIOC_VIN_SetImageOffset(pVIN, 0, 0, 0);
	VIOC_VIN_SetImageCropOffset(pVIN, \
		data->main_set.vin_crop.left, data->main_set.vin_crop.top);
	VIOC_VIN_SetImageCropSize(pVIN, \
		data->main_set.vin_crop.width, data->main_set.vin_crop.height);

	//2018.05.08 - Using VIN-LUT for LVDS SVM Display Image Enhancement
	//	     - Don't use Y2R block of VIN component when the camera device uses VIN-LUT.
	if(vdev->data.cam_info == DAUDIO_CAMERA_LVDS || vdev->data.cam_info == DAUDIO_ADAS_PRK || vdev->data.cam_info == DAUDIO_DVRS_RVM) {
		VIOC_VIN_SetLUT(pVIN, vdev->vioc.lut.address);
		VIOC_VIN_SetLUTEnable(pVIN, ON, ON, ON);
//		VIOC_VIN_SetLUT_by_table(pVIN, rgb);
		tccxxx_cif_vin_lut_update(vdev);
		vioc_intr_enable(vdev->data.vioc_vin_intr.id, vdev->data.vioc_vin_intr.bits);
	}
	else {
		VIOC_VIN_SetLUTEnable(pVIN, OFF, OFF, OFF);
	}
	

	VIOC_VIN_SetEnable(pVIN, ON);
	
	return 0;
}

int direct_display_set_viqe(struct tcc_camera_device * vdev) {
	struct device_node	* camera_np = vdev->camera_np, * handler_np = NULL;
	struct cif_c_t * data = (struct cif_c_t *)&vdev->data.cif_cfg;
	
#ifdef FEATURE_USE_DEINTLS_REAR_CAMERA
	unsigned int * pDeintls = (unsigned int *)vdev->vioc.deintls.address;
	
	log("DEINTL_S: %d * %d\n", data->main_set.vin_crop.width, data->main_set.vin_crop.height);
	
	VIOC_CONFIG_PlugIn(VIOC_DEINTLS, VIOC_VIQE_VIN_00);
	/*	mode = 3,	case: TCC8960 / TCC8970
	 *	mode = 0,	others
	 */
#if defined(CONFIG_ARCH_TCC896X) || defined(CONFIG_ARCH_TCC897X)
	VIOC_DEINTLS_SetDeIntlMode(pDeintls, 3);
#else
	VIOC_DEINTLS_SetDeIntlMode(pDeintls, 0);
#endif
#else
	VIQE		* pVIQE		= (VIQE *)vdev->vioc.viqe.address;
	
	unsigned int	* viqe_set_reg1 = NULL;
	unsigned int	* viqe_set_reg2 = NULL;
	
	unsigned int	viqe_width	= 0;
	unsigned int	viqe_height	= 0;
	unsigned int	format		= FMT_FC_YUV420;
	unsigned int	bypass_deintl	= VIOC_VIQE_DEINTL_MODE_3D;
	unsigned int	offset		= \
		data->main_set.vin_crop.width * data->main_set.vin_crop.height * 2 * 2;
	unsigned int	deintl_base0	= vdev->pmap_viqe.base;
	unsigned int	deintl_base1	= deintl_base0 + offset;
	unsigned int	deintl_base2	= deintl_base1 + offset;
	unsigned int	deintl_base3	= deintl_base2 + offset;
	
	unsigned int	cdf_lut_en	= OFF;
	unsigned int	his_en		= OFF;
	unsigned int	gamut_en	= OFF;
	unsigned int	d3d_en		= OFF;
	unsigned int	deintl_en	= ON;
	
	if(!(handler_np = of_parse_phandle(camera_np, "camera_viqe_set", 0))) {
		printk("could not find camera_viqe_set node!! \n");
		return -1;
	}
	viqe_set_reg1 = (unsigned int *)of_iomap(handler_np, 0);
	viqe_set_reg2 = (unsigned int *)of_iomap(handler_np, 1);
	
	log("VIQE: %d * %d\n", data->main_set.vin_crop.width, data->main_set.vin_crop.height);
	
	VIOC_CONFIG_PlugIn(VIOC_VIQE, VIOC_VIQE_VIN_00);
	
	BITCSET(*viqe_set_reg1, 1<<3, 1<<3);
	BITCSET(*viqe_set_reg2, 1<<8 | 1<<9 , 1<<8 | 1<<9);

	VIOC_VIQE_SetImageY2RMode(pVIQE, 2);
	VIOC_VIQE_SetImageY2REnable(pVIQE, ON);
	VIOC_VIQE_SetControlRegister(pVIQE, viqe_width, viqe_height, format);
	VIOC_VIQE_SetDeintlRegister(pVIQE, format, OFF, viqe_width, viqe_height, bypass_deintl, \
					deintl_base0, deintl_base1, deintl_base2, deintl_base3);
	VIOC_VIQE_SetControlEnable(pVIQE, cdf_lut_en, his_en, gamut_en, d3d_en, deintl_en);
		
	// YUV422 Format
	BITCSET(pVIQE->cDEINTL_COMP.nVIQE_FC_MISC.nREG,	0x0000f000,		(1 << 12));			// 0x160
	BITCSET(pVIQE->cDEINTL.nDI_FMT,			0x00000001,		(1 <<  0));			// 0x2E8
	
	// Like Weave Mode
	BITCSET(pVIQE->cDEINTL.nDI_ENGINE0,		0xffffffff,		0x0204ff08);			// 0x284
	BITCSET(pVIQE->cDEINTL.nDI_ENGINE4,		0xffffffff,		0x124f2582);			// 0x294
	BITCSET(pVIQE->cDEINTL.nDI_CTRL,		((1<<5)|(1<<4)|(1<<0)),	((0<<5)|(0<<4)|(0<<0)));	// 0x280
	BITCSET(pVIQE->cDEINTL.nDI_ENGINE3,		0xfff00000,		(0<<20));			// 0x290
#endif
	return 0;
}

int direct_display_set_scaler(struct tcc_camera_device * vdev, unsigned int outW, unsigned int outH) {
	VIOC_SC * pSC = (VIOC_SC *)vdev->vioc.scaler.address;
	struct cif_c_t * data = (struct cif_c_t *)&vdev->data.cif_cfg;
	unsigned int srcW, srcH, dstW, dstH;
	int diffLcdW_SrcW, diffLcdH_SrcH, outX, outY;
	unsigned int interlaced = data->intl_en == ON ? 2 : 1;

	srcW = data->main_set.vin_crop.width;
	srcH = data->main_set.vin_crop.height * interlaced;
	dstW = outW;
	diffLcdW_SrcW = diffLcdH_SrcH = outX = 0;
	outY = vdev->rcam_misc.preview_crop_y;
	dstH = outH + vdev->rcam_misc.preview_additional_height;
	
	log("src: %d * %d\n", srcW, srcH);
	log("lcd: %d * %d\n", outW, outH);
	log("diff: %d * %d\n", diffLcdW_SrcW, diffLcdH_SrcH);
	log("dst: %d * %d\n", dstW, dstH);
	log("out: (%d, %d) %d * %d\n", outX, outY, outW, outH);
	
	VIOC_CONFIG_PlugIn(VIOC_SC0 + vdev->vioc.scaler.index, VIOC_SC_VIN_00);
	VIOC_SC_SetBypass(pSC, OFF);
	VIOC_SC_SetDstSize(pSC, dstW, dstH);
	VIOC_SC_SetOutPosition(pSC, outX, outY);
	VIOC_SC_SetOutSize(pSC, outW, outH);
	VIOC_SC_SetUpdate(pSC);
	
	return 0;
}

int direct_display_set_wmix(struct tcc_camera_device * vdev, unsigned int width, unsigned int height) {
	VIOC_WMIX * pWMIX = (VIOC_WMIX *)vdev->vioc.wmixer.address;
#if defined(CONFIG_TCC_REAR_CAMERA_DRV)
	unsigned int layer		= 0x0;
	unsigned int key_R		= 0xFF;
	unsigned int key_G		= 0xFF;
	unsigned int key_B		= 0xFF;
	unsigned int key_mask_R 	= 0xF8;
	unsigned int key_mask_G 	= 0xF8;
	unsigned int key_mask_B 	= 0xF8; 
#endif//defined(CONFIG_TCC_REAR_CAMERA_DRV)
	
	log("WMIX[%d] = 0x%08x, size = %d * %d\n", vdev->vioc.wmixer.index, (unsigned int)pWMIX, width, height);
	
#if defined(CONFIG_TCC_REAR_CAMERA_DRV)
	// Linux
	if(viocmg_is_feature_rear_cam_use_parking_line()) {
		VIOC_WMIX_SetSize(pWMIX, width, height);
		VIOC_WMIX_SetPosition(pWMIX, 1, vdev->rcam_misc.parking_line_x, vdev->rcam_misc.parking_line_y);
		VIOC_WMIX_SetChromaKey(pWMIX, layer, ON, key_R, key_G, key_B, key_mask_R, key_mask_G, key_mask_B);				
		VIOC_CONFIG_WMIXPath(WMIX50, ON);
	}
#else
	// Android
	VIOC_CONFIG_WMIXPath(WMIX50, OFF);
#endif
	VIOC_WMIX_SetUpdate(pWMIX);
	
	return 0;
}

int direct_display_set_wdma(struct tcc_camera_device * vdev, unsigned int addrY, unsigned int width, unsigned int height, unsigned int fmt) {
	VIOC_WDMA * pWDMA = (VIOC_WDMA *)vdev->vioc.wdma.address;
	
	log("WDMA[%d] = 0x%08x, size = %d * %d, FMT = %d\n", vdev->vioc.wdma.index, (unsigned int)pWDMA, width, height, fmt);

		//VIOC_WDMA_SetImageY2RMode(pWDMA, 0);
		//VIOC_WDMA_SetImageY2REnable(pWDMA, OFF);

	// Because of using YCbCr color space LUT formala to set VIN_LUT
	if(vdev->data.cam_info == DAUDIO_CAMERA_LVDS || vdev->data.cam_info == DAUDIO_ADAS_PRK || vdev->data.cam_info == DAUDIO_DVRS_RVM) {
		VIOC_WDMA_SetImageY2RMode(pWDMA, 2);
		VIOC_WDMA_SetImageY2REnable(pWDMA, 1);
	}

	VIOC_WDMA_SetImageBase(pWDMA, addrY, 0, 0);
	VIOC_WDMA_SetImageFormat(pWDMA, fmt);
	VIOC_WDMA_SetImageSize(pWDMA, width, height);
	VIOC_WDMA_SetImageOffset(pWDMA, fmt, width);
	VIOC_WDMA_SetImageEnable(pWDMA, ON);
	VIOC_WDMA_SetImageUpdate(pWDMA);
	
	return 0;
}

int direct_display_set_rdma(struct tcc_camera_device * vdev, unsigned int addrY, unsigned int width, unsigned int height, unsigned int fmt) {
	VIOC_RDMA * pRDMA = (VIOC_RDMA *)vdev->vioc.rdma.address;
	
	log("RDMA[%d] = 0x%08x, size = %d * %d, FMT = %d\n", vdev->vioc.rdma.index, (unsigned int)pRDMA, width, height, fmt);

	VIOC_RDMA_SetImageFormat(pRDMA, fmt);
	VIOC_RDMA_SetImageSize(pRDMA, width, height);
	VIOC_RDMA_SetImageOffset(pRDMA, fmt, width);
	VIOC_RDMA_SetImageBase(pRDMA, addrY, 0, 0);
	VIOC_RDMA_SetImageEnable(pRDMA);
	VIOC_RDMA_SetImageUpdate(pRDMA);
	
	return 0;
}

int direct_display_set_display(struct tcc_camera_device * vdev, unsigned int pos_x, unsigned int pos_y, unsigned int on_off) {
	VIOC_WMIX * pWMIX = (VIOC_WMIX *)vdev->vioc.wmixer_out.address;
	unsigned int num_layer = 1; //vdev->vioc.rdma.index; For 8 inch LCD models(ex-JSN) rdma & wmix number was set different in buildtime. => RearCamera always started from (0 = pos_x,0 = pos_y) in Set Display menu 
	
	log("WMIX[%d] = 0x%08x\n", vdev->vioc.wmixer_out.index, (unsigned int)pWMIX);
	
	VIOC_WMIX_SetPosition(pWMIX, num_layer, pos_x, pos_y);
	if(on_off) {
		VIOC_WMIX_SetOverlayPriority(pWMIX, 24);
	}
	else {
		VIOC_WMIX_SetOverlayPriority(pWMIX, 24);
	}
	VIOC_WMIX_SetUpdate(pWMIX);	
	return 0;
}

void direct_display_enable_async_fifo(struct tcc_camera_device * vdev, unsigned int nWDMA, unsigned int nRDMA0, unsigned int nRDMA1, unsigned int nRDMA2) {
	VIOC_FIFO * pFIFO = (VIOC_FIFO *)vdev->vioc.fifo.address;
	
	log("WDMA: 0x%x, RDMA0: 0x%x, RDMA1: 0x%x, RDMA2: 0x%x\n", nWDMA, nRDMA0, nRDMA1, nRDMA2);
	
	VIOC_ASYNC_FIFO_ConfigDMA(pFIFO, nWDMA, nRDMA0, nRDMA1, nRDMA2);
	VIOC_ASYNC_FIFO_ConfigEntry(pFIFO, vdev->addrPreview);
	VIOC_ASYNC_FIFO_SetEnable(pFIFO, 1, 1, 0, 0);
}

void direct_display_disable_async_fifo(struct tcc_camera_device * vdev) {
	VIOC_FIFO * pFIFO = (VIOC_FIFO *)vdev->vioc.fifo.address;
	
	VIOC_ASYNC_FIFO_SetEnable(pFIFO, 0, 0, 0, 0);
}

int direct_display_start(struct tcc_camera_device * vdev) {
#if !defined(CONFIG_TCC_REAR_CAMERA_DRV)
	VIOC_RDMA		* pPGL		= (VIOC_RDMA		*)vdev->vioc.pgl.address;
#endif//!defined(CONFIG_TCC_REAR_CAMERA_DRV)
	VIOC_DISP		* pDISP		= (VIOC_DISP		*)vdev->vioc.disp.address;
	
	unsigned int	lcdW = 0, lcdH = 0;
	unsigned int	outX = vdev->gParams.preview_x;
	unsigned int	outY = vdev->gParams.preview_y;
	unsigned int	outW = vdev->gParams.preview_width;
	unsigned int	outH = vdev->gParams.preview_height;
	unsigned int	offset = 0, idxBuf = 0;
	
	FUNCTION_IN
	
	VIOC_DISP_GetSize(pDISP, &lcdW, &lcdH);
	log("lcd: %d * %d\n", lcdW, lcdH);
	printk("out: (%d, %d), %d * %d\n", outX, outY, outW, outH);
	if(!lcdW || !lcdH)
		return -1;

	if(!outW || !outH) {
		vdev->gParams.preview_width = outW = lcdW;
		vdev->gParams.preview_height = outH = lcdH;
		vdev->gParams.preview_x = outX = 0;
		vdev->gParams.preview_y = outY = 0;
		printk("default preview size: %d * %d\n", outW, outH);
	}

	if(outX + outW > lcdW || outY + outH > lcdH || \
		outX < 0 || outY < 0) {
		pr_err("%s error - pos & size (lcd(%d x %x) pos and size(%d, %d)(%d, %d) \n", \
			__func__, lcdW, lcdH, outX, outY, outW, outH);
		return -1;
	}

	if(outX == -1 || outY == -1) {
		vdev->gParams.preview_x = outX = 0;
		vdev->gParams.preview_y = outY = 0;
	}

	if(vdev->gParams.handover) {
		log("### HANDOVER (Skip the VIOC initialization) ###\n");

#if !defined(CONFIG_TCC_REAR_CAMERA_DRV)
		// disable PGL.
		VIOC_RDMA_SetImageDisable(pPGL);
#endif//!defined(CONFIG_TCC_REAR_CAMERA_DRV)
		mdelay(100);

		vioc_intr_enable(vdev->data.vioc_vin_intr.id, vdev->data.vioc_vin_intr.bits);

		direct_display_set_display(vdev, 0, 0, ON);

		vdev->gParams.handover = 0;
		return 0;
	}
	
#if 0 /* TODO: this code is moved to direct_display_if_initialize() by mobis */
	// Open and configure camera sensor.
	sensor_if_open(vdev);
	sensor_if_change_mode_ex(vdev->gParams.camera_type, vdev->gParams.camera_encode, vdev);
	mdelay(100);
#endif
	mdelay(50);

	// allocate preview memory
	vdev->addrPreview = (unsigned int *)kmalloc((sizeof(unsigned int) * PREVIEW_BUFFER_NUMBER), GFP_KERNEL);
	offset = outW * outH * 4;
	for(idxBuf=0; idxBuf < PREVIEW_BUFFER_NUMBER; idxBuf++) {
		vdev->addrPreview[idxBuf] = vdev->pmap_preview.base + (offset * idxBuf);
		log("addrPreview[%d] = 0x%08x\n", idxBuf, vdev->addrPreview[idxBuf]);
	}
	
	// reset vioc component
	direct_display_reset_vioc_path(vdev);
	
	if(direct_display_set_port(vdev) < 0) {
		printk("%s - FAILED: direct_display_set_port\n", __func__);
		return -1;
	}
	
#if defined(CONFIG_TCC_REAR_CAMERA_DRV)
	// set PGL
	direct_display_set_pgl(vdev, vdev->mLineBufAddr[vdev->mLineBufIndex],	\
					vdev->rcam_misc.parking_line_width,	\
					vdev->rcam_misc.parking_line_height,	\
					vdev->rcam_misc.parking_line_format);
#endif//defined(CONFIG_TCC_REAR_CAMERA_DRV)
	
	// set VIN
	direct_display_set_vin(vdev);
	
	// set Deinterlacer
	if(vdev->tcc_sensor_info.intl_en)
		direct_display_set_viqe(vdev);
	
	// set SCaler
	direct_display_set_scaler(vdev, outW, outH);
	
	// set WMIXer
	direct_display_set_wmix(vdev, outW, outH);
	
	// set WDMA
	direct_display_set_wdma(vdev, vdev->addrPreview[0], outW, outH, VIOC_IMG_FMT_ARGB8888);
	msleep(50);

	// set DISPlay
	direct_display_set_display(vdev, outX, outY, ON);

	// set Display
	direct_display_set_rdma(vdev, vdev->addrPreview[0], outW, outH, VIOC_IMG_FMT_ARGB8888);
	
	// set AsyncFIFO
	direct_display_enable_async_fifo(vdev, vdev->vioc.wdma.index,	\
		vdev->vioc.rdma.index, vdev->vioc.rdma.index, vdev->vioc.rdma.index);

	FUNCTION_OUT
	return 0;
}

int direct_display_stop(struct tcc_camera_device * vdev) {
	VIOC_DISP	* pDISP	= (VIOC_DISP	*)vdev->vioc.disp.address;
	VIOC_RDMA	* pRDMA	= (VIOC_RDMA	*)vdev->vioc.rdma.address;
	VIOC_WDMA	* pWDMA	= (VIOC_WDMA	*)vdev->vioc.wdma.address;
#ifndef FEATURE_USE_DEINTLS_REAR_CAMERA
	VIQE		* pVIQE = (VIQE		*)vdev->vioc.viqe.address;
#endif
	VIOC_VIN	* pVIN	= (VIOC_VIN	*)vdev->vioc.vin.address;
#if defined(CONFIG_TCC_REAR_CAMERA_DRV)
	VIOC_RDMA	* pPGL	= (VIOC_RDMA	*)vdev->vioc.pgl.address;
#endif//defined(CONFIG_TCC_REAR_CAMERA_DRV)
	
	struct cif_c_t * data = (struct cif_c_t *)&vdev->data.cif_cfg;
	int		idxLoop = 0;

	FUNCTION_IN
	
	// disable AsyncFIFO
	direct_display_disable_async_fifo(vdev);
	
	// disable RDMA
	VIOC_RDMA_SetImageDisable(pRDMA);
	
	// Display Off(pos_x, pos_y are not used)
	direct_display_set_display(vdev, 0, 0, OFF);
	
	// disable WDMA
	VIOC_WDMA_SetIreqMask(pWDMA, VIOC_WDMA_IREQ_ALL_MASK, ON);	// disable WDMA interrupt
	VIOC_WDMA_SetImageDisable(pWDMA);
	for(idxLoop=0; idxLoop<20; idxLoop++) {
		if(pWDMA->uIRQSTS.nREG & VIOC_WDMA_IREQ_EOFR_MASK)
			break;
		else
			printk("!@#---- %s - WDMA is still writing\n", __func__);
		mdelay(5);
	}
	if(idxLoop == 20)  {
		printk("!@#---- %s - [WDMA:0x%p] is not disabled, STATUS: 0x%08x, CTRL: 0x%08x\n",
			__func__, pWDMA, (unsigned)pWDMA->uIRQSTS.nREG,  (unsigned)pWDMA->uCTRL.nREG);
	}
	
	// disable WMIX, but don't care
	
	// disable SCaler
	VIOC_CONFIG_PlugOut(VIOC_SC0 + vdev->vioc.scaler.index);
	
	// disable Deinterlacer
	if(data->intl_en) {
#ifdef FEATURE_USE_DEINTLS_REAR_CAMERA
		// disable Simple De-Interlacer
		VIOC_CONFIG_PlugOut(VIOC_DEINTLS);
#else
		// disable VIEQ
		VIOC_VIQE_SetControlEnable(pVIQE, OFF, OFF, OFF, OFF, OFF);
		VIOC_CONFIG_PlugOut(VIOC_VIQE);
#endif
	}
	
	// disable VIN
	vioc_intr_disable(vdev->data.vioc_vin_intr.id, vdev->data.vioc_vin_intr.bits);
	VIOC_VIN_SetEnable(pVIN, OFF);

#if defined(CONFIG_TCC_REAR_CAMERA_DRV)
	// disable PGL
	VIOC_RDMA_SetImageDisable(pPGL);
#endif//defined(CONFIG_TCC_REAR_CAMERA_DRV)
	
	// reset vioc component
	direct_display_reset_vioc_path(vdev);
	
	// Close sensor.
	sensor_if_cleanup(vdev);
	
	FUNCTION_OUT
	return 0;
}

int direct_display_monitor_thread(void * data) {
	struct tcc_camera_device * vdev = (struct tcc_camera_device *)data;
	VIOC_WDMA *pWDMA = (VIOC_WDMA *)vdev->vioc.wdma.address;
	VIOC_RDMA * pRDMA = (VIOC_RDMA *)vdev->vioc.rdma.address;
	
	volatile unsigned int prev_addr, curr_addr;
	volatile unsigned int nCheck, idxCheck, delay = 30U;

	bool bRecover;
	unsigned int nRdmaEnable = 0;

	FUNCTION_IN
	
	while(1) {
		nCheck = 17;

		bRecover = true;
		curr_addr = pWDMA->uCBASE;
		for(idxCheck=0; idxCheck<nCheck; idxCheck++) {
			if(kthread_should_stop())
				return 0;

			prev_addr = curr_addr;

			// Force RDMA 1 to be enabled if RDMA 1 is not set
			VIOC_RDMA_GetImageEnable(pRDMA, &nRdmaEnable);
			if(!nRdmaEnable)
			{
				VIOC_RDMA_SetImageEnable(pRDMA);
				printk("RDMA1 was recovered\n");
			}

			msleep(delay);

			curr_addr = pWDMA->uCBASE;

			if(prev_addr != curr_addr){
				bRecover = false;
				break;
			}
			else
				;//LOG("[%d] prev_addr: 0x%08x, curr_addr: 0x%08x\n", idxCheck, prev_addr, curr_addr);
		}
		if(bRecover) {
			printk("%s::Recovery mode will be entered.\n", __func__);
			vdev->cam_err.soc_dma_err = 1;
			//printk("cam_info: SoC DMA error\n");
			
			test_registers(vdev);

			mutex_lock(&vdev->dd_lock);
			
			direct_display_stop(vdev);

			// Open and configure camera sensor
			sensor_if_open(vdev);
			//sensor_if_change_mode_ex(vdev->gParams.camera_type, vdev->gParams.camera_encode, vdev);

			// Reduce thread termination delay (max: 100ms)
			if(kthread_should_stop()) {
				mutex_unlock(&vdev->dd_lock);
				break;
			}

			direct_display_start(vdev);
			
			mutex_unlock(&vdev->dd_lock);
		}
		else if(vdev->cam_err.soc_dma_err) {
			//printk("cam_info: SoC DMA normal\n");
			vdev->cam_err.soc_dma_err = 0;
		}
	}
	
	FUNCTION_OUT
	return 0;
}

int direct_display_start_monitor(struct tcc_camera_device * vdev) {
//	if(vdev->gParams.camera_type != 0 && vdev->gParams.camera_type != 1) // camera type 5/6/7 will return 0
//		return 0;

	vdev->threadRecovery = kthread_run(direct_display_monitor_thread, vdev, "threadRecovery"); // camera type 0 & 1 will do recovery from here
	if(IS_ERR_OR_NULL(vdev->threadRecovery)) {
		printk("%s - FAILED: kthread_run\n", __func__);
		vdev->threadRecovery = NULL;
		return -1;
	}
	return 0;
}

int direct_display_stop_monitor(struct tcc_camera_device * vdev) {
//	if(vdev->gParams.camera_type != 0 && vdev->gParams.camera_type != 1) // camera type 5/6/7 will return 0
//		return 0;

	if(IS_ERR_OR_NULL(vdev->threadRecovery)) { // camera type 0 & 1 will do recovery from here
		printk("%s - FAILED: threadRecovery is NULL\n", __func__);
		return -1;
	}
	else {
		if(kthread_stop(vdev->threadRecovery) != 0)
			printk("%s - FAILED: kthread_stop\n", __func__);
		vdev->threadRecovery = NULL;
	}
	return 0;
}

int direct_display_if_initialize(struct tcc_camera_device * vdev) {
	printk("%s\n",__func__);
	FUNCTION_IN
	
	pmap_get_info("rearcamera",		&vdev->pmap_preview);
#ifndef FEATURE_USE_DEINTLS_REAR_CAMERA
	pmap_get_info("rearcamera_viqe",	&vdev->pmap_viqe);
#endif
	
	if(direct_display_parse_device_tree(vdev) < 0) {
		printk("%s - FAILED: direct_display_parse_device_tree()\n", __func__);
		return -1;
	}
	if(direct_display_init_parameters(vdev) < 0) {
		printk("%s - FAILED: direct_display_init_parameters()\n", __func__);
		return -1;
	}

	sensor_if_api_connect(vdev, 0);

	if(!tcc_cm_ctrl_knock()) {
		// Open and configure camera sensor.
		sensor_if_open(vdev);
		//sensor_if_change_mode_ex(vdev->gParams.camera_type, vdev->gParams.camera_encode, vdev);
	}

	FUNCTION_OUT
	return 0;
}

int direct_display_if_start(DIRECT_DISPLAY_IF_PARAMETERS params, struct tcc_camera_device * vdev) {
	int ret = -1;
	
	printk("%s\n",__func__);
	FUNCTION_IN
	
	vdev->gParams = params;
	log("camera_type = 0x%x, camera_encode = 0x%x\n", vdev->gParams.camera_type, vdev->gParams.camera_encode);
	
	mutex_init(&vdev->dd_lock);
	mutex_lock(&vdev->dd_lock);

	if(!vdev->cam_irq) {
		if((tccxxx_cif_vin_irq_request(vdev)) < 0) {
			printk("FAILED to aquire camera-vin-irq.\n");
		}
		else { 
			vdev->cam_irq = ENABLE;
			log("cif_vin_irq_request success ! \n");
		}
	}

	// Start Camera Preview.
	ret = direct_display_start(vdev);
	
	mutex_unlock(&vdev->dd_lock);
	
	if(ret < 0) {
		printk("FAILED to rearcamera preview start. \n");
		return -1;
	}
	
	// Start monitor thread.
	if(direct_display_start_monitor(vdev) < 0) {
		printk("FAILED direct_display_start_monitor\n");
		return -1;
	}
	
	// increase streaming count
	vdev->preview_method = PREVIEW_DD;
	vdev->cam_streaming++;
	
	FUNCTION_OUT
	return 0;
}

int direct_display_if_stop(struct tcc_camera_device * vdev) {
	int ret = -1;
	
	printk("%s\n",__func__);
	FUNCTION_IN
	
	// Stop monitor thread.
	if(direct_display_stop_monitor(vdev) < 0) {
		printk("FAILED direct_display_stop_monitor\n");
		return -1;
	}
	
	mutex_lock(&vdev->dd_lock);
	
	// Stop Camera Preview.
	ret = direct_display_stop(vdev);

	if(vdev->cam_irq){
		tccxxx_cif_vin_irq_free(vdev);
	}
	vdev->cam_irq = DISABLE;

	mutex_unlock(&vdev->dd_lock);
	mutex_destroy(&vdev->dd_lock);
	
	if(ret < 0) {
		printk("FAILED to rearcamera preview stop. \n");
		return -1;
	}
	
	// decrease streaming count
	vdev->preview_method = PREVIEW_V4L2;
	vdev->cam_streaming--;
	
	FUNCTION_OUT
	return ret;
}

int direct_display_if_terminate(void) {
	printk("%s\n",__func__);
	FUNCTION_IN
	FUNCTION_OUT
	return 0;
}
#if defined(CONFIG_TCC_REAR_CAMERA_DRV)
static void rcam_dump_misc(struct tcc_camera_device * vdev) {
	printk("%-40s = %d\n", "rcam_misc.preview_x", vdev->rcam_misc.preview_x);
	printk("%-40s = %d\n", "rcam_misc.preview_y", vdev->rcam_misc.preview_y);

	printk("%-40s = %d\n", "rcam_misc.preview_width", vdev->rcam_misc.preview_width);
	printk("%-40s = %d\n", "rcam_misc.preview_height", vdev->rcam_misc.preview_height);

	printk("%-40s = %d\n", "rcam_misc.preview_format", vdev->rcam_misc.preview_format);

	printk("%-40s = %d\n", "rcam_misc.preview_additional_width", vdev->rcam_misc.preview_additional_width);
	printk("%-40s = %d\n", "rcam_misc.preview_additional_height", vdev->rcam_misc.preview_additional_height);

	printk("%-40s = %d\n", "rcam_misc.parking_line_x", vdev->rcam_misc.parking_line_x);
	printk("%-40s = %d\n", "rcam_misc.parking_line_y", vdev->rcam_misc.parking_line_y);

	printk("%-40s = %d\n", "rcam_misc.parking_line_width", vdev->rcam_misc.parking_line_width);
	printk("%-40s = %d\n", "rcam_misc.parking_line_height", vdev->rcam_misc.parking_line_height);

	printk("%-40s = %d\n", "rcam_misc.parking_line_format", vdev->rcam_misc.parking_line_format);

	printk("%-40s = %d\n", "vdev->gear_active_level", vdev->gear_active_level);
	printk("%-40s = %d\n", "vdev->gear_port", vdev->gear_port);
}

static int rcam_parse_misc(struct tcc_camera_device * vdev) {
	struct device_node * np = vdev->camera_np;
	struct device_node * rear_cam_np;
	unsigned int val;
	int result = -1;

	rear_cam_np = of_parse_phandle(np, "rear_cam_misc", 0);
	if(rear_cam_np < 1) {
		pr_err("count not find rear_cam_misc\r\n");
		result = -ENODEV;
		goto END_PROCESS;				 
	}

	result = of_property_read_u32(rear_cam_np,"telechips,preview_x",&val);
	if(result) { pr_err("cann't parse telechips,preview_x\r\n"); goto END_PROCESS; }
	vdev->rcam_misc.preview_x = val;

	result = of_property_read_u32(rear_cam_np,"telechips,preview_y",&val);
	if(result) { pr_err("cann't parse telechips,preview_y\r\n"); goto END_PROCESS; }
	vdev->rcam_misc.preview_y = val;

	result = of_property_read_u32(rear_cam_np,"telechips,preview_width",&val);
	if(result) { pr_err("cann't parse telechips,preview_width\r\n"); goto END_PROCESS; }
	vdev->rcam_misc.lcd_width = vdev->rcam_misc.preview_width = vdev->rcam_misc.draw_preview_width = val;

	result = of_property_read_u32(rear_cam_np,"telechips,preview_height",&val);
	if(result) { pr_err("cann't parse telechips,preview_height\r\n"); goto END_PROCESS; }
	vdev->rcam_misc.lcd_height = vdev->rcam_misc.preview_height = vdev->rcam_misc.draw_preview_height = val;

	result = of_property_read_u32(rear_cam_np,"telechips,preview_format",&val);
	if(result) { pr_err("cann't parse telechips,preview_format\r\n"); goto END_PROCESS; }
	vdev->rcam_misc.preview_format = val;

	result = of_property_read_u32(rear_cam_np,"telechips,preview_additional_width",&val);
	if(result) { pr_err("cann't parse telechips,preview_additional_width\r\n"); goto END_PROCESS; }
	vdev->rcam_misc.preview_additional_width = val;

	result = of_property_read_u32(rear_cam_np,"telechips,preview_additional_height",&val);
	if(result) { pr_err("cann't parse telechips,preview_additional_height\r\n"); goto END_PROCESS; }
	vdev->rcam_misc.preview_additional_height = val;

	result = of_property_read_u32(rear_cam_np,"telechips,parking_line_x",&val);
	if(result) { pr_err("cann't parse telechips,parking_line_x\r\n"); goto END_PROCESS; }
	vdev->rcam_misc.parking_line_x = val;

	result = of_property_read_u32(rear_cam_np,"telechips,parking_line_y",&val);
	if(result) { pr_err("cann't parse telechips,parking_line_y\r\n"); goto END_PROCESS; }
	vdev->rcam_misc.parking_line_y = val;

	result = of_property_read_u32(rear_cam_np,"telechips,parking_line_width",&val);
	if(result) { pr_err("cann't parse telechips,parking_line_width\r\n"); goto END_PROCESS; }
	vdev->rcam_misc.parking_line_width = vdev->rcam_misc.draw_parking_line_width = val;

	result = of_property_read_u32(rear_cam_np,"telechips,parking_line_height",&val);
	if(result) { pr_err("cann't parse telechips,parking_line_height\r\n"); goto END_PROCESS; }
	vdev->rcam_misc.parking_line_height = vdev->rcam_misc.draw_parking_line_height = val;

	result = of_property_read_u32(rear_cam_np,"telechips,parking_line_format",&val);
	if(result) { pr_err("cann't parse telechips,parking_line_format\r\n"); goto END_PROCESS; }
	vdev->rcam_misc.parking_line_format = val;

	if(debug)
		rcam_dump_misc(vdev);

END_PROCESS:
	return result;
}

void rcam_get_line_buf_addr_by_index(struct tcc_camera_device * vdev, RCAM_LINE_BUFFER_INFO * rcam_line_buffer_info) {
	unsigned long index;

	index = rcam_line_buffer_info->index;

	rcam_line_buffer_info->rear_linebuf_addr = vdev->mLineBufAddr[index];
	rcam_line_buffer_info->buffer_size = vdev->mLineBufAddr[1] - vdev->mLineBufAddr[0];
	
	log("index = %ld, lineBufAddr = 0x%x, bufferSize = %ld \n",	\
		rcam_line_buffer_info->index, (unsigned)rcam_line_buffer_info->rear_linebuf_addr, rcam_line_buffer_info->buffer_size);
}

void rcam_line_buf_update(struct tcc_camera_device * vdev, RCAM_LINE_BUFFER_UPDATE_INFO * pInfo) {
	VIOC_RDMA	* pPGL		= (VIOC_RDMA	*)vdev->vioc.pgl.address;
	VIOC_WMIX	* pWMIX		= (VIOC_WMIX	*)vdev->vioc.wmixer.address;
	
	printk("%s, bufnum=%d\n", __func__, (unsigned int)pInfo->mBufnum);
	if(viocmg_is_feature_rear_cam_use_parking_line()) {
		if(pInfo->mBufnum < 2) {
			/* parking line buff change */
			vdev->mLineBufIndex = pInfo->mBufnum;
			
			/* parking line size set(width, height, offset) */
			vdev->rcam_misc.parking_line_width	= pInfo->mSize.width;
			vdev->rcam_misc.parking_line_height	= pInfo->mSize.height;
			
			/* parking line position set */
			vdev->rcam_misc.parking_line_x = pInfo->mPos.line_x;
			vdev->rcam_misc.parking_line_y = pInfo->mPos.line_y;
			
			if((vdev->rcam_misc.lcd_width - vdev->rcam_misc.parking_line_x) < (vdev->rcam_misc.parking_line_width))
				vdev->rcam_misc.parking_line_width = vdev->rcam_misc.lcd_width - vdev->rcam_misc.parking_line_x;
			
			if((vdev->rcam_misc.lcd_height - vdev->rcam_misc.parking_line_y) < (vdev->rcam_misc.parking_line_height))
				vdev->rcam_misc.parking_line_height = vdev->rcam_misc.lcd_height - vdev->rcam_misc.parking_line_y;
			
//			if(vdev->preview_method == PREVIEW_DD) {
				VIOC_RDMA_SetImageBase(pPGL, vdev->mLineBufAddr[pInfo->mBufnum], vdev->mLineBufAddr[pInfo->mBufnum], vdev->mLineBufAddr[pInfo->mBufnum]);
				VIOC_RDMA_SetImageSize(pPGL, vdev->rcam_misc.parking_line_width, vdev->rcam_misc.parking_line_height);
				VIOC_RDMA_SetImageOffset(pPGL, vdev->rcam_misc.parking_line_format, vdev->rcam_misc.parking_line_width);
				VIOC_WMIX_SetPosition(pWMIX, 1, vdev->rcam_misc.parking_line_x, vdev->rcam_misc.parking_line_y);
			
				VIOC_RDMA_SetImageUpdate(pPGL);
				VIOC_WMIX_SetUpdate(pWMIX);
//			}
		}
	}
}

void rcam_parse_vioc_dt_data(struct tcc_camera_device * vdev) {
	rcam_parse_misc(vdev);
}

int rcam_process_pmap(struct tcc_camera_device * vdev) {
	unsigned int pg_pffset;
	int ret = -1;
	
	/* parking_guide */
	ret = pmap_get_info("parking_gui", &vdev->pmap_pgl);
	if(ret && (vdev->pmap_pgl.size > 0)){
		log("[PMAP] %s: 0x%08x ~ 0x%08x (0x%08x)\n", vdev->pmap_pgl.name, vdev->pmap_pgl.base, vdev->pmap_pgl.size);
		
		pg_pffset = (((vdev->rcam_misc.parking_line_width * vdev->rcam_misc.parking_line_height) << 2) + 4096) & (~4095);
		vdev->mLineBufAddr[0] = vdev->pmap_pgl.base;
		vdev->mLineBufAddr[1] = vdev->pmap_pgl.base + pg_pffset;
		
		if(vdev->pmap_pgl.size < (pg_pffset << 1)){
			printk(" >> %s	parking-guied-line buffer is small %d, need %d bytes  \n", __func__, vdev->pmap_pgl.size,(pg_pffset << 1));
			return -ENODEV;
		}
		
		log("[%s] mLineBufAddr 0x%x 0x%x  \n", __func__, \
				(unsigned int)vdev->mLineBufAddr[0], \
				(unsigned int)vdev->mLineBufAddr[1]);
	} else {
		pr_err("error..!! empty parking gui memory..!!\r\n");
		return -ENODEV;
	}
	return 0;
}
#endif//defined(CONFIG_TCC_REAR_CAMERA_DRV)

