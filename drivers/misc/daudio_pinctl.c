#ifdef CONFIG_DAUDIO_PIN_CONTROL

#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/major.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#include <mach/daudio.h>
#include <mach/daudio_debug.h>
#include <mach/daudio_info.h>
#include <mach/daudio_pinctl.h>
#include <mach/gpio.h>
#include <mach/hardware.h>

#include <mach/irqs.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>

#include <mach/structures_smu_pmu.h>
#include <mach/bsp.h>
#include <asm/uaccess.h>

#define VPRINTK(fmt, args...) if (debug_pinctl) printk(KERN_INFO TAG_DAUDIO_PINCTL fmt, ##args)

#define DAUDIO_PINCTL_MAJOR     229 // TODO : need device major number header file
#define MAX_STRING_LENGTH	    255
#define PINCTL_FAIL				-1

#ifdef CONFIG_DAUDIO_KK
#define GPIO_OEM_TYPE TCC_GPB(24)
#define GPIO_MON_TYPE TCC_GPB(13)
#define GPIO_FACTORY_CHECK TCC_GPB(8)
#define GPIO_LVDS_VIDEO_SIGNAL_CHECK TCC_GPE(17)
#define GPIO_LVDS_VARIANT_CHECK TCC_GPB(19)
#define GPIO_DIGITAL_VARIANT_CHECK TCC_GPB(22)
#define GPIO_GET_CAMERA_VARIANT TCC_GPB(23)
#elif defined(CONFIG_WIDE_PE_COMMON)
#define GPIO_OEM_TYPE TCC_GPB(16)
#define GPIO_MON_TYPE TCC_GPC(14)
static unsigned gpio_oem = 0; /* TCC_GPB(16) */
static unsigned gpio_mon = 0; /* TCC_GPC(14) */
static unsigned gpio_factory = 0; /* TCC_GPB(13) */
static unsigned gpio_lvds_video_signal_check = 0; /* TCC_GPG(6) */
static unsigned gpio_lvds_variant_check = 0; /* TCC_GPE(6) */
static unsigned gpio_digital_variant_check = 0; /*TCC_GPE(14) */
static unsigned gpio_get_camera_variant_check = 0; /*TCC_GPE(15) */
static unsigned gpio_get_cabin_camera_use = 0; /*TCC_GPSD1(1) */
static unsigned gpio_get_cabin_camera_type = 0; /*TCC_GPSD1(2) */
static int camera_type = 0;
static int cabin_camera_type = 0;
#endif // CONFIG_WIDE_PE_COMMON


static DEFINE_MUTEX(daudio_pinctl_lock);
static struct class *daudio_pinctl_class;

static DEFINE_SPINLOCK(ext_irq_lock);
static DEFINE_MUTEX(enable_interrupt_mutex);

static int debug_pinctl = DEBUG_DAUDIO_PINCTL;
static int gpio_list_size = 0;

#if 0	//5b42c46e8a0f13f5b7c1036de9856747fe36943d
#if defined(INCLUDE_TDMB)
static int ext_irq_noti_1 = 0;  //spidev1.0
#endif
#endif

static int ext_irq_noti = 0;  //spidev2.0
//static int ext_irq_status = 0;

static D_AUDIO_GPIO_LIST *gpio_list_p;

/*
 * @author	yongki.kim@mobis.co.kr
 * @desc	structure for wrapping to fill the information about enable/disable external interrupt
 * @date	2015-08-20
 */
struct st_ext_int_info {
	unsigned int gpio;
	unsigned int extint;
	unsigned int hw_INT_bit;
	unsigned int source;
	const char label[MAX_STRING_LENGTH];
	unsigned int fn_num;
	unsigned long flags;
	/*
		EXAMPLE - CMMB IRQ

		gpio			TCC_GPE(16)				: input, gpio port number (gpio.h)
		extint			INT_EINT11				: input, irq numbers for interrupt handler (irq.h)
		hw_INT_bit		Hw14					: Bit index, for SET IEN0 Register
		source			EXTINT_GPIOE_16			: Check - CPU_FULL_SPEC document
		label			"cmmb_uevent"			: optional(Can be NULL)
		fn_num			GPIO_FN(0)				: Check - CPU_FULL_SPEC document
		flags			IRQF_TRIGGER_FALLING	: RISING/FALLING
	*/
};

static D_AUDIO_GPIO_LIST gpio_list[] = {
	{DET_SDCARD_WP,             TCC_GPA(13),    "DET_SDCARD_WP"},
	{CTL_SDCARD_PWR_EN,         TCC_GPA(14),    "CTL_SDCARD_PWR_EN"},
	{DET_SDCARD_CD,             TCC_GPA(15),    "DET_SDCARD_CD"},
	{CTL_ANT_SENSE_SEL,		TCC_GPB(0),	"CTL_ANT_SENSE_SEL"},
	{DET_ANT_POWER_ERR,         TCC_GPB(1),     "DET_ANT_POWER_ERR"},
	{CTL_DMB_ANT_PWRON,         TCC_GPB(2),     "CTL_DMB_ANT_PWRON"},
	{CTL_DAB_BOOT_MODE,		TCC_GPB(3),	"CTL_DAB_BOOT_MODE"},   //CPU_SATURN_BOOTSTRAP0
	{CTL_GPS_ANT_PWR_ON,        TCC_GPB(5),     "CTL_GPS_ANT_PWR_ON"},
	{CTL_XM_SIRIUS_RESET,       TCC_GPB(6),     "CTL_XM_SIRIUS_RESET"},
	{DET_9277_LOCK,		TCC_GPB(9),     "DTT_9277_LOCK"},
	{CTL_XM_SHDN,               TCC_GPB(10),    "CTL_XM_SHDN"},
	{DET_WL_HOST_WAKE_UP,       TCC_GPB(12),    "DET_WL_HOST_WAKE_UP"},
	{CTL_WL_REG_ON,             TCC_GPB(14),    "CTL_WL_REG_ON"},
	{DET_M105_CPU_4_5V,         TCC_GPB(15),    "DET_M105_CPU_4.5V"},
	{CTL_DRM_RESET,             TCC_GPB(16),    "CTL_DRM_RESET"},
	{CTL_BT_RESET,              TCC_GPB(20),    "CTL_BT_RESET"},
	{CTL_SERDES_LINE_FALUT,     TCC_GPB(21),    "CTL_SERDES_LINE_FALUT"},
	{CTL_96751_PWDNB,           TCC_GPB(29),    "CTL_96751_PWDNB"},
	{DET_CDMA_BOOT_OK_OUT,      TCC_GPC(13),    "DET_CDMA_BOOT_OK_OUT"},
	{ENABLE_SATURN_SPI_INT,       TCC_GPC(21),    "ENABLE_TPEG_SPI_INT"},//M136_SATURN_SPI_INT
	{CTL_GPS_RESET_N,           TCC_GPC(20),    "CTL_GPS_RESET_N"},
	{ENABLE_DAB_SPI_INT,	TCC_GPC(15),	"ENABLE_DAB(DMB)_SPI_INT"}, //M144_SATURN_DMB/HD_SPI_INT
	{CTL_CPU_BOOT_OK,           TCC_GPC(29),    "CTL_CPU_BOOT_OK"},
	{DET_XM_F_ACT,              TCC_GPE(6),     "DET_XM_F_ACT"},
	{DET_REVERSE_CPU,		TCC_GPE(8),	"DET_M111_REVERSE_CPU"},
	{CTL_CAS_RESET,		TCC_GPE(10),	"CTL_CAS_RESET"},
	{CTL_FM1688_RESET,		TCC_GPE(15),	"CTL_FM1688_RESET"},
	{CTL_FM1688_SWITCH,      TCC_GPE(31),    "CTL_FM1688_SWITCH"},
	{CTL_TOUCH_RESET,		TCC_GPF(18),	"CTL_TOUCH_RESET"}, //TOUCH_POR
	{DET_I2C_TOUCH_INT,         TCC_GPF(21),    "DET_I2C_TOUCH_INT"},
	{DET_M36_SD_BOOT_MODE_F24,  TCC_GPF(24),    "DET_M36_SD_BOOT_MODE_F24"},
	{DET_M36_SD_BOOT_MODE_F25,  TCC_GPF(25),    "DET_M36_SD_BOOT_MODE_F25"},
	{CTL_DAB_RESET,		TCC_GPG(0),	"CTL_DAB_RESET"},       //CPU_SATURN_RESET
	{DET_TOUCH_JIG,             TCC_GPG(4),     "DET_TOUCH_JIG"},
	{CTL_M95_MICOM_PD10,        TCC_GPG(5),     "CTL_M95_MICOM_PD10"},
	{CTL_I2C_CHANGE,            TCC_GPA(3),     "CTL_I2C_CHANGE"},
	{CTL_TW9990_RESET,          TCC_GPG(16),    "CTL_TW9990_RESET"},

	{CTL_GPS_BOOT_MODE,		TCC_GPA(0),	"CTL_GPS_BOOT_MODE - RESERVED"},
#if defined(INCLUDE_TDMB)
	{DET_GPS_ANT_SHORT,		TCC_GPA(0),	"DET_GPS(+DMB)_ANT_SHORT - ADC Check"},
	{DET_GPS_ANT_OPEN,		TCC_GPA(0),	"DET_GPS(+DMB)_ANT_OPEN - ADC Check"},
#else
	{DET_GPS_ANT_SHORT,		TCC_GPA(0),	"DET_GPS_ANT_SHORT - ADC Check"},
	{DET_GPS_ANT_OPEN,		TCC_GPA(0),	"DET_GPS_ANT_OPEN - ADC Check"},
#endif
	{DET_DMB_ANT_SHORT,		TCC_GPA(0),	"DET_DMB_ANT_SHORT - ADC Check"},
	{DET_DMB_ANT_OPEN,		TCC_GPA(0),	"DET_DMB_ANT_OPEN - ADC Check"},
	{DET_GPS_RTC,		TCC_GPA(0),	"DET-GPS_RTC - ADC Check"},
#ifdef CONFIG_SNAPSHOT_BOOT
	{DET_LOG_LEVEL,		TCC_GPC(22),	"DET-LOG - LEVEL Check"}, //D-Audio 1.1 loglevel
#endif
	{DET_SERDES_CONN,		TCC_GPA(0),	"DET_SERDES_CONN Check"},  //serdes conn check 20180622 mhjung
	{DET_SEPERATED_MON, 	TCC_GPA(0),	"DET_SEPERATED_MON Check"},

};

#define GPIO_BASE_ADDRESS		0x74200000
#define GPIO_REGS_STEP			0x40
#define GPIO_OUTPUT_ENABLE_REG	0x04
#define GPIO_OUTPUT_ENABLE		0x01
#define GPIO_REG(reg)			IO_ADDRESS(reg)

static int sizeof_list(void)
{
	/* you need check target board version ?
	 * 1. check gpio & get target board version
	 * 2. select the version appropriate structure */
	int ret = sizeof(gpio_list) / sizeof(D_AUDIO_GPIO_LIST);
	return ret;
}

static int init_list(void)
{
	int ret = 0;
	/* TODO : check target board version ? */
	gpio_list_size = sizeof_list();

	if(gpio_list_size > 0) {
		gpio_list_p = gpio_list;

		if(gpio_list_p != 0) {
			ret = 1;
		}
	}
	else {
		ret = 1;
	}

	return ret;
}

static int atoi(const char *name)
{
	int val = 0;
	for (;; name++) {
		switch (*name) {
			case '0'...'9':
				val = 10 * val + (*name - '0');
				break;
			default:
				return val;
		}
	}
	return val;
}

static void get_gpio_group_name(unsigned gpio, char *name)
{
	char group_name[11] = {'T', 'C', 'C', '_', 'G', 'P', ' ', ' ', ' ', ' ', };
	int group = gpio / 32;
	unsigned gpio_group_g = TCC_GPG(19) / 32;
	unsigned gpio_group_hdmi = TCC_GPHDMI(4) / 32;
	unsigned gpio_group_adc = TCC_GPADC(5) / 32;
	unsigned gpio_group_sd = TCC_GPSD(10) / 32;

	if (group >= 0 && group <= gpio_group_g) {
		group_name[6] = 'A' + group;
		group_name[10] = '\0';
	}
	else if (group == gpio_group_hdmi) {
		group_name[6] = 'H';
		group_name[7] = 'D';
		group_name[9] = 'M';
		group_name[9] = 'I';
		group_name[10] = '\0';
	}
	else if (group == gpio_group_adc) {
		group_name[6] = 'A';
		group_name[7] = 'D';
		group_name[8] = 'C';
		group_name[10] = '\0';
	}
	else if (group == gpio_group_sd) {
		group_name[6] = 'S';
		group_name[7] = 'D';
		group_name[10] = '\0';
	}

	if (name != NULL)
		memcpy(name, group_name, 11);
	else {
		printk(KERN_ERR "%s name is NULL\n", __func__);
	}
}

//=============================================================
//CPU_VARIANT_OE/PIO
int get_oemtype(void)
{
        int oem = 0;
#ifdef CONFIG_DAUDIO_KK
        oem = gpio_get_value(GPIO_OEM_TYPE);
#elif defined(CONFIG_WIDE_PE_COMMON)
        oem = gpio_get_value(gpio_oem);
#else
        printk(KERN_INFO "[ERROR] func=%s, line=%d\n", __func__, __LINE__);
        oem = -1;
#endif
        return oem;
}
EXPORT_SYMBOL(get_oemtype);
//=============================================================
int get_montype(void)
{
        int mon = 0;
#ifdef CONFIG_DAUDIO_KK
        mon = gpio_get_value(GPIO_MON_TYPE);
#elif defined(CONFIG_WIDE_PE_COMMON)
        mon = gpio_get_value(gpio_mon);
#else
        printk(KERN_INFO "[ERROR] func=%s, line=%d\n", __func__, __LINE__);
        mon = -1;
#endif
        return mon;
}
EXPORT_SYMBOL(get_montype);
//=============================================================
int get_factorycheck(void)
{
        int factory = 0;
#ifdef CONFIG_DAUDIO_KK
        factory = gpio_get_value(GPIO_FACTORY_CHECK);
#elif defined(CONFIG_WIDE_PE_COMMON)
        factory = gpio_get_value(gpio_factory);
#else
        printk(KERN_INFO "[ERROR] func=%s, line=%d\n", __func__, __LINE__);
        factory = -1;
#endif
        return factory;
}
EXPORT_SYMBOL(get_factorycheck);

/**
 * @return 0 is input, 1 is output
 **/
static int get_gpio_output_enable(unsigned gpio)
{
	int group = gpio / 32;
	int number = gpio % 32;
	int *output_enable = (int *)IO_ADDRESS(GPIO_BASE_ADDRESS +
										   (group * GPIO_REGS_STEP) + GPIO_OUTPUT_ENABLE_REG);
	int ret_val = (*output_enable >> number) & GPIO_OUTPUT_ENABLE;
	return ret_val;
}

int get_gpio_number(unsigned gpio)
{
	int i;
	if (gpio >= 0) {
		for (i = 0; i < gpio_list_size; i++) {
			if (gpio_list_p[i].gpio_name == gpio)
				return gpio_list_p[i].gpio_number;
		}
	}
	return PINCTL_FAIL;
}
#ifdef CONFIG_TOUCHSCREEN_ATMEL_MXT_336S_AT_MODULE
EXPORT_SYMBOL(get_gpio_number);
#endif

#if defined(INCLUDE_XM)
#define GPIO_UART4_TX TCC_GPF(13)
#define GPIO_UART4_RX TCC_GPF(14)
#define GPIO_I2C_SDA	TCC_GPG(6)
#define GPIO_I2C_SCL	TCC_GPG(7)

void sabre_i2c_pinctl(bool value)
{
	if (value) {
		tcc_gpio_config(GPIO_I2C_SDA, GPIO_FN(0) | GPIO_INPUT | GPIO_PULL_DISABLE);
		tcc_gpio_config(GPIO_I2C_SCL, GPIO_FN(0) | GPIO_INPUT | GPIO_PULL_DISABLE);
		printk("Set SABRE pin as GPIO\n");
	}
	else {
		tcc_gpio_config(GPIO_I2C_SDA, GPIO_FN(4));
		tcc_gpio_config(GPIO_I2C_SCL, GPIO_FN(4));
		printk("Set SABRE pin as I2C\n");
	}

}


void daudio_uart_pinctl(bool value)
{
	if (value) {
		printk(KERN_INFO "Set UART4 as UART\n");
		tcc_gpio_config(GPIO_UART4_TX, GPIO_FN(9));
		tcc_gpio_config(GPIO_UART4_RX, GPIO_FN(9));
	}
	else {
		printk(KERN_INFO "Set UART4 as GPIO\n");
		tcc_gpio_config(GPIO_UART4_TX, GPIO_FN(0) | GPIO_OUTPUT | GPIO_LOW | GPIO_PULL_DISABLE);
		tcc_gpio_config(GPIO_UART4_RX, GPIO_FN(0) | GPIO_OUTPUT | GPIO_LOW | GPIO_PULL_DISABLE);
	}
}
#endif

/*
 * @author	yongki.kim@mobis.co.kr
 * @func	ext_irq_handler, show_ext_spi_int, store_ext_spi_int
 * @desc	external interrupt handler(ext_irq_handler), file operations(show, store)
 *          You can add new external handler and file operations below.
 */
static irqreturn_t ext_irq_handler(int irq, void *dev_id)
{
	unsigned long flag;

	spin_lock_irqsave(&ext_irq_lock, flag);
	// kobject_uevent(&vfd->dev.kobj, KOBJ_CHANGE);

#if 0
	if(ext_irq_status == 0) {
		printk("%s irq No:%d ext_irq_status(%d)\n", __func__, irq, ext_irq_status);
		ext_irq_status++;
	}
	else {
		printk("%s irq No:%d SET ext_irq_noti=1 (status > 0)\n", __func__, irq);
		ext_irq_noti = 1;
	}
#else
//	printk("%s irq No:%d SET ext_irq_noti=1 (status > 0)\n", __func__, irq);
	ext_irq_noti = 1;
#endif

	spin_unlock_irqrestore(&ext_irq_lock, flag);

	return IRQ_HANDLED;
}

static ssize_t show_ext_spi_int(struct device *dev, struct device_attribute *attr, char *buf)
{
	int temp;
	size_t ret;
	unsigned long flag;

	spin_lock_irqsave(&ext_irq_lock, flag);
	temp = ext_irq_noti;
	ext_irq_noti = 0;
	spin_unlock_irqrestore(&ext_irq_lock, flag);

	ret = sprintf(buf, "%d", temp);

	return ret;
}

static ssize_t store_ext_spi_int(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
//    char i;
	int i;
	unsigned long flag;

	sscanf(buf, "%d", &i);

	spin_lock_irqsave(&ext_irq_lock, flag);
	ext_irq_noti = i;
	spin_unlock_irqrestore(&ext_irq_lock, flag);

	printk("ext_irq_noti_write %d\n", i);

	return count;
}

#if 0 //jason.ku 2018.10.30
#if defined(INCLUDE_TDMB)
static irqreturn_t ext_irq_handler_1(int irq, void *dev_id)
{
	unsigned long flag;

	spin_lock_irqsave(&ext_irq_lock, flag);
	ext_irq_noti_1 = 1;
	spin_unlock_irqrestore(&ext_irq_lock, flag);

	return IRQ_HANDLED;
}

static ssize_t show_ext_spi_int_1(struct device *dev, struct device_attribute *attr, char *buf)
{
	int temp;
	size_t ret;
	unsigned long flag;

	spin_lock_irqsave(&ext_irq_lock, flag);
	temp = ext_irq_noti;
	ext_irq_noti_1 = 0;
	spin_unlock_irqrestore(&ext_irq_lock, flag);

	ret = sprintf(buf, "%d", temp);

	return ret;
}


static ssize_t store_ext_spi_int_1(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	//    char i;
	int i;
	unsigned long flag;

	sscanf(buf, "%d", &i);

	spin_lock_irqsave(&ext_irq_lock, flag);
	ext_irq_noti_1 = i;
	spin_unlock_irqrestore(&ext_irq_lock, flag);

	printk("ext_irq_noti_1_write %d\n", i);

	return count;
}

static DEVICE_ATTR(ext_spi_int_1, 0444, show_ext_spi_int_1, store_ext_spi_int_1);
#endif
#endif //#if 0 //jason.ku 2018.10.30

void clear_irq_variables(void)
{
	unsigned long flag;

	spin_lock_irqsave(&ext_irq_lock, flag);
	ext_irq_noti = 0;
#if 0 //jason.ku 2018.10.30
#if defined(INCLUDE_TDMB)
	ext_irq_noti_1 = 0;
#endif
#endif	// #if 0 //jason.ku 2018.10.30
	spin_unlock_irqrestore(&ext_irq_lock, flag);
}

static DEVICE_ATTR(ext_spi_int, 0444, show_ext_spi_int, store_ext_spi_int);



static int daudio_pinctl_set_gpio(int gpio, int value)
{
	int ret = PINCTL_FAIL;

	if (gpio >= 0) {
		if (gpio == TCC_GPG(5)) {
			printk(KERN_ERR "%s() Ignoring CTL_M95_MICOM_PD10 control\n", __FUNCTION__);
			return ret;
		}
		if (get_gpio_output_enable(gpio)) {
#if defined(INCLUDE_XM)
			if ((gpio == TCC_GPA(3)) && ((value == 0) || (value == 1)))
				sabre_i2c_pinctl(value);

			if ((gpio == TCC_GPB(10)) && (value == 0))
				daudio_uart_pinctl(0);
#elif defined(INCLUDE_DAB)
#if 0
			if ((gpio == TCC_GPC(22)) && (value == 1)) {
				tcc_gpio_config(TCC_GPD(22), GPIO_FN0);
				tcc_gpio_config(TCC_GPD(23), GPIO_FN0);
				tcc_gpio_config(TCC_GPD(24), GPIO_FN0 | GPIO_INPUT);
				tcc_gpio_config(TCC_GPD(25), GPIO_FN0);
			}
			else if ((gpio == TCC_GPC(22)) && (value == 0)) {
				tcc_gpio_config(TCC_GPD(22), GPIO_FN15);
				tcc_gpio_config(TCC_GPD(23), GPIO_FN15);
				tcc_gpio_config(TCC_GPD(24), GPIO_FN15 | GPIO_OUTPUT | GPIO_LOW);
				tcc_gpio_config(TCC_GPD(25), GPIO_FN15);
			}
#endif
#endif
			gpio_set_value(gpio, value);
#if defined(INCLUDE_XM)
			if ((gpio == TCC_GPB(10)) && (value == 1))
				daudio_uart_pinctl(1);
#endif
			ret = 0;
		}
		else {
			//input setting
#if 0	//not support
			int pull_status = 0;
			char gpio_group[11] = {' ',};
			get_gpio_group_name(gpio, gpio_group);

			pull_status = value ? GPIO_PULLUP : GPIO_PULLDOWN;
			gpio_request(gpio, gpio_group);
			tcc_gpio_config(gpio, GPIO_FN0 | GPIO_INPUT | pull_status);
#endif
			printk(KERN_INFO "%s Not support input control.\n", __func__);
		}
	}

	return ret;
}

static int daudio_pinctl_get_gpio(int gpio)
{
	if (gpio >= 0)
		return gpio_get_value(gpio);
	else
		return PINCTL_FAIL;
}

static void print_usage(void)
{
	printk(KERN_INFO "echo <debug> > /dev/daudio_pinctl\n");
	printk(KERN_INFO "<debug> > 1:Enable, 0:Disable\n\n");
	printk(KERN_INFO "echo <gpio_num> <value> > /dev/daudio_pinctl\n");
	printk(KERN_INFO "<gpio_num> gpio index: [0 ~ %d]\n", gpio_list_size - 1);
	printk(KERN_INFO "<value> gpio value 1:High, 0:Low\n\n");
}

static ssize_t daudio_pinctl_read(struct file *file, char *buf, size_t count, loff_t *pos)
{
	int i;

	mutex_lock(&daudio_pinctl_lock);
	print_usage();
	printk(KERN_INFO "========== D-AUDIO GPIO FUNCTION DEBUG =========\n");
	printk(KERN_INFO "D-Audio PINCTL debug status : %s\n",
		   debug_pinctl ? "ENABLE" : "DISABLE");
	printk(KERN_INFO "========== D-AUDIO GPIO FUNCTION LIST ==========\n");
	printk(KERN_INFO "NUM            GPIO DIRECTION\tVALUE\tDESCRIPTION\n");
	for (i = 0; i < gpio_list_size; i++) {
		int output_enable = get_gpio_output_enable(gpio_list_p[i].gpio_number);
		int number = gpio_list_p[i].gpio_number % 32;
		char *group_name = 0;
		char *direction = output_enable ? "OUTPUT" : "INPUT ";
		char *value = daudio_pinctl_get_gpio(gpio_list_p[i].gpio_number) ?
					  "HIGH" : "LOW ";
#if defined(CONFIG_DAUDIO_ECO) || defined(CONFIG_DAUDIO_KK)
		if(gpio_list_p[i].gpio_name == DET_DMB_ANT_SHORT)
			value = get_dmb_diag_short() ? "HIGH" : "LOW";
		else if(gpio_list_p[i].gpio_name == DET_DMB_ANT_OPEN)
			value = get_dmb_diag_open() ? "HIGH" : "LOW";
		else if(gpio_list_p[i].gpio_name == DET_GPS_ANT_SHORT)
			value = get_gps_diag_short() ? "HIGH" : "LOW";
		else if(gpio_list_p[i].gpio_name == DET_GPS_ANT_OPEN)
			value = get_gps_diag_open() ? "HIGH" : "LOW";
		else if(gpio_list_p[i].gpio_name == DET_SERDES_CONN) //serdes conn check 20180622 mhjung
			value = get_serdes_conn() ? "HIGH" : "LOW";
		else if(gpio_list_p[i].gpio_name == DET_SEPERATED_MON)
			value = get_seperated_mon() ? "HIGH" : "LOW";

#endif
		group_name = kmalloc(11, GFP_KERNEL);
		get_gpio_group_name(gpio_list_p[i].gpio_number, group_name);

		printk(KERN_INFO "[%02d] %s(%02d)  %s\t%s\t%s\n",
			   i, group_name, number, direction, value, gpio_list_p[i].gpio_description);
		kfree(group_name);
	}
	printk(KERN_INFO "\n");
	mutex_unlock(&daudio_pinctl_lock);

	return 0;
}

static ssize_t daudio_pinctl_write(struct file *file, const char *buf, size_t count, loff_t *pos)
{
	unsigned char *gpio;
	unsigned char *cmd;
	int gpio_name = -1;
	int gpio_cmd = -1;
	int gpio_number = -1;
	int i = 0;
	int seperator = -1;
	int ret = -1;

	mutex_lock(&daudio_pinctl_lock);

	for (i = 0; i < count; i++) {
		if (buf[i] == ' ' || buf[i] == '\0') {
			seperator = i;
			break;
		}
	}

	if (seperator == count || seperator < 0) {
		if (count <= 2) {	//debug setting
			int temp = atoi(buf);
			if (temp == 0)
				debug_pinctl = 0;
			else if (temp == 1)
				debug_pinctl = 1;
			printk(KERN_INFO "D-Audio GPIO Function setting: %s\n",
				   debug_pinctl ? "Enable" : "Disable");
		}
		else {
			printk(KERN_ERR "%s args error.\n", __func__);
		}
		mutex_unlock(&daudio_pinctl_lock);
		return count;
	}

	gpio = kmalloc(seperator, GFP_KERNEL);
	cmd = kmalloc(count - (seperator + 1) - 1, GFP_KERNEL);

	memcpy(gpio, buf, seperator);
	memcpy(cmd, buf + seperator + 1, count - (seperator + 1) - 1);

	gpio_name = atoi(gpio);
	gpio_cmd = atoi(cmd);

#if 0	//debug
	printk(KERN_INFO "%s cmd: %s, size: %d\n", __func__, cmd,
		   (count - (seperator + 1) - 1));

	printk(KERN_INFO "%s string: [%s] count: %d gpio: %d cmd: %d\n",
		   __func__, buf, count, gpio_name, gpio_cmd);
#endif

	kfree(gpio);
	kfree(cmd);

	if (gpio_name >= 0 && gpio_name < gpio_list_size)
		gpio_number = gpio_list_p[gpio_name].gpio_number;
	else
		gpio_number = -1;

	if (gpio_number >= 0)
		ret = daudio_pinctl_set_gpio(gpio_number, gpio_cmd);
	else {
		ret = -1;
		printk(KERN_ERR "%s failed daudio_pinctl_set_gpio\n", __func__);
	}

	if (ret == 0) {
		char *value = gpio_cmd > 0 ? "HIGH" : "LOW";
		char *group = kmalloc(11, GFP_KERNEL);
		get_gpio_group_name(gpio_number, group);

		printk(KERN_INFO "%s  %s(%d), value: %s\n", __func__,
			   group, gpio_number % 32, value);
		kfree(group);
	}

	mutex_unlock(&daudio_pinctl_lock);

	return count;
}

/*
 * @author	yongki.kim@mobis.co.kr
 * @desc	insert into values to st_ext_int_info structure
 *			in order to (gpio, extint) values
 * @date	2015-08-20
 */
static int daudio_pinctl_fill_parameter(struct st_ext_int_info *int_info)
{
	int ret = -1;

	if((int_info == NULL) || (int_info->extint == 0) || (int_info->gpio == 0))
		return ret;

	switch (int_info->extint) {
		case INT_EINT0:
			break;
		case INT_EINT1:
			break;
		case INT_EINT2:
			break;
		case INT_EINT3:
			break;
		case INT_EINT4:
			break;
		case INT_EINT5:
			break;
		case INT_EINT6:
			break;
		case INT_EINT7:
			break;
		case INT_EINT8:
			break;
		case INT_EINT9:  /*USED(TPEG&COMMAND)*/
			int_info->hw_INT_bit = Hw12; // Have to check IEN0 Register
			int_info->source = EXTINT_GPIOC_21;
			int_info->fn_num = GPIO_FN0;
			int_info->flags = IRQF_TRIGGER_FALLING;
			strncpy((char *)(int_info->label), "ext_irq_9", MAX_STRING_LENGTH);
			break;
		case INT_EINT10:	/* EXAMPLE */
			int_info->hw_INT_bit = Hw13;
			int_info->source = EXTINT_GPIOE_15;
			int_info->fn_num = GPIO_FN0;
			int_info->flags = IRQF_TRIGGER_FALLING;
			strncpy((char *)(int_info->label), "ext_irq_10", MAX_STRING_LENGTH);
			break;
		case INT_EINT11:	/* USED */
			int_info->hw_INT_bit = Hw14;
			// TODO : need to add feature that can determinate boroadcasting moudule(ex) CMMB, DMB, and so on.
			int_info->source = EXTINT_GPIOC_15;
			int_info->fn_num = GPIO_FN0;
			int_info->flags = IRQF_TRIGGER_FALLING;
			strncpy((char *)(int_info->label), "ext_irq_11", MAX_STRING_LENGTH);
			break;

		default:
			printk("%s DEFAULT:No matching case int_info->extint:%d\n", __func__, int_info->extint);
			return ret;
	}

	printk("%s matching case gpio(0x%x) extint(%u) hwBit(0x%x) source(%u) fn_num(0x%x) flags(%lu)\n",
		   __func__, int_info->gpio, int_info->extint, int_info->hw_INT_bit, int_info->source, int_info->fn_num, int_info->flags);

	return 0;
}

/*
 * @author	yongki.kim@mobis.co.kr
 * @desc	external interrupt enable function
 * @date	2015-08-20
 */
static int daudio_pinctl_enable_interrupt(unsigned int gpio, unsigned int extint)
{
	int ret = -1;
	struct st_ext_int_info int_info;

	volatile PPIC pPIC = (PPIC)tcc_p2v(HwPIC_BASE);

	mutex_lock(&enable_interrupt_mutex);

	memset(&int_info, 0x00, sizeof(int_info));
	int_info.gpio = gpio;
	int_info.extint = extint;
	daudio_pinctl_fill_parameter(&int_info);

	// Check the INT is already enabled
	if(ISSET(pPIC->IEN0.nREG, int_info.hw_INT_bit)) {
		printk(KERN_ERR "%s Interrupt is already enabled. Return the request\n", __func__);
		mutex_unlock(&enable_interrupt_mutex);
		return ret;
	}

	// Configure new IRQ Handle
	ret = gpio_request(gpio, int_info.label);
	if (ret) {
		printk(KERN_ERR "%s [error] gpio_request(%d,%s) failed, ret:%d\n", __func__, gpio, int_info.label, ret);
		mutex_unlock(&enable_interrupt_mutex);
		return ret;
	}

	// SET GPIO
	gpio_direction_input(gpio);
	tcc_gpio_config(gpio, int_info.fn_num);
	tcc_gpio_config_ext_intr(extint, int_info.source);

	clear_irq_variables();
#if 0 //jason.ku 2018.10.30
#if defined(INCLUDE_TDMB)
	if (gpio == TCC_GPC(15)) {
		ret = request_irq(extint, ext_irq_handler_1, int_info.flags, int_info.label, NULL);
		if(ret < 0) {
			printk(KERN_ERR "FAILED to aquire irq handle. ret:%d\n", ret);
			mutex_unlock(&enable_interrupt_mutex);
			return ret;
		}
	}
	else
#endif
#endif
	{
		ret = request_irq(extint, ext_irq_handler, int_info.flags, int_info.label, NULL);
		if(ret < 0) {
			printk(KERN_ERR "FAILED to aquire irq handle. ret:%d\n", ret);
			mutex_unlock(&enable_interrupt_mutex);
			return ret;
		}
	}

	// SET Interrupt Enable Register
	BITSET(pPIC->IEN0.nREG, int_info.hw_INT_bit);
	printk("%s ENABLE INT is successed\n", __func__);

	mutex_unlock(&enable_interrupt_mutex);

	return 0;
}

/*
 * @author	yongki.kim@mobis.co.kr
 * @desc	external interrupt disable function
 * @date	2015-08-20
 */
static int daudio_pinctl_disable_interrupt(unsigned int gpio, unsigned int extint)
{
	int ret = -1;
	struct st_ext_int_info int_info;

	volatile PPIC pPIC = (PPIC)tcc_p2v(HwPIC_BASE);

	mutex_lock(&enable_interrupt_mutex);

	memset(&int_info, 0x00, sizeof(int_info));
	int_info.gpio = gpio;
	int_info.extint = extint;
	daudio_pinctl_fill_parameter(&int_info);

	// Check the INT is already disabled
	if(ISZERO(pPIC->IEN0.nREG, int_info.hw_INT_bit)) {
		printk(KERN_ERR "%s Interrupt is already disabled. Return the request\n", __func__);
		mutex_unlock(&enable_interrupt_mutex);
		return ret;
	}

	BITCLR(pPIC->IEN0.nREG, int_info.hw_INT_bit);
	free_irq(extint, NULL);

	tcc_gpio_clear_ext_intr(extint);
	gpio_free(gpio);

	clear_irq_variables();

	printk("%s DISABLE INT is successed\n", __func__);

	mutex_unlock(&enable_interrupt_mutex);

	return 0;
}

/*
 * @author	yongki.kim@mobis.co.kr
 * @desc	external interrupt enable/disable management function
 *      	You can add new case below.
 * @date	2015-08-20
 */
static void daudio_pinctl_manage_interrupt(int gpio, int value)
{
	int ret = -1;
	int cmd = gpio;
	printk("%s cmd:%d value:%d\n", __func__, gpio, value);

	switch(cmd) {
			/*
			 * 1. gpio port number (gpio.h)
			 * 2. IRQ numbers for interrupt handler (irq.h)
			 */
#if 0 // not find CMMB interrupt pin map
		case ENABLE_CMMB_SPI_UPDATE: {
			if(value)
				ret = daudio_pinctl_enable_interrupt(TCC_GPE(16), INT_EINT11);
			else
				ret = daudio_pinctl_disable_interrupt(TCC_GPE(16), INT_EINT11);
		}
		break;
#endif

#if 0 //jason.ku 2018.10.30
		case ENABLE_DAB_SPI_INT: {
			if(value)
				ret = daudio_pinctl_enable_interrupt(TCC_GPC(15), INT_EINT11);
			else
				ret = daudio_pinctl_disable_interrupt(TCC_GPC(15), INT_EINT11);
		}
		break;
#else	//#if 0 //jason.ku 2018.10.30
		case ENABLE_SATURN_SPI_INT: {
			if(value)
				ret = daudio_pinctl_enable_interrupt(TCC_GPC(21), INT_EINT9);
			else
				ret = daudio_pinctl_disable_interrupt(TCC_GPC(21), INT_EINT9);
		}
		break;
#endif	// #if 0 //jason.ku 2018.10.30

		/* New case can be add here */

		default:
			printk("%s Undefined cmd:%d value:%d\n", __func__, gpio, value);
			break;
	}

	if( ret < 0 )
		printk("%s FAIL No:%d, value:%d FAIL\n", __func__, gpio, value);
	else
		printk("%s SUCCESS No:%d, value:%d SUCCESS\n", __func__, gpio, value);

}

static long daudio_pinctl_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct D_Audio_pinioc_cmddata *userdata = NULL;
	struct D_Audio_pinioc_cmddata *data = NULL;
	char *value = 0;
	int gpio_group;
	int gpio_num;
	int gpio_number=0;

	mutex_lock(&daudio_pinctl_lock);

	if (cmd < 0) {
		ret = PINCTL_FAIL;
		printk(KERN_ERR "%s cmd < 0\n", __func__);
	}
	else {
		userdata = (struct D_Audio_pinioc_cmddata *)arg;
		if (userdata == NULL) {
			printk(KERN_ERR, "%s() cmd:%u\n", __func__, cmd);
			mutex_unlock(&daudio_pinctl_lock);
			return PINCTL_FAIL;
		}
		data = (struct D_Audio_pinioc_cmddata *)kmalloc(sizeof(struct D_Audio_pinioc_cmddata), GFP_KERNEL);
		if (data == NULL) {
			printk(KERN_ERR "%s memory is not enough!!!\n", __func__);
			mutex_unlock(&daudio_pinctl_lock);
			return -ENOMEM;
		}

		if (copy_from_user(data, userdata, sizeof(struct D_Audio_pinioc_cmddata))) {
				printk(KERN_ERR "%s copy_from_user!!!\n", __func__);
				kfree(data);
				mutex_unlock(&daudio_pinctl_lock);
				return -EFAULT;
		}
		gpio_number = get_gpio_number(data->pin_name);
//		printk("%s() cmd:%u, gpio_number:%d, value:%d\n", __func__, cmd, data->pin_name, data->pin_value);

		if (gpio_number < 0) {
			printk(KERN_ERR "%s gpio_number(cmd number, value) is (%d, %d)\n", __func__, data->pin_name, data->pin_value);
			kfree(data);
			mutex_unlock(&daudio_pinctl_lock);
			return PINCTL_FAIL;
		}

		switch (cmd) {
			case DAUDIO_PINCTL_CTL:
#if 0
				printk(KERN_INFO "%s DAUDIO_PINCTL_CTL pin:%d value:%d\n",
					   __func__, gpio_number, data->pin_value);
#endif
#if defined(INCLUDE_DAB)
				if((daudio_pinctl_get_gpio(TCC_GPB(3)) == 1) && (data->pin_name == 233)) 	//jason.ku 2018.10.23
					clear_irq_variables();
				else
#endif
					ret = daudio_pinctl_set_gpio(gpio_number, data->pin_value);
				break;

			case DAUDIO_PINCTL_DET:
#if 0
				printk(KERN_INFO "%s DAUDIO_PINCTL_DET\n", __func__);
#endif
#if defined(CONFIG_DAUDIO_ECO) || defined(CONFIG_DAUDIO_KK)
				if(data->pin_name == DET_DMB_ANT_SHORT)
					data->pin_value = get_dmb_diag_short();
				else if(data->pin_name == DET_DMB_ANT_OPEN)
					data->pin_value = get_dmb_diag_open();
				else if(data->pin_name == DET_GPS_ANT_SHORT)
					data->pin_value = get_gps_diag_short();
				else if(data->pin_name == DET_GPS_ANT_OPEN)
					data->pin_value = get_gps_diag_open();
				else if(data->pin_name == DET_SERDES_CONN) //serdes conn check 20180622 mhjung
					data->pin_value = get_serdes_conn();
				else if(data->pin_name == DET_SEPERATED_MON)
					data->pin_value = get_seperated_mon();
				else
#endif
#if defined(INCLUDE_DAB)
				{
					if((daudio_pinctl_get_gpio(TCC_GPB(3)) == 1) && (data->pin_name == 233)) {	// jason.ku 2018.10.23
						if(ext_irq_noti) {
							udelay(10);
							if(daudio_pinctl_get_gpio(gpio_number) == 0)
								data->pin_value = 1;
							else
								data->pin_value = 0;
						}
						else
							data->pin_value = 0;
					}
					else
						data->pin_value = daudio_pinctl_get_gpio(gpio_number);
				}
#else
					data->pin_value = daudio_pinctl_get_gpio(gpio_number);
#endif
				if (data->pin_value != 0 && data->pin_value != 1)
					ret = PINCTL_FAIL;
				else
					ret = copy_to_user(userdata, data, sizeof(struct D_Audio_pinioc_cmddata));
				break;

			case DAUDIO_PINCTL_INT:
				printk("%s() DAUDIO_PINCTL_INT CMD gpio:%d, value:%d\n", __func__, data->pin_name, data->pin_value);
				daudio_pinctl_manage_interrupt(data->pin_name, data->pin_value);
				break;

			default:	//TODO Delete below code later - valky
				ret = daudio_pinctl_set_gpio(data->pin_name, data->pin_value);
				break;
		}

		if (ret == 0 && debug_pinctl) {
			char *group = 0;
			value = data->pin_value > 0 ? "HIGH" : "LOW";
			group = kmalloc(11, GFP_KERNEL);

			if (cmd == DAUDIO_PINCTL_CTL || cmd == DAUDIO_PINCTL_DET) {
				gpio_group = gpio_number / 32;
				gpio_num = gpio_number % 32;
				get_gpio_group_name(gpio_number, group);
			}
			else {
				gpio_group = data->pin_name / 32;
				gpio_num = data->pin_name % 32;
				get_gpio_group_name(data->pin_name, group);
			}

			if (data->pin_name != DET_REVERSE_CPU)	//reverse signal check
				VPRINTK("%s  %s(%d), value: %s\n", __func__, group, gpio_num, value);

			kfree(group);
		}
	}
	kfree(data);
	mutex_unlock(&daudio_pinctl_lock);

	return ret;
}

static int daudio_pinctl_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int daudio_pinctl_release(struct inode *inode, struct file *file)
{
	return 0;
}

int daudio_pinctl_probe(struct platform_device *pdev)
{
	VPRINTK("%s\n", __func__);

	return 0;
}

int daudio_pinctl_remove(struct platform_device *pdev)
{
	VPRINTK("%s\n", __func__);

	return 0;
}

int daudio_pinctl_suspend(struct platform_device *pdev, pm_message_t state)
{
	VPRINTK("%s\n", __func__);

	return 0;
}

int daudio_pinctl_resume(struct platform_device *pdev)
{
	VPRINTK("%s\n", __func__);

	return 0;
}

static struct file_operations daudio_pinctl_fops = {
	.owner			= THIS_MODULE,
	.unlocked_ioctl	= daudio_pinctl_ioctl,
	.read			= daudio_pinctl_read,
	.write			= daudio_pinctl_write,
	.open			= daudio_pinctl_open,
	.release		= daudio_pinctl_release,
};

static int __init daudio_pinctl_init(void)
{
	struct device *dev = NULL;
	int ret = 0;

	printk("%s\n", __func__);

	ret = register_chrdev(DAUDIO_PINCTL_MAJOR, PINCTL_DRIVER_NAME, &daudio_pinctl_fops);
	if (ret < 0) {
		VPRINTK("%s failed to register_chrdev\n", __func__);
		ret = -EIO;
		goto out;
	}

	daudio_pinctl_class = class_create(THIS_MODULE, PINCTL_DRIVER_NAME);
	if (IS_ERR(daudio_pinctl_class)) {
		ret = PTR_ERR(daudio_pinctl_class);
		goto out_chrdev;
	}

	dev = device_create(daudio_pinctl_class, NULL, MKDEV(DAUDIO_PINCTL_MAJOR, 0),
						NULL, PINCTL_DRIVER_NAME);
	VPRINTK("%s %d\n", __func__, ret);

	// CMMB ISR device init
	if(dev) {
		// CMMB Update application checks "ext_spi_int" device every 500ms
		// value 1 represents "receive an interrupt signal from CMMB module"
		device_create_file(dev, &dev_attr_ext_spi_int);
#if 0 //jason.ku 2018.10.30
#if defined(INCLUDE_TDMB)
		device_create_file(dev, &dev_attr_ext_spi_int_1);
#endif
#endif	//#if 0 //jason.ku 2018.10.30
		printk("[pigfish] %s, dev_path: LATER\n", __func__);
	}
	else {
		printk("%s [ERROR] cannot create CMMB ISR device\n", __func__);
		goto out;
	}

	if (init_list() > 0) {
#if 0  /* TODO : check target board version ? */
		printk("%s : gpio list init failed\n", __func__);
#endif
		goto out;
	}

out_chrdev:
	unregister_chrdev(DAUDIO_PINCTL_MAJOR, PINCTL_DRIVER_NAME);
out:
	return ret;
}

static void __exit daudio_pinctl_exit(void)
{
	printk("%s\n", __func__);

	device_destroy(daudio_pinctl_class, MKDEV(DAUDIO_PINCTL_MAJOR, 0));
	class_destroy(daudio_pinctl_class);
	unregister_chrdev(DAUDIO_PINCTL_MAJOR, PINCTL_DRIVER_NAME);
}

module_init(daudio_pinctl_init);
module_exit(daudio_pinctl_exit);

MODULE_AUTHOR("Cleinsoft.");
MODULE_DESCRIPTION("DAUDIO Pin control driver");

#endif //CONFIG_DAUDIO_PIN_CONTROL
