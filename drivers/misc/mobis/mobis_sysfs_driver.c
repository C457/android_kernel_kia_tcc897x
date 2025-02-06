/*
 *  mobis_sysfs_driver.c
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
*/

#include <linux/spinlock.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/syscalls.h>

#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/sysfs.h>

#include <linux/kernel.h>
#include <linux/timer.h>

#include <linux/rtc.h>

//==========================================================================================
#define TIME_PERIOD	89
#define ALIVE_COUNT_MAX	160

static void mobis_alive_wq (struct work_struct *unused);
static int alive_count=0;
static int alive_max_count=0;
static int alive_status=1;
static struct timer_list mobis_alive_timer;
static DEFINE_MUTEX(mobis_alive_lock);
static DECLARE_WORK(mobis_alivewq, mobis_alive_wq);

static void mobis_alive_wq (struct work_struct *unused)
{
	struct timespec ts;
	struct rtc_time tm;
	struct tm local_tm;
	getnstimeofday(&ts);
	rtc_time_to_tm(ts.tv_sec, &tm);
	time_to_tm(ts.tv_sec, sys_tz.tz_minuteswest * 60 * (-1), &local_tm);
	printk("[%d-%02d-%02d %02d:%02d:%02d UTC],[%02d-%02d %02d:%02d:%02d] kernel alive (%d)...\n",
		tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec,
		local_tm.tm_mon+1, local_tm.tm_mday, local_tm.tm_hour, local_tm.tm_min, local_tm.tm_sec, alive_count);
}

static void mobis_alive_timer_func(unsigned long data)
{
	mutex_lock(&mobis_alive_lock);
	if(alive_status == 1 && alive_count < alive_max_count)
	{
		mobis_alive_timer.expires = jiffies + (TIME_PERIOD*HZ);
		add_timer(&mobis_alive_timer);
		schedule_work(&mobis_alivewq);
		alive_count++;
	}
	if(alive_status == 1 && alive_count == alive_max_count)
	{
		printk("kernel alive and max count reached(%d).\n", alive_count);
	}
	mutex_unlock(&mobis_alive_lock);
}
//==========================================================================================
static ssize_t mobis_alive_status_show(struct kobject *kobj, struct kobj_attribute *attr,
			char *buf)
{
	int ret = 0;
	mutex_lock(&mobis_alive_lock);
	ret = sprintf(buf, "%d", alive_status);
	printk("func=%s, alive_status value=%d\n", __func__, alive_status);
	mutex_unlock(&mobis_alive_lock);
	return ret;
}

static ssize_t mobis_alive_status_store(struct kobject *kobj, struct kobj_attribute *attr,
			 const char *buf, size_t count)
{
	int value = 0;
	mutex_lock(&mobis_alive_lock);
	sscanf(buf, "%d", &value);
	if (value == 1 && alive_status == 0)
	{
		del_timer_sync(&mobis_alive_timer);
		mobis_alive_timer.expires = jiffies + (TIME_PERIOD*HZ);
		add_timer(&mobis_alive_timer);
		schedule_work(&mobis_alivewq);
	}
	alive_status = !!value;
	mutex_unlock(&mobis_alive_lock);	
	return count;
}
//==========================================================================================
static ssize_t mobis_max_count_show(struct kobject *kobj, struct kobj_attribute *attr,
			char *buf)
{
	int ret = 0;
	mutex_lock(&mobis_alive_lock);
	ret = sprintf(buf, "%d", alive_max_count);
	printk("func=%s, alive_max_count value=%d\n", __func__, alive_max_count);
	mutex_unlock(&mobis_alive_lock);
	return ret;
}

static ssize_t mobis_max_count_store(struct kobject *kobj, struct kobj_attribute *attr,
			 const char *buf, size_t count)
{
	mutex_lock(&mobis_alive_lock);
	sscanf(buf, "%d", &alive_max_count);
	mutex_unlock(&mobis_alive_lock);
	return count;
}

//==========================================================================================
static struct kobj_attribute mobis_alive_status_attribute =
	__ATTR(status, 0664, mobis_alive_status_show, mobis_alive_status_store);
static struct kobj_attribute mobis_alive_max_count_attribute =
	__ATTR(max_count, 0664, mobis_max_count_show, mobis_max_count_store);

static struct attribute *alive_attrs[] = {
	&mobis_alive_status_attribute.attr,
	&mobis_alive_max_count_attribute.attr,
	NULL,
};

static struct attribute_group alive_attr_group = {
	.name ="alive",
	.attrs = alive_attrs,
};

//==========================================================================================

static struct kobject *mobis_sysfs_kobj;

static int __init mobis_sysfs_driver_init(void)
{
	int retval;
	mobis_sysfs_kobj = kobject_create_and_add("mobis_sysfs", kernel_kobj);
	if (!mobis_sysfs_kobj)
		return -ENOMEM;

	retval = sysfs_create_group(mobis_sysfs_kobj, &alive_attr_group);
	if (retval)
		kobject_put(mobis_sysfs_kobj);


	alive_max_count = ALIVE_COUNT_MAX;
	init_timer(&mobis_alive_timer);
	mobis_alive_timer.expires = jiffies + (TIME_PERIOD*HZ);
	mobis_alive_timer.data = 0;
	mobis_alive_timer.function = &mobis_alive_timer_func;

	add_timer(&mobis_alive_timer);
	return retval;
}

static void __exit mobis_sysfs_driver_exit(void)
{
	kobject_put(mobis_sysfs_kobj);
	del_timer(&mobis_alive_timer);
}

module_init(mobis_sysfs_driver_init);
module_exit(mobis_sysfs_driver_exit);

MODULE_AUTHOR("BSP Cell <kibum.lee@mobis.co.kr>");
MODULE_DESCRIPTION("MOBIS sysfs driver");
MODULE_LICENSE("GPL");
