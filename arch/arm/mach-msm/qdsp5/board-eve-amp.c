/* arch/arm/mach-msm/amp_ctl.c
 *
 * Copyright (C) 2007 LGE, Inc.
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

#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/device.h>
#include <asm/gpio.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <asm/system.h>

#define MODULE_NAME "amp_max9877"

#define DEBUG_AMP_CTL	1

#if DEBUG_AMP_CTL
#define D(fmt, args...) printk(fmt, ##args)
#else
#define D(fmt, args...) do {} while(0)
#endif
struct amp_dev {
	struct i2c_client *client;
};

static struct amp_dev *_dev = NULL;

int amp_write_register(char reg, char val)
{
	unsigned int err;
	unsigned char buf[2];
	struct i2c_msg	msg = {	
		_dev->client->addr, 0, 2, &buf 
	};

	buf[0] = reg;
	buf[1] = val;
	
	if ((err = i2c_transfer(_dev->client->adapter, &msg, 1)) < 0) {
		dev_err(&_dev->client->dev, "i2c write error\n");
	}else{
		D(KERN_INFO "%s():i2c write ok:%x %x\n", __FUNCTION__, reg, val);
	}

	return 0;
}

int amp_read_register(unsigned char reg, unsigned char *ret)
{

	unsigned int err;
	unsigned char buf = reg;

	struct i2c_msg msg[2] = {
		{ _dev->client->addr, 0, 1, &buf },
		{ _dev->client->addr, I2C_M_RD, 1, ret}
	};

	if ((err = i2c_transfer(_dev->client->adapter, &msg, 2)) < 0) {
		dev_err(&_dev->client->dev, "i2c read error\n");
	}else{
		D(KERN_INFO "%s():i2c read ok:%x\n", __FUNCTION__, reg);
	}

	return 0;
}

static int eve_amp_ctl_probe(struct i2c_client *client)
{
	int err;
	struct amp_dev *dev;
	
	printk(KERN_INFO "%s()\n", __FUNCTION__);
	dev = kzalloc(sizeof (struct amp_dev), GFP_KERNEL);
	if (NULL == dev) {
			return -ENOMEM;
	}
	_dev = dev;
	dev->client = client;
	i2c_set_clientdata(client, dev);
	return 0;
}

static int eve_amp_ctl_remove(struct i2c_client *client)
{
	printk(KERN_INFO "%s()\n", __FUNCTION__);
	i2c_set_clientdata(client, NULL);
	return 0;
}

static struct i2c_device_id eve_amp_idtable[] = {
	{ "amp_max9877", 0 },
};
MODULE_DEVICE_TABLE(i2c, eve_amp_idtable);

static struct i2c_driver eve_amp_ctl_driver = {
	.probe = eve_amp_ctl_probe,
	.remove = eve_amp_ctl_remove,
	.id_table = eve_amp_idtable,
	.driver = {
		.name = MODULE_NAME,
	},
};

static int __init eve_amp_ctl_init(void)
{
	return i2c_add_driver(&eve_amp_ctl_driver);	
}

static void __exit eve_amp_ctl_exit(void)
{
	return i2c_del_driver(&eve_amp_ctl_driver);
}

module_init(eve_amp_ctl_init);
module_exit(eve_amp_ctl_exit);

MODULE_DESCRIPTION("GW620 Amp Control");
MODULE_AUTHOR("Eunhye Kim <kimeh@lge.com>");
MODULE_LICENSE("GPL");
