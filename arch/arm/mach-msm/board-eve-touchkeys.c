/* drivers/input/keyboard/synaptics_i2c_rmi.c
 *
 * Copyright (C) 2007 Google, Inc.
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
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/earlysuspend.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/gpio.h>         /*instead tlmm_bsp.c */
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <mach/vreg.h>          //LGE_CHANGE [diyu@lge.com] To set vreg
#include <mach/system.h>        //for reading REV. in Board.
#include "board-eve.h"

#define SYNAPTICS_I2C_TOUCH_NAME "touch_so240001"

#define KBDIRQNO(kbdrec)  MSM_GPIO_TO_INT(GPIO_IRQ_PP2106M2)    /*kbdrec->mykeyboard->irq) */
#define QKBD_PHYSLEN 128
#if 1
#define TS_X_RIGHT      670     /*1024 */
#define TS_X_LEFT       330     /*1024 */
#define TS_Y_RIGHT      120     /*1024 */
#define TS_Y_LEFT       120     /*1024 */

#define GPIO_TOUCH_IRQ          20 /**/
#define TOUCH_SCL_PIN           42 /**/
#define TOUCH_SDA_PIN           41 /**/
#define TOUCH_OT_ADDR                                                   0x2C    // OneTouch Device I2C Slave Address
#define TOUCH_OT_TIMEOUT                        0x50000 /*50ms */
#define TOUCH_OT_HAL_CLK_STRETCH_LIMIT          5000000 /*500ms */
#define TOUCH_OT_SUCCESS                        0x00
#define TOUCH_OT_FAILURE                        0x01
#define TOUCH_OT_READ                           0x01    //i2c read bit
#define TOUCH_OT_DATA_REG_START_ADDR            0x0108
#define TOUCH_OT_DATA_REG_END_ADDR              0x0108
#define TOUCH_OT_GPIO_STATE_ADDR_HIGH           ((0x0108&0xFF00)>>8)
#define TOUCH_OT_GPIO_STATE_ADDR_LOW            (0x0108&0x00FF)
#define TOUCH_OT_BUTTON_STATE_ADDR_HIGH         ((0x0109&0xFF00)>>8)
#define TOUCH_OT_BUTTON_STATE_ADDR_LOW          (0x0109&0x00FF)
#define TOUCH_OT_BUTTON_1_0_PRESSURE_ADDR_HIGH          ((0x010C&0xFF00)>>8)
#define TOUCH_OT_BUTTON_1_0_PRESSURE_ADDR_LOW           (0x010C&0x00FF)
#define TOUCH_OT_BUTTON_3_2_PRESSURE_ADDR_HIGH          ((0x010D&0xFF00)>>8)
#define TOUCH_OT_BUTTON_3_2_PRESSURE_ADDR_LOW           (0x010D&0x00FF)
#define TOUCH_OT_DATA_REG_START_ADDR_HIGH               ((TOUCH_OT_DATA_REG_START_ADDR & 0xFF00)>>8)
#define TOUCH_OT_DATA_REG_START_ADDR_LOW                (TOUCH_OT_DATA_REG_START_ADDR & 0x00FF)
#define OT_CONFIG_TABLE_ROWS                    11
/* //not used
   static u8 TOUCH_OT_Config_Table[OT_CONFIG_TABLE_ROWS][4] = {
// {address-high-byte, address-low-byte, data-hi-byte, data-lo-byte }
{ 0x00, 0x00, 0x00, 0x07 },
{ 0x00, 0x01, 0x00, 0x00 },
{ 0x00, 0x04, 0x00, 0x07 },
{ 0x00, 0x0E, 0x07, 0x00 },
{ 0x00, 0x10, 0xAA, 0xAA },
{ 0x00, 0x11, 0xAA, 0xAA },
{ 0x00, 0x12, 0xAA, 0xAA },
{ 0x00, 0x13, 0xAA, 0xAA },
{ 0x00, 0x22, 0x00, 0x00 },
{ 0x00, 0x23, 0xFE, 0xFF },
{ 0x00, 0x24, 0xFF, 0xFF }
};
*/
#define TOUCH_OT_CONFIG_REG_START_ADDR          0x0000
#define TOUCH_OT_CONFIG_REG_END_ADDR            0x0024
#define TOUCH_OT_NUM_CONFIG_REG_BYTES           ((TOUCH_OT_CONFIG_REG_END_ADDR - TOUCH_OT_CONFIG_REG_START_ADDR + 1)*2)
// Total number of Configuration Bytes
#define TOUCH_OT_NUM_CONFIG_BYTES               (TOUCH_OT_NUM_CONFIG_REG_BYTES + 2)
static u8 g_OT_Config[TOUCH_OT_NUM_CONFIG_BYTES] = {
	// High byte, followed by low Byte
	0x00, 0x00,             //(1) base address of config registers
	0x00, 0x07,             //(2) values of 0x0000
	0x00, 0x30,             //(3) values of 0x0001 /*NOT NEED GPI Attention*/
	0x00, 0x00,             //(4) values of 0x0002
	0x00, 0x00,             //(5) values of 0x0003
	0x00, 0x06,             //(6) values of 0x0004
	0x00, 0x00,             //(7) values of 0x0005
	0x00, 0x00,             //(8) values of 0x0006
	0x00, 0x00,             //(9) values of 0x0007
	0x00, 0x00,             //(10) values of 0x0008
	0x00, 0x00,             //(11) values of 0x0009
	0x00, 0x00,             //(12) values of 0x000A
	0x00, 0x00,             //(13) values of 0x000B
	0x00, 0x00,             //(14) values of 0x000C
	0x00, 0x00,             //(15) values of 0x000D
	0x00, 0x00,             //(16) values of 0x000E
	0x00, 0x00,             //(17) values of 0x000F
	0x9B, 0x00,             //(18) values of 0x0010 HOME: 0xAA(170)->0x96(150) -> 0xA0 (160)                  ->  0x9B (155)  rev.d :0x9A(154)
	0x00, 0x9A,             //(19) values of 0x0011 BACK: 0xAA(170)->0x96(150)->0x9A(154) -> 0xA0 (160)  ->  0x9A (154)  rev.d :0x9A(154)
	0x00, 0x00,             //(20) values of 0x0012
	0x00, 0x00,             //(21) values of 0x0013
	0x00, 0x00,             //(22) values of 0x0014
	0x00, 0x00,             //(23) values of 0x0015
	0x00, 0x00,             //(24) values of 0x0016
	0x00, 0x00,             //(25) values of 0x0017
	0x00, 0x00,             //(26) values of 0x0018
	0x00, 0x00,             //(27) values of 0x0019
	0x00, 0x00,             //(28) values of 0x001A
	0x00, 0x00,             //(29) values of 0x001B
	0x00, 0x00,             //(30) values of 0x001C
	0x00, 0x00,             //(31) values of 0x001D
	0x00, 0x00,             //(32) values of 0x001E
	0x00, 0x00,             //(33) values of 0x001F
	0x00, 0x00,             //(34) values of 0x0020
	0x00, 0x00,             //(35) values of 0x0021
	0x00, 0x00,             //(36) values of 0x0022
	0x9F, 0x00,             //(37) values of 0x0023
	0x00, 0x9F              //(38) values of 0x0024
};

#endif

#define TBSIZE 2

static unsigned char touchbutton_keycode[TBSIZE] = {
	KEY_MENU, KEY_BACK
};

/*struct touchbutton {
  unsigned char keycode[ARRAY_SIZE(touchbutton_keycode)];
  };*/

//#define swap(x, y) do { typeof(x) z = x; x = y; y = z; } while (0)
//static int count = 0;
static struct workqueue_struct *synaptics_wq;

struct synaptics_ts_data {
	uint16_t addr;
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct hrtimer timer;
	struct work_struct work;
	uint16_t max[2];
	uint32_t flags;
	int (*power) (int on);
	int pressed;
	struct early_suspend early_suspend;
	unsigned char keycode[ARRAY_SIZE(touchbutton_keycode)];
} *g_ts;

#ifdef CONFIG_HAS_EARLYSUSPEND
static void synaptics_ts_early_suspend(struct early_suspend *h);
static void synaptics_ts_late_resume(struct early_suspend *h);
#endif

/* block the home and back button this time after the menu button has been pressed down */
static int block_after_menu_value = 800;
static int invalid_touch_event_time = 100;
static int touch_longkey_timer_value = 600;

#define TOUCH_HOME_KEY  1
#define TOUCH_BACK_KEY  2


static int is_menukey_pressed = 0;
int is_slideon_event = 0;
static int is_menutimer_initialized = 0;

unsigned long touch_time = 0;

static int is_longkey_timer_running;

struct timer_list lg_enhanced_menukey;
struct timer_list lg_enhanced_longkey;

static void lg_enhanced_longkey_timer(unsigned long arg)
{
	input_report_key(g_ts->input_dev, g_ts->pressed, 1);
	input_sync(g_ts->input_dev);
	eve_vibrator_set(25);
	is_longkey_timer_running = 0;
}

void lg_enhanced_menukey_func(int value)
{
	if (is_menutimer_initialized) {
		is_menukey_pressed = 1;
		mod_timer(&lg_enhanced_menukey,
				jiffies + (block_after_menu_value * HZ / 1000));
	}
}
EXPORT_SYMBOL(lg_enhanced_menukey_func);

static void lg_enhanced_menukey_timer(unsigned long arg)
{
	is_menukey_pressed = 0;
}

static int synaptics_ts_set_vreg(int onoff)
{

	struct vreg *vreg_touch;
	int rc;

	vreg_touch = vreg_get(0, "synt");

	if (onoff) {
		gpio_tlmm_config(GPIO_CFG(GPIO_TOUCH_IRQ, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
				GPIO_CFG_ENABLE);  /*TOUCH_ATTEN(IRQ) */
		gpio_tlmm_config(GPIO_CFG(TOUCH_SDA_PIN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
				GPIO_CFG_ENABLE);  /*TOUCH_SDA */
		gpio_tlmm_config(GPIO_CFG(TOUCH_SCL_PIN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
				GPIO_CFG_ENABLE);  /*TOUCH_SCL */

		vreg_touch = vreg_get(0, "synt");
		vreg_enable(vreg_touch);
		rc = vreg_set_level(vreg_touch, 2800);
		if (rc != 0) {
			printk(KERN_ERR "%s: vreg_touch failed\n", __func__);
			return -1;
		}
	} else {
		vreg_disable(vreg_touch);

		gpio_tlmm_config(GPIO_CFG(GPIO_TOUCH_IRQ, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
				GPIO_CFG_ENABLE);  /*TOUCH_ATTEN(IRQ) */
		gpio_tlmm_config(GPIO_CFG(TOUCH_SDA_PIN, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
				GPIO_CFG_ENABLE);  /*TOUCH_SDA */
		gpio_tlmm_config(GPIO_CFG(TOUCH_SCL_PIN, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
				GPIO_CFG_ENABLE);  /*TOUCH_SCL */
	}

	return 0;
}

static int synaptics_init_panel(struct synaptics_ts_data *ts)
{
	int ret;
	printk("%s\n", __FUNCTION__);

	ret = i2c_smbus_write_byte_data(ts->client, 0xff, 0x10);        /* page select = 0x10 */
	if (ret < 0) {
		printk(KERN_ERR "i2c_smbus_write_byte_data failed for page select\n");
		goto err_page_select_failed;
	}
	ret = i2c_smbus_write_byte_data(ts->client, 0x41, 0x04);        /* Set "No Clip Z" */
	if (ret < 0)
		printk(KERN_ERR "i2c_smbus_write_byte_data failed for No Clip Z\n");

err_page_select_failed:
	ret = i2c_smbus_write_byte_data(ts->client, 0xff, 0x04);        /* page select = 0x04 */
	if (ret < 0)
		printk(KERN_ERR "i2c_smbus_write_byte_data failed for page select\n");
	/* normal operation, 80 reports per second */
	ret = i2c_smbus_write_byte_data(ts->client, 0xf0, 0x81);
	if (ret < 0)
		printk(KERN_ERR "synaptics_ts_resume: i2c_smbus_write_byte_data failed\n");
	return ret;
}

static void synaptics_ts_work_func(struct work_struct *work)
{
	int ret;
	uint8_t buf[15], buf2[15];
	u8 pReg[4], pReg2[4];
	struct synaptics_ts_data *ts =
		container_of(work, struct synaptics_ts_data, work);
	struct i2c_msg data_read_msg[] = {
		{
			.addr = ts->client->addr,
			.flags = 0 /*WRITE*/,
			.len = 4,
			.buf = pReg,
		},
		{
			.addr = ts->client->addr,
			.flags = I2C_M_RD /*READ*/,
			.len = sizeof(buf),
			.buf = buf,
		},
	};
	struct i2c_msg data_read_msg2[] = {
		{
			.addr = ts->client->addr,
			.flags = 0 /*WRITE*/,
			.len = 4,
			.buf = pReg2,
		},
		{
			.addr = ts->client->addr,
			.flags = I2C_M_RD /*READ*/,
			.len = sizeof(buf2),
			.buf = buf2,
		},
	};

	printk("synaptics_ts_work_func\n");
	// GPIO State
	pReg[0] = TOUCH_OT_GPIO_STATE_ADDR_HIGH;
	pReg[1] = TOUCH_OT_GPIO_STATE_ADDR_LOW;
	// Button State
	pReg[2] = TOUCH_OT_BUTTON_STATE_ADDR_HIGH;
	pReg[3] = TOUCH_OT_BUTTON_STATE_ADDR_LOW;

	// GPIO State
	pReg2[0] = TOUCH_OT_BUTTON_1_0_PRESSURE_ADDR_HIGH;
	pReg2[1] = TOUCH_OT_BUTTON_1_0_PRESSURE_ADDR_LOW;
	// Button State
	pReg2[2] = TOUCH_OT_BUTTON_3_2_PRESSURE_ADDR_HIGH;
	pReg2[3] = TOUCH_OT_BUTTON_3_2_PRESSURE_ADDR_LOW;

	ret = i2c_transfer(ts->client->adapter, data_read_msg, 2);
	//printk("i2c_button_state - ret : %x %x %x %x\n", buf[0], buf[1], buf[2], buf[3]);

	ret = i2c_transfer(ts->client->adapter, data_read_msg2, 2);
	/*printk("i2c_PRESSURE - ret : %d/  %d / %d / %d\n", buf2[0], buf2[1],
	  buf2[2], buf2[3]);*/

	/* LGE_CHANGE [bluerti@lge.com] 2009-09-15
	 * <skip Home/Back key Event for some period after menukey. >
	 */
	if (is_menukey_pressed) {
		printk("touchkey: blocked by menu\n");
		return;
	}

	printk("buf[3] = %x\n", buf[3]);
	if (buf[3] == 0x02 || buf[3] == 0x04) {
		printk("Touch key pressed\n");
		mod_timer(&lg_enhanced_longkey,
				jiffies + (touch_longkey_timer_value * HZ / 1000));
		is_longkey_timer_running = 1;
		if (buf[3] == 0x04)
			ts->pressed = KEY_BACK;
		else
			ts->pressed = KEY_HOME;

		touch_time = jiffies;
	} else if (buf[3] == 0x00) {
		printk("Some key released\n");
		if (!ts->pressed)
			return;

		unsigned long uptime = jiffies*1000/HZ;
		if (is_longkey_timer_running) {
			del_timer(&lg_enhanced_longkey);
			if ((uptime - touch_time) > invalid_touch_event_time) {
				/* report down, if it was not already reported by the longkey_timer
				   and the touch time span is not invalid */
				input_report_key(ts->input_dev, ts->pressed, 1);
				input_sync(ts->input_dev);
				mdelay(10);
			}
		}
		if ((uptime - touch_time) > invalid_touch_event_time
			|| !is_longkey_timer_running) {
			/* report up, if a down was reported by the longkey_timer
			   or it is a valid time span */
			input_report_key(ts->input_dev, ts->pressed, 0);
			input_sync(ts->input_dev);
			if (is_longkey_timer_running)
				eve_vibrator_set(15);
		}
		is_longkey_timer_running = 0;
		ts->pressed = 0;
	}
}

static irqreturn_t synaptics_ts_irq_handler(int irq, void *dev_id)
{
	struct synaptics_ts_data *ts = dev_id;
	printk("synaptics_ts_irq_handler\n");
	queue_work(synaptics_wq, &ts->work);
	return IRQ_HANDLED;
}

static int synaptics_ts_init_chip(struct i2c_client *client)
{
	int ret;
	u8 pReg[2];
	struct i2c_msg data_read_msg[] = {
		{
			.addr = client->addr,
			.flags = 0 /*WRITE*/,
			.len = 2,
			.buf = pReg,
		},
	};
	struct i2c_msg data_register_msg[] = {
		{
			.addr = client->addr,
			.flags = 1 /*READ*/,
			.len = 2,
			.buf = pReg,
		},
	};

	struct i2c_msg msg[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = TOUCH_OT_NUM_CONFIG_BYTES,
			.buf = g_OT_Config,
		},
	};
	ret = i2c_transfer(client->adapter, msg, 1);
	printk("Touch-key               REV.1.0 \n");

	// set base address
	pReg[0] = TOUCH_OT_DATA_REG_START_ADDR_HIGH;
	pReg[1] = TOUCH_OT_DATA_REG_START_ADDR_LOW;

	ret = i2c_transfer(client->adapter, data_read_msg, 1);
	// read the data register to deassert the attention line

	ret = i2c_transfer(client->adapter, data_register_msg, 1);

	return ret;
}

static int synaptics_ts_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct synaptics_ts_data *ts;
	int ret = 0;
	struct synaptics_i2c_rmi_platform_data *pdata;
	u32 break_mark = 0;     //TIMER to GPIO-20 ( TOUCH_GPIO_IRQ )

	printk("%s\n", __FUNCTION__);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		printk(KERN_ERR "synaptics_ts_probe: need I2C_FUNC_I2C\n");
		ret = -ENODEV;
		goto err_check_functionality_failed;
	}

	g_ts = ts = kzalloc(sizeof(*ts), GFP_KERNEL);
	if (ts == NULL) {
		ret = -ENOMEM;
		goto err_alloc_data_failed;
	}
	INIT_WORK(&ts->work, synaptics_ts_work_func);
	ts->client = client;
	i2c_set_clientdata(client, ts);
	pdata = client->dev.platform_data;

	if (pdata) {
		ts->power = synaptics_ts_set_vreg;
	}

	synaptics_ts_set_vreg(1);
	if (ts->power) {
		ret = ts->power(1);
		if (ret < 0) {
			printk(KERN_ERR "synaptics_ts_probe power on failed\n");
			goto err_power_failed;
		}
	}

	mdelay(50);

	while (break_mark == 0) {
		if (gpio_get_value(GPIO_TOUCH_IRQ) == 0) {
			ret = synaptics_ts_init_chip(client);
			break_mark = 1;
			printk("gpio(20):  10  \n initial_chip\n");
		} else {
			static int init_cnt = 0;        // [bluerti@lge.com] 2009-06-02
			if (init_cnt++ < 10) {
				mdelay(100);
			} else {
				init_cnt = 0;
				break_mark = 1; // after waiting 1sec, exit while loop. it seems that there is no chip or failed chip.
			}
		}
	}

	if (ret == 0)
		printk("ret %d \n", ret);

	ts->input_dev = input_allocate_device();
	if (ts->input_dev == NULL) {
		ret = -ENOMEM;
		printk(KERN_ERR "synaptics_ts_probe: Failed to allocate input device\n");
		goto err_input_dev_alloc_failed;
	}
	ts->input_dev->name = "synaptics-touch-button";
	set_bit(EV_SYN, ts->input_dev->evbit);
	set_bit(EV_KEY, ts->input_dev->evbit);
	set_bit(BTN_TOUCH, ts->input_dev->keybit);
	set_bit(BTN_2, ts->input_dev->keybit);
	set_bit(EV_ABS, ts->input_dev->evbit);

	ts->input_dev->keycode = ts->keycode;
	ts->input_dev->keycodesize = sizeof(unsigned char);
	ts->input_dev->keycodemax = ARRAY_SIZE(touchbutton_keycode);

	set_bit(KEY_HOME, ts->input_dev->keybit);        //diyu@lge.com
	set_bit(KEY_BACK, ts->input_dev->keybit);       //diyu@lge.com

	ts->pressed = 0;

	ret = input_register_device(ts->input_dev);
	if (ret) {
		printk(KERN_ERR "synaptics_ts_probe: Unable to register %s input device\n",
				ts->input_dev->name);
		goto err_input_register_device_failed;
	}

	printk("%s:  client->irq :  %d \n", __FUNCTION__, client->irq);

	gpio_request(client->irq, "touch_irq");
	gpio_direction_input(client->irq);
	ret = request_irq(client->irq,
			synaptics_ts_irq_handler, IRQF_TRIGGER_FALLING,
			client->name, ts);
	if (ret == 0) {
		ret = i2c_smbus_write_byte_data(ts->client, 0xf1, 0x01);        /* enable abs int */
		if (ret)
			free_irq(client->irq, ts);
	}

	setup_timer(&lg_enhanced_longkey, lg_enhanced_longkey_timer, 0);
	setup_timer(&lg_enhanced_menukey, lg_enhanced_menukey_timer, 0);
	is_menutimer_initialized = 1;   // LGE_CHANGE [bluerti@lge.com] 2009-09-24
#ifdef CONFIG_HAS_EARLYSUSPEND
	ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	ts->early_suspend.suspend = synaptics_ts_early_suspend;
	ts->early_suspend.resume = synaptics_ts_late_resume;
	register_early_suspend(&ts->early_suspend);
#endif

	printk(KERN_INFO "synaptics_ts_probe: Start touchscreen %s\n",
			ts->input_dev->name);

	return 0;

err_input_register_device_failed:
	input_free_device(ts->input_dev);

err_input_dev_alloc_failed:
err_power_failed:
	kfree(ts);
err_alloc_data_failed:
err_check_functionality_failed:
	return ret;
}

static int synaptics_ts_remove(struct i2c_client *client)
{
	struct synaptics_ts_data *ts = i2c_get_clientdata(client);
	printk("%s\n", __FUNCTION__);

	unregister_early_suspend(&ts->early_suspend);
	free_irq(client->irq, ts);
	input_unregister_device(ts->input_dev);
	kfree(ts);
	return 0;
}

static int synaptics_ts_suspend(struct i2c_client *client, pm_message_t mesg)
{
	int ret;
	struct synaptics_ts_data *ts = i2c_get_clientdata(client);
	printk("%s\n", __FUNCTION__);

	disable_irq(client->irq);
	ret = cancel_work_sync(&ts->work);
	ret = i2c_smbus_write_byte_data(ts->client, 0xf1, 0);   /* disable interrupt */
	if (ret < 0)
		printk(KERN_ERR "synaptics_ts_suspend: i2c_smbus_write_byte_data failed\n");

	ret = i2c_smbus_write_byte_data(client, 0xf0, 0x86);    /* deep sleep */
	if (ret < 0)
		printk(KERN_ERR "synaptics_ts_suspend: i2c_smbus_write_byte_data failed\n");

	ret = synaptics_ts_set_vreg(0);
	if (ts->power) {
		ret = ts->power(0);
		if (ret < 0)
			printk(KERN_ERR "synaptics_ts_resume power off failed\n");
	}
	return 0;
}

static int synaptics_ts_resume(struct i2c_client *client)
{
	int ret;
	struct synaptics_ts_data *ts = i2c_get_clientdata(client);
	u32 break_mark = 0;     //TIMER to GPIO-20 ( TOUCH_GPIO_IRQ )
	printk("%s\n", __FUNCTION__);

	ret = synaptics_ts_set_vreg(1);
	if (ts->power) {
		ret = ts->power(1);
		if (ret < 0)
			printk(KERN_ERR "synaptics_ts_resume power on failed\n");
	}
	mdelay(50);

	while (break_mark == 0) {
		if (gpio_get_value(GPIO_TOUCH_IRQ) == 0) {
			ret = synaptics_ts_init_chip(client);
			break_mark = 1;
			printk("gpio(20):  10  \n initial_chip\n");
		} else {
			static int init_cnt = 0;        // [bluerti@lge.com] 2009-06-02
			if (init_cnt++ < 10) {
				mdelay(100);
			} else {
				init_cnt = 0;
				// after waiting 1sec, exit while loop.
				// it seems that there is no chip or failed chip.
				break_mark = 1;
			}
		}
	}

	synaptics_init_panel(ts);

	enable_irq(client->irq);
	i2c_smbus_write_byte_data(ts->client, 0xf1, 0x01);      /* enable abs int */

	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void synaptics_ts_early_suspend(struct early_suspend *h)
{
	struct synaptics_ts_data *ts;
	printk("%s\n", __FUNCTION__);

	ts = container_of(h, struct synaptics_ts_data, early_suspend);
	synaptics_ts_suspend(ts->client, PMSG_SUSPEND);
}

static void synaptics_ts_late_resume(struct early_suspend *h)
{
	struct synaptics_ts_data *ts;
	printk("%s\n", __FUNCTION__);

	ts = container_of(h, struct synaptics_ts_data, early_suspend);
	synaptics_ts_resume(ts->client);
}
#endif

static const struct i2c_device_id synaptics_ts_id[] = {
	{"touch_so240001", 0},
	{}
};

static struct i2c_driver synaptics_ts_driver = {
	.probe = synaptics_ts_probe,
	.remove = synaptics_ts_remove,
#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend = synaptics_ts_suspend,
	.resume = synaptics_ts_resume,
#endif
	.id_table = synaptics_ts_id,
	.driver = {
		.name = "touch_so240001",
		.owner = THIS_MODULE,
	},
};

static int __devinit synaptics_ts_init(void)
{
	printk("%s\n", __FUNCTION__);

	synaptics_wq = create_singlethread_workqueue("synaptics_wq");
	if (!synaptics_wq)
		return -ENOMEM;

	return i2c_add_driver(&synaptics_ts_driver);
}

static void __exit synaptics_ts_exit(void)
{
	printk("%s\n", __FUNCTION__);

	i2c_del_driver(&synaptics_ts_driver);
	if (synaptics_wq)
		destroy_workqueue(synaptics_wq);
}

module_init(synaptics_ts_init);
module_exit(synaptics_ts_exit);

MODULE_DESCRIPTION("Synaptics Touchscreen Driver");
MODULE_LICENSE("GPL");

