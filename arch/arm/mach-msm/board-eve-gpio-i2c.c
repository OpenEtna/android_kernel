/* linux/arch/arm/mach-msm/board-eve-gpio-i2c.c
 *
 * Copyright (C) 2009 LGE, Inc.
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

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <mach/gpio.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>

/* I2C Bus Num */
#define I2C_BUS_NUM_MOTION	1
#define I2C_BUS_NUM_BACKLIGHT	2
#define I2C_BUS_NUM_CAMERA	3
#define I2C_BUS_NUM_AMP		4
#define I2C_BUS_NUM_TOUCH 	5 
#define I2C_BUS_NUM_PROX 	6
#define I2C_BUS_NUM_COMPASS 	7

/* For Backlight */
#define GPIO_BL_I2C_SCL 84
#define GPIO_BL_I2C_SDA 85

static struct i2c_board_info bl_i2c_device = {
	I2C_BOARD_INFO("backlight", 0xEC >> 1),
};

static struct i2c_gpio_platform_data bl_i2c_adap_pdata = {
	.sda_pin = GPIO_BL_I2C_SDA,
	.sda_is_open_drain = 0,
	.scl_pin = GPIO_BL_I2C_SCL,
	.scl_is_open_drain = 0,
	.udelay = 2,
};

static struct platform_device eve_bl_i2c_adap_device = {
	.name = "i2c-gpio",
	.id = I2C_BUS_NUM_BACKLIGHT,
	.dev.platform_data = &bl_i2c_adap_pdata,
};

int __init eve_init_i2c_backlight(void)
{
	int rc;
	printk(KERN_INFO"%s: \n",__func__);	
	gpio_direction_input(GPIO_BL_I2C_SDA); /* make sda pin high */
	gpio_direction_input(GPIO_BL_I2C_SCL); /* make scl pin high */
	
	rc = i2c_register_board_info(I2C_BUS_NUM_BACKLIGHT, &bl_i2c_device, 1);
	printk(KERN_INFO"%s: %d\n",__func__, rc);	
	return platform_device_register(&eve_bl_i2c_adap_device);
}

/* Proximity Sensor */
#define GPIO_PROX_I2C_SDA 89
#define GPIO_PROX_I2C_SCL 90
#define GPIO_PROX_IRQ 88 /*PROX_OUT*/

static struct i2c_gpio_platform_data prox_i2c_pdata = {
	.sda_pin = GPIO_PROX_I2C_SDA,
	.sda_is_open_drain = 0,
	.scl_pin = GPIO_PROX_I2C_SCL,
	.scl_is_open_drain = 0,
	.udelay = 2,
};

static struct platform_device eve_prox_i2c_device = {
	.name = "i2c-gpio",  /*"i2c-prox",*/
	.id = I2C_BUS_NUM_PROX,
	.dev.platform_data = &prox_i2c_pdata,
};

static struct i2c_board_info prox_i2c_bdinfo = {
	I2C_BOARD_INFO("gp2ap002", 0x44),
	.type = "gp2ap002",
	.irq = MSM_GPIO_TO_INT(GPIO_PROX_IRQ),
};

void __init eve_init_prox(void)
{
	/*gpio_direction_input(GPIO_MOTION_I2C_SDA);
	gpio_direction_input(GPIO_MOTION_I2C_SCL);
	
	i2c_register_board_info(1, &accel_i2c_bdinfo, 1);
	platform_device_register(&eve_accel_i2c_device);*///original

	gpio_configure(GPIO_PROX_I2C_SDA, GPIOF_DRIVE_OUTPUT);
	gpio_set_value(GPIO_PROX_I2C_SDA, 1);
	gpio_configure(GPIO_PROX_I2C_SCL, GPIOF_DRIVE_OUTPUT);
	gpio_set_value(GPIO_PROX_I2C_SCL, 1);

	/*temp3*/printk("diyu1: Function : %s \n", __FUNCTION__);
	i2c_register_board_info(I2C_BUS_NUM_PROX, &prox_i2c_bdinfo, 1);
	platform_device_register(&eve_prox_i2c_device);
}

/* Capacitive Touch (Menu/Back button)*/
//#define GPIO_TOUCH_ATTEN 40 /*REV.A*/
#define GPIO_TOUCH_ATTEN 20 /*REV.B*/
#define GPIO_TOUCH_I2C_SDA 41
#define GPIO_TOUCH_I2C_SCL 42

static struct i2c_gpio_platform_data touch_i2c_pdata = {
	.sda_pin = GPIO_TOUCH_I2C_SDA,
	.sda_is_open_drain = 0,
	.scl_pin = GPIO_TOUCH_I2C_SCL,
	.scl_is_open_drain = 0,
	.udelay = 2,
};

static struct platform_device eve_touch_i2c_device = {
	.name = "i2c-gpio", /*"i2c-touch",*/
	.id = I2C_BUS_NUM_TOUCH,
	.dev.platform_data = &touch_i2c_pdata,
};


static struct i2c_board_info touch_i2c_bdinfo = {
	I2C_BOARD_INFO("touch_so240001", 0x2C),/**/
	.type = "touch_so240001",
	.irq = MSM_GPIO_TO_INT(GPIO_TOUCH_ATTEN),
};

#if 0 /*REV.A*/
static struct i2c_board_info touch_i2c_bdinfo = {
	I2C_BOARD_INFO("touch_so281001", 0x2C),/**/
	.type = "touch_so281001",
	.irq = MSM_GPIO_TO_INT(GPIO_TOUCH_ATTEN),
};
#endif
void __init eve_init_touch(void)
{
	/*gpio_direction_input(GPIO_MOTION_I2C_SDA);
	gpio_direction_input(GPIO_MOTION_I2C_SCL);
	
	i2c_register_board_info(1, &accel_i2c_bdinfo, 1);
	platform_device_register(&adam_accel_i2c_device);*///original

	gpio_configure(GPIO_TOUCH_I2C_SDA, GPIOF_DRIVE_OUTPUT);
	//gpio_set_value(GPIO_TOUCH_I2C_SDA, 1);
	gpio_configure(GPIO_TOUCH_I2C_SCL, GPIOF_DRIVE_OUTPUT);
	//gpio_set_value(GPIO_TOUCH_I2C_SCL, 1);

	/*temp3*/printk("diyu1: Function : %s \n", __FUNCTION__);
	i2c_register_board_info(I2C_BUS_NUM_TOUCH, &touch_i2c_bdinfo, 1);
	platform_device_register(&eve_touch_i2c_device);
}

/*Motion Sensor*/
#define GPIO_MOTION_I2C_SDA 23
#define GPIO_MOTION_I2C_SCL 33
#define GPIO_MOTION_IRQ 49 /*MOTION_INT*/

static struct i2c_gpio_platform_data accel_i2c_pdata = {
	.sda_pin = GPIO_MOTION_I2C_SDA,
	.sda_is_open_drain = 0,
	.scl_pin = GPIO_MOTION_I2C_SCL,
	.scl_is_open_drain = 0,
	.udelay = 2,
};

static struct platform_device eve_accel_i2c_device = {
	.name = "i2c-gpio",
	.id = I2C_BUS_NUM_MOTION,
	.dev.platform_data = &accel_i2c_pdata,
};

static struct i2c_board_info accel_i2c_bdinfo = {
	I2C_BOARD_INFO("bma150",/* "accel_bma150",*/ 0x38),/*SMB380 ,0x38), MMA7660 0x4C original */
	.type = "bma150",/* "accel_bma150",*/
	.irq = MSM_GPIO_TO_INT(GPIO_MOTION_IRQ),
};

void __init eve_init_accel(void)
{
	/*gpio_direction_input(GPIO_MOTION_I2C_SDA);
	gpio_direction_input(GPIO_MOTION_I2C_SCL);
	
	i2c_register_board_info(1, &accel_i2c_bdinfo, 1);
	platform_device_register(&eve_accel_i2c_device);*///original

	gpio_configure(GPIO_MOTION_I2C_SDA, GPIOF_DRIVE_OUTPUT);
	gpio_set_value(GPIO_MOTION_I2C_SDA, 1);
	gpio_configure(GPIO_MOTION_I2C_SCL, GPIOF_DRIVE_OUTPUT);
	gpio_set_value(GPIO_MOTION_I2C_SCL, 1);

	/*temp3*/printk("diyu1: Function : %s \n", __FUNCTION__);
	i2c_register_board_info(I2C_BUS_NUM_MOTION, &accel_i2c_bdinfo, 1);
	platform_device_register(&eve_accel_i2c_device);
}

#define GPIO_CAMERA_I2C_SDA 61
#define GPIO_CAMERA_I2C_SCL 60

static struct i2c_gpio_platform_data eve_camera_i2c_adap_pdata =
{
    .sda_pin= GPIO_CAMERA_I2C_SDA,
    .sda_is_open_drain = 0,
    .scl_pin = GPIO_CAMERA_I2C_SCL,
    .scl_is_open_drain = 0,
    .udelay = 2,
};

static struct platform_device eve_camera_i2c_adap_device =
{
    .name= "i2c-gpio",
    .id = I2C_BUS_NUM_CAMERA,
    .dev.platform_data = &eve_camera_i2c_adap_pdata,
};

static struct i2c_board_info eve_camera_i2c_device[] =
{
    { I2C_BOARD_INFO("mv9319", 0x50 >> 1) },
    { I2C_BOARD_INFO("mv9319_firmware", 0x40 >> 1) },
};

void __init eve_init_i2c_camera(void)
{
    gpio_direction_input(GPIO_CAMERA_I2C_SDA); // make sda pin high
    gpio_direction_input(GPIO_CAMERA_I2C_SCL); // make scl pin high

    i2c_register_board_info(I2C_BUS_NUM_CAMERA, &eve_camera_i2c_device, 2);
    platform_device_register(&eve_camera_i2c_adap_device);
}

#define GPIO_AMP_I2C_SCL		27		/* GPIO_27 */
#define GPIO_AMP_I2C_SDA		17		/* GPIO_17 */

static struct i2c_gpio_platform_data amp_i2c_pdata = {
	.sda_pin = GPIO_AMP_I2C_SDA,
	.sda_is_open_drain = 0,
	.scl_pin = GPIO_AMP_I2C_SCL,
	.scl_is_open_drain = 0,
	.udelay = 2
};

static struct platform_device eve_amp_i2c_device = {
	.name = "i2c-gpio",
	.id = I2C_BUS_NUM_AMP,
	.dev.platform_data = &amp_i2c_pdata,
};

static struct i2c_board_info amp_i2c_bdinfo = {
	I2C_BOARD_INFO("amp_max9877", 0x9A>>1), 
};

void __init eve_init_i2c_amp(void)
{
	gpio_configure(GPIO_AMP_I2C_SDA, GPIOF_DRIVE_OUTPUT);
	gpio_configure(GPIO_AMP_I2C_SCL, GPIOF_DRIVE_OUTPUT);

	i2c_register_board_info(I2C_BUS_NUM_AMP, &amp_i2c_bdinfo, 1);
	platform_device_register(&eve_amp_i2c_device);
}

#define	GPIO_COMPASS_I2C_SCL	1	
#define	GPIO_COMPASS_I2C_SDA	2
#define	GPIO_COMPASS_IRQ	80
#define GPIO_COMPASS_RESET	91

static struct i2c_gpio_platform_data compass_i2c_pdata = {
	.sda_pin = GPIO_COMPASS_I2C_SDA,
	.sda_is_open_drain = 0,
	.scl_pin = 	GPIO_COMPASS_I2C_SCL,
	.scl_is_open_drain = 0,
	.udelay = 2
};

static struct platform_device eve_compass_i2c_device = {
	.name = "i2c-gpio",
	.id = I2C_BUS_NUM_COMPASS,
	.dev.platform_data = &compass_i2c_pdata,
};

static struct i2c_board_info compass_i2c_bdinfo = {
	I2C_BOARD_INFO("akm8973", 0x38 >> 1),
	.irq = MSM_GPIO_TO_INT(GPIO_COMPASS_IRQ),
};

void __init eve_init_i2c_compass(void)
{
	/*temp3*/printk("Function : %s start.\n", __FUNCTION__);
	
	gpio_configure(GPIO_COMPASS_I2C_SCL, GPIOF_DRIVE_OUTPUT);
	gpio_set_value(GPIO_COMPASS_I2C_SCL, 1);
	gpio_configure(GPIO_COMPASS_I2C_SDA, GPIOF_DRIVE_OUTPUT);
	gpio_set_value(GPIO_COMPASS_I2C_SDA, 1);
	gpio_configure(GPIO_COMPASS_IRQ, GPIOF_DRIVE_OUTPUT);
	gpio_set_value(GPIO_COMPASS_IRQ, 1);
	gpio_configure(GPIO_COMPASS_RESET, GPIOF_DRIVE_OUTPUT);
	gpio_set_value(GPIO_COMPASS_RESET, 1);
	
	/*temp3*/printk("Function : %s processing.\n", __FUNCTION__);
	
	i2c_register_board_info(I2C_BUS_NUM_COMPASS, &compass_i2c_bdinfo, 1);
	platform_device_register(&eve_compass_i2c_device);
}

