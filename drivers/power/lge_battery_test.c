/* drivers/power/gw650_power_test.c
 *
 * Copyright (C) 2008 LGE Inc.
 * Author : Kim, Do-Yeob <dobi77@lge.com>
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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>

struct proc_dir_entry *battery_fp = NULL;
struct proc_dir_entry *b_status_fp = NULL;
struct proc_dir_entry *b_health_fp = NULL;
struct proc_dir_entry *b_present_fp = NULL;
struct proc_dir_entry *b_capacity_fp = NULL;

char battery_status[PAGE_SIZE - 80] = { 0, };
char battery_health[PAGE_SIZE - 80] = { 0, };
char battery_present[PAGE_SIZE - 80] = { 0, };
char battery_capacity[PAGE_SIZE - 80] = { 0, };

extern int set_battery_status(int status);
extern int set_battery_health(int health);
extern int set_battery_present(int present);
extern int set_battery_capacity(int capacity);

static int write_battery_status(struct file *file, const char __user *buf, unsigned long count, void *data)
{
	int len;
	char *real_data;
	int status;

	real_data = (char *) data;

	if (copy_from_user(real_data, buf, count))
		return -EFAULT;

	real_data[count] = '\0';
	len = strlen(real_data);
	if (real_data[len-1] == '\n')
		real_data[--len] = 0;

	status = simple_strtoul(real_data, NULL, 10);
	if (status >= 0 && status <= 4)
		set_battery_status(status);

	switch(status) {
	case 0:
		printk(KERN_INFO "\nbattery status = %d : Unknown\n", status);
		break;
	case 1:
		printk(KERN_INFO "\nbattery status = %d : Charging \n", status);
		break;
	case 2:
		printk(KERN_INFO "\nbattery status = %d : Discharging\n", status);
		break;
	case 3:
		printk(KERN_INFO "\nbattery status = %d : Not charging\n", status);
		break;
	case 4:
		printk(KERN_INFO "\nbattery status = %d : Full\n", status);
		break;
	default:
		printk(KERN_INFO "\nbattery status = Wrong value : 0 ~ 4\n");
		break;
	}

	return count;
}

static int write_battery_health(struct file *file, const char __user *buf, unsigned long count, void *data)
{
	int len;
	char *real_data;
	int health;

	real_data = (char *) data;

	if (copy_from_user(real_data, buf, count))
		return -EFAULT;

	real_data[count] = '\0';
	len = strlen(real_data);
	if (real_data[len-1] == '\n')
		real_data[--len] = 0;

	health = simple_strtoul(real_data, NULL, 10);
	if (health >= 0 && health <= 5)
		set_battery_health(health);

	switch(health) {
	case 0:
		printk(KERN_INFO "\nbattery health = %d : Unknown\n", health);
		break;
	case 1:
		printk(KERN_INFO "\nbattery health = %d : Good \n", health);
		break;
	case 2:
		printk(KERN_INFO "\nbattery health = %d : Overheat\n", health);
		break;
	case 3:
		printk(KERN_INFO "\nbattery health = %d : Dead\n", health);
		break;
	case 4:
		printk(KERN_INFO "\nbattery health = %d : Over voltage\n", health);
		break;
	case 5:
		printk(KERN_INFO "\nbattery health = %d : Unspecified failure\n", health);
		break;
	default:
		printk(KERN_INFO "\nbattery health = Wrong value : 0 ~ 5\n");
		break;
	}

	return count;
}

static int write_battery_present(struct file *file, const char __user *buf, unsigned long count, void *data)
{
	int len;
	char *real_data;
	int present;

	real_data = (char *) data;

	if (copy_from_user(real_data, buf, count))
		return -EFAULT;

	real_data[count] = '\0';
	len = strlen(real_data);
	if (real_data[len-1] == '\n')
		real_data[--len] = 0;

	present = simple_strtoul(real_data, NULL, 10);
	if (present >= 0 && present <= 1)
		set_battery_present(present);

	switch(present) {
	case 0:
		printk(KERN_INFO "\nbattery present = %d : No battery exists.\n", present);
		break;
	case 1:
		printk(KERN_INFO "\nbattery present = %d : battery exists.\n", present);
		break;
	default:
		printk(KERN_INFO "\nbattery present = Wrong value : 0 or 1\n");
		break;
	}

	return count;
}

static int write_battery_capacity(struct file *file, const char __user *buf, unsigned long count, void *data)
{
	int len;
	char *real_data;
	int capacity;

	real_data = (char *) data;

	if (copy_from_user(real_data, buf, count))
		return -EFAULT;

	real_data[count] = '\0';
	len = strlen(real_data);
	if (real_data[len-1] == '\n')
		real_data[--len] = 0;

	capacity = simple_strtoul(real_data, NULL, 10);
	if (capacity >= 0 && capacity <= 100)
		set_battery_capacity(capacity);

	printk(KERN_INFO "\nbattery capacity = %d\n", capacity);

	return count;
}

static int __init test_gw650_power_init(void)
{
	battery_fp = proc_mkdir("battery", 0);

	b_status_fp = create_proc_entry("status", S_IFREG | S_IWUSR, battery_fp);
	if (b_status_fp) {
		b_status_fp->data = &battery_status ; 
		b_status_fp->read_proc = NULL;
		b_status_fp->write_proc = write_battery_status;
	}

	b_health_fp = create_proc_entry("health", S_IFREG | S_IWUSR, battery_fp);
	if (b_health_fp) {
		b_health_fp->data = &battery_health;
		b_health_fp->read_proc = NULL; 
		b_health_fp->write_proc = write_battery_health;
	}

	b_present_fp = create_proc_entry("present", S_IFREG | S_IWUSR, battery_fp);
	if (b_present_fp) {
		b_present_fp->data = &battery_present;
		b_present_fp->read_proc = NULL;
		b_present_fp->write_proc = write_battery_present;
	}

	b_capacity_fp = create_proc_entry("capacity", S_IFREG | S_IWUSR, battery_fp);
	if (b_capacity_fp) {
		b_capacity_fp->data = &battery_capacity;
		b_capacity_fp->read_proc = NULL;
		b_capacity_fp->write_proc = write_battery_capacity;
	}

	return 0;
}

static void __exit test_gw650_power_exit(void)
{
	remove_proc_entry("capacity", battery_fp);
	remove_proc_entry("present", battery_fp);
	remove_proc_entry("health", battery_fp);
	remove_proc_entry("status", battery_fp);
	remove_proc_entry("battery", 0);
}	

module_init(test_gw650_power_init);
module_exit(test_gw650_power_exit);

MODULE_LICENSE("GPL");
