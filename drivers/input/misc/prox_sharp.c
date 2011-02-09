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
#include <linux/wakelock.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
//#include <linux/ioctl.h>
#include <linux/types.h>
//#include <linux/slab.h>
#include <linux/input.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <asm/gpio.h>
#include <mach/vreg.h>
//#include <asm/uaccess.h>

static struct psensor_dev {
	struct i2c_client *client;
	int gpio;
	int enabled;
	int status;
	struct wake_lock wlock;
} psdev;

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
		rc = vreg_set_level(vreg_proximity, 2800);
		vreg_enable(vreg_proximity);
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
	if(ret) {
		printk("%s: prox_i2c_write(GAIN_REG) returned %d\n", __func__, ret);
		return ret;
	}
	ret = prox_i2c_write(client, HYS_REG, HYS_MODE_A,  PROX_I2C_INT_NO_CLEAR);
	if(ret) {
		printk("%s: prox_i2c_write(HYS_REG) returned %d\n", __func__, ret);
		return ret;
	}
	ret = prox_i2c_write(client, CYCLE_REG, 0x04, PROX_I2C_INT_NO_CLEAR);
	if(ret) {
		printk("%s: prox_i2c_write(CYCLE_REG) returned %d\n", __func__, ret);
		return ret;
	}
	ret = prox_i2c_write(client, OPMOD_REG, OPMODE_NORMAL_MODE, PROX_I2C_INT_NO_CLEAR);
	if(ret) {
		printk("%s: prox_i2c_write(OPMOD_REG) returned %d\n", __func__, ret);
		return ret;
	}
	ret = prox_i2c_write(client, CON_REG, CON_VOUT_ENABLE, PROX_I2C_INT_NO_CLEAR);
	if(ret) {
		printk("%s: prox_i2c_write(CON_REG) returned %d\n", __func__, ret);
	}
	return ret;
}

int is_proxi_open(void) {
	return psdev.status;
}
EXPORT_SYMBOL(is_proxi_open);

void sharp_prox_enable(int on) {
	printk("%s: %d -> %d\n", __func__, psdev.enabled, on);
	if( on == psdev.enabled )
		return;

	psdev.enabled = on;
    prox_vreg_set(on);
    udelay(100);
    if(on)
		sharp_psensor_init(psdev.client);

	set_irq_wake(MSM_GPIO_TO_INT(psdev.gpio), on);
}
EXPORT_SYMBOL(sharp_prox_enable);

static irqreturn_t gp2ap002_isr(int o, void *_data) {

    printk("%s: entered, gpio= %d\n", __func__, gpio_get_value(psdev.gpio));

	wake_lock_timeout(&psdev.wlock, 2 * HZ);
	psdev.status = gpio_get_value(psdev.gpio);

	return IRQ_HANDLED;
}

static int gp2ap002_probe(struct i2c_client *client,
		const struct i2c_device_id *dev_id)
{
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		printk("%s: need I2C_FUNC_I2C\n", __func__);
		return -ENODEV;
	}

	psdev.client = client;
	i2c_set_clientdata(client, &psdev);
	psdev.gpio = client->irq;

	psdev.enabled = 0;
	wake_lock_init(&psdev.wlock, WAKE_LOCK_SUSPEND, "prox_sharp");
	request_irq(MSM_GPIO_TO_INT(psdev.gpio), gp2ap002_isr, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "gp2ap002_irq", &psdev);

	return 0;
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
	.probe		= gp2ap002_probe,
	.remove		= __devexit_p(gp2ap002_remove),
	.id_table	= gp2ap002_idtable,
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
