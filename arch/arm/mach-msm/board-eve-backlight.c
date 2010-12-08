/* drivers/video/backlight/bl_bd6083.c
 *
 * Copyright (C) 2008 LGE, Inc
 * Author: Seung-Ho Park <parksh03@lge.com>
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
#include <linux/delay.h>
#include <linux/backlight.h>
#include <linux/fb.h>
#include <linux/i2c.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <asm/gpio.h>
#include <asm/system.h>
#include <linux/earlysuspend.h>
#include <mach/system.h>

#define BL_SLAVE_ADDRESS	0xEC
#define I2C_NO_REG			0xFF

#define GPIO_LCD_BL_EN		34

#define MAX_BRIGHTNESS		0x63	/* 99 */
#define DEFAULT_BRIGHTNESS	0x40	/* 64 */

#define MODULE_NAME    "bl_bd6083"

/* reset, lowest bit */
#define BG6083_REG_RESET 0x00

#define BG6083_REG_LEDPIN 0x01
#define BG6083_W6MD		1 << 3
#define BG6083_W5MD     1 << 2
#define BG6083_W4MD     1 << 1
#define BG6083_MLEDMD	1 << 0

#define BD6083_REG_LEDPWRCTL 0x02
#define BD6083_MLEDEN	1 << 0
#define BD6083_ALCEN	1 << 6

#define BD6083_REG_MGC  0x03
/* lower 7 bits */

/* main current transition */
#define BD6083_REG_MCT	0x09
/* lower 4 bit are	TLH, upper are THL */
#define BD6083_VAL_MCT(TLH,THL) (TLH | (THL << 4))

/* measurement mode settings */
#define BD6083_REG_MM	0x0a
#define BD6083_SBIASON	1 << 0

/* alc slope curve settings */
#define BD6083_REG_ASC	0x0b

/* led current at ambient level */
#define BD6083_REG_LEDA 0x0d
/* lower 7 bits */

#define BD6083_REG_LEDMAX 0x0e
/* lower 7 bits */

#define BD6083_REG_LDO12 0x14
/* lower 4 bits: LDO1 */
/* uppwer 4 bits: LDO2 */

#define BD6083_REG_LDO34 0x15
/* lower 4 bits: LDO3 */
/* uppwer 4 bits: LDO4 */


#ifdef DEBUG_INFO
#define D(fmt, args...) printk(fmt, ##args)
#else
#define D(fmt, args...) do {} while (0)
#endif

#ifdef DEBUG_FUNCTION
#define D_FUNC(fmt, args...) printk(fmt, ##args)
#else
#define D_FUNC(fmt, args...) do {} while (0)
#endif

struct bd6083_device {
	struct i2c_client *client;
	struct backlight_device *bl_dev;
	struct work_struct work;
};

struct bl_init_table_s {
	unsigned short reg;
	unsigned short val;
};

enum {
	BL_CAMERA_ON,
	BL_CAMERA_OFF
};

#define MDDI_DEBUG

#ifdef MDDI_DEBUG
enum {
	NORMAL_STATE = 1,
	SUSPEND_STATE = 0,
};

static int power_state_flag = NORMAL_STATE;
extern void force_pan(void);
#endif

unsigned int cur_camera_mode = BL_CAMERA_OFF;

static DEFINE_MUTEX(bl_lock);

static int bd6083_write_reg(struct i2c_client *client, unsigned char reg,
			    unsigned char val);
static int bd6083_read_reg(struct i2c_client *client, unsigned char reg,
			   unsigned char *ret);
static int eve_bl_set_intensity(struct backlight_device *bd);
static int eve_bl_get_intensity(struct backlight_device *bd);

#ifdef CONFIG_HAS_EARLYSUSPEND
static struct early_suspend bl_early_suspend;
static void bd6083_early_suspend(struct early_suspend *h);
static void bd6083_late_resume(struct early_suspend *h);
#endif

unsigned int cur_main_lcd_level = DEFAULT_BRIGHTNESS;

static struct bd6083_device *bd6083_dev = NULL;

#if 0
static struct bl_init_table_s bd6083_normal_mode_config[] = {
	// Normal Mode(Non ALC Mode)
	{0x00, 0x00},
	{0x14, 0xc0},		/*LDO2 Vout Control for the vibrator: 3.0V *//*diyu@lge.com */
	{0x01, 0x0f},
	{0x03, 0x40},
	{0x09, 0x67},
	{0x0a, 0x01},
	{0x0b, 0x13},
	{0x0d, 0x3b},
	{0x0e, 0x63},
	// {0x02, 0x40},
	// {ALC_DELAY, 0x00},
	// {0x02, 0x01},
	{I2C_NO_REG, 0x00}	/* End of array */
};

void bd6083_hreset(void)
{
	D_FUNC("%s()\n", __FUNCTION__);

	printk("blight_reset()\n");
	gpio_set_value(GPIO_LCD_BL_EN, 1);
	udelay(10);
	gpio_set_value(GPIO_LCD_BL_EN, 0);
	udelay(10);
	gpio_set_value(GPIO_LCD_BL_EN, 1);
	udelay(10);
}
#endif

static int bd6083_write_reg(struct i2c_client *client, unsigned char reg,
			    unsigned char val)
{
	int err;
	unsigned char buf[2];
	struct i2c_msg msg = {
		client->addr, 0, 2, buf
	};

	D_FUNC("%s() reg : %x, val : %x\n", __FUNCTION__, reg, val);

	buf[0] = reg;
	buf[1] = val;

	if ((err = i2c_transfer(client->adapter, &msg, 1)) < 0) {
		dev_err(&client->dev, "i2c write error\n");
	}

	return 0;
}


#define ZDBG(fmt, args...) do {} while (0)

#define MAINTAIN_LDO_REGISTER_STATUS 1
#if MAINTAIN_LDO_REGISTER_STATUS
/* XXX: workaround to maintain current register value because i2c_read returns always zero. */
#define INDEX_13H_POWER_CONTROL     0
#define INDEX_14H_12VOUT_CONTROL    1
#define INDEX_15H_34VOUT_CONTROL    2

static unsigned char bd6083_ldo_reg_stats[3];
#endif /* MAINTAIN_LDO_REGISTER_STATUS */

int bd6083_set_ldo_power(int ldo_no, int on)
{
	unsigned char tmp;
	unsigned char new_val;
	printk("%s: ldo_no[%d], on/off[%d]\n", __func__, ldo_no, on);

	if (ldo_no > 0 && ldo_no < 5)
		new_val = 1 << (ldo_no - 1);
	else
		return -1;

	if (bd6083_dev && bd6083_dev->client) {
#if MAINTAIN_LDO_REGISTER_STATUS
		tmp = bd6083_ldo_reg_stats[INDEX_13H_POWER_CONTROL];
#else
		/* TODO: check return value */
		rc = bd6083_read_reg(bd6083_dev->client, 0x13, &tmp);
		printk("%s: current ldo power register value 0x%x\n",
		     __func__, tmp);
#endif /* MAINTAIN_LDO_REGISTER_STATUS */

		if (on)
			tmp |= new_val;
		else
			tmp &= ~new_val;

		printk("%s: new ldo power register value 0x%x\n", __func__, tmp);
		bd6083_write_reg(bd6083_dev->client, 0x13, tmp);
#if MAINTAIN_LDO_REGISTER_STATUS
		bd6083_ldo_reg_stats[INDEX_13H_POWER_CONTROL] = tmp;
#endif

		{
			tmp = 0;
			bd6083_read_reg(bd6083_dev->client, 0x13, &tmp);
			printk("%s: VERIFY register[0x13], value[0x%x]\n",
			     __func__, tmp);
		}

		return 0;
	} else {
		return -1;
	}
}

struct ldo_vout_struct {
	unsigned char reg_value;
	unsigned mv;
};

static struct ldo_vout_struct ldo_vout_table[] = {
	{ /* 0000 */ 0x00, 1200},
	{ /* 0001 */ 0x01, 1300},
	{ /* 0010 */ 0x02, 1500},
	{ /* 0011 */ 0x03, 1600},
	{ /* 0100 */ 0x04, 1800},
	{ /* 0101 */ 0x05, 2200},
	{ /* 0110 */ 0x06, 2400},
	{ /* 0111 */ 0x07, 2500},
	{ /* 1000 */ 0x08, 2600},
	{ /* 1001 */ 0x09, 2700},
	{ /* 1010 */ 0x0A, 2800},
	{ /* 1011 */ 0x0B, 2900},
	{ /* 1100 */ 0x0C, 3000},
	{ /* 1101 */ 0x0D, 3100},
	{ /* 1110 */ 0x0E, 3200},
	{ /* 1111 */ 0x0F, 3300},
	{ /* Invalid */ 0xFF, 0},
};

static unsigned char bd6083_find_ldo_vout_reg_value(unsigned mv)
{
	int i = 0;
	do {
		if (ldo_vout_table[i].mv == mv)
			return ldo_vout_table[i].reg_value;
		else
			i++;
	} while (ldo_vout_table[i].mv != 0);

	return ldo_vout_table[i].reg_value;
}

int bd6083_set_ldo_vout(int ldo_no, unsigned vout_mv)
{
	unsigned char target_reg;
	unsigned char reg_val;
	unsigned char tmp;

	printk("%s: ldo_no[%d], mV[%u]\n", __func__, ldo_no, vout_mv);

	switch (ldo_no) {
	case 1:
	case 2:
		target_reg = 0x14;
#if MAINTAIN_LDO_REGISTER_STATUS
		tmp = bd6083_ldo_reg_stats[INDEX_14H_12VOUT_CONTROL];
#endif
		break;
	case 3:
	case 4:
		target_reg = 0x15;
#if MAINTAIN_LDO_REGISTER_STATUS
		tmp = bd6083_ldo_reg_stats[INDEX_15H_34VOUT_CONTROL];
#endif
		break;
	default:
		return -1;
	}

	reg_val = bd6083_find_ldo_vout_reg_value(vout_mv);
	if (reg_val == 0xFF)
		return -1;
	printk("%s: found vout register value 0x%x\n", __func__, reg_val);

	if (bd6083_dev->client != NULL) {
#if !MAINTAIN_LDO_REGISTER_STATUS
		bd6083_read_reg(bd6083_dev->client, target_reg, &tmp);
#endif /* MAINTAIN_LDO_REGISTER_STATUS */

		switch (ldo_no) {
		case 1:
			tmp = tmp & 0xF0;
			break;
		case 2:
			tmp = tmp & 0x0F;
			reg_val = reg_val << 4;
			break;
		case 3:
			tmp = tmp & 0xF0;
			break;
		case 4:
			tmp = tmp & 0x0F;
			reg_val = reg_val << 4;
			break;
		default:
			return -1;
		}

		tmp = tmp | reg_val;

		printk("%s: target register[0x%x], value[0x%x]\n", __func__,
		     target_reg, tmp);
		bd6083_write_reg(bd6083_dev->client, target_reg, tmp);
#if MAINTAIN_LDO_REGISTER_STATUS
		if (target_reg == 0x14)
			bd6083_ldo_reg_stats[INDEX_14H_12VOUT_CONTROL] = tmp;
		else
			bd6083_ldo_reg_stats[INDEX_15H_34VOUT_CONTROL] = tmp;
#endif

		{
			tmp = 0;
			bd6083_read_reg(bd6083_dev->client, target_reg, &tmp);
			printk("%s: VERIFY register[0x%x], value[0x%x]\n",
			     __func__, target_reg, tmp);

		}
		return 0;
	} else {
		return -1;
	}
}

EXPORT_SYMBOL(bd6083_set_ldo_power);
EXPORT_SYMBOL(bd6083_set_ldo_vout);

static int bd6083_read_reg(struct i2c_client *client, unsigned char reg,
			   unsigned char *ret)
{
	int err;
	unsigned char buf = reg;

	struct i2c_msg msg[2] = {
		{client->addr, 0, 1, &buf},
		{client->addr, I2C_M_RD, 1, ret}
	};

	D_FUNC("%s()\n", __FUNCTION__);

	if ((err = i2c_transfer(client->adapter, msg, 2)) < 0) {
		printk("%s: i2c error %d\n",__func__,err);
		return err;
	} else {
		return 0;
	}
}

#if 0
void bd6083_init(struct i2c_client *client)
{
	unsigned int i;
	int ret;

	D_FUNC("%s()\n", __FUNCTION__);

	for (i = 0; bd6083_normal_mode_config[i].reg != I2C_NO_REG; i++) {
		ret =
		    bd6083_write_reg(client, bd6083_normal_mode_config[i].reg,
				     bd6083_normal_mode_config[i].val);
		D("init register : %x,  %x\n", bd6083_normal_mode_config[i].reg,
		  bd6083_normal_mode_config[i].val);
	}
	bd6083_write_reg(client, 0x02, 0x01);

}
#endif

static void bd6083_set_main_current_level(struct i2c_client *client, int level)
{
	D_FUNC("%s()\n", __FUNCTION__);

	cur_main_lcd_level = level;

	//bd6083_write_reg(client, 0x0d, level);	// registry 0x0d , value : level(hex)
	bd6083_write_reg(client, BD6083_REG_MGC, level & 0x7f);
}

static void bd6083_set_camera_on_mode_internal(void);
static void bd6083_init_register(struct i2c_client *client)
{
	int ret;

	if (cur_camera_mode == BL_CAMERA_ON) {
		bd6083_set_camera_on_mode_internal();
		return;
	}
	mutex_lock(&bl_lock);
	ret = bd6083_write_reg(client, BG6083_REG_RESET, 0x00);
	ret = bd6083_write_reg(client, BD6083_REG_LDO12, 0xc0); /* 3 V for vibator */
	ret = bd6083_write_reg(client, BG6083_REG_LEDPIN, BG6083_W6MD | BG6083_W5MD | BG6083_W4MD);
	ret = bd6083_write_reg(client, BD6083_REG_MGC, 0x40);
	ret = bd6083_write_reg(client, BD6083_REG_MCT, BD6083_VAL_MCT(0x7,0x8));
	ret = bd6083_write_reg(client, BD6083_REG_MM, BD6083_SBIASON);
	ret = bd6083_write_reg(client, BD6083_REG_ASC, 0x00);
	ret = bd6083_write_reg(client, BD6083_REG_LEDA, 0x14);
	ret = bd6083_write_reg(client, BD6083_REG_LEDMAX, 0x2b);
	mdelay(82);
	ret = bd6083_write_reg(client, BD6083_REG_LEDPWRCTL, BD6083_ALCEN | BD6083_MLEDEN);
	mutex_unlock(&bl_lock);
}

#if 0
static void bd6083_set_normal_mode(struct i2c_client *client)
{
	int ret;
	//unsigned long flags;

	D_FUNC("%s()\n", __FUNCTION__);
	mutex_lock(&bl_lock);
	if (lge_hw_rev > 7) {
		ret = bd6083_write_reg(client, 0x00, 0x00);
		ret = bd6083_write_reg(client, 0x01, 0x0f);
		ret = bd6083_write_reg(client, 0x03, 0x40);
		ret = bd6083_write_reg(client, 0x09, 0x87);
		ret = bd6083_write_reg(client, 0x0d, 0x02);
		ret = bd6083_write_reg(client, 0x0e, 0x68);
	} else {
		ret = bd6083_write_reg(client, 0x00, 0x00);
		ret = bd6083_write_reg(client, 0x01, 0x0f);
		ret = bd6083_write_reg(client, 0x03, 0x40);
		ret = bd6083_write_reg(client, 0x09, 0x87);
		ret = bd6083_write_reg(client, 0x0d, 0x13);
		ret = bd6083_write_reg(client, 0x0e, 0x4f);
	}

	ret = bd6083_write_reg(client, 0x02, 0x01);
	mutex_unlock(&bl_lock);
	cur_alc_mode = BL_MODE_NORMAL;
	/* XXX: This should be the last because it can change cur_alc_mode to BL_MODE_ALC
	 * So we avoid that cur_alc_mode value is overwritten */
	ret = eve_bl_set_intensity(bd6083_dev->bl_dev);
}
#endif
static void bd6083_set_camera_on_mode_internal(void)
{
	int ret;
	//unsigned long flags;

	D_FUNC("%s()\n", __FUNCTION__);

	mutex_lock(&bl_lock);
		ret = bd6083_write_reg(bd6083_dev->client, 0x00, 0x00);
		ret = bd6083_write_reg(bd6083_dev->client, 0x01, 0x0f);
		ret = bd6083_write_reg(bd6083_dev->client, 0x03, 0x40);
		ret = bd6083_write_reg(bd6083_dev->client, 0x09, 0x87);
		ret = bd6083_write_reg(bd6083_dev->client, 0x0d, 0x13);
		ret = bd6083_write_reg(bd6083_dev->client, 0x0e, 0x40);
	ret = bd6083_write_reg(bd6083_dev->client, 0x02, 0x01);
	mutex_unlock(&bl_lock);
}
int bd6083_set_camera_on_mode(void)
{

	D_FUNC("%s()\n", __FUNCTION__);

	bd6083_set_camera_on_mode_internal();
	cur_camera_mode = BL_CAMERA_ON;

	return 0;
}

int bd6083_set_camera_off_mode(void)
{
	D_FUNC("%s()\n", __FUNCTION__);

	/* Case 1: Camera close/Sleep in Normal Backlight Mode */
	if (cur_camera_mode == BL_CAMERA_OFF)
		return 0;

	cur_camera_mode = BL_CAMERA_OFF;
	bd6083_init_register(bd6083_dev->client);

	return 0;
}

EXPORT_SYMBOL(bd6083_set_camera_on_mode);
EXPORT_SYMBOL(bd6083_set_camera_off_mode);

static int eve_bl_set_intensity(struct backlight_device *bd)
{
	struct i2c_client *client = to_i2c_client(bd->dev.parent);

	printk("%s() cur_main_lcd_level : %d, level : %d\n", __FUNCTION__,
	  cur_main_lcd_level, bd->props.brightness);

	bd6083_set_main_current_level(client,
						      bd->props.brightness);
	return 0;
}

static int eve_bl_get_intensity(struct backlight_device *bd)
{
	D_FUNC("%s()\n", __FUNCTION__);

	cur_main_lcd_level = bd->props.brightness;
	return cur_main_lcd_level;
}

ssize_t lcd_backlight_show_alc_brightness(struct device * dev,
					  struct device_attribute * attr,
					  char *buf)
{
	int err, r;
	struct i2c_client *client = to_i2c_client(dev->parent);
	unsigned char ret, level;

	D_FUNC("%s()\n", __FUNCTION__);

	err = bd6083_read_reg(client, 0x0C, &ret);
	level = ret;

	D_FUNC("%s() alc_brightness : %d\n", __FUNCTION__, level);

	//printk("%s() reg:0x0c, value:0x%x\n", __func__, ret);
	r = snprintf((char *)buf, PAGE_SIZE, "%d\n", level);

	return r;
}

ssize_t lcd_backlight_onoff(struct device * dev, struct device_attribute * attr,
			    const char *buf, size_t count)
{
	int ret, onoff;		// = simple_strtol(buf, NULL, count);

	D_FUNC("%s()\n", __FUNCTION__);
	printk(KERN_INFO "%s: ######################\n", __func__);

	ret = eve_bl_get_intensity(bd6083_dev->bl_dev);
	sscanf(buf, "%d", &onoff);

	if (onoff) {
		D_FUNC("%s(): onoff - %d\n", __FUNCTION__, onoff);

		bd6083_write_reg(bd6083_dev->client, 0x02, 0x41);
		ret = eve_bl_set_intensity(bd6083_dev->bl_dev);
	} else {

		D_FUNC("%s(): onoff - %d\n", __FUNCTION__, onoff);
		bd6083_write_reg(bd6083_dev->client, 0x02, 0x00);
	}

	return ret;
}

DEVICE_ATTR(alc_brightness, 0666, lcd_backlight_show_alc_brightness, NULL);
DEVICE_ATTR(bl_onoff, 0666, NULL, lcd_backlight_onoff);

static struct backlight_ops eve_bl_ops = {
	.update_status = eve_bl_set_intensity,
	.get_brightness = eve_bl_get_intensity,
};

static struct backlight_properties eve_bl_props = {
	.max_brightness = MAX_BRIGHTNESS,
};
static int __init bd6083_probe(struct i2c_client *i2c_dev,
			       const struct i2c_device_id *i2c_dev_id)
{
	int ret;
	struct bd6083_device *dev;
	struct backlight_device *bl_dev;

	D_FUNC("%s()\n", __FUNCTION__);

	dev = kzalloc(sizeof(struct bd6083_device), GFP_KERNEL);
	if (dev == NULL) {
		dev_err(&i2c_dev->dev, "fail alloc for bd6083_device\n");
		return 0;
	}

	if (system_rev <= 7) {
		printk("%s: Unsupported hardware!\n", __func__);
		return -1;
	}

	bl_dev =
	    backlight_device_register("adam-bl", &i2c_dev->dev, NULL,
				      &eve_bl_ops, &eve_bl_props);
	bl_dev->props.max_brightness = MAX_BRIGHTNESS;
	bl_dev->props.brightness = DEFAULT_BRIGHTNESS;
	bl_dev->props.power = FB_BLANK_UNBLANK;

	bd6083_dev = dev;
	bd6083_dev->bl_dev = bl_dev;
	bd6083_dev->client = i2c_dev;
	i2c_set_clientdata(i2c_dev, bd6083_dev);

	bd6083_init_register(i2c_dev);
#ifdef CONFIG_HAS_EARLYSUSPEND
	bl_early_suspend.suspend = bd6083_early_suspend;
	bl_early_suspend.resume = bd6083_late_resume;
	bl_early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN - 40;
	register_early_suspend(&bl_early_suspend);
#endif

	ret = device_create_file(&bl_dev->dev, &dev_attr_alc_brightness);
	return device_create_file(&bl_dev->dev, &dev_attr_bl_onoff);
}

static int bd6083_remove(struct i2c_client *i2c_dev)
{
	struct bd6083_device *dev;

	D_FUNC("%s()\n", __FUNCTION__);

	dev = (struct bd6083_device *)i2c_get_clientdata(i2c_dev);
	backlight_device_unregister(dev->bl_dev);
	i2c_set_clientdata(i2c_dev, NULL);

	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void bd6083_early_suspend(struct early_suspend *h)
{
	int ret;

	D_FUNC("%s()\n", __FUNCTION__);
#ifdef MDDI_DEBUG
	power_state_flag = SUSPEND_STATE;
#endif

	ret = eve_bl_get_intensity(bd6083_dev->bl_dev);
	ret = bd6083_write_reg(bd6083_dev->client, 0x02, 0x00);
}

static void bd6083_late_resume(struct early_suspend *h)
{
	D_FUNC("%s()start.\n", __FUNCTION__);

#ifdef MDDI_DEBUG
	/* LGE_CHANGE [dojip.kim@lge.com] 2010-04-27, check the fb suspend */
	/*
	if (need_force_pan) {
		force_pan();
		need_force_pan = 0;
	}
	*/
	power_state_flag = NORMAL_STATE;
#endif

	if (cur_camera_mode == BL_CAMERA_ON) {
		bd6083_set_camera_on_mode_internal();
		return;
	}

	bd6083_init_register(bd6083_dev->client);

	D_FUNC("%s() done.\n", __FUNCTION__);
}
#endif


static struct i2c_device_id bd6083_idtable[] = {
	{"backlight", 1},
};

MODULE_DEVICE_TABLE(i2c, bd6083_idtable);

static struct i2c_driver bd6083_driver = {
	.probe = bd6083_probe,
	.remove = bd6083_remove,
	.id_table = bd6083_idtable,
	.driver = {
		   .name = "bl_bd6083",
		   .owner = THIS_MODULE,
	},
};

int eve_backlight_init(void)
{
	D_FUNC("%s()\n", __FUNCTION__);

	return i2c_add_driver(&bd6083_driver);
}

MODULE_DESCRIPTION("GW650 LCD Backlight Control");
MODULE_AUTHOR("SeungHo Park <parksh03@lge.com>");
MODULE_LICENSE("GPL");
