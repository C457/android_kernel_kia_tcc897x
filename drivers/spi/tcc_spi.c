/*
 * linux/drivers/spi/tcc_spi.c
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

#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/sched.h>

#include <linux/spi/spi.h>
#include <linux/spi/spi_bitbang.h>

#include <linux/of.h>
#include <linux/of_gpio.h>

#include <asm/io.h>
#include <asm/dma.h>
#include <linux/module.h>

//#include <mach/bsp.h>
//#include <mach/iomap.h>
#include "tca_spi.h"

#define tcc_spi_writel		__raw_writel
#define tcc_spi_readl		__raw_readl

extern void __iomem *devm_ioremap(struct device *dev, resource_size_t offset,
			   unsigned long size);


#define SPI_DMA_SIZE 2048
struct tca_spi_ex_handle {
    struct spi_device *spi;
    int rx_len, rx_cnt;
    int tx_len, tx_cnt;
    struct {
        int xfer_pos;
        int value;
    } done;

    unsigned char stopping;
    wait_queue_head_t wait_q;
    struct list_head queue;
    struct spi_message *current_message;
    spinlock_t lock;
   	struct workqueue_struct *spi_workqueue;
	struct work_struct spi_transfer_work;
};

struct tca_spi_global_setting{
    int open;
    int irq_no;
    void __iomem * reg_base;
	u32 phy_reg_base; // physical gpsb register base
	void __iomem * port_reg;
	void __iomem * pid_reg;
    struct clk *pclk;
    struct clk *hclk;
    unsigned gpsb_port;
    const char *gpsb_clk_name;
    struct spi_device *current_spi;

	// GDMA Configuration
	struct tca_spi_gdma_config gdma;
	unsigned int gdma_use;
	struct device *dev;
};
static struct tca_spi_global_setting tcc_spi_info[2];

static int tcc_spi_open(struct spi_device *spi);
static void tcc_spi_close(struct spi_device *spi);

#ifndef TCC_DMA_ENGINE
static void tcc_spi_gdma_config(tca_spi_handle_t *H, int length, int is_tx);
#endif


#ifndef TCC_DMA_ENGINE
void tcc_spi_gdma_regs_dump(tca_spi_handle_t *H, int is_tx)
{
	if(tca_spi_is_use_gdma(H))
	{
		void __iomem *regbase;

		//printk("gpsb_channel = %d, gdma base : 0x%08X\n", H->gpsb_channel,H->gdma_config.reg_base);

		regbase = is_tx ? H->gdma_config.tx_reg : H->gdma_config.rx_reg;
		printk("### %s GDMA @0x%p\n",is_tx ? "TX" : "RX", (void *)regbase);
        printk("[GDMA] ST_SADR = 0x%08x\n", tcc_spi_readl(regbase+TCC_GPSB_GDMA_ST_SADR));
        printk("[GDMA] SPARAM = 0x%08x\n", tcc_spi_readl(regbase+TCC_GPSB_GDMA_SPARAM));
        printk("[GDMA] C_SADR = 0x%08x\n", tcc_spi_readl(regbase+TCC_GPSB_GDMA_C_SADR));
        printk("[GDMA] ST_DADR = 0x%08x\n", tcc_spi_readl(regbase+TCC_GPSB_GDMA_ST_DADR));
        printk("[GDMA] DPARAM = 0x%08x\n", tcc_spi_readl(regbase+TCC_GPSB_GDMA_DPARAM));
        printk("[GDMA] C_DADR = 0x%08x\n", tcc_spi_readl(regbase+TCC_GPSB_GDMA_C_DADR));
        printk("[GDMA] HCOUNT = 0x%08x\n", tcc_spi_readl(regbase+TCC_GPSB_GDMA_HCOUNT));
        printk("[GDMA] CHCTRL = 0x%08x\n", tcc_spi_readl(regbase+TCC_GPSB_GDMA_CHCTRL));
        printk("[GDMA] RPTCTRL = 0x%08x\n", tcc_spi_readl(regbase+TCC_GPSB_GDMA_RPTCTRL));
        printk("[GDMA] EXTREQ = 0x%08x\n", tcc_spi_readl(regbase+TCC_GPSB_GDMA_EXTREQ));
	}
}
#endif


void tcc_spi_regs_dump(tca_spi_handle_t *H)
{
	void __iomem *regbase;

	regbase = H->regs;

	printk("gpsb_channel = %d\n", H->gpsb_channel);
    printk("[SPI%d] PORT = 0x%08x\n", H->gpsb_channel, tcc_spi_readl(regbase + TCC_GPSB_PORT));
    printk("[SPI%d] STAT = 0x%08x\n", H->gpsb_channel, tcc_spi_readl(regbase + TCC_GPSB_STAT));
    printk("[SPI%d] INTEN = 0x%08x\n", H->gpsb_channel, tcc_spi_readl(regbase + TCC_GPSB_INTEN));
    printk("[SPI%d] MODE = 0x%08x\n", H->gpsb_channel, tcc_spi_readl(regbase + TCC_GPSB_MODE));
    printk("[SPI%d] CTRL = 0x%08x\n", H->gpsb_channel, tcc_spi_readl(regbase + TCC_GPSB_CTRL));
    printk("[SPI%d] EVTCTRL = 0x%08x\n", H->gpsb_channel, tcc_spi_readl(regbase + TCC_GPSB_EVTCTRL));
    printk("[SPI%d] CCV = 0x%08x\n", H->gpsb_channel, tcc_spi_readl(regbase + TCC_GPSB_CCV));

	if(!tca_spi_is_use_gdma(H))
	{
		printk("[SPI%d] TXBASE = 0x%08x\n", H->gpsb_channel, tcc_spi_readl(regbase + TCC_GPSB_TXBASE));
		printk("[SPI%d] RXBASE = 0x%08x\n", H->gpsb_channel, tcc_spi_readl(regbase + TCC_GPSB_RXBASE));
		printk("[SPI%d] PACKET = 0x%08x\n", H->gpsb_channel, tcc_spi_readl(regbase + TCC_GPSB_PACKET));
		printk("[SPI%d] DMACTR = 0x%08x\n", H->gpsb_channel, tcc_spi_readl(regbase + TCC_GPSB_DMACTR));
		printk("[SPI%d] DMASTR = 0x%08x\n", H->gpsb_channel, tcc_spi_readl(regbase + TCC_GPSB_DMASTR));
		printk("[SPI%d] DMAICR = 0x%08x\n", H->gpsb_channel, tcc_spi_readl(regbase + TCC_GPSB_DMAICR));
	}
}


static ssize_t show_port(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "gpsb port : [%d][%d]\n", tcc_spi_info[0].gpsb_port, tcc_spi_info[1].gpsb_port);
}

static ssize_t store_port(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct platform_device *pdev = to_platform_device(dev);
	u32 port;

	port = simple_strtoul(buf, (char **)NULL, 16);
	/* valid port: 0xB, 0xC, 0xD, 0xE */
	if (port > 20 ) {
		printk("%s%d: invalid port! (use 0xb/c/d/e)\n", pdev->name, pdev->id);
		return -EINVAL;
	}

	tcc_spi_info[0].gpsb_port = port;
	return count;
}
static DEVICE_ATTR(tcc_port, S_IRUSR|S_IWUSR, show_port, store_port);

static void tea_free_dma_linux(struct tea_dma_buf *tdma, struct device *dev)
{
#ifdef CONFIG_ARM64
	if(dev == NULL)
	{
		printk("\x1b[1;33m[%s:%d] DEV is NULL!!!\x1b[0m\n", __func__, __LINE__);
		return;
	}
#endif
    if (tdma) {
        if (tdma->v_addr != 0) {
            dma_free_writecombine(dev, tdma->buf_size, tdma->v_addr, tdma->dma_addr);
        }
        memset(tdma, 0, sizeof(struct tea_dma_buf));
    }
}

static int tea_alloc_dma_linux(struct tea_dma_buf *tdma, unsigned int size, struct device *dev)
{
    int ret = -1;
#ifdef CONFIG_ARM64
	if(dev == NULL)
	{
		printk("\x1b[1;33m[%s:%d] DEV is NULL!!!\x1b[0m\n", __func__, __LINE__);
		return ret;
	}
#endif
    if (tdma) {
        tea_free_dma_linux(tdma,dev);
        tdma->buf_size = size;

        tdma->v_addr = dma_alloc_writecombine(dev, tdma->buf_size, &tdma->dma_addr, GFP_KERNEL);
        //printk("tcc_spi: alloc DMA buffer @0x%p(Phy=0x%Xp), size:%d\n",
        //       tdma->v_addr,
        //       tdma->dma_addr,
        //       tdma->buf_size);

        ret = tdma->v_addr ? 0 : 1;
    }
    return ret;
}

/* DMA Engine Specific */
#ifdef TCC_DMA_ENGINE
#include <linux/dmaengine.h>

static bool tcc_spi_dma_filter(struct dma_chan *chan, void *pdata)
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

static void tcc_spi_stop_dma(struct tca_spi_handle *tspi)
{
	if(tspi->dma.chan_tx)
		dmaengine_terminate_all(tspi->dma.chan_tx);
	if(tspi->dma.chan_rx)
		dmaengine_terminate_all(tspi->dma.chan_rx);
}

static void tcc_spi_release_dma(struct tca_spi_handle *tspi)
{
	if(tspi->dma.chan_tx)
	{
		dma_release_channel(tspi->dma.chan_tx);
		tspi->dma.chan_tx = NULL;
	}
	if(tspi->dma.chan_rx)
	{
		dma_release_channel(tspi->dma.chan_rx);
		tspi->dma.chan_rx = NULL;
	}
}

static void tcc_dma_tx_callback(void *data)
{
}

static void tcc_dma_rx_callback(void *data)
{
	struct spi_master *master = data;
    struct tca_spi_handle *tspi = spi_master_get_devdata(master);
    struct tca_spi_ex_handle *tspi_ex = (struct tca_spi_ex_handle *)tspi->private_data;

	tspi->dma_stop(tspi);
	tspi_ex->tx_cnt = tspi_ex->tx_len;
	tspi_ex->rx_cnt = tspi_ex->rx_len;
    wake_up(&(tspi_ex->wait_q));
}

static void tcc_spi_dma_slave_config_addr_width(struct dma_slave_config *slave_config, u8 bytes_per_word, int src)
{
	// Set WSIZE
	if(src)
		slave_config->dst_addr_width = DMA_SLAVE_BUSWIDTH_UNDEFINED;
	else
		slave_config->src_addr_width = DMA_SLAVE_BUSWIDTH_UNDEFINED;

	if(bytes_per_word == 4)
	{
		if(src)
			slave_config->src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
		else
			slave_config->dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	}
	else
	{
		if(src)
			slave_config->src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
		else
			slave_config->dst_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	}
}

static int tcc_spi_dma_slave_config(struct tca_spi_handle *tspi, struct dma_slave_config *slave_config, u8 bytes_per_word)
{
	int ret = 0;

	// Set BSIZE
	slave_config->dst_maxburst = MST_GDMA_BSIZE;
	slave_config->src_maxburst = MST_GDMA_BSIZE;

	// Set Address
	slave_config->dst_addr = (dma_addr_t)tspi->phy_reg_base + TCC_GPSB_PORT;
	slave_config->src_addr = (dma_addr_t)tspi->phy_reg_base + TCC_GPSB_PORT;

	// Set Tx Channel
	slave_config->direction = DMA_MEM_TO_DEV;
	slave_config->slave_id	= tspi->gdma_config.tx_ext;	// DMA Ext. Request.
	tcc_spi_dma_slave_config_addr_width(slave_config, bytes_per_word, 1);
	if(dmaengine_slave_config(tspi->dma.chan_tx, slave_config))
	{
		printk("\x1b[1;33m[%s:%d] Failed to configrue tx dma channel. \x1b[0m\n", __func__, __LINE__);
		ret = -EINVAL;
	}

	// Set Rx Channel
	slave_config->direction = DMA_DEV_TO_MEM;
	slave_config->slave_id	= tspi->gdma_config.rx_ext;	// DMA Ext. Request.
	tcc_spi_dma_slave_config_addr_width(slave_config,bytes_per_word, 0);
	if(dmaengine_slave_config(tspi->dma.chan_rx, slave_config))
	{
		printk("\x1b[1;33m[%s:%d] Failed to configrue rx dma channel. \x1b[0m\n", __func__, __LINE__);
		ret = -EINVAL;
	}

	return ret;
}

#define TCC_MST_DMA_SG_LEN	1
static int tcc_spi_dma_submit(struct spi_master *master, u32 flen)
{
    struct tca_spi_handle *tspi = spi_master_get_devdata(master);
	struct dma_chan *txchan = tspi->dma.chan_tx;
	struct dma_chan *rxchan = tspi->dma.chan_rx;
	struct dma_async_tx_descriptor *txdesc;
	struct dma_async_tx_descriptor *rxdesc;
	struct dma_slave_config slave_config;
	dma_cookie_t cookie;
	u32 len;
	u8 bytes_per_word;

	if(!rxchan || !txchan)
	{
		printk("\x1b[1;33m[%s:%d] rxchan(%p) or txchan(%p) are NULL\x1b[0m\n", __func__, __LINE__,rxchan,txchan);	
		return -ENODEV;
	}

	bytes_per_word = MST_GDMA_WSIZE;
	if(flen & 0x3){
	//	printk("length: %d\n", length);
		bytes_per_word = 1;
	}
	len = flen / (bytes_per_word == 4 ? bytes_per_word : 1);

	// Prepare the TX dma transfer
	sg_init_table(&tspi->dma.sgtx,TCC_MST_DMA_SG_LEN);
	sg_dma_len(&tspi->dma.sgtx) = len;
	if(tspi->flag)
	{
		sg_dma_address(&tspi->dma.sgtx) = tspi->tx_dma.dma_addr;
		tspi->flag = 0;
	}
	else
	{
		sg_dma_address(&tspi->dma.sgtx) = tspi->tx_dma_1.dma_addr;
		tspi->flag = 1;
	};

	// Prepare the RX dma transfer
	sg_init_table(&tspi->dma.sgrx,TCC_MST_DMA_SG_LEN);
	sg_dma_len(&tspi->dma.sgrx) = len;
	sg_dma_address(&tspi->dma.sgrx) = tspi->rx_dma.dma_addr;

	// Config dma slave
	if(tcc_spi_dma_slave_config(tspi, &slave_config, bytes_per_word))
	{
		printk("\x1b[1;33m[%s:%d] Slave config failed.\x1b[0m\n", __func__, __LINE__);
		return -ENOMEM;
	}

	// Send scatterlists
	txdesc = dmaengine_prep_slave_sg(txchan, &tspi->dma.sgtx, TCC_MST_DMA_SG_LEN,
		DMA_MEM_TO_DEV,
		DMA_PREP_INTERRUPT | DMA_CTRL_ACK);
	if(!txdesc)
	{
		printk("\x1b[1;33m[%s:%d] Preparing RX DMA Desc.\x1b[0m\n", __func__, __LINE__);
		goto err_dma;
	}

	txdesc->callback = tcc_dma_tx_callback;
	txdesc->callback_param = master;

	rxdesc = dmaengine_prep_slave_sg(rxchan, &tspi->dma.sgrx, TCC_MST_DMA_SG_LEN,
		DMA_DEV_TO_MEM,
		DMA_PREP_INTERRUPT | DMA_CTRL_ACK);
	if(!rxdesc)
	{
		printk("\x1b[1;33m[%s:%d] Preparing RX DMA Desc.\x1b[0m\n", __func__, __LINE__);
		goto err_dma;
	}

	rxdesc->callback = tcc_dma_rx_callback;
	rxdesc->callback_param = master;

	// Enable GPSB Interrupt
	if(bytes_per_word == 4){
		tcc_spi_writel(tcc_spi_readl(tspi->regs + TCC_GPSB_INTEN) | (Hw27 | Hw26 | Hw25 | Hw24),tspi->regs + TCC_GPSB_INTEN);
	}else{
		tcc_spi_writel(tcc_spi_readl(tspi->regs + TCC_GPSB_INTEN) & ~(Hw27 | Hw26 | Hw25 | Hw24),tspi->regs + TCC_GPSB_INTEN);
	}

	// Submit desctriptors
	cookie = dmaengine_submit(txdesc);
	if(dma_submit_error(cookie))
	{
		printk("\x1b[1;33m[%s:%d] TX Desc. submitting error! (cookie: %X) \x1b[0m\n", __func__, __LINE__,(unsigned int)cookie);
		goto err_dma;
	}

	cookie = dmaengine_submit(rxdesc);
	if(dma_submit_error(cookie))
	{
		printk("\x1b[1;33m[%s:%d] RX Desc. submitting error! (cookie: %X) \x1b[0m\n", __func__, __LINE__,(unsigned int)cookie);
		goto err_dma;
	}

	// Issue pendings
	dma_async_issue_pending(txchan);
	dma_async_issue_pending(rxchan);

	return 0;
err_dma:
	tcc_spi_stop_dma(tspi);
	return -ENOMEM;
}

static int tcc_spi_dma_configuration(struct platform_device *pdev, struct tca_spi_handle *tspi)
{
	struct dma_slave_config slave_config;
	struct device *dev = &pdev->dev;
	int ret;

	dma_cap_mask_t mask;
	dma_cap_zero(mask);
	dma_cap_set(DMA_SLAVE, mask);

	tspi->dma.chan_tx = dma_request_slave_channel_compat(mask,tcc_spi_dma_filter,&tspi->dma,dev,"tx");
	if(!tspi->dma.chan_tx)
	{
		printk("\x1b[1;33m[%s:%d] DMA TX channel request Error!\x1b[0m\n", __func__, __LINE__);
		ret = -EBUSY;
		goto error;
	}

	tspi->dma.chan_rx = dma_request_slave_channel_compat(mask,tcc_spi_dma_filter,&tspi->dma,dev,"rx");
	if(!tspi->dma.chan_rx)
	{
		printk("\x1b[1;33m[%s:%d] DMA RX channel request Error!\x1b[0m\n", __func__, __LINE__);
		ret = -EBUSY;
		goto error;
	}

	ret = tcc_spi_dma_slave_config(tspi,&slave_config,MST_GDMA_WSIZE);
	if(ret)
		goto error;

	printk("\x1b[1;33mUsing %s (TX) and %s (RX) for DMA trnasfers\x1b[0m\n",
		dma_chan_name(tspi->dma.chan_tx),dma_chan_name(tspi->dma.chan_rx));

	return 0;

error:
	tcc_spi_release_dma(tspi);
	return ret;
}
#endif /* TCC_DMA_ENGINE */

static inline void tcc_spi_set_clk(u32 en, u32 clk, int id)
{
	if(en == ENABLE)
        clk_set_rate(tcc_spi_info[id].pclk, clk*2);
}

static int tcc_spi_start_rxtx(struct spi_master *master, unsigned int flen)
{
    struct tca_spi_handle *tspi = spi_master_get_devdata(master);
    struct tca_spi_ex_handle *tspi_ex = (struct tca_spi_ex_handle *)tspi->private_data;

    tspi_ex->done.value = -EIO;
    //mutex_lock(&(tspi_ex->pm_mutex));

    if ((flen > 0) && (flen <= tspi->rx_dma.buf_size)) {
        tspi_ex->rx_len = flen;
        tspi_ex->tx_len = flen;
        tspi_ex->rx_cnt = 0;
        tspi_ex->tx_cnt = 0;

		// Before start gpsb, should clear Tx/Rx FIFO counter
        tspi->clear_fifo_packet(tspi);
        tspi->set_packet_cnt(tspi, flen);

		if(tca_spi_is_use_gdma(tspi))
		{
#ifndef TCC_DMA_ENGINE
			// GPSB TX GDMA Config
			tcc_spi_gdma_config(tspi,flen,1);
			// GPSB RX GDMA Config
			tcc_spi_gdma_config(tspi,flen,0);
#else
			tcc_spi_dma_submit(master, flen);
#endif
		}

	    tspi->dma_start(tspi);

        if (wait_event_interruptible_timeout((tspi_ex->wait_q),
                                             (tspi_ex->rx_cnt == tspi_ex->rx_len),
                                             msecs_to_jiffies(WAIT_TIME_FOR_DMA_DONE)) == 0) {
#ifdef TCC_DMA_ENGINE
			if(tca_spi_is_use_gdma(tspi))
			{
				tcc_spi_stop_dma(tspi);
			}
#endif
            tcc_spi_regs_dump(tspi);
            tspi->dma_stop(tspi);

            printk("[%s] wait_event timeout  (%dms) !!!\n",
                   __func__, WAIT_TIME_FOR_DMA_DONE);
        } else {
            if (tspi_ex->rx_cnt == tspi_ex->rx_len) {
                tspi_ex->done.value = 0;
            }
        }
    }

    //tspi_ex->current_message = NULL;
    //mutex_unlock(&(tspi_ex->pm_mutex));

    return tspi_ex->done.value;
}

static void tcc_spi_next_message(struct spi_master *master)
{
    struct tca_spi_handle *tspi = NULL;
    struct spi_message *msg = NULL;
    struct spi_device *spi = NULL;
    struct spi_transfer *xfer = NULL;
    unsigned int flen = 0;
    unsigned int copy_len = 0, total_len = 0;
    struct tca_spi_ex_handle *tspi_ex = NULL;
    unsigned int bit_width = 0;

    unsigned int bits= 0;
	unsigned int clk_hz;
    unsigned long flags = 0;

    tspi = spi_master_get_devdata(master);
    tspi_ex = (struct tca_spi_ex_handle *)tspi->private_data;

    //BUG_ON(tspi_ex->current_message);
    spin_lock_irqsave(&(tspi_ex->lock), flags);
    msg = list_entry(tspi_ex->queue.next, struct spi_message, queue);
    list_del_init(&(msg->queue));
    //tspi_ex->current_message = msg;
    spin_unlock_irqrestore(&(tspi_ex->lock), flags);

    spi = msg->spi;

    list_for_each_entry(xfer, &(msg->transfers), transfer_list) {
        tspi_ex->done.xfer_pos = 0;
        tspi_ex->done.value = 0;
        total_len = xfer->len;
		tspi->ctf = !xfer->cs_change;	/* verify continuous transfer mode */
	
		if (xfer->speed_hz != spi->max_speed_hz) {
			clk_hz = xfer->speed_hz ? xfer->speed_hz : spi->max_speed_hz;
			tcc_spi_set_clk(ENABLE, clk_hz, master->bus_num);
		}
		
		if (xfer->bits_per_word != spi->bits_per_word) {
			bits = xfer->bits_per_word ? xfer->bits_per_word : spi->bits_per_word;
			if (bits != 8 && bits != 16 && bits != 32) {
				dev_dbg(&spi->dev, "setup: invalid bits_per_word %u\n", bits);
				return;
			}
			tspi->set_bit_width(tspi, bits);
		}

		#if defined(INCLUDE_TDMB)
			tspi->set_bit_width(tspi,32);
			static bool logprint = true;
			if(logprint)
			{
				printk("%s : set bit width : 32\n",__func__);
				logprint = false;
			}

		#endif

        do {
			copy_len = (total_len > tspi->rx_dma.buf_size) ? tspi->rx_dma.buf_size : total_len;
			tspi->tx_pkt_remain = (total_len > tspi->rx_dma.buf_size) ? 1 : 0 ;
			if (copy_len & 0x3) {
			   copy_len %= 4;
			   bit_width = tcc_spi_readl(tspi->regs + TCC_GPSB_MODE);
			   bit_width = ((bit_width >> 8) & 0x1F) + 1;
			   tspi->set_bit_width(tspi, 8);
			}

			if(!tca_spi_is_use_gdma(tspi))	// if use dedicated DMA
			{
				if (xfer->tx_buf) {
					if (tspi->flag) {
						tcc_spi_writel(tspi->tx_dma.dma_addr, tspi->regs + TCC_GPSB_TXBASE);
						memcpy(tspi->tx_dma.v_addr, xfer->tx_buf + tspi_ex->done.xfer_pos, copy_len);
						tspi->flag = 0;
					} else {
						tcc_spi_writel(tspi->tx_dma_1.dma_addr, tspi->regs + TCC_GPSB_TXBASE);
						memcpy(tspi->tx_dma_1.v_addr, xfer->tx_buf + tspi_ex->done.xfer_pos, copy_len);
						tspi->flag = 1;
					}
				}
			}
			else	// if use GDMA
			{
				if(xfer->tx_buf){
#ifndef TCC_DMA_ENGINE
					void __iomem *gdma_tx_regs = tspi->gdma_config.tx_reg;
					if (tspi->flag) {
						tcc_spi_writel(tspi->tx_dma.dma_addr,gdma_tx_regs+TCC_GPSB_GDMA_ST_SADR);
						memcpy(tspi->tx_dma.v_addr, xfer->tx_buf + tspi_ex->done.xfer_pos, copy_len);
						tspi->flag = 0;
					} else {
						tcc_spi_writel(tspi->tx_dma_1.dma_addr,gdma_tx_regs+TCC_GPSB_GDMA_ST_SADR);
						memcpy(tspi->tx_dma_1.v_addr, xfer->tx_buf + tspi_ex->done.xfer_pos, copy_len);
						tspi->flag = 1;
					}
#else
					if (tspi->flag) {
						memcpy(tspi->tx_dma.v_addr, xfer->tx_buf + tspi_ex->done.xfer_pos, copy_len);
					} else {
						memcpy(tspi->tx_dma_1.v_addr, xfer->tx_buf + tspi_ex->done.xfer_pos, copy_len);
					}
#endif
				}
			}

			if (tcc_spi_start_rxtx(master, copy_len)) {
			    tspi->clear_fifo_packet(tspi);
			    goto lb_return;
			}

			if (xfer->rx_buf) {
			    memcpy(xfer->rx_buf + tspi_ex->done.xfer_pos, tspi->rx_dma.v_addr, copy_len);
			}

			if (bit_width) {
			    tspi->set_bit_width(tspi, bit_width);
			    bit_width = 0;
			}

			total_len -= copy_len;
			tspi_ex->done.xfer_pos += copy_len;
        } while (total_len);
        flen += xfer->len;
    }

lb_return:
    msg->status = tspi_ex->done.value;
    if (msg->status == 0) {
        msg->actual_length = flen;
    }
    msg->complete(msg->context);

    
}

static irqreturn_t tcc_spi_dma_isr(int irq, void *_spi_mster)
{
    struct spi_master *master = _spi_mster;
    struct tca_spi_handle *tspi = spi_master_get_devdata(master);
    //unsigned long flags = 0;
    unsigned long dma_done_reg = 0;
    struct tca_spi_ex_handle *tspi_ex = (struct tca_spi_ex_handle *)tspi->private_data;
    void __iomem *gpsb_pcfg_reg = (void __iomem *)tspi->port_regs;//(void __iomem *)io_p2v(TCC_PA_GPSB_PORTCFG);
    int irq_idx = tspi->gpsb_channel;

	if(!tca_spi_is_use_gdma(tspi))	// if use dedicated DMA
	{
		// Check GPSB GPSB DMA IRQ
	    if ((tcc_spi_readl(gpsb_pcfg_reg+0xC) & (1<<((irq_idx*2)+1))) == 0)
	    {
	        return IRQ_NONE;
	    }

		// For GPSB Channle using dedicated DMA
		dma_done_reg = tcc_spi_readl(tspi->regs + TCC_GPSB_DMAICR);

		if (dma_done_reg & Hw29) {
		    tspi->dma_stop(tspi);
		    tspi_ex->tx_cnt = tspi_ex->tx_len;
		    tspi_ex->rx_cnt = tspi_ex->rx_len;
		    wake_up(&(tspi_ex->wait_q));
		}
	}
	else
	{
		//printk("\x1b[1;33m[%s:%d] GDMA regbase : %08X\x1b[0m\n", __func__, __LINE__,tspi->gdma_config.reg_base);
		// For GPSB Channle using GDMA
		void __iomem *gdma_tx_regs, *gdma_rx_regs, *gdma_regbase;
		int gdma_intr_ch;

		gdma_regbase = tspi->gdma_config.reg_base;
		gdma_tx_regs = tspi->gdma_config.tx_reg;
		gdma_rx_regs = tspi->gdma_config.rx_reg;

		gdma_intr_ch = tca_spi_gdma_intr_ch(tspi);

		if(gdma_intr_ch == tspi->gdma_config.tx_ch)
		{
			dma_done_reg = tcc_spi_readl(gdma_tx_regs + TCC_GPSB_GDMA_CHCTRL);
			if(dma_done_reg & (Hw3))
			{
				tcc_spi_writel(tcc_spi_readl(gdma_tx_regs + TCC_GPSB_GDMA_CHCTRL) | Hw3, gdma_tx_regs + TCC_GPSB_GDMA_CHCTRL);
			}
		}
		else if(gdma_intr_ch == tspi->gdma_config.rx_ch)
		{
			dma_done_reg = tcc_spi_readl(gdma_rx_regs + TCC_GPSB_GDMA_CHCTRL);
			if(dma_done_reg & (Hw3))
			{
				tcc_spi_writel(tcc_spi_readl(gdma_rx_regs + TCC_GPSB_GDMA_CHCTRL) | Hw3, gdma_rx_regs + TCC_GPSB_GDMA_CHCTRL);
				tspi->dma_stop(tspi);
				tspi_ex->tx_cnt = tspi_ex->tx_len;
				tspi_ex->rx_cnt = tspi_ex->rx_len;
				wake_up(&(tspi_ex->wait_q));
			}
		}
		else
		{
			return IRQ_NONE;
		}
	}

    //local_save_flags(flags);
    //local_irq_disable();

    //local_irq_restore(flags);
    return IRQ_HANDLED;
}

#define MODEBITS (SPI_CPOL | SPI_CPHA | SPI_CS_HIGH | SPI_LSB_FIRST)
static int tcc_spi_setup(struct spi_device *spi)
{
    struct tca_spi_handle *tspi = NULL;
    unsigned int bits = spi->bits_per_word;
    struct tca_spi_ex_handle *tspi_ex = NULL;

#if defined(CONFIG_DAUDIO_KK)
/**
 * @author sjpark@cleinsoft
 * @date 2013/12/03
 * no need controller state control - removed
 **/
#else
    if( spi->controller_state == NULL)
    {
        // to skip initial call when registering master.
        spi->controller_state = (void *)true;
        return 0;
    }
#endif

    if(tcc_spi_info[spi->master->bus_num].open == 0){
        if(tcc_spi_open(spi) != 0)
            return -EINVAL;
        tcc_spi_info[spi->master->bus_num].open = 1;
    }

    tspi = spi_master_get_devdata(spi->master);
    tspi_ex = (struct tca_spi_ex_handle *)tspi->private_data;

    if (tspi_ex->stopping)
        return -ESHUTDOWN;
    if (spi->chip_select > spi->master->num_chipselect) {
        dev_dbg(&spi->dev,
                "setup: invalid chipselect %u (%u defined)\n",
                spi->chip_select, spi->master->num_chipselect);
        return -EINVAL;
    }

    if (bits == 0) {
        spi->bits_per_word = bits = 32;
	}
    if (bits != 8 && bits != 16 && bits != 32) {
        dev_dbg(&spi->dev, "setup: invalid bits_per_word %u\n", bits);
        return -EINVAL;
    }
    tspi->set_bit_width(tspi, bits);

    if (spi->mode & ~MODEBITS) {
        dev_dbg(&spi->dev, "[%s]setup: unsupported mode bits %x\n",
                __func__, spi->mode & ~MODEBITS);
        return -EINVAL;
    }

    tca_spi_setCPOL(tspi->regs, (spi->mode & SPI_CPOL));
    tca_spi_setCPHA(tspi->regs, (spi->mode & SPI_CPHA));
    tca_spi_setCS_HIGH(tspi->regs, (spi->mode & SPI_CS_HIGH));
    tca_spi_setLSB_FIRST(tspi->regs, (spi->mode & SPI_LSB_FIRST));

	if (spi->max_speed_hz) {
		tcc_spi_set_clk(ENABLE, spi->max_speed_hz, spi->master->bus_num);
	} else {
		return -EINVAL;
	}

	printk("%s: spi->bus_num(%d),spi->mode(0x%x), spi->max_speed_hz(%d)\n", __func__,spi->master->bus_num,spi->mode, spi->max_speed_hz);

	//tcc_spi_regs_dump(tspi);
    return 0;
}

static int tcc_spi_transfer(struct spi_device *spi, struct spi_message *msg)
{
    struct tca_spi_handle *tspi = NULL;
    unsigned long flags = 0;
    struct tca_spi_ex_handle *tspi_ex = NULL;
	int was_empty;
	
    tspi = (struct tca_spi_handle *)spi_master_get_devdata(spi->master);
    tspi_ex = (struct tca_spi_ex_handle *)tspi->private_data;

    if (unlikely(list_empty(&(msg->transfers)))) {
        return -EINVAL;
    }

    if (tspi_ex->stopping) {
        return -ESHUTDOWN;
    }

    msg->status = -EINPROGRESS;
    msg->actual_length = 0;

    spin_lock_irqsave(&(tspi_ex->lock), flags);
	was_empty = list_empty(&tspi_ex->queue);
    list_add_tail(&(msg->queue), &(tspi_ex->queue));
    if (was_empty)
    {
		queue_work(tspi_ex->spi_workqueue, &tspi_ex->spi_transfer_work);
    }
    spin_unlock_irqrestore(&(tspi_ex->lock), flags);
    return 0;
}

static void tcc_spi_cleanup(struct spi_device *spi)
{
    if (!spi->controller_state)
        return;

    if(tcc_spi_info[spi->master->bus_num].open){
        tcc_spi_info[spi->master->bus_num].open = 0;
        tcc_spi_close(spi);
    }
    dev_dbg(&spi->dev, "tcc_spi_cleanup\n");
}

static void tcc_spi_close(struct spi_device *spi)
{
	struct tca_spi_handle *tspi = NULL;
	struct tca_spi_ex_handle *tspi_ex = NULL;
	struct spi_message *msg = NULL;
	struct pinctrl *pinctrl;

	tspi = spi_master_get_devdata(spi->master);
	tspi_ex = (struct tca_spi_ex_handle *)tspi->private_data;
	spin_lock_irq(&(tspi_ex->lock));
    tspi_ex->stopping = 1;
    spin_unlock_irq(&tspi_ex->lock);
    list_for_each_entry(msg, &(tspi_ex->queue), queue) {
        msg->status = -ESHUTDOWN;
        msg->complete(msg->context);
    }
#if !defined(CONFIG_TCC_CODESONAR_BLOCKED)
#else
	if (tspi_ex) { kfree(tspi_ex); }
#endif
	
	if(!tca_spi_is_use_gdma(tspi))	// if use dedicated DMA
		free_irq(tspi->irq, spi->master);
#ifndef TCC_DMA_ENGINE
	else
		free_irq(tspi->gdma_config.gdma_irq,spi->master);
#endif

	pinctrl = pinctrl_get_select(tcc_spi_info[spi->master->bus_num].dev, "idle");
	if(IS_ERR(pinctrl))
		printk("%s : pinctrl error[0x%p]\n", __func__, pinctrl);
	else
		pinctrl_put(pinctrl);

	tca_spi_clean(tspi);
	tcc_spi_set_clk(DISABLE, spi->max_speed_hz, spi->master->bus_num);
	if (tcc_spi_info[spi->master->bus_num].pclk && tcc_spi_info[spi->master->bus_num].hclk){
		clk_disable_unprepare(tcc_spi_info[spi->master->bus_num].pclk);
		clk_disable_unprepare(tcc_spi_info[spi->master->bus_num].hclk);
		tcc_spi_info[spi->master->bus_num].pclk = NULL;
		tcc_spi_info[spi->master->bus_num].hclk = NULL;
	}

	destroy_workqueue(tspi_ex->spi_workqueue);
#if !defined(CONFIG_TCC_CODESONAR_BLOCKED)
	if (tspi_ex) { kfree(tspi_ex); }
#endif
}

static void tcc_spi_transfer_work(struct work_struct *work)
{
   	struct tca_spi_ex_handle *tspi_ex = NULL;
	unsigned long flags;
   	tspi_ex =  container_of(work, struct tca_spi_ex_handle, spi_transfer_work);
    spin_lock_irqsave(&tspi_ex->lock, flags);
	while (!list_empty(&tspi_ex->queue) && !tspi_ex->stopping) 
	{
	    spin_unlock_irqrestore(&tspi_ex->lock, flags);
		tcc_spi_next_message(tspi_ex->spi->master);
        spin_lock_irqsave(&tspi_ex->lock, flags);
	}
	spin_unlock_irqrestore(&tspi_ex->lock, flags);

}

static int tcc_spi_open(struct spi_device *spi)
{
	int ret = 0;
	struct spi_master *master = NULL;
	struct tca_spi_handle *tspi = NULL;
	struct tca_spi_ex_handle *tspi_ex = NULL;
	struct tca_spi_port_config port_config;
	struct pinctrl *pinctrl;
	struct device *dev;

	master = spi->master;
	if(!master->dev.of_node)
		return -EINVAL;
	tcc_spi_info[master->bus_num].hclk = of_clk_get(master->dev.of_node, 1);
	tcc_spi_info[master->bus_num].pclk = of_clk_get(master->dev.of_node, 0);
    	if (IS_ERR(tcc_spi_info[master->bus_num].pclk) ||IS_ERR(tcc_spi_info[master->bus_num].hclk) ) {
		printk(KERN_WARNING "SPI: failed to get gpsb clock\n");
		goto err;
	}
	clk_prepare_enable(tcc_spi_info[master->bus_num].pclk);
	clk_prepare_enable(tcc_spi_info[master->bus_num].hclk);

	tcc_spi_info[master->bus_num].current_spi =  spi;

	clk_set_rate(tcc_spi_info[master->bus_num].pclk, spi->max_speed_hz);

	tspi = spi_master_get_devdata(spi->master);

	tspi_ex = (struct tca_spi_ex_handle *)kmalloc(sizeof(struct tca_spi_ex_handle), GFP_KERNEL);
	if (tspi_ex == NULL) {
		return -ENOMEM; 
	}
	memset(tspi_ex, 0, sizeof(struct tca_spi_ex_handle));

	/* GPSB port */
	port_config.gpsb_port = tcc_spi_info[master->bus_num].gpsb_port;

	pinctrl = pinctrl_get_select(tcc_spi_info[master->bus_num].dev, "active");
	if(IS_ERR(pinctrl))
		printk("%s : pinctrl error[0x%p]\n", __func__, pinctrl);

#ifdef CONFIG_ARM64
	dev = &spi->dev;
#else
	dev = NULL;
#endif

	/* GPSB GDMA Config */
	//tca_spi_print_gdma_config(tcc_spi_info[master->bus_num].gdma);

	if (tca_spi_init(tspi,
					tcc_spi_info[master->bus_num].reg_base,
					tcc_spi_info[master->bus_num].phy_reg_base,
					tcc_spi_info[master->bus_num].port_reg,
					tcc_spi_info[master->bus_num].pid_reg,
					tcc_spi_info[master->bus_num].irq_no,
					tea_alloc_dma_linux,
					tea_free_dma_linux,
					SPI_DMA_SIZE,
					master->bus_num,
					0,
					&port_config,
					&tcc_spi_info[master->bus_num].gdma,
					tcc_spi_info[master->bus_num].gpsb_clk_name,
					dev)) {
		goto err;
	}

	tspi->private_data = (void *)tspi_ex;
	tspi_ex->spi = spi;

	spin_lock_init(&(tspi_ex->lock));
	INIT_LIST_HEAD(&(tspi_ex->queue));

	tspi->clear_fifo_packet(tspi);
	tspi->dma_stop(tspi);
	init_waitqueue_head(&(tspi_ex->wait_q));

	tspi->hw_init(tspi);

	if(!tca_spi_is_use_gdma(tspi)) // if use dedicated DMA
		ret = request_irq(tspi->irq, tcc_spi_dma_isr, IRQF_SHARED, dev_name(&(master->dev)), master);	
#ifndef TCC_DMA_ENGINE
	else // if use GDMA
		ret = request_irq(tcc_spi_info[master->bus_num].gdma.gdma_irq, tcc_spi_dma_isr, IRQF_SHARED, dev_name(&(master->dev)), master);
#endif

	if (ret) { goto err; }

    tspi_ex->spi_workqueue = create_singlethread_workqueue("tcc spi");
	if (!tspi_ex->spi_workqueue) {
		ret = -ENODEV;
		goto err;
	}

	INIT_WORK(&tspi_ex->spi_transfer_work, tcc_spi_transfer_work);
	return 0;

err:
	printk("%s: error!!!\n", __func__);
	tca_spi_clean(tspi);
	if (tcc_spi_info[master->bus_num].pclk && tcc_spi_info[master->bus_num].hclk){
		clk_disable_unprepare(tcc_spi_info[master->bus_num].pclk);
		clk_disable_unprepare(tcc_spi_info[master->bus_num].hclk);
		tcc_spi_info[master->bus_num].pclk = NULL;
		tcc_spi_info[master->bus_num].hclk = NULL;
	}
	if (tspi_ex) { kfree(tspi_ex); }
	return ret;
}

#ifdef CONFIG_OF
static void tcc_spi_port_parse_dt(struct device_node *np, struct tca_spi_port_config *pgpios)
{
    pgpios->name = np->name;

    of_property_read_u32(np, "gpsb-id", &pgpios->gpsb_id);
    of_property_read_u32(np, "gpsb-port", &pgpios->gpsb_port);
	//printk("\x1b[1;33m[%s:%d] Name: %d ID: %d Port: %d\x1b[0m\n",
	//	__func__, __LINE__,pgpios->name, pgpios->gpsb_id,pgpios->gpsb_port);
}

static void tcc_spi_gdma_parse_dt(struct device_node *np, struct tca_spi_gdma_config *pgdma)
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
static void __iomem *tcc_spi_gdma_base_ioremap_addr(struct platform_device *pdev,
	struct resource *regs, struct tca_spi_gdma_config *pgdma)
{
	void __iomem *retval;
	resource_size_t base_addr;

	base_addr = regs->start + (TCC_GDMA_CTRL_REG_OFFSET * pgdma->tx_ctrl);
	retval = devm_ioremap(&pdev->dev, base_addr, TCC_GPSB_GDMA_CHCONFIG+0x4);
	//printk("\x1b[1;33m[%s:%d] GDMA Base Addr : 0x%X\x1b[0m\n", __func__, __LINE__,(unsigned int)base_addr);

	return retval;
}

static void __iomem *tcc_spi_gdma_ioremap_addr(struct platform_device *pdev,
	struct resource *regs, struct tca_spi_gdma_config *pgdma, int is_tx)
{
	void __iomem *retval;
	unsigned int ctrl, ch;
	resource_size_t base_addr;

	if(regs == NULL)
		return 0;

	base_addr = regs->start;
	ctrl = is_tx ? pgdma->tx_ctrl : pgdma->rx_ctrl;
	ch = is_tx ? pgdma->tx_ch : pgdma->rx_ch;

	base_addr = regs->start + (TCC_GDMA_CTRL_REG_OFFSET * ctrl) + (TCC_GDMA_CH_REG_OFFSET * ch);
	retval = devm_ioremap(&pdev->dev, base_addr, TCC_GDMA_CH_REG_OFFSET);

#if 0
	printk("\x1b[1;31m[%s:%d] ## SPI IOREMAP GDMA ADDR (%s) ##\x1b[0m\n", __func__, __LINE__,is_tx ? "Tx" : "Rx");
	printk("\x1b[1;33m[%s:%d] GDMA Base Addr : 0x%X\x1b[0m\n", __func__, __LINE__,base_addr);
	printk("\x1b[1;33m[%s:%d] GDMA Controller ID : 0x%02X\x1b[0m\n", __func__, __LINE__,ctrl);
	printk("\x1b[1;33m[%s:%d] GDMA Channel ID : 0x%02X\x1b[0m\n", __func__, __LINE__,ch);
	printk("\x1b[1;32m[%s:%d] GDMA Addr : Phy@0x%X\x1b[0m\n", __func__, __LINE__,retval);
	printk("\x1b[1;32m[%s:%d] GDMA Addr : Virt@0x%X\x1b[0m\n", __func__, __LINE__,retval);
#endif

	return retval;
}

static void tcc_spi_gdma_config(tca_spi_handle_t *H, int length, int is_tx)
{
	void __iomem *reg_base;
	unsigned int  ext_num;
	int _MST_GDMA_WSIZE = MST_GDMA_WSIZE;//gdma_wsize;

	if(!H->gdma_config.gpsb_use_gdma)
	{
		printk("\x1b[1;31m[%s:%d] Error: GPSB ID %d is not available using GDMA.\x1b[0m\n", __func__, __LINE__,H->id);
		return;
	}

	if(is_tx)
	{
		reg_base = H->gdma_config.tx_reg;
		ext_num = H->gdma_config.tx_ext;
	}
	else
	{
		reg_base = H->gdma_config.rx_reg;
		ext_num = H->gdma_config.rx_ext;
	}

	if(length & 0x3){
	//	printk("length: %d\n", length);
		_MST_GDMA_WSIZE = 1;
	}
	if(_MST_GDMA_WSIZE == 4){
		tcc_spi_writel(tcc_spi_readl(H->regs + TCC_GPSB_INTEN) | (Hw27 | Hw26 | Hw25 | Hw24),H->regs + TCC_GPSB_INTEN);
	}else{
		tcc_spi_writel(tcc_spi_readl(H->regs + TCC_GPSB_INTEN) & ~(Hw27 | Hw26 | Hw25 | Hw24),H->regs + TCC_GPSB_INTEN);
	}

	// Disable GDMA
	tcc_spi_writel(tcc_spi_readl(reg_base+TCC_GPSB_GDMA_CHCTRL) & ~Hw0, reg_base+TCC_GPSB_GDMA_CHCTRL);

	// Set Source/Destination Block Parameter Register
	tcc_spi_writel(0x0, reg_base+(is_tx ? TCC_GPSB_GDMA_DPARAM : TCC_GPSB_GDMA_SPARAM));
	tcc_spi_writel(0x0 | (_MST_GDMA_WSIZE == 4 ? _MST_GDMA_WSIZE : 1), reg_base+(is_tx ? TCC_GPSB_GDMA_SPARAM : TCC_GPSB_GDMA_DPARAM));

	// Set HOP Counter Register
	tcc_spi_writel(length / (_MST_GDMA_WSIZE == 4 ? _MST_GDMA_WSIZE : 1), reg_base+TCC_GPSB_GDMA_HCOUNT);

	//tcc_spi_writel(tcc_spi_readl(reg_base+TCC_GPSB_GDMA_CHCTRL) | (Hw1|Hw3|Hw8|Hw9|Hw11|Hw13),reg_base+TCC_GPSB_GDMA_CHCTRL);
	tcc_spi_writel(tcc_spi_readl(reg_base+TCC_GPSB_GDMA_CHCTRL) | (Hw3 | Hw11 | Hw13),reg_base+TCC_GPSB_GDMA_CHCTRL);

	// Set Burst Size
	if(MST_GDMA_BSIZE == 1)
		tcc_spi_writel(tcc_spi_readl(reg_base+TCC_GPSB_GDMA_CHCTRL) & ~(Hw7 | Hw6) ,reg_base+TCC_GPSB_GDMA_CHCTRL);
	else if(MST_GDMA_BSIZE == 2)
		tcc_spi_writel(tcc_spi_readl(reg_base+TCC_GPSB_GDMA_CHCTRL) | Hw6, reg_base+TCC_GPSB_GDMA_CHCTRL);
	else if(MST_GDMA_BSIZE == 4)
		tcc_spi_writel(tcc_spi_readl(reg_base+TCC_GPSB_GDMA_CHCTRL) | Hw7, reg_base+TCC_GPSB_GDMA_CHCTRL);
	else if(MST_GDMA_BSIZE == 8)
		tcc_spi_writel(tcc_spi_readl(reg_base+TCC_GPSB_GDMA_CHCTRL) | (Hw7 | Hw6),reg_base+TCC_GPSB_GDMA_CHCTRL);

	// Set Word Size
	if(_MST_GDMA_WSIZE == 1)
		tcc_spi_writel(tcc_spi_readl(reg_base+TCC_GPSB_GDMA_CHCTRL) & ~(Hw5 | Hw4), reg_base+TCC_GPSB_GDMA_CHCTRL);
	else if(_MST_GDMA_WSIZE == 2)
		tcc_spi_writel(tcc_spi_readl(reg_base+TCC_GPSB_GDMA_CHCTRL) | Hw4, reg_base+TCC_GPSB_GDMA_CHCTRL);
	else if(_MST_GDMA_WSIZE == 4)
		tcc_spi_writel(tcc_spi_readl(reg_base+TCC_GPSB_GDMA_CHCTRL) | Hw5, reg_base+TCC_GPSB_GDMA_CHCTRL);

	// Set Repeat Control Register
	tcc_spi_writel(tcc_spi_readl(reg_base+TCC_GPSB_GDMA_RPTCTRL) & ~Hw31, reg_base+TCC_GPSB_GDMA_RPTCTRL);

	// Set External Request Register
	//reg_base = H->gdma_config.reg_base; // GDMA Base Register
	tcc_spi_writel(tcc_spi_readl(reg_base+TCC_GPSB_GDMA_EXTREQ) & ~(Hw32-Hw0), reg_base+TCC_GPSB_GDMA_EXTREQ);	// Clear EXTREQ
	tcc_spi_writel(tcc_spi_readl(reg_base+TCC_GPSB_GDMA_EXTREQ) | (0x1 << ext_num), reg_base+TCC_GPSB_GDMA_EXTREQ);	// Set EXTREQ
}
#endif /* TCC_DMA_ENGINE */

#else
static void tcc_spi_port_parse_dt(struct device_node *np, struct tca_spi_port_config *pgpios)
{
}
static void tcc_spi_gdma_parse_dt(struct device_node *np, struct tca_spi_gdma_config *pgdma)
{
}
#endif

static int tcc_spi_probe(struct platform_device *pdev)
{
    int ret = 0;
    int irq = -1;
    struct resource *regs = NULL;
    struct resource *regs1 = NULL;
    struct resource *regs2 = NULL;
    struct tca_spi_port_config port;
	struct tca_spi_gdma_config gdma;
    struct spi_master *master = NULL;
    
#ifdef TCC_DMA_ENGINE
	struct tca_spi_handle *tspi = NULL;
#endif

    if (!pdev->dev.of_node)
        return -EINVAL;

	// GPSB Register
    regs = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!regs) {
		dev_err(&pdev->dev,
			"Found SPI Master with no register addr. Check %s setup!\n",
			dev_name(&pdev->dev));
        return -ENXIO;
    }

	// Port Configuration Register
    regs1 = platform_get_resource(pdev, IORESOURCE_MEM, 1);
    if (!regs1) {
		dev_err(&pdev->dev,
			"Found SPI Master PCFG with no register addr. Check %s setup!\n",
			dev_name(&pdev->dev));
        return -ENXIO;
    }

	// PID Table Register
    regs2 = platform_get_resource(pdev, IORESOURCE_MEM, 2);
    if (!regs2) {
		dev_err(&pdev->dev,
			"Found SPI Master PID Tabble with no register addr. Check %s setup!\n",
			dev_name(&pdev->dev));
        return -ENXIO;
    }

    irq = platform_get_irq(pdev, 0);
    if (irq < 0) {
        return -ENXIO;
    }

#ifdef CONFIG_OF
    tcc_spi_port_parse_dt(pdev->dev.of_node, &port);
	tcc_spi_gdma_parse_dt(pdev->dev.of_node, &gdma);
#endif

    master = spi_alloc_master(&(pdev->dev), sizeof(struct tca_spi_handle));
    if (!master) {
        ret = -ENOMEM;
        goto err;
    }

    pdev->id = port.gpsb_id;
    master->bus_num = pdev->id;
    master->num_chipselect = 1;
    master->mode_bits = MODEBITS;
    master->setup = tcc_spi_setup;
    master->transfer = tcc_spi_transfer;
    master->cleanup = tcc_spi_cleanup;
    master->dev.of_node = pdev->dev.of_node;

    memset(&tcc_spi_info[pdev->id], 0x0, sizeof(struct tca_spi_global_setting));
    tcc_spi_info[pdev->id].irq_no = irq;
	tcc_spi_info[pdev->id].phy_reg_base = regs->start;
    tcc_spi_info[pdev->id].reg_base = devm_ioremap_resource(&pdev->dev,regs);
    tcc_spi_info[pdev->id].port_reg = devm_ioremap(&pdev->dev, regs1->start, regs1->end - regs1->start + 1);
    tcc_spi_info[pdev->id].pid_reg = devm_ioremap(&pdev->dev, regs2->start, regs2->end - regs2->start + 1);
    tcc_spi_info[pdev->id].gpsb_clk_name = port.name;
    tcc_spi_info[pdev->id].gpsb_port = port.gpsb_port;
	tcc_spi_info[pdev->id].gdma_use = gdma.gpsb_use_gdma;	// Does GPSB use GDMA?
	tcc_spi_info[pdev->id].dev = &pdev->dev;

	//printk("\x1b[1;33m[%s:%d][ID: %d] reg_base: 0x%x (0x%x)\x1b[0m\n", __func__, __LINE__,pdev->id,tcc_spi_info[pdev->id].reg_base,regs->start);
	//printk("\x1b[1;33m[%s:%d][ID: %d] port_base: 0x%x (0x%x)\x1b[0m\n", __func__, __LINE__,pdev->id,tcc_spi_info[pdev->id].port_reg,regs1->start);
	//printk("\x1b[1;33m[%s:%d][ID: %d] port_base: 0x%x (0x%x)\x1b[0m\n", __func__, __LINE__,pdev->id,tcc_spi_info[pdev->id].pid_reg,regs2->start);

	if(gdma.gpsb_use_gdma)	// GDMA Register (if does not use GDMA Address is NULL)
	{

		printk("\x1b[1;32m[%s:%d] GPSB id: %d (GDMA)\x1b[0m\n", __func__, __LINE__,pdev->id );
#ifndef TCC_DMA_ENGINE
		struct resource *regs3 = NULL;

		printk("\x1b[1;32m[%s:%d] GPSB id: %d (GDMA)\x1b[0m\n", __func__, __LINE__,pdev->id );
		// GDMA Base Address
		regs3 = platform_get_resource(pdev, IORESOURCE_MEM, 3);
		if(!regs3){
			printk("[%s:%d] Fail to gdma probing (need to gdma address)\n", __func__, __LINE__);
			return -1;
		}
		else{
			// GDMA Register Addr
			gdma.reg_base = tcc_spi_gdma_base_ioremap_addr(pdev,regs3,&gdma);
			gdma.tx_reg = tcc_spi_gdma_ioremap_addr(pdev,regs3,&gdma,1);
			gdma.rx_reg = tcc_spi_gdma_ioremap_addr(pdev,regs3,&gdma,0);

			// GDMA Irq Number
			gdma.gdma_irq = platform_get_irq(pdev, 1);
		    if (gdma.gdma_irq < 0) {
				printk("[%s:%d]Fail to gdma probing (need to gdma irq no - %d)\n", __func__, __LINE__,gdma.gdma_irq);
		        return -ENXIO;
		    }
			printk("GDMA - Addr[0x%p] TX[%d:%d:%d] RX[%d:%d:%d] IRQ[%d]\n",
				gdma.reg_base,gdma.tx_ctrl,gdma.tx_ch,gdma.tx_ext,
				gdma.rx_ctrl,gdma.rx_ch,gdma.rx_ext,gdma.gdma_irq);

			tcc_spi_info[pdev->id].gdma = gdma;
			//tca_spi_print_gdma_config(tcc_spi_info[pdev->id].gdma);
		}
#else
		tspi = spi_master_get_devdata(master);
		if(tspi)
			ret = tcc_spi_dma_configuration(pdev, tspi);
		else
			printk("\x1b[1;33m[%s:%d] tca_spi_handle tspi is NULL\x1b[0m\n", __func__, __LINE__);

		if(ret != 0)
		{
			printk("\x1b[1;33m[%s:%d] tcc dma engine configration fail!!\x1b[0m\n", __func__, __LINE__);
			goto err;
		}
			tcc_spi_info[pdev->id].gdma = gdma;
#endif
	}
	else
	{
		printk("\x1b[1;31m[%s:%d] GPSB id: %d (Dedicated DMA)\x1b[0m\n", __func__, __LINE__, pdev->id );
	}

    platform_set_drvdata(pdev, master);

    ret = spi_register_master(master);

    printk("[%s:%d]%d, %d, %p, %d, %s, ret = %d\n",__func__, __LINE__,
		pdev->id, irq, tcc_spi_info[pdev->id].reg_base, port.gpsb_port, port.name, ret);

    if (ret) { goto err; }

	ret = device_create_file(&pdev->dev, &dev_attr_tcc_port);

    printk("%s%d: init[%d]\n", pdev->name, pdev->id, ret);
    return 0;

err:
    spi_master_put(master);

    return ret;
}

static int tcc_spi_remove(struct platform_device *pdev)
{
    struct spi_master *master = platform_get_drvdata(pdev);

#ifdef TCC_DMA_ENGINE
	struct tca_spi_handle *tspi = NULL;
	tspi = spi_master_get_devdata(master);
	tcc_spi_release_dma(tspi);
#endif
    spi_unregister_master(master);
	device_remove_file(&pdev->dev, &dev_attr_tcc_port);
    return 0;
}

static int tcc_spi_suspend(struct platform_device *pdev, pm_message_t state)
{
    if(tcc_spi_info[pdev->id].open){
        //tcc_spi_info[pdev->id].open = 0;
        tcc_spi_close(tcc_spi_info[pdev->id].current_spi);
    }
	return 0;
}
static int tcc_spi_resume(struct platform_device *pdev)
{
	if(tcc_spi_info[pdev->id].open){
		tcc_spi_info[pdev->id].open = 0;
		tcc_spi_setup(tcc_spi_info[pdev->id].current_spi);
	}
	return 0;
}

#ifdef CONFIG_OF
static struct of_device_id tcc_spi_of_match[] = {
	{ .compatible = "telechips,tcc89xx-spi" },
	{}
};
MODULE_DEVICE_TABLE(of, tcc_spi_of_match);

static struct spi_board_info spi0_board_info[] = {
    {
        .modalias = "spidev",
        .bus_num = 0,                   // spi channel 0
        .chip_select = 0,
        /* you can modify the following member variables */
        .max_speed_hz = 2*1000*1000,    // default 2Mhz
        .mode = 0                       // default 0, you can choose [SPI_CPOL|SPI_CPHA|SPI_CS_HIGH|SPI_LSB_FIRST]
    },
};
/* if need to more spi device, add spi board info */

static struct spi_board_info spi1_board_info[] = {
    {
        .modalias = "spidev",
        .bus_num = 1,                   // spi channel 0
        .chip_select = 0,
        // you can modify the following member variables
#if defined(INCLUDE_TDMB)
        .max_speed_hz = 10*1000*1000,    // default 10Mhz
	.mode = SPI_MODE_1,
#else
	.max_speed_hz = 2*1000*1000,    // default 2Mhz
        .mode = 0                       // default 0, you can choose [SPI_CPOL|SPI_CPHA|SPI_CS_HIGH|SPI_LSB_FIRST]
#endif
    },
};

static struct spi_board_info spi2_board_info[] = {
    {
        .modalias = "spidev",
        .bus_num = 2,                   // spi channel 0
        .chip_select = 0,
        // you can modify the following member variables
#if defined(INCLUDE_TDMB)
	 .max_speed_hz = 10*1000*1000,    // 10Mhz
	 .mode = SPI_MODE_1,             // default 0, you can choose [SPI_CPOL|SPI_CPHA|SPI_CS_HIGH|SPI_LSB_FIRST]
#else
        .max_speed_hz = 10*1000*1000,    // 10Mhz
        .mode = 0               // default 0, you can choose [SPI_CPOL|SPI_CPHA|SPI_CS_HIGH|SPI_LSB_FIRST]
#endif
    },
};


#endif

static struct platform_driver tcc_spidrv = {
    .probe		= tcc_spi_probe,
    .remove		= tcc_spi_remove,
    .suspend		= tcc_spi_suspend,
    .resume		= tcc_spi_resume,
    .driver		= {
        .name		= "tcc-spi",
        .owner		= THIS_MODULE,
        .of_match_table = of_match_ptr(tcc_spi_of_match)
   },
};

static int __init tcc_spi_init(void)
{
	printk("%s\n", __func__);
#ifdef CONFIG_OF
//	spi_register_board_info(spi0_board_info, ARRAY_SIZE(spi0_board_info));
/* if need to more spi device, add spi board info */
	spi_register_board_info(spi1_board_info, ARRAY_SIZE(spi1_board_info));
	spi_register_board_info(spi2_board_info, ARRAY_SIZE(spi2_board_info));
#endif
	return platform_driver_register(&tcc_spidrv);
}

static void __exit tcc_spi_exit(void)
{
    platform_driver_unregister(&tcc_spidrv);
}
module_init(tcc_spi_init);
module_exit(tcc_spi_exit);

MODULE_AUTHOR("Telechips Inc. linux@telechips.com");
MODULE_DESCRIPTION("Telechips GPSB Master (SPI) Driver");
MODULE_LICENSE("GPL");
