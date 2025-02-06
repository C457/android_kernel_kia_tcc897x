/*
 ** When you modify this source code, 
 ** please notify this change to the Framework Group Members
 **                                & Engineering Mode Developers.
 */

#include <asm/io.h>
#include <asm/setup.h>
#include <linux/fs.h>
#include <linux/major.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/swap.h>
#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/swap.h>

#include <mach/daudio.h>
#include <mach/daudio_info.h>
#include <mach/daudio_settings.h>
#include <mach/daudio_pinctl.h>
#include <mach/gpio.h>

#define DRIVER_NAME				"daudio_info"

#define DEBUG_DAUDIO_VERSION	0

#if (DEBUG_DAUDIO_VERSION)
#define VPRINTK(fmt, args...) printk(KERN_INFO "[cleinsoft Info] " fmt, ##args)
#else
#define VPRINTK(args...) do {} while(0)
#endif

EXPORT_SYMBOL(daudio_lcd_version);
EXPORT_SYMBOL(daudio_main_version);
EXPORT_SYMBOL(daudio_lcd_type_lvds_check);
EXPORT_SYMBOL(daudio_lcd_inch_check);

static struct class *daudio_info_class;
static struct mutex info_lock;
static struct device *daudio_info_dev;

extern int daudio_info_adc_init(struct device *dev);
static int daudio_incell_version_for_user(void);
static int daudio_vendor_verson_for_user(void);
static char **daudio_lcd_versions;

#if !defined(CONFIG_DAUDIO_ECO) && !defined(CONFIG_DAUDIO_KK)
static GPS_LCD_Version_t gps_lcd_ver[] =
{
    {KET_GPS, LCD_7},
    {TRIMBLE, LCD_8_LG},
    {KET_GPS, LCD_8_LG},
    {TRIMBLE, LCD_8_AUO},
    {TRIMBLE, LCD_7},
    {KET_GPS, LCD_8_AUO},
	{0      , 0}			// RESERVED
};
#endif


D_Audio_bsp_version	bsp_version;
D_Audio_bsp_adc		bsp_adc;

int tag_lk_version = -1;

static char swsusp_header_sig[QB_SIG_SIZE];
static unsigned int swsusp_header_addr = 0;

static unsigned char vehicle_info = 0;
static unsigned char country_info = 0;

unsigned char get_vehicle_info(void)
{
	return vehicle_info;
}
EXPORT_SYMBOL(get_vehicle_info);

static int __init vehicle_info_setup(char *str)
{
	sscanf(str, "%x", &vehicle_info);
	return 1;
}
__setup("vehicle_info=", vehicle_info_setup);

unsigned char get_country_info(void)
{
	return country_info;
}
EXPORT_SYMBOL(get_country_info);

static int __init country_info_setup(char *str)
{
	sscanf(str, "%x", &country_info);
	return 1;
}
__setup("country_info=", country_info_setup);

#if !defined(CONFIG_DAUDIO_ECO) && !defined(CONFIG_DAUDIO_KK)
static unsigned int size_gps_lcd = sizeof(gps_lcd_ver)/sizeof(gps_lcd_ver[0]);

int get_lcd_version(int ver)
{

if((ver>=0) && (ver<size_gps_lcd))
	return gps_lcd_ver[ver].lcd_ver;
else
	return 0;
}

int get_gps_version(int ver)
{
if((ver>=0)&&(ver<size_gps_lcd))
	return gps_lcd_ver[ver].gps_ver;
else
	return 0;
}
#endif

int daudio_hw_version(void)
{

	return bsp_version.hw_version;
}

int daudio_main_version(void)
{
	return bsp_version.main_version;
}

int daudio_bt_version(void)
{
	return bsp_version.bt_version;
}


int daudio_gps_version(void)
{
    return bsp_version.gps_version;
}


int daudio_lcd_version(void)
{
    return bsp_version.lcd_version;
}

int daudio_hw_adc(void)
{
	return bsp_adc.hw_adc;
}

int daudio_main_adc(void)
{
	return bsp_adc.main_adc;
}

int daudio_bt_adc(void)
{
	return bsp_adc.bt_adc;
}


int daudio_lcd_adc(void)
{
	return bsp_adc.lcd_adc;
}

static int parse_lk_ver(int lk_version)
{
	bsp_version.lk_version = lk_version;
#if defined(CONFIG_DAUDIO_BSP_VERSION)
	bsp_version.kernel_version = CONFIG_DAUDIO_BSP_VERSION;
#endif

	VPRINTK("BSP LK ver: %d, Kernel ver: %d\n", bsp_version.lk_version, bsp_version.kernel_version);

	return 0;
}

#if defined(CONFIG_DAUDIO_INFO_ATAGS)

static int __init parse_tag_lk_version(const struct tag *tag)
{
	return parse_lk_version((int *)(&tag->u));
}
__tagtable(ATAG_DAUDIO_LK_VERSION, parse_tag_lk_version);

#elif defined(CONFIG_DAUDIO_INFO_CMDLINE)
static int __init parse_cmdline_lk_ver(char* sz)
{
	int lk_version;

	sscanf(sz, "%d", &lk_version);

	return parse_lk_ver(lk_version);
}

__setup("daudio_lk_ver=", parse_cmdline_lk_ver);

#endif
static int lcd_version_check(int oem, int mon)
{
	if((oem == 1) && (mon == 1))
		return daudio_lcd_versions_oe_int_label;
	else if((oem == 0) && (mon == 1))
		return daudio_lcd_versions_pio_int_label;
	else if((oem == 1) && (mon == 0))
		return daudio_lcd_versions_oe_de_label;
	else if((oem == 0) && (mon == 0))
		return daudio_lcd_versions_pio_de_label;
}

static int daudio_incell_version_for_user(void)
{
	int version;
	char *label;
	int mon, oem;
	oem = gpio_get_value(TCC_GPB(24));
	mon = gpio_get_value(TCC_GPB(13));
	daudio_lcd_versions = lcd_version_check(oem, mon);
	version = daudio_lcd_version();
	label = strstr(daudio_lcd_versions[version], "INCELL");
	if(label == NULL)
		return 0;
	else
		return 1;
}

static int daudio_vendor_version_for_user(void)
{
	int version;
	int mon, oem;
	oem = gpio_get_value(TCC_GPB(24));
	mon = gpio_get_value(TCC_GPB(13));
	daudio_lcd_versions = lcd_version_check(oem, mon);
	version = daudio_lcd_version();

	if(strstr(daudio_lcd_versions[version], "LG") != NULL)
	{
		return VENDOR_LG;
	}	
	else if(strstr(daudio_lcd_versions[version], "AUO") != NULL)
	{
		return VENDOR_AUO;
	}
	else if(strstr(daudio_lcd_versions[version], "TRULY") != NULL)
	{
		return VENDOR_TRULY;
	}
	else if(strstr(daudio_lcd_versions[version], "TIANMA") != NULL)
	{
		return VENDOR_TIANMA;
	}
	else if(strstr(daudio_lcd_versions[version], "BOE") != NULL)
	{
		return VENDOR_BOE;
	}
	else
	{
		printk("[Not Spec]Out of Boundary\n");
		return -1;
	}
	
}

static int daudio_lcd_color_temperature_check(void)
{
	int temperature = 7800;	//default = 7800K, 12.3' = 8200K
	int mon, oem;
	oem = gpio_get_value(TCC_GPB(24));
	mon = gpio_get_value(TCC_GPB(13));

	if((oem == 1) && (mon == 0))
	{
		if(daudio_lcd_version() == DAUDIOKK_LCD_OD_12_30_1920_720_INCELL_Si_LG)
			temperature = 8200;
	}

	return temperature;
}

static int daudio_lcd_touchkey_model_check(void)
{
        int touchkey_model = 0; //only PIO 1920 model touchkey_model = ture
        int mon, oem;
        oem = gpio_get_value(TCC_GPB(24));
        mon = gpio_get_value(TCC_GPB(13));

        if((oem == 0) && (mon == 1))
        {
                if(daudio_lcd_version() == DAUDIOKK_LCD_PI_10_25_1920_720_PIO_AUO)
                        touchkey_model = 1;
        }

        return touchkey_model;
}

int daudio_lcd_type_lvds_check(void)
{
	int lvds = 0; //LVDS : 1 , HDMI : 0
	int mon, oem, lcd_ver;

	oem = gpio_get_value(TCC_GPB(24));
        mon = gpio_get_value(TCC_GPB(13));
	lcd_ver = daudio_lcd_version();

	if((oem == 1) && (mon == 1))
	{
		if(lcd_ver == DAUDIOKK_LCD_OI_08_00_1280_720_OGS_Si_BOE)
			lvds = 1;
	}
	else if((oem == 0) && (mon == 1))
	{
		if(lcd_ver == DAUDIOKK_LCD_PI_08_00_800_400_PIO_TRULY)
			lvds = 1;
		else if(lcd_ver == DAUDIOKK_LCD_PI_DISCONNECTED)
			lvds = 1;
	}

	return lvds;
}

/* lcd_size 243.648mm -> 243648 (1um)
*/

int daudio_lcd_inch_check(void)
{
        int version;
        int mon, oem;
        double inch;

        oem = get_oemtype();
        mon = get_montype();
        daudio_lcd_versions = lcd_version_check(oem, mon);
        version = daudio_lcd_version();

        if(strstr(daudio_lcd_versions[version], "10_25") != NULL)
        {
                inch = 10;
        }
        else if(strstr(daudio_lcd_versions[version], "12_30") != NULL)
        {
                inch = 12;
        }
        else if(strstr(daudio_lcd_versions[version], "08_00") != NULL)
        {
                inch = 8;
        }
        else
        {
                inch = 0;
        }

        return inch;
}

static int daudio_lcd_size_check(double *size_x, double *size_y)
{
        int version;
        int mon, oem;

        oem = gpio_get_value(TCC_GPB(24));
        mon = gpio_get_value(TCC_GPB(13));
        daudio_lcd_versions = lcd_version_check(oem, mon);
        version = daudio_lcd_version();

        if(strstr(daudio_lcd_versions[version], "10_25") != NULL)
        {
                (*size_x) = 243.648;
		(*size_y) = 91.368;
        }
        else if(strstr(daudio_lcd_versions[version], "12_30") != NULL)
        {
                (*size_x) = 292.032;
                (*size_y) = 109.512;
        }
        else if(strstr(daudio_lcd_versions[version], "08_00") != NULL)
        {
                (*size_x) = 176.64;
                (*size_y) = 99.36;
        }
        else
        {
		(*size_x) = 243.648;
                (*size_y) = 91.368;
        }

        return 0;
}

static int daudio_lcd_resolution_check(int *resolution_x, int *resolution_y)
{
        int version;
        int mon, oem;

        oem = gpio_get_value(TCC_GPB(24));
        mon = gpio_get_value(TCC_GPB(13));
        daudio_lcd_versions = lcd_version_check(oem, mon);
        version = daudio_lcd_version();

        if(strstr(daudio_lcd_versions[version], "1920_720") != NULL)
        {
                (*resolution_x) = 1920;
		(*resolution_y) = 720;
        }
        else if(strstr(daudio_lcd_versions[version], "1280_720") != NULL)
        {
                (*resolution_x) = 1280;
		(*resolution_y) = 720;
        }
        else if(strstr(daudio_lcd_versions[version], "800_400") != NULL)
        {
                (*resolution_x) = 1280;
		(*resolution_y) = 720;
        }
        else
        {
		(*resolution_x) = 1920;
                (*resolution_y) = 720;
        }

        return 0;
}

/* pixel pitch 0.1269mm -> 1269 (10um)
*/
static int daudio_lcd_pixel_pitch_check(double *pixel_pitch_x, double *pixel_pitch_y)
{
        int version;
        int mon, oem;

        oem = gpio_get_value(TCC_GPB(24));
        mon = gpio_get_value(TCC_GPB(13));
        daudio_lcd_versions = lcd_version_check(oem, mon);
        version = daudio_lcd_version();

        if(strstr(daudio_lcd_versions[version], "10_25") != NULL)
        {
                (*pixel_pitch_x) = 0.1269;
                (*pixel_pitch_y) = 0.1269;
        }
        else if(strstr(daudio_lcd_versions[version], "12_30") != NULL)
        {
		(*pixel_pitch_x) = 0.1521;
                (*pixel_pitch_y) = 0.1521;
        }
        else if(strstr(daudio_lcd_versions[version], "08_00") != NULL)
        {
		(*pixel_pitch_x) = 0.138;
                (*pixel_pitch_y) = 0.138;
        }
        else
        {
		(*pixel_pitch_x) = 0.1269;
                (*pixel_pitch_y) = 0.1269;
        }

        return 0;
}

/*
 * Structure of H/W version
 * temp (1byte) + HW version (1byte) + Platform version (1byte) + BT version (1byte)
 */
static int parse_hw_version(int hw_version)
{
#if !defined(CONFIG_DAUDIO_ECO) && !defined(CONFIG_DAUDIO_KK)
	int gpslcd = 0;
#endif

	bsp_version.hw_version = (hw_version & 0x000000FF);
	bsp_version.main_version = (hw_version & 0x0000FF00) >> 8;
	bsp_version.bt_version = (hw_version & 0xFF000000) >> 24;

#if defined(CONFIG_DAUDIO_ECO) || defined(CONFIG_DAUDIO_KK)
	bsp_version.lcd_version = (hw_version & 0x00FF0000) >> 16;
	 VPRINTK("H/W HW ver: %d, Platform ver: %d,LCD ver: %d\n", bsp_version.hw_version, bsp_version.main_version,bsp_version.lcd_version);
#else
	gpslcd = (*hw_version & 0xFF000000) >> 24;
	bsp_version.gps_version = get_gps_version(gpslcd);
	bsp_version.lcd_version = get_lcd_version(gpslcd);
	VPRINTK("H/W HW ver: %d, Platform ver: %d, BT ver: %d, GPS ver: %d, LCD ver: %d\n", bsp_version.hw_version, bsp_version.main_version, bsp_version.bt_version, bsp_version.gps_version, bsp_version.lcd_version);
#endif	
	return 0;
}

#if defined(CONFIG_DAUDIO_INFO_ATAGS)

static int __init parse_tag_hw_version(const struct tag *tag)
{
	return parse_hw_version((int *)(&tag->u));
}
__tagtable(ATAG_DAUDIO_HW_VERSION, parse_tag_hw_version);

#elif defined(CONFIG_DAUDIO_INFO_CMDLINE)

static int __init parse_cmdline_hw_ver(char *sz)
{
	int hw_version;

	sscanf(sz, "%d", &hw_version);

	return parse_hw_version(hw_version);
}
__setup("daudio_hw_ver=", parse_cmdline_hw_ver);

#endif

#if defined(CONFIG_SNAPSHOT_BOOT)

#if defined(CONFIG_DAUDIO_INFO_ATAGS)

static int __init parse_tag_quickboot_info(const struct tag *tag)
{
	struct quickboot_info *info = (struct quickboot_info *)(&tag->u);
	char temp_sig[QB_SIG_SIZE+1];

	swsusp_header_addr = info->addr;
	memcpy(swsusp_header_sig, info->sig, QB_SIG_SIZE);

	memset(temp_sig, 0, QB_SIG_SIZE+1);
	memcpy(temp_sig, swsusp_header_sig, QB_SIG_SIZE);

	VPRINTK("Quickboot info sig: [%s], addr: 0x%x\n", temp_sig, swsusp_header_addr);

	return 0;
}
__tagtable(ATAG_DAUDIO_QUICKBOOT_INFO, parse_tag_quickboot_info);

#elif defined(CONFIG_DAUDIO_INFO_CMDLINE)

static int parse_qb_info_sig(char* info_sig)
{
	char temp_sig[QB_SIG_SIZE+1];

	sscanf(info_sig, "%s", &temp_sig);
	VPRINTK("quickboot_info_sig : %s \n", temp_sig);

	return 0;
}

static int __init parse_cmdline_quickboot_info_sig(char* sz)
{
	char temp_sig[QB_SIG_SIZE+1];

	sprintf(temp_sig, sz);

	memcpy(swsusp_header_sig, temp_sig, QB_SIG_SIZE);

	VPRINTK("quickboot_info_sig : %s \n", temp_sig);

	return 0;
}

__setup("daudio_qb_sig=", parse_cmdline_quickboot_info_sig);

static int __init parse_cmdline_quickboot_info_addr(char* sz)
{
	int addr_value;
	sscanf(sz, "%d", &addr_value);
	swsusp_header_addr = addr_value;

	VPRINTK("quickboot_info_addr : 0x%x \n", addr_value);

	return 0;
}

__setup("daudio_qb_addr=", parse_cmdline_quickboot_info_addr);

#endif

#endif // CONFIG_SNAPSHOT_BOOT

#define parse_board_adc(adc,value) \
	bsp_adc.adc = value

#if defined(CONFIG_DAUDIO_INFO_ATAGS)

static int __init parse_tag_board_adc(const struct tag *tag)
{
	D_Audio_bsp_adc *info = (D_Audio_bsp_adc *)(&tag->u);

	parse_board_adc(hw_adc, info->hw_adc);
	parse_board_adc(main_adc, info->main_adc);
	parse_board_adc(bt_adc, info->bt_adc);
	parse_board_adc(gps_adc, info->gps_adc);

	VPRINTK("H/W HW adc: %d mv, Platform adc: %d mv, BT adc: %d mv, GPS&LCD adc: %d mv\n",
			bsp_adc.hw_adc, bsp_adc.main_adc, bsp_adc.bt_adc, bsp_adc.gps_adc);

	return 0;
}
__tagtable(ATAG_DAUDIO_BOARD_ADC, parse_tag_board_adc);

#elif defined(CONFIG_DAUDIO_INFO_CMDLINE)

#define setup_cmdline_board_adc(name)	\
	static int __init parse_cmdline_board_adc_##name(char *sz) { \
		int adc; \
		sscanf(sz, "%d", &adc); \
		parse_board_adc(name, adc); \
		return 0; \
	} \
	__setup("daudio_adc_" #name "=",parse_cmdline_board_adc_##name);

static int __init parse_cmdline_board_adc_lcd(char* sz)
{
	int adc;
	sscanf(sz, "%d", &adc);
	parse_board_adc(lcd_adc, adc);

	VPRINTK("H/W HW adc: %d mv, Platform adc: %d mv, LCD adc: %d mv, BT adc: %d mv\n",bsp_adc.hw_adc, bsp_adc.main_adc,bsp_adc.lcd_adc, bsp_adc.bt_adc);

	return 0;
}

setup_cmdline_board_adc(hw_adc);
setup_cmdline_board_adc(main_adc);
setup_cmdline_board_adc(bt_adc);
__setup("daudio_adc_lcd=", parse_cmdline_board_adc_lcd);

#endif

#if defined(QUICKBOOT_SIG)
#define HIBERNATE_SIG  QUICKBOOT_SIG
#else
#define HIBERNATE_SIG  "S1SUSPEND"
#endif

static ssize_t daudio_info_read(struct file *file, char *buf, size_t count, loff_t *offset)
{
	char snapshot_sig[QB_SIG_SIZE+1];
	char kernel_sig[QB_SIG_SIZE+1];

	int oem, mon;
	   
	oem = gpio_get_value(TCC_GPB(24));
	mon = gpio_get_value(TCC_GPB(13));
	
	daudio_lcd_versions = lcd_version_check(oem, mon);

	memset(snapshot_sig, 0, QB_SIG_SIZE+1);
	memset(kernel_sig, 0, QB_SIG_SIZE+1);
	memcpy(snapshot_sig, swsusp_header_sig, QB_SIG_SIZE);
#if defined(CONFIG_HIBERNATION)
	get_hibernate_sig(kernel_sig);
#else
	memset(kernel_sig, 0, QB_SIG_SIZE + 1);
#endif

	printk(KERN_INFO "BSP LK Version : %d\n", bsp_version.lk_version);
	printk(KERN_INFO "BSP Kernel Version: %d\n", bsp_version.kernel_version);
	printk(KERN_INFO "H/W HW Version: %d, adc %d mv\n", bsp_version.hw_version, bsp_adc.hw_adc);
	printk(KERN_INFO "H/W Mainboard Version: %d, adc %d mv\n", bsp_version.main_version, bsp_adc.main_adc);
	printk(KERN_INFO "H/W BT Version: %d, adc: %d mv\n", bsp_version.bt_version, bsp_adc.bt_adc);
//	printk(KERN_INFO "H/W GPS Version: %d, adc: %d mv\n", bsp_version.gps_version, bsp_adc.gps_adc);
	printk(KERN_INFO "H/W LCD Version[OEM(%d), MON(%d)]: %s(%d), adc: %d mv\n", oem, mon, daudio_lcd_versions[bsp_version.lcd_version], bsp_version.lcd_version, bsp_adc.lcd_adc);
	printk(KERN_INFO "H/W INCELL Version for user : %d\n", daudio_incell_version_for_user());
	printk(KERN_INFO "H/W LCD Vendor Version : %d\n", daudio_vendor_version_for_user());
//	printk(KERN_INFO "Quickboot Kernel define sig: [%s], snapshot header sig: [%s]\n", kernel_sig, snapshot_sig);
//	printk(KERN_INFO "Quickboot start addr: 0x%x\n", swsusp_header_addr);	
//	printk(KERN_INFO "GPS(0)/RTC(1) DET adc : %ld mv\n", get_ant_diag_adc(6));
//	printk(KERN_INFO "GPS(0)/RTC(1) DET : %d\n", get_gps_rtc_det()); 
#if defined(CONFIG_DAUDIO_ECO) || defined(CONFIG_DAUDIO_KK)
	#if defined(INCLUDE_TDMB)
	printk(KERN_INFO "GPS/DMB ANT adc: %ld mv\n", get_ant_diag_adc(2));
	#else
	printk(KERN_INFO "GPS ANT adc: %ld mv\n", get_ant_diag_adc(2));
	printk(KERN_INFO "DAB/ISDB_T ANT adc: %ld mv\n", get_ant_diag_adc(8));
	#endif
#endif
	return 0;
}

static long daudio_info_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	switch (cmd)
	{
		case DAUDIO_BSP_VERSION_DET:
			{
				D_Audio_bsp_version bsp_ver;
				ret = copy_from_user(&bsp_ver, (D_Audio_bsp_version*)arg, sizeof(D_Audio_bsp_version));
				if (ret) {
					printk(KERN_ERR "%s: failed to copy_from_user (%d)\n", __func__, ret);
					return -EFAULT;
				}
				bsp_ver.lk_version = bsp_version.lk_version;
				bsp_ver.kernel_version = bsp_version.kernel_version;
				bsp_ver.hw_version = bsp_version.hw_version;
				bsp_ver.main_version = bsp_version.main_version;
				bsp_ver.bt_version = bsp_version.bt_version;
				bsp_ver.gps_version = bsp_version.gps_version; 
				bsp_ver.lcd_version = bsp_version.lcd_version;
				bsp_ver.incell_version = daudio_incell_version_for_user();
				bsp_ver.vendor_version = daudio_vendor_version_for_user();
				ret = copy_to_user((D_Audio_bsp_version*)arg, &bsp_ver, sizeof(D_Audio_bsp_version));
				if (ret) {
					printk(KERN_ERR "%s: failed to copy_to_user (%d)\n", __func__, ret);
					return -EFAULT;
				}
				ret = 1;
			}
			break;

		case DAUDIO_BSP_ADC_DET:
			{
				D_Audio_bsp_adc     hw_adc;
				ret = copy_from_user(&hw_adc, (D_Audio_bsp_adc*)arg, sizeof(D_Audio_bsp_adc));
				if (ret) {
					printk(KERN_ERR "%s: failed to copy_from_user (%d)\n", __func__, ret);
					return -EFAULT;
				}
				hw_adc.hw_adc = bsp_adc.hw_adc;
				hw_adc.main_adc = bsp_adc.main_adc;
				hw_adc.bt_adc = bsp_adc.bt_adc;
				hw_adc.lcd_adc = bsp_adc.lcd_adc;
				ret = copy_to_user((D_Audio_bsp_adc*)arg, &hw_adc, sizeof(D_Audio_bsp_adc));
				if (ret) {
					printk(KERN_ERR "%s: failed to copy_to_user (%d)\n", __func__, ret);
					return -EFAULT;
				}
				ret = 1;
			}
			break;
		case DAUDIO_BSP_LCD_COLOR_TEMPERATURE_DET:
			{
				int temperature = 0;

				temperature = daudio_lcd_color_temperature_check();
				ret = copy_to_user((int*)arg, &temperature, sizeof(int));
				if (ret) {
					printk(KERN_ERR "%s: failed to copy_to_user (%d)\n", __func__, ret);
					return -EFAULT;
				}
				ret = 1;
			}
			break;
		case DAUDIO_BSP_LCD_INFO_DET:
                        {
                                D_Audio_bsp_lcd_info bsp_lcd_info;

				daudio_lcd_resolution_check(&bsp_lcd_info.resolution[0], &bsp_lcd_info.resolution[1]);
				daudio_lcd_size_check(&bsp_lcd_info.size[0], &bsp_lcd_info.size[1]);
				daudio_lcd_pixel_pitch_check(&bsp_lcd_info.pixel_pitch[0], &bsp_lcd_info.pixel_pitch[1]);
				bsp_lcd_info.temperature = daudio_lcd_color_temperature_check();
				bsp_lcd_info.touchkey_model = daudio_lcd_touchkey_model_check();

                                ret = copy_to_user((int*)arg, &bsp_lcd_info, sizeof(D_Audio_bsp_lcd_info));
                                if (ret) {
                                        printk(KERN_ERR "%s: failed to copy_to_user (%d)\n", __func__, ret);
                                        return -EFAULT;
                                }
                                ret = 1;
                        }
                        break;
		case DAUDIO_BSP_BACK_LCD_INFO_DET:
                        {
                                D_Audio_bsp_lcd_info bsp_lcd_info;

				bsp_lcd_info.resolution[0] = 0;
				bsp_lcd_info.resolution[1] = 0;
				bsp_lcd_info.size[0] = 0;
				bsp_lcd_info.size[1] = 0;
				bsp_lcd_info.pixel_pitch[0] = 0;
				bsp_lcd_info.pixel_pitch[1] = 0;
				bsp_lcd_info.temperature = 0;
				bsp_lcd_info.touchkey_model = 0;

                                ret = copy_to_user((int*)arg, &bsp_lcd_info, sizeof(D_Audio_bsp_lcd_info));
                                if (ret) {
                                        printk(KERN_ERR "%s: failed to copy_to_user (%d)\n", __func__, ret);
                                        return -EFAULT;
                                }
                                ret = 1;
                        }
                        break;
	}

	return ret;
}

static int daudio_info_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int daudio_info_release(struct inode *inode, struct file *file)
{
	return 0;
}

static struct file_operations daudio_info_fops =
{
	.owner          = THIS_MODULE,
	.unlocked_ioctl = daudio_info_ioctl,
	.open           = daudio_info_open,
	.release        = daudio_info_release,
	.read           = daudio_info_read,
};

static __init int daudio_info_init(void)
{
	int ret = register_chrdev(DAUDIO_INFO_MAJOR, DRIVER_NAME, &daudio_info_fops);
	VPRINTK("%s %d\n", __func__, ret);
	if (ret < 0)
	{
		printk(KERN_ERR "%s failed to register_chrdev\n", __func__);
		ret = -EIO;
		goto out;
	}

	daudio_info_class = class_create(THIS_MODULE, DRIVER_NAME);
	if (IS_ERR(daudio_info_class))
	{
		ret = PTR_ERR(daudio_info_class);
		goto out_chrdev;
	}

	daudio_info_dev = device_create(daudio_info_class, NULL, MKDEV(DAUDIO_INFO_MAJOR, 0), NULL, DRIVER_NAME);
	mutex_init(&info_lock);

	daudio_info_adc_init(daudio_info_dev);
#if defined(CONFIG_DAUDIO_INFO)
	get_daudio_rev();
#endif
	goto out;

out_chrdev:
	unregister_chrdev(DAUDIO_INFO_MAJOR, DRIVER_NAME);
out:
	return ret;
}

static __exit void daudio_info_exit(void)
{
	device_destroy(daudio_info_class, MKDEV(DAUDIO_INFO_MAJOR, 0));
	class_destroy(daudio_info_class);
	unregister_chrdev(DAUDIO_INFO_MAJOR, DRIVER_NAME);
}

subsys_initcall(daudio_info_init);
module_exit(daudio_info_exit);

MODULE_AUTHOR("Cleinsoft");
MODULE_DESCRIPTION("D-Audio Info Driver");
MODULE_LICENSE("GPL");
