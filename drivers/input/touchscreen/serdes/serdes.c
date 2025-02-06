#include "../melfas_mip4/mip4_ts.h"
#include "../siw/touch_sw17700.h"
#include"../atmel/atmel_mxt336S.h"

#include <mach/vioc_lut.h>
#include <mach/vioc_blk.h>
#include <mach/tcc_lut_ioctl.h>
#include <linux/sched.h>

#ifdef CONFIG_DAUDIO_KK
#include <mach/mobis_nvdata_handler.h>
#elif defined(CONFIG_WIDE_PE_COMMON)
#include <mobis/mobis_nvdata_handler.h>
#endif

#undef VERIFY_OCTAL_PERMISSIONS
#define VERIFY_OCTAL_PERMISSIONS(perms) (perms)

#include <linux/switch.h>
#define DRIVER_NAME     "serdes_recovery"
#define SERDES_DEVICE_NAME "serdes_recovery"

#define SERDES_RESET_TIME 4000
#define MAX_TOUCH_COUNTER 10

#define iDTC_OK_ALL 0x0
#define iDTC_LINE_FAULT 0x1 // AB010101
#define iDTC_LOCK_BIT 0x2 //AB010102
#define iDTC_SER_VIDEO_LOCK 0x4 //AB010201
#define iDTC_TOUCH_WATCHDOG_ERR 0x8 //A020101
#define iDTC_TOUCH_CRC_ERR 0x10 //A020103

u8 mobis_idtc_check_read(void);
void mobis_idtc_check_write(u8 *serdes_check_value_arr, int lock_pin, int line_fault, bool touch_watchdog, bool touch_crc_err);
bool mobis_touch_counter_flag_check(int id);
void mobis_touch_counter_press(char *buf, int id);
void mobis_touch_counter_release(char *buf, int id);
void mobis_touch_counter_print(void);
void mobis_touch_counter_reset(void);

EXPORT_SYMBOL(mobis_idtc_check_read);
EXPORT_SYMBOL(mobis_idtc_check_write);
EXPORT_SYMBOL(mobis_touch_update_check);
EXPORT_SYMBOL(mobis_touch_update_complete);
EXPORT_SYMBOL(mobis_serdes_need_log);
EXPORT_SYMBOL(mobis_show_need_log_count);
EXPORT_SYMBOL(mobis_serdes_status_count_reset);
EXPORT_SYMBOL(mobis_serdes_status_count_update);
EXPORT_SYMBOL(mobis_serdes_status_count_read);
EXPORT_SYMBOL(mobis_serdes_status_count_write);
EXPORT_SYMBOL(PRINT_SERDES_LOG);
EXPORT_SYMBOL(mobis_touch_counter_flag_check);
EXPORT_SYMBOL(mobis_touch_counter_press);
EXPORT_SYMBOL(mobis_touch_counter_release);
EXPORT_SYMBOL(mobis_touch_counter_print);
EXPORT_SYMBOL(mobis_touch_counter_reset);
int need_log_count = 0;

int PRINT_SERDES_LOG = 1;

static int iDTC_READY = 0;

static u8 last_serdes_check_value[5];
int serdes_update_check[3] = {0,0,0};
static u8 serdes_idtc_check_value;

struct serdes_info {
        struct i2c_client *client;
struct {
                struct delayed_work reset_dwork;
                struct mutex lock;
} serdes;

	struct switch_dev serdes_dev;
	struct switch_dev idtc_dev;
};

struct serdes_info *g_info = NULL;

int red_value = 255;
int green_value = 255;
int blue_value = 255;

bool touch_counter_lock_flag = false;
int touch_counter_cnt = 0;
char touch_counter_log[MAX_TOUCH_COUNTER*2][200];

bool mobis_touch_counter_flag_check(int id)
{
	if(id == 0)
		return touch_counter_lock_flag;
	else
		return true;
}
void mobis_touch_counter_press(char *buf, int id)
{
	int index = (touch_counter_cnt % MAX_TOUCH_COUNTER) * 2;

	unsigned long rem_nsec;
	char time_log[20];

	if(id != 0)
		return;

	u64 ts = local_clock();

	if(touch_counter_lock_flag == true)
		return;

	rem_nsec = do_div(ts, 1000000000);

	sprintf(time_log, "[%5lu.%06lu] ", (unsigned long)ts, rem_nsec / 1000);

	strcpy(touch_counter_log[index], time_log);
	strcat(touch_counter_log[index], buf);

	touch_counter_lock_flag = true;
	return;
}
void mobis_touch_counter_release(char *buf, int id)
{
	int index = (touch_counter_cnt % MAX_TOUCH_COUNTER) * 2 + 1;

	unsigned long rem_nsec;
        char time_log[20];

	if(id != 0)
		return;

        u64 ts = local_clock();

        rem_nsec = do_div(ts, 1000000000);

        sprintf(time_log, "[%5lu.%06lu] ", (unsigned long)ts, rem_nsec / 1000);

	strcpy(touch_counter_log[index], time_log);
	strcat(touch_counter_log[index], buf);

	touch_counter_lock_flag = false;
	touch_counter_cnt++;
	return;
}
void mobis_touch_counter_print(void)
{
	int index;
	int counter = 0;

	if(touch_counter_cnt >= MAX_TOUCH_COUNTER)
	{
		index = touch_counter_cnt % MAX_TOUCH_COUNTER * 2;
		counter = 10;
	}
	else
	{
		index = 0;
		counter = touch_counter_cnt * 2;
	}

	printk("--------- Last %d Press/Release Touch Log ---------\n", MAX_TOUCH_COUNTER);

	for(; counter > 0  ; counter--)
	{
		printk("%s\n",touch_counter_log[index]);
		if(index < MAX_TOUCH_COUNTER * 2 - 1)
			index++;
		else
			index = 0;
	}

	printk("\nTouch Total Count : %d\n", touch_counter_cnt);

	printk("----------------------------------------------------\n\n");
}
void mobis_touch_counter_reset(void)
{
	touch_counter_lock_flag = false;
	touch_counter_cnt = 0;
}

int mobis_touch_update_check(void)
{
        char buffer[2] = {0x00, 0x00};
        mobis_misc_nvdata_read(MOBIS_NVDATA_MISC_TOUCH_UPDATE, buffer, 1);
        if(buffer[0] == 0x1)
        {
                printk("Touch Firmware Update Set\n",__func__);
                // TODO touch firmware update and clear misc data
                return 1;
        }
        else
        {
                printk("Touch Firmware nothing todo....\n",__func__);
                return 0;
        }
}
void mobis_touch_update_complete(void)
{
        char buffer[2] = {0x00, 0x00};
        memset(buffer, 0x00, sizeof(buffer));
        mobis_misc_nvdata_write(MOBIS_NVDATA_MISC_TOUCH_UPDATE, buffer, 1);

        printk("Touch Firmware Update Complete\n",__func__);
}

u8 mobis_idtc_check_read(void)
{
	return serdes_idtc_check_value;
}
void mobis_idtc_check_write(u8 *serdes_check_value_arr, int lock_pin, int line_fault, bool touch_watchdog, bool touch_crc_err)
{
	if(iDTC_READY == 0)
	{
		last_serdes_check_value[0] = serdes_check_value_arr[0];
		last_serdes_check_value[4] = serdes_check_value_arr[4];
		return;
	}

	if(iDTC_READY == 2)
        {
                printk("line_fault : 0x%X lock pin : 0x%X video_lock : 0x%X touch watchdog : %d touch crc : %d\n",
                 (line_fault==-1) ? 0 : line_fault , lock_pin, serdes_check_value_arr[1], touch_watchdog, touch_crc_err);
        }

	if(lock_pin != 0 || line_fault != 0)
	{
		if(((serdes_check_value_arr[1] != 0xE1) || lock_pin == -1))
		{
			switch_set_state(&g_info->idtc_dev, iDTC_SER_VIDEO_LOCK);
			if(iDTC_READY == 2)
				printk("iDTC_SER_VIDEO_LOCK\n");
			last_serdes_check_value[0] = serdes_check_value_arr[0];
	                last_serdes_check_value[4] = serdes_check_value_arr[4];
			return;
		}

		if(line_fault == 0x2E && lock_pin != -1 && (lock_pin&0xFA)!=0xDA)
		{
			switch_set_state(&g_info->idtc_dev, iDTC_LINE_FAULT);
			if(iDTC_READY == 2)
				printk("iDTC_LINE_FAULT\n");
			last_serdes_check_value[0] = serdes_check_value_arr[0];
	                last_serdes_check_value[4] = serdes_check_value_arr[4];
			return;
		}

		if((lock_pin&0xFA)!=0xDA && line_fault != 0x2E && lock_pin != -1)
		{
			switch_set_state(&g_info->idtc_dev, iDTC_LOCK_BIT);
			if(iDTC_READY == 2)
				printk("iDTC_LOCK_BIT\n");
			last_serdes_check_value[0] = serdes_check_value_arr[0];
	                last_serdes_check_value[4] = serdes_check_value_arr[4];
			return;
		}
	}

	if(touch_watchdog == true)
	{
		printk("Ser Err : %d(%d) Des Err : %d(%d)\n",serdes_check_value_arr[0],last_serdes_check_value[0], serdes_check_value_arr[4], last_serdes_check_value[4]);
		if(serdes_check_value_arr[0] >= 0x20 || serdes_check_value_arr[4] >= 0x20 
		|| last_serdes_check_value[0] >= 0x20 || last_serdes_check_value[4] >= 0x20)
		{
			switch_set_state(&g_info->idtc_dev, iDTC_LOCK_BIT);
			if(iDTC_READY == 2)
				printk("iDTC_LOCK_BIT\n");
		}
		else
		{
			switch_set_state(&g_info->idtc_dev, iDTC_TOUCH_WATCHDOG_ERR);
                        if(iDTC_READY == 2)
                                printk("iDTC_TOUCH_WATCHDOG_ERR\n");
		}
		last_serdes_check_value[0] = serdes_check_value_arr[0];
                last_serdes_check_value[4] = serdes_check_value_arr[4];
		return;
	}

	if(touch_crc_err == true)
	{
		switch_set_state(&g_info->idtc_dev, iDTC_TOUCH_CRC_ERR);
		if(iDTC_READY == 2)
			printk("iDTC_TOUCH_CRC_ERR/n");
		last_serdes_check_value[0] = serdes_check_value_arr[0];
                last_serdes_check_value[4] = serdes_check_value_arr[4];
		return;
	}

	switch_set_state(&g_info->idtc_dev, iDTC_OK_ALL);
	if(iDTC_READY == 2)
		printk("iDTC_OK_ALL\n");
	last_serdes_check_value[0] = serdes_check_value_arr[0];
        last_serdes_check_value[4] = serdes_check_value_arr[4];
	return;
}

void mobis_serdes_need_log(void)
{
        if(g_info != NULL)
        {
                need_log_count++;
                switch_set_state(&g_info->serdes_dev, need_log_count);
        }
}

void mobis_show_need_log_count(void)
{
        printk("need log count : %d\n\n",need_log_count);
}

void mobis_serdes_status_count_reset(void)
{
        char buffer[4] = {0x00, 0x00, 0x00, 0x00};
        memset(buffer, 0x00, sizeof(buffer));

        printk("%s [Start]\n",__func__);

	serdes_update_check[0] = 0;
	serdes_update_check[1] = 0;
	serdes_update_check[2] = 0;

        mobis_serdes_status_count_write(buffer);
}

void mobis_serdes_status_count_update(u8 *serdes_check_value_arr, int lock_pin, int line_fault)
{
        char buffer[4] = {0x00, 0x00, 0x00, 0x00};
        memset(buffer, 0x00, sizeof(buffer));

        mobis_serdes_status_count_read(buffer);

        if(serdes_check_value_arr[1] != 0xE1 && serdes_update_check[0] == 0)
        {
                if(buffer[0] < 0xFF)
                        buffer[0]++;

                serdes_update_check[0] = 1;
        }

        if(line_fault != 0x2A && ((lock_pin&0xfa)!=0xda) && (serdes_update_check[1] == 0))
        {
                if(buffer[1] < 0xFF)
                        buffer[1]++;

                serdes_update_check[1] = 1;
        }

        if(((lock_pin&0xfa)!=0xda || serdes_check_value_arr[0] >= 0x20 || serdes_check_value_arr[4] >= 0x20) && serdes_update_check[2] == 0)
        {
                if(buffer[2] < 0xFF)
                        buffer[2]++;

                serdes_update_check[2] = 1;
        }

        buffer[3] = 1;

        mobis_serdes_status_count_write(buffer);
}

void mobis_serdes_status_count_read(char *buffer)
{
        mobis_misc_nvdata_read(MOBIS_NVDATA_MISC_SERDES_STATUS, buffer, 4);
}

void mobis_serdes_status_count_write(char *buffer)
{
        mobis_misc_nvdata_write(MOBIS_NVDATA_MISC_SERDES_STATUS, buffer, 4);
}

inline void serdes_reset_dwork_stop(struct serdes_info *info)
{
        cancel_delayed_work(&info->serdes.reset_dwork);
}

inline void serdes_reset_dwork_start(struct serdes_info *info)
{
        serdes_reset_dwork_stop(info);
        schedule_delayed_work(&info->serdes.reset_dwork,msecs_to_jiffies(SERDES_RESET_TIME));
}

static void serdes_reset_dwork(struct work_struct *work)
{
	int lcd_ver = 0;
	struct serdes_info *info = container_of(work, struct serdes_info, serdes.reset_dwork);

	lcd_ver = daudio_lcd_version();

	printk("%s, GPIO_B24 = %d, GPIO_B13 = %d, GPIO_B8 = %d,  daudio_lcd_version() = %d \n", __func__, get_oemtype(), get_montype(), get_factorycheck(), lcd_ver);
#ifdef CONFIG_TOUCHSCREEN_ONEBIN
	if(get_oemtype())
	{
		if(get_montype())
                {
			switch(lcd_ver)
			{
				case DAUDIOKK_LCD_OI_10_25_1920_720_INCELL_Si_LG:
	                        case DAUDIOKK_LCD_OI_10_25_1920_720_INCELL_Si_2_LG:
				case DAUDIOKK_LCD_OI_DISCONNECTED:
					printk("%s mip4_init()\n",__func__);
					mip4_init();
                                        break;
				case DAUDIOKK_LCD_OI_10_25_1920_720_INCELL_LTPS_LG:
					printk("%s sw17700_init()\n",__func__);
					sw17700_init();
					break;
				case DAUDIOKK_LCD_OI_08_00_1280_720_OGS_Si_BOE:
					printk("%s mxt336s_init()\n",__func__);
                                        mxt336s_init();
				default:
					break;
			}
                }
                else
                {
			switch(lcd_ver)
                        {
				case DAUDIOKK_LCD_OD_12_30_1920_720_INCELL_Si_LG:
					printk("%s mip4_init()\n",__func__);
                                        mip4_init();
                                        break;
				case DAUDIOKK_LCD_OD_10_25_1920_720_INCELL_LTPS_LG:
					if(!get_factorycheck())
					{
						printk("%s mip4_init()\n",__func__);
						mip4_init();
						break;
					}
					printk("%s sw17700_init()\n",__func__);
                                        sw17700_init();
					break;
				case DAUDIOKK_LCD_OD_08_00_1280_720_OGS_Si_BOE:
					printk("%s mxt336s_init()\n",__func__);
                                        mxt336s_init();
				default:
					break;
                        }
                }
	}
	else
	{
		printk("%s mxt336s_init()\n",__func__);
		mxt336s_init();
	}
#endif
}

static ssize_t set_rmmod(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
        int state;
        struct i2c_client *client = to_i2c_client(dev);
        struct serdes_info *info = i2c_get_clientdata(client);

        sscanf(buf, "%d", &state);
#ifdef CONFIG_TOUCHSCREEN_ONEBIN
        if(state == 1)
        {
                mip4_cleanup();
                printk("mip4_cleanup [Done]\n");
        }
        else if(state == 2)
        {
                sw17700_cleanup();
                printk("sw17700_cleanup [Done]\n");
        }
#endif
        return count;
}

static ssize_t set_insmod(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
        int state;
        struct i2c_client *client = to_i2c_client(dev);
        struct serdes_info *info = i2c_get_clientdata(client);

        sscanf(buf, "%d", &state);
#ifdef CONFIG_TOUCHSCREEN_ONEBIN
        if(state == 1)
        {
                mip4_init();
                printk("mip4_init [Done]\n");
        }
        else if(state == 2)
        {
                sw17700_init();
                printk("sw17700_init [Done]\n");
        }
#endif
        return count;
}

static ssize_t _set_bluelight(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
        int i = 0;

        unsigned int BGR_Gamma[256];

        sscanf(buf, "%d %d %d", &red_value, &green_value, &blue_value);

        printk("%s Start %d %d %d\n",__func__,red_value, green_value, blue_value);

        tcc_set_lut_enable(TVC_LUT(LUT_DEV0), true);

        for(i = 0; i < 256; i++)
        {
                BGR_Gamma[i] = ((int)(blue_value*i/255) << 16) | ((int)(green_value*i/255) << 8) | (int)(red_value*i/255);
        }

        tcc_set_lut_table(TVC_LUT(LUT_DEV0), BGR_Gamma);

        return count;
}

static ssize_t _show_bluelight(struct device *dev, struct device_attribute *attr, const char *buf)
{
	printk("%s Start %d %d %d\n",__func__,red_value, green_value, blue_value);
	snprintf(buf, 255, "%d %d %d\n", red_value, green_value, blue_value);
}

static ssize_t _show_fw_update(struct device *dev, struct device_attribute *attr, const char *buf)
{
        char buffer[2] = {0x00, 0x00};

        printk("%s [Start]\n",__func__);

        mobis_misc_nvdata_read(MOBIS_NVDATA_MISC_TOUCH_UPDATE, buffer, 1);

        if(buffer[0] == 0x01)
                printk("Fw Update State : Set\n");
        else if(buffer[0] == 0x00)
                printk("Fw Update State : Cancel\n");
        else
                printk("Fw Update State : Wrong\n");
}
static ssize_t _set_fw_update(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
        int state;
        char buffer[2] = {0x00, 0x00};

        printk("%s [Start]\n",__func__);

        sscanf(buf, "%d", &state);

        if(state == 1)
        {
                printk("Set Fw Update Next boot\n");
                memset(buffer, 0x01, sizeof(buffer));
                mobis_misc_nvdata_write(MOBIS_NVDATA_MISC_TOUCH_UPDATE, buffer, 1);
        }
        else if(state == 0)
        {
                printk("Cancel Fw Update\n");
                memset(buffer, 0x00, sizeof(buffer));
                mobis_misc_nvdata_write(MOBIS_NVDATA_MISC_TOUCH_UPDATE, buffer, 1);
        }
        else
        {
                printk("Input wrong value : %d\n",state);
        }
        return count;
}

static ssize_t _show_need_log(struct device *dev, struct device_attribute *attr, const char *buf)
{
        mobis_show_need_log_count();
}

static ssize_t _set_need_log(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
        mobis_serdes_need_log();

        return count;
}

static ssize_t _show_serdes_status_count(struct device *dev, struct device_attribute *attr, const char *buf)
{
        int i;
        char buffer[4] = {0x00, 0x00, 0x00, 0x00};

        printk("%s [Start]\n",__func__);

        mobis_serdes_status_count_read(buffer);
        for(i = 0;i<4;i++)
                printk("%s buffer[%d] = %d\n",__func__, i, buffer[i]);
}

static ssize_t _set_serdes_status_count(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
        int i;
        int state1, state2;
        char buffer[4] = {0x00, 0x00, 0x00, 0x00};

        printk("%s [Start]\n",__func__);

        sscanf(buf, "%d %d", &state1, &state2);

        mobis_serdes_status_count_read(buffer);

        for(i = 0;i<4;i++)
                printk("%s buffer[%d] = %d\n",__func__, i, buffer[i]);

        buffer[state1] = state2;

        mobis_serdes_status_count_write(buffer);

        for(i = 0;i<4;i++)
                printk("%s buffer[%d] = %d\n",__func__, i, buffer[i]);

        return count;
}

static ssize_t show_idtc_ready(struct device *dev, struct device_attribute *attr, const char *buf)
{
        printk("%s [Start]\n",__func__);

	printk("Current iDTC state : %d\n",iDTC_READY);

        return 0;
}

static ssize_t set_idtc_ready(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int state;

        printk("%s [Start]\n",__func__);

        if(sscanf(buf, "%d", &state) < 0)
                return -EINVAL;

        if(state == 1)
        {
                printk("Set iDTC ready\n");
		iDTC_READY = state;
        }
        else if(state == 0)
        {
                printk("Cancel iDTC ready\n");
		iDTC_READY = state;
        }
	else if(state == 2)
        {
                printk("Set iDTC ready and Log Enable\n");
                iDTC_READY = state;
        }
        else
        {
                printk("Input wrong value : %d\n",state);
        }
        if(count < INT_MAX)
                return count;
        else
                return INT_MAX;
}

static DEVICE_ATTR(rmmod, S_IRUGO | S_IWUSR | S_IWGRP, NULL, set_rmmod);
static DEVICE_ATTR(insmod, S_IRUGO | S_IWUSR | S_IWGRP, NULL, set_insmod);
static DEVICE_ATTR(bluelight, S_IRUGO | S_IWUSR | S_IWGRP , _show_bluelight, _set_bluelight);
static DEVICE_ATTR(fw_update, S_IRUGO | S_IWUGO ,_show_fw_update, _set_fw_update);
static DEVICE_ATTR(need_log, S_IRUGO | S_IWUSR | S_IWGRP ,_show_need_log, _set_need_log);
static DEVICE_ATTR(serdes_status_count, S_IRUGO | S_IWUSR | S_IWGRP ,_show_serdes_status_count, _set_serdes_status_count);
static DEVICE_ATTR(idtc_ready, S_IRUGO | S_IWUGO ,show_idtc_ready, set_idtc_ready);

static struct attribute *serdes_attrs[] = {
	&dev_attr_rmmod.attr,
        &dev_attr_insmod.attr,
	&dev_attr_bluelight.attr,
	&dev_attr_fw_update.attr,
	&dev_attr_need_log.attr,
	&dev_attr_serdes_status_count.attr,
	&dev_attr_idtc_ready.attr,
	NULL,
};

static const struct attribute_group serdes_attr_group = {
        .attrs = serdes_attrs,
};

static int serdes_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct serdes_info *info;
	int ret;

	printk("%s [Start]\n",__func__);

	serdes_idtc_check_value = 0;

	info = devm_kzalloc(&client->dev, sizeof(struct serdes_info), GFP_KERNEL);

        g_info = info;

        info->client = client;

	i2c_set_clientdata(client, info);

	info->serdes_dev.name = "serdes_display";

        ret = switch_dev_register(&info->serdes_dev);
        if (ret < 0)
                printk("Error serdes_switch_dev_register\n");

	info->idtc_dev.name = "idtc_display";

	ret = switch_dev_register(&info->idtc_dev);
        if (ret < 0)
		printk("Error idtc_switch_dev_register\n");

	if(sysfs_create_group(&client->dev.kobj, &serdes_attr_group)) {
		printk("%s [ERROR] sysfs_create_group\n", __func__);
	}

	if (sysfs_create_link(NULL, &client->dev.kobj, "serdes_drv")) {
		printk("%s [ERROR] sysfs_create_link\n", __func__);
	}

	INIT_DELAYED_WORK(&info->serdes.reset_dwork, serdes_reset_dwork);
        serdes_reset_dwork_start(info);

        return 0;
}

static int serdes_remove(struct i2c_client *client)
{
	printk("%s [Start]\n",__func__);
	return 0;
}

static const struct of_device_id serdes_of_match_table[] = {
        { .compatible = "serdes,serdes_recovery", },
        { },
};
MODULE_DEVICE_TABLE(of, serdes_of_match_table);

static const struct i2c_device_id serdes_id[] = {
        {SERDES_DEVICE_NAME, 0},
        {},
};
MODULE_DEVICE_TABLE(i2c, serdes_id);

static struct i2c_driver serdes_driver = {
	.id_table = serdes_id,
	.probe = serdes_probe,
	.remove = serdes_remove,
	.driver = {
		.name   = DRIVER_NAME,
                .owner  = THIS_MODULE,
		.of_match_table = serdes_of_match_table,
	},
};

static int __init serdes_init(void)
{
	int err;
	printk("%s Start \n", __func__);

	err = i2c_add_driver(&serdes_driver);
	if(err)
		printk("serdes driver i2c driver add failed (errno = %d)\n", err);
	return err;
}
module_init(serdes_init);

static void __exit serdes_cleanup(void)
{
}
module_exit(serdes_cleanup);

