/* 
 * linux/drivers/char/tcc_dxb/ctrl/tcc_dxb_control.c
 *
 * Author:  <linux@telechips.com>
 * Created: 10th Jun, 2008 
 * Description: Telechips Linux DxB Control DRIVER
 *
 * Copyright (c) Telechips, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/of_gpio.h>
//#include <mach/gpio.h>
#include <linux/sched.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#include "../tcc_hwdemux/tcc_cipher_ioctl.h"
#include "../tcc_hwdemux/tca_hwdemux_tsif.h"

#include "tcc_dxb_control.h"


/*****************************************************************************
 * Log Message
 ******************************************************************************/
#define LOG_TAG    "[TCC_DXB_CTRL]"

static int dev_debug = 0;

module_param(dev_debug, int, 0644);
MODULE_PARM_DESC(dev_debug, "Turn on/off device debugging (default:off).");

#define dprintk(msg...)                                \
{                                                      \
	if (dev_debug)                                     \
		printk(KERN_INFO LOG_TAG "(D)" msg);           \
}

#define eprintk(msg...)                                \
{                                                      \
	printk(KERN_INFO LOG_TAG " (E) " msg);             \
}


/*****************************************************************************
 * Defines
 ******************************************************************************/
#define MAX_DEVICE_NO  (3)
#define PWRCTRL_OFF     0
#define PWRCTRL_ON      1
#define PWRCTRL_AUTO    2

#define DXB_LOCK


/*****************************************************************************
 * structures
 ******************************************************************************/
struct tcc_dxb_ctrl_t
{
	int board_type;
	int power_status[MAX_DEVICE_NO + 1];

	int ant_ctrl_mode;

	int gpio_dxb_on;

	int gpio_dxb_0_pwdn;
	int gpio_dxb_0_rst;
	int gpio_dxb_0_irq;
	int gpio_dxb_0_sdo;

	int gpio_dxb_1_pwdn;
	int gpio_dxb_1_rst;
	int gpio_dxb_1_irq;
	int gpio_dxb_1_sdo;

	int gpio_ant_pwr;
	int gpio_check_ant_overload;

	int irq_check_ant_overlaod;

#ifdef DXB_LOCK
	struct mutex mLock;
	int iLockInstance;
	int iWaitLockInstance;
	wait_queue_head_t *pWaitQueue;
#endif
};

#ifdef DXB_LOCK
typedef struct {
	int lock_instance;
	wait_queue_head_t wait_q;
} tcc_dxb_ctrl_private_t;
#endif


/*****************************************************************************
 * Variables
 ******************************************************************************/
static struct class *tcc_dxb_ctrl_class;
struct tcc_dxb_ctrl_t *gDxbCtrl = NULL;


/*****************************************************************************
 * Gpio Functions
 ******************************************************************************/
static void GPIO_OUT_INIT(int pin)
{
	int rc;
	if (!gpio_is_valid(pin)) {
		eprintk("%s pin(0x%X) error\n", __FUNCTION__, pin);
		return;
	}
	rc = gpio_request(pin, NULL);
	if (rc) {
		eprintk("%s pin(0x%X) error(%d)\n", __FUNCTION__, pin, rc);
		return;
	}
	printk("%s pin[0x%X] value[0]\n", __func__, pin);
	gpio_direction_output(pin, 0);
	gpio_free(pin);
}

static void GPIO_IN_INIT(int pin)
{
	int rc;
	if (!gpio_is_valid(pin)) {
		eprintk("%s pin(0x%X) error\n", __FUNCTION__, pin);
		return;
	}
	rc = gpio_request(pin, NULL);
	if (rc) {
		eprintk("%s pin(0x%X) error(%d)\n", __FUNCTION__, pin, rc);
		return;
	}
	dprintk("%s pin[0x%X]\n", __func__, pin);
	gpio_direction_input(pin);
}

static void GPIO_SET_VALUE(int pin, int value)
{
	if (!gpio_is_valid(pin)) {
		eprintk("%s pin(0x%X) error\n", __FUNCTION__, pin);
		return;
	}
	dprintk("%s pin[0x%X] value[%d]\n", __func__, pin, value);
	if (gpio_cansleep(pin))
		gpio_set_value_cansleep(pin, value);
	else
		gpio_set_value(pin, value);
}

static int GPIO_GET_VALUE(int pin)
{
	if (!gpio_is_valid(pin)) {
		eprintk("%s pin(0x%X) error\n", __FUNCTION__, pin);
		return -1;
	}
	dprintk("%s pin[0x%X]\n", __func__, pin);
	if (gpio_cansleep(pin))
		return gpio_get_value_cansleep(pin);
	else
		return gpio_get_value(pin);
}

static void GPIO_FREE(int pin)
{
	dprintk("%s pin[0x%X]\n", __func__, pin);
	if (gpio_is_valid(pin)) {
		gpio_free(pin);
	}
}


/*****************************************************************************
 * Baseband Functions
 ******************************************************************************/
#include "tcc3150.h"
#include "tcc3161.h"
#include "dib9090.h"
#include "tcc3510.h"
#include "tcc353x.h"
#include "tcc3171.h"

struct baseband {
	int type;
	int (*ioctl)(struct tcc_dxb_ctrl_t *ctrl, unsigned int cmd, unsigned long arg);
	int tsif_mode; // tsif interface (0: serial, 1: parallel)
	int ant_ctrl;  // antenna power control (0: disable, 1: enable)
} BB[] = {
	{ BOARD_TDMB_TCC3150,       TCC3150_IOCTL,      0,  0 },
	{ BOARD_TDMB_TCC3161,       TCC3161_IOCTL,      0,  0 },
	{ BOARD_DVBT_DIB7070,       NULL,               0,  0 },
	{ BOARD_DVBT_DIB9090,       DIB9090_IOCTL,      0,  0 },
	{ BOARD_ISDBT_DIB10096,     NULL,               0,  0 },
	{ BOARD_DXB_TCC3510,        TCC3510_IOCTL,      0,  0 },
	{ BOARD_DVBT_DIB9090M_PA,   NULL,               0,  0 },
	{ BOARD_ISDBT_MTV818,       NULL,               0,  0 },
	{ BOARD_DXB_NMI32X,         NULL,               0,  0 },
	{ BOARD_ISDBT_TOSHIBA,      NULL,               0,  0 },
	{ BOARD_ISDBT_TCC353X,      TCC353X_IOCTL,      0,  0 },
	{ BOARD_ISDBT_TCC353X_FSMA, NULL,               0,  0 },
	{ BOARD_DVBT_MXL101SF_YJ,   NULL,               1,  1 },
	{ BOARD_DVBT2_MN88472_YJ,   NULL,               1,  1 },
	{ BOARD_DVBS2_AVL6211_YJ,   NULL,               1,  0 },
	{ BOARD_DXB_TCC3171,        TCC3171_IOCTL,      0,  0 },
};


/*****************************************************************************
 * TCC_DXB_CTRL Functions
 ******************************************************************************/
static irqreturn_t isr_checking_ant_overload(int irq, void *dev_data)
{
	struct tcc_dxb_ctrl_t *ctrl = (struct tcc_dxb_ctrl_t *)dev_data;

	printk("%s : ant overload = %d\n", __func__, GPIO_GET_VALUE(ctrl->gpio_check_ant_overload));
	if(GPIO_GET_VALUE(ctrl->gpio_check_ant_overload) == 1)
	{
		GPIO_SET_VALUE(ctrl->gpio_ant_pwr, 1);
	}
	else
	{
		GPIO_SET_VALUE(ctrl->gpio_ant_pwr, 0);
	}
	return IRQ_HANDLED;
}

static int tcc_dxb_ctrl_ant_on(struct tcc_dxb_ctrl_t *ctrl)
{
	int err;

	if (ctrl->gpio_ant_pwr == 0)
		return 0;

	switch (ctrl->ant_ctrl_mode)
	{
	case PWRCTRL_OFF:
		break;

	case PWRCTRL_ON:
		GPIO_OUT_INIT(ctrl->gpio_ant_pwr);
		GPIO_SET_VALUE(ctrl->gpio_ant_pwr, 1);//ANT_PWR_CTRL
		break;
	case PWRCTRL_AUTO:
		GPIO_OUT_INIT(ctrl->gpio_ant_pwr);
		GPIO_SET_VALUE(ctrl->gpio_ant_pwr, 1);

		if (ctrl->gpio_check_ant_overload == 0)
			break;

		GPIO_IN_INIT(ctrl->gpio_check_ant_overload);
		msleep (5);
		if(GPIO_GET_VALUE(ctrl->gpio_check_ant_overload) == 0) {
			printk("ANT_OVERLOAD is low\n");
			GPIO_SET_VALUE(ctrl->gpio_ant_pwr, 0);
			return 0;
		}
		ctrl->irq_check_ant_overlaod = gpio_to_irq(gDxbCtrl->gpio_check_ant_overload);
		#if 1
			err = request_irq(ctrl->irq_check_ant_overlaod, isr_checking_ant_overload, IRQ_TYPE_EDGE_FALLING | IRQF_DISABLED, "ANT_OVERLOAD_F", NULL);
		#else
			err = request_irq(ctrl->irq_check_ant_overlaod, isr_checking_ant_overload, IRQ_TYPE_EDGE_RISING  | IRQF_DISABLED, "ANT_OVERLOAD_R", NULL);
		#endif
		if (err) {
			printk("Unable to request ANT_OVERLOAD IRQ[%d].\n", err);
			ctrl->irq_check_ant_overlaod = 0;
		}
		break;
	}

	return 0;
}

static int tcc_dxb_ctrl_ant_off(struct tcc_dxb_ctrl_t *ctrl)
{
	if (ctrl->gpio_ant_pwr == 0)
		return 0;

	if (ctrl->irq_check_ant_overlaod)
		free_irq(ctrl->irq_check_ant_overlaod, NULL);

	ctrl->irq_check_ant_overlaod = 0;

	GPIO_SET_VALUE(ctrl->gpio_ant_pwr, 0);

	return 0;
}

static long tcc_dxb_ctrl_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	dprintk("%s cmd[0x%X]\n", __func__, cmd);
	switch (cmd)
	{
		// for HWDEMUX Cipher
		case IOCTL_DXB_CTRL_HWDEMUX_CIPHER_SET_ALGORITHM:
			{
				stHWDEMUXCIPHER_ALGORITHM stAlgorithm;
				if (copy_from_user((void *)&stAlgorithm, (const void *)arg, sizeof(stHWDEMUXCIPHER_ALGORITHM)) == 0)
				{
					tca_hwdemux_cipher_Set_Algorithm(&stAlgorithm);
				}
			}
			break;

		case IOCTL_DXB_CTRL_HWDEMUX_CIPHER_SET_KEY:
			{
				stHWDEMUXCIPHER_KEY stKeyInfo;
				if (copy_from_user((void *)&stKeyInfo, (const void *)arg, sizeof(stHWDEMUXCIPHER_KEY)) == 0)
				{
					tca_hwdemux_cipher_Set_Key(&stKeyInfo);
				}
			}
			break;

		case IOCTL_DXB_CTRL_HWDEMUX_CIPHER_SET_VECTOR:
			{
				stHWDEMUXCIPHER_VECTOR stVectorInfo;
				if (copy_from_user((void *)&stVectorInfo, (const void *)arg, sizeof(stHWDEMUXCIPHER_VECTOR)) == 0)
				{
					tca_hwdemux_cipher_Set_IV(&stVectorInfo);
				}
			}
			break;

		case IOCTL_DXB_CTRL_HWDEMUX_CIPHER_ENCRYPT:
			{
				stHWDEMUXCIPHER_EXECUTE stEncryptInfo;
				if (copy_from_user((void *)&stEncryptInfo, (const void *)arg, sizeof(stHWDEMUXCIPHER_EXECUTE)) == 0)
				{
					tca_hwdemux_cipher_Cipher_Run(&stEncryptInfo);
				}
			}
			break;

		case IOCTL_DXB_CTRL_HWDEMUX_CIPHER_DECRYPT:
			{
				stHWDEMUXCIPHER_EXECUTE stDecryptInfo;
				if (copy_from_user((void *)&stDecryptInfo, (const void *)arg, sizeof(stHWDEMUXCIPHER_EXECUTE)) == 0)
				{
					tca_hwdemux_cipher_Cipher_Run(&stDecryptInfo);
				}
			}
			break;

		case IOCTL_DXB_CTRL_HWDEMUX_CIPHER_EXECUTE:
			{
				stHWDEMUXCIPHER_EXECUTE stExecute;
				if (copy_from_user((void *)&stExecute, (const void *)arg, sizeof(stHWDEMUXCIPHER_EXECUTE)) == 0)
				{
					tca_hwdemux_cipher_Cipher_Run(&stExecute);
				}
			}
			break;

		case IOCTL_DXB_CTRL_SET_CTRLMODE:
			{
				unsigned int uiAntCtrlMode;
				if (copy_from_user((void *)&uiAntCtrlMode, (const void *)arg, sizeof(unsigned int)) != 0)
					break;
				gDxbCtrl->ant_ctrl_mode = uiAntCtrlMode;
			}
			break;

		case IOCTL_DXB_CTRL_OFF:
			{
				if (gDxbCtrl->board_type < BOARD_MAX)
				{
					if (BB[gDxbCtrl->board_type].ant_ctrl == 1)
					{
						tcc_dxb_ctrl_ant_off(gDxbCtrl);
					}
					if (BB[gDxbCtrl->board_type].ioctl != NULL)
					{
						return BB[gDxbCtrl->board_type].ioctl(gDxbCtrl, cmd, arg);
					}
				}
			}
			break;

		case IOCTL_DXB_CTRL_ON:
			{
				if (gDxbCtrl->board_type < BOARD_MAX)
				{
					if (BB[gDxbCtrl->board_type].ant_ctrl == 1)
					{
						tcc_dxb_ctrl_ant_on(gDxbCtrl);
					}
					if (BB[gDxbCtrl->board_type].ioctl != NULL)
					{
						return BB[gDxbCtrl->board_type].ioctl(gDxbCtrl, cmd, arg);
					}
				}
			}
			break;

		case IOCTL_DXB_CTRL_SET_BOARD:
			{
				unsigned int uiboardtype;
				if (copy_from_user((void *)&uiboardtype, (const void *)arg, sizeof(unsigned int)) != 0)
					break;

				gDxbCtrl->board_type = uiboardtype;
				if (gDxbCtrl->board_type < BOARD_MAX)
				{
					tca_tsif_set_interface(BB[gDxbCtrl->board_type].tsif_mode);
					if (BB[gDxbCtrl->board_type].ioctl != NULL)
					{
						return BB[gDxbCtrl->board_type].ioctl(gDxbCtrl, cmd, arg);
					}
				}
			}
			break;
#ifdef DXB_LOCK
		case IOCTL_DXB_CTRL_UNLOCK:
			{
				tcc_dxb_ctrl_private_t *priv = (tcc_dxb_ctrl_private_t*) filp->private_data;
				if (priv->lock_instance == -1)
					return -1;

				mutex_lock(&gDxbCtrl->mLock);
				if (gDxbCtrl->iLockInstance != priv->lock_instance)
				{
					mutex_unlock(&gDxbCtrl->mLock);
					return -2;
				}
				if (gDxbCtrl->iWaitLockInstance != -1)
				{
					gDxbCtrl->iLockInstance = gDxbCtrl->iWaitLockInstance;
					gDxbCtrl->iWaitLockInstance = -1;
					wake_up(gDxbCtrl->pWaitQueue);
				}
				else
				{
					gDxbCtrl->iLockInstance = -1;
				}
				mutex_unlock(&gDxbCtrl->mLock);
			}
			break;
		case IOCTL_DXB_CTRL_LOCK:
			{
				tcc_dxb_ctrl_private_t *priv= (tcc_dxb_ctrl_private_t*) filp->private_data;
				priv->lock_instance = (int) arg;
				mutex_lock(&gDxbCtrl->mLock);
				if (gDxbCtrl->iLockInstance != -1)
				{
					if (gDxbCtrl->iWaitLockInstance != -1)
					{
						mutex_unlock(&gDxbCtrl->mLock);
						return -1;
					}
					gDxbCtrl->iWaitLockInstance = priv->lock_instance;
					gDxbCtrl->pWaitQueue = &priv->wait_q;
					mutex_unlock(&gDxbCtrl->mLock);
					wait_event_timeout(priv->wait_q, gDxbCtrl->iLockInstance == priv->lock_instance, msecs_to_jiffies(100));
					mutex_lock(&gDxbCtrl->mLock);
					if (gDxbCtrl->iLockInstance != priv->lock_instance)
					{
						gDxbCtrl->iWaitLockInstance = -1;
						mutex_unlock(&gDxbCtrl->mLock);
						return -2;
					}
				}
				gDxbCtrl->iLockInstance = priv->lock_instance;
				mutex_unlock(&gDxbCtrl->mLock);
				break;
			}
			break;
#endif
		default:
			{
				if (gDxbCtrl->board_type < BOARD_MAX)
				{
					if (BB[gDxbCtrl->board_type].ioctl != NULL)
					{
						return BB[gDxbCtrl->board_type].ioctl(gDxbCtrl, cmd, arg);
					}
				}
			}
			break;
	}
	return 0;
}

static int tcc_dxb_ctrl_release(struct inode *inode, struct file *filp)
{
#ifdef DXB_LOCK
	tcc_dxb_ctrl_private_t *priv = (tcc_dxb_ctrl_private_t*) filp->private_data;

	if (priv->lock_instance != -1)
	{
		wake_up(&priv->wait_q);

		mutex_lock(&gDxbCtrl->mLock);
		if (gDxbCtrl->iLockInstance == priv->lock_instance)
		{
			gDxbCtrl->iLockInstance = -1;
		}
		if (gDxbCtrl->iWaitLockInstance == priv->lock_instance)
		{
			gDxbCtrl->iWaitLockInstance = -1;
		}
		mutex_unlock(&gDxbCtrl->mLock);
	}

	kfree(priv);
#endif
	GPIO_FREE(gDxbCtrl->gpio_dxb_on);

	GPIO_FREE(gDxbCtrl->gpio_dxb_0_pwdn);
	GPIO_FREE(gDxbCtrl->gpio_dxb_0_rst);
	GPIO_FREE(gDxbCtrl->gpio_dxb_0_irq);
	GPIO_FREE(gDxbCtrl->gpio_dxb_0_sdo);

	GPIO_FREE(gDxbCtrl->gpio_dxb_1_pwdn);
	GPIO_FREE(gDxbCtrl->gpio_dxb_1_rst);
	GPIO_FREE(gDxbCtrl->gpio_dxb_1_irq);
	GPIO_FREE(gDxbCtrl->gpio_dxb_1_sdo);

	GPIO_FREE(gDxbCtrl->gpio_ant_pwr);
	GPIO_FREE(gDxbCtrl->gpio_check_ant_overload);

	dprintk("%s::%d\n", __FUNCTION__, __LINE__);

	return 0;
}

static int tcc_dxb_ctrl_open(struct inode *inode, struct file *filp)
{
#ifdef DXB_LOCK
	tcc_dxb_ctrl_private_t *priv = kzalloc(sizeof(tcc_dxb_ctrl_private_t), GFP_KERNEL);

	priv->lock_instance = -1;

	init_waitqueue_head(&priv->wait_q);

	filp->private_data = priv;
#endif

	dprintk("%s::%d\n", __FUNCTION__, __LINE__);

	return 0;
}

struct file_operations tcc_dxb_ctrl_fops =
{
	.owner          = THIS_MODULE,
	.open           = tcc_dxb_ctrl_open,
	.release        = tcc_dxb_ctrl_release,
	.unlocked_ioctl = tcc_dxb_ctrl_ioctl,
};


/*****************************************************************************
 * TCC_DXB_CTRL Probe/Remove
 ******************************************************************************/
static ssize_t tcc_dxb_ctrl_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static ssize_t tcc_dxb_ctrl_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int overload = -1;
	if (gDxbCtrl->gpio_check_ant_overload != 0)
	{
		overload = GPIO_GET_VALUE(gDxbCtrl->gpio_check_ant_overload);
	}
	return sprintf(buf, "ANT_OVERLOAD=%d\n", overload);
}
static DEVICE_ATTR(state, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH, tcc_dxb_ctrl_show, tcc_dxb_ctrl_store);


/*****************************************************************************
 * TCC_DXB_CTRL Probe/Remove
 ******************************************************************************/
static int tcc_dxb_ctrl_probe(struct platform_device *pdev)
{
	int res, major_num;
	struct device *dev = &pdev->dev;

	res = register_chrdev(0, DXB_CTRL_DEV_NAME, &tcc_dxb_ctrl_fops);
	if (res < 0)
		return res;

	major_num = res;
	tcc_dxb_ctrl_class = class_create(THIS_MODULE, DXB_CTRL_DEV_NAME);
	if(NULL == device_create(tcc_dxb_ctrl_class, NULL, MKDEV(major_num, DXB_CTRL_DEV_MINOR), NULL, DXB_CTRL_DEV_NAME))
		eprintk("%s device_create failed\n", __FUNCTION__);

	gDxbCtrl = kzalloc(sizeof(struct tcc_dxb_ctrl_t), GFP_KERNEL);
	if (gDxbCtrl == NULL)
		return -ENOMEM;

	if (device_create_file(dev, &dev_attr_state))
		eprintk("Failed to create file.\n");

#ifdef CONFIG_OF
	gDxbCtrl->gpio_dxb_on     = of_get_named_gpio(dev->of_node, "pw-gpios",   0);

	gDxbCtrl->gpio_dxb_0_pwdn = of_get_named_gpio(dev->of_node, "dxb0-gpios", 0);
	gDxbCtrl->gpio_dxb_0_rst  = of_get_named_gpio(dev->of_node, "dxb0-gpios", 1);
	gDxbCtrl->gpio_dxb_0_irq  = of_get_named_gpio(dev->of_node, "dxb0-gpios", 2);
	gDxbCtrl->gpio_dxb_0_sdo  = of_get_named_gpio(dev->of_node, "dxb0-gpios", 3);

	gDxbCtrl->gpio_dxb_1_pwdn = of_get_named_gpio(dev->of_node, "dxb1-gpios", 0);
	gDxbCtrl->gpio_dxb_1_rst  = of_get_named_gpio(dev->of_node, "dxb1-gpios", 1);
	gDxbCtrl->gpio_dxb_1_irq  = of_get_named_gpio(dev->of_node, "dxb1-gpios", 2);
	gDxbCtrl->gpio_dxb_1_sdo  = of_get_named_gpio(dev->of_node, "dxb1-gpios", 3);

	gDxbCtrl->gpio_ant_pwr            = of_get_named_gpio(dev->of_node, "ant-gpios",   0);
	gDxbCtrl->gpio_check_ant_overload = of_get_named_gpio(dev->of_node, "ant-gpios",   1);
#endif
	printk("\x1b[1;33m %s [0x%X][0x%X][0x%X][0x%X][0x%X][0x%X][0x%X][0x%X][0x%X][0x%X][0x%X]\x1b[0m\n", __func__, 
	gDxbCtrl->gpio_dxb_on,
	gDxbCtrl->gpio_dxb_0_pwdn, gDxbCtrl->gpio_dxb_0_rst, gDxbCtrl->gpio_dxb_0_irq, gDxbCtrl->gpio_dxb_0_sdo,
	gDxbCtrl->gpio_dxb_1_pwdn, gDxbCtrl->gpio_dxb_1_rst, gDxbCtrl->gpio_dxb_1_irq, gDxbCtrl->gpio_dxb_1_sdo,
	gDxbCtrl->gpio_ant_pwr, gDxbCtrl->gpio_check_ant_overload);
	#if defined(INCLUDE_ISDB)
	printk("\x1b[1;33m ## [%s:ISDB!!!!] ##\x1b[0m\n", __func__); 
	gDxbCtrl->board_type = BOARD_ISDBT_TCC353X;
	#else
	gDxbCtrl->board_type = BOARD_TDMB_TCC3150;
	#endif
	gDxbCtrl->ant_ctrl_mode = PWRCTRL_AUTO;
#ifdef DXB_LOCK
	gDxbCtrl->iLockInstance = -1;
	gDxbCtrl->iWaitLockInstance = -1;

	mutex_init(&gDxbCtrl->mLock);
#endif
	return 0;
}

static int tcc_dxb_ctrl_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
#ifdef DXB_LOCK
	mutex_destroy(&gDxbCtrl->mLock);
#endif
	device_remove_file(dev, &dev_attr_state);
	device_destroy(tcc_dxb_ctrl_class, MKDEV(DXB_CTRL_DEV_MAJOR, DXB_CTRL_DEV_MINOR));
	class_destroy(tcc_dxb_ctrl_class);
	unregister_chrdev(DXB_CTRL_DEV_MAJOR, DXB_CTRL_DEV_NAME);

	if (gDxbCtrl)
		kfree(gDxbCtrl);
	gDxbCtrl = NULL;

	return 0;
}


/*****************************************************************************
 * TCC_DXB_CTRL Module Init/Exit
 ******************************************************************************/
#ifdef CONFIG_OF
static const struct of_device_id tcc_dxb_ctrl_of_match[] = {
	{.compatible = "telechips,tcc_dxb_ctrl", },
	{ },
};
MODULE_DEVICE_TABLE(of, tcc_dxb_ctrl_of_match);
#endif

static struct platform_driver tcc_dxb_ctrl = {
	.probe	= tcc_dxb_ctrl_probe,
	.remove	= tcc_dxb_ctrl_remove,
	.driver	= {
		.name	= DXB_CTRL_DEV_NAME,
		.owner	= THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = tcc_dxb_ctrl_of_match,
#endif
	},
};

static int __init tcc_dxb_ctrl_init(void)
{
	return platform_driver_register(&tcc_dxb_ctrl);
}

static void __exit tcc_dxb_ctrl_exit(void)
{
	platform_driver_unregister(&tcc_dxb_ctrl);
}

module_init(tcc_dxb_ctrl_init);
module_exit(tcc_dxb_ctrl_exit);

MODULE_AUTHOR("Telechips Inc.");
MODULE_DESCRIPTION("TCC Broadcasting Control(Power, Reset..)");
MODULE_LICENSE("GPL");
