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


#include "tcc_cam_i2c.h"
#include <asm/delay.h>
#include <linux/delay.h>
#ifdef CONFIG_I2C
#include <linux/i2c.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#endif
#include "sensor_if.h"
#include <asm/gpio.h>
#include <linux/slab.h>
#include <linux/module.h>

#include "tcc_camera_device.h"

#include <mach/gpio.h>

#include <tcc_cam_cm_control.h>

#define 		I2C_WR		0
#define 		I2C_RD		1

static int debug	= 0;
#define dprintk(msg...)	if(debug) { printk( "tcc_cam_i2c: " msg); }

#ifdef USING_HW_I2C

#include <linux/i2c.h>

static int camera_type = 0;
static const int LVDS_SVM_97 = 4;
static const int LVDS_SVM_111 = 6;
static const int DVRS_RVM = 5;
static const int ADAS_PRK = 7;
static unsigned char camic_error = 0;

struct cam_i2c_chip_info {
	unsigned gpio_start;
	uint16_t reg_output;
	uint16_t reg_direction;

	struct i2c_client *client;
	struct gpio_chip gpio_chip;
};

int gpio_get_camera_variant(void)
{
	int mode0, mode1, mode2;
	int cam_type;

	mode0 = gpio_get_value(TCC_GPB(22));
	mode1 = gpio_get_value(TCC_GPB(19));
	mode2 = gpio_get_value(TCC_GPB(23));

	cam_type = (mode0<<2)|(mode1<<1)|mode2;

	printk("mode0(0x%x), mode1(0x%x), mode2(0x%x), camera_type(0x%x) \n", \
		mode0, mode1, mode2, cam_type);	

	return cam_type;
}
EXPORT_SYMBOL(gpio_get_camera_variant);

int get_camera_type(void)
{
	return camera_type;
}
EXPORT_SYMBOL(get_camera_type);

#if defined(CONFIG_VIDEO_DUAL_CAMERA_SUPPORT)


static const struct i2c_device_id cam_i2c_id0[] = {
	{ "tcc-cam-sensor-0", 0, },
	{ }
};

static const struct i2c_device_id cam_i2c_id1[] = {
	{ "tcc-cam-sensor-1", 1, },
	{ }
};

static struct of_device_id cam0_i2c_of_match[] = {
	{ .compatible = BACK_CAM_I2C_NAME },
	{}
};
MODULE_DEVICE_TABLE(of, cam0_i2c_of_match);

static struct of_device_id cam1_i2c_of_match[] = {
	{ .compatible = FRONT_CAM_I2C_NAME },
	{}
};
MODULE_DEVICE_TABLE(of, cam1_i2c_of_match);

static struct i2c_client *cam_i2c_client[] = { NULL, NULL };

static int cam_i2c_probe0(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct cam_i2c_chip_info 		*chip;

	chip = kzalloc(sizeof(struct cam_i2c_chip_info), GFP_KERNEL);
	if(chip == NULL)
	{
		printk("\n tcc_cam_i2c  :  no chip info. \n");
		return -ENOMEM;
	}

	chip->client = client;
	i2c_set_clientdata(client, chip);
	cam_i2c_client[0] = client;
	printk(KERN_INFO "_______%s() :  addr = 0x%x , client = 0x%p \n", __func__, (client->addr)<<1,cam_i2c_client[0]);

	return 0;
}

static int cam_i2c_remove0(struct i2c_client *client)
{
	struct cam_i2c_chip_info 		*chip  = i2c_get_clientdata(client);

	kfree(chip);
	cam_i2c_client[0] = NULL;
	
	return 0;
}

static int cam_i2c_probe1(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct cam_i2c_chip_info 		*chip;

	chip = kzalloc(sizeof(struct cam_i2c_chip_info), GFP_KERNEL);
	if(chip == NULL)
	{
		printk("\n tcc_cam_i2c  :  no chip info. \n");
		return -ENOMEM;
	}

	chip->client = client;
	i2c_set_clientdata(client, chip);
	cam_i2c_client[1] = client;
	printk(KERN_INFO "_______%s() :  addr = 0x%x , client = 0x%p \n", __func__, (client->addr)<<1,cam_i2c_client[1]);

	return 0;
}

static int cam_i2c_remove1(struct i2c_client *client)
{
	struct cam_i2c_chip_info 		*chip  = i2c_get_clientdata(client);

	kfree(chip);
	cam_i2c_client[1] = NULL;
	
	return 0;
}

static struct i2c_driver cam_i2c_driver0 = {	// I2C driver for Back Camera
	.driver = {
		.name	= "tcc-cam-sensor-0",
		.of_match_table = of_match_ptr(cam0_i2c_of_match),			
	},
	.probe		= cam_i2c_probe0,
	.remove		= cam_i2c_remove0,
	.id_table	= cam_i2c_id0,
};

static struct i2c_driver cam_i2c_driver1 = {	// I2C driver for Front Camera
	.driver = {
		.name	= "tcc-cam-sensor-1",
		.of_match_table = of_match_ptr(cam1_i2c_of_match),			
	},
	.probe		= cam_i2c_probe1,
	.remove		= cam_i2c_remove1,
	.id_table	= cam_i2c_id1,
};


#else // CONFIG_VIDEO_DUAL_CAMERA_SUPPORT

static const struct i2c_device_id cam_i2c_id[] = {
	{ "tcc-cam-sensor", 0, },
	{ }
};

static struct of_device_id cam_i2c_of_match[] = {
	{ .compatible = CAM_I2C_NAME },
	{}
};
MODULE_DEVICE_TABLE(of, cam_i2c_of_match);

static struct i2c_client *cam_i2c_client = NULL;

#if defined(CONFIG_VIDEO_ATV_SENSOR)
struct dentry *sensor_debugfs_dir;

static int atohex (const char *name)
{
	int val = 0;

	for(;; name++)
	{
		switch (*name)
		{
			case '0'...'9':
				val = 16 * val + (*name - '0');
				break;
			case 'A'...'F':
				val = 16 * val + (*name - 'A' + 10);
				break;
			case 'a'...'f':
				val = 16 * val + (*name - 'a' + 10);
				break;
			default:
				return val;
		}
	}
}

static ssize_t show_sensor(struct file *file, char __user *user_buf, size_t count, loff_t *ppos)
{
	int len = 0;
	unsigned char data = 0;
	const struct sensor_reg *next = sensor_initialize;
	char* buf = 0;

	printk("[%s]\n", __func__);

	if(!cam_i2c_client)
		return -ENOMEM;

	if(!(buf = kmalloc(count, GFP_KERNEL)))
		return -ENOMEM;

	memset(buf, 0x0, count);

	while(next->reg != REG_TERM)
	{
		DDI_I2C_Read(next->reg, 1, &data, 1, NULL);

		len += sprintf(buf+len,"REG : 0x%x, VAL : 0x%x\n",next->reg, data);
		next++;
	}

	if(len >= 0)
		len = simple_read_from_buffer(user_buf, count, ppos, buf, len);

	kfree(buf);

	return len;
}

static ssize_t store_sensor(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
	int seperator = 0, i;
	unsigned char *reg, *val, *buf;
	unsigned char data[2];
	struct sensor_reg writereg;

	printk("[%s]\n", __func__);

	if(!(buf = kmalloc(count, GFP_KERNEL)))
		return -ENOMEM;

	if(copy_from_user(buf, user_buf, count))
		return -EFAULT;

	/** reg - 2bytes, seperator - 1byte, val - 2bytes **/
	if(count != 6 || buf[2] != ' ')
	{
		printk("invalid format : 'echo [reg-2bytes] [val-2bytes] > sensor'\n");
		printk("'reg' and 'val' should be inserted as hex format without '0x'\n");
		return -EFAULT;
	}

	if(reg = kmalloc((size_t)3, GFP_KERNEL))
	{
		memcpy(reg, buf, 2);    reg[2] = '\0';
		writereg.reg = (unsigned char)atohex(reg);
		kfree(reg);
	}
	else
	{
		printk("memory allocation fail, return null.\n");
		return -ENOMEM;
	}

	if(val = kmalloc((size_t)3, GFP_KERNEL))
	{
		memcpy(val, &buf[3], 2);    val[2] = '\0';
		writereg.val = (unsigned char)atohex(val);
		kfree(val);
	}
	else
	{
		printk("memory allocation fail, return null.\n");
		return -ENOMEM;
	}

	printk("[Write] REG : 0x%x, VAL : 0x%x\n", writereg.reg, writereg.val);

	/** 1. write register **/
	data[0] = writereg.reg;
	data[1] = writereg.val;

	if(DDI_I2C_Write(data, 1, 1, NULL))
	{
		printk("write error for read!!!! \n");			
		return -EIO; 
	}

	/** 2. read register **/
	DDI_I2C_Read(writereg.reg, 1, &writereg.val, 1, NULL);

	printk("[Result] REG : 0x%x, VAL : 0x%x\n", writereg.reg, writereg.val);

	kfree(buf);

	return count;
}

static const struct file_operations sensor_file_fops = {
	.open = simple_open,
	.read = show_sensor,
	.write = store_sensor,
	.llseek = default_llseek,
};

static ssize_t show_camic_error(struct file *file, char __user *user_buf, size_t count, loff_t *ppos)
{
	int len = 0;
	unsigned char data = 0;
	unsigned int camic_read;
	const struct sensor_reg *next = sensor_initialize;
	char* buf = 0;

	if(!cam_i2c_client)
		return -ENOMEM;

	if(!(buf = kmalloc(count, GFP_KERNEL)))
		return -ENOMEM;

	memset(buf, 0x0, count);

	camic_error = DDI_I2C_Read(0x00, 1, &camic_read, 1, NULL);
    	printk(KERN_INFO "%s() : Camera IC Error Status - (%s)\n", __func__, (camic_error==0)?"NORMAL":"ERROR");

	data = (camic_error==0)?1:0;

	len = sprintf(buf,"%u\n", data);

	if(len >= 0)
		len = simple_read_from_buffer(user_buf, count, ppos, buf, len);

	kfree(buf);

	return len;
}


static const struct file_operations camic_err_file_fops = {
	.open = simple_open,
	.read = show_camic_error,
	.llseek = default_llseek,
};
#endif

#define MAX96706_ID	0x94
#define TW9990_ID	0x88

static int cam_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct cam_i2c_chip_info 		*chip;

	camera_type = gpio_get_camera_variant();

	if(camera_type == LVDS_SVM_97 || camera_type == LVDS_SVM_111 || camera_type == ADAS_PRK || camera_type == DVRS_RVM) {
		client->addr = (MAX96706_ID >> 1);
	}
	else {
		client->addr = (TW9990_ID >> 1);
	}

	chip = kzalloc(sizeof(struct cam_i2c_chip_info), GFP_KERNEL);
	if(chip == NULL)
	{
		printk("\n tcc_cam_i2c  :  no chip info. \n");
		return -ENOMEM;
	}

	chip->client = client;
	i2c_set_clientdata(client, chip);
	cam_i2c_client = client;
	client->flags &= ~(I2C_M_NO_STOP);
	printk(KERN_INFO "%s() : camera type : %d, addr = 0x%x , client = 0x%p \n", \
					__func__, camera_type, \
					(client->addr)<<1, cam_i2c_client);
#if defined(CONFIG_VIDEO_ATV_SENSOR)
	sensor_debugfs_dir = debugfs_create_dir("camera", NULL);
	if(IS_ERR(sensor_debugfs_dir))
	{
		int err = PTR_ERR(sensor_debugfs_dir);
		sensor_debugfs_dir = NULL;
		return err;
	}
	if(debugfs_create_file("sensor", 0664, sensor_debugfs_dir, NULL, &sensor_file_fops))
		debugfs_remove(sensor_debugfs_dir);

	if(debugfs_create_file("camic_error", 0664, sensor_debugfs_dir, NULL, &camic_err_file_fops))
		debugfs_remove(sensor_debugfs_dir);
#endif

	return 0;
}

static int cam_i2c_remove(struct i2c_client *client)
{
	struct cam_i2c_chip_info 		*chip  = i2c_get_clientdata(client);

	kfree(chip);
	cam_i2c_client = NULL;

#if defined(CONFIG_VIDEO_ATV_SENSOR)
	debugfs_remove(sensor_debugfs_dir);
#endif

	return 0;
}

static int cam_i2c_suspend(struct i2c_client *client)
{
	return 0;
}

static int cam_i2c_resume(struct i2c_client *client)
{
	camera_type = gpio_get_camera_variant();

	if(camera_type == LVDS_SVM_97 || camera_type == LVDS_SVM_111 || camera_type == ADAS_PRK || camera_type == DVRS_RVM) {
		client->addr = (MAX96706_ID >> 1);
	}
	else {
		client->addr = (TW9990_ID >> 1);
	}
	printk(KERN_INFO "%s() : camera type : %d, addr = 0x%x , client = 0x%p \n", \
					__func__, camera_type, \
					(client->addr)<<1, cam_i2c_client);
	return 0;
}

static struct i2c_driver cam_i2c_driver = {
	.driver = {
		.name	= "tcc-cam-sensor",
		.of_match_table = of_match_ptr(cam_i2c_of_match),
	},
	.probe		= cam_i2c_probe,
	.remove		= cam_i2c_remove,
	.suspend	= cam_i2c_suspend,
	.resume		= cam_i2c_resume,
	.id_table	= cam_i2c_id,
};

#endif // CONFIG_VIDEO_DUAL_CAMERA_SUPPORT

#endif /* USING_HW_I2C */

#if defined(CONFIG_LVDS_CAMERA)||defined(INCLUDE_LVDS_CAMERA)
void getChipAddress(void)
{
	if(cam_i2c_client !=NULL)
		printk("%s, chip address = 0x%x \n", __func__ ,cam_i2c_client->addr);
}
#endif

int DDI_I2C_Write(unsigned char* data, unsigned short reg_bytes, unsigned short data_bytes, struct tcc_camera_device * vdev)
{
	unsigned short bytes = reg_bytes + data_bytes;

	if(!tcc_cm_ctrl_knock()) {
		#if defined(USING_HW_I2C)
		{
			#if defined(CONFIG_VIDEO_DUAL_CAMERA_SUPPORT)
			{
				if(i2c_master_send(cam_i2c_client[vdev->CameraID], data, bytes) != bytes)
				{
					printk("write error!!!! \n");
					return -EIO; 
				}
			}
			#else // CONFIG_VIDEO_DUAL_CAMERA_SUPPORT
			{
				if(!cam_i2c_client) {
					printk("%s() cam_i2c_client is null !! \n");
					return -ENODEV;
				}

				if(i2c_master_send(cam_i2c_client, data, bytes) != bytes)
				{
					printk("write error!!!! \n");
					return -EIO; 
				}
			}
			#endif // CONFIG_VIDEO_DUAL_CAMERA_SUPPORT
		}
		#endif // USING_HW_I2C
	}
	else
		printk("%s() CM4 is running. Ignore sensor I2C!!!!\n",__func__);

	return 0;
}

int DDI_I2C_Read(unsigned short reg, unsigned char reg_bytes, unsigned char *val, unsigned short val_bytes, struct tcc_camera_device * vdev)
{
	unsigned char data[2];
	
	if(!tcc_cm_ctrl_knock()) {
		if(reg_bytes == 2)
		{
			data[0]= reg>>8;
			data[1]= (u8)reg&0xff;
		}
		else
		{
			data[0]= (u8)reg&0xff;
		}

		#ifdef USING_HW_I2C	
		{
			#if defined(CONFIG_VIDEO_DUAL_CAMERA_SUPPORT)
			{
				if(i2c_master_send(cam_i2c_client[vdev->CameraID], data, reg_bytes) != reg_bytes)
				{
				//	printk("write error for read!!!! \n");			
					return -EIO; 
				}

				if(i2c_master_recv(cam_i2c_client[vdev->CameraID], val, val_bytes) != val_bytes)
				{
					printk("read error!!!! \n");
					return -EIO; 
				}
			}
			#else // CONFIG_VIDEO_DUAL_CAMERA_SUPPORT
			{
				if(!cam_i2c_client) {
					printk("%s() cam_i2c_client is null !! \n");
					return -ENODEV;
				}

				if(i2c_master_send(cam_i2c_client, data, reg_bytes) != reg_bytes)
				{
				//	printk("write error for read!!!! \n");			
					return -EIO; 
				}

				if(i2c_master_recv(cam_i2c_client, val, val_bytes) != val_bytes)
				{
					printk("read error!!!! \n");
					return -EIO; 
				}
			}
			#endif // CONFIG_VIDEO_DUAL_CAMERA_SUPPORT
		}
		#endif
	}
	else
		printk("%s() CM4 is running. Ignore sensor I2C!!!!\n",__func__);

    return 0;
}

static int __init cam_i2c_init(void)
{
	int ret = 0;
	
	camera_type = gpio_get_camera_variant();

	if(camera_type == LVDS_SVM_97 || camera_type == LVDS_SVM_111 || camera_type == DVRS_RVM) {
		dprintk("camera_type : LVDS or DVRS_RVM(%d) \n", camera_type);
	}
	else if(camera_type == ADAS_PRK) {
		dprintk("camera_type : ADAS_PRK(%d) \n", camera_type);
	}
	else {
		dprintk("camera_type : CVBS(%d) \n", camera_type);
	}

	#if defined(CONFIG_VIDEO_DUAL_CAMERA_SUPPORT)
	{
		ret = i2c_add_driver(&cam_i2c_driver0);
		ret = i2c_add_driver(&cam_i2c_driver1);
	}
	#else // CONFIG_VIDEO_DUAL_CAMERA_SUPPORT

	ret = i2c_add_driver(&cam_i2c_driver);

	#endif // CONFIG_VIDEO_DUAL_CAMERA_SUPPORT
	
	return ret;
}
module_init(cam_i2c_init);

static void __exit cam_i2c_exit(void)
{
	#if defined(CONFIG_VIDEO_DUAL_CAMERA_SUPPORT)
	{
		i2c_del_driver(&cam_i2c_driver0);
		i2c_del_driver(&cam_i2c_driver1);
	}
	#else // CONFIG_VIDEO_DUAL_CAMERA_SUPPORT

	i2c_del_driver(&cam_i2c_driver);

	#endif // CONFIG_VIDEO_DUAL_CAMERA_SUPPORT
}
module_exit(cam_i2c_exit);

