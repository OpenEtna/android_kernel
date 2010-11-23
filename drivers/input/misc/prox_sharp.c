/* reference : drivers/input/misc/prox_sharp.c
 *
 * Sharp proximity driver
 *
 * Copyright (C) 2008 LGE, Inc.
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
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <asm/gpio.h>
#include <mach/vreg.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>

static struct psensor_dev {
	struct i2c_client *client;
	int gpio;
	int major;
} psdev;

static atomic_t opened = ATOMIC_INIT(-1);

/*
 * Sharp Proximity Sensor Interface
 * */
#define PROX_REG	0
#define GAIN_REG	1
#define HYS_REG		2
#define HYS_MODE_A		0xC2
#define HYS_MODE_B1_VO0	0x40
#define HYS_MODE_B1_VO1	0x20
#define	HYS_MODE_B2_VO0	0x20
#define HYS_MODE_B2_VO1	0x00
#define CYCLE_REG	3
#define	OPMOD_REG	4
#define OPMODE_POWER_ON			(1 << 0)
#define	OPMODE_INTR_MODE		(1 << 1)
#define OPMODE_OPERTATING_MODE  0x01
#define OPMODE_NORMAL_MODE  0x01
#define OPMODE_INTERRUPT_MODE  0x03
#define OPMODE_SHUTDONW_MODE  0x00


#define	CON_REG		6
#define	CON_VOUT_ENABLE		0x00
#define CON_VOUT_DISABLE	0x18

#define PROX_I2C_INT_NO_CLEAR	0

#define	PROX_I2C_INT_CLEAR		0x80

int prox_i2c_write(struct i2c_client *client, u8 addr, u8 value, u8 intr_clr)
{
	return i2c_smbus_write_byte_data(client, addr | intr_clr, value);
}

static int prox_vreg_set(int onoff)
{
	struct vreg *vreg_proximity;
	int rc = -1;

	vreg_proximity = vreg_get(0, "gp6");
	if (onoff)
	{
		vreg_enable(vreg_proximity);
		rc = vreg_set_level(vreg_proximity, 2800);
	}
	else
	{
		rc = vreg_disable(vreg_proximity);
	}
	if (rc != 0) {
		printk("%s failed, err=%d\n", __func__, rc);
		return -1;
	}
	return rc;
}

static int sharp_psensor_init(struct i2c_client *client)
{
	int ret;

	ret = prox_i2c_write(client, GAIN_REG, 0x08, PROX_I2C_INT_NO_CLEAR);
	if(ret)
		return ret;
	ret = prox_i2c_write(client, HYS_REG, HYS_MODE_A,  PROX_I2C_INT_NO_CLEAR);
	if(ret)
		return ret;
	ret = prox_i2c_write(client, CYCLE_REG, 0x04, PROX_I2C_INT_NO_CLEAR);
	if(ret)
		return ret;
	ret = prox_i2c_write(client, OPMOD_REG, OPMODE_NORMAL_MODE, PROX_I2C_INT_NO_CLEAR);
	if(ret)
		return ret;
	ret = prox_i2c_write(client, CON_REG, CON_VOUT_ENABLE, PROX_I2C_INT_NO_CLEAR);
	return ret;
}

static int gp2ap002_suspend(struct i2c_client *client, pm_message_t mesg)
{
	return prox_vreg_set(0);
}

static int gp2ap002_resume(struct i2c_client *client)
{
	int ret;
	struct psensor_dev *pdev = i2c_get_clientdata(client);

	//this is a race condition
	if(atomic_read(&opened) > -1) {
		ret = prox_vreg_set(1);
		if(ret)
			return ret;

		udelay(100);

		ret = sharp_psensor_init(pdev->client);
	}
	return ret;
}

static int device_open(struct inode *inode, struct file *file) {
	int ret;

	if(!atomic_inc_not_zero(&opened))
		return -EBUSY;

	ret = prox_vreg_set(1);
    if(ret)
        return ret;

    udelay(100);

	printk("proximity device_open\n");
    ret = sharp_psensor_init(psdev.client);

	if(ret) {
		prox_vreg_set(0);
		atomic_dec(&opened);
	}
	return ret;
}

static int device_release(struct inode *inode, struct file *file) {

	prox_vreg_set(0);

	atomic_dec(&opened);
	return 0;
}

static ssize_t device_read(struct file *filp,	/* see include/linux/fs.h   */
			   char *buffer,	/* buffer to fill with data */
			   size_t length,	/* length of the buffer     */
			   loff_t * offset) {
	char p;

	if( length == 0 ) return 0;

	p = (char)gpio_get_value(psdev.gpio);;
	put_user(p, buffer);
	return 1;
}

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = device_open,
	.release = device_release,
	.read = device_read,
};

static struct miscdevice proximity_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "proximity",
    .fops = &fops,
};
static int gp2ap002_probe(struct i2c_client *client,
		const struct i2c_device_id *dev_id)
{
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		printk("%s: need I2C_FUNC_I2C\n", __func__);
		return -ENODEV;
		//goto err_check_functionality;
	}

	psdev.client = client;
	i2c_set_clientdata(client, &psdev);
	psdev.gpio = client->irq;

	return misc_register(&proximity_device);
}

static int gp2ap002_remove(struct i2c_client *client)
{
	return 0;
}

static struct i2c_device_id gp2ap002_idtable[] = {
	{ "gp2ap002", 1 },
};

static struct i2c_driver i2c_gp2ap002_driver = {
	.driver = {
		.name = "gp2ap002"
	},
	.probe 	= gp2ap002_probe,
	.remove = __devexit_p(gp2ap002_remove),
	.id_table 	= gp2ap002_idtable,
	.suspend	= gp2ap002_suspend,
	.resume 	= gp2ap002_resume,
};

static int gp2ap002_init(void)
{
	int ret;

	ret = i2c_add_driver(&i2c_gp2ap002_driver);
	if (ret) {
		printk(KERN_ERR"SHARP GP2AP002 : Proximity Sensor Driver Registeration Failed!!\n");
		return ret;
	}

	return 0;
}

static void gp2ap002_exit(void)
{
	i2c_del_driver(&i2c_gp2ap002_driver);
}

module_init(gp2ap002_init);
module_exit(gp2ap002_exit);

MODULE_DESCRIPTION("Sharp Proximity Sensor Driver(gp2ap002)");
MODULE_AUTHOR("Dae il, yu <diyu@lge.com>");
MODULE_LICENSE("GPL");
