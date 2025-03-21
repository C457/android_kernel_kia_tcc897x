/*
 * Debugfs support for hosts and cards
 *
 * Copyright (C) 2008 Atmel Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/moduleparam.h>
#include <linux/export.h>
#include <linux/debugfs.h>
#include <linux/fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/fault-inject.h>

#include <linux/mmc/card.h>
#include <linux/mmc/host.h>

#include "core.h"
#include "mmc_ops.h"

#ifdef CONFIG_FAIL_MMC_REQUEST

static DECLARE_FAULT_ATTR(fail_default_attr);
static char *fail_request;
module_param(fail_request, charp, 0);

#endif /* CONFIG_FAIL_MMC_REQUEST */

/* The debugfs functions are optimized away when CONFIG_DEBUG_FS isn't set. */
static int mmc_ios_show(struct seq_file *s, void *data)
{
	static const char *vdd_str[] = {
		[8]	= "2.0",
		[9]	= "2.1",
		[10]	= "2.2",
		[11]	= "2.3",
		[12]	= "2.4",
		[13]	= "2.5",
		[14]	= "2.6",
		[15]	= "2.7",
		[16]	= "2.8",
		[17]	= "2.9",
		[18]	= "3.0",
		[19]	= "3.1",
		[20]	= "3.2",
		[21]	= "3.3",
		[22]	= "3.4",
		[23]	= "3.5",
		[24]	= "3.6",
	};
	struct mmc_host	*host = s->private;
	struct mmc_ios	*ios = &host->ios;
	const char *str;

	seq_printf(s, "clock:\t\t%u Hz\n", ios->clock);
	if (host->actual_clock)
		seq_printf(s, "actual clock:\t%u Hz\n", host->actual_clock);
	seq_printf(s, "vdd:\t\t%u ", ios->vdd);
	if ((1 << ios->vdd) & MMC_VDD_165_195)
		seq_printf(s, "(1.65 - 1.95 V)\n");
	else if (ios->vdd < (ARRAY_SIZE(vdd_str) - 1)
			&& vdd_str[ios->vdd] && vdd_str[ios->vdd + 1])
		seq_printf(s, "(%s ~ %s V)\n", vdd_str[ios->vdd],
				vdd_str[ios->vdd + 1]);
	else
		seq_printf(s, "(invalid)\n");

	switch (ios->bus_mode) {
	case MMC_BUSMODE_OPENDRAIN:
		str = "open drain";
		break;
	case MMC_BUSMODE_PUSHPULL:
		str = "push-pull";
		break;
	default:
		str = "invalid";
		break;
	}
	seq_printf(s, "bus mode:\t%u (%s)\n", ios->bus_mode, str);

	switch (ios->chip_select) {
	case MMC_CS_DONTCARE:
		str = "don't care";
		break;
	case MMC_CS_HIGH:
		str = "active high";
		break;
	case MMC_CS_LOW:
		str = "active low";
		break;
	default:
		str = "invalid";
		break;
	}
	seq_printf(s, "chip select:\t%u (%s)\n", ios->chip_select, str);

	switch (ios->power_mode) {
	case MMC_POWER_OFF:
		str = "off";
		break;
	case MMC_POWER_UP:
		str = "up";
		break;
	case MMC_POWER_ON:
		str = "on";
		break;
	default:
		str = "invalid";
		break;
	}
	seq_printf(s, "power mode:\t%u (%s)\n", ios->power_mode, str);
	seq_printf(s, "bus width:\t%u (%u bits)\n",
			ios->bus_width, 1 << ios->bus_width);

	switch (ios->timing) {
	case MMC_TIMING_LEGACY:
		str = "legacy";
		break;
	case MMC_TIMING_MMC_HS:
		str = "mmc high-speed";
		break;
	case MMC_TIMING_SD_HS:
		str = "sd high-speed";
		break;
	case MMC_TIMING_UHS_SDR50:
		str = "sd uhs SDR50";
		break;
	case MMC_TIMING_UHS_SDR104:
		str = "sd uhs SDR104";
		break;
	case MMC_TIMING_UHS_DDR50:
		str = "sd uhs DDR50";
		break;
	case MMC_TIMING_MMC_DDR52:
		str = "mmc DDR52";
		break;
	case MMC_TIMING_MMC_HS200:
		str = "mmc HS200";
		break;
	case MMC_TIMING_MMC_HS400:
		str = "mmc HS400";
		break;
	default:
		str = "invalid";
		break;
	}
	seq_printf(s, "timing spec:\t%u (%s)\n", ios->timing, str);

	switch (ios->signal_voltage) {
	case MMC_SIGNAL_VOLTAGE_330:
		str = "3.30 V";
		break;
	case MMC_SIGNAL_VOLTAGE_180:
		str = "1.80 V";
		break;
	case MMC_SIGNAL_VOLTAGE_120:
		str = "1.20 V";
		break;
	default:
		str = "invalid";
		break;
	}
	seq_printf(s, "signal voltage:\t%u (%s)\n", ios->chip_select, str);

	return 0;
}

static int mmc_ios_open(struct inode *inode, struct file *file)
{
	return single_open(file, mmc_ios_show, inode->i_private);
}

static const struct file_operations mmc_ios_fops = {
	.open		= mmc_ios_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int mmc_clock_opt_get(void *data, u64 *val)
{
	struct mmc_host *host = data;

	*val = host->ios.clock;

	return 0;
}

static int mmc_clock_opt_set(void *data, u64 val)
{
	struct mmc_host *host = data;

	/* We need this check due to input value is u64 */
	if (val > host->f_max)
		return -EINVAL;

	mmc_claim_host(host);
	mmc_set_clock(host, (unsigned int) val);
	mmc_release_host(host);

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(mmc_clock_fops, mmc_clock_opt_get, mmc_clock_opt_set,
	"%llu\n");

void mmc_add_host_debugfs(struct mmc_host *host)
{
	struct dentry *root;

	root = debugfs_create_dir(mmc_hostname(host), NULL);
	if (IS_ERR(root))
		/* Don't complain -- debugfs just isn't enabled */
		return;
	if (!root)
		/* Complain -- debugfs is enabled, but it failed to
		 * create the directory. */
		goto err_root;

	host->debugfs_root = root;

	if (!debugfs_create_file("ios", S_IRUSR, root, host, &mmc_ios_fops))
		goto err_node;

	if (!debugfs_create_file("clock", S_IRUSR | S_IWUSR, root, host,
			&mmc_clock_fops))
		goto err_node;

#ifdef CONFIG_MMC_CLKGATE
	if (!debugfs_create_u32("clk_delay", (S_IRUSR | S_IWUSR),
				root, &host->clk_delay))
		goto err_node;
#endif
#ifdef CONFIG_FAIL_MMC_REQUEST
	if (fail_request)
		setup_fault_attr(&fail_default_attr, fail_request);
	host->fail_mmc_request = fail_default_attr;
	if (IS_ERR(fault_create_debugfs_attr("fail_mmc_request",
					     root,
					     &host->fail_mmc_request)))
		goto err_node;
#endif
	return;

err_node:
	debugfs_remove_recursive(root);
	host->debugfs_root = NULL;
err_root:
	dev_err(&host->class_dev, "failed to initialize debugfs\n");
}

void mmc_remove_host_debugfs(struct mmc_host *host)
{
	debugfs_remove_recursive(host->debugfs_root);
}

static int mmc_dbg_card_status_get(void *data, u64 *val)
{
	struct mmc_card	*card = data;
	u32		status;
	int		ret;

	mmc_get_card(card);

	ret = mmc_send_status(data, &status);
	if (!ret)
		*val = status;

	mmc_put_card(card);

	return ret;
}
DEFINE_SIMPLE_ATTRIBUTE(mmc_dbg_card_status_fops, mmc_dbg_card_status_get,
		NULL, "%08llx\n");

#define EXT_CSD_STR_LEN 1025

static int mmc_ext_csd_open(struct inode *inode, struct file *filp)
{
	struct mmc_card *card = inode->i_private;
	char *buf;
	ssize_t n = 0;
	u8 *ext_csd;
	int err, i;

	buf = kmalloc(EXT_CSD_STR_LEN + 1, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	ext_csd = kmalloc(512, GFP_KERNEL);
	if (!ext_csd) {
		err = -ENOMEM;
		goto out_free;
	}

	mmc_get_card(card);
	err = mmc_send_ext_csd(card, ext_csd);
	mmc_put_card(card);
	if (err)
		goto out_free;

	for (i = 0; i < 512; i++)
		n += sprintf(buf + n, "%02x", ext_csd[i]);
	n += sprintf(buf + n, "\n");
	BUG_ON(n != EXT_CSD_STR_LEN);

	filp->private_data = buf;
	kfree(ext_csd);
	return 0;

out_free:
	kfree(buf);
	kfree(ext_csd);
	return err;
}

static ssize_t mmc_ext_csd_read(struct file *filp, char __user *ubuf,
				size_t cnt, loff_t *ppos)
{
	char *buf = filp->private_data;

	return simple_read_from_buffer(ubuf, cnt, ppos,
				       buf, EXT_CSD_STR_LEN);
}

static int mmc_ext_csd_release(struct inode *inode, struct file *file)
{
	kfree(file->private_data);
	return 0;
}

static const struct file_operations mmc_dbg_ext_csd_fops = {
	.open		= mmc_ext_csd_open,
	.read		= mmc_ext_csd_read,
	.release	= mmc_ext_csd_release,
	.llseek		= default_llseek,
};

#ifdef CONFIG_DAUDIO_KK
#define MICRON_TLC_MANIFID 0x13
#define MAX_PRE_LOADING_DATA_LEN 8
#define MAX_PRE_LOADING_DATA_64G "0028b706" // please refer micron datasheet 0x06b72800h
#define MAX_PRE_LOADING_DATA_128G "00506e0d" // please refer micron datasheet 0x0d6e5000h
static int mtlc_check_value=0;

#define MMC_GEN_CMD_ARGUMENT 0x00000039
#define MMC_GEN_CMD_STR_COMPLETE_LEN 5
//========================================================================
static int mmc_ext_csd_open_ex(struct inode *inode, struct file *filp)
{
	struct mmc_card *card = inode->i_private;
	char *buf;
	char bbuf[EXT_CSD_STR_LEN+1] = {0,};
	ssize_t n = 0;
	u8 *ext_csd;
	int err, i;

	buf = kmalloc(MAX_PRE_LOADING_DATA_LEN + 1, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	memset(buf, 0x00, MAX_PRE_LOADING_DATA_LEN + 1);

	if (card->cid.manfid != MICRON_TLC_MANIFID)
	{
		strcpy(buf, "false\n");
		mtlc_check_value=-1;
		filp->private_data = buf;
		return 0;
	}

	ext_csd = kmalloc(512, GFP_KERNEL);
	if (!ext_csd) {
		err = -ENOMEM;
		goto out_free;
	}

	mmc_get_card(card);
	err = mmc_send_ext_csd(card, ext_csd);
	mmc_put_card(card);
	if (err)
		goto out_free;

	for (i = 0; i < 512; i++)
		n += sprintf(bbuf + n, "%02x", ext_csd[i]);
	n += sprintf(bbuf + n, "\n");
	BUG_ON(n != EXT_CSD_STR_LEN);

	// micron datasheet MAX_PRE_LOADING_DATA_SIZE ECSD[18~21]
	strncpy(buf, &bbuf[36], 8);
	if((strcmp(buf, MAX_PRE_LOADING_DATA_64G) == 0) || (strcmp(buf, MAX_PRE_LOADING_DATA_128G) == 0))
	{
		mtlc_check_value=1;
		memset(buf, 0x00, MAX_PRE_LOADING_DATA_LEN + 1);
		strcpy(buf, "true\n");
	}
	else
	{
		mtlc_check_value=-1;
		memset(buf, 0x00, MAX_PRE_LOADING_DATA_LEN + 1);
		strcpy(buf, "false\n");
	}

	filp->private_data = buf;
	kfree(ext_csd);
	return 0;

out_free:
	kfree(buf);
	kfree(ext_csd);
	return err;
}

static ssize_t mmc_ext_csd_read_ex(struct file *filp, char __user *ubuf,
				size_t cnt, loff_t *ppos)
{
	char *buf = filp->private_data;

	return simple_read_from_buffer(ubuf, cnt, ppos,
				       buf, MAX_PRE_LOADING_DATA_LEN+1);
}

static const struct file_operations mmc_dbg_fops_mtlc_check = {
	.open		= mmc_ext_csd_open_ex,
	.read		= mmc_ext_csd_read_ex,
	.release	= mmc_ext_csd_release,
	.llseek		= default_llseek,
};

//========================================================================
static int mmc_gen_cmd_open(struct inode *inode, struct file *filp)
{
	struct mmc_card *card = inode->i_private;
	int *buf;
	u8 *gen_buffer;
	int err;
	int percentage=0;
	unsigned int upercentage=0;

	buf = kmalloc(MMC_GEN_CMD_STR_COMPLETE_LEN, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	memset(buf, 0x00, MMC_GEN_CMD_STR_COMPLETE_LEN);

	gen_buffer = kmalloc(512, GFP_KERNEL);
	if (!gen_buffer) {
		err = -ENOMEM;
		goto out_free;
	}

	mmc_get_card(card);
	err = mmc_send_gen56_data(card, gen_buffer);
	mmc_put_card(card);
	if (err)
		goto out;

	// refer micron data sheet
	//sprintf(&buf[0], "%d", gen_buffer[217]);	//0xD9
	//sprintf(&buf[1], "%d", gen_buffer[216]);	//0xD8
	// 0x2710 => 10000,  divide by 100 => 100%
	percentage = (int)gen_buffer[217] *256;		// data * 0x100
	percentage = percentage + (int)gen_buffer[216];

	if (percentage > 10000)
		strcpy(buf, "0%\n");
	else if (mtlc_check_value == -1)
		strcpy(buf, "0%\n");
	else
	{
		upercentage  = percentage/100;
		sprintf(buf, "%3d%\n", upercentage);
	}
	filp->private_data = buf;
	kfree(gen_buffer);
	return 0;

out_free:
	kfree(buf);
	kfree(gen_buffer);
	return err;

out :
	strcpy(buf, "0%\n");
	filp->private_data = buf;
	printk("eMMC gen56 command timeout error!!\n");
	kfree(gen_buffer);
	return 0;
}

static ssize_t mmc_gen_cmd_read(struct file *filp, char __user *ubuf,
				size_t cnt, loff_t *ppos)
{
	char *buf = filp->private_data;
	return simple_read_from_buffer(ubuf, cnt, ppos, buf, MMC_GEN_CMD_STR_COMPLETE_LEN);
}

static int mmc_gen_cmd_relase(struct inode *inode, struct file *file)
{
	kfree(file->private_data);
	return 0;
}


static const struct file_operations mmc_dbg_fops_mtlc_complete = {
	.open		= mmc_gen_cmd_open,
	.read		= mmc_gen_cmd_read,
	.release	= mmc_gen_cmd_relase,
	.llseek		= default_llseek,
};
#endif //CONFIG_WIDE_PE_COMMON


void mmc_add_card_debugfs(struct mmc_card *card)
{
	struct mmc_host	*host = card->host;
	struct dentry	*root;

	if (!host->debugfs_root)
		return;

	root = debugfs_create_dir(mmc_card_id(card), host->debugfs_root);
	if (IS_ERR(root))
		/* Don't complain -- debugfs just isn't enabled */
		return;
	if (!root)
		/* Complain -- debugfs is enabled, but it failed to
		 * create the directory. */
		goto err;

	card->debugfs_root = root;

	if (!debugfs_create_x32("state", S_IRUSR, root, &card->state))
		goto err;

	if (mmc_card_mmc(card) || mmc_card_sd(card))
		if (!debugfs_create_file("status", S_IRUSR, root, card,
					&mmc_dbg_card_status_fops))
			goto err;

	if (mmc_card_mmc(card))
		if (!debugfs_create_file("ext_csd", S_IRUSR, root, card,
					&mmc_dbg_ext_csd_fops))
			goto err;

#ifdef CONFIG_DAUDIO_KK
	if (mmc_card_mmc(card))
		if (!debugfs_create_file("mtlc_check", S_IRUGO, root, card,
					&mmc_dbg_fops_mtlc_check))
			goto err;
	if (mmc_card_mmc(card))
		if (!debugfs_create_file("mtlc_complete", S_IRUGO, root, card,
					&mmc_dbg_fops_mtlc_complete))
			goto err;
#endif
	return;

err:
	debugfs_remove_recursive(root);
	card->debugfs_root = NULL;
	dev_err(&card->dev, "failed to initialize debugfs\n");
}

void mmc_remove_card_debugfs(struct mmc_card *card)
{
	debugfs_remove_recursive(card->debugfs_root);
}
