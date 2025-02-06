/*
 * MELFAS MIP4 Touchscreen
 *
 * Copyright (C) 2000-2017 MELFAS Inc.
 *
 *
 * mip4_ts.c
 *
 * Version : 2017.05.23
 */

#include "mip4_ts.h"
#include <linux/miscdevice.h>
#undef VERIFY_OCTAL_PERMISSIONS
#define VERIFY_OCTAL_PERMISSIONS(perms) (perms)
/*
 * Start-up run command
 */

void mip4_ts_restart(struct mip4_ts_info *info);
int mip4_ts_startup_run(struct mip4_ts_info *info);
int mip4_ts_config(struct mip4_ts_info *info);
static irqreturn_t mip4_ts_interrupt(int irq, void *dev_id);
int mip4_ts_i2c_read(struct mip4_ts_info *info, char *write_buf, unsigned int write_len, char *read_buf, unsigned int read_len);
static int ser_i2c_bitrate_check(struct i2c_client *client);
static int serdes_line_fault_check(struct i2c_client *client);

static int LCD_TOUCH_RESET_NEED = 0;
static int LCD_DES_RESET_NEED = 0;
static int LCD_DES_SET_ONETIME_CHECK = 1;
static int LCD_TOUCH_INT_CHECK = 0;
static int FW_UPDATE_RESULT = 0;
int MELFAS_LCD_VER = 0;

/* for debugging, enable DEBUG_TRACE */

static DEFINE_SPINLOCK(reset_lock);

int melfas_debug;
int melfas_touch_cnt = 5;
static u8 serdes_check_value[5];
module_param(melfas_debug, int, 0644);
MODULE_PARM_DESC(melfas_debug, "Activate debugging output");

#define MIP4_SERDES_RESET_TIME 3000

struct mip4_ts_info *info_mip4;

ssize_t mip4_set_adm(struct device *dev, const char *buf, size_t count);

static int mip4_touch_reboot_notifier(struct notifier_block *this, unsigned long code, void *unused);

static struct notifier_block mip4_reboot_notifier = {
 .notifier_call =	mip4_touch_reboot_notifier,
};
static int mip4_touch_reboot_notifier(struct notifier_block *this,
                                        unsigned long code, void *unused)
{
        struct device *dev = NULL;
        char buf[32] = {0, };

        printk("%s [START] code : %d\n",__func__,code);
        if (info_mip4 == NULL)
        {
                pr_err("g_ts value is NULL!!! %s, %d\n", __func__, __LINE__);
                return NOTIFY_DONE;
        }
        dev = &info_mip4->client->dev;
        if (dev == NULL)
        {
                pr_err("dev value is NULL!!! %s, %d\n", __func__, __LINE__);
                return NOTIFY_DONE;
        }

        if ((code == SYS_HALT) || (code == SYS_POWER_OFF) || (code == SYS_RESTART)) // need to check HALT or POWER_OFF
        {
                buf[0] = '1';
                if (mip4_set_adm(dev, buf, 1) < 0)
                {
                        pr_err("_set_adm return value is zero!!! %s, %d\n", __func__, __LINE__);
                        return NOTIFY_DONE;
                }
        }
        return NOTIFY_DONE;
}

static int serdes_i2c_reset(struct i2c_client *client, bool des_only)
{
        int ret;
        int i;
        u8 buf[3];
	u8 buf2[3];
        int retry;
        unsigned short addr;

        addr = client->addr;

        printk("-----------%s\n", __func__);

        for(i = 1; i < (sizeof(serdes_1920_720_12_3_config_Si_LG) / sizeof(serdes_config_5)) ;i++) {
                retry = 0;

		client->addr = serdes_1920_720_12_3_config_Si_LG[i].chip_addr;
                buf[0] = (serdes_1920_720_12_3_config_Si_LG[i].reg_addr >> 8) & 0xff;
                buf[1] = serdes_1920_720_12_3_config_Si_LG[i].reg_addr & 0xff;
                buf[2] = serdes_1920_720_12_3_config_Si_LG[i].value;

		if(des_only && (client->addr == SER_ADDRESS))
			continue;

		if(client->addr == DES_DELAY)
                {
			mdelay(buf[2]);
			continue;
                }
		ret = i2c_master_send(client, buf, 3);

		if(ret != 3) {
                        dev_err(&client->dev, "%s: i2c fail\n", __func__);
                        goto out_i2c;
                }
		if(client->addr != DES_DELAY)
		{
			ret = i2c_master_send(client, buf, 2);
			ret = i2c_master_recv(client, buf2, 1);
		}

		if(des_only)
			continue;

		if(client->addr == SER_ADDRESS)
			printk("SER ");
		else if(client->addr == DES_ADDRESS)
			printk("DES ");
		printk("0x%04x write:0x%02x right:0x%02x",serdes_1920_720_12_3_config_Si_LG[i].reg_addr, serdes_1920_720_12_3_config_Si_LG[i].value,serdes_1920_720_12_3_config_Si_LG[i].right_value);
		if(buf2[0] == serdes_1920_720_12_3_config_Si_LG[i].right_value)
			printk(" == ");
		else
			printk(" != ");
		printk("read:0x%02x\n",buf2[0]);
	}
	printk("-----------%s Success\n", __func__);

	LCD_DES_SET_ONETIME_CHECK = 1;

	client->addr = addr;
        return 0;
out_i2c:
        client->addr = addr;
        return 1;
}

static int serdes_i2c_show(struct i2c_client *client)
{
	int ret, i;
        u8 buf1[3];
        u8 buf2[3];
        unsigned short addr;

	addr = client->addr;

        printk("-----------%s\n", __func__);

        for(i = 0; i < (sizeof(serdes_1920_720_12_3_config_Si_LG) / sizeof(serdes_config_5)) ;i++) {

                client->addr = serdes_1920_720_12_3_config_Si_LG[i].chip_addr;
                buf1[0] = (serdes_1920_720_12_3_config_Si_LG[i].reg_addr >> 8) & 0xff;
                buf1[1] = serdes_1920_720_12_3_config_Si_LG[i].reg_addr & 0xff;
                buf1[2] = serdes_1920_720_12_3_config_Si_LG[i].value;

                if(client->addr == DES_DELAY)
                        continue;

                ret = i2c_master_send(client, buf1, 2);
                ret = i2c_master_recv(client, buf2, 1);

                if(client->addr == SER_ADDRESS)
                        printk("SER ");
                else if(client->addr == DES_ADDRESS)
                        printk("DES ");
                printk("0x%04x write:0x%02x right:0x%02x",serdes_1920_720_12_3_config_Si_LG[i].reg_addr, serdes_1920_720_12_3_config_Si_LG[i].value,serdes_1920_720_12_3_config_Si_LG[i].right_value);
                if(buf2[0] == serdes_1920_720_12_3_config_Si_LG[i].right_value)
                        printk(" == ");
                else
                        printk(" != ");
                printk("read:0x%02x\n",buf2[0]);
        }
        printk("-----------%s Success\n", __func__);

        client->addr = addr;
	return 0;
}

static int ser_i2c_bitrate_3gbps_to_6gbps(struct i2c_client *client)
{
        int ret;
        int i;
        u8 buf[3];
        int retry;
        unsigned short addr;

        addr = client->addr;

        printk("-----------%s\n", __func__);

        for(i = 0; i < 3; i++) {
                retry = 0;

		client->addr = ser_6gbps_config[i][0];
                buf[0] = (ser_6gbps_config[i][1] >> 8) & 0xff;
                buf[1] = ser_6gbps_config[i][1] & 0xff;
                buf[2] = ser_6gbps_config[i][2] & 0xff;

                if(client->addr == DES_DELAY)
                {
	                mdelay(buf[2]);
                        continue;
                }
                ret =  i2c_master_send(client, buf, 3);
        }
        printk("-----------%s Success\n", __func__);
        client->addr = addr;
        return 0;
}
static int des_i2c_bitrate_3gbps_to_6gbps(struct i2c_client *client)
{
        int ret;
        int i;
        u8 buf[3];
        int retry;
        unsigned short addr;

        addr = client->addr;

        printk("-----------%s\n", __func__);

        for(i = 0; i < 3; i++) {
                retry = 0;

                client->addr = des_6gbps_config[i][0];
                buf[0] = (des_6gbps_config[i][1] >> 8) & 0xff;
                buf[1] = des_6gbps_config[i][1] & 0xff;
                buf[2] = des_6gbps_config[i][2] & 0xff;

                if(client->addr == DES_DELAY)
                {
                        mdelay(buf[2]);
                        continue;
                }
                ret =  i2c_master_send(client, buf, 3);
        }
        printk("-----------%s Success\n", __func__);
        client->addr = addr;
        return 0;
}
static int serdes_i2c_bitrate_6gbps_to_3gbps(struct i2c_client *client)
{
        int ret;
        int i;
        u8 buf[3];
        int retry;
        unsigned short addr;

        addr = client->addr;

        printk("-----------%s\n", __func__);

        for(i = 0; i < 4; i++) {
                retry = 0;

                client->addr = serdes_3gbps_config[i][0];
                buf[0] = (serdes_3gbps_config[i][1] >> 8) & 0xff;
                buf[1] = serdes_3gbps_config[i][1] & 0xff;
                buf[2] = serdes_3gbps_config[i][2] & 0xff;

                if(client->addr == DES_DELAY)
                {
                        mdelay(buf[2]);
                        continue;
                }
                ret =  i2c_master_send(client, buf, 3);
        }
        printk("-----------%s Success\n", __func__);
        client->addr = addr;
        return 0;
}

static int i2c_set_check(struct i2c_client *client, serdes_config_5 *reglist, int size)
{
        int ret;
        int i;
        u8 buf[2];
        u8 buf2[3];
        int check_ret;
        unsigned short addr;
	unsigned int serdes_config_size = sizeof(serdes_config_5);
	int count = 0;

	if((serdes_config_size != 0) && ((size / serdes_config_size) <= INT_MAX))
        {
                count = size / serdes_config_size;
        }
	else
	{
		printk("%s serdes_config_size : %d Error",__func__,serdes_config_size);
                return -1;
        }

        addr = client->addr;

        check_ret = 0;

        for(i = 0; i < count; i++)
        {
                client->addr = reglist[i].chip_addr;
                buf[0] = (reglist[i].reg_addr >> 8) & 0xff;
                buf[1] = reglist[i].reg_addr & 0xff;

                ret = i2c_master_send(client, buf, 2);
                ret = i2c_master_recv(client, buf2, 1);

                if(ret != 1)
                {
                        printk("%s : %s Register(0x%04x) i2c fail\n",__func__,((client->addr == SER_ADDRESS) ? "SER" : "DES") , reglist[i].reg_addr);
                        continue;
                }

                if((buf2[0]&reglist[i].value) != reglist[i].right_value)
                {
                        printk("%s : %s Register(0x%04x) Value(0x%02x) != Read(0x%02x)(Raw : 0x%02x)\n",__func__,((client->addr == SER_ADDRESS) ? "SER" : "DES") ,reglist[i].reg_addr,reglist[i].right_value, buf2[0]&reglist[i].value, buf2[0]);
                        if(check_ret < count)
				check_ret++;
                }
                else if(false)
                {
                        printk("%s : %s Register(0x%04x) Value(0x%02x) == Read(0x%02x)(Raw : 0x%02x)\n",__func__,((client->addr == SER_ADDRESS) ? "SER" : "DES") ,reglist[i].reg_addr,reglist[i].right_value, buf2[0]&reglist[i].value, buf2[0]);
                }
        }

        client->addr = addr;

        return check_ret;
}

static int serdes_i2c_set_check(struct i2c_client *client)
{
        return i2c_set_check(client, serdes_set_check, sizeof(serdes_set_check));
}

static int des_i2c_set_check(struct i2c_client *client)
{
        int lcd_type;

        serdes_config_5 *reglist;

	reglist = des_oldi_set_check_1920_720_12_3_Si_LG;

        LCD_DES_SET_ONETIME_CHECK = 0;

        return (i2c_set_check(client, des_gpio_set_check, sizeof(des_gpio_set_check)) +
                 i2c_set_check(client, reglist, sizeof(reglist)));
}

static int ser_i2c_bitrate_check(struct i2c_client *client)
{
        int ret;
        int i;
        u8 buf[2];
        u8 buf2[3];
        int retry;
        unsigned short addr;
        struct mip4_ts_info *info = i2c_get_clientdata(client);

        addr = client->addr;

	client->addr = ser_bitrate_check[0][0];
        buf[0] = (ser_bitrate_check[0][1] >> 8) & 0xff;
        buf[1] = ser_bitrate_check[0][1] & 0xff;

	ret = i2c_master_send(client, buf, 2);
        ret = i2c_master_recv(client, buf2, 1);

	printk("%s Ser bitrate = 0x%02x\n",__func__,buf2[0]);

        client->addr = addr;
        return buf2[0];
}

static int des_i2c_bitrate_check(struct i2c_client *client)
{
        int ret;
        int i;
        u8 buf[2];
        u8 buf2[3];
        int retry;
        unsigned short addr;
        struct mip4_ts_info *info = i2c_get_clientdata(client);

        addr = client->addr;

	client->addr = des_bitrate_check[0][0];
        buf[0] = (des_bitrate_check[0][1] >> 8) & 0xff;
        buf[1] = des_bitrate_check[0][1] & 0xff;

        ret = i2c_master_send(client, buf, 2);
        ret = i2c_master_recv(client, buf2, 1);

	printk("%s Des bitrate = 0x%02x\n",__func__,buf2[0]);

        client->addr = addr;
        return buf2[0];
}

static int serdes_line_fault_check(struct i2c_client *client)
{
	int ret;
	int check_ret;
        u8 buf[3];
        u8 buf2[3];
        int retry;
        unsigned short addr;
        struct mip4_ts_info *info = i2c_get_clientdata(client);

        addr = client->addr;

	client->addr = serdes_line_fault_config[0][0];
        buf[0] = (serdes_line_fault_config[0][1] >> 8) & 0xff;
        buf[1] = serdes_line_fault_config[0][1] & 0xff;
	buf[2] = serdes_line_fault_config[0][2] & 0xff;

	ret = i2c_master_send(client, buf, 3);

	mdelay(20);

	ret = i2c_master_send(client, buf, 2);
        ret = i2c_master_recv(client, buf2, 1);

	printk("%s 0x%02x -> 0x%02x\n",__func__, buf2[0], buf2[0]&0x0C);
	check_ret = buf2[0];

	buf[2] = serdes_line_fault_config[1][2] & 0xff;
	
	ret = i2c_master_send(client, buf, 3);

	client->addr = serdes_line_fault_config[2][0];
        buf[0] = (serdes_line_fault_config[2][1] >> 8) & 0xff;
        buf[1] = serdes_line_fault_config[2][1] & 0xff;

	ret = i2c_master_send(client, buf, 2);
        ret = i2c_master_recv(client, buf2, 1);

	//printk("%s check_ret : %d\n",__func__, check_ret);

	client->addr = addr;

	return check_ret;
}

static int serdes_i2c_connect(struct i2c_client *client)
{
	int ret;
	int i;
	u8 buf[3];
	u8 buf2[3];
	int retry;
	unsigned short addr;
	struct mip4_ts_info *info = i2c_get_clientdata(client);

	addr = client->addr;

	client->addr = serdes_link_lock_config[0][0];
	buf[0] = (serdes_link_lock_config[0][1] >> 8) & 0xff;
	buf[1] = serdes_link_lock_config[0][1] & 0xff;

	ret = i2c_master_send(client, buf, 2);
	ret = i2c_master_recv(client, buf2, 1);

	if((buf2[0]&0xfa)!=0xda) {
		printk(KERN_ERR "---------%s: i2c read  (%d), {0x%02x, 0x%02x}\n", __func__, ret, client->addr, buf2[0]);
	}

	client->addr = addr;

	if(ret != 1)
		return -1;

	return buf2[0];
}

static void serdes_video_signal_print(int lock)
{
	int i;

	for(i = 0; i < (sizeof(serdes_video_signal_config) / sizeof(serdes_config_5)) ; i++)
        {
		if(((lock&0xfa)==0xda) || (serdes_video_signal_config[i].chip_addr == SER_ADDRESS))
			printk(KERN_ERR "%s : {%s register : 0x%04x read : 0x%02x}\n", __func__, ((serdes_video_signal_config[i].chip_addr == SER_ADDRESS) ? "SER" : "DES"), serdes_video_signal_config[i].reg_addr, serdes_check_value[i]);
	}

	return;
}

static int serdes_video_signal_check(struct i2c_client *client, int lock_bit)
{
        int ret;
        int i;
        u8 buf[3];
        u8 buf2[3];
        int retry;
        unsigned short addr;
	int need_log = 0;
        struct mip4_ts_info *info = i2c_get_clientdata(client);

        addr = client->addr;

	for(i = 0; i < (sizeof(serdes_video_signal_config) / sizeof(serdes_config_5)) ; i++)
	{
		if((serdes_video_signal_config[i].chip_addr == DES_ADDRESS) && ((lock_bit&0xfa) != 0xda) && (lock_bit != 0))
			continue;

	        client->addr = serdes_video_signal_config[i].chip_addr;
	        buf[0] = (serdes_video_signal_config[i].reg_addr >> 8) & 0xff;
	        buf[1] = serdes_video_signal_config[i].reg_addr & 0xff;

	        ret = i2c_master_send(client, buf, 2);
	        ret = i2c_master_recv(client, buf2, 1);

		serdes_check_value[i] = buf2[0];

		if(PRINT_SERDES_LOG || (lock_bit == 0))
		{
			printk(KERN_ERR "%s : ret (%d), {%s register : 0x%04x read : 0x%02x}\n", __func__, ret, ((client->addr == SER_ADDRESS) ? "SER" : "DES"), serdes_video_signal_config[i].reg_addr, buf2[0]);
		}

		if(((serdes_video_signal_config[i].value == 0xFF) ? (buf2[0] >= serdes_video_signal_config[i].right_value) : ((buf2[0]&serdes_video_signal_config[i].value) != serdes_video_signal_config[i].right_value)))
		{
			need_log = 1;
		}
	}

	if(PRINT_SERDES_LOG)
		PRINT_SERDES_LOG = 0;

        client->addr = addr;

        return need_log;
}

inline void mip4_serdes_reset_dwork_stop(struct mip4_ts_info *info)
{
	cancel_delayed_work(&info->serdes.reset_dwork);
}

inline void mip4_serdes_reset_dwork_start(struct mip4_ts_info *info)
{
	mip4_serdes_reset_dwork_stop(info);
	schedule_delayed_work(&info->serdes.reset_dwork,
						  msecs_to_jiffies(MIP4_SERDES_RESET_TIME));
}

static int mip4_serdes_reset(struct mip4_ts_info *info)
{
	int lock_pin = 0;
	int line_fault = -1;
        int ret = 0, i = 0;
        int retry = I2C_RETRY_COUNT;
	int ser_bitrate = 0;
        int des_bitrate = 0;
	int need_log = 0;
	struct i2c_client *client = info->client;
	int lock_pin_begin_value = 0;

        mutex_lock(&info->serdes.lock);

	if(info->irq_enabled) {
                disable_irq(info->client->irq);
                info->irq_enabled = false;
        }

        lock_pin = serdes_i2c_connect(client);
	lock_pin_begin_value = lock_pin;

	if(lock_pin == -1)
	{
		LCD_DES_RESET_NEED = 2;
		printk("Ser I2C Error Occur : LCD_DES_RESET_NEED set %d\n",LCD_DES_RESET_NEED);
	}

	need_log = serdes_video_signal_check(client, lock_pin);

	if(((lock_pin&0xfa) == 0xda) && (LCD_DES_RESET_NEED == 2))
        {
                LCD_DES_RESET_NEED = 0;
                printk("LCD_DES_RESET_NEED set 0\n");
        }

	// Check Des Register Set
        if((!LCD_DES_RESET_NEED) && ((lock_pin&0xfa) == 0xda))
	{
		if(serdes_i2c_set_check(client) != 0)
		{
			LCD_DES_RESET_NEED = 1;
			printk("LCD_DES_RESET_NEED set because setting not complete%d\n", LCD_DES_RESET_NEED);
                }
		else if(LCD_DES_SET_ONETIME_CHECK && (des_i2c_set_check(client) != 0))
		{
			LCD_DES_RESET_NEED = 1;
			printk("LCD_DES_RESET_NEED set because wrong setting value%d\n", LCD_DES_RESET_NEED);
		}
	}

	// Des Reset
        if(((lock_pin&0xfa)==0xda) && (LCD_DES_RESET_NEED == 1))
	{
                printk("---------%s: DISPLAY & TouchIC Recovery !!! \n", __func__);
                printk("%s Lock pin :0x[%x]!!!\r\n", __func__, lock_pin);

                ret = serdes_i2c_reset(client,false);

		if(ret)
		{
			LCD_DES_RESET_NEED = 1;
			LCD_TOUCH_INT_CHECK = 2;
			printk("%s [ERROR] serdes_i2c_reset\n", __func__);
		}
		else
		{
			ser_bitrate = ser_i2c_bitrate_check(client);

			if(ser_bitrate != 0x84)
			{
				serdes_i2c_bitrate_6gbps_to_3gbps(client);
                                mdelay(100);

				ser_bitrate = ser_i2c_bitrate_check(client);
                                des_bitrate = des_i2c_bitrate_check(client);

				if((ser_bitrate == 0x84) && (des_bitrate == 0x01))
					printk("%s DES Set 3Gbps\n",__func__);
				else
				{
					printk("%s DES Doesn't Set 3Gbps SER : 0x%x DES : 0x%x\n",__func__, ser_bitrate, des_bitrate);
				}
			}

	                mip4_ts_clear_input(info);

	                mip4_ts_restart(info);

        	        while(retry--) {
	                        ret = mip4_ts_config(info);

        	                if(ret == 0)
	                                break;
	                        msleep(200);
        	                printk("msleep(200)\n");
	                }

	                if (ret) {
	                        LCD_DES_RESET_NEED = 1;
	                        LCD_TOUCH_INT_CHECK = 2; //prevent touch reset again right after serdes reset
	                        printk("%s [ERROR] mip4_ts_config\n", __func__);
	                }
	                else {
	                        LCD_DES_RESET_NEED = 0;
				LCD_TOUCH_INT_CHECK = 2;
				printk("%s LCD_DES_RESET_NEED set %d\n", __func__, LCD_DES_RESET_NEED);
	                }

	                mip4_ts_config_input(info);
		}
        }
        else if(((lock_pin)&0xfa)!=0xda && lock_pin != -1)
	{
		line_fault = serdes_line_fault_check(client);
		if(line_fault != 0x2E)
		{
			printk("---------%s: Line Fault OK !!! \n", __func__);

			ser_bitrate = ser_i2c_bitrate_check(client);

			if(ser_bitrate==0x84)
			{
				ser_i2c_bitrate_3gbps_to_6gbps(client);
				ser_bitrate = ser_i2c_bitrate_check(client);
                                des_bitrate = des_i2c_bitrate_check(client);

				lock_pin = serdes_i2c_connect(client);

				if(((lock_pin)&0xfa)==0xda)
				{
					if(serdes_i2c_set_check(client)!=0)
						serdes_i2c_reset(client,false);

					serdes_i2c_bitrate_6gbps_to_3gbps(client);

					mdelay(100);

					ser_bitrate = ser_i2c_bitrate_check(client);
					des_bitrate = des_i2c_bitrate_check(client);

					if(des_bitrate != 0x01)
					{
						printk("%s DES Doesn't Set 3Gbps\n",__func__);
					}
					else
					{
						printk("%s DES Set 3Gbps\n",__func__);

						mip4_ts_clear_input(info);

						mip4_ts_restart(info);

						while(retry--) {
							ret = mip4_ts_config(info);

							if(ret == 0)
								break;
							msleep(200);
							printk("msleep(200)\n");
						}

						if (ret) {
				                        LCD_TOUCH_INT_CHECK = 2; //prevent touch reset again right after serdes reset
				                        printk("%s [ERROR] mip4_ts_config\n", __func__);
			        	        }
				                else {
				                        LCD_DES_RESET_NEED = 0;
							LCD_TOUCH_INT_CHECK = 2;
							printk("%s LCD_DES_RESET_NEED set %d\n", __func__, LCD_DES_RESET_NEED);
						}

						mip4_ts_config_input(info);
					}
				}
			}
			else
			{
				serdes_i2c_bitrate_6gbps_to_3gbps(client);
				ser_bitrate = ser_i2c_bitrate_check(client);
				des_bitrate = des_i2c_bitrate_check(client);

				lock_pin = serdes_i2c_connect(client);

                                if(((lock_pin)&0xfa)==0xda)
                                {
					if(serdes_i2c_set_check(client)!=0)
						serdes_i2c_reset(client,false);

					if(des_bitrate != 0x01)
					{
						printk("%s DES Doesn't Set 3Gbps\n",__func__);
					}
					else
					{
						printk("%s DES Set 3Gbps\n",__func__);

						mip4_ts_clear_input(info);

						mip4_ts_restart(info);

						while(retry--) {
							ret = mip4_ts_config(info);

							if(ret == 0)
								break;
							msleep(200);
							printk("msleep(200)\n");
						}

						if (ret) {
							LCD_TOUCH_INT_CHECK = 2; //prevent touch reset again right after serdes reset
							printk("%s [ERROR] mip4_ts_config\n", __func__);
						}
						else {
							LCD_DES_RESET_NEED = 0;
							LCD_TOUCH_INT_CHECK = 2;
							printk("%s LCD_DES_RESET_NEED set %d\n", __func__, LCD_DES_RESET_NEED);
						}
						mip4_ts_config_input(info);
					}
				}
			}
		}
		else
		{
			printk("---------%s: Line Fault ERROR !!! \n", __func__);
			if(LCD_DES_RESET_NEED != 2)
			{
				LCD_DES_RESET_NEED = 2;
				printk("LCD_DES_RESET_NEED set %d\n",LCD_DES_RESET_NEED);
			}
		}
        }

	// Serdes Connect Dignosis
	if((lock_pin&0xfa)==0xda)
	{
		set_serdes_conn_check(0);
	}
	else
	{
		set_serdes_conn_check(2);
		need_log = 1;
        }

	if(need_log)
	{
		serdes_video_signal_print(lock_pin);
		mobis_serdes_need_log();
		if(line_fault < 0 && lock_pin != -1)
			line_fault = serdes_line_fault_check(client);
		mobis_serdes_status_count_update(serdes_check_value, lock_pin_begin_value, line_fault);
		mobis_idtc_check_write(serdes_check_value, lock_pin_begin_value, line_fault, false, false);
	}

	if(!info->irq_enabled) {
                enable_irq(info->client->irq);
                info->irq_enabled = true;
        }

        mutex_unlock(&info->serdes.lock);

	return lock_pin;
}

static void mip4_touch_reset(struct mip4_ts_info *info)
{
	int ret = 0;
	int retry = I2C_RETRY_COUNT;

	if(LCD_TOUCH_INT_CHECK > 0) {
		spin_lock_irq(&reset_lock);
		LCD_TOUCH_INT_CHECK = 0;
		LCD_TOUCH_RESET_NEED = 0;
		spin_unlock_irq(&reset_lock);
	}
	else {
		LCD_TOUCH_RESET_NEED = 1;
	}

	if(LCD_TOUCH_RESET_NEED == 1) {
		dev_info(&info->client->dev, "%s LCD_TOUCH_RESET [START]", __func__);

		if(info->irq_enabled) {
			disable_irq(info->client->irq);
			info->irq_enabled = false;
		}

		mip4_ts_clear_input(info);

		mip4_ts_restart(info);

		while(retry--) {
			ret = mip4_ts_config(info);

			if(ret == 0)
				break;
			msleep(200);
			printk("msleep(200)\n");
		}

		if(ret != 0) {
			dev_err(&info->client->dev, "%s [ERROR]\n", __func__);
		}
		else
		{
			mip4_ts_config_input(info);

			LCD_TOUCH_RESET_NEED = 0;
			LCD_TOUCH_INT_CHECK = 0;

			dev_info(&info->client->dev, "%s LCD_TOUCH_RESET [DONE]", __func__);
		}

		if(!info->irq_enabled) {
			enable_irq(info->client->irq);
			info->irq_enabled = true;
		}

		mobis_idtc_check_write(serdes_check_value, 0, 0, true, false);
	}
	else
	{
		mobis_idtc_check_write(serdes_check_value, 0, 0, false, false);
	}

}

static void mip4_serdes_reset_dwork(struct work_struct *work)
{
	struct input_dev *dev;
	struct mip4_ts_info *info = container_of(work, struct mip4_ts_info, serdes.reset_dwork);

	if(!get_montype())
	{
		if((mip4_serdes_reset(info)&0xfa) == 0xda)
		{
			mip4_touch_reset(info);
		}
	}
	else
		mip4_touch_reset(info);

	mip4_serdes_reset_dwork_start(info);
}

static void mip4_fw_update_dwork(struct work_struct *work)
{
        struct input_dev *dev;
        struct mip4_ts_info *info = container_of(work, struct mip4_ts_info, upgrade_work);
	int ret = -1;
	int val;

	mip4_serdes_reset_dwork_stop(info);

	MELFAS_LCD_VER = daudio_lcd_version();

        if(!get_montype() && (MELFAS_LCD_VER != DAUDIOKK_LCD_OI_DISCONNECTED))
        {
		if(info->irq_enabled) {
	                disable_irq(info->client->irq);
	                info->irq_enabled = false;
	        }

		val = serdes_i2c_connect(info->client);

		if(!info->irq_enabled) {
	                enable_irq(info->client->irq);
	                info->irq_enabled = true;
	        }
		if((val&0xfa) == 0xda)
		{
			if(mobis_touch_update_check())
			{
				mip4_ts_power_on(info);
				ret = mip4_ts_fw_update_from_kernel(info, false);

				mip4_ts_config(info);
				mip4_ts_config_input(info);
			}
		}
        }
        else if(MELFAS_LCD_VER != DAUDIOKK_LCD_OI_DISCONNECTED)
	{
		if(mobis_touch_update_check())
		{
			mip4_ts_power_on(info);
			ret = mip4_ts_fw_update_from_kernel(info, false);

			mip4_ts_config(info);
			mip4_ts_config_input(info);
		}
	}

	if((ret == fw_err_uptodate) || (ret == fw_err_none))
        {
		mobis_touch_update_complete();
	}

	LCD_TOUCH_INT_CHECK = 2;

	mip4_serdes_reset_dwork_start(info);
}

int mip4_ts_startup_run(struct mip4_ts_info *info)
{
	u8 wbuf[4];

	dev_dbg(&info->client->dev, "%s [START]\n", __func__);

	wbuf[0] = MIP4_R0_CTRL;
	wbuf[1] = MIP4_R1_CTRL_RUN;
	wbuf[2] = 1;
	if (mip4_ts_i2c_write(info, wbuf, 3)) {
		dev_err(&info->client->dev, "%s [ERROR] mip4_ts_i2c_write\n", __func__);
		goto error;
	}

	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);
	return 0;

error:
	dev_err(&info->client->dev, "%s [ERROR]\n", __func__);
	return 1;
}

/*
 * Start-up process
 */
int mip4_ts_startup(struct mip4_ts_info *info)
{
	int ret = 0;

	dev_dbg(&info->client->dev, "%s [START]\n", __func__);

#if 0
	reinit_completion(&info->startup_done);
	dev_dbg(&info->client->dev, "%s - startup : reinit_completion\n", __func__);
#else
	init_completion(&info->startup_done);
	dev_dbg(&info->client->dev, "%s - startup : init_completion\n", __func__);
#endif

	mip4_ts_power_on(info);
	msleep(30);

	if (!info->irq_enabled) {
		enable_irq(info->client->irq);
		info->irq_enabled = true;
	}

	/* Wait start-up interrupt */
	dev_dbg(&info->client->dev, "%s - startup : wait_for_completion_interruptible_timeout[%d]\n", __func__, STARTUP_TIMEOUT);
	ret = wait_for_completion_interruptible_timeout(&info->startup_done, msecs_to_jiffies(STARTUP_TIMEOUT));
	if (ret > 0) {
		dev_dbg(&info->client->dev, "%s - startup : completed[%d]\n", __func__, jiffies_to_msecs(ret));
	}
	else {
		dev_dbg(&info->client->dev, "%s - startup : timeout[%d]\n", __func__, ret);

#if USE_FW_RECOVERY
		/* Restore firmware */
		dev_dbg(&info->client->dev, "%s - flash_fail_type : critical\n", __func__);
		ret = mip4_ts_fw_restore_critical_section(info);
		if (ret < 0) {
			dev_err(&info->client->dev, "%s [ERROR] mip4_ts_fw_restore_critical_section\n", __func__);
		}
#endif /* USE_FW_RECOVERY */
	}

	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);
	return 0;
}

/*
 * Reset chip
 */
void mip4_ts_reset(struct mip4_ts_info *info)
{
	dev_dbg(&info->client->dev, "%s [START]\n", __func__);

	mip4_ts_power_off(info);

	mip4_ts_power_on(info);

	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);
}

/*
 * Restart chip
 */
void mip4_ts_restart(struct mip4_ts_info *info)
{
	dev_dbg(&info->client->dev, "%s [START]\n", __func__);

	mip4_ts_power_off(info);

#if USE_STARTUP_WAITING
	mip4_ts_startup(info);
#else
	mip4_ts_power_on(info);
#endif /* USE_STARTUP_WAITING */

	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);
}

/*
 * I2C Read
 */
int mip4_ts_i2c_read(struct mip4_ts_info *info, char *write_buf, unsigned int write_len, char *read_buf, unsigned int read_len)
{
	int retry = I2C_RETRY_COUNT;
	int res;

	struct i2c_msg msg[] = {
		{
			.addr = info->client->addr,
			.flags = 0,
			.buf = write_buf,
			.len = write_len,
		}, {
			.addr = info->client->addr,
			.flags = I2C_M_RD,
			.buf = read_buf,
			.len = read_len,
		},
	};

	while (retry--) {
		res = i2c_transfer(info->client->adapter, msg, ARRAY_SIZE(msg));

		if (res == ARRAY_SIZE(msg)) {
			goto exit;
		}
		else if (res < 0) {
			dev_err(&info->client->dev, "%s [ERROR] i2c_transfer - errno[%d]\n", __func__, res);
		}
		else {
			dev_err(&info->client->dev, "%s [ERROR] i2c_transfer - size[%zu] result[%d]\n", __func__, ARRAY_SIZE(msg), res);
		}
	}

	goto error_reboot;

error_reboot:
#if RESET_ON_I2C_ERROR
	//mip4_ts_restart(info);
#endif /* RESET_ON_I2C_ERROR */
	return 1;

exit:
	return 0;
}

/*
 * I2C Write
 */
int mip4_ts_i2c_write(struct mip4_ts_info *info, char *write_buf, unsigned int write_len)
{
	int retry = I2C_RETRY_COUNT;
	int res;
	printk("%s\n", __func__);

	while (retry--) {
		res = i2c_master_send(info->client, write_buf, write_len);

		if (res == write_len) {
			goto mip4_exit;
		}
		else if (res < 0) {
			dev_err(&info->client->dev, "%s [ERROR] i2c_master_send - errno [%d]\n", __func__, res);
		}
		else {
			dev_err(&info->client->dev, "%s [ERROR] i2c_master_send - write[%d] result[%d]\n", __func__, write_len, res);
		}
	}

	goto error_reboot;

error_reboot:
#if RESET_ON_I2C_ERROR
	//mip4_ts_restart(info);
#endif /* RESET_ON_I2C_ERROR */
	return 1;

mip4_exit:
	return 0;
}

/*
 * Enable device
 */
int mip4_ts_enable(struct mip4_ts_info *info)
{
	dev_dbg(&info->client->dev, "%s [START]\n", __func__);

	if (info->enabled) {
		dev_err(&info->client->dev, "%s [ERROR] device already enabled\n", __func__);
		goto mip4_exit;
	}

	mutex_lock(&info->lock);

#if USE_WAKEUP_GESTURE
	mip4_ts_set_power_state(info, MIP4_CTRL_POWER_ACTIVE);

	if (wake_lock_active(&info->wake_lock)) {
		wake_unlock(&info->wake_lock);
		dev_dbg(&info->client->dev, "%s - wake_unlock\n", __func__);
	}

	info->gesture_wakeup_mode = false;
	dev_dbg(&info->client->dev, "%s - gesture wake-up mode : off\n", __func__);
#else
#if USE_STARTUP_WAITING
	mip4_ts_startup(info);
#else
	if(info->init) {
		info->init = false;
	}
	else {
		mip4_ts_power_on(info);
	}


	if (!info->irq_enabled) {
		enable_irq(info->client->irq);
		info->irq_enabled = true;
	}
#endif /* USE_STARTUP_WAITING */
#endif /* USE_WAKEUP_GESTURE */

	info->enabled = true;
	info->i2c_error_cnt = 0;

	mutex_unlock(&info->lock);

mip4_exit:
	dev_info(&info->client->dev, MIP4_TS_DEVICE_NAME" - Enabled\n");

	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);
	return 0;
}

/*
 * Disable device
 */
int mip4_ts_disable(struct mip4_ts_info *info)
{
	dev_dbg(&info->client->dev, "%s [START]\n", __func__);

	if (!info->enabled) {
		dev_err(&info->client->dev, "%s [ERROR] device already disabled\n", __func__);
		goto mip4_exit;
	}

	mutex_lock(&info->lock);

#if USE_WAKEUP_GESTURE
	info->wakeup_gesture_code = 0;

	mip4_ts_set_wakeup_gesture_type(info, MIP4_EVENT_GESTURE_ALL);
	mip4_ts_set_power_state(info, MIP4_CTRL_POWER_LOW);

	info->gesture_wakeup_mode = true;
	dev_dbg(&info->client->dev, "%s - gesture wake-up mode : on\n", __func__);

	if (!wake_lock_active(&info->wake_lock)) {
		wake_lock(&info->wake_lock);
		dev_dbg(&info->client->dev, "%s - wake_lock\n", __func__);
	}
#else
	disable_irq(info->client->irq);
	info->irq_enabled = false;

	mip4_ts_power_off(info);
#endif /* USE_WAKEUP_GESTURE */

	mip4_ts_clear_input(info);

	info->enabled = false;

	mutex_unlock(&info->lock);

mip4_exit:
	dev_info(&info->client->dev, MIP4_TS_DEVICE_NAME" - Disabled\n");

	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);
	return 0;
}

#if USE_FB_NOTIFY
/*
 * FB notifier callback
 */
static int mip4_ts_fb_notifier_callback(struct notifier_block *self, unsigned long event, void *data)
{
	struct mip4_ts_info *info = container_of(self, struct mip4_ts_info, fb_notifier);
	struct fb_event *event_data = data;
	unsigned int blank;

	dev_dbg(&info->client->dev, "%s [START]\n", __func__);

	/* Check event */
	//if (event != FB_EARLY_EVENT_BLANK) {
	if (event != FB_EVENT_BLANK) {
		dev_dbg(&info->client->dev, "%s - skip event [0x%02X]\n", __func__, event);
		goto mip4_exit;
	}

	/* Check blank */
	blank = *(int *)event_data->data;

	dev_dbg(&info->client->dev, "%s - blank[0x%02X]\n", __func__, blank);

	switch (blank) {
		case FB_BLANK_UNBLANK:
			/* Enable */
			dev_dbg(&info->client->dev, "%s - FB_BLANK_UNBLANK\n", __func__);
			dev_info(&info->client->dev, "%s - Display : On\n", __func__);

			break;

		case FB_BLANK_POWERDOWN:
			/* Disable */
			dev_dbg(&info->client->dev, "%s - FB_BLANK_POWERDOWN\n", __func__);
			dev_info(&info->client->dev, "%s - Display : Off\n", __func__);

			break;

		default:
			dev_dbg(&info->client->dev, "%s - skip blank [0x%02X]\n", __func__, blank);
			break;
	}

mip4_exit:
	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);
	return 0;
}
#endif /* USE_FB_NOTIFY */

#if USE_INPUT_OPEN_CLOSE
/*
 * Open input device
 */
static int mip4_ts_input_open(struct input_dev *dev)
{
	struct mip4_ts_info *info = input_get_drvdata(dev);

	dev_dbg(&info->client->dev, "%s [START]\n", __func__);

	if (info->init == true) {
		info->init = false;
	}
	else {
		mip4_ts_enable(info);
	}

	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);
	return 0;
}

/*
 * Close input device
 */
static void mip4_ts_input_close(struct input_dev *dev)
{
	struct mip4_ts_info *info = input_get_drvdata(dev);

	dev_dbg(&info->client->dev, "%s [START]\n", __func__);

	mip4_ts_disable(info);

	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);
}
#endif /* USE_INPUT_OPEN_CLOSE */

/*
 * Get ready status
 */
int mip4_ts_get_ready_status(struct mip4_ts_info *info)
{
	u8 wbuf[16];
	u8 rbuf[16];
	int ret = 0;

	//dev_dbg(&info->client->dev, "%s [START]\n", __func__);

	wbuf[0] = MIP4_R0_CTRL;
	wbuf[1] = MIP4_R1_CTRL_READY_STATUS;
	if (mip4_ts_i2c_read(info, wbuf, 2, rbuf, 1)) {
		dev_err(&info->client->dev, "%s [ERROR] mip4_ts_i2c_read\n", __func__);
		goto error;
	}
	ret = rbuf[0];

	/* Check status */
	if ((ret == MIP4_CTRL_STATUS_NONE) || (ret == MIP4_CTRL_STATUS_LOG) || (ret == MIP4_CTRL_STATUS_READY)) {
		//dev_dbg(&info->client->dev, "%s - status[0x%02X]\n", __func__, ret);
	}
	else {
		dev_err(&info->client->dev, "%s [ERROR] Unknown status[0x%02X]\n", __func__, ret);
		goto error;
	}

	if (ret == MIP4_CTRL_STATUS_LOG) {
		/* Skip log event */
		wbuf[0] = MIP4_R0_LOG;
		wbuf[1] = MIP4_R1_LOG_TRIGGER;
		wbuf[2] = 0;
		if (mip4_ts_i2c_write(info, wbuf, 3)) {
			dev_err(&info->client->dev, "%s [ERROR] mip4_ts_i2c_write\n", __func__);
		}
	}

	//dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);
	return ret;

error:
	dev_err(&info->client->dev, "%s [ERROR]\n", __func__);
	return 1;
}

/*
 * Read chip firmware version
 */
int mip4_ts_get_fw_version(struct mip4_ts_info *info, u8 *ver_buf)
{
	u8 rbuf[8];
	u8 wbuf[2];
	int i;

	wbuf[0] = MIP4_R0_INFO;
	wbuf[1] = MIP4_R1_INFO_VERSION_BOOT;
	if (mip4_ts_i2c_read(info, wbuf, 2, rbuf, 8)) {
		goto error;
	};

	for (i = 0; i < FW_MAX_SECT_NUM; i++) {
		ver_buf[0 + (i * 2)] = rbuf[1 + (i * 2)];
		ver_buf[1 + (i * 2)] = rbuf[0 + (i * 2)];
	}

	return 0;

error:
	for (i = 0; i < (FW_MAX_SECT_NUM * 2); i++) {
		ver_buf[i] = 0xFF;
	}

	dev_err(&info->client->dev, "%s [ERROR]\n", __func__);
	return 1;
}

/*
 * Read chip firmware version for u16
 */
int mip4_ts_get_fw_version_u16(struct mip4_ts_info *info, u16 *ver_buf_u16)
{
	u8 rbuf[8];
	int i;

	if (mip4_ts_get_fw_version(info, rbuf)) {
		goto error;
	}

	for (i = 0; i < FW_MAX_SECT_NUM; i++) {
		ver_buf_u16[i] = (rbuf[0 + (i * 2)] << 8) | rbuf[1 + (i * 2)];
	}

	return 0;

error:
	for (i = 0; i < FW_MAX_SECT_NUM; i++) {
		ver_buf_u16[i] = 0xFFFF;
	}

	dev_err(&info->client->dev, "%s [ERROR]\n", __func__);
	return 1;
}

#if (CHIP_MODEL != CHIP_NONE)
/*
 * Read bin(file) firmware version
 */
int mip4_ts_get_fw_version_from_bin(struct mip4_ts_info *info, u8 *ver_buf)
{
	const struct firmware *fw;
	int i;

	dev_dbg(&info->client->dev, "%s [START]\n", __func__);

	request_firmware(&fw, FW_PATH_INTERNAL, &info->client->dev);

	if (!fw) {
		dev_err(&info->client->dev, "%s [ERROR] request_firmware\n", __func__);
		goto error;
	}

	if (mip4_ts_bin_fw_version(info, fw->data, fw->size, ver_buf)) {
		for (i = 0; i < (FW_MAX_SECT_NUM * 2); i++) {
			ver_buf[i] = 0xFF;
		}
		dev_err(&info->client->dev, "%s [ERROR] mip4_ts_bin_fw_version\n", __func__);
		goto error;
	}

	release_firmware(fw);

	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);
	return 0;

error:
	dev_err(&info->client->dev, "%s [ERROR]\n", __func__);
	return 1;
}
#endif /* CHIP_MODEL */

/*
 * Set power state
 */
int mip4_ts_set_power_state(struct mip4_ts_info *info, u8 mode)
{
	u8 wbuf[3];

	dev_dbg(&info->client->dev, "%s [START]\n", __func__);

	dev_dbg(&info->client->dev, "%s - mode[%u]\n", __func__, mode);

	wbuf[0] = MIP4_R0_CTRL;
	wbuf[1] = MIP4_R1_CTRL_POWER_STATE;
	wbuf[2] = mode;
	if (mip4_ts_i2c_write(info, wbuf, 3)) {
		dev_err(&info->client->dev, "%s [ERROR] mip4_ts_i2c_write\n", __func__);
		goto error;
	}

	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);
	return 0;

error:
	dev_err(&info->client->dev, "%s [ERROR]\n", __func__);
	return 1;
}

/*
 * Set wake-up gesture type
 */
int mip4_ts_set_wakeup_gesture_type(struct mip4_ts_info *info, u32 type)
{
	u8 wbuf[6];

	dev_dbg(&info->client->dev, "%s [START]\n", __func__);

	dev_dbg(&info->client->dev, "%s - type[%08X]\n", __func__, type);

	wbuf[0] = MIP4_R0_CTRL;
	wbuf[1] = MIP4_R1_CTRL_GESTURE_TYPE;
	wbuf[2] = (type >> 24) & 0xFF;
	wbuf[3] = (type >> 16) & 0xFF;
	wbuf[4] = (type >> 8) & 0xFF;
	wbuf[5] = type & 0xFF;
	if (mip4_ts_i2c_write(info, wbuf, 6)) {
		dev_err(&info->client->dev, "%s [ERROR] mip4_ts_i2c_write\n", __func__);
		goto error;
	}

	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);
	return 0;

error:
	dev_err(&info->client->dev, "%s [ERROR]\n", __func__);
	return 1;
}

/*
 * Disable ESD alert
 */
int mip4_ts_disable_esd_alert(struct mip4_ts_info *info)
{
	u8 wbuf[4];
	u8 rbuf[4];

	dev_dbg(&info->client->dev, "%s [START]\n", __func__);

	wbuf[0] = MIP4_R0_CTRL;
	wbuf[1] = MIP4_R1_CTRL_DISABLE_ESD_ALERT;
	wbuf[2] = 1;
	if (mip4_ts_i2c_write(info, wbuf, 3)) {
		dev_err(&info->client->dev, "%s [ERROR] mip4_ts_i2c_write\n", __func__);
		goto error;
	}

	if (mip4_ts_i2c_read(info, wbuf, 2, rbuf, 1)) {
		dev_err(&info->client->dev, "%s [ERROR] mip4_ts_i2c_read\n", __func__);
		goto error;
	}

	if (rbuf[0] != 1) {
		dev_dbg(&info->client->dev, "%s [ERROR] failed\n", __func__);
		goto error;
	}

	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);
	return 0;

error:
	dev_err(&info->client->dev, "%s [ERROR]\n", __func__);
	return 1;
}

/*
 * Alert event handler - ESD
 */
static int mip4_ts_alert_handler_esd(struct mip4_ts_info *info, u8 *rbuf)
{
	u8 frame_cnt = rbuf[1];

	dev_dbg(&info->client->dev, "%s [START]\n", __func__);

	dev_dbg(&info->client->dev, "%s - frame_cnt[%d]\n", __func__, frame_cnt);

	if (frame_cnt == 0) {
		/* Panel defect, not ESD */
		if(info->esd_cnt < 255)
			info->esd_cnt++;
		dev_dbg(&info->client->dev, "%s - esd_cnt[%d]\n", __func__, info->esd_cnt);

		if (info->disable_esd == true) {
			mip4_ts_disable_esd_alert(info);
			info->esd_cnt = 0;
		}
		else if (info->esd_cnt > ESD_COUNT_FOR_DISABLE) {
			if (!mip4_ts_disable_esd_alert(info)) {
				info->disable_esd = true;
				info->esd_cnt = 0;
			}
		}
		else {
			mip4_ts_restart(info);
		}
	}
	else {
		/* ESD detected */
		info->esd_cnt = 0;
		mip4_ts_restart(info);
	}

	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);
	return 0;
}

/*
 * Alert event handler - Wake-up
 */
static int mip4_ts_alert_handler_wakeup(struct mip4_ts_info *info, u8 *rbuf)
{
	int gesture_code = rbuf[1];

	dev_dbg(&info->client->dev, "%s [START]\n", __func__);

	if (mip4_ts_gesture_wakeup_event_handler(info, gesture_code)) {
		goto error;
	}

	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);
	return 0;

error:
	dev_err(&info->client->dev, "%s [ERROR]\n", __func__);
	return 1;
}

/*
 * Alert event handler - Input type
 */
static int mip4_ts_alert_handler_inputtype(struct mip4_ts_info *info, u8 *rbuf)
{
	u8 input_type = rbuf[1];

	dev_dbg(&info->client->dev, "%s [START]\n", __func__);

	switch (input_type) {
		case 0:
			dev_dbg(&info->client->dev, "%s - Input type : Finger\n", __func__);
			break;
		case 1:
			dev_dbg(&info->client->dev, "%s - Input type : Glove\n", __func__);
			break;
		default:
			dev_err(&info->client->dev, "%s - Input type : Unknown[%d]\n", __func__, input_type);
			goto error;
	}

	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);
	return 0;

error:
	dev_err(&info->client->dev, "%s [ERROR]\n", __func__);
	return 1;
}

/*
 * Alert event handler - Image
 */
static int mip4_ts_alert_handler_image(struct mip4_ts_info *info, u8 *rbuf)
{
	u8 image_type = rbuf[1];

	dev_dbg(&info->client->dev, "%s [START]\n", __func__);

	switch (image_type) {
#if USE_WAKEUP_GESTURE
		case MIP4_IMG_TYPE_GESTURE:
			if (mip4_ts_gesture_wakeup_event_handler(info, 1)) {
				dev_err(&info->client->dev, "%s [ERROR] mip4_ts_wakeup_event_handler\n", __func__);
				goto error;
			}
			if (mip4_ts_get_image(info, rbuf[1])) {
				dev_err(&info->client->dev, "%s [ERROR] mip4_ts_get_image\n", __func__);
				goto error;
			}
			break;
#endif /* USE_WAKEUP_GESTURE */
		default:
			goto error;
	}

	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);
	return 0;

error:
	dev_err(&info->client->dev, "%s [ERROR]\n", __func__);
	return 1;
}

/*
 * Alert event handler - Flash failure
 */
int mip4_ts_alert_handler_flash(struct mip4_ts_info *info, u8 *data)
{
#if USE_FW_RECOVERY
	int ret = 0;
	int retry = 1;
	u8 sections[64];
	int i_byte = 0;
	int i_bit = 0;

	dev_dbg(&info->client->dev, "%s [START]\n", __func__);

	print_hex_dump(KERN_ERR, MIP4_TS_DEVICE_NAME " Sections : ", DUMP_PREFIX_OFFSET, 16, 1, data, 8, false);

	for (i_byte = 0; i_byte < 8; i_byte++) {
		dev_info(&info->client->dev, "%s - packet #%d [0x%02X]\n", __func__, i_byte, data[i_byte]);

		for (i_bit = 0; i_bit < 8; i_bit++) {
			sections[i_byte * 8 + i_bit] = (data[i_byte] >> i_bit) & 0x01;
			dev_info(&info->client->dev, "%s - section #%d [%d]\n", __func__, i_byte * 8 + i_bit, sections[i_byte * 8 + i_bit]);
		}
	}

	ret = mip4_ts_fw_restore_from_kernel(info, sections, retry);

	if (ret < fw_err_none) {
		dev_err(&info->client->dev, "%s - fw_err[%d]\n", __func__, ret);
		goto error;
	}

	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);
	return 0;

error:
	dev_err(&info->client->dev, "%s [ERROR]\n", __func__);
	return 1;
#else
	return 0;
#endif /* USE_FW_RECOVERY */
}

/*
 * Alert event handler - SRAM failure
 */
static int mip4_ts_alert_handler_sram(struct mip4_ts_info *info, u8 *data)
{
	int i;

	dev_dbg(&info->client->dev, "%s [START]\n", __func__);

	info->sram_addr_num = (unsigned int) (data[0] | (data[1] << 8));
	dev_info(&info->client->dev, "%s - sram_addr_num [%d]\n", __func__, info->sram_addr_num);

	if (info->sram_addr_num > 8) {
		dev_err(&info->client->dev, "%s [ERROR] sram_addr_num [%d]\n", __func__, info->sram_addr_num);
		goto error;
	}

	for (i = 0; i < info->sram_addr_num; i++) {
		info->sram_addr[i] = data[2 + (4 * i)] | (data[2 + (4 * i) + 1] << 8) | (data[2 + (4 * i) + 2] << 16) | (data[2 + (4 * i) + 3] << 24);
		dev_info(&info->client->dev, "%s - sram_addr #%d [0x%08X]\n", __func__, i, info->sram_addr[i]);
	}
	for (i = info->sram_addr_num; i < 8; i++) {
		info->sram_addr[i] = 0;
		dev_info(&info->client->dev, "%s - sram_addr #%d [0x%08X]\n", __func__, i, info->sram_addr[i]);
	}

	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);
	return 0;

error:
	dev_err(&info->client->dev, "%s [ERROR]\n", __func__);
	return 1;
}

/*
 * Alert event handler - Custom
 */
static int __maybe_unused mip4_ts_alert_handler_f1(struct mip4_ts_info *info, u8 *rbuf, u8 size)
{

	u8 wbuf[8];
	u8 rbuf1[256];
        u8 rbuf2[256];
	u8 rbuf3[256];

	wbuf[0] = MIP4_R0_EVENT;
        wbuf[1] = 0x13;
        if (mip4_ts_i2c_read(info, wbuf, 2, rbuf2, 1)) {
                dev_err(&info->client->dev, "%s [ERROR] Read packet data\n", __func__);
        }

        wbuf[0] = MIP4_R0_EVENT;
        wbuf[1] = 0x14;
        if (mip4_ts_i2c_read(info, wbuf, 2, rbuf3, 1)) {
                dev_err(&info->client->dev, "%s [ERROR] Read packet data\n", __func__);
        }

	if(rbuf2[0] || rbuf3[0] || (melfas_debug >= DEBUG_TRACE))
	{

		wbuf[0] = MIP4_R0_EVENT;
	        wbuf[1] = 0x12;
	        if (mip4_ts_i2c_read(info, wbuf, 2, rbuf1, 1)) {
	                dev_err(&info->client->dev, "%s [ERROR] Read packet data\n", __func__);
	        }

		dev_info(&info->client->dev, "%s 0x0212 = %d\n",__func__,rbuf1[0]);
		dev_info(&info->client->dev, "%s 0x0213 = %d\n",__func__,rbuf2[0]);
		dev_info(&info->client->dev, "%s 0x0214 = %d\n",__func__,rbuf3[0]);

		wbuf[0] = MIP4_R0_EVENT;
	        wbuf[1] = 0x15;
	        if (mip4_ts_i2c_read(info, wbuf, 2, rbuf2, 1)) {
	                dev_err(&info->client->dev, "%s [ERROR] Read packet data\n", __func__);
	        }

		dev_info(&info->client->dev, "%s 0x0215 = %d\n",__func__,rbuf2[0]);
		
	        wbuf[0] = MIP4_R0_EVENT;
        	wbuf[1] = 0x16;
	        if (mip4_ts_i2c_read(info, wbuf, 2, rbuf2, 1)) {
	                dev_err(&info->client->dev, "%s [ERROR] Read packet data\n", __func__);
	        }

		dev_info(&info->client->dev, "%s 0x0216 = %d\n",__func__,rbuf2[0]);
	}

#if USE_INT_DBG
	dev_dbg(&info->client->dev, "%s [START]\n", __func__);
#endif

#if USE_AOT
	if (rbuf[1] == 2) {
		if (mip4_ts_aot_event_handler(info, rbuf, size)) {
			dev_err(&info->client->dev, "%s [ERROR]\n", __func__);
		}
	}
#endif /* USE_AOT */

#if USE_LPWG
	if (mip4_ts_lpwg_event_handler(info, rbuf, size)) {
		dev_err(&info->client->dev, "%s [ERROR]\n", __func__);
	}
#endif /* USE_LPWG */
#if USE_INT_DBG
	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);
#endif
	return 0;
}

/*
 * Interrupt handler
 */
static irqreturn_t mip4_ts_interrupt(int irq, void *dev_id)
{
	struct mip4_ts_info *info = dev_id;
	u8 wbuf[8];
	u8 rbuf[256];
	unsigned int size = 0;
	u8 category = 0;
	u8 alert_type = 0;
	bool startup = false;
	u8 flash_fail_type = flash_fail_none;
#if 1
	if(melfas_debug >= DEBUG_TRACE)
		dev_info(&info->client->dev, "%s [START]\n", __func__);
#endif

#if USE_STARTUP_WAITING
	/* Start-up interrupt */
	if (info->startup_init == false) {
		dev_dbg(&info->client->dev, "%s - skip\n", __func__);
		goto exit;
	}
	if (!completion_done(&info->startup_done)) {
		startup = true;
		dev_dbg(&info->client->dev, "%s - start-up\n", __func__);
	}
#endif /* USE_STARTUP_WAITING */

	/* Read packet info */
	wbuf[0] = MIP4_R0_EVENT;
	wbuf[1] = MIP4_R1_EVENT_PACKET_INFO;
	if (mip4_ts_i2c_read(info, wbuf, 2, rbuf, 1)) {
		dev_err(&info->client->dev, "%s [ERROR] Read packet info\n", __func__);
		if (!startup) {
			info->i2c_error_cnt++;
			goto error;
		}
		else {
			flash_fail_type = flash_fail_critical;
			goto startup_exit;
		}
	}

	size = (rbuf[0] & 0x3F);
	category = ((rbuf[0] >> 6) & 0x1);

#if 1//USE_INT_DBG
	if(melfas_debug >= DEBUG_TRACE)
		dev_info(&info->client->dev, "%s - packet info : size[%d] category[%d]\n", __func__, size, category);
#endif

	/* Check packet size */
	if ((size <= 0) || (size > 200)) {
		dev_err(&info->client->dev, "%s [ERROR] Packet size [%d]\n", __func__, size);
		if (!startup) {
			info->i2c_error_cnt++;
			goto error;
		}
		else {
			flash_fail_type = flash_fail_critical;
			goto startup_exit;
		}
	}

	/* Read packet data */
	wbuf[0] = MIP4_R0_EVENT;
	wbuf[1] = MIP4_R1_EVENT_PACKET_DATA;
	if (mip4_ts_i2c_read(info, wbuf, 2, rbuf, size)) {
		dev_err(&info->client->dev, "%s [ERROR] Read packet data\n", __func__);
		if (!startup) {
			info->i2c_error_cnt++;
			goto error;
		}
		else {
			flash_fail_type = flash_fail_critical;
			goto startup_exit;
		}
	}

	/* Event handler */
	if (startup == false) {
		spin_lock_irq(&reset_lock);
		if(LCD_TOUCH_INT_CHECK < INT_MAX)
			LCD_TOUCH_INT_CHECK ++;
		spin_unlock_irq(&reset_lock);
		if (category == 0) {
			/* Input event */
			info->esd_cnt = 0;

			mip4_ts_input_event_handler(info, size, rbuf);
		}
		else {
			/* Alert event */
			alert_type = rbuf[0];
#if 1//USE_INT_DBG
			if(melfas_debug >= DEBUG_TRACE)
				dev_info(&info->client->dev, "%s - alert type[%d]\n", __func__, alert_type);
#endif
			switch (alert_type) {
				case MIP4_ALERT_ESD:
					if (mip4_ts_alert_handler_esd(info, rbuf)) {
						goto error;
					}
					break;
				case MIP4_ALERT_WAKEUP:
					if (mip4_ts_alert_handler_wakeup(info, rbuf)) {
						goto error;
					}
					break;
				case MIP4_ALERT_INPUT_TYPE:
					if (mip4_ts_alert_handler_inputtype(info, rbuf)) {
						goto error;
					}
					break;
				case MIP4_ALERT_IMAGE:
					if (mip4_ts_alert_handler_image(info, rbuf)) {
						goto error;
					}
					break;
				case MIP4_ALERT_FLASH_FAILURE:
					if (mip4_ts_alert_handler_flash(info, &rbuf[1])) {
						goto error;
					}
					break;
				case MIP4_ALERT_SRAM_FAILURE:
					if (mip4_ts_alert_handler_sram(info, &rbuf[1])) {
						goto error;
					}
					break;
				case MIP4_ALERT_BOOT_SUCCEEDED:
					mip4_ts_startup_config(info);
					mip4_ts_startup_run(info);
					break;
				case MIP4_ALERT_F1:
					if (mip4_ts_alert_handler_f1(info, rbuf, size)) {
						goto error;
					}
					break;
				default:
					dev_err(&info->client->dev, "%s [ERROR] Unknown alert type[%d]\n", __func__, alert_type);
					goto error;
			}
		}
	}
	else {
#if USE_STARTUP_WAITING
		/* Start-up */
		if (category == 1) {
			alert_type = rbuf[0];
			dev_dbg(&info->client->dev, "%s - alert type[%d]\n", __func__, alert_type);

			switch (alert_type) {
				case MIP4_ALERT_FLASH_FAILURE:
					flash_fail_type = flash_fail_section;
					break;
				case MIP4_ALERT_BOOT_SUCCEEDED:
					flash_fail_type = flash_fail_none;
					break;
				default:
					dev_err(&info->client->dev, "%s [ERROR] Unknown alert type[%d]\n", __func__, alert_type);
					flash_fail_type = flash_fail_critical;
					break;
			}
		}
		else {
			dev_err(&info->client->dev, "%s [ERROR] Unknown catetory[%d]\n", __func__, category);
			flash_fail_type = flash_fail_critical;
		}
		goto startup_exit;
#endif /* USE_STARTUP_WAITING */
	}

#if USE_STARTUP_WAITING
exit:
#endif /* USE_STARTUP_WAITING */
	info->i2c_error_cnt = 0;
#if USE_INT_DBG
	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);
#endif
	return IRQ_HANDLED;

startup_exit:
#if USE_STARTUP_WAITING
	complete_all(&info->startup_done);
	dev_dbg(&info->client->dev, "%s - startup : complete_all\n", __func__);

	if (alert_type == MIP4_ALERT_BOOT_SUCCEEDED) {
		mip4_ts_startup_config(info);
		mip4_ts_startup_run(info);
	}

#if USE_FW_RECOVERY
	if (flash_fail_type == flash_fail_critical) {
		mip4_ts_fw_restore_critical_section(info);
	}
	else if (flash_fail_type == flash_fail_section) {
		mip4_ts_alert_handler_flash(info, &rbuf[1]);
	}
#endif /* USE_FW_RECOVERY */
#endif /* USE_STARTUP_WAITING */
#if USE_INT_DBG
	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);
#endif
	return IRQ_HANDLED;

error:
#if RESET_ON_EVENT_ERROR
	//dev_err(&info->client->dev, "%s - Reset on event error\n", __func__);
	//mip4_ts_restart(info);
#endif /* RESET_ON_EVENT_ERROR */
#if USE_STARTUP_WAITING

#endif /* USE_STARTUP_WAITING */
#if USE_FW_RECOVERY
	if (info->i2c_error_cnt >= I2C_ERR_CNT_FOR_FW_RECOVERY) {
		mip4_ts_fw_restore_critical_section(info);
	}
#endif /* USE_FW_RECOVERY */

	dev_err(&info->client->dev, "%s [ERROR]\n", __func__);
	return IRQ_HANDLED;
}

/*
 * Config module
 */
int mip4_ts_config(struct mip4_ts_info *info)
{
	u8 wbuf[4];
	u8 rbuf[16];
	int ret = 0;
	int retry = I2C_RETRY_COUNT;

	dev_dbg(&info->client->dev, "%s [START]\n", __func__);

	/* Product name */
	wbuf[0] = MIP4_R0_INFO;
	wbuf[1] = MIP4_R1_INFO_PRODUCT_NAME;
	ret = mip4_ts_i2c_read(info, wbuf, 2, rbuf, 16);
	if (ret) {
		dev_err(&info->client->dev, "%s [ERROR] mip4_ts_i2c_read\n", __func__);
		goto error;
	}

	memcpy(info->product_name, rbuf, 16);
	dev_dbg(&info->client->dev, "%s - product_name[%s]\n", __func__, info->product_name);

	/* Firmware version */
	ret = mip4_ts_get_fw_version(info, rbuf);
	if (ret) {
		dev_err(&info->client->dev, "%s [ERROR] mip4_ts_i2c_read\n", __func__);
		goto error;
	}

	memcpy(info->fw_version, rbuf, 8);
	dev_info(&info->client->dev, "%s - F/W Version : %02X.%02X %02X.%02X %02X.%02X %02X.%02X\n", __func__, info->fw_version[0], info->fw_version[1], info->fw_version[2], info->fw_version[3], info->fw_version[4], info->fw_version[5], info->fw_version[6], info->fw_version[7]);

	/* Sometimes chip didn't enough time to read f/w info after chip reset.
	(f/w version read 00.00 00.00 00.00 00.00)
	So add 200ms delay, and retry to read f/w info(from ESD test 18/5/2)*/
	if((info->fw_version[0] == 0) && (info->fw_version[1] == 0)) {
		dev_err(&info->client->dev, "%s - F/W not correct\n", __func__);
		goto error;
	}

	/* Resolution */
	wbuf[0] = MIP4_R0_INFO;
	wbuf[1] = MIP4_R1_INFO_RESOLUTION_X;
	ret = mip4_ts_i2c_read(info, wbuf, 2, rbuf, 14);
	if (ret) {
		dev_err(&info->client->dev, "%s [ERROR] mip4_ts_i2c_read\n", __func__);
		goto error;
	}

	info->max_x = (rbuf[0]) | (rbuf[1] << 8);
	info->max_y = (rbuf[2]) | (rbuf[3] << 8);
	dev_dbg(&info->client->dev, "%s - max_x[%d] max_y[%d]\n", __func__, info->max_x, info->max_y);

	info->ppm_x = rbuf[12];
	info->ppm_y = rbuf[13];
	dev_dbg(&info->client->dev, "%s - ppm_x[%d] ppm_y[%d]\n", __func__, info->ppm_x, info->ppm_y);

	/* Node info */
	info->node_x = rbuf[4];
	info->node_y = rbuf[5];
	info->node_key = rbuf[6];
	dev_dbg(&info->client->dev, "%s - node_x[%d] node_y[%d] node_key[%d]\n", __func__, info->node_x, info->node_y, info->node_key);

	/* Key info */
	if (info->node_key > 0) {
		/* Enable touchkey */
		info->key_enable = true;
		info->key_num = info->node_key;
	}

	/* Protocol */
	wbuf[0] = MIP4_R0_EVENT;
	wbuf[1] = MIP4_R1_EVENT_FORMAT;
	while (retry--) {
		if (mip4_ts_i2c_read(info, wbuf, 2, rbuf, 3)) {
			dev_err(&info->client->dev, "%s [ERROR] mip4_ts_i2c_read - event format\n", __func__);
		}
		else {
			info->event_format = rbuf[0] | (rbuf[1] << 8);
			info->event_size = rbuf[2];
			if (info->event_size <= 0) {
				dev_err(&info->client->dev, "%s [ERROR] event_size[%d]\n", __func__, info->event_size);
				goto error;
			}
			dev_dbg(&info->client->dev, "%s - event_format[%d] event_size[%d]\n", __func__, info->event_format, info->event_size);
			break;
		}
	}
	if (retry < 0) {
		dev_err(&info->client->dev, "%s [ERROR] event format - retry limit\n", __func__);
		goto error;
	}

	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);
	return 0;

error:
	dev_err(&info->client->dev, "%s [ERROR]\n", __func__);
	return 1;
}

/*
 * Config platform
 */
static int mip4_ts_config_platform(struct mip4_ts_info *info)
{
#ifdef CONFIG_ACPI
	struct acpi_device *a_dev;
	acpi_status a_status;
#endif /* CONFIG_ACPI */
	int ret = 0;

	dev_dbg(&info->client->dev, "%s [START]\n", __func__);

#ifdef CONFIG_ACPI
	/* ACPI */
	dev_dbg(&info->client->dev, "%s - ACPI\n", __func__);

	a_status = acpi_bus_get_device(ACPI_HANDLE(&info->client->dev), &a_dev);
	if (ACPI_SUCCESS(a_status)) {
		if (strncmp(dev_name(&a_dev->dev), ACPI_ID, 8) != 0) {
			dev_err(&info->client->dev, "%s [ERROR] ACPI_ID mismatch [%s]\n", __func__, dev_name(&a_dev->dev));
			ret = -EINVAL;
			goto mip4_exit;
		}
	}
#else

#ifdef CONFIG_OF
	/* Devicetree */
	if (&info->client->dev.of_node) {
		dev_dbg(&info->client->dev, "%s - Devicetree\n", __func__);

		ret = mip4_ts_parse_devicetree(&info->client->dev, info);
		if (ret) {
			dev_err(&info->client->dev, "%s [ERROR] mip4_ts_parse_devicetree\n", __func__);
			goto mip4_exit;
		}
	}
	else {
		dev_err(&info->client->dev, "%s [ERROR] of_node\n", __func__);
		goto mip4_exit;
	}
#else
	/* Platform device */
	dev_dbg(&info->client->dev, "%s - Platform device\n", __func__);

	info->pdata = dev_get_platdata(&info->client->dev);
	if (info->pdata == NULL) {
		ret = -EINVAL;
		dev_err(&info->client->dev, "%s [ERROR] dev_get_platdata\n", __func__);
		goto mip4_exit;
	}

	info->gpio_intr = info->pdata->gpio_intr;
	info->gpio_ce = info->pdata->gpio_ce;
	info->gpio_vd33_en = info->pdata->gpio_vd33_en;

	ret = mip4_ts_config_gpio(info);
	if (ret) {
		dev_err(&info->client->dev, "%s [ERROR] mip4_ts_config_gpio\n", __func__);
		goto mip4_exit;
	}
#endif /* CONFIG_OF */

#endif /* CONFIG_ACPI */

mip4_exit:
	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);
	return ret;
}

#if (CHIP_MODEL != CHIP_NONE)
#if USE_FW_RECOVERY
/*
 * Restore critical section of firmware from kernel built-in firmware file
 */
int mip4_ts_fw_restore_critical_section(struct mip4_ts_info *info)
{
	int ret = 0;
	int i;
	u8 sections[FW_SECTION_NUM];
	int retry = 1;

	dev_dbg(&info->client->dev, "%s [START]\n", __func__);

	/* Restore firmware */
	memset(sections, 0, FW_SECTION_NUM);
	for (i = 0; i < FW_CRITICAL_SECTION_NUM; i++) {
		sections[i] = 1;
	}

	ret = mip4_ts_fw_restore_from_kernel(info, sections, retry);
	if (ret < 0) {
		dev_err(&info->client->dev, "%s [ERROR] mms_fw_restore_from_kernel\n", __func__);
		goto error;
	}

	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);
	return 0;

error:
	dev_err(&info->client->dev, "%s [ERROR]\n", __func__);
	return 1;
}

/*
 * Restore firmware from kernel built-in firmware file
 */
int mip4_ts_fw_restore_from_kernel(struct mip4_ts_info *info, u8 *sections, int retry)
{
	const char *fw_name = FW_PATH_INTERNAL;
	const struct firmware *fw;
	int ret = fw_err_none;

	dev_dbg(&info->client->dev, "%s [START]\n", __func__);

	//mutex_lock(&info->lock);
	mip4_ts_clear_input(info);

	print_hex_dump(KERN_ERR, MIP4_TS_DEVICE_NAME " Sections : ", DUMP_PREFIX_OFFSET, 16, 1, sections, FW_SECTION_NUM, false);

	/* Get firmware */
	request_firmware(&fw, fw_name, &info->client->dev);
	if (!fw) {
		dev_err(&info->client->dev, "%s [ERROR] request_firmware\n", __func__);
		ret = fw_err_file_open;
		goto error;
	}

	/* Restore firmware */
	while (retry > 0) {
		ret = mip4_ts_restore_fw(info, fw->data, fw->size, sections);
		dev_err(&info->client->dev, "%s - mip4_ts_restore_fw[%d]\n", __func__, ret);
		if (ret >= fw_err_none) {
			break;
		}

		retry--;
	}
	if (retry <= 0) {
		dev_err(&info->client->dev, "%s [ERROR] mip4_ts_restore_fw failed\n", __func__);
		ret = fw_err_download;
	}
	else {
		ret = fw_err_none;
	}

	release_firmware(fw);

	if (ret < fw_err_none) {
		goto error;
	}

	//mutex_unlock(&info->lock);

	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);
	return 0;

error:
	//mutex_unlock(&info->lock);

	dev_err(&info->client->dev, "%s [ERROR]\n", __func__);
	return ret;
}
#endif

/*
 * Update firmware from kernel built-in firmware file
 */
int mip4_ts_fw_update_from_kernel(struct mip4_ts_info *info, bool force)
{
	const char *fw_name = FW_PATH_INTERNAL;
	const struct firmware *fw;
	int retry = 1;
	int retry2 = 5;
	int ret = fw_err_none;
	u16 ver_chip[FW_MAX_SECT_NUM];

	dev_dbg(&info->client->dev, "%s [START]\n", __func__);

	/* Disable IRQ */
	mutex_lock(&info->lock);
	if(info->irq_enabled){
		disable_irq(info->client->irq);
		info->irq_enabled = false;
	}
	mip4_ts_clear_input(info);

	/* Get firmware */

	while(retry2--) {
		mip4_ts_get_fw_version_u16(info, ver_chip);
		if((ver_chip[3] != 0) && (ver_chip[2] != 0))
			break;
		msleep(100);
		dev_info(&info->client->dev, "%s MELFAS CHIP msleep(100)\n",__func__);
	}


	dev_info(&info->client->dev, "Chip firmware version [0x%04X 0x%04X 0x%04X 0x%04X]\n", ver_chip[0], ver_chip[1], ver_chip[2], ver_chip[3]);

	if(ver_chip[2] == 0x0100)
        {
                if((ver_chip[3] & 0xF000) == 0xF000)
                {
                         request_firmware(&fw, FW_PATH_INTERNAL_10_25_plastic, &info->client->dev);
                }
                else
                {
                        request_firmware(&fw, FW_PATH_INTERNAL_10_25, &info->client->dev);
                }
        }
        else if((ver_chip[2] == 0x0200) || (ver_chip[2] == 0x0300))
        {
                request_firmware(&fw, FW_PATH_INTERNAL_12_3, &info->client->dev);
        }
        else
        {
                force = true;
                if(!get_montype()) {
                        request_firmware(&fw, FW_PATH_INTERNAL_12_3, &info->client->dev);
                }
                else {
                        request_firmware(&fw, FW_PATH_INTERNAL_10_25, &info->client->dev);
                }
        }

	if (!fw) {
		dev_err(&info->client->dev, "%s [ERROR] request_firmware\n", __func__);
		ret = fw_err_file_open;
		goto error;
	}

	/* Update firmware */
	do {
		ret = mip4_ts_flash_fw(info, fw->data, fw->size, force, true);
		if (ret >= fw_err_none) {
			break;
		}
	}
	while (--retry);

	if (!retry) {
		dev_err(&info->client->dev, "%s [ERROR] mip4_ts_flash_fw failed\n", __func__);
		ret = fw_err_download;
	}

	release_firmware(fw);

	/* Enable IRQ */
	if(!info->irq_enabled) {
		enable_irq(info->client->irq);
		info->irq_enabled = true;
	}
	mutex_unlock(&info->lock);

	if (ret < fw_err_none) {
		goto error;
	}

	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);
	return 0;

error:
	dev_err(&info->client->dev, "%s [ERROR]\n", __func__);
	return ret;
}

/*
 * Update firmware from external storage
 */
int mip4_ts_fw_update_from_storage(struct mip4_ts_info *info, char *path, bool force)
{
	struct file *fp;
	mm_segment_t old_fs;
	size_t fw_size, nread;
	unsigned char *fw_data;
	int ret = fw_err_none;

	dev_dbg(&info->client->dev, "%s [START]\n", __func__);

	/* Disable IRQ */
	mutex_lock(&info->lock);
	disable_irq(info->client->irq);
	mip4_ts_clear_input(info);

	/* Get firmware */
	old_fs = get_fs();
	set_fs(KERNEL_DS);

	fp = filp_open(path, O_RDONLY, S_IRUSR);
	if (IS_ERR(fp)) {
		dev_err(&info->client->dev, "%s [ERROR] file_open - path[%s]\n", __func__, path);
		ret = fw_err_file_open;
		goto error;
	}

	fw_size = fp->f_path.dentry->d_inode->i_size;
	if (fw_size > 0) {
		/* Read firmware */
		fw_data = kzalloc(fw_size, GFP_KERNEL);
		nread = vfs_read(fp, (char __user *)fw_data, fw_size, &fp->f_pos);
		dev_dbg(&info->client->dev, "%s - path[%s] size[%zu]\n", __func__, path, fw_size);

		if (nread != fw_size) {
			dev_err(&info->client->dev, "%s [ERROR] vfs_read - size[%zu] read[%zu]\n", __func__, fw_size, nread);
			ret = fw_err_file_read;
		}
		else {
			/* Update firmware */
			ret = mip4_ts_flash_fw(info, fw_data, fw_size, force, true);
		}

		kfree(fw_data);
	}
	else {
		dev_err(&info->client->dev, "%s [ERROR] fw_size[%zu]\n", __func__, fw_size);
		ret = fw_err_file_read;
	}

	filp_close(fp, current->files);

error:
	set_fs(old_fs);

	/* Enable IRQ */
	enable_irq(info->client->irq);
	mutex_unlock(&info->lock);

	if (ret < fw_err_none) {
		dev_err(&info->client->dev, "%s [ERROR]\n", __func__);
	}
	else {
		dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);
	}

	return ret;
}

/*
 * Sysfs - firmware update
 */
static ssize_t mip4_ts_sys_fw_update(struct device *dev, struct device_attribute *attr, char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct mip4_ts_info *info = i2c_get_clientdata(client);
	int result = 0;
	u8 data[255];
	int ret = 0;
	const struct firmware *fw;
	int retry = 5;
	u16 ver_chip[FW_MAX_SECT_NUM];
	int state;

	dev_info(&info->client->dev, "%s [START]\n", __func__);

	if(sscanf(buf, "%d", &state) < 0)
		return -EINVAL;

	if((state == 1) || (state == 3))
	{
		FW_UPDATE_RESULT = 1;

		mip4_serdes_reset_dwork_stop(info);

		if(state == 1)
			ret = mip4_ts_fw_update_from_kernel(info, false);
		else if(state == 3)
			ret = mip4_ts_fw_update_from_kernel(info, true);
	}
	else if((state == 2) || (state == 4))
	{
		FW_UPDATE_RESULT = 1;

		mip4_serdes_reset_dwork_stop(info);

		while(retry--) {
			mip4_ts_get_fw_version_u16(info, ver_chip);
	                if((ver_chip[3] != 0) && (ver_chip[2] != 0))
	                        break;
	                msleep(100);
	                printk("%s MELFAS CHIP msleep(100)\n",__func__);
	        }

		dev_info(&info->client->dev, "Chip firmware version [0x%04X 0x%04X 0x%04X 0x%04X]\n", ver_chip[0], ver_chip[1], ver_chip[2], ver_chip[3]);

		if(ver_chip[2] == 0x0100)
	        {
			if(state == 4)
				info->fw_path_ext = kstrdup(FW_PATH_EXTERNAL_10_25, GFP_KERNEL);
			else
				info->fw_path_ext = kstrdup(FW_PATH_EXTERNAL_USB_10_25, GFP_KERNEL);
	        }
	        else if((ver_chip[2] == 0x0200) || (ver_chip[2] == 0x0300))
	        {
			if(state == 4)
				info->fw_path_ext = kstrdup(FW_PATH_EXTERNAL_12_3, GFP_KERNEL);
			else
				info->fw_path_ext = kstrdup(FW_PATH_EXTERNAL_USB_12_3, GFP_KERNEL);
	        }
	        else
	        {
	                if(!get_montype()) {
				if(state == 4)
					info->fw_path_ext = kstrdup(FW_PATH_EXTERNAL_12_3, GFP_KERNEL);
				else
					info->fw_path_ext = kstrdup(FW_PATH_EXTERNAL_USB_12_3, GFP_KERNEL);
	                }
	                else {
				if(state == 4)
					info->fw_path_ext = kstrdup(FW_PATH_EXTERNAL_10_25, GFP_KERNEL);
				else
					info->fw_path_ext = kstrdup(FW_PATH_EXTERNAL_10_25, GFP_KERNEL);
	                }
	        }

		ret = mip4_ts_fw_update_from_storage(info, info->fw_path_ext, true);
	}
	else
	{
		dev_info(&info->client->dev, "F/W upgrade end with wrong value %d\n", state);	
		return -EINVAL;
	}

	switch (ret) {
		case fw_err_none:
			snprintf(data, sizeof(data), "F/W update success.\n");
			break;
		case fw_err_uptodate:
			snprintf(data, sizeof(data), "F/W is already up-to-date.\n");
			break;
		case fw_err_download:
			snprintf(data, sizeof(data), "F/W update failed : Download error\n");
			break;
		case fw_err_file_type:
			snprintf(data, sizeof(data), "F/W update failed : File type error\n");
			break;
		case fw_err_file_open:
			snprintf(data, sizeof(data), "F/W update failed : File open error[%s]\n", info->fw_path_ext);
			break;
		case fw_err_file_read:
			snprintf(data, sizeof(data), "F/W update failed : File read error\n");
			break;
		default:
			snprintf(data, sizeof(data), "F/W update failed.\n");
			break;
	}

	/* Re-config driver */
	mip4_ts_config(info);
	mip4_ts_config_input(info);

	dev_info(&info->client->dev, "%s [DONE]\n", __func__);

	LCD_TOUCH_INT_CHECK = 2;

	FW_UPDATE_RESULT = 0;

	mip4_serdes_reset_dwork_start(info);

	result = snprintf(buf, 255, "%s\n", data);
	return result;
}

static ssize_t mip4_ts_sys_fw_update_select(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct mip4_ts_info *info = i2c_get_clientdata(client);
	int result = 0;
	u8 data[255];
	int ret = 0;
	int choice;
	const struct firmware *fw;
	int retry = 1;

	dev_dbg(&info->client->dev, "%s [START]\n", __func__);

	if(sscanf(buf, "%d", &choice) < 0)
		return 0;


	switch(choice) {
		case 0:
			request_firmware(&fw, "melfas/Auto10d25_FW_v0005_171123", &info->client->dev);
			break;
		case 1:
			request_firmware(&fw, "melfas/Auto10d25_FW_v0006_180110", &info->client->dev);
			break;
		case 2:
			request_firmware(&fw, "melfas/Auto10d25_FW_v0009_180406", &info->client->dev);
			break;
		case 3:
			request_firmware(&fw, "melfas/Auto10d25_FW_v0011_180516", &info->client->dev);
			break;
		case 4:
			request_firmware(&fw, "melfas/Auto10d25_FW_v0012_180726", &info->client->dev);
			break;
		case 5:
                        request_firmware(&fw, "melfas/Auto10d25_FW_v0014_180904", &info->client->dev);
                        break;
		case 6:
			request_firmware(&fw, "melfas/Auto10d25_FW_v0015_190123", &info->client->dev);
			break;
		case 7:
                        request_firmware(&fw, "melfas/Auto10d25_FW_v0016_200608", &info->client->dev);
                        break;
                case 8:
                        request_firmware(&fw, "melfas/Auto10d25_Plastic2T_Test_FW_vF001", &info->client->dev);
                        break;
                case 9:
                        request_firmware(&fw, "melfas/Auto12d3_FW_v0200_v0220_180226", &info->client->dev);
                        break;
                case 10:
                        request_firmware(&fw, "melfas/Auto12d3_GMSL2_FW_v0001_180906", &info->client->dev);
                        break;
                case 11:
                        request_firmware(&fw, "melfas/Auto12d3_GMSL2_FW_v0003_181012", &info->client->dev);
                        break;
                case 12:
                        request_firmware(&fw, "melfas/Auto12d3_GMSL2_FW_v0007_190128", &info->client->dev);
                        break;
                case 13:
                        request_firmware(&fw, "melfas/Auto12d3_GMSL2_FW_v0008_190322", &info->client->dev);
                        break;
                case 14:
                        request_firmware(&fw, "melfas/Auto12d3_GMSL2_FW_v0009_191010", &info->client->dev);
                        break;
                case 15:
                        request_firmware(&fw, "melfas/Auto12d3_GMSL2_FW_v0010_200608", &info->client->dev);
                        break;
		default:
			printk("number you choose is wrong : %d\n", choice);
			return 0;
			break;
	}

	mip4_serdes_reset_dwork_stop(info);

	/* Disable IRQ */
	mutex_lock(&info->lock);
	disable_irq(info->client->irq);
	mip4_ts_clear_input(info);

	/* Update firmware */
	do {
		ret = mip4_ts_flash_fw(info, fw->data, fw->size, true, true);
		if (ret >= fw_err_none) {
			break;
		}
	}
	while (--retry);

	if (!retry) {
		dev_err(&info->client->dev, "%s [ERROR] mip4_ts_flash_fw failed\n", __func__);
		ret = fw_err_download;
	}

	release_firmware(fw);

	/* Enable IRQ */
	enable_irq(info->client->irq);
	mutex_unlock(&info->lock);

	switch (ret) {
		case fw_err_none:
			snprintf(data, sizeof(data), "F/W update success.\n");
			break;
		case fw_err_uptodate:
			snprintf(data, sizeof(data), "F/W is already up-to-date.\n");
			break;
		case fw_err_download:
			snprintf(data, sizeof(data), "F/W update failed : Download error\n");
			break;
		case fw_err_file_type:
			snprintf(data, sizeof(data), "F/W update failed : File type error\n");
			break;
		case fw_err_file_open:
			snprintf(data, sizeof(data), "F/W update failed : File open error[%s]\n", info->fw_path_ext);
			break;
		case fw_err_file_read:
			snprintf(data, sizeof(data), "F/W update failed : File read error\n");
			break;
		default:
			snprintf(data, sizeof(data), "F/W update failed.\n");
			break;
	}

	/* Re-config driver */
	mip4_ts_config(info);
	mip4_ts_config_input(info);

	dev_dbg(&info->client->dev, "%s [DONE]\n", __func__);

	LCD_TOUCH_INT_CHECK = 2;

	mip4_serdes_reset_dwork_start(info);

	result = snprintf(buf, 255, "%s\n", data);
	return result;
}

static ssize_t mip4_ts_sys_fw_update_result(struct device *dev, struct device_attribute *attr, char *buf)
{
        int ret;
        struct i2c_client *client = to_i2c_client(dev);
        struct mip4_ts_info *info = i2c_get_clientdata(client);

	dev_info(&info->client->dev, "%s [START] %d\n", __func__,FW_UPDATE_RESULT);

        ret = snprintf(buf, 255, "%d\n", FW_UPDATE_RESULT);

        return ret;
}

static ssize_t mip4_ts_show_list(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct mip4_ts_info *info = i2c_get_clientdata(client);
	u16 ver_chip[FW_MAX_SECT_NUM];

	mip4_ts_get_fw_version_u16(info, ver_chip);

	return snprintf(buf, PAGE_SIZE, "Current hip firmware version [0x%04X 0x%04X 0x%04X 0x%04X]\n"
					"0:Auto10d25_FW_v0005_171123\n"
					"1:Auto10d25_FW_v0006_180110\n"
					"2:Auto10d25_FW_v0009_180406\n"
					"3:Auto10d25_FW_v0011_180516\n"
					"4:Auto10d25_FW_v0012_180726\n"
					"5:Auto10d25_FW_v0014_180904\n"
					"6:Auto10d25_FW_v0015_190123\n"
					"7:Auto10d25_FW_v0016_200608\n"
                                        "8:Auto10d25_Plastic2T_Test_FW_vF001\n"
                                        "9:Auto12d3_FW_v0200_v0220_180226\n"
                                        "10:Auto12d3_GMSL2_FW_v0001_180906\n"
                                        "11:Auto12d3_GMSL2_FW_v0003_181012\n"
                                        "12:Auto12d3_GMSL2_FW_v0007_190128\n"
                                        "13:Auto12d3_GMSL2_FW_v0008_190322\n"
                                        "14:Auto12d3_GMSL2_FW_v0009_191010\n"
                                        "15:Auto12d3_GMSL2_FW_v0010_200608\n"
					, ver_chip[0], ver_chip[1], ver_chip[2], ver_chip[3]);
}

static ssize_t show_debug(struct device *dev, struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "CURRENT DEBUG MODE : %d\n"
					"0:NONE\n" "1:INFO\n"
					"2:MESSAGES\n" "3:TRACE\n", melfas_debug);
}

static ssize_t set_debug(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int state;
	struct i2c_client *client = to_i2c_client(dev);
	struct mip4_ts_info *info = i2c_get_clientdata(client);

	if(sscanf(buf, "%d", &state) < 0)
		return -EINVAL;

	if ((state >= DEBUG_NONE) && (state <= DEBUG_INT)) {
		melfas_debug = state;
		pr_info("\tCURRENT DEBUG MODE : %d\n"
				"\t0:NONE\n" "\t1:INFO\n"
				"\t2:MESSAGES\n" "\t3:TRACE\n \t4:INTURRUPT\n", melfas_debug);
	}
	else {
		return -EINVAL;
	}

	if(count < INT_MAX)
                return count;
        else
                return INT_MAX;
}

static ssize_t show_sensitivity(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret;
	struct i2c_client *client = to_i2c_client(dev);
	struct mip4_ts_info *info = i2c_get_clientdata(client);

	u8 buf1[3];
	u8 buf2[3];
	u8 data[255];

	buf1[0] = 0x06;
	buf1[1] = 0x23;

	ret = i2c_master_send(client, buf1, 2);
	ret = i2c_master_recv(client, buf2, 1);

//	dev_info(&info->client->dev, "%s current touch sensitivity : %d %d %d\n", __func__, buf2[0], buf2[1], buf2[2]);

	snprintf(data, sizeof(data), "%d\n", buf2[0]);

	ret = snprintf(buf, 255, "%s\n", data);

	return ret;

}


static ssize_t set_sensitivity(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int state;
	int ret;
	struct i2c_client *client = to_i2c_client(dev);
	struct mip4_ts_info *info = i2c_get_clientdata(client);

	u8 buf1[3];
	u8 buf2[3];
	u8 data[255];

	buf1[0] = 0x06;
	buf1[1] = 0x23;

	ret = i2c_master_send(client, buf1, 2);
	ret = i2c_master_recv(client, buf2, 1);

	dev_info(&info->client->dev, "%s current touch sensitivity : %d\n", __func__, buf2[0]);

	if(sscanf(buf, "%d", &state) < 0)
		return -EINVAL;

	if ((state >= 0) && (state <= 3)) {
		buf1[2] = state;
		ret =  i2c_master_send(client, buf1, 3);
	}
	else {
		return -EINVAL;
	}

	if(ret != 3) {
		dev_err(&client->dev, "%s: i2c fail\n", __func__);
	}

	ret = i2c_master_send(client, buf1, 2);
	ret = i2c_master_recv(client, buf2, 1);

	dev_info(&info->client->dev, "%s change touch sensitivity : %d\n", __func__, buf2[0]);

	if(count < INT_MAX)
                return count;
        else
                return INT_MAX;
}
static ssize_t show_read_register(struct device *dev, struct device_attribute *attr, char *buf)
{
        int ret, i;
        struct i2c_client *client = to_i2c_client(dev);
        struct mip4_ts_info *info = i2c_get_clientdata(client);
        u8 buf1[3];
        u8 buf2[3];
        unsigned short addr;

	mip4_serdes_reset_dwork_stop(info);
        mutex_lock(&info->serdes.lock);

        if(info->irq_enabled) {
                disable_irq(info->client->irq);
                info->irq_enabled = false;
        }

        addr = client->addr;

        printk("-----------%s\n", __func__);

	serdes_video_signal_check(info->client, 0);

	client->addr = serdes_line_fault_config[0][0];
	buf1[0] = (serdes_line_fault_config[0][1] >> 8) & 0xff;
	buf1[1] = serdes_line_fault_config[0][1] & 0xff;
	buf1[2] = serdes_line_fault_config[0][2];

	ret = i2c_master_send(client, buf1, 3);

	mdelay(20);

        for(i = 0; i < 6 ;i++) {

                client->addr = serdes_read[i][0];
                buf1[0] = (serdes_read[i][1] >> 8) & 0xff;
                buf1[1] = serdes_read[i][1] & 0xff;
                buf1[2] = serdes_read[i][2];

                if(client->addr != DES_DELAY)
                {
                        ret = i2c_master_send(client, buf1, 2);
                        ret = i2c_master_recv(client, buf2, 1);
                }
		if(client->addr == SER_ADDRESS)
			printk("SER ");
		else if(client->addr == DES_ADDRESS)
			printk("DES ");
                printk("0x%04X write:0x%02X read:0x%02X\n",serdes_read[i][1], serdes_read[i][2], buf2[0]);
        }

	client->addr = serdes_line_fault_config[1][0];
        buf1[0] = (serdes_line_fault_config[1][1] >> 8) & 0xff;
        buf1[1] = serdes_line_fault_config[1][1] & 0xff;
        buf1[2] = serdes_line_fault_config[1][2];

        ret = i2c_master_send(client, buf1, 3);

	mdelay(100);

	client->addr = serdes_line_fault_config[2][0];
        buf1[0] = (serdes_line_fault_config[2][1] >> 8) & 0xff;
        buf1[1] = serdes_line_fault_config[2][1] & 0xff;

	ret = i2c_master_send(client, buf1, 2);
        ret = i2c_master_recv(client, buf2, 1);

	printk("%s err 0x%x\n",__func__, buf2[0]);

        printk("-----------%s Success\n", __func__);

        client->addr = addr;

        if(!info->irq_enabled) {
                enable_irq(info->client->irq);
                info->irq_enabled = true;
        }
        mutex_unlock(&info->serdes.lock);
	mip4_serdes_reset_dwork_start(info);

        return ;
}

static ssize_t set_des_bitrate_change(struct device *dev, struct device_attribute *attr, char *buf)
{
        struct i2c_client *client = to_i2c_client(dev);
        struct mip4_ts_info *info = i2c_get_clientdata(client);

        mip4_serdes_reset_dwork_stop(info);
        mutex_lock(&info->serdes.lock);

        if(info->irq_enabled) {
                disable_irq(info->client->irq);
                info->irq_enabled = false;
        }

        des_i2c_bitrate_3gbps_to_6gbps(info->client);

	if(!info->irq_enabled) {
                enable_irq(info->client->irq);
                info->irq_enabled = true;
        }
        mutex_unlock(&info->serdes.lock);
        mip4_serdes_reset_dwork_start(info);

	return;
}

static ssize_t show_serdes_setting(struct device *dev, struct device_attribute *attr, char *buf)
{
        int ret, i;
        struct i2c_client *client = to_i2c_client(dev);
        struct mip4_ts_info *info = i2c_get_clientdata(client);
	u8 buf1[3];
        u8 buf2[3];
	unsigned short addr;

	mip4_serdes_reset_dwork_stop(info);
	mutex_lock(&info->serdes.lock);
        if(info->irq_enabled) {
                disable_irq(info->client->irq);
                info->irq_enabled = false;
        }

	serdes_i2c_show(client);

	if(!info->irq_enabled) {
                enable_irq(info->client->irq);
                info->irq_enabled = true;
        }
        mutex_unlock(&info->serdes.lock);
	mip4_serdes_reset_dwork_start(info);

        return 0;
}

static ssize_t set_serdes_setting(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
        struct i2c_client *client = to_i2c_client(dev);
        struct mip4_ts_info *info = i2c_get_clientdata(client);
	int ser_bitrate = 0;
	int des_bitrate = 0;

	mip4_serdes_reset_dwork_stop(info);
	mutex_lock(&info->serdes.lock);
	if(info->irq_enabled) {
		disable_irq(info->client->irq);
	        info->irq_enabled = false;
	}

	ser_bitrate = ser_i2c_bitrate_check(info->client);
        des_bitrate = des_i2c_bitrate_check(info->client);

        printk("Ser_Bitrate : 0x%x Des_Bitrate : 0x%x\n",ser_bitrate,des_bitrate);

	serdes_i2c_reset(info->client,false);

	if(!info->irq_enabled) {
		enable_irq(info->client->irq);
		info->irq_enabled = true;
	}
	mutex_unlock(&info->serdes.lock);
	mip4_serdes_reset_dwork_start(info);

	if(count < INT_MAX)
                return count;
        else
                return INT_MAX;
}

static ssize_t set_des_setting(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
        struct i2c_client *client = to_i2c_client(dev);
        struct mip4_ts_info *info = i2c_get_clientdata(client);
	int ser_bitrate = 0;
	int des_bitrate = 0;

	mip4_serdes_reset_dwork_stop(info);
        mutex_lock(&info->serdes.lock);
        if(info->irq_enabled) {
                disable_irq(info->client->irq);
                info->irq_enabled = false;
        }

	ser_bitrate = ser_i2c_bitrate_check(info->client);
        des_bitrate = des_i2c_bitrate_check(info->client);

        printk("Ser_Bitrate : 0x%x Des_Bitrate : 0x%x\n",ser_bitrate,des_bitrate);

	serdes_i2c_reset(info->client,true);

        if(!info->irq_enabled) {
                enable_irq(info->client->irq);
                info->irq_enabled = true;
        }
        mutex_unlock(&info->serdes.lock);
	mip4_serdes_reset_dwork_start(info);

	if(count < INT_MAX)
                return count;
        else
                return INT_MAX;
}

static ssize_t set_serdes_6gbps(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
        struct i2c_client *client = to_i2c_client(dev);
        struct mip4_ts_info *info = i2c_get_clientdata(client);
        int ser_bitrate = 0;
        int des_bitrate = 0;

	mip4_serdes_reset_dwork_stop(info);
        mutex_lock(&info->serdes.lock);
        if(info->irq_enabled) {
                disable_irq(info->client->irq);
                info->irq_enabled = false;
        }

        ser_bitrate = ser_i2c_bitrate_check(info->client);
        des_bitrate = des_i2c_bitrate_check(info->client);

        printk("Before Ser_Bitrate : 0x%x Des_Bitrate : 0x%x\n",ser_bitrate,des_bitrate);

	des_i2c_bitrate_3gbps_to_6gbps(info->client);
	ser_i2c_bitrate_3gbps_to_6gbps(info->client);

        ser_bitrate = ser_i2c_bitrate_check(info->client);
        des_bitrate = des_i2c_bitrate_check(info->client);

        printk("Now Ser_Bitrate : 0x%x Des_Bitrate : 0x%x\n",ser_bitrate,des_bitrate);

        if(!info->irq_enabled) {
                enable_irq(info->client->irq);
                info->irq_enabled = true;
        }
        mutex_unlock(&info->serdes.lock);
	mip4_serdes_reset_dwork_start(info);

	if(count < INT_MAX)
                return count;
        else
                return INT_MAX;
}

static ssize_t set_serdes_3gbps(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
        struct i2c_client *client = to_i2c_client(dev);
        struct mip4_ts_info *info = i2c_get_clientdata(client);
        int ser_bitrate = 0;
        int des_bitrate = 0;

	mip4_serdes_reset_dwork_stop(info);
        mutex_lock(&info->serdes.lock);
        if(info->irq_enabled) {
                disable_irq(info->client->irq);
                info->irq_enabled = false;
        }

        ser_bitrate = ser_i2c_bitrate_check(info->client);
        des_bitrate = des_i2c_bitrate_check(info->client);

        printk("Before Ser_Bitrate : 0x%x Des_Bitrate : 0x%x\n",ser_bitrate,des_bitrate);

        serdes_i2c_bitrate_6gbps_to_3gbps(info->client);
	mdelay(100);

        ser_bitrate = ser_i2c_bitrate_check(info->client);
        des_bitrate = des_i2c_bitrate_check(info->client);

        printk("Now Ser_Bitrate : 0x%x Des_Bitrate : 0x%x\n",ser_bitrate,des_bitrate);

        if(!info->irq_enabled) {
                enable_irq(info->client->irq);
                info->irq_enabled = true;
        }
        mutex_unlock(&info->serdes.lock);
	mip4_serdes_reset_dwork_start(info);

	if(count < INT_MAX)
                return count;
        else
                return INT_MAX;
}

static ssize_t set_recovery_on(struct device *dev, struct device_attribute *attr, char *buf)
{
        int ret;
        struct i2c_client *client = to_i2c_client(dev);
        struct mip4_ts_info *info = i2c_get_clientdata(client);

	printk("%s [Start]\n",__func__);

	mip4_serdes_reset_dwork_start(info);

	return 0;
}

static ssize_t set_recovery_off(struct device *dev, struct device_attribute *attr, char *buf)
{
        int ret;
        struct i2c_client *client = to_i2c_client(dev);
        struct mip4_ts_info *info = i2c_get_clientdata(client);

	printk("%s [Start]\n",__func__);

	mip4_serdes_reset_dwork_stop(info);

	return 0;
}

static ssize_t set_int_on(struct device *dev, struct device_attribute *attr, char *buf)
{
        int ret;
        struct i2c_client *client = to_i2c_client(dev);
        struct mip4_ts_info *info = i2c_get_clientdata(client);

        printk("%s [Start]\n",__func__);

	if(!info->irq_enabled) {
                enable_irq(info->client->irq);
		info->irq_enabled = true;
        }

	return 0;
}

static ssize_t set_int_off(struct device *dev, struct device_attribute *attr, char *buf)
{
        int ret;
        struct i2c_client *client = to_i2c_client(dev);
        struct mip4_ts_info *info = i2c_get_clientdata(client);

        printk("%s [Start]\n",__func__);

	if(info->irq_enabled) {
                disable_irq(info->client->irq);
		info->irq_enabled = false;
	}

	return 0;
}

static ssize_t show_test(struct device *dev, struct device_attribute *attr, char *buf)
{
        int ret;
        struct i2c_client *client = to_i2c_client(dev);
        struct mip4_ts_info *info = i2c_get_clientdata(client);

        printk("%s [Start]\n",__func__);

        return 0;
}

static ssize_t set_test(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
        int ret;
        struct i2c_client *client = to_i2c_client(dev);
        struct mip4_ts_info *info = i2c_get_clientdata(client);

	unsigned long rem_nsec;

	u64 ts_nsec = local_clock();

	printk("%s [Start]\n",__func__);

	rem_nsec = do_div(ts_nsec, 1000000000);

	printk("[%5lu.%06lu]\n", (unsigned long)ts_nsec, rem_nsec / 1000);

	return count;
}

static ssize_t set_lvds_con(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
        int state;
        struct i2c_client *client = to_i2c_client(dev);
        struct mip4_ts_info *info = i2c_get_clientdata(client);
        u8 buf1[3];
        unsigned short addr;

        if(sscanf(buf, "%d", &state) < 0)
		return -EINVAL;

        printk("%s Start %d\n",__func__,state);

	mip4_serdes_reset_dwork_stop(info);
	mutex_lock(&info->serdes.lock);
        if(info->irq_enabled) {
                disable_irq(info->client->irq);
                info->irq_enabled = false;
        }

	if(state)
	{
		addr = client->addr;

                client->addr = 0x48;
                buf1[0] = 0x01;
                buf1[1] = 0xCF;
                buf1[2] = 0x01;

                i2c_master_send(client, buf1, 3);
                client->addr = addr;
	}
	else
	{
		addr = client->addr;

                client->addr = 0x48;
                buf1[0] = 0x01;
                buf1[1] = 0xCF;
                buf1[2] = 0xC7;

                i2c_master_send(client, buf1, 3);
                client->addr = addr;
	}

	if(!info->irq_enabled) {
                enable_irq(info->client->irq);
                info->irq_enabled = true;
        }
        mutex_unlock(&info->serdes.lock);
	mip4_serdes_reset_dwork_start(info);

	if(count < INT_MAX)
                return count;
        else
                return INT_MAX;
}

static ssize_t show_blu_reg(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
        struct mip4_ts_info *info = i2c_get_clientdata(client);
        u8 buf1[3];
	u8 buf2[3];
        unsigned short addr;

	printk("%s [Start]\n",__func__);

	mip4_serdes_reset_dwork_stop(info);
	mutex_lock(&info->serdes.lock);
        if(info->irq_enabled) {
                disable_irq(info->client->irq);
                info->irq_enabled = false;
        }

	addr = client->addr;

	client->addr = 0x40;
	buf1[0] = 0x02;
	buf1[1] = 0x06;

	i2c_master_send(client, buf1, 2);
        i2c_master_recv(client, buf2, 1);

	printk("SER 0x0206 : 0x%x\n",buf2[0]);

	client->addr = 0x48;
        buf1[0] = 0x02;
        buf1[1] = 0x12;

        i2c_master_send(client, buf1, 2);
        i2c_master_recv(client, buf2, 1);

        printk("DES 0x0212 : 0x%x\n",buf2[0]);

	client->addr = 0x40;
        buf1[0] = 0x02;
        buf1[1] = 0x15;

        i2c_master_send(client, buf1, 2);
        i2c_master_recv(client, buf2, 1);

        printk("SER 0x0215 : 0x%x\n",buf2[0]);
	if((buf2[0]&0x08)==0x08)
		printk("Micom BLU HIGH\n");
	else
		printk("Micom BLU LOW\n");

        client->addr = 0x48;
        buf1[0] = 0x02;
        buf1[1] = 0x15;

        i2c_master_send(client, buf1, 2);
        i2c_master_recv(client, buf2, 1);

        printk("DES 0x0215 : 0x%x\n",buf2[0]);

	if((buf2[0]&0x06)==0x04)
		printk("DES BLU Default");
	else if((buf2[0]&0x06)==0x02)
		printk("DES BLU Forced");
	else
		printk("DES BLU Unknown");

	if((buf2[0]&0x08)==0x08)
		printk(" HIGH\n");
	else
		printk(" LOW\n");

	client->addr = addr;

	if(!info->irq_enabled) {
                enable_irq(info->client->irq);
                info->irq_enabled = true;
        }
        mutex_unlock(&info->serdes.lock);
	mip4_serdes_reset_dwork_start(info);

	return 0;
}

static ssize_t set_blu_con(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
        int state;
        struct i2c_client *client = to_i2c_client(dev);
        struct mip4_ts_info *info = i2c_get_clientdata(client);
        u8 buf1[3];
        unsigned short addr;

        if(sscanf(buf, "%d", &state) < 0)
		return -EINVAL;

        printk("%s Start %d\n",__func__,state);

	mip4_serdes_reset_dwork_stop(info);
	mutex_lock(&info->serdes.lock);
        if(info->irq_enabled) {
                disable_irq(info->client->irq);
                info->irq_enabled = false;
        }

        if(state == 1) //default to micom
        {
                addr = client->addr;

		client->addr = 0x48;
                buf1[0] = 0x02;
                buf1[1] = 0x15;
                buf1[2] = 0x24;

                i2c_master_send(client, buf1, 3);

		client->addr = 0x48;
                buf1[0] = 0x00;
                buf1[1] = 0x10;
                buf1[2] = 0x31;

                i2c_master_send(client, buf1, 3);

                client->addr = addr;
        }
	else if(state == 2) //forced on
	{
		addr = client->addr;

                client->addr = 0x48;
                buf1[0] = 0x02;
                buf1[1] = 0x15;
                buf1[2] = 0x32;

                i2c_master_send(client, buf1, 3);

                client->addr = addr;
	}
        else //forced off
        {
                addr = client->addr;

		client->addr = 0x48;
                buf1[0] = 0x02;
                buf1[1] = 0x15;
                buf1[2] = 0x22;

                i2c_master_send(client, buf1, 3);

                client->addr = addr;
        }

	if(!info->irq_enabled) {
                enable_irq(info->client->irq);
                info->irq_enabled = true;
        }
        mutex_unlock(&info->serdes.lock);
	mip4_serdes_reset_dwork_start(info);

	if(count < INT_MAX)
                return count;
        else
                return INT_MAX;
}

static ssize_t set_touch_log(struct device *dev, struct device_attribute *attr, char *buf)
{
        int ret;
        struct i2c_client *client = to_i2c_client(dev);
        struct mip4_ts_info *info = i2c_get_clientdata(client);
	int ser_bitrate, des_bitrate;
	u8 buf1[3];
        u8 buf2[3];
	u8 rbuf[16];
	unsigned short addr;
	int i;

	printk("\n----------------------------------------------------\n\n");
        printk("%s [Start]\n\n",__func__);

	mip4_serdes_reset_dwork_stop(info);
	mutex_lock(&info->serdes.lock);
        if(info->irq_enabled) {
                disable_irq(info->client->irq);
                info->irq_enabled = false;
        }

	mip4_ts_get_fw_version(info, rbuf);
	printk("F/W Version : %02X.%02X/%02X.%02X/%02X.%02X/%02X.%02X\n\n", rbuf[0], rbuf[1], rbuf[2], rbuf[3], rbuf[4], rbuf[5], rbuf[6], rbuf[7]);

	mobis_touch_counter_print();

	if(!get_montype())
	{
		mobis_show_need_log_count();

		ser_bitrate = ser_i2c_bitrate_check(info->client);
	        des_bitrate = des_i2c_bitrate_check(info->client);

		printk("Ser_bitrate : ");
		if(ser_bitrate == 0x88)
			printk("6Gbps");
		else if(ser_bitrate == 0x84)
			printk("3Gbps");
		else
			printk("Wrong");

		printk(" Des_bitrate : ");
                if(des_bitrate == 0x02)
                        printk("6Gbps");
                else if(des_bitrate == 0x01)
                        printk("3Gbps");
		else if(des_bitrate == 0x81)
			printk("3Gbps 175mV");
                else
                        printk("Wrong");
                printk("\n\n");

		addr = client->addr;

		client->addr = 0x40;
	        buf1[0] = 0x02;
	        buf1[1] = 0x15;

	        i2c_master_send(client, buf1, 2);
	        i2c_master_recv(client, buf2, 1);

	        printk("SER 0x0215 read:0x%02x\n",buf2[0]);
	        if((buf2[0]&0x08)==0x08)
	                printk("Micom BLU HIGH\n");
	        else
	                printk("Micom BLU LOW\n");

	        client->addr = 0x48;
	        buf1[0] = 0x02;
	        buf1[1] = 0x15;

	        i2c_master_send(client, buf1, 2);
	        i2c_master_recv(client, buf2, 1);

	        printk("DES 0x0215 read:0x%02x\n",buf2[0]);

	        if((buf2[0]&0x06)==0x04)
	                printk("DES BLU Default");
	        else if((buf2[0]&0x06)==0x02)
	                printk("DES BLU Forced");
	        else
	                printk("DES BLU Unknown");

	        if((buf2[0]&0x08)==0x08)
	                printk(" HIGH");
	        else
	                printk(" LOW");

	        client->addr = addr;

		printk("\n\n");

		serdes_video_signal_check(info->client, 0);

		printk("\n");

		addr = client->addr;
		for(i = 0; i < 6 ;i++) {
			client->addr = serdes_read[i][0];
			buf1[0] = (serdes_read[i][1] >> 8) & 0xff;
			buf1[1] = serdes_read[i][1] & 0xff;
			buf1[2] = serdes_read[i][2];

			if(client->addr != DES_DELAY)
			{
				i2c_master_send(client, buf1, 2);
				i2c_master_recv(client, buf2, 1);
			}

			if(client->addr == SER_ADDRESS)
				printk("SER ");
			else if(client->addr == DES_ADDRESS)
                                printk("DES ");
			printk("0x%04X write:0x%02X read:0x%02X\n",serdes_read[i][1], serdes_read[i][2], buf2[0]);
		}
		client->addr = addr;

		printk("\n");

		serdes_i2c_show(client);

                printk("\n");
	}

	melfas_touch_cnt = 5;

	mobis_touch_counter_reset();

	printk("----------------------------------------------------\n");

	if(!info->irq_enabled) {
                enable_irq(info->client->irq);
                info->irq_enabled = true;
        }
        mutex_unlock(&info->serdes.lock);
	mip4_serdes_reset_dwork_start(info);

	return 0;
}

static ssize_t set_adm(struct device *dev, const char *buf, size_t count)
{
        int state;
	struct i2c_client *client = to_i2c_client(dev);
        struct mip4_ts_info *info = i2c_get_clientdata(client);
        u8 buf1[3];
        unsigned short addr;

        if(sscanf(buf, "%d", &state) < 0)
		return -EINVAL;

        printk("%s state %d\n",__func__, state);
        if (state == 1) {
                printk("%s Set ADM Mode\n",__func__);
                mip4_serdes_reset_dwork_stop(info);

                mdelay(20);

                printk("%s Set LVDS off Mode\n",__func__);

                addr = client->addr;

                client->addr = 0x48;
                buf1[0] = 0x01;
                buf1[1] = 0xCF;
                buf1[2] = 0xC7;

                i2c_master_send(client, buf1, 3);
                client->addr = addr;
        }
        else{
                return -EINVAL;
        }

	if(count < INT_MAX)
                return count;
        else
                return INT_MAX;
}

static ssize_t mip4_set_drag_test(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
        int state;
        struct i2c_client *client = to_i2c_client(dev);
        struct mip4_ts_info *info = i2c_get_clientdata(client);
        u8 buf1[3];
        unsigned short addr;

        struct input_dev *input = info->input_dev;

        if(sscanf(buf, "%d", &state) < 0)
		return -EINVAL;

        printk("%s Start1\n",__func__);

        input_report_key(input, BTN_TOUCH, 1);

        input_report_key(input, BTN_TOOL_FINGER, 1);

        input_report_abs(input, ABS_MT_TRACKING_ID, 0);

        input_report_abs(input, ABS_MT_POSITION_X, 0x640);

        input_report_abs(input, ABS_MT_POSITION_Y, 0x17e);

        input_report_abs(input, ABS_MT_PRESSURE, 0x7);

        input_sync(input);

        mdelay(16);

        input_report_abs(input, ABS_MT_TRACKING_ID, 0);

        input_report_abs(input, ABS_MT_POSITION_X, state);

        input_sync(input);

        mdelay(16);

        input_report_abs(input, ABS_MT_TRACKING_ID, 0);

        input_report_abs(input, ABS_MT_POSITION_X, state);

        input_sync(input);

        mdelay(16);

        input_report_abs(input, ABS_MT_TRACKING_ID, -1);

        input_sync(input);

	if(count < INT_MAX)
                return count;
        else
                return INT_MAX;
}

ssize_t mip4_set_adm(struct device *dev, const char *buf, size_t count)
{
	return set_adm(dev, buf, count);
}

static ssize_t show_serdes_log(struct device *dev, struct device_attribute *attr, const char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct mip4_ts_info *info = i2c_get_clientdata(client);

	unsigned short addr;
	u8 data[255];

	int i;
	int ret;

	char serdes_log[10];
	char buffer[4];
	int sizeofbuffer, sizeofserdes_log;
	int lock_pin, line_fault;
	memset(serdes_log, 0x00, sizeof(serdes_log));

	if(get_montype())
        {
                printk("%s This function support only departed lcd model which is include Serdes chip\n",__func__);
                return 0;
        }

	printk("%s [Start]\n",__func__);

	mip4_serdes_reset_dwork_stop(info);
	mutex_lock(&info->serdes.lock);

        if(info->irq_enabled) {
                disable_irq(info->client->irq);
                info->irq_enabled = false;
        }

	serdes_log[0] = 0x62;
	serdes_log[1] = 0xF1;
	serdes_log[2] = 0x20;

	lock_pin = serdes_i2c_connect(client);

	serdes_video_signal_check(client, lock_pin);

	line_fault = serdes_line_fault_check(client);

	if(serdes_check_value[1] != 0xE1)
		serdes_log[3] = 0x01;

	if((line_fault == 0x2E) && ((lock_pin&0xfa)!=0xda))
		serdes_log[4] = 0x01;

        if((lock_pin&0xfa)!=0xda)
        {
		serdes_log[5] = 0x01;
        }
        else if((serdes_check_value[0] >= 0x20) || (serdes_check_value[4] >= 0x20))
        {
		serdes_log[5] = 0x01;
        }

	mobis_serdes_status_count_read(buffer);

	sizeofbuffer = sizeof(buffer) / sizeof(char);
	sizeofserdes_log = sizeof(serdes_log) / sizeof(char);

	for(i = 0;i < sizeofbuffer ;i++)
		serdes_log[sizeofserdes_log - sizeofbuffer + i] = buffer[i];

	for(i = 0; i < sizeofserdes_log ; i++)
	{
		printk("Serdes Byte[%d] : %02X\n",i+1,serdes_log[i]);
	}

	snprintf(data, sizeof(data), "%02X%02X%02X%02X%02X%02X%02X\n",serdes_log[3], serdes_log[4], serdes_log[5], serdes_log[6], serdes_log[7], serdes_log[8], serdes_log[9]);

	ret = snprintf(buf, 255, "%s\n", data);

	mobis_serdes_status_count_reset();

	if(!info->irq_enabled) {
                enable_irq(info->client->irq);
                info->irq_enabled = true;
        }
        mutex_unlock(&info->serdes.lock);
	mip4_serdes_reset_dwork_start(info);

	return ret;
}

static DEVICE_ATTR(fw_update, S_IRUGO | S_IWUGO, mip4_ts_sys_fw_update_result, mip4_ts_sys_fw_update);
static DEVICE_ATTR(fw_update_select, S_IRUGO | S_IWUSR | S_IWGRP, mip4_ts_show_list, mip4_ts_sys_fw_update_select);
static DEVICE_ATTR(fw_show_list, S_IRUGO, mip4_ts_show_list, NULL);
static DEVICE_ATTR(8_debug, S_IRUGO | S_IWUSR | S_IWGRP, show_debug, set_debug);
static DEVICE_ATTR(touch_sensitivity,  S_IRUGO | S_IWUSR | S_IWGRP , show_sensitivity, set_sensitivity);
static DEVICE_ATTR(serdes_setting, S_IRUGO | S_IWUSR | S_IWGRP , show_serdes_setting, set_serdes_setting);
static DEVICE_ATTR(des_setting, S_IRUGO | S_IWUSR | S_IWGRP ,NULL, set_des_setting);
static DEVICE_ATTR(recovery_on, S_IRUGO, set_recovery_on, NULL);
static DEVICE_ATTR(recovery_off, S_IRUGO, set_recovery_off, NULL);
static DEVICE_ATTR(read_register, S_IRUGO, show_read_register, NULL);
static DEVICE_ATTR(int_on, S_IRUGO, set_int_on, NULL);
static DEVICE_ATTR(int_off, S_IRUGO, set_int_off, NULL);
static DEVICE_ATTR(test, S_IRUGO | S_IWUSR | S_IWGRP, show_test, set_test);
static DEVICE_ATTR(adm, S_IRUGO | S_IWUSR | S_IWGRP ,NULL, set_adm);
static DEVICE_ATTR(set_serdes_6gbps, S_IRUGO | S_IWUSR | S_IWGRP ,NULL, set_serdes_6gbps);
static DEVICE_ATTR(set_serdes_3gbps, S_IRUGO | S_IWUSR | S_IWGRP ,NULL, set_serdes_3gbps);
static DEVICE_ATTR(linklock_disconnect, S_IRUGO, set_des_bitrate_change, NULL);
static DEVICE_ATTR(drag_test_m, S_IRUGO | S_IWUSR | S_IWGRP ,NULL, mip4_set_drag_test);
static DEVICE_ATTR(lvds_con, S_IRUGO | S_IWUSR | S_IWGRP ,NULL, set_lvds_con);
static DEVICE_ATTR(blu_con, S_IRUGO | S_IWUSR | S_IWGRP ,show_blu_reg, set_blu_con);
static DEVICE_ATTR(touch_log, S_IRUGO, set_touch_log, NULL);
static DEVICE_ATTR(serdes_log, S_IRUGO, show_serdes_log, NULL);

#endif /* CHIP_MODEL */

/*
 * Sysfs attr info
 */
static struct attribute *mip4_ts_attrs[] = {
#if (CHIP_MODEL != CHIP_NONE)
	&dev_attr_fw_update.attr,
	&dev_attr_fw_update_select.attr,
	&dev_attr_fw_show_list.attr,
#endif /* CHIP_MODEL */
	&dev_attr_touch_sensitivity.attr,
	&dev_attr_8_debug.attr,
	&dev_attr_serdes_setting.attr,
	&dev_attr_des_setting.attr,
	&dev_attr_recovery_on.attr,
	&dev_attr_recovery_off.attr,
	&dev_attr_read_register.attr,
	&dev_attr_int_on.attr,
        &dev_attr_int_off.attr,
	&dev_attr_test.attr,
	&dev_attr_adm.attr,
	&dev_attr_set_serdes_6gbps.attr,
	&dev_attr_set_serdes_3gbps.attr,
	&dev_attr_linklock_disconnect.attr,
	&dev_attr_drag_test_m.attr,
	&dev_attr_lvds_con.attr,
	&dev_attr_blu_con.attr,
	&dev_attr_touch_log.attr,
	&dev_attr_serdes_log.attr,
	NULL,
};

/*
 * Sysfs attr group info
 */
static const struct attribute_group mip4_ts_attr_group = {
	.attrs = mip4_ts_attrs,
};


static ssize_t mip4_fops_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
	printk("%s\n", __func__);
	return 0;
}

static ssize_t mip4_fops_write(struct file *file, const char *buf, size_t count, loff_t *ppos)
{
	printk("%s\n", __func__);
	return 0;
}

static int mip4_fops_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int mip4_fops_release(struct inode *inode, struct file *file)
{
	return 0;
}


static long mip4_fops_ioctl(struct file *file, unsigned int cmd, unsigned long _arg)
{
	u8 buf[3];
	int ret;
	
	switch(cmd){
		case DAUDIO_TOUCHIOC_CHECK_RE_CAL:
			/*
			printk("%s DAUDIO_TOUCHIOC_CHECK_RE_CAL\n",__func__);
			wbuf[0] = MIP4_R0_CTRL;
			wbuf[1] = MIP4_R1_CTRL_RECALIBRATE;
			wbuf[2] = 1;
			mip4_ts_i2c_write(info, wbuf, 3);
			*/
			break;
		case DAUDIO_TOUCHIOC_BLU_ON:
			if(MELFAS_LCD_VER == DAUDIOKK_LCD_OI_10_25_1920_720_INCELL_Si_LG)
			{		
				printk("%s DAUDIO_TOUCHIOC_BLU_ON [Start]\n",__func__);

				buf[0] = 0x06;
				buf[1] = 0x12;
				buf[2] = 1;

				ret =  i2c_master_send(info_mip4->client, buf, 3);
			}
			break;
		case DAUDIO_TOUCHIOC_BLU_OFF:
			if(MELFAS_LCD_VER == DAUDIOKK_LCD_OI_10_25_1920_720_INCELL_Si_LG)
			{
				printk("%s DAUDIO_TOUCHIOC_BLU_OFF [Start]\n",__func__);

	                        buf[0] = 0x06;
	                        buf[1] = 0x1F;
	                        buf[2] = 1;

        	                ret =  i2c_master_send(info_mip4->client, buf, 3);
			}
                        break;			
		default:
			break;
	}

	return 0;
}

static const struct file_operations mip4_misc_fops = {
	/**
	* @ legolamp@cleinsoft
	* @ date 2014.05.01
	* @ add mxt336S_fops_write, mxt336S_fops_read, mxt336S_fops_ioctl
	**/
	.unlocked_ioctl = mip4_fops_ioctl,
	.write                  = mip4_fops_write,
	.read                   = mip4_fops_read,
	.open                   = mip4_fops_open,
	.release                = mip4_fops_release,
};

static struct miscdevice mip4_misc = {
	.name = "mxt_misc",
	.fops = &mip4_misc_fops,
	.minor = MISC_DYNAMIC_MINOR,

	.mode = 0660,
};

/*
 * Initialize driver
 */
static int mip4_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct mip4_ts_info *info = NULL;
	struct input_dev *input_dev;
	int ret = 0;

	dev_dbg(&client->dev, "%s [START]\n", __func__);

	/* Check I2C functionality */
	if (!i2c_check_functionality(adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "%s [ERROR] i2c_check_functionality\n", __func__);
		ret = -EIO;
		goto error_i2c;
	}

	/* Init info data */
	info = devm_kzalloc(&client->dev, sizeof(struct mip4_ts_info), GFP_KERNEL);
	input_dev = devm_input_allocate_device(&client->dev);
	if (!info || !input_dev) {
		dev_err(&client->dev, "%s [ERROR]\n", __func__);
		ret = -ENOMEM;
		goto error_info;
	}

	info->client = client;
	info->input_dev = input_dev;
	info->init = true;
	info->power = -1;
	info->irq_enabled = false;
	info->fw_path_ext = kstrdup(FW_PATH_EXTERNAL, GFP_KERNEL);
	mutex_init(&info->lock);
	init_completion(&info->startup_done);
	info->startup_init = false;

	/* Config platform */
	ret = mip4_ts_config_platform(info);
	if (ret) {
		dev_err(&client->dev, "%s [ERROR] mip4_ts_config_platform\n", __func__);
		goto error_info;
	}

	/* Config input device */
	info->input_dev->name = "MELFAS_" CHIP_NAME "_Touchscreen";
	snprintf(info->phys, sizeof(info->phys), "%s/input0", dev_name(&client->dev));
	info->input_dev->phys = info->phys;
	info->input_dev->id.bustype = BUS_I2C;
	info->input_dev->id.vendor = 0x13C5;
	info->input_dev->dev.parent = &client->dev;
#if USE_INPUT_OPEN_CLOSE
	info->input_dev->open = mip4_ts_input_open;
	info->input_dev->close = mip4_ts_input_close;
#endif /* USE_INPUT_OPEN_CLOSE */

	info_mip4 = info;

	/* Set info data */
	input_set_drvdata(input_dev, info);
	i2c_set_clientdata(client, info);

	/* Config regulator */
//	mip4_ts_config_regulator(info);

	mip4_ts_power_off(info);

	/* Firmware update */
	mip4_ts_power_on(info);
#if 0//USE_AUTO_FW_UPDATE
	if(mobis_touch_update_check())
		ret = mip4_ts_fw_update_from_kernel(info, false);
	if (ret) {
		dev_err(&client->dev, "%s [ERROR] mip4_ts_fw_update_from_kernel\n", __func__);
	}
	else
		mobis_touch_update_complete();
//	mip4_ts_power_off(info);
#endif /* USE_AUTO_FW_UPDATE */

	/* Set interrupt handler */
	//ret = devm_request_threaded_irq(&client->dev, client->irq, NULL, mip4_ts_interrupt, IRQF_TRIGGER_LOW | IRQF_ONESHOT, MIP4_TS_DEVICE_NAME, info);
	ret = devm_request_threaded_irq(&client->dev, client->irq, NULL, mip4_ts_interrupt, IRQF_TRIGGER_FALLING | IRQF_ONESHOT, MIP4_TS_DEVICE_NAME, info);

	if (ret) {
		dev_err(&client->dev, "%s [ERROR] request_threaded_irq\n", __func__);
		goto error_irq;
	}

	disable_irq(info->client->irq);
	info->irq_enabled = false;
	info->startup_init = true;

	/* Enable device */
	mip4_ts_enable(info);

	/* Config module */
	ret = mip4_ts_config(info);

	if (ret) {
		dev_err(&client->dev, "%s [ERROR] mip4_ts_config\n", __func__);
		mip4_ts_disable(info);
		LCD_TOUCH_RESET_NEED = 1;
	}

	/* Config input interface */
	mip4_ts_config_input(info);

	/* Register input device */
	ret = input_register_device(input_dev);
	if (ret) {
		dev_err(&client->dev, "%s [ERROR] input_register_device\n", __func__);
		ret = -EIO;
		goto error_device;
	}

	misc_register(&mip4_misc);

#if USE_WAKEUP_GESTURE
	/* Wake-lock for wake-up gesture mode */
	wake_lock_init(&info->wake_lock, WAKE_LOCK_SUSPEND, "mip4_ts_wake_lock");
#endif /* USE_WAKEUP_GESTURE */

#if USE_FB_NOTIFY
	info->fb_notifier.notifier_call = mip4_ts_fb_notifier_callback;
	fb_register_client(&info->fb_notifier);
#endif /* USE_FB_NOTIFY */

	ret = register_reboot_notifier(&mip4_reboot_notifier);
	if(ret != 0){
		pr_err("cannot register reboot notifier (err=%d)\n", ret);
		goto error_notifier;
	}

#if USE_DEV
	/* Create dev node (optional) */
	if (mip4_ts_dev_create(info)) {
		dev_err(&client->dev, "%s [ERROR] mip4_ts_dev_create\n", __func__);
	}

	info->class = class_create(THIS_MODULE, MIP4_TS_DEVICE_NAME);
	device_create(info->class, NULL, info->mip4_ts_dev, NULL, MIP4_TS_DEVICE_NAME);
#endif /* USE_DEV */

#if USE_SYS
	/* Create sysfs for development functions (optional) */
	if (mip4_ts_sysfs_create(info)) {
		dev_err(&client->dev, "%s [ERROR] mip4_ts_sysfs_create\n", __func__);
	}
#endif /* USE_SYS */

#if USE_CMD
	/* Create sysfs for command functions (optional) */
	if (mip4_ts_sysfs_cmd_create(info)) {
		dev_err(&client->dev, "%s [ERROR] mip4_ts_sysfs_cmd_create\n", __func__);
	}
#endif /* USE_CMD */

	/* Create sysfs */
	if (sysfs_create_group(&client->dev.kobj, &mip4_ts_attr_group)) {
		dev_err(&client->dev, "%s [ERROR] sysfs_create_group\n", __func__);
	}
	if (sysfs_create_link(NULL, &client->dev.kobj, "touch_drv")) {
		dev_err(&client->dev, "%s [ERROR] sysfs_create_link\n", __func__);
	}

	mutex_init(&info->serdes.lock);
	INIT_DELAYED_WORK(&info->serdes.reset_dwork, mip4_serdes_reset_dwork);
	mip4_serdes_reset_dwork_start(info);

#if USE_AUTO_FW_UPDATE
	INIT_DELAYED_WORK(&info->upgrade_work, mip4_fw_update_dwork);
	schedule_delayed_work(&info->upgrade_work, 0);
#endif

	dev_dbg(&client->dev, "%s [DONE]\n", __func__);
	dev_info(&client->dev, "MELFAS " CHIP_NAME " Touchscreen\n");
	return 0;
error_notifier:
	unregister_reboot_notifier(&mip4_reboot_notifier);
error_irq:
	free_irq(info->irq, info);
error_device:
	input_unregister_device(info->input_dev);
error_info:
error_i2c:
	dev_dbg(&client->dev, "%s [ERROR]\n", __func__);
	dev_err(&client->dev, "MELFAS " CHIP_NAME " Touchscreen initialization failed.\n");
	return ret;
}

/*
 * Remove driver
 */
static int mip4_ts_remove(struct i2c_client *client)
{
	struct mip4_ts_info *info = i2c_get_clientdata(client);

	unregister_reboot_notifier(&mip4_reboot_notifier);

#if USE_CMD
	mip4_ts_sysfs_cmd_remove(info);
#endif /* USE_CMD */

#if USE_SYS
	mip4_ts_sysfs_remove(info);
#endif /* USE_SYS */

	sysfs_remove_group(&info->client->dev.kobj, &mip4_ts_attr_group);
	sysfs_remove_link(NULL, "touch_drv");

#if USE_DEV
	device_destroy(info->class, info->mip4_ts_dev);
	class_destroy(info->class);
#endif /* USE_DEV */

	mip4_serdes_reset_dwork_stop(info);
	mutex_destroy(&info->serdes.lock);

#if USE_FB_NOTIFY
	fb_unregister_client(&info->fb_notifier);
#endif /* USE_FB_NOTIFY */

	misc_deregister(&mip4_misc);

	input_unregister_device(info->input_dev);

	return 0;
}

#ifdef CONFIG_PM
/*
 * Device suspend event handler
 */
int mip4_ts_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct mip4_ts_info *info = i2c_get_clientdata(client);

	dev_dbg(&client->dev, "%s [START]\n", __func__);

	mip4_ts_disable(info);

	dev_dbg(&client->dev, "%s [DONE]\n", __func__);

	return 0;
}

/*
 * Device resume event handler
 */
int mip4_ts_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct mip4_ts_info *info = i2c_get_clientdata(client);
	int ret = 0;

	dev_dbg(&client->dev, "%s [START]\n", __func__);

	mip4_ts_enable(info);

	dev_dbg(&client->dev, "%s [DONE]\n", __func__);

	return ret;
}


/*
 * PM info
 */
const struct dev_pm_ops mip4_ts_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(mip4_ts_suspend, mip4_ts_resume)
};
#endif /* CONFIG_PM */

#ifdef CONFIG_OF
/*
 * Devicetree match table
 */
static const struct of_device_id mip4_ts_of_match_table[] = {
	{ .compatible = "melfas,mip4_ts", },
	{ },
};
MODULE_DEVICE_TABLE(of, mip4_ts_of_match_table);
#endif /* CONFIG_OF */

#ifdef CONFIG_ACPI
/*
 * ACPI match table
 */
static const struct acpi_device_id mip4_ts_acpi_match_table[] = {
	{ACPI_ID, 0},
	{ },
};
MODULE_DEVICE_TABLE(acpi, mip4_ts_acpi_match_table);
#endif /* CONFIG_ACPI */

/*
 * I2C Device ID
 */
static const struct i2c_device_id mip4_ts_id[] = {
	{MIP4_TS_DEVICE_NAME, 0},
	{},
};
MODULE_DEVICE_TABLE(i2c, mip4_ts_id);

/*
 * I2C driver info
 */
static struct i2c_driver mip4_ts_driver = {
	.id_table = mip4_ts_id,
	.probe = mip4_ts_probe,
	.remove = mip4_ts_remove,
	.driver = {
		.name = MIP4_TS_DEVICE_NAME,
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = of_match_ptr(mip4_ts_of_match_table),
#endif /* CONFIG_OF */
#ifdef CONFIG_ACPI
		.acpi_match_table = ACPI_PTR(mip4_ts_acpi_match_table),
#endif /* CONFIG_ACPI */
#ifdef CONFIG_PM
		.pm = &mip4_ts_pm_ops,
#endif /* CONFIG_PM */
	},
};

#if defined(CONFIG_DAUDIO_KK) && !defined(CONFIG_TOUCHSCREEN_ONEBIN)
int factory_connect_for_lcd(void)
{
        if((!gpio_get_value(TCC_GPB(8)))&&daudio_lcd_version()==DAUDIOKK_LCD_OD_10_25_1920_720_INCELL_LTPS_LG)
                return 1;
        else
                return 0;

        return 0;
}
static int __init mip4_init(void)
{
        int err;

        MELFAS_LCD_VER = daudio_lcd_version();

        printk("%s, GPIO_B24 = %d, GPIO_B13 = %d, GPIO_B8 = %d,  daudio_lcd_version() = %d \n", __func__, gpio_get_value(TCC_GPB(24)), gpio_get_value(TCC_GPB(13)), gpio_get_value(TCC_GPB(8)), daudio_lcd_version());

        if(gpio_get_value(TCC_GPB(24))) // OE
                switch(MELFAS_LCD_VER) {
                        case DAUDIOKK_LCD_OI_10_25_1920_720_INCELL_Si_LG:
                        case DAUDIOKK_LCD_OI_10_25_1920_720_INCELL_Si_2_LG:
                        case DAUDIOKK_LCD_OD_12_30_1920_720_INCELL_Si_LG:
                        case DAUDIOKK_LCD_OI_DISCONNECTED:
                                err = i2c_add_driver(&mip4_ts_driver);
                                if(err)
                                        printk("mip4 touch driver i2c driver add failed (errno = %d)\n", err);
                                return err;
                                break;
                        case DAUDIOKK_LCD_OD_10_25_1920_720_INCELL_LTPS_LG:
                                if(factory_connect_for_lcd())
                                {
                                        err = i2c_add_driver(&mip4_ts_driver);
                                        if(err)
                                                printk("mip4 touch driver i2c driver add failed (errno = %d)\n", err);
                                        return err;
                                        break;
                                }
                        default:
                                return -ENODEV;
                                break;
                }
}
module_init(mip4_init);

static void __exit mip4_cleanup(void)
{
        i2c_del_driver(&mip4_ts_driver);
}
module_exit(mip4_cleanup);
#elif defined(CONFIG_WIDE_PE_COMMON) || defined(CONFIG_TOUCHSCREEN_ONEBIN)
int mip4_init(void)
{
	int err;

	MELFAS_LCD_VER = daudio_lcd_version();

	printk("%s, GPIO_B16 = %d, GPIO_C14 = %d, GPIO_B13 = %d,  daudio_lcd_version() = %d \n", __func__, get_oemtype(), get_montype(), get_factorycheck(), daudio_lcd_version());

	err = i2c_add_driver(&mip4_ts_driver);
	if(err)
		printk("mip4 touch driver i2c driver add failed (errno = %d)\n", err);
	return err;
}
EXPORT_SYMBOL(mip4_init);

void mip4_cleanup(void)
{
	i2c_del_driver(&mip4_ts_driver);
}
EXPORT_SYMBOL(mip4_cleanup);
#endif

MODULE_DESCRIPTION("MELFAS MIP4 Touchscreen");
MODULE_VERSION("2017.05.23");
MODULE_AUTHOR("Sangwon Jee <jeesw@melfas.com>");
MODULE_LICENSE("GPL");

