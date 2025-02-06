/*
 * linux/drivers/spi/tcc_gpsb_tsif.c
 *
 * Copyright (C) 2010 Telechips, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/irq.h>
#include <linux/dma-mapping.h>
#include <linux/poll.h>
#include <linux/spi/spi.h>
#include <linux/sched.h>
#include <linux/of.h>
#include <linux/of_gpio.h>

#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/dma.h>
//#include <asm/mach-types.h>

//#include <asm/system.h>
//#include <mach/bsp.h>
//#include <mach/gpio.h>
//#include <plat/globals.h>
//#include <mach/iomap.h>
#include <linux/spi/tcc_tsif.h>
#include "tca_spi.h"
#include "tsdemux/TSDEMUX_sys.h"

#include <linux/cdev.h>

#define tcc_tsif_writel	__raw_writel
#define tcc_tsif_readl	__raw_readl

/***************************************tcc_tsif_devices global variables***************************************/
struct class *tsif_class;
static struct cdev tsif_device_cdev;
int tsif_major_num = 0;
int gpsb_tsif_num = 0;
int hwdmx_tsif_num = 0;
#ifdef SUPPORT_BLOCK_TSIF
int block_tsif_num = 0;
#endif
#define MAX_SUPPORT_TSIF_DEVICE 8
static int tsif_mode[MAX_SUPPORT_TSIF_DEVICE]= {0, };
int *pTsif_mode = &tsif_mode[0];

EXPORT_SYMBOL(tsif_class);
EXPORT_SYMBOL(tsif_major_num);
EXPORT_SYMBOL(gpsb_tsif_num);
EXPORT_SYMBOL(hwdmx_tsif_num);
EXPORT_SYMBOL(pTsif_mode);

/***************************************tcc_tsif_devices global variables***************************************/

//#define USE_STATIC_DMA_BUFFER
#if defined(CONFIG_DAUDIO_KK)
#define iTotalDriverHandle 3
#else
#define iTotalDriverHandle 2
#endif
#define ALLOC_DMA_SIZE 0x100000

extern void __iomem *devm_ioremap(struct device *dev, resource_size_t offset,
			   unsigned long size);

static struct tea_dma_buf *g_static_dma;
static char g_use_tsif_export_ioctl = 0;

static struct clk *gpsb_hclk[iTotalDriverHandle];
static struct clk *gpsb_pclk[iTotalDriverHandle];
struct fo {
    int minor;
};
struct fo fodevs[iTotalDriverHandle];

#define	MAX_PCR_CNT 2
struct tca_spi_pri_handle {
    wait_queue_head_t wait_q;
    struct mutex mutex;
    int open_cnt;
    u32 gpsb_port;
    u32 gpsb_channel;
	void __iomem *reg_base;
    resource_size_t phy_reg_base;
    void __iomem *port_reg;
	void __iomem *pid_reg;
    u32 drv_major_num;
    u32 pcr_pid[MAX_PCR_CNT];
    u32 bus_num;
    u32 irq_no;
    u32 is_suspend;  //0:not in suspend, 1:in suspend
    u32 packet_read_count;
#if defined(CONFIG_SPI_DEBUG_BUS_SYSFS)
    int packet_size;
    u32 read_byte;
#endif
    const char *name;

	// GDMA Configuration
#ifdef TCC_DMA_ENGINE
	/* DMA-engine specific */
	struct tcc_spi_dma	dma;
#endif
	struct tca_spi_gdma_config gdma;
	unsigned int gdma_use;
	struct device *dev;
};

static struct tca_spi_port_config port_cfg[iTotalDriverHandle];
static tca_spi_handle_t tsif_handle[iTotalDriverHandle];
static struct tca_spi_pri_handle tsif_pri[iTotalDriverHandle];

static int tcc_gpsb_tsif_init(int id);
static void tcc_gpsb_tsif_deinit(int id);

static int tcc_gpsb_tsif_probe(struct platform_device *pdev);
static int tcc_gpsb_tsif_remove(struct platform_device *pdev);
static int tcc_gpsb_tsif_open(struct inode *inode, struct file *filp);
static int tcc_gpsb_tsif_release(struct inode *inode, struct file *filp);
static long tcc_gpsb_tsif_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
static ssize_t tcc_gpsb_tsif_read(struct file *filp, char *buf, size_t len, loff_t *ppos);
static ssize_t tcc_gpsb_tsif_write(struct file *filp, const char *buf, size_t len, loff_t *ppos);
static unsigned int tcc_gpsb_tsif_poll(struct file *filp, struct poll_table_struct *wait);
static int tcc_gpsb_tsif_set_external_tsdemux(struct file *filp, int (*decoder)(char *p1, int p1_size, char *p2, int p2_size, int devid), int max_dec_packet, int devid);
static int tcc_gpsb_tsif_buffer_flush(struct file *filp, unsigned long x, int size);

struct file_operations tcc_gpsb_tsif_fops =
{
    .owner          = THIS_MODULE,
    .read           = tcc_gpsb_tsif_read,
    .write          = tcc_gpsb_tsif_write,
    .unlocked_ioctl = tcc_gpsb_tsif_ioctl,
    .compat_ioctl	= tcc_gpsb_tsif_ioctl,
    .open           = tcc_gpsb_tsif_open,
    .release        = tcc_gpsb_tsif_release,
    .poll           = tcc_gpsb_tsif_poll,
};

//External Decoder :: Send packet to external kernel ts demuxer
typedef struct
{
    int is_active; //0:don't use external demuxer, 1:use external decoder
    int index;
    int call_decoder_index;
    int (*tsdemux_decoder)(char *p1, int p1_size, char *p2, int p2_size, int devid);
}tsdemux_extern_t;
static tsdemux_extern_t tsdemux_extern_handle[iTotalDriverHandle];
static int tsif_get_readable_cnt(tca_spi_handle_t *H);

static int tcc_gpsb_tsif_set_external_tsdemux(struct file *filp, int (*decoder)(char *p1, int p1_size, char *p2, int p2_size, int devid), int max_dec_packet, int devid)
{
    if(max_dec_packet == 0)
    {
        //turn off external decoding
        memset(&tsdemux_extern_handle[devid], 0x0, sizeof(tsdemux_extern_t));
        return 0;
    }

    if(tsdemux_extern_handle[devid].call_decoder_index != max_dec_packet)
    {
        tsdemux_extern_handle[devid].is_active = 1;
        tsdemux_extern_handle[devid].tsdemux_decoder = decoder;
        tsdemux_extern_handle[devid].index = 0;
        if(tsif_handle[devid].dma_intr_packet_cnt < max_dec_packet)
            tsdemux_extern_handle[devid].call_decoder_index = max_dec_packet; //every max_dec_packet calling isr, call decoder
        else
            tsdemux_extern_handle[devid].call_decoder_index = 1;
        printk("%s::%d::max_dec_packet[%d]int_packet[%d]\n", __func__, __LINE__, tsdemux_extern_handle[devid].call_decoder_index, tsif_handle[devid].dma_intr_packet_cnt);
    }

    return 0;
}

static int tcc_gpsb_tsif_buffer_flush(struct file *filp, unsigned long x, int size)
{
	return 0;
}

static char is_use_tsif_export_ioctl(void)
{
	return g_use_tsif_export_ioctl;
}

static ssize_t show_port(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "gpsb port : %d\n", tsif_pri[0].gpsb_port);

}
static ssize_t store_port(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	u32 port;

	port = simple_strtoul(buf, (char **)NULL, 16);
	/* valid port: 0xC, 0xD, 0xE */
	if (port > 20 ) {
		printk("tcc-tsif: invalid port! (use 0xc/d/e)\n");
		return -EINVAL;
	}

	tsif_pri[0].gpsb_port = port;
	return count;
}
static DEVICE_ATTR(tcc_port, S_IRUSR|S_IWUSR, show_port, store_port);

#if defined(CONFIG_SPI_DEBUG_BUS_SYSFS)
/**
 * show_info - attribute show func
 * @dev : device
 * @attr: device attribute
 * @buf : buffer to copy message
 * display spi transfer information
 */
static ssize_t show_packet(struct device *dev,struct device_attribute *attr, char *buf)
{

    return sprintf(buf, "gpsb[0] packet size : %d \n"
        "gpsb[0] read packet count : %d \n"
        "gpsb[0] read byte : %d \n"
	#if defined(INCLUDE_ISDB)
	"gpsb[1] packet size : %d\n"
	"gpsb[1] packet count : %d\n"
	"gpsb[1] read byte : %d\n"
	#endif
        ,tsif_pri[0].packet_size
        ,tsif_pri[0].packet_read_count
        ,tsif_pri[0].read_byte
	#if defined(INCLUDE_ISDB)
	,tsif_pri[1].packet_size
	,tsif_pri[1].packet_read_count
	,tsif_pri[1].read_byte
	#endif
	);
}

static ssize_t store_packet(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{

	return count;
}

static DEVICE_ATTR(packet_count, S_IRUSR|S_IWUSR, show_packet, store_packet);

#endif

#ifdef TCC_DMA_ENGINE
#include <linux/dmaengine.h>

static bool tcc_tsif_dma_filter(struct dma_chan *chan, void *pdata)
{
	struct tca_spi_handle *tspi = pdata;
	struct tcc_dma_slave *dma_slave;

	if(!tspi)
	{
		printk("\x1b[1;33m[%s:%d] tspi is NULL!!\x1b[0m\n", __func__, __LINE__);
		return false;
	}

	dma_slave = &tspi->dma.dma_slave;
	if(dma_slave->dma_dev == chan->device->dev) {
		chan->private = dma_slave;
		return true;
	}
	else {
		printk("\x1b[1;33m[%s:%d] dma_dev(%p) != dev(%p)\x1b[0m\n", __func__, __LINE__,
			dma_slave->dma_dev, chan->device->dev);
		return false;
	}
}

static void tcc_tsif_stop_dma(struct tca_spi_handle *tspi)
{
	if(tspi->dma.chan_rx)
		dmaengine_terminate_all(tspi->dma.chan_rx);
}

static void tcc_tsif_release_dma(struct tca_spi_handle *tspi)
{
	if(tspi->dma.chan_rx)
	{
		dma_release_channel(tspi->dma.chan_rx);
		tspi->dma.chan_rx = NULL;
	}
}

static int tcc_tsif_dma_submit(struct tca_spi_handle *tspi);
static void tcc_tsif_dma_rx_callback(void *data)
{
    struct tca_spi_handle *tspi = (struct tca_spi_handle *)data;
    struct tca_spi_pri_handle *tpri = (struct tca_spi_pri_handle *)tspi->private_data;


	tspi->cur_q_pos += tspi->dma_intr_packet_cnt;
	if(tspi->cur_q_pos >= tspi->dma_total_packet_cnt){
		tspi->cur_q_pos = 0;
	}
	wake_up(&(tpri->wait_q));

	tcc_tsif_dma_submit(tspi);
}

static void tcc_tsif_dma_slave_config_addr_width(struct dma_slave_config *slave_config, u8 bytes_per_word)
{
	// Set WSIZE
	slave_config->src_addr_width = DMA_SLAVE_BUSWIDTH_UNDEFINED;

	if(bytes_per_word == 4)
		slave_config->dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	else
		slave_config->dst_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
}

static int tcc_tsif_dma_slave_config(struct tca_spi_handle *tspi, struct dma_slave_config *slave_config, u8 bytes_per_word)
{
	int ret = 0;

	if(!tspi->dma.chan_rx)
	{
		printk("\x1b[1;33m[%s:%d] RX DMA channel is NULL!!\x1b[0m\n", __func__, __LINE__);
		return -EINVAL;
	}

	// Set BSIZE
	slave_config->dst_maxburst = SLV_GDMA_BSIZE;
	slave_config->src_maxburst = SLV_GDMA_BSIZE;

	// Set Address
	slave_config->src_addr = (dma_addr_t)tspi->phy_reg_base + TCC_GPSB_PORT;

	// Set Rx Channel
	slave_config->direction = DMA_DEV_TO_MEM;
	slave_config->slave_id	= tspi->gdma_config.rx_ext;	// DMA Ext. Request.
	tcc_tsif_dma_slave_config_addr_width(slave_config,bytes_per_word);
	if(dmaengine_slave_config(tspi->dma.chan_rx, slave_config))
	{
		printk("\x1b[1;33m[%s:%d] Failed to configrue rx dma channel. \x1b[0m\n", __func__, __LINE__);
		ret = -EINVAL;
	}

	return ret;
}

#define TCC_SLV_DMA_SG_LEN	1
static int tcc_tsif_dma_submit(struct tca_spi_handle *tspi)
{
	struct dma_chan *rxchan = tspi->dma.chan_rx;
	struct dma_async_tx_descriptor *rxdesc;
	struct dma_slave_config slave_config;
	dma_cookie_t cookie;
	u32 len;
    
	if(!rxchan)
	{
		printk("\x1b[1;33m[%s:%d] rxchan(%p) is NULL\x1b[0m\n", __func__, __LINE__,rxchan);	
		return -ENODEV;
	}

	len = tspi->dma_intr_packet_cnt*SPI_GDMA_PACKET_SIZE/(SLV_GDMA_BSIZE*SLV_GDMA_WSIZE);

	// Prepare the RX dma transfer
	sg_init_table(&tspi->dma.sgrx,TCC_SLV_DMA_SG_LEN);
	sg_dma_len(&tspi->dma.sgrx) = len;	// Set HOP Count
	sg_dma_address(&tspi->dma.sgrx) = tspi->rx_dma.dma_addr;

	// Config dma slave
	if(tcc_tsif_dma_slave_config(tspi, &slave_config, SLV_GDMA_WSIZE))
	{
		printk("\x1b[1;33m[%s:%d] Slave config failed.\x1b[0m\n", __func__, __LINE__);
		return -ENOMEM;
	}

	// Send scatterlist for RX
	rxdesc = dmaengine_prep_slave_sg(rxchan, &tspi->dma.sgrx, TCC_SLV_DMA_SG_LEN,
		DMA_DEV_TO_MEM,
		DMA_PREP_INTERRUPT | DMA_CTRL_ACK);
	if(!rxdesc)
	{
		printk("\x1b[1;33m[%s:%d] Preparing RX DMA Desc.\x1b[0m\n", __func__, __LINE__);
		goto err_dma;
	}

	rxdesc->callback = tcc_tsif_dma_rx_callback;
	rxdesc->callback_param = tspi;

	// Enable GPSB Interrupt
	if(SLV_GDMA_WSIZE == 4){
		tcc_tsif_writel(tcc_tsif_readl(tspi->regs + TCC_GPSB_INTEN) | (Hw25 | Hw24),tspi->regs + TCC_GPSB_INTEN);
	}else{
		tcc_tsif_writel(tcc_tsif_readl(tspi->regs + TCC_GPSB_INTEN) & ~(Hw25 | Hw24),tspi->regs + TCC_GPSB_INTEN);
	}

	// Submit desctriptors
	cookie = dmaengine_submit(rxdesc);
	if(dma_submit_error(cookie))
	{
		printk("\x1b[1;33m[%s:%d] RX Desc. submitting error! (cookie: %X) \x1b[0m\n", __func__, __LINE__,(unsigned int)cookie);
		goto err_dma;
	}

	// Issue pendings
	dma_async_issue_pending(rxchan);

	return 0;
err_dma:
	tcc_tsif_stop_dma(tspi);
	return -ENOMEM;
}

static int tcc_tsif_dma_configuration(struct platform_device *pdev, struct tca_spi_handle *tspi)
{
	struct dma_slave_config slave_config;
	struct device *dev = &pdev->dev;
	int ret;

	dma_cap_mask_t mask;
	dma_cap_zero(mask);
	dma_cap_set(DMA_SLAVE, mask);

	tspi->dma.chan_tx = NULL;

	tspi->dma.chan_rx = dma_request_slave_channel_compat(mask,tcc_tsif_dma_filter,&tspi->dma,dev,"rx");
	if(!tspi->dma.chan_rx)
	{
		printk("\x1b[1;33m[%s:%d] DMA RX channel request Error!\x1b[0m\n", __func__, __LINE__);
		ret = -EBUSY;
		goto error;
	}

	ret = tcc_tsif_dma_slave_config(tspi,&slave_config,SLV_GDMA_WSIZE);
	if(ret)
		goto error;

	printk("\x1b[1;33mUsing %s (RX) for DMA trnasfers\x1b[0m\n",
		dma_chan_name(tspi->dma.chan_rx));

	return 0;

error:
	tcc_tsif_release_dma(tspi);
	return ret;
}
#endif /* TCC_DMA_ENGINE */

#ifdef CONFIG_OF
static void tcc_gpsb_tsif_parse_dt(struct device_node *np, struct tca_spi_port_config *pgpios)
{
	pgpios->name = np->name;
    of_property_read_u32(np, "gpsb-id", &pgpios->gpsb_id);
    of_property_read_u32(np, "gpsb-port", &pgpios->gpsb_port);
}

static void tcc_tsif_gdma_parse_dt(struct device_node *np, struct tca_spi_gdma_config *pgdma)
{
	if(of_find_property(np, "gpsb-gdma-use", 0))
	{
		pgdma->gpsb_use_gdma = 1;
#ifndef TCC_DMA_ENGINE
		// GDMA Tx/Rx Controller
	    of_property_read_u32(np, "gpsb-gdma-tx-ctrl", &pgdma->tx_ctrl);
	    of_property_read_u32(np, "gpsb-gdma-rx-ctrl", &pgdma->rx_ctrl);
		// GDMA Tx/Rx Channel of each Tx/Rx Controller
	    of_property_read_u32(np, "gpsb-gdma-tx-ch", &pgdma->tx_ch);
	    of_property_read_u32(np, "gpsb-gdma-rx-ch", &pgdma->rx_ch);
#endif
		// GDMA Tx/Rx External DMA Request Number
	    of_property_read_u32(np, "gpsb-gdma-tx-ext", &pgdma->tx_ext);
	    of_property_read_u32(np, "gpsb-gdma-rx-ext", &pgdma->rx_ext);
	}
	else
	{
		pgdma->gpsb_use_gdma = 0;
	}
}

#ifndef TCC_DMA_ENGINE
static void __iomem *tcc_tsif_gdma_base_ioremap_addr(struct platform_device *pdev,
	struct resource *regs, struct tca_spi_gdma_config *pgdma)
{
	void __iomem *retval;
	resource_size_t base_addr;

	base_addr = regs->start + (TCC_GDMA_CTRL_REG_OFFSET * pgdma->tx_ctrl);
	retval = devm_ioremap(&pdev->dev, base_addr, TCC_GPSB_GDMA_CHCONFIG+0x4);
	//printk("\x1b[1;33m[%s:%d] GDMA Base Addr : 0x%X\x1b[0m\n", __func__, __LINE__,(unsigned int)base_addr);

	return retval;
}

static void __iomem *tcc_tsif_gdma_ioremap_addr(struct platform_device *pdev,
	struct resource *regs, struct tca_spi_gdma_config *pgdma, int is_tx)
{
	void __iomem *retval;
	unsigned int ctrl, ch;
	resource_size_t base_addr;

	if(regs == NULL)
		return 0;

	base_addr = regs->start;
	ctrl = pgdma->rx_ctrl;
	ch = pgdma->rx_ch;
#if 0
	printk("\x1b[1;31m[%s:%d] ## TSIF IOREMAP GDMA ADDR (%s) ##\x1b[0m\n", __func__, __LINE__,"Rx");
	printk("\x1b[1;33m[%s:%d] GDMA Base Addr : 0x%016X\x1b[0m\n", __func__, __LINE__,base_addr);
	printk("\x1b[1;33m[%s:%d] GDMA Controller ID : 0x%02X\x1b[0m\n", __func__, __LINE__,ctrl);
	printk("\x1b[1;33m[%s:%d] GDMA Channel ID : 0x%02X\x1b[0m\n", __func__, __LINE__,ch);
#endif
	base_addr = regs->start + (TCC_GDMA_CTRL_REG_OFFSET * ctrl) + (TCC_GDMA_CH_REG_OFFSET * ch);
	retval = devm_ioremap(&pdev->dev, base_addr, TCC_GDMA_CH_REG_OFFSET);
#if 0
	printk("\x1b[1;32m[%s:%d] GDMA Addr : Phy@0x%016X\x1b[0m\n", __func__, __LINE__,base_addr);
	printk("\x1b[1;32m[%s:%d] GDMA Addr : Virt@0x%p\x1b[0m\n", __func__, __LINE__,retval);
#endif
	return retval;
}

static void tcc_tsif_gdma_config(tca_spi_handle_t *H)
{
	void __iomem *reg_base;
	unsigned int ext_num;
	unsigned int dma_buffer, gdma_hop_count;


	if(!H->gdma_config.gpsb_use_gdma)
	{
		printk("\x1b[1;31m[%s:%d] Error: GPSB ID %d is not available using GDMA.\x1b[0m\n", __func__, __LINE__,H->id);
		return;
	}

	reg_base = H->gdma_config.rx_reg;
	ext_num = H->gdma_config.rx_ext;

	if(SLV_GDMA_WSIZE == 4){
		tcc_tsif_writel(tcc_tsif_readl(H->regs + TCC_GPSB_INTEN) | (Hw25 | Hw24),H->regs + TCC_GPSB_INTEN);
	}else{
		tcc_tsif_writel(tcc_tsif_readl(H->regs + TCC_GPSB_INTEN) & ~(Hw25 | Hw24),H->regs + TCC_GPSB_INTEN);
	}

	dma_buffer = 0xFFFFFF00 / (((H->dma_total_packet_cnt*SPI_GDMA_PACKET_SIZE)>>4)<<8);
	dma_buffer = dma_buffer * (((H->dma_total_packet_cnt*SPI_GDMA_PACKET_SIZE)>>4)<<8);

	// Disable GDMA
	tcc_tsif_writel(tcc_tsif_readl(reg_base+TCC_GPSB_GDMA_CHCTRL) & ~Hw0, reg_base+TCC_GPSB_GDMA_CHCTRL);

	// Set Source Block Parameter Register
	tcc_tsif_writel(0x0, reg_base+TCC_GPSB_GDMA_SPARAM);
	tcc_tsif_writel((dma_buffer | SLV_GDMA_WSIZE), reg_base+TCC_GPSB_GDMA_DPARAM);

	// Set HOP Counter Register
	gdma_hop_count = H->dma_intr_packet_cnt*SPI_GDMA_PACKET_SIZE/(SLV_GDMA_BSIZE*SLV_GDMA_WSIZE);
	tcc_tsif_writel(gdma_hop_count, reg_base+TCC_GPSB_GDMA_HCOUNT);

	tcc_tsif_writel(tcc_tsif_readl(reg_base+TCC_GPSB_GDMA_CHCTRL) | (Hw1|Hw3|Hw8|Hw9|Hw11|Hw13|Hw15),reg_base+TCC_GPSB_GDMA_CHCTRL);

	// Set Burst Size
	if(SLV_GDMA_BSIZE == 1)
		tcc_tsif_writel(tcc_tsif_readl(reg_base+TCC_GPSB_GDMA_CHCTRL) & ~(Hw7 | Hw6) ,reg_base+TCC_GPSB_GDMA_CHCTRL);
	else if(SLV_GDMA_BSIZE == 2)
		tcc_tsif_writel(tcc_tsif_readl(reg_base+TCC_GPSB_GDMA_CHCTRL) | Hw6, reg_base+TCC_GPSB_GDMA_CHCTRL);
	else if(SLV_GDMA_BSIZE == 4)
		tcc_tsif_writel(tcc_tsif_readl(reg_base+TCC_GPSB_GDMA_CHCTRL) | Hw7, reg_base+TCC_GPSB_GDMA_CHCTRL);
	else if(SLV_GDMA_BSIZE == 8)
		tcc_tsif_writel(tcc_tsif_readl(reg_base+TCC_GPSB_GDMA_CHCTRL) | (Hw7 | Hw6),reg_base+TCC_GPSB_GDMA_CHCTRL);

	// Set Word Size
	if(SLV_GDMA_WSIZE == 1)
		tcc_tsif_writel(tcc_tsif_readl(reg_base+TCC_GPSB_GDMA_CHCTRL) & ~(Hw5 | Hw4), reg_base+TCC_GPSB_GDMA_CHCTRL);
	else if(SLV_GDMA_WSIZE == 2)
		tcc_tsif_writel(tcc_tsif_readl(reg_base+TCC_GPSB_GDMA_CHCTRL) | Hw4, reg_base+TCC_GPSB_GDMA_CHCTRL);
	else if(SLV_GDMA_WSIZE == 4)
		tcc_tsif_writel(tcc_tsif_readl(reg_base+TCC_GPSB_GDMA_CHCTRL) | Hw5, reg_base+TCC_GPSB_GDMA_CHCTRL);

	// Set Repeat Control Register
	tcc_tsif_writel(tcc_tsif_readl(reg_base+TCC_GPSB_GDMA_RPTCTRL) & ~Hw31, reg_base+TCC_GPSB_GDMA_RPTCTRL);

	// Set External Request Register
	tcc_tsif_writel(tcc_tsif_readl(reg_base+TCC_GPSB_GDMA_EXTREQ) & ~(Hw32-Hw0), reg_base+TCC_GPSB_GDMA_EXTREQ);	// Clear EXTREQ
	tcc_tsif_writel(tcc_tsif_readl(reg_base+TCC_GPSB_GDMA_EXTREQ) | (0x1 << ext_num), reg_base+TCC_GPSB_GDMA_EXTREQ);	// Set EXTREQ
}
#endif /* TCC_DMA_ENGINE */

#else
static void tcc_gpsb_tsif_parse_dt(struct device_node *np, struct tca_spi_port_config *pgpios)
{
}
#endif

static int tcc_gpsb_tsif_probe(struct platform_device *pdev)
{
    int ret = 0;
    int irq = -1;
    int id = -1;
    int i;
    struct resource *regs = NULL;
    struct resource *regs1 = NULL;
    struct resource *regs2 = NULL;
    struct tca_spi_port_config tmpcfg;
    struct tca_spi_gdma_config gdma;
#ifdef USE_STATIC_DMA_BUFFER
	if(g_static_dma == NULL) {
		g_static_dma = kmalloc(sizeof(struct tea_dma_buf), GFP_KERNEL);
		if(g_static_dma) {
			g_static_dma->buf_size = ALLOC_DMA_SIZE;
			g_static_dma->v_addr = dma_alloc_writecombine(0, g_static_dma->buf_size, &g_static_dma->dma_addr, GFP_KERNEL);
			printk("tcc-tsif : dma buffer alloc @0x%X(Phy=0x%X), size:%d\n", (unsigned int)g_static_dma->v_addr, (unsigned int)g_static_dma->dma_addr, g_static_dma->buf_size);
			if(g_static_dma->v_addr == NULL) {
				kfree(g_static_dma);
				g_static_dma = NULL;
			}
		}
	}
#endif

    if(!pdev->dev.of_node)
        return -EINVAL;

	// GPSB Register
    regs = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!regs) {
		dev_err(&pdev->dev,
			"Found SPI TSIF with no register addr. Check %s setup!\n",
			dev_name(&pdev->dev));
        return -ENXIO;
    }

	// Port Configuration Register
    regs1 = platform_get_resource(pdev, IORESOURCE_MEM, 1);
    if (!regs1) {
		dev_err(&pdev->dev,
			"Found SPI TSIF PCFG with no register addr. Check %s setup!\n",
			dev_name(&pdev->dev));
        return -ENXIO;
    }

	// PID Table Register
    regs2 = platform_get_resource(pdev, IORESOURCE_MEM, 2);
    if (!regs2) {
		dev_err(&pdev->dev,
			"Found SPI TSIF PID Table with no register addr. Check %s setup!\n",
			dev_name(&pdev->dev));
        return -ENXIO;
    }

    irq = platform_get_irq(pdev, 0);
    if (irq < 0) {
        return -ENXIO;
    }

#ifdef CONFIG_OF
    tcc_gpsb_tsif_parse_dt(pdev->dev.of_node, &tmpcfg);
	tcc_tsif_gdma_parse_dt(pdev->dev.of_node, &gdma);
#endif

    id = tmpcfg.gpsb_id;
    memset(&tsdemux_extern_handle[id], 0x0, sizeof(tsdemux_extern_t));
    memcpy(&port_cfg[id], &tmpcfg, sizeof(struct tca_spi_port_config));
    mutex_init(&(tsif_pri[id].mutex));

    pdev->id = id;
    port_cfg[id].name = regs->name+1;

    for (i=0; i < MAX_PCR_CNT; i++)
        tsif_pri[id].pcr_pid[i] = 0xFFFF;
    tsif_pri[id].drv_major_num = tsif_major_num;
    tsif_pri[id].bus_num = id;
    tsif_pri[id].irq_no = irq;
    tsif_pri[id].reg_base = devm_ioremap_resource(&pdev->dev,regs);// tcc_p2v(res->start);
    tsif_pri[id].phy_reg_base = regs->start;
    tsif_pri[id].port_reg = devm_ioremap(&pdev->dev, regs1->start, regs1->end - regs1->start + 1);// tcc_p2v(res->start);
    tsif_pri[id].pid_reg = devm_ioremap(&pdev->dev, regs2->start, regs2->end - regs2->start + 1);// tcc_p2v(res->start);
    tsif_pri[id].gpsb_port = port_cfg[id].gpsb_port;
    tsif_pri[id].name = port_cfg[id].name;
	tsif_pri[id].gdma_use = gdma.gpsb_use_gdma;	// Does GPSB use GDMA?
	tsif_pri[id].dev = &pdev->dev;
	//printk("\x1b[1;33m[%s:%d][ID: %d] reg_base: 0x%x (0x%x)\x1b[0m\n", __func__, __LINE__,pdev->id,tsif_pri[pdev->id].reg_base,regs->start);
	//printk("\x1b[1;33m[%s:%d][ID: %d] port_base: 0x%x (0x%x)\x1b[0m\n", __func__, __LINE__,pdev->id,tsif_pri[pdev->id].port_reg,regs1->start);
	//printk("\x1b[1;33m[%s:%d][ID: %d] port_base: 0x%x (0x%x)\x1b[0m\n", __func__, __LINE__,pdev->id,tsif_pri[pdev->id].pid_reg,regs2->start);

	if(gdma.gpsb_use_gdma)	// GDMA Register (if does not use GDMA Address is NULL)
	{
#ifndef TCC_DMA_ENGINE
		struct resource *regs3 = NULL;

		// GDMA Base Address
		regs3 = platform_get_resource(pdev, IORESOURCE_MEM, 3);

		// GDMA Register Addr
		gdma.reg_base = tcc_tsif_gdma_base_ioremap_addr(pdev,regs3,&gdma);
		gdma.tx_reg = tcc_tsif_gdma_ioremap_addr(pdev,regs3,&gdma,1);
		gdma.rx_reg = tcc_tsif_gdma_ioremap_addr(pdev,regs3,&gdma,0);
		if(regs3){
			// GDMA Irq Number
			gdma.gdma_irq = platform_get_irq(pdev, 1);
		    if (gdma.gdma_irq < 0) {
				printk("[%s:%d]Fail to gdma probing (need to gdma irq no - %d)\n", __func__, __LINE__,gdma.gdma_irq);
		        return -ENXIO;
		    }
			printk("GDMA - Addr[0x%p] TX[%d:%d:%d] RX[%d:%d:%d] IRQ[%d]\n",
				gdma.reg_base,gdma.tx_ctrl,gdma.tx_ch,gdma.tx_ext,
				gdma.rx_ctrl,gdma.rx_ch,gdma.rx_ext,gdma.gdma_irq);

			tsif_pri[id].gdma = gdma;
		}
#else
		printk("\x1b[1;32m[%s:%d] GPSB id: %d (GDMA)\x1b[0m\n", __func__, __LINE__,id );
		ret = tcc_tsif_dma_configuration(pdev, &tsif_handle[id]);
		if(ret != 0)
		{
			printk("\x1b[1;33m[%s:%d] tcc dma engine configration fail!!\x1b[0m\n", __func__, __LINE__);
			return -ENXIO;
		}

		tsif_pri[id].gdma = gdma;
#endif

	}
	else
	{
		printk("\x1b[1;31m[%s:%d] GPSB id: %d (Dedicated DMA)\x1b[0m\n", __func__, __LINE__, id );
	}

    printk(KERN_NOTICE "Telechips TSIF ID:%d, %p, %u, %s, %lu\n", id, (void __iomem *)regs->start, regs->end, regs->name, regs->flags);
    printk("[%s:%d]%d, %d, %p, %d, %s, ret = %d\n",__func__, __LINE__,
		pdev->id, irq, tsif_pri[id].reg_base, port_cfg[id].gpsb_port, port_cfg[id].name, ret);
    //printk(KERN_NOTICE "Telechips TSIF Bus:%d, IRQ:%d, Address:%x, port:%d, name:%s\n",
    //    tsif_pri[id].bus_num, tsif_pri[id].irq_no, tsif_pri[id].reg_base, tsif_pri[id].gpsb_port, tsif_pri[id].name);

    device_create(tsif_class, NULL, MKDEV(tsif_pri[id].drv_major_num, id), NULL, TSIF_DEV_NAMES, id);

    gpsb_tsif_num++;
    tsif_mode[id] = TSIF_MODE_GPSB;

    gpsb_hclk[id] = of_clk_get(pdev->dev.of_node, 1);
    gpsb_pclk[id] = of_clk_get(pdev->dev.of_node, 0);

    platform_set_drvdata(pdev, &tsif_handle[id]);

    /* TODO: device_remove_file(&pdev->dev, &dev_attr_tcc_port); */
    ret = device_create_file(&pdev->dev, &dev_attr_tcc_port);
#if defined(CONFIG_SPI_DEBUG_BUS_SYSFS)
    ret = device_create_file(&pdev->dev, &dev_attr_packet_count);
#endif
    //printk("[%s]%d: init port:%d re:%d\n", pdev->name, tsif_pri[id].gpsb_channel, tsif_pri[id].gpsb_port, ret);

    return 0;
}

static int tcc_gpsb_tsif_remove(struct platform_device *pdev)
{
	int id = pdev->id;

	if (gpsb_pclk[id] && gpsb_hclk[id]){
		clk_disable_unprepare(gpsb_pclk[id]);
		clk_disable_unprepare(gpsb_hclk[id]);
		clk_put(gpsb_pclk[id]);
		clk_put(gpsb_hclk[id]);
		gpsb_pclk[id] = NULL;
		gpsb_hclk[id] = NULL;
	}

	tsif_mode[id] = 0;
	gpsb_tsif_num--;
	device_destroy(tsif_class, MKDEV(tsif_major_num, id));

	if(g_static_dma)
	{
		dma_free_writecombine(0, g_static_dma->buf_size, g_static_dma->v_addr, g_static_dma->dma_addr);
		kfree(g_static_dma);
		g_static_dma = NULL;
	}

#ifdef TCC_DMA_ENGINE
	tcc_tsif_release_dma(&tsif_handle[id]);
#endif

	return 0;
}

static void tea_free_dma_linux(struct tea_dma_buf *tdma, struct device *dev)
{
#ifdef      SUPORT_USE_SRAM
#else    
    if(g_static_dma){        
        return;
    }
    
    if (tdma) {
        if (tdma->v_addr != 0) {
            dma_free_writecombine(dev, tdma->buf_size, tdma->v_addr, tdma->dma_addr);
        }
        memset(tdma, 0, sizeof(struct tea_dma_buf));
    }
#endif    
}

static int tea_alloc_dma_linux(struct tea_dma_buf *tdma, unsigned int size, struct device *dev)
{
#ifdef      SUPORT_USE_SRAM
    tdma->buf_size = SRAM_TOT_PACKET*MPEG_PACKET_SIZE;
    tdma->v_addr = (void *)SRAM_VIR_ADDR;
    tdma->dma_addr = (unsigned int)SRAM_PHY_ADDR;        
    printk("tcc_tsif: alloc DMA buffer @0x%X(Phy=0x%X), size:%d\n",
               (unsigned int)tdma->v_addr,
               (unsigned int)tdma->dma_addr,
               tdma->buf_size);
    return 0;
#else    
    int ret = -1;
     if(g_static_dma){        
        tdma->buf_size = g_static_dma->buf_size;
        tdma->v_addr = g_static_dma->v_addr;
        tdma->dma_addr = g_static_dma->dma_addr;
        return 0;
    }

    if (tdma) {
        tea_free_dma_linux(tdma,dev);
        tdma->buf_size = size;
        tdma->v_addr = dma_alloc_writecombine(dev, tdma->buf_size, &tdma->dma_addr, GFP_KERNEL);
        //printk("tcc_tsif: alloc DMA buffer @0x%X(Phy=0x%X), size:%d\n",
        //       (unsigned int)tdma->v_addr,
        //       (unsigned int)tdma->dma_addr,
        //       tdma->buf_size);
        ret = tdma->v_addr ? 0 : 1;
    }
    return ret;
#endif    
}

static irqreturn_t tcc_gpsb_tsif_dma_handler(int irq, void *dev_id)
{
    struct tca_spi_handle *tspi = (struct tca_spi_handle *)dev_id;
    struct tca_spi_pri_handle *tpri = (struct tca_spi_pri_handle *)tspi->private_data;
    void __iomem *gpsb_pcfg_reg = (void __iomem *)tspi->port_regs;//(void __iomem *)io_p2v(TCC_PA_GPSB_PORTCFG);
    unsigned long dma_done_reg = 0;
    int i, fCheckSTC;
    int id = tspi->id;
    int irq_idx = tspi->gpsb_channel;
	if(!tca_spi_is_use_gdma(tspi))	// if use dedicated DMA
	{
		// Check GPSB Core IRQ

	    if ((__raw_readl(gpsb_pcfg_reg+0xC) & (1<<((irq_idx*2)+1))) == 0)
	    {
	        return IRQ_NONE;
	    }
		dma_done_reg = tcc_tsif_readl(tspi->regs + TCC_GPSB_DMAICR);

		if (dma_done_reg & (Hw28 | Hw29)) {
			TCC_GPSB_BITSET(tspi->regs + TCC_GPSB_DMAICR, Hw29 | Hw28);
			tspi->cur_q_pos = (int)(tcc_tsif_readl(tspi->regs + TCC_GPSB_DMASTR) >> 17);
		    if(tsdemux_extern_handle[id].is_active)
		    {
		        if (tpri->open_cnt > 0)
		        {
		            if(tsdemux_extern_handle[id].tsdemux_decoder)
		            {

		                if( tspi->cur_q_pos == tsif_handle[id].q_pos )
		                {
		                    return IRQ_HANDLED;
		                }

		                if(++tsdemux_extern_handle[id].index >= tsdemux_extern_handle[id].call_decoder_index)
		                {
		                    char *p1 = NULL, *p2 = NULL;
		                    int p1_size = 0, p2_size = 0;
		                    if( tspi->cur_q_pos > tsif_handle[id].q_pos )
		                    {
#if defined(CONFIG_DAUDIO_KK)
		                        p1 = (char *)tsif_handle[id].rx_dma.v_addr + tsif_handle[id].q_pos*tsif_handle[id].packet_size;
		                        p1_size = (tspi->cur_q_pos - tsif_handle[id].q_pos)*tsif_handle[id].packet_size;
#else
		                        p1 = (char *)tsif_handle[id].rx_dma.v_addr + tsif_handle[id].q_pos*TSIF_PACKET_SIZE;
		                        p1_size = (tspi->cur_q_pos - tsif_handle[id].q_pos)*TSIF_PACKET_SIZE;
#endif
		                    }
		                    else
		                    {
#if defined(CONFIG_DAUDIO_KK)
		                        p1 = (char *)tsif_handle[id].rx_dma.v_addr + tsif_handle[id].q_pos*tsif_handle[id].packet_size;
		                        p1_size = (tspi->dma_total_packet_cnt - tsif_handle[id].q_pos)*tsif_handle[id].packet_size;

		                        p2 = (char *)tsif_handle[id].rx_dma.v_addr;
		                        p2_size = tspi->cur_q_pos*tsif_handle[id].packet_size;
#else
		                        p1 = (char *)tsif_handle[id].rx_dma.v_addr + tsif_handle[id].q_pos*TSIF_PACKET_SIZE;
		                        p1_size = (tspi->dma_total_packet_cnt - tsif_handle[id].q_pos)*TSIF_PACKET_SIZE;

		                        p2 = (char *)tsif_handle[id].rx_dma.v_addr;
		                        p2_size = tspi->cur_q_pos*TSIF_PACKET_SIZE;
#endif
		                    }
		                    if(tsdemux_extern_handle[id].tsdemux_decoder(p1, p1_size, p2, p2_size, id) == 0)
		                    {
		                        tsif_handle[id].q_pos = tspi->cur_q_pos;
		                        tsdemux_extern_handle[id].index = 0;
		                    }
		                }
		            }
		        }
		    }

		    if (tpri->open_cnt > 0) {
		        //Check PCR & Make STC
		        fCheckSTC = 0;
		        for (i=0; i < MAX_PCR_CNT; i++) {
		            if (tpri->pcr_pid[i] < 0x1FFF) {
		                fCheckSTC = 1;
		                break;
		            }
		        }
		        if(fCheckSTC) {
		            int start_pos, search_pos, search_size;
		            start_pos = tspi->cur_q_pos - tspi->dma_intr_packet_cnt;
		            if(start_pos > 0 ) {
		                search_pos = start_pos;
		                search_size = tspi->dma_intr_packet_cnt;
		            } else {
		                search_pos = 0;
		                search_size = tspi->cur_q_pos;
		            }
		            for (i=0; i < MAX_PCR_CNT; i++) {
		                if (tpri->pcr_pid[i] < 0x1FFF)
#if defined(CONFIG_DAUDIO_KK)
		                    TSDEMUX_MakeSTC((unsigned char *)tsif_handle[id].rx_dma.v_addr + 
                                    search_pos*tsif_handle[id].packet_size, search_size*tsif_handle[id].packet_size, tpri->pcr_pid[i], i );
                    }
#else
		                    TSDEMUX_MakeSTC((unsigned char *)tsif_handle[id].rx_dma.v_addr + search_pos*TSIF_PACKET_SIZE, search_size*TSIF_PACKET_SIZE, tpri->pcr_pid[i], i );
		            }
#endif
		        }
		        //check read count & wake_up wait_q
		        if(tsdemux_extern_handle[id].tsdemux_decoder == NULL)
		        {
		            if( tsif_get_readable_cnt(tspi) >= tpri->packet_read_count)
		                wake_up(&(tpri->wait_q));
		        }
		    }
		}
	}
	else	// if use GDMA
	{

		if(tca_spi_gdma_intr_ch(tspi) == tspi->gdma_config.rx_ch)
		{
			void __iomem *gdma_rx_regs;

			gdma_rx_regs = tspi->gdma_config.rx_reg;
			dma_done_reg = tcc_tsif_readl(gdma_rx_regs + TCC_GPSB_GDMA_CHCTRL);

			if (dma_done_reg & (Hw3)) {	//rx
				tcc_tsif_writel(tcc_tsif_readl(gdma_rx_regs + TCC_GPSB_GDMA_CHCTRL) | Hw3, gdma_rx_regs + TCC_GPSB_GDMA_CHCTRL);

				tspi->cur_q_pos += tspi->dma_intr_packet_cnt;
				if(tspi->cur_q_pos >= tspi->dma_total_packet_cnt){
					tspi->cur_q_pos = 0;
				}
				wake_up(&(tpri->wait_q));
			}
		}
		else
		{
	        return IRQ_NONE;
		}
	}
    return IRQ_HANDLED;
}

static int tsif_get_readable_cnt(tca_spi_handle_t *H)
{
    if (H) {
        int dma_pos = H->cur_q_pos;
        int q_pos = H->q_pos;
        int readable_cnt = 0;

        if (dma_pos > q_pos) {
            readable_cnt = dma_pos - q_pos;
        } else if (dma_pos < q_pos) {
            readable_cnt = H->dma_total_packet_cnt - q_pos;
            readable_cnt += dma_pos;
        } 
        return readable_cnt;
    }

    return 0;
}

static ssize_t tcc_gpsb_tsif_read(struct file *filp, char *buf, size_t len, loff_t *ppos)
{
    int readable_cnt = 0, copy_cnt = 0;
    int copy_byte = 0;
    int id = ((struct fo *)filp->private_data)->minor;
	int packet_size = 0;
//	int cmd_i=0;

	if(!tca_spi_is_use_gdma(&tsif_handle[id]))	// if use dedicated DMA
	{
		packet_size = TSIF_PACKET_SIZE;
	}
	else
	{
		packet_size = SPI_GDMA_PACKET_SIZE;
	}

#if defined(CONFIG_SPI_DEBUG_BUS_SYSFS)
	tsif_pri[id].packet_size = packet_size;
#endif

    readable_cnt = tsif_get_readable_cnt(&tsif_handle[id]);

    if (readable_cnt > 0) {

#if defined(CONFIG_DAUDIO_KK)
		if(!tca_spi_is_use_gdma(&tsif_handle[id]))	// if use dedicated DMA
		{
	        if(tsif_pri[id].packet_read_count != len/tsif_handle[id].packet_size)
	            printk("set packet_read_count=%d\n", len/tsif_handle[id].packet_size);
#ifndef CONFIG_SPI_TCC_SLAVE_TEST
	        tsif_pri[id].packet_read_count = len/tsif_handle[id].packet_size;
#endif
		}

        copy_byte = readable_cnt * tsif_handle[id].packet_size;
        if (copy_byte > len) {
            copy_byte = len;
        }

        copy_byte -= copy_byte % tsif_handle[id].packet_size;
        copy_cnt = copy_byte / tsif_handle[id].packet_size;
        copy_cnt -= copy_cnt % tsif_handle[id].dma_intr_packet_cnt;
        copy_byte = copy_cnt * tsif_handle[id].packet_size;
#if defined(CONFIG_SPI_DEBUG_BUS_SYSFS)
        tsif_pri[id].read_byte += copy_byte;
#endif
#else
		if(!tca_spi_is_use_gdma(&tsif_handle[id]))	// if use dedicated DMA
		{
	        if(tsif_pri[id].packet_read_count != len/packet_size)
	            printk("set packet_read_count=%d\n", (int)len/packet_size);
#ifndef CONFIG_SPI_TCC_SLAVE_TEST
	        tsif_pri[id].packet_read_count = len/packet_size;
#endif
		}

        copy_byte = readable_cnt * packet_size;
        if (copy_byte > len) {
            copy_byte = len;
        }

        copy_byte -= copy_byte % packet_size;
        copy_cnt = copy_byte / packet_size;
        copy_cnt -= copy_cnt % tsif_handle[id].dma_intr_packet_cnt;
        copy_byte = copy_cnt * packet_size;
#endif
        if (copy_cnt >= tsif_handle[id].dma_intr_packet_cnt) {
#if defined(CONFIG_DAUDIO_KK)
            int offset = tsif_handle[id].q_pos * tsif_handle[id].packet_size;
#ifdef TCC_DMA_ENGINE
            // When does not use repeat mode, rx_dma buf address offset should be zero.
            if(tca_spi_is_use_gdma(&tsif_handle[id]))
                offset = 0;
#endif /* TCC_DMA_ENGINE */
            if (copy_cnt > tsif_handle[id].dma_total_packet_cnt - tsif_handle[id].q_pos) {
                int first_copy_byte = (tsif_handle[id].dma_total_packet_cnt - tsif_handle[id].q_pos) * tsif_handle[id].packet_size;
                int first_copy_cnt = first_copy_byte / tsif_handle[id].packet_size;
                int second_copy_byte = (copy_cnt - first_copy_cnt) * tsif_handle[id].packet_size;
#else
            int offset = tsif_handle[id].q_pos * packet_size;
#ifdef TCC_DMA_ENGINE
            // When does not use repeat mode, rx_dma buf address offset should be zero.
            if(tca_spi_is_use_gdma(&tsif_handle[id]))
                offset = 0;
#endif /* TCC_DMA_ENGINE */
            if (copy_cnt > tsif_handle[id].dma_total_packet_cnt - tsif_handle[id].q_pos) {
                int first_copy_byte = (tsif_handle[id].dma_total_packet_cnt - tsif_handle[id].q_pos) * packet_size;
                int first_copy_cnt = first_copy_byte / packet_size;
                int second_copy_byte = (copy_cnt - first_copy_cnt) * packet_size;
#endif

                if (copy_to_user(buf, tsif_handle[id].rx_dma.v_addr + offset, first_copy_byte)) {
                    return -EFAULT;
                }
                if (copy_to_user(buf + first_copy_byte, tsif_handle[id].rx_dma.v_addr, second_copy_byte)) {
                    return -EFAULT;
                }

                tsif_handle[id].q_pos = copy_cnt - first_copy_cnt;
            } else {
                if (copy_to_user(buf, tsif_handle[id].rx_dma.v_addr + offset, copy_byte)) {
                    return -EFAULT;
                }else{
			//printk("copy_byte[%x], rx_addr[%x]\n", copy_byte,tsif_handle[0].rx_dma.v_addr);
		/*	
			for(cmd_i = 0; cmd_i <copy_byte; cmd_i++)
			{
				if(*((char *)(tsif_handle[0].rx_dma.v_addr + offset)) != 0xff){
				printk("0x%x ", *((char *)(tsif_handle[0].rx_dma.v_addr + offset + cmd_i)));
				if((cmd_i%10)==0)
					printk("\n");
			//printk("[STAT] STAT = 0x%08x\n", tcc_tsif_readl(tsif_handle[0].regs + TCC_GPSB_STAT));
				}
			}
		*/

		}

                tsif_handle[id].q_pos += copy_cnt;
                if (tsif_handle[id].q_pos >= tsif_handle[id].dma_total_packet_cnt) {
                    tsif_handle[id].q_pos = 0;
                }
            }
            return copy_byte;
        }
    }
    return 0;
}

static ssize_t tcc_gpsb_tsif_write(struct file *filp, const char *buf, size_t len, loff_t *ppos)
{
    //int id = ((struct fo *)filp->private_data)->minor;
    return 0;
}

static unsigned int tcc_gpsb_tsif_poll(struct file *filp, struct poll_table_struct *wait)
{
    int id = ((struct fo *)filp->private_data)->minor;

    if (tsif_get_readable_cnt(&tsif_handle[id]) >= tsif_pri[id].packet_read_count) {
        return	(POLLIN | POLLRDNORM);
    }

    poll_wait(filp, &(tsif_pri[id].wait_q), wait);
    if (tsif_get_readable_cnt(&tsif_handle[id]) >= tsif_pri[id].packet_read_count) {
        return	(POLLIN | POLLRDNORM);
    }

    return 0;
}

static ssize_t tcc_gpsb_tsif_copy_from_user(void *dest, void *src, size_t copy_size)
{
	int ret = 0;
	if(is_use_tsif_export_ioctl() == 1) {
		memcpy(dest, src, copy_size);
	} else {
		ret = copy_from_user(dest, src, copy_size);
	}
	return ret;
}

static ssize_t tcc_gpsb_tsif_copy_to_user(void *dest, void *src, size_t copy_size)
{
	int ret = 0;
	if(is_use_tsif_export_ioctl() == 1) {
		memcpy(dest, src, copy_size);
	} else {
		ret = copy_to_user(dest, src, copy_size);
	}
	return ret;
}

static long  tcc_gpsb_tsif_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    int id = ((struct fo *)filp->private_data)->minor;
	int packet_size = 0;

	//printk("%s::%d::0x%X\n", __func__, __LINE__, cmd);
	if(!tca_spi_is_use_gdma(&tsif_handle[id]))	// if use dedicated DMA
	{
		packet_size = TSIF_PACKET_SIZE;
	}
	else
	{
		packet_size = SPI_GDMA_PACKET_SIZE;
	}

    switch (cmd) {
    case IOCTL_TSIF_DMA_START:
    case IOCTL_TSIF_HWDMX_START:
        {
            struct tcc_tsif_param param;
            if (tcc_gpsb_tsif_copy_from_user(&param, (void *)arg, sizeof(struct tcc_tsif_param))) {
                printk("cannot copy from user tcc_tsif_param in IOCTL_TSIF_DMA_START !!! \n");
                return -EFAULT;
            }

#if defined(CONFIG_DAUDIO_KK)
            tsif_handle[id].packet_size = param.packet_size;
	    packet_size = param.packet_size;
            if (((tsif_handle[id].packet_size * param.ts_total_packet_cnt) > tsif_handle[id].dma_total_size)
                || (param.ts_total_packet_cnt <= 0)) {
                printk("so big ts_total_packet_cnt[%d:%d] !!! \n", (tsif_handle[id].packet_size * param.ts_total_packet_cnt), tsif_handle[id].dma_total_size);
                param.ts_total_packet_cnt = tsif_handle[id].dma_total_size/(tsif_handle[id].packet_size*param.ts_intr_packet_cnt);
            }
#else
            if (((TSIF_PACKET_SIZE * param.ts_total_packet_cnt) > tsif_handle[id].dma_total_size)
                || (param.ts_total_packet_cnt <= 0)) {
                printk("so big ts_total_packet_cnt[%d:%d] !!! \n", (packet_size * param.ts_total_packet_cnt), tsif_handle[id].dma_total_size);
                param.ts_total_packet_cnt = tsif_handle[id].dma_total_size/(packet_size*param.ts_intr_packet_cnt);
            }
#endif
	
	#if defined(CONFIG_SPI_DEBUG_BUS_SYSFS)
		tsif_pri[id].packet_size = packet_size;
	#endif

            if(param.ts_total_packet_cnt > 0x1fff) //Max packet is 0x1fff(13bit)
                param.ts_total_packet_cnt = 0x1fff;

#ifdef TCC_DMA_ENGINE
			tcc_tsif_stop_dma(&tsif_handle[id]);
#endif
            tsif_handle[id].dma_stop(&tsif_handle[id]);
	
            tca_spi_setCPHA(tsif_handle[id].regs, param.mode & SPI_CPHA);
            tca_spi_setCPOL(tsif_handle[id].regs, param.mode & SPI_CPOL);
            tca_spi_setCS_HIGH(tsif_handle[id].regs, param.mode & SPI_CS_HIGH);
            tca_spi_setLSB_FIRST(tsif_handle[id].regs, param.mode & SPI_LSB_FIRST);

            tsif_handle[id].dma_mode = param.dma_mode;
            if (tsif_handle[id].dma_mode == 0) {
                tsif_handle[id].set_dma_addr(&tsif_handle[id]);
                tsif_handle[id].set_mpegts_pidmode(&tsif_handle[id], 0);
            }

            tsif_handle[id].dma_total_packet_cnt = param.ts_total_packet_cnt;
            tsif_handle[id].dma_intr_packet_cnt = param.ts_intr_packet_cnt;

			if(tsif_handle[id].dma_total_packet_cnt == tsif_handle[id].dma_intr_packet_cnt)
			{
				printk("## Warning! total_packet_cnt[%d] and intr_packet_cnt[%d] is same!! ##\n## --> intr_packet_cnt is set to [%d] ##\n",
					tsif_handle[id].dma_total_packet_cnt,tsif_handle[id].dma_intr_packet_cnt,
					tsif_handle[id].dma_total_packet_cnt / TCC_GPSB_TSIF_DEF_INTR_INTERVAL);
				tsif_handle[id].dma_intr_packet_cnt = tsif_handle[id].dma_total_packet_cnt / TCC_GPSB_TSIF_DEF_INTR_INTERVAL;
			}

            tsif_pri[id].packet_read_count =  tsif_handle[id].dma_intr_packet_cnt;

            #ifdef      SUPORT_USE_SRAM
            tsif_handle[id].dma_total_packet_cnt = SRAM_TOT_PACKET;
            tsif_handle[id].dma_intr_packet_cnt = SRAM_INT_PACKET;
            #endif
            tsif_handle[id].clear_fifo_packet(&tsif_handle[id]);
            tsif_handle[id].q_pos = tsif_handle[id].cur_q_pos = 0;

			if(tca_spi_is_use_gdma(&tsif_handle[id]))	// if use GDMA
			{
#ifndef TCC_DMA_ENGINE
				// Configurate GDMA
				tcc_tsif_gdma_config(&tsif_handle[id]);
#else
				tcc_tsif_dma_submit(&tsif_handle[id]);
#endif
			}

#if defined(CONFIG_DAUDIO_KK)
            tsif_handle[id].set_packet_cnt(&tsif_handle[id], tsif_handle[id].packet_size);
            tsif_handle[id].set_bit_width(&tsif_handle[id], param.bit_width);
#else
            tsif_handle[id].set_packet_cnt(&tsif_handle[id], packet_size);
#endif
            printk("interrupt packet count [%u]\n", tsif_handle[id].dma_intr_packet_cnt);
            tsif_handle[id].dma_start(&tsif_handle[id]);
        }
        break;
		
    case IOCTL_TSIF_DMA_STOP:
    case IOCTL_TSIF_HWDMX_STOP:
#ifdef TCC_DMA_ENGINE
			tcc_tsif_stop_dma(&tsif_handle[id]);
#endif
            tsif_handle[id].dma_stop(&tsif_handle[id]);
        break;
		
    case IOCTL_TSIF_GET_MAX_DMA_SIZE:
        {
            struct tcc_tsif_param param;
#if defined(CONFIG_DAUDIO_KK)
            param.ts_total_packet_cnt = tsif_handle[id].dma_total_size / tsif_handle[id].packet_size;
#else
            param.ts_total_packet_cnt = tsif_handle[id].dma_total_size / TSIF_PACKET_SIZE;
#endif
            param.ts_intr_packet_cnt = 1;
            if (tcc_gpsb_tsif_copy_from_user((void *)arg, (void *)&param, sizeof(struct tcc_tsif_param))) {
                printk("cannot copy to user tcc_tsif_param in IOCTL_TSIF_GET_MAX_DMA_SIZE !!! \n");
                return -EFAULT;
            }
        }
        break;
		
    case IOCTL_TSIF_SET_PID:
        {
            struct tcc_tsif_pid_param param;
            if (tcc_gpsb_tsif_copy_from_user(&param, (void *)arg, sizeof(struct tcc_tsif_pid_param))) {
                printk("cannot copy from user tcc_tsif_pid_param in IOCTL_TSIF_SET_PID !!! \n");
                return -EFAULT;
            }
            ret = tca_spi_register_pids(&tsif_handle[id], param.pid_data, param.valid_data_cnt);
        } 
        break;
		
	case IOCTL_TSIF_DXB_POWER:
		{
			// the power control moves to tcc_dxb_control driver.
		}
		break;
	case IOCTL_TSIF_SET_PCRPID:		
	{
		struct tcc_tsif_pcr_param pcr_param;

		if (tcc_gpsb_tsif_copy_from_user((void *)&pcr_param, (void *)arg, sizeof(struct tcc_tsif_pcr_param)))
			return -EFAULT;
		if (pcr_param.index >= MAX_PCR_CNT) return -EFAULT;
		tsif_pri[id].pcr_pid[pcr_param.index] = pcr_param.pcr_pid;
		printk("Set PCR PID[0x%X]\n", tsif_pri[id].pcr_pid[pcr_param.index]);
		if( tsif_pri[id].pcr_pid[pcr_param.index] < 0x1FFF)
			TSDEMUX_Open(pcr_param.index);
		}		
		break;
	case IOCTL_TSIF_GET_STC:	
		{
		unsigned int uiSTC, index;

		if (tcc_gpsb_tsif_copy_from_user((void*)&index, (void*)arg, sizeof(int)))
			return -EFAULT;
		if (index >= MAX_PCR_CNT) return -EFAULT;
		uiSTC = TSDEMUX_GetSTC(index);
			//printk("STC %d\n", uiSTC);
			if (tcc_gpsb_tsif_copy_to_user((void *)arg, (void *)&uiSTC, sizeof(int))) {
                printk("cannot copy to user tcc_tsif_param in IOCTL_TSIF_GET_PCR !!! \n");
                return -EFAULT;
            }
		}
		break;	
    case IOCTL_TSIF_RESET:
        break;
    default:
        printk("tsif: unrecognized ioctl (0x%X)\n", cmd);
        ret = -EINVAL;
        break;
    }
    return ret;
}

static int tcc_gpsb_tsif_init(int id)
{
	int ret = 0, packet_size = 0;
	struct device *dev;

    //memset(&tsif_handle[id], 0, sizeof(tca_spi_handle_t));

#ifdef CONFIG_ARM64
	dev = tsif_pri[id].dev;
#else
	dev = NULL;
#endif

    if (tca_spi_init(&tsif_handle[id],
                    //(volatile struct tca_spi_regs *)tsif_pri[id].reg_base,
                    tsif_pri[id].reg_base,
                    tsif_pri[id].phy_reg_base,
                    tsif_pri[id].port_reg,
                    tsif_pri[id].pid_reg,
                    tsif_pri[id].irq_no,
                    tea_alloc_dma_linux,
                    tea_free_dma_linux,
                    ALLOC_DMA_SIZE,
                    tsif_pri[id].bus_num,
                    1,
                    &port_cfg[id],
                    &tsif_pri[id].gdma,
                    tsif_pri[id].name,
                    dev)) {
        printk("%s: tca_spi_init error !!!!!\n", __func__);
        ret = -EBUSY;
        goto err_spi;
    }
    
    tsif_handle[id].private_data = (void *)&tsif_pri[id];
    init_waitqueue_head(&(tsif_pri[id].wait_q));
    tsif_handle[id].clear_fifo_packet(&tsif_handle[id]);
    tsif_handle[id].dma_stop(&tsif_handle[id]);

    //init_waitqueue_head(&(tsif_pri[id].wait_q));
    //mutex_init(&(tsif_pri[id].mutex));

	if(!tca_spi_is_use_gdma(&tsif_handle[id]))	// if use dedicated DMA
	{
		packet_size = TSIF_PACKET_SIZE;
	}
	else	// if use GDMA
	{
		packet_size = SPI_GDMA_PACKET_SIZE;
	}
#if defined(CONFIG_SPI_DEBUG_BUS_SYSFS)
	tsif_pri[id].packet_size = packet_size;
#endif

#if defined(CONFIG_DAUDIO_KK)
    /**  
     * @author sjpark@cleinsoft     
     * @date 2014/12/15             
     * sets the packet size using the ioctl
     **/
    tsif_handle[id].packet_size = 0;
#else
    tsif_handle[id].dma_total_packet_cnt = tsif_handle[id].dma_total_size / packet_size;
#endif
    tsif_handle[id].dma_intr_packet_cnt = 1;

    tsif_handle[id].hw_init(&tsif_handle[id]);
	
	if(!tca_spi_is_use_gdma(&tsif_handle[id])) // if use dedicated DMA
	    ret = request_irq(tsif_handle[id].irq, tcc_gpsb_tsif_dma_handler, IRQF_SHARED, tsif_pri[id].name, &tsif_handle[id]);
#ifndef TCC_DMA_ENGINE
	else // if use GDMA
	    ret = request_irq(tsif_pri[id].gdma.gdma_irq, tcc_gpsb_tsif_dma_handler, IRQF_SHARED, tsif_pri[id].name, &tsif_handle[id]);
#endif

    if (ret) { 
        goto err_irq; 
    }

#if defined(CONFIG_DAUDIO_KK)
    /**  
     * @author sjpark@cleinsoft     
     * @date 2014/12/15             
     * sets the packet size using the ioctl
     **/
#else
    tsif_handle[id].set_packet_cnt(&tsif_handle[id], packet_size);
#endif
    
    return 0;

err_irq:
	
	if(!tca_spi_is_use_gdma(&tsif_handle[id])) // if use dedicated DMA
		free_irq(tsif_handle[id].irq, &tsif_handle[id]);
#ifndef TCC_DMA_ENGINE
	else
		free_irq(tsif_pri[id].gdma.gdma_irq, &tsif_handle[id]);
#endif

err_spi:
    tca_spi_clean(&tsif_handle[id]);
    return ret;
}

static void tcc_gpsb_tsif_deinit(int id)
{
	if(!tca_spi_is_use_gdma(&tsif_handle[id])) // if use dedicated DMA
    free_irq(tsif_handle[id].irq, &tsif_handle[id]);
#ifndef TCC_DMA_ENGINE
	else
		free_irq(tsif_pri[id].gdma.gdma_irq, &tsif_handle[id]);
#endif
    tca_spi_clean(&tsif_handle[id]);
}

static int tcc_gpsb_tsif_open(struct inode *inode, struct file *filp)
{
    int i, major_number = 0, minor_number = 0;
    int ret = -1;
    struct fo *fodp = NULL;
    struct pinctrl *pinctrl;

    if (inode)
    {
        major_number = imajor(inode);
        minor_number = iminor(inode);
        fodp = &fodevs[minor_number];
        fodp->minor = minor_number;
    }

    if(filp)
    {
        filp->f_op = &tcc_gpsb_tsif_fops;
        filp->private_data = (void *) fodp;
    }

    if (tsif_pri[minor_number].open_cnt == 0)
        tsif_pri[minor_number].open_cnt++;
    else
        return -EBUSY;

    if (IS_ERR(gpsb_pclk[minor_number]) ||IS_ERR(gpsb_hclk[minor_number]) ) {
        printk(KERN_WARNING "TSIF#%d: failed to get gpsb clock\n", minor_number);
        return -EINVAL;
    }

    clk_prepare_enable(gpsb_pclk[minor_number]);
    clk_prepare_enable(gpsb_hclk[minor_number]);

    pinctrl = pinctrl_get_select(tsif_pri[minor_number].dev, "active");
	if(IS_ERR(pinctrl))
		printk("%s : pinctrl active error[0x%p]\n", __func__, pinctrl);

    mutex_lock(&(tsif_pri[minor_number].mutex));

    for (i=0; i < MAX_PCR_CNT; i++)
        tsif_pri[minor_number].pcr_pid[i] = 0xFFFF;

    ret = tcc_gpsb_tsif_init(minor_number);
    if(ret) {
		printk("%s : tcc_gpsb_tsif_init failed(%d)\n", __func__, ret);
		clk_disable_unprepare(gpsb_pclk[minor_number]);
		clk_disable_unprepare(gpsb_hclk[minor_number]);
		pinctrl = pinctrl_get_select(tsif_pri[minor_number].dev, "idle");
		if(IS_ERR(pinctrl))
			printk("%s : pinctrl idle error[0x%p]\n", __func__, pinctrl);
		else
			pinctrl_put(pinctrl);
    }

    mutex_unlock(&(tsif_pri[minor_number].mutex));
    
    return ret;
}

static int tcc_gpsb_tsif_release(struct inode *inode, struct file *filp)
{
    int major_number = 0, minor_number = 0;
    struct pinctrl *pinctrl;

    if (inode)
    {
        major_number = imajor(inode);
        minor_number = iminor(inode);
    }

    if (tsif_pri[minor_number].open_cnt > 0)
        tsif_pri[minor_number].open_cnt--;

    if (tsif_pri[minor_number].open_cnt == 0)
    {
        mutex_lock(&(tsif_pri[minor_number].mutex));
        tsif_handle[minor_number].dma_stop(&tsif_handle[minor_number]);
        tcc_gpsb_tsif_deinit(minor_number);
        TSDEMUX_Close();
        mutex_unlock(&(tsif_pri[minor_number].mutex));

        if (gpsb_pclk[minor_number] && gpsb_hclk[minor_number]){
            clk_disable_unprepare(gpsb_pclk[minor_number]);
            clk_disable_unprepare(gpsb_hclk[minor_number]);
		}

		pinctrl = pinctrl_get_select(tsif_pri[minor_number].dev, "idle");
		if(IS_ERR(pinctrl))
			printk("%s : pinctrl idle error[0x%p]\n", __func__, pinctrl);
		else
			pinctrl_put(pinctrl);
    }
    return 0;
}

/***************************************tcc_tsif_devices***************************************/
#ifdef CONFIG_OF
static struct of_device_id tcc_gpsb_tsif_of_match[] = {
	{ .compatible = "telechips,tcc89xx-tsif" },
	{}
};
MODULE_DEVICE_TABLE(of, tcc_gpsb_tsif_of_match);
#endif

static struct platform_driver tcc_gpsb_tsif_driver = {
	.probe	= tcc_gpsb_tsif_probe,
	.remove = tcc_gpsb_tsif_remove,
	.driver = {
		.name	= "tcc-gpsb-tsif",
		.owner	= THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = of_match_ptr(tcc_gpsb_tsif_of_match),
#endif
	},
};


extern int tcc_hwdmx_tsif_probe(struct platform_device *pdev);
extern int tcc_hwdmx_tsif_remove(struct platform_device *pdev);

#ifdef CONFIG_OF
static struct of_device_id tcc_hwdmx_tsif_of_match[] = {
	{ .compatible = "telechips,tcc89xx-hwdmx-tsif" },
	{}
};
MODULE_DEVICE_TABLE(of, tcc_hwdmx_tsif_of_match);
#endif

static struct platform_driver tcc_hwdmx_tsif_driver = {
	.probe	= tcc_hwdmx_tsif_probe,
	.remove = tcc_hwdmx_tsif_remove,
	.driver = {
		.name	= "tcc-hwdmx-tsif",
		.owner	= THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = of_match_ptr(tcc_hwdmx_tsif_of_match),
#endif
	},
};


#ifdef SUPPORT_BLOCK_TSIF
extern int tcc_block_tsif_probe(struct platform_device *pdev);
extern int tcc_block_tsif_remove(struct platform_device *pdev);

#ifdef CONFIG_OF
static struct of_device_id tcc_block_tsif_of_match[] = {
	{ .compatible = "telechips,tcc89xx-block-tsif" },
	{}
};
MODULE_DEVICE_TABLE(of, tcc_block_tsif_of_match);
#endif

static struct platform_driver tcc_block_tsif_driver = {
	.probe	= tcc_block_tsif_probe,
	.remove = tcc_block_tsif_remove,
	.driver = {
		.name	= "tcc-block-tsif",
		.owner	= THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = of_match_ptr(tcc_block_tsif_of_match),
#endif
	},
};
#endif

static void __exit tsif_exit(void)
{
	class_destroy(tsif_class);
	cdev_del(&tsif_device_cdev);
	unregister_chrdev_region(MKDEV(tsif_major_num, 0), TSIF_MINOR_NUMBER);

#ifdef SUPPORT_BLOCK_TSIF
	platform_driver_unregister(&tcc_block_tsif_driver);
#endif
	platform_driver_unregister(&tcc_hwdmx_tsif_driver);
	platform_driver_unregister(&tcc_gpsb_tsif_driver);
}

extern struct file_operations tcc_hwdmx_tsif_fops;

static int tcc_tsif_open( struct inode *inode, struct file *filp)
{
	if(MINOR(inode->i_rdev) >= MAX_SUPPORT_TSIF_DEVICE)	return -EBADF;
	switch(tsif_mode[MINOR(inode->i_rdev)])
	{
		case TSIF_MODE_GPSB: filp->f_op = &tcc_gpsb_tsif_fops;break;
#ifdef SUPPORT_BLOCK_TSIF
		case TSIF_MODE_BLOCK: filp->f_op = &tcc_block_tsif_fops;break;
#endif
		case TSIF_MODE_HWDMX: filp->f_op = &tcc_hwdmx_tsif_fops;break;
		default : return -ENXIO;
	}
#if !defined(CONFIG_TCC_CODESONAR_BLOCKED)
	if (filp->f_op->open)
		return filp->f_op->open(inode,filp);
#else
	if (filp->f_op && filp->f_op->open)
		return filp->f_op->open(inode,filp);
#endif
	return -EBADF;
}

struct file_operations tcc_tsif_fops =
{
	.owner			= THIS_MODULE,
	.open			= tcc_tsif_open,
};

static int __init tsif_init(void)
{
	int ret = 0;
	dev_t dev = 0;

	ret = alloc_chrdev_region(&dev, 0, TSIF_MINOR_NUMBER, "TSIF");
	tsif_major_num = MAJOR(dev);

	cdev_init(&tsif_device_cdev, &tcc_tsif_fops);
	if ((ret = cdev_add(&tsif_device_cdev, dev, TSIF_MINOR_NUMBER)) != 0) {
		printk(KERN_ERR "tsif: unable register character device\n");
		goto tsif_init_error;
	}

	tsif_class = class_create(THIS_MODULE, "tsif");
	if (IS_ERR(tsif_class)) {
		ret = PTR_ERR(tsif_class);
		goto tsif_init_error;
	}

	platform_driver_register(&tcc_gpsb_tsif_driver);
	platform_driver_register(&tcc_hwdmx_tsif_driver);
#ifdef SUPPORT_BLOCK_TSIF
	platform_driver_register(&tcc_block_tsif_driver);
#endif

	return 0;

tsif_init_error:
	cdev_del(&tsif_device_cdev);
	unregister_chrdev_region(dev, TSIF_MINOR_NUMBER);
	return ret;
}

extern int tcc_hwdmx_tsif_open(struct inode *inode, struct file *filp);

int tcc_tsif_open_ex(unsigned int tsif_type, struct inode *inode, struct file *filp)
{
	switch(tsif_type) {
		case TSIF_MODE_GPSB:
			return tcc_gpsb_tsif_open(inode, filp);
		break;
#ifdef SUPPORT_BLOCK_TSIF
		case TSIF_MODE_BLOCK:
			return tcc_block_tsif_open(inode, filp);
		break;
#endif
		case TSIF_MODE_HWDMX:
			return tcc_hwdmx_tsif_open(inode, filp);
		break;
		default:
		break;
	}
	return -EFAULT;
}

extern int tcc_hwdmx_tsif_release(struct inode *inode, struct file *filp);

int tcc_tsif_release_ex(unsigned int tsif_type, struct inode *inode, struct file *filp)
{
	switch(tsif_type) {
		case TSIF_MODE_GPSB:
			return tcc_gpsb_tsif_release(inode, filp);
		break;
#ifdef SUPPORT_BLOCK_TSIF
		case TSIF_MODE_BLOCK:
			return tcc_block_tsif_release(inode, filp);
		break;
#endif
		case TSIF_MODE_HWDMX:
			return tcc_hwdmx_tsif_release(inode, filp);
		break;
		default:
		break;
	}
	return -EFAULT;
}

extern int tcc_hwdmx_tsif_ioctl_ex(struct file *filp, unsigned int cmd, unsigned long arg);

int tcc_tsif_ioctl_ex(unsigned int tsif_type, struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	switch(tsif_type) {
		case TSIF_MODE_GPSB:
#if 1
			g_use_tsif_export_ioctl = 1;
			ret = tcc_gpsb_tsif_ioctl(filp, cmd, arg);
			g_use_tsif_export_ioctl = 0;
#else
			return tcc_gpsb_tsif_ioctl(filp, cmd, arg);
#endif
		break;
#ifdef SUPPORT_BLOCK_TSIF
		case TSIF_MODE_BLOCK:
			return tcc_block_tsif_ioctl(filp, cmd, arg);
		break;
#endif
		case TSIF_MODE_HWDMX:
			return tcc_hwdmx_tsif_ioctl_ex(filp, cmd, arg);
		break;
		default:
		break;
	}
	return -EFAULT;
}

/*
extern unsigned int tcc_hwdmx_tsif_poll(struct file *filp, struct poll_table_struct *wait);

int tcc_tsif_poll_ex(unsigned int tsif_type, struct file *filp, struct poll_table_struct *wait)
{
	switch(tsif_type) {
		case TSIF_MODE_GPSB:
			return tcc_gpsb_tsif_poll(filp, wait);
		break;
#ifdef SUPPORT_BLOCK_TSIF
		case TSIF_MODE_BLOCK:
			return tcc_block_tsif_poll(filp, wait);
		break;
#endif
		case TSIF_MODE_HWDMX:
			return tcc_hwdmx_tsif_poll(filp, wait);
		break;
		default:
		break;
	}
	return -EFAULT;
}

extern ssize_t tcc_hwdmx_tsif_write(struct file *filp, const char *buf, size_t len, loff_t *ppos);

ssize_t tcc_tsif_write_ex(unsigned int tsif_type, struct file *filp, const char *buf, size_t len, loff_t *ppos)
{
	switch(tsif_type) {
		case TSIF_MODE_GPSB:
			return tcc_gpsb_tsif_write(filp, buf, len, ppos);
		break;
#ifdef SUPPORT_BLOCK_TSIF
		case TSIF_MODE_BLOCK:
			return tcc_block_tsif_write(filp, buf, len, ppos);
		break;
#endif
		case TSIF_MODE_HWDMX:
			return tcc_hwdmx_tsif_write(filp, buf, len, ppos);
		break;
		default:
		break;
	}
	return -EFAULT;
}

extern ssize_t tcc_hwdmx_tsif_read(struct file *filp, char *buf, size_t len, loff_t *ppos);

ssize_t tcc_tsif_read_ex(unsigned int tsif_type, struct file *filp, char *buf, size_t len, loff_t *ppos)
{
	switch(tsif_type) {
		case TSIF_MODE_GPSB:
			return tcc_gpsb_tsif_read(filp, buf, len, ppos);
		break;
#ifdef SUPPORT_BLOCK_TSIF
		case TSIF_MODE_BLOCK:
			return tcc_block_tsif_read(filp, buf, len, ppos);
		break;
#endif
		case TSIF_MODE_HWDMX:
			return tcc_hwdmx_tsif_read(filp, buf, len, ppos);
		break;
		default:
		break;
	}
	return -EFAULT;
}
*/

extern int tcc_hwdmx_tsif_buffer_flush(struct file *filp, unsigned long x, int size);

int tcc_tsif_buffer_flush_ex(unsigned int tsif_type, struct file *filp, unsigned long p, int size)
{
	switch(tsif_type) {
		case TSIF_MODE_GPSB:
			return tcc_gpsb_tsif_buffer_flush(filp, p, size);
		break;
#ifdef SUPPORT_BLOCK_TSIF
		case TSIF_MODE_BLOCK:
			return tcc_block_tsif_buffer_flush(filp, p, size);
		break;
#endif
		case TSIF_MODE_HWDMX:
			return tcc_hwdmx_tsif_buffer_flush(filp, p, size);
		break;
		default:
		break;
	}
	return -EFAULT;
}

extern int tcc_hwdmx_tsif_set_external_tsdemux(struct file *filp, int (*decoder)(char *p1, int p1_size, char *p2, int p2_size, int devid), int max_dec_packet, int devid);

int tcc_tsif_set_external_tsdemux_ex(unsigned int tsif_type, struct file *filp, int (*decoder)(char *p1, int p1_size, char *p2, int p2_size, int devid), int max_dec_packet, int devid)
{
	switch(tsif_type) {
		case TSIF_MODE_GPSB:
			return tcc_gpsb_tsif_set_external_tsdemux(filp, decoder, max_dec_packet, devid);
		break;
#ifdef SUPPORT_BLOCK_TSIF
		case TSIF_MODE_BLOCK:
			return tcc_block_tsif_set_external_tsdemux(filp, decoder, max_dec_packet, devid);
		break;
#endif
		case TSIF_MODE_HWDMX:
			return tcc_hwdmx_tsif_set_external_tsdemux(filp, decoder, max_dec_packet, devid);
		break;
		default:
		break;
	}
	return -EFAULT;
}

EXPORT_SYMBOL(tcc_tsif_open_ex);
EXPORT_SYMBOL(tcc_tsif_release_ex);
EXPORT_SYMBOL(tcc_tsif_ioctl_ex);
/*
EXPORT_SYMBOL(tcc_tsif_poll_ex);
EXPORT_SYMBOL(tcc_tsif_write_ex);
EXPORT_SYMBOL(tcc_tsif_read_ex);
*/
EXPORT_SYMBOL(tcc_tsif_buffer_flush_ex);
EXPORT_SYMBOL(tcc_tsif_set_external_tsdemux_ex);

module_init(tsif_init);
module_exit(tsif_exit);

MODULE_AUTHOR("Telechips Inc.");
MODULE_DESCRIPTION("Telechips TSIF(GPSB Slave, HWDEMUX TSIF, Block TSIF) driver");
MODULE_LICENSE("GPL");
/***************************************tcc_tsif_devices***************************************/

