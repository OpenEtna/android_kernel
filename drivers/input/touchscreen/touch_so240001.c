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
#include <linux/delay.h>
#include <linux/earlysuspend.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/gpio.h>		/*instead tlmm_bsp.c */
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <mach/gpio_keypad.h>	//for keyled //keyled_pm.c
#include <linux/platform_device.h>
#include <linux/synaptics_i2c_rmi.h>
#include <mach/vreg.h>		//LGE_CHANGE [diyu@lge.com] To set vreg
#include  <linux/at_kpd_eve.h>	//LGE_CHANGE [antispoon@lge.com,diyu@lge.com] 2009-07-17 for AT+GKPD
#include <mach/system.h>	//for reading REV. in Board.

#define SYNAPTICS_I2C_TOUCH_NAME "touch_so240001"

//LGE_CHANGE_S [bluerti@lge.com] 
extern int msm_touch_option;
extern int msm_touch_timer_value;

#define TOUCH_IGNORE_EVENT  	1
#define TOUCH_INVERT_EVENT 	2

#define LONGKEY_TIMER_STOPPED	0
#define LONGKEY_TIMER_STARTED 	1
#define LONGKEY_TIMER_EXPIRED	2

//LGE_CHANGE [blue.park@lge.com]
#define TOUCH_HOME_KEY	1
#define TOUCH_BACK_KEY	2

#define TS_DEBUG 0
#if TS_DEBUG
#define TSDBG(fmt, args...) printk(fmt, ##args)
#else
#define TSDBG(fmt, args...) do {} while (0)
#endif /* TS_DEBUG */

static int lg_block_touch_event = 0;
static int touch_longkey_timer_value = 500;
static int is_menukey_pressed = 0;
int is_slideon_event = 0;
static int is_menutimer_initialized = 0;
static int is_slideon_timer_initialized = 0;
extern int slideon_timer_value;

//LGE_CHANGE [blue.park@lge.com]
extern int invalid_touch_event_time;
unsigned long home_touch_time = 0;
unsigned long back_touch_time = 0;

struct timer_list lg_enhanced_longkey;
struct timer_list lg_enhanced_menukey;	//[bluerti@lge.com] 2009-09-15 
struct timer_list lg_enhanced_sliedon;	//[bluerti@lge.com] 2009-10-16
static int is_longkey_timer_state = LONGKEY_TIMER_STOPPED;
struct input_dev *input_dev_ptr = NULL;
//static struct touchbutton *touchbutton; //touch-key //diyu@lge.com

void lg_enhanced_slideon_timer(unsigned long arg)
{
	// Expired SlideOn Timer. Clear is_slideon_event state.
	is_slideon_event = 0;	// to allow Home Key.
}

void lg_slideon_event_func(void)
{
	if (is_slideon_timer_initialized) {
		is_slideon_event = 1;
		mod_timer(&lg_enhanced_sliedon,
			  jiffies + (slideon_timer_value * HZ / 1000));

		if (is_longkey_timer_state == LONGKEY_TIMER_STARTED) {	// remove longkey_timer
			del_timer(&lg_enhanced_longkey);
			is_longkey_timer_state = LONGKEY_TIMER_STOPPED;
		}

	}
}

void lg_block_touch_event_func(int value)
{
	if (value == 0 || value == 1)
		lg_block_touch_event = value;

	if (lg_block_touch_event == 1 && is_longkey_timer_state == LONGKEY_TIMER_STARTED) {	// remove longkey_timer
		del_timer(&lg_enhanced_longkey);
		is_longkey_timer_state = LONGKEY_TIMER_STOPPED;
	}
}

static void lg_enhanced_longkey_timer(unsigned long arg)
{
	input_report_key(input_dev_ptr, KEY_HOME, 1);
	is_longkey_timer_state = LONGKEY_TIMER_EXPIRED;
}

void lg_enhanced_menukey_func(int value)
{
	if (is_menutimer_initialized) {
		is_menukey_pressed = 1;
		mod_timer(&lg_enhanced_menukey,
			  jiffies + (msm_touch_timer_value * HZ / 1000));

		if (is_longkey_timer_state == LONGKEY_TIMER_STARTED) {	// remove longkey_timer
			del_timer(&lg_enhanced_longkey);
			is_longkey_timer_state = LONGKEY_TIMER_STOPPED;
		}
	}
}

static void lg_enhanced_menukey_timer(unsigned long arg)
{
	// Expired MenuKey Timer. Clear is_menukey_pressed state.
	is_menukey_pressed = 0;
}

// LGE_CHANGE_S [blue.park@lge.com] 2010.2.16 
static unsigned long blue_get_time(void)
{
	struct timeval tv;

	do_gettimeofday(&tv);
	return (1000000 * tv.tv_sec + tv.tv_usec);
}

static int Is_valid_touch_event(int key_type)
{
	unsigned long curr_time;

	curr_time = blue_get_time();

	if (key_type == TOUCH_HOME_KEY) {
		if ((curr_time - home_touch_time) >
		    invalid_touch_event_time * 1000)
			return 1;
	} else if (key_type == TOUCH_BACK_KEY) {
		if ((curr_time - back_touch_time) >
		    invalid_touch_event_time * 1000)
			return 1;
	}

	return 0;
}

// LGE_CHANGE_E [blue.park@lge.com] 2010.2.16 

//LGE_CHANGE_E [bluerti@lge.com]
#define KBDIRQNO(kbdrec)  MSM_GPIO_TO_INT(GPIO_IRQ_PP2106M2)	/*kbdrec->mykeyboard->irq) */
#define QKBD_PHYSLEN 128
#if 1
#define TS_X_RIGHT	670	/*1024 */
#define TS_X_LEFT	330	/*1024 */
#define TS_Y_RIGHT	120	/*1024 */
#define TS_Y_LEFT	120	/*1024 */

#define GPIO_TOUCH_IRQ		20 /**/
#define TOUCH_SCL_PIN		42 /**/
#define TOUCH_SDA_PIN		41 /**/
#define	TOUCH_OT_ADDR							0x2C	// OneTouch Device I2C Slave Address
#define TOUCH_OT_TIMEOUT			0x50000	/*50ms */
#define TOUCH_OT_HAL_CLK_STRETCH_LIMIT 		5000000	/*500ms */
#define TOUCH_OT_SUCCESS 			0x00
#define TOUCH_OT_FAILURE 			0x01
#define TOUCH_OT_READ 	 			0x01	//i2c read bit
#define TOUCH_OT_DATA_REG_START_ADDR		0x0108
#define TOUCH_OT_DATA_REG_END_ADDR		0x0108
#define TOUCH_OT_GPIO_STATE_ADDR_HIGH		((0x0108&0xFF00)>>8)
#define TOUCH_OT_GPIO_STATE_ADDR_LOW	 	(0x0108&0x00FF)
#define TOUCH_OT_BUTTON_STATE_ADDR_HIGH		((0x0109&0xFF00)>>8)
#define TOUCH_OT_BUTTON_STATE_ADDR_LOW	 	(0x0109&0x00FF)
#define TOUCH_OT_BUTTON_1_0_PRESSURE_ADDR_HIGH		((0x010C&0xFF00)>>8)
#define TOUCH_OT_BUTTON_1_0_PRESSURE_ADDR_LOW	 	(0x010C&0x00FF)
#define TOUCH_OT_BUTTON_3_2_PRESSURE_ADDR_HIGH		((0x010D&0xFF00)>>8)
#define TOUCH_OT_BUTTON_3_2_PRESSURE_ADDR_LOW	 	(0x010D&0x00FF)
#define	TOUCH_OT_DATA_REG_START_ADDR_HIGH		((TOUCH_OT_DATA_REG_START_ADDR & 0xFF00)>>8)
#define	TOUCH_OT_DATA_REG_START_ADDR_LOW		(TOUCH_OT_DATA_REG_START_ADDR & 0x00FF)
#define OT_CONFIG_TABLE_ROWS			11
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
#define TOUCH_OT_CONFIG_REG_START_ADDR		0x0000
#define TOUCH_OT_CONFIG_REG_END_ADDR		0x0024
#define TOUCH_OT_NUM_CONFIG_REG_BYTES		((TOUCH_OT_CONFIG_REG_END_ADDR - TOUCH_OT_CONFIG_REG_START_ADDR + 1)*2)
    // Total number of Configuration Bytes
#define TOUCH_OT_NUM_CONFIG_BYTES		(TOUCH_OT_NUM_CONFIG_REG_BYTES + 2)
static u8 g_OT_Config[TOUCH_OT_NUM_CONFIG_BYTES] = {
	// High byte, followed by low Byte
	0x00, 0x00,		//(1) base address of config registers
	0x00, 0x07,		//(2) values of 0x0000
	0x00, 0x30,		//(3) values of 0x0001 /*NOT NEED GPI Attention*/
	0x00, 0x00,		//(4) values of 0x0002
	0x00, 0x00,		//(5) values of 0x0003
	0x00, 0x06,		//(6) values of 0x0004
	0x00, 0x00,		//(7) values of 0x0005
	0x00, 0x00,		//(8) values of 0x0006
	0x00, 0x00,		//(9) values of 0x0007
	0x00, 0x00,		//(10) values of 0x0008
	0x00, 0x00,		//(11) values of 0x0009
	0x00, 0x00,		//(12) values of 0x000A
	0x00, 0x00,		//(13) values of 0x000B
	0x00, 0x00,		//(14) values of 0x000C
	0x00, 0x00,		//(15) values of 0x000D
	0x00, 0x00,		//(16) values of 0x000E
	0x00, 0x00,		//(17) values of 0x000F
	0x9B, 0x00,		//(18) values of 0x0010 HOME: 0xAA(170)->0x96(150) -> 0xA0 (160)                  ->  0x9B (155)  rev.d :0x9A(154)
	0x00, 0x9A,		//(19) values of 0x0011 BACK: 0xAA(170)->0x96(150)->0x9A(154) -> 0xA0 (160)  ->  0x9A (154)  rev.d :0x9A(154)
	0x00, 0x00,		//(20) values of 0x0012
	0x00, 0x00,		//(21) values of 0x0013
	0x00, 0x00,		//(22) values of 0x0014
	0x00, 0x00,		//(23) values of 0x0015
	0x00, 0x00,		//(24) values of 0x0016
	0x00, 0x00,		//(25) values of 0x0017
	0x00, 0x00,		//(26) values of 0x0018
	0x00, 0x00,		//(27) values of 0x0019
	0x00, 0x00,		//(28) values of 0x001A
	0x00, 0x00,		//(29) values of 0x001B
	0x00, 0x00,		//(30) values of 0x001C
	0x00, 0x00,		//(31) values of 0x001D
	0x00, 0x00,		//(32) values of 0x001E
	0x00, 0x00,		//(33) values of 0x001F
	0x00, 0x00,		//(34) values of 0x0020
	0x00, 0x00,		//(35) values of 0x0021
	0x00, 0x00,		//(36) values of 0x0022
	0x9F, 0x00,		//(37) values of 0x0023
	0x00, 0x9F		//(38) values of 0x0024
};

static u8 g_OT_REV_D_Config[TOUCH_OT_NUM_CONFIG_BYTES] = {
	// High byte, followed by low Byte
	0x00, 0x00,		//(1) base address of config registers
	0x00, 0x07,		//(2) values of 0x0000
	0x00, 0x30,		//(3) values of 0x0001 /*NOT NEED GPI Attention*/
	0x00, 0x00,		//(4) values of 0x0002
	0x00, 0x00,		//(5) values of 0x0003
	0x00, 0x06,		//(6) values of 0x0004
	0x00, 0x00,		//(7) values of 0x0005
	0x00, 0x00,		//(8) values of 0x0006
	0x00, 0x00,		//(9) values of 0x0007
	0x00, 0x00,		//(10) values of 0x0008
	0x00, 0x00,		//(11) values of 0x0009
	0x00, 0x00,		//(12) values of 0x000A
	0x00, 0x00,		//(13) values of 0x000B
	0x00, 0x00,		//(14) values of 0x000C
	0x00, 0x00,		//(15) values of 0x000D
	0x00, 0x00,		//(16) values of 0x000E
	0x00, 0x00,		//(17) values of 0x000F
	0x9A, 0x00,		//(18) values of 0x0010 HOME: 0xAA(170)->0x96(150) -> 0xA0 (160)                  ->  0x9B (155)  rev.d :0x9A(154)
	0x00, 0x9A,		//(19) values of 0x0011 BACK: 0xAA(170)->0x96(150)->0x9A(154) -> 0xA0 (160)  ->  0x9A (154)  rev.d :0x9A(154)
	0x00, 0x00,		//(20) values of 0x0012
	0x00, 0x00,		//(21) values of 0x0013
	0x00, 0x00,		//(22) values of 0x0014
	0x00, 0x00,		//(23) values of 0x0015
	0x00, 0x00,		//(24) values of 0x0016
	0x00, 0x00,		//(25) values of 0x0017
	0x00, 0x00,		//(26) values of 0x0018
	0x00, 0x00,		//(27) values of 0x0019
	0x00, 0x00,		//(28) values of 0x001A
	0x00, 0x00,		//(29) values of 0x001B
	0x00, 0x00,		//(30) values of 0x001C
	0x00, 0x00,		//(31) values of 0x001D
	0x00, 0x00,		//(32) values of 0x001E
	0x00, 0x00,		//(33) values of 0x001F
	0x00, 0x00,		//(34) values of 0x0020
	0x00, 0x00,		//(35) values of 0x0021
	0x00, 0x00,		//(36) values of 0x0022
	0x9F, 0x00,		//(37) values of 0x0023
	0x00, 0x9F		//(38) values of 0x0024
};

static u8 g_OT_REV_B_C_Config[TOUCH_OT_NUM_CONFIG_BYTES] = {
	// High byte, followed by low Byte
	0x00, 0x00,		//(1) base address of config registers
	0x00, 0x07,		//(2) values of 0x0000
	0x00, 0x30,		//(3) values of 0x0001 /*NOT NEED GPI Attention*/
	0x00, 0x00,		//(4) values of 0x0002
	0x00, 0x00,		//(5) values of 0x0003
	0x00, 0x06,		//(6) values of 0x0004
	0x00, 0x00,		//(7) values of 0x0005
	0x00, 0x00,		//(8) values of 0x0006
	0x00, 0x00,		//(9) values of 0x0007
	0x00, 0x00,		//(10) values of 0x0008
	0x00, 0x00,		//(11) values of 0x0009
	0x00, 0x00,		//(12) values of 0x000A
	0x00, 0x00,		//(13) values of 0x000B
	0x00, 0x00,		//(14) values of 0x000C
	0x00, 0x00,		//(15) values of 0x000D
	0x00, 0x00,		//(16) values of 0x000E
	0x00, 0x00,		//(17) values of 0x000F
	0xAA, 0x00,		//(18) values of 0x0010 HOME: 0xAA(170)->0x96(150) -> 0xA0 (160)                  ->  0x9B (155)
	0x00, 0xAA,		//(19) values of 0x0011 BACK: 0xAA(170)->0x96(150)->0x9A(154) -> 0xA0 (160)  ->  0x9A (154) 
	0x00, 0x00,		//(20) values of 0x0012
	0x00, 0x00,		//(21) values of 0x0013
	0x00, 0x00,		//(22) values of 0x0014
	0x00, 0x00,		//(23) values of 0x0015
	0x00, 0x00,		//(24) values of 0x0016
	0x00, 0x00,		//(25) values of 0x0017
	0x00, 0x00,		//(26) values of 0x0018
	0x00, 0x00,		//(27) values of 0x0019
	0x00, 0x00,		//(28) values of 0x001A
	0x00, 0x00,		//(29) values of 0x001B
	0x00, 0x00,		//(30) values of 0x001C
	0x00, 0x00,		//(31) values of 0x001D
	0x00, 0x00,		//(32) values of 0x001E
	0x00, 0x00,		//(33) values of 0x001F
	0x00, 0x00,		//(34) values of 0x0020
	0x00, 0x00,		//(35) values of 0x0021
	0x00, 0x00,		//(36) values of 0x0022
	0x9F, 0x00,		//(37) values of 0x0023
	0x00, 0x9F		//(38) values of 0x0024
};

#endif

#define TBSIZE 2

static unsigned char touchbutton_keycode[TBSIZE] = {
	KEY_MENU, KEY_BACK
};

/*struct touchbutton {
	unsigned char keycode[ARRAY_SIZE(touchbutton_keycode)];
};*/

#define swap(x, y) do { typeof(x) z = x; x = y; y = z; } while (0)
//static int count = 0;
static struct workqueue_struct *synaptics_wq;

struct synaptics_ts_data {
	uint16_t addr;
	struct i2c_client *client;
	struct input_dev *input_dev;
	int use_irq;
	struct hrtimer timer;
	struct work_struct work;
	uint16_t max[2];
	uint32_t flags;
	int (*power) (int on);
	unsigned int leftkey_pressed;	//LGE_CHANGE diyu@lge.com
	unsigned int rightkey_pressed;	//LGE_CHANGE diyu@lge.com
	struct early_suspend early_suspend;
	unsigned char keycode[ARRAY_SIZE(touchbutton_keycode)];
};

#ifdef CONFIG_HAS_EARLYSUSPEND
static void synaptics_ts_early_suspend(struct early_suspend *h);
static void synaptics_ts_late_resume(struct early_suspend *h);
#endif

static int synaptics_ts_set_vreg(int onoff)
{

	struct vreg *vreg_touch;
	int rc;

	TSDBG("[Touch] %s() onoff:%d\n", __FUNCTION__, onoff);
	vreg_touch = vreg_get(0, "synt");

	if (onoff) {
		gpio_tlmm_config(GPIO_CFG(GPIO_TOUCH_IRQ, 0, GPIO_INPUT, GPIO_PULL_UP, GPIO_2MA), 
				 GPIO_ENABLE);	/*TOUCH_ATTEN(IRQ) */
		gpio_tlmm_config(GPIO_CFG(TOUCH_SDA_PIN, 0, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_2MA), 
				 GPIO_ENABLE);	/*TOUCH_SDA */
		gpio_tlmm_config(GPIO_CFG(TOUCH_SCL_PIN, 0, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_2MA), 
				 GPIO_ENABLE);	/*TOUCH_SCL */

		vreg_touch = vreg_get(0, "synt");
		vreg_enable(vreg_touch);
		rc = vreg_set_level(vreg_touch, 2800);
		if (rc != 0) {
			TSDBG(KERN_ERR "%s: vreg_touch failed\n", __func__);
			return -1;
		}
	} else {
		vreg_disable(vreg_touch);

		gpio_tlmm_config(GPIO_CFG(GPIO_TOUCH_IRQ, 0, GPIO_INPUT, GPIO_NO_PULL, GPIO_2MA), 
				 GPIO_ENABLE);	/*TOUCH_ATTEN(IRQ) */
		gpio_tlmm_config(GPIO_CFG(TOUCH_SDA_PIN, 0, GPIO_INPUT, GPIO_NO_PULL, GPIO_2MA), 
			 	 GPIO_ENABLE);	/*TOUCH_SDA */
		gpio_tlmm_config(GPIO_CFG(TOUCH_SCL_PIN, 0, GPIO_INPUT, GPIO_NO_PULL, GPIO_2MA), 
				 GPIO_ENABLE);	/*TOUCH_SCL */
	}

	return 0;
}

static int synaptics_init_panel(struct synaptics_ts_data *ts)
{
	int ret;
	TSDBG("%s\n", __FUNCTION__);

	ret = i2c_smbus_write_byte_data(ts->client, 0xff, 0x10);	/* page select = 0x10 */
	if (ret < 0) {
		TSDBG(KERN_ERR "i2c_smbus_write_byte_data failed for page select\n");
		goto err_page_select_failed;
	}
	ret = i2c_smbus_write_byte_data(ts->client, 0x41, 0x04);	/* Set "No Clip Z" */
	if (ret < 0)
		TSDBG(KERN_ERR "i2c_smbus_write_byte_data failed for No Clip Z\n");

err_page_select_failed:
	ret = i2c_smbus_write_byte_data(ts->client, 0xff, 0x04);	/* page select = 0x04 */
	if (ret < 0)
		TSDBG(KERN_ERR "i2c_smbus_write_byte_data failed for page select\n");
	/* normal operation, 80 reports per second */
	ret = i2c_smbus_write_byte_data(ts->client, 0xf0, 0x81);	
	if (ret < 0)
		TSDBG(KERN_ERR "synaptics_ts_resume: i2c_smbus_write_byte_data failed\n");
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

	//printk("diyu %s\n",__FUNCTION__);     

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
	TSDBG("i2c_PRESSURE - ret : %d/  %d / %d / %d\n", buf2[0], buf2[1],
	       buf2[2], buf2[3]);

	/* LGE_CHANGE [bluerti@lge.com] 2009-09-15 
	 * <skip Home/Back key Event for some period after menukey. >
	 */
	if (!is_menukey_pressed && !is_slideon_event) {	
		//(buf2[0]>30)  : HOME-KEY
		if (buf[3] == 0x02) {
	//if((buf[3] ==0x02) && ((buf2[0]&0x0ff)>30)&&((buf2[0]&0x0ff)<128) ) {
			/* LGE_CHANGE_S [antispoon@lge.com,diyu@lge.com] 2009-07-17 
			 * send HOME key (AT cmd ASCII "soft2" key ) for AT+GKPD
			 */
			write_gkpd_value(93);	
			TSDBG("Left key pressed\n");
			if (lg_block_touch_event == 0) {
				if (msm_touch_option == TOUCH_INVERT_EVENT) {	// Invert Event 
					mod_timer(&lg_enhanced_longkey, 
						  jiffies + (touch_longkey_timer_value * HZ / 1000));	
					//input_report_key(ts->input_dev, KEY_HOME/*KEY_MENU*/, 0);
					is_longkey_timer_state =
					    LONGKEY_TIMER_STARTED;
					input_dev_ptr = ts->input_dev;
				} else
					input_report_key(ts->input_dev, KEY_HOME	/*KEY_MENU */
							 , 1);
			}
			input_sync(ts->input_dev);
			ts->leftkey_pressed = 1;
			home_touch_time = blue_get_time();	
			//LGE_CHAGNE [blue.park@lge.com]
#ifdef CONFIG_KEYLED
			//msm_touchled_send(); //keyled on //code is in keyled_pm.c
#endif
		}
		//(buf2[4]>30)  : BACK-KEY      
		if (buf[3] == 0x04) {
		//if((buf[3] ==0x04) && ((buf2[3]&0x0ff)>30)&&((buf2[3]&0x0ff)<128)) {
			/* LGE_CHANGE_S [antispoon@lge.com,diyu@lge.com] 2009-07-17 
			 * send HOME key (AT cmd ASCII "soft2" key ) for AT+GKPD 
			 */
			write_gkpd_value(93);	
			TSDBG("Right key pressed\n");
			if (lg_block_touch_event == 0) {
				if (msm_touch_option == TOUCH_INVERT_EVENT) ;	//input_report_key(ts->input_dev, KEY_BACK, 0);
				else
					input_report_key(ts->input_dev,
							 KEY_BACK, 1);
			}
			input_sync(ts->input_dev);
			ts->rightkey_pressed = 1;
			back_touch_time = blue_get_time();	
			//LGE_CHANGE [blue.park@lge.com]
#ifdef CONFIG_KEYLED
			//msm_touchled_send(); //keyled on //code is in keyled_pm.c
#endif
		}

		if (buf[3] == 0x00) {
			if (ts->leftkey_pressed) {
				if (is_longkey_timer_state == LONGKEY_TIMER_STARTED) {	// remove longkey_timer
					del_timer(&lg_enhanced_longkey);
					is_longkey_timer_state =
					    LONGKEY_TIMER_STOPPED;
				}
				TSDBG("Left key released\n");
				ts->leftkey_pressed = 0;
				//LGE_CHANGE [blue.park@lge.com]
				if (lg_block_touch_event == 0 && Is_valid_touch_event(TOUCH_HOME_KEY)) {
					if (msm_touch_option ==
					    TOUCH_INVERT_EVENT) {
						input_report_key(ts->input_dev,
								 KEY_HOME, 1);
						mdelay(50);
						input_report_key(ts->input_dev,
								 KEY_HOME, 0);

					} else
						input_report_key(ts->input_dev,
								 KEY_HOME, 0);
				}

				if (is_longkey_timer_state ==
				    LONGKEY_TIMER_EXPIRED
				    && lg_block_touch_event == 1) {
					input_report_key(ts->input_dev,
							 KEY_HOME, 0);
					is_longkey_timer_state =
					    LONGKEY_TIMER_STOPPED;
				}

			} else if (ts->rightkey_pressed) {
				TSDBG("Right key released\n");
				ts->rightkey_pressed = 0;
				//LGE_CHANGE [blue.park@lge.com]
				if (lg_block_touch_event == 0 && Is_valid_touch_event(TOUCH_BACK_KEY)) {
					if (msm_touch_option ==
					    TOUCH_INVERT_EVENT) {
						input_report_key(ts->input_dev,
								 KEY_BACK, 1);
						mdelay(50);
						input_report_key(ts->input_dev,
								 KEY_BACK, 0);
					} else
						input_report_key(ts->input_dev,
								 KEY_BACK, 0);
				}
			}
		}

	}			//LGE_CHANGE [bluerti@lge.com] 2009-09-15 

	if (ts->use_irq) {
		enable_irq(ts->client->irq);
		//printk("test irq-test \n");
	}

}

static enum hrtimer_restart synaptics_ts_timer_func(struct hrtimer *timer)
{
	struct synaptics_ts_data *ts =
	    container_of(timer, struct synaptics_ts_data, timer);
	/* printk("synaptics_ts_timer_func\n"); */
	TSDBG("%s\n", __FUNCTION__);

	queue_work(synaptics_wq, &ts->work);

	hrtimer_start(&ts->timer, ktime_set(0, 12500000), HRTIMER_MODE_REL);
	return HRTIMER_NORESTART;
}

static irqreturn_t synaptics_ts_irq_handler(int irq, void *dev_id)
{
	struct synaptics_ts_data *ts = dev_id;
	//u8 pReg[4];
	//int ret;
	TSDBG("%s\n", __FUNCTION__);

	//printk("synaptics_ts_irq_handler\n : %d == %d", ts->client->irq, gpio_to_irq(GPIO_TOUCH_IRQ)); 
	disable_irq_nosync(ts->client->irq);
	queue_work(synaptics_wq, &ts->work);
	return IRQ_HANDLED;
}

static int synaptics_init_gpio_panel(void)
{
	int ret;

	synaptics_ts_set_vreg(1);

#if 1				/* PULL-UP */
	//printk("%s - 1\n", __FUNCTION__);
	ret = gpio_tlmm_config(GPIO_CFG(GPIO_TOUCH_IRQ, 0, GPIO_INPUT, GPIO_PULL_UP, GPIO_2MA), 
			       GPIO_ENABLE);	/*TOUCH_ATTEN(IRQ) */
	ret = gpio_tlmm_config(GPIO_CFG(TOUCH_SDA_PIN, 0, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_2MA), 
			       GPIO_ENABLE);	/*TOUCH_SDA */
	ret = gpio_tlmm_config(GPIO_CFG(TOUCH_SCL_PIN, 0, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_2MA), 
			       GPIO_ENABLE);	/*TOUCH_SCL */
#else /*NO_PULL: This case is not to work */
#endif
	//printk("%s - 2\n", __FUNCTION__);
	mdelay(25);

#if 0
	printk("%s - 2\n", __FUNCTION__);

	gpio_configure(GPIO_TOUCH_IRQ, GPIOF_DRIVE_OUTPUT | GPIOF_OUTPUT_LOW);
//      msleep(20);
	gpio_configure(GPIO_TOUCH_IRQ, GPIOF_DRIVE_OUTPUT | GPIOF_OUTPUT_HIGH);
	ret = gpio_tlmm_config(GPIO_CFG(GPIO_TOUCH_IRQ, 0, GPIO_INPUT, GPIO_PULL_UP, GPIO_2MA), 
			       GPIO_ENABLE);	/*TOUCH_ATTEN(IRQ) */
#endif
	return 0;

}

static int synaptics_ts_init_chip(struct i2c_client *client)
{
	int ret;
	u8 pReg[2];
	unsigned int lge_hw_rev;
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

	//printk("%s - 6\n", __FUNCTION__);
	//      Write the configuration to the OneTouch
	/*for reading Board revision, by jinwoonam@lge.com */
	lge_hw_rev = system_rev;
	lge_hw_rev = LGE_PCB_VER_C;
	if (lge_hw_rev >= LGE_PCB_VER_1_0) {
		struct i2c_msg msg[] = {
			{
			 .addr = client->addr,
			 .flags = 0,
			 .len = TOUCH_OT_NUM_CONFIG_BYTES,
			 .buf = g_OT_Config,
			 },
		};
		ret = i2c_transfer(client->adapter, msg, 1);
		TSDBG("Touch-key  		REV.1.0 \n");
	} else if (lge_hw_rev == LGE_PCB_VER_D) {
		struct i2c_msg msg[] = {
			{
			 .addr = client->addr,
			 .flags = 0,
			 .len = TOUCH_OT_NUM_CONFIG_BYTES,
			 .buf = g_OT_REV_D_Config,
			 },
		};
		ret = i2c_transfer(client->adapter, msg, 1);
		TSDBG("Touch-key  		REV.D\n");
	} else if (lge_hw_rev == LGE_PCB_VER_C) {
		struct i2c_msg msg[] = {
			{
			 .addr = client->addr,
			 .flags = 0,
			 .len = TOUCH_OT_NUM_CONFIG_BYTES,
			 .buf = g_OT_REV_B_C_Config,
			 },
		};
		ret = i2c_transfer(client->adapter, msg, 1);
		TSDBG("Touch-key  		REV.C\n");
	} else {
		struct i2c_msg msg[] = {
			{
			 .addr = client->addr,
			 .flags = 0,
			 .len = TOUCH_OT_NUM_CONFIG_BYTES,
			 .buf = g_OT_REV_B_C_Config,
			 },
		};
		ret = i2c_transfer(client->adapter, msg, 1);
		TSDBG("Touch-key  		REV.B\n");
	}

	//printk("%s - 8\n", __FUNCTION__);
	// set base address             
	pReg[0] = TOUCH_OT_DATA_REG_START_ADDR_HIGH;
	pReg[1] = TOUCH_OT_DATA_REG_START_ADDR_LOW;

	ret = i2c_transfer(client->adapter, data_read_msg, 1);
	//printk("i2c_data_register_msg - failret : %d\n", ret);

	/*if(ret == TOUCH_OT_FAILURE)
	   printk("i2c_DATA_read - fail ret : %d\n"); */

	// read the data register to deassert the attention line

	ret = i2c_transfer(client->adapter, data_register_msg, 1);
	//printk("i2c_data_register_msg - failret : %d\n", ret);

	/*if(ret == TOUCH_OT_FAILURE)
	   printk("i2c_DATA_Register_read - failret : %d\n"); */

#if 0				/*DUMP_REGS */
	dump_regs();
#endif

	return ret;
}

static int synaptics_ts_probe(struct i2c_client *client,
			      const struct i2c_device_id *id)
{
	struct synaptics_ts_data *ts;
	int ret = 0;
	struct synaptics_i2c_rmi_platform_data *pdata;
	u32 bCounter = 0x0, break_mark = 0;	//TIMER to GPIO-20 ( TOUCH_GPIO_IRQ ) 
	//struct touchbutton *touchbutton; //touch-key //diyu@lge.com

	TSDBG("%s\n", __FUNCTION__);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		TSDBG(KERN_ERR "synaptics_ts_probe: need I2C_FUNC_I2C\n");
		ret = -ENODEV;
		goto err_check_functionality_failed;
	}

	ts = kzalloc(sizeof(*ts), GFP_KERNEL);
	if (ts == NULL) {
		ret = -ENOMEM;
		goto err_alloc_data_failed;
	}
	INIT_WORK(&ts->work, synaptics_ts_work_func);
	ts->client = client;
	i2c_set_clientdata(client, ts);
	pdata = client->dev.platform_data;

	if (pdata) {
		//ts->power = pdata->power;
		ts->power = synaptics_ts_set_vreg;
	}

	synaptics_ts_set_vreg(1);
	if (ts->power) {
		ret = ts->power(1);
		if (ret < 0) {
			TSDBG(KERN_ERR "synaptics_ts_probe power on failed\n");
			goto err_power_failed;
		}
	}

	/*do{
	   bCounter++;
	   }while(      (gpio_get_value(GPIO_TOUCH_IRQ) == 1)&( bCounter < TOUCH_OT_TIMEOUT));
	 */
	mdelay(50);
	//synaptics_init_gpio_panel();

	while (break_mark == 0) {
		if (gpio_get_value(GPIO_TOUCH_IRQ) == 0) {
			ret = synaptics_ts_init_chip(client);
			break_mark = 1;
			TSDBG("gpio(20):  10  \n initial_chip\n");
		} else {
			static int init_cnt = 0;	// [bluerti@lge.com] 2009-06-02
			if (init_cnt++ < 10) {
				mdelay(100);
			} else {
				init_cnt = 0;
				break_mark = 1;	// after waiting 1sec, exit while loop. it seems that there is no chip or failed chip.
			}
		}
	}

	if (ret == 0)
		TSDBG("ret %d \n", ret);

	ts->input_dev = input_allocate_device();
	if (ts->input_dev == NULL) {
		ret = -ENOMEM;
		TSDBG(KERN_ERR "synaptics_ts_probe: Failed to allocate input device\n");
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

	set_bit(KEY_HOME /*KEY_MENU */ , ts->input_dev->keybit);	//diyu@lge.com
	set_bit(KEY_BACK, ts->input_dev->keybit);	//diyu@lge.com

	ts->leftkey_pressed = 0;
	ts->rightkey_pressed = 0;

	/* ts->input_dev->name = ts->keypad_info->name; */
	ret = input_register_device(ts->input_dev);
	if (ret) {
		printk(KERN_ERR "synaptics_ts_probe: Unable to register %s input device\n",
		       ts->input_dev->name);
		goto err_input_register_device_failed;
	}
	if (client->irq) {
		i2c_smbus_write_byte_data(ts->client, 0xf1, 0x00);	/* enable abs int */
		printk("%s:  client->irq :  %d \n", __FUNCTION__, client->irq);
		//gpio_to_irq(40);
		//Do Not need. To assign at board_adam_gpio_i2c.c

		//ret = request_irq(client->irq, synaptics_ts_irq_handler, IRQF_TRIGGER_RISING|IRQF_TRIGGER_FALLING, client->name, ts);
		ret =
		    request_irq(client->irq,
				synaptics_ts_irq_handler, IRQF_TRIGGER_FALLING,
				client->name, ts);
		if (ret == 0) {
			ret = i2c_smbus_write_byte_data(ts->client, 0xf1, 0x01);	/* enable abs int */
			if (ret)
				free_irq(client->irq, ts);
		}
		if (ret == 0)
			ts->use_irq = 1;
		else
			dev_err(&client->dev, "request_irq failed\n");
	}
	/* temporary 5/14 no work
	   if (!ts->use_irq) {
	   hrtimer_init(&ts->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	   ts->timer.function = synaptics_ts_timer_func;
	   hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
	   } */
	// LGE_CHANGE [bluerti@lge.com] 2009-09-01 
	setup_timer(&lg_enhanced_longkey, lg_enhanced_longkey_timer, 0);	
	// LGE_CHANGE [bluerti@lge.com] 2009-09-15
	setup_timer(&lg_enhanced_menukey, lg_enhanced_menukey_timer, 0);	
	// LGE_CHANGE [bluerti@lge.com] 2009-10-16
	setup_timer(&lg_enhanced_sliedon, lg_enhanced_slideon_timer, 0);	
	is_menutimer_initialized = 1;	// LGE_CHANGE [bluerti@lge.com] 2009-09-24
	is_slideon_timer_initialized = 1;	// LGE_CHANGE [bluerti@lge.com] 2009-10-16
#ifdef CONFIG_HAS_EARLYSUSPEND
	ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	ts->early_suspend.suspend = synaptics_ts_early_suspend;
	ts->early_suspend.resume = synaptics_ts_late_resume;
	register_early_suspend(&ts->early_suspend);
#endif

	printk(KERN_INFO "synaptics_ts_probe: Start touchscreen %s in %s mode\n",
	       ts->input_dev->name, ts->use_irq ? "interrupt" : "polling");

	return 0;

err_input_register_device_failed:
	input_free_device(ts->input_dev);

err_input_dev_alloc_failed:
err_detect_failed:
err_power_failed:
	kfree(ts);
err_alloc_data_failed:
err_check_functionality_failed:
	return ret;
}

static int synaptics_ts_remove(struct i2c_client *client)
{
	struct synaptics_ts_data *ts = i2c_get_clientdata(client);
	TSDBG("%s\n", __FUNCTION__);

	unregister_early_suspend(&ts->early_suspend);
	if (ts->use_irq)
		free_irq(client->irq, ts);
	/*else
	   hrtimer_cancel(&ts->timer); */
	input_unregister_device(ts->input_dev);
	kfree(ts);
	return 0;
}

static int synaptics_ts_suspend(struct i2c_client *client, pm_message_t mesg)
{
	int ret;
	struct synaptics_ts_data *ts = i2c_get_clientdata(client);
	TSDBG("%s\n", __FUNCTION__);

	if (ts->use_irq)
		disable_irq(client->irq);
	/*else
	   hrtimer_cancel(&ts->timer); */
	ret = cancel_work_sync(&ts->work);
	if (ret && ts->use_irq)	/* if work was pending disable-count is now 2 */
		enable_irq(client->irq);
	ret = i2c_smbus_write_byte_data(ts->client, 0xf1, 0);	/* disable interrupt */
	if (ret < 0)
		TSDBG(KERN_ERR "synaptics_ts_suspend: i2c_smbus_write_byte_data failed\n");

	ret = i2c_smbus_write_byte_data(client, 0xf0, 0x86);	/* deep sleep */
	if (ret < 0)
		TSDBG(KERN_ERR "synaptics_ts_suspend: i2c_smbus_write_byte_data failed\n");

	ret = synaptics_ts_set_vreg(0);
	if (ts->power) {
		ret = ts->power(0);
		if (ret < 0)
			TSDBG(KERN_ERR "synaptics_ts_resume power off failed\n");
	}
	return 0;
}

static int synaptics_ts_resume(struct i2c_client *client)
{
	int ret;
	struct synaptics_ts_data *ts = i2c_get_clientdata(client);
	u32 bCounter = 0x0, break_mark = 0;	//TIMER to GPIO-20 ( TOUCH_GPIO_IRQ ) 
	TSDBG("%s\n", __FUNCTION__);

	ret = synaptics_ts_set_vreg(1);
	if (ts->power) {
		ret = ts->power(1);
		if (ret < 0)
			TSDBG(KERN_ERR "synaptics_ts_resume power on failed\n");
	}
	// LGE_CHANGE_S
	/*do{
	   bCounter++;
	   }while(      (gpio_get_value(GPIO_TOUCH_IRQ) == 1)&( bCounter < TOUCH_OT_TIMEOUT));
	 */
	mdelay(50);
	//synaptics_init_gpio_panel();

	while (break_mark == 0) {
		if (gpio_get_value(GPIO_TOUCH_IRQ) == 0) {
			ret = synaptics_ts_init_chip(client);
			break_mark = 1;
			TSDBG("gpio(20):  10  \n initial_chip\n");
		} else {
			static int init_cnt = 0;	// [bluerti@lge.com] 2009-06-02
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
	// LGE_CHANGE_E

	synaptics_init_panel(ts);

	if (ts->use_irq)
		enable_irq(client->irq);

	if (ts->use_irq)
		i2c_smbus_write_byte_data(ts->client, 0xf1, 0x01);	/* enable abs int */
	/*hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
	   else */

	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void synaptics_ts_early_suspend(struct early_suspend *h)
{
	struct synaptics_ts_data *ts;
	TSDBG("%s\n", __FUNCTION__);

	ts = container_of(h, struct synaptics_ts_data, early_suspend);
	synaptics_ts_suspend(ts->client, PMSG_SUSPEND);
}

static void synaptics_ts_late_resume(struct early_suspend *h)
{
	struct synaptics_ts_data *ts;
	TSDBG("%s\n", __FUNCTION__);

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
	TSDBG("%s\n", __FUNCTION__);

	synaptics_wq = create_singlethread_workqueue("synaptics_wq");
	if (!synaptics_wq)
		return -ENOMEM;

	return i2c_add_driver(&synaptics_ts_driver);
}

static void __exit synaptics_ts_exit(void)
{
	TSDBG("%s\n", __FUNCTION__);

	i2c_del_driver(&synaptics_ts_driver);
	if (synaptics_wq)
		destroy_workqueue(synaptics_wq);
}

module_init(synaptics_ts_init);
module_exit(synaptics_ts_exit);

MODULE_DESCRIPTION("Synaptics Touchscreen Driver");
MODULE_LICENSE("GPL");
