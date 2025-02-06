/*
 * drivers/watchdog/tcc_wdt.c
 *
 * Author:  <leesh@telechips.com>
 * Created: June, 2011
 * Description: to use the watchdog for Telechips chipset
 *
 * Copyright (C) Telechips, Inc.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * You should have received a copy of the  GNU General Public License along
 * with this program; if not, write  to the Free Software Foundation, Inc.,
 * 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*===========================================================================

                         INCLUDE FILES FOR MODULE

===========================================================================*/

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/miscdevice.h>
#include <linux/watchdog.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/irq.h>

#include <linux/of.h>
#include <linux/of_address.h>

#include <soc/tcc/timer.h>

//#define KERNEL_FILE_INTERFACE_USED

static DEFINE_SPINLOCK(wdt_lock);

#define wdt_readl		__raw_readl
#define wdt_writel		__raw_writel

#define WDT_EN			(1<<31)
//#define WDT_CLR			(1<<30)
#define WDT_EN_STS		(1<<24)
#define WDT_CLR_STS		(1<<23)
#define WDT_CNT_MASK		(0xFFFF)

#define TWDCFG			0x00
#define TWDCLR			0x04
#define TWDCNT			0x08

#define TWDCFG_EN		(1<<0)
#define TWDCFG_IEN		(1<<3)
#define TWDCFG_TCLKSEL_SHIFT	4
#define TWDCFG_TCLKSEL_MASK	(7<<4)
#define TWDCFG_TCLKSET_4	32
#define TWDCFG_TCLKSET_5	1024
#define TWDCFG_TCLKSET_6	4096

static void __iomem	*wdt_ctrl_base;
static void __iomem	*wdt_clear_reg;
#ifndef KERNEL_FILE_INTERFACE_USED
static void __iomem	*wdt_timer_base;
static void __iomem	*wdt_timer_irq_reg;
static u32		wdt_timer_irq_bit;
static u32		wdt_clear_bit;
static struct clk	*timer_clk;
static u32		timer_rate;
static unsigned long	twdcfg_value;
static struct tcc_timer wdt_timer_res;
#endif
static struct clk	*wdt_clk;
static u32		wdt_rate;
static int		watchdog_status;
static int		tccwdt_reset_time;
static int		tccwdt_kick_regval;


#define DBG(msg...) do { \
	if (1) \
		printk(KERN_INFO msg); \
	} while (0)

#ifdef CONFIG_DAUDIO_KK
#define TCCWDT_RESET_TIME	10 // sec unit
#else
#define TCCWDT_RESET_TIME	40 // sec unit
#endif
#define TCCWDT_KICK_TIME	1  // sec unit
#define TCCWDT_CNT_MASK_UNIT	(wdt_rate/65536)
#ifdef KERNEL_FILE_INTERFACE_USED
#define TCCWDT_KICK_TIME_DIV	4
#endif

static void tccwdt_start(void);
static void tccwdt_stop(void);

#ifdef CONFIG_DAUDIO_KK
static int tcc_wdt_status=0;
static ssize_t tcc_wdt_status_show(struct kobject *kobj, struct kobj_attribute *attr,
			char *buf)
{
	int ret = 0;
	if (wdt_timer_res.dev == NULL)
	{
		printk("func=%s, tcc_wdt is not initialized\n", __func__);
		return ret;
	}
	ret = sprintf(buf, "%d", tcc_wdt_status);
	printk("func=%s, tcc_wdt_status value=%d\n", __func__, tcc_wdt_status);
	return ret;
}

static ssize_t tcc_wdt_status_store(struct kobject *kobj, struct kobj_attribute *attr,
			 const char *buf, size_t count)
{
	int value = 0;
	if (wdt_timer_res.dev == NULL)
	{
		printk("func=%s, tcc_wdt is not initialized\n", __func__);
		return count;
	}
	sscanf(buf, "%d", &value);
	value = !!value;
	if (value == 1 && tcc_wdt_status == 0)
	{
		tccwdt_stop();
		tccwdt_start();
		printk("func=%s, tcc_wdt_status value=%d\n", __func__, tcc_wdt_status);
	}
	else if (value == 0&& tcc_wdt_status == 1)
	{
		tccwdt_stop();
		printk("func=%s, tcc_wdt_status value=%d\n", __func__, tcc_wdt_status);
	}
	return count;
}

static struct kobj_attribute tcc_wdt_status_attribute =
	__ATTR(tcc_wdt_status, 0644, tcc_wdt_status_show, tcc_wdt_status_store);


static struct attribute *tcc_wdt_fs_attrs[] = {
	&tcc_wdt_status_attribute.attr,
	NULL
};

static struct attribute_group tcc_wdt_fs_attr_group = {
	.attrs = tcc_wdt_fs_attrs
};
#endif

static void tccwdt_start(void)
{
	volatile unsigned int wtd_kick_pulse;
	volatile unsigned int wtd_kick_time;
#ifndef KERNEL_FILE_INTERFACE_USED
	volatile unsigned int twd_cfg_value;
#endif
	spin_lock(&wdt_lock);

	wdt_writel(wdt_readl(wdt_ctrl_base)&(~WDT_EN), wdt_ctrl_base);	/* disable watchdog */
	wdt_writel((wdt_readl(wdt_ctrl_base)&(~WDT_CNT_MASK))|tccwdt_reset_time*TCCWDT_CNT_MASK_UNIT,
		wdt_ctrl_base);
	wdt_writel(wdt_readl(wdt_ctrl_base)|WDT_EN, wdt_ctrl_base);	/* enable watchdog */

#ifdef KERNEL_FILE_INTERFACE_USED
	wtd_kick_time = tccwdt_reset_time/TCCWDT_KICK_TIME_DIV
#else
	if ((TCCWDT_KICK_TIME > 22) || (TCCWDT_KICK_TIME <= 0)) {
		BUG(); // Maximum kick time: 22 sec
	} else if ((TCCWDT_KICK_TIME > 5) && (TCCWDT_KICK_TIME <= 22)) {
		twdcfg_value = 6; 
		wtd_kick_pulse = timer_rate/TWDCFG_TCLKSET_6;
	} else {
		twdcfg_value = 5; 
		wtd_kick_pulse = timer_rate/TWDCFG_TCLKSET_5;
	}

	tccwdt_kick_regval = 65536 - (TCCWDT_KICK_TIME * wtd_kick_pulse);
	twd_cfg_value = wdt_readl(wdt_timer_base+TWDCFG)&(~TWDCFG_TCLKSEL_MASK);
	wdt_writel(twd_cfg_value|(twdcfg_value<<TWDCFG_TCLKSEL_SHIFT)|(TWDCFG_EN|TWDCFG_IEN),
		wdt_timer_base+TWDCFG);
	wdt_writel(tccwdt_kick_regval, wdt_timer_base+TWDCNT);
#endif
	DBG("%s : watchdog_time=%dsec, kick_time=%dsec\n", __func__,
		tccwdt_reset_time, wtd_kick_time);

#ifdef CONFIG_DAUDIO_KK
	tcc_wdt_status= 1;
#endif
	spin_unlock(&wdt_lock);
}

static void tccwdt_stop(void)
{
	spin_lock(&wdt_lock);

#ifndef KERNEL_FILE_INTERFACE_USED
	wdt_writel(tccwdt_kick_regval, wdt_timer_base+TWDCNT);
	wdt_writel(wdt_readl(wdt_timer_base+TWDCFG)&(~(TWDCFG_EN|TWDCFG_IEN)),
		wdt_timer_base+TWDCFG);
#endif
	wdt_writel(wdt_readl(wdt_ctrl_base)&(~WDT_EN), wdt_ctrl_base);	/* disable watchdog */
	DBG("%s\n", __func__);

#ifdef CONFIG_DAUDIO_KK
	tcc_wdt_status= 0;
#endif
	spin_unlock(&wdt_lock);
}

static int tccwdt_get_status(void)
{
	int stat = 0;

	DBG("%s\n", __func__);

	spin_lock(&wdt_lock);
	stat = wdt_readl(wdt_ctrl_base)&WDT_EN ? 1 : 0;
	spin_unlock(&wdt_lock);

	return stat;
}

static void tccwdt_kickdog(void)
{
	//spin_lock(&wdt_lock);
	wdt_writel(wdt_readl(wdt_clear_reg)|(1<<wdt_clear_bit), wdt_clear_reg);
	wdt_writel(wdt_readl(wdt_clear_reg)&(~(1<<wdt_clear_bit)), wdt_clear_reg);
	//spin_unlock(&wdt_lock);
}


#ifdef KERNEL_FILE_INTERFACE_USED
static int tccwdt_open(struct inode *inode, struct file *file)
{
	tccwdt_start();
	return nonseekable_open(inode, file);
}

static int tccwdt_release(struct inode *inode, struct file *file)
{
	tccwdt_stop();
	return 0;
}

static ssize_t tccwdt_write(struct file *file, const char __user *data,
				size_t len, loff_t *ppos)
{
	tccwdt_kickdog();
	return len;
}

static long tccwdt_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	int __user *p = argp;

	switch (cmd) {
	case WDIOC_KEEPALIVE:
		tccwdt_kickdog();
		return 0;
	case WDIOC_SETTIMEOUT:
		if (get_user(tccwdt_reset_time, p))
			return -EFAULT;
		tccwdt_stop();
		tccwdt_start();
		return put_user(tccwdt_reset_time, p);
	case WDIOC_GETTIMEOUT:
		return put_user(tccwdt_reset_time, p);
	default:
		return -ENOTTY;
	}
}

static const struct file_operations tccwdt_fops = {
	.owner		= THIS_MODULE,
	.llseek		= no_llseek,
	.write		= tccwdt_write,
	.unlocked_ioctl	= tccwdt_ioctl,
	.open		= tccwdt_open,
	.release	= tccwdt_release,
};

static struct miscdevice tccwdt_miscdev = {
	.minor		= WATCHDOG_MINOR,
	.name		= "watchdog",
	.fops		= &tccwdt_fops,
};
#else
static irqreturn_t tccwdt_timer_handler(int irq, void *dev_id)
{
	/* Process only watchdog interrupt source. */
	if (wdt_readl(wdt_timer_irq_reg) &  (1 << wdt_timer_irq_bit)) {
//		DBG("%s\n", __func__);
		wdt_writel((1<<(8+wdt_timer_irq_bit))|(1<<wdt_timer_irq_bit), wdt_timer_irq_reg);
		tccwdt_kickdog();
		wdt_writel(tccwdt_kick_regval, wdt_timer_base+TWDCNT);
		return IRQ_HANDLED;
	}
	return IRQ_NONE;
}
#endif

static int tccwdt_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	unsigned int irq = platform_get_irq(pdev, 0);
	int ret;

	tccwdt_reset_time = TCCWDT_RESET_TIME;

	wdt_ctrl_base = of_iomap(np, 0);
	if (of_property_read_u32(np, "clock-frequency", &wdt_rate)) {
		printk("%s: Can't read clock-frequency\n", __func__);
		wdt_rate = 12000000;
	}
	wdt_clk = of_clk_get(np, 0);
	if (IS_ERR(wdt_clk)) {
		pr_warn("Unable to get wdt clock.\n");
		BUG();
	} else
		clk_set_rate(wdt_clk, wdt_rate);

	clk_prepare_enable(wdt_clk);

	wdt_clear_reg = of_iomap(np,1);

	if (of_property_read_u32(np, "clear-bit", &wdt_clear_bit)) {
		pr_warn("%s: Can't read watchdog interrupt offset\n", __func__);
		wdt_clear_bit = 0;
	}

#ifdef KERNEL_FILE_INTERFACE_USED
	ret = misc_register(&tccwdt_miscdev);
	if (ret) {
		printk("cannot register miscdev on minor=%d (%d)\n", WATCHDOG_MINOR, ret);
		return -1;
	}
#else
	wdt_timer_base = of_iomap(np, 2);
	timer_clk = of_clk_get(np, 1);
	if (IS_ERR(timer_clk)) {
		pr_warn("Unable to get timer clock.\n");
		BUG();
	}
	timer_rate = clk_get_rate(timer_clk);
	wdt_timer_irq_reg = of_iomap(np, 3);
	if (of_property_read_u32(np, "int-offset", &wdt_timer_irq_bit)) {
		pr_warn("%s: Can't read watchdog interrupt offset\n", __func__);
		BUG();
	}

#ifdef CONFIG_DAUDIO_KK
	ret = sysfs_create_group(&pdev->dev.kobj, &tcc_wdt_fs_attr_group);
	if (ret) {
		pr_err("unable to register tcc_wdt sysfs nodes\n");
	}
#endif

	wdt_timer_res.id = wdt_timer_irq_bit;
	wdt_timer_res.dev = &pdev->dev;
	ret = request_irq(irq, tccwdt_timer_handler, IRQF_SHARED, "TCC-WDT", &wdt_timer_res);
	tccwdt_start();
#endif

	return 0;
}

static int tccwdt_remove(struct platform_device *dev)
{
	DBG("%s: remove=%p\n", __func__, dev);
	tccwdt_stop();
	return 0;
}

static void tccwdt_shutdown(struct platform_device *dev)
{
	DBG("%s: shutdown=%p\n", __func__, dev);
	tccwdt_stop();
}

#ifdef CONFIG_PM
static int tccwdt_suspend(struct device *dev)
{
	DBG("%s: suspend=%p\n", __func__, dev);
	watchdog_status = tccwdt_get_status();
	tccwdt_stop();
	return 0;
}

static int tccwdt_resume(struct device *dev)
{
	DBG("%s: resume=%p\n", __func__, dev);
	if(watchdog_status)
		tccwdt_start();
	return 0;
}
#else
#define tccwdt_suspend NULL
#define tccwdt_resume  NULL
#endif

static const struct of_device_id tcc_wdt_of_match[] = {
	{.compatible = "telechips,wdt", },
	{ },
};
MODULE_DEVICE_TABLE(of, tcc_wdt_of_match);



//+[TCCQB] For QuickBoot WatchDog
#ifdef CONFIG_PM
static int tccwdt_freeze(struct device *dev)
{
#ifdef CONFIG_QUICKBOOT_WATCHDOG
	tccwdt_kickdog();
#else
	DBG("%s: suspend=%p\n", __func__, dev);
	watchdog_status = tccwdt_get_status();
	tccwdt_stop();
#endif

	return 0;
}

void tccwdt_qb_reset_time(int sec);
static int tccwdt_restore(struct device *dev)
{
#ifdef CONFIG_QUICKBOOT_WATCHDOG
	tccwdt_kickdog();
	tccwdt_qb_reset_time(0);	// Default QB WatchDog Reset Time
#else
	DBG("%s: resume=%p\n", __func__, dev);
	if(watchdog_status)
		tccwdt_start();
#endif

	return 0;
}
static const struct dev_pm_ops tcc_wdt_pm_ops = {
//	.runtime_suspend      = tcc_wdt_runtime_suspend,
//	.runtime_resume       = tcc_wdt_runtime_resume,
	.suspend    = tccwdt_suspend,
	.resume 	= tccwdt_resume,
	.freeze 	= tccwdt_freeze,
	.thaw 		= tccwdt_restore,
	.restore 	= tccwdt_restore,
};
#endif
//-[TCCQB]
//


static struct platform_driver tccwdt_driver = {
	.probe		= tccwdt_probe,
	.remove		= tccwdt_remove,
	.shutdown	= tccwdt_shutdown,
#ifndef CONFIG_PM
	.suspend	= tccwdt_suspend,
	.resume		= tccwdt_resume,
#endif
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "tcc-wdt",
		.pm 	= &tcc_wdt_pm_ops,
		.of_match_table = tcc_wdt_of_match,
	},
};

static int __init watchdog_init(void)
{
	return platform_driver_register(&tccwdt_driver);
}

static void __exit watchdog_exit(void)
{
	tccwdt_stop();
	platform_driver_unregister(&tccwdt_driver);
}

//+[TCCQB] For QuickBoot WatchDog
/*===========================================================================

                             QuickBoot Watchdog

===========================================================================*/
#ifdef CONFIG_QUICKBOOT_WATCHDOG

#ifdef CONFIG_DAUDIO_KK
#define TCCWDT_RESET_TIME_QB       10 //5 -> 10 // sec unit
#else
#define TCCWDT_RESET_TIME_QB       20 // sec unit
#endif
void tccwdt_qb_init(void)
{
	if (wdt_ctrl_base == NULL) {
		printk("\x1b[1;31mWatchDog is not enabled. Check dtsi file. - %s\x1b[0m\n", __func__);
		return;
	}

	watchdog_status = tccwdt_get_status();
	tccwdt_kickdog();
	tccwdt_stop();	//	To Disable TCC WatchDog

	tccwdt_reset_time = TCCWDT_RESET_TIME_QB;	// SET QuickBoot WatchDog Time.

	wdt_writel(wdt_readl(wdt_ctrl_base)&(~WDT_EN), wdt_ctrl_base);	/* disable watchdog */
	wdt_writel((wdt_readl(wdt_ctrl_base)&(~WDT_CNT_MASK))|tccwdt_reset_time*TCCWDT_CNT_MASK_UNIT, wdt_ctrl_base);
	wdt_writel(wdt_readl(wdt_ctrl_base)|WDT_EN, wdt_ctrl_base);	/* enable watchdog */

	printk(KERN_INFO "Enable QuickBoot WatchDog Reset Time : %d sec\n", tccwdt_reset_time);

	tccwdt_kickdog();	// Kick QuickBoot WatchDog
}
void tccwdt_qb_exit(void)
{ 
	if (wdt_ctrl_base == NULL) {
		printk("\x1b[1;31mWatchDog is not enabled. Check dtsi file. - %s\x1b[0m\n", __func__);
		return;
	}
	
	printk(KERN_INFO "*** Disable QuickBoot WatchDog ***\n");
	tccwdt_stop();	// Disable QuickBoot WatchDog

	tccwdt_reset_time = TCCWDT_RESET_TIME;	// SET TCC WatchDog Time.

	if(watchdog_status) {
		tccwdt_start();	// Enable TCC WatchDog
		printk(KERN_INFO "Enable TCC WatchDog Reset Time : %d sec\n", tccwdt_reset_time);
	}
}
void tccwdt_qb_kickdog(void) 
{
	if (wdt_ctrl_base == NULL) {
//		printk("\x1b[1;31mWatchDog is not initialized!. QB WatchDog cannot work! - %s\x1b[0m\n", __func__);
		return;
	}
	
	tccwdt_kickdog();
}
void tccwdt_qb_reset_time(int wdt_sec)
{
	if (wdt_ctrl_base == NULL) {
		printk("\x1b[1;31mWatchDog is not enabled. Check dtsi file. - %s\x1b[0m\n", __func__);
		return;
	}
	
	tccwdt_stop();

	/*	SET Default TCCWDT_RESET_TIME_QB	*/
	if (wdt_sec == 0)
		wdt_sec = TCCWDT_RESET_TIME_QB;

	/*	SET New Wdt Reset Time	*/

	wdt_writel(wdt_readl(wdt_ctrl_base)&(~WDT_EN), wdt_ctrl_base);	/* disable watchdog */
	wdt_writel((wdt_readl(wdt_ctrl_base)&(~WDT_CNT_MASK))|wdt_sec*TCCWDT_CNT_MASK_UNIT, wdt_ctrl_base);
	wdt_writel(wdt_readl(wdt_ctrl_base)|WDT_EN, wdt_ctrl_base);	/* enable watchdog */

	printk(KERN_INFO "QuickBoot WatchDog Reset Time : %d sec\n", wdt_sec);

	tccwdt_kickdog();
}
EXPORT_SYMBOL(tccwdt_qb_init);
EXPORT_SYMBOL(tccwdt_qb_exit);
EXPORT_SYMBOL(tccwdt_qb_kickdog);
EXPORT_SYMBOL(tccwdt_qb_reset_time);
#endif
//-[TCCQB] 
//

module_init(watchdog_init);
module_exit(watchdog_exit);

MODULE_AUTHOR("Bruce <leesh@telechips.com>");
MODULE_DESCRIPTION("TCC Watchdog Device Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS_MISCDEV(WATCHDOG_MINOR);
MODULE_ALIAS("platform:tcc-wdt");

