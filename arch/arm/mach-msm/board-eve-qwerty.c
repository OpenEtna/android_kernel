/*
 *  drivers/input/keyboard/kbd_pp2016.c
 *
 *  Copyright (c) 2008 QUALCOMM USA, INC.
 *
 *  All source code in this file is licensed under the following license
 *  except where indicated.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  version 2 as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, you can find it at http://www.fsf.org
 *
 *  Driver for QWERTY keyboard with I/O communications via
 *  the I2C Interface. The keyboard hardware is a reference design supporting
 *  the standard XT/PS2 scan codes (sets 1&2).
 */

#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <asm/gpio.h>
#include <linux/delay.h>
#include <linux/slab.h>

#define DRIVER_VERSION "v1.0"
#define GPIO_SDA_PP2106M2   38
#define GPIO_SCL_PP2106M2  37
#define GPIO_RESET_PP2106M2 36
#define GPIO_IRQ_PP2106M2       39
#define KEY_DRIVER_NAME "eve_qwerty"

static const char *kbd_name  = "eve_qwerty"; //"kbd_pp2016";

MODULE_VERSION(DRIVER_VERSION);
MODULE_AUTHOR("diyu");
MODULE_DESCRIPTION("QWERTY keyboard driver");
MODULE_LICENSE("GPL v2");

#define QWERTY_DEBUG 0
#if QWERTY_DEBUG
#define QDBG(fmt, args...) printk(fmt, ##args)
#else
#define QDBG(fmt, args...) do {} while (0)
#endif /* QWERTY_DEBUG */

enum
{
	GPIO_LOW_VALUE  = 0,
	GPIO_HIGH_VALUE = 1
} ;

enum {
	QWERTY_START_BIT = 0,
	QWERTY_1ST_BIT7,
	QWERTY_1ST_BIT6,
	QWERTY_1ST_BIT5,
	QWERTY_1ST_BIT4,
	QWERTY_1ST_BIT3,
	QWERTY_1ST_BIT2,
	QWERTY_1ST_BIT1,
	QWERTY_1ST_BIT0,
	QWERTY_ACK_BIT,
	QWERTY_MAX_BIT
} ;
#define QKBD_IN_MXKYEVTS 256
static unsigned char pp2106m2_keycode[QKBD_IN_MXKYEVTS]  = {
	0xff, KEY_LEFT  , KEY_SPACE , KEY_MENU  , KEY_SEARCH/*KEY_BACK*/, KEY_LEFTSHIFT, KEY_DOT        , KEY_8, KEY_PROG3,/* 0x00~0x08 */
	0xff, 0xff              , 0xff          , 0xff          , 0xff    , 0xff                 , 0xff         ,
	0xff, KEY_M             , KEY_B         , KEY_C         , KEY_V   , KEY_X                , KEY_N        , KEY_5, KEY_VIDEO_PREV, /* 0x10~0x18 */
	0xff, 0xff              , 0xff          , 0xff          , 0xff    , 0xff                 , 0xff         ,
	0xff, KEY_K             , KEY_H         , KEY_F         , KEY_G   , KEY_D                , KEY_J        , KEY_0, KEY_CAMERA, /* 0x20~0x28 */
	0xff, 0xff              , 0xff          , 0xff          , 0xff    , 0xff                 , 0xff         ,
	0xff, KEY_I             , KEY_Y         , KEY_R         , KEY_T   , KEY_E                , KEY_U        , KEY_7, KEY_MENU ,  /* 0x30~0x38 */
	0xff, 0xff              , 0xff          , 0xff          , 0xff    , 0xff                 , 0xff         ,
	0xff, KEY_DOWN  , KEY_A         , KEY_1         , KEY_Z   , (unsigned char)KEY_FN                , KEY_RIGHT, KEY_6, /* 0x40~0x47 */
	0xff, 0xff              , 0xff          , 0xff          , 0xff    , 0xff                 , 0xff         , 0xff,
	0xff, KEY_UP    , KEY_L         , KEY_W         , KEY_BACKSPACE , KEY_S  , KEY_ENTER, KEY_2, /* 0x50~0x57 */
	0xff, 0xff              , 0xff          , 0xff          , 0xff    , 0xff                 , 0xff     , 0xff,
	0xff, KEY_O             , KEY_P         , KEY_Q         , KEY_4   , KEY_3                , KEY_9    , 0xff, /* 0x60~0x67 */
	0xff, 0xff              , 0xff          , 0xff          , 0xff    , 0xff                 , 0xff     , 0xff,
	0
};
#define QKBD_PHYSLEN 128

#define KEY_SCL_PIN             GPIO_SCL_PP2106M2 /*13*/
#define KEY_SDA_PIN             GPIO_SDA_PP2106M2 /*14*/

#define QWERTY_SDA_OUTPUT_LOW() { gpio_direction_output(KEY_SDA_PIN, 0); udelay(25);}
#define QWERTY_SDA_HIGH()       { gpio_set_value(KEY_SDA_PIN,GPIO_HIGH_VALUE); udelay(25); }
#define QWERTY_SDA_LOW()        { gpio_set_value(KEY_SDA_PIN,GPIO_LOW_VALUE); udelay(25); }
#define QWERTY_SDA_INPUT()      { gpio_direction_input(KEY_SDA_PIN); udelay(25);}
#define QWERTY_SDA_READ()       gpio_get_value(KEY_SDA_PIN)
#define QWERTY_SCL_OUTPUT_HIGH(){ gpio_direction_output(KEY_SCL_PIN, 1); udelay(25); }
#define QWERTY_SCL_HIGH()       { gpio_set_value(KEY_SCL_PIN,GPIO_HIGH_VALUE); udelay(25); }
#define QWERTY_SCL_LOW()        { gpio_set_value(KEY_SCL_PIN,GPIO_LOW_VALUE); udelay(25); }
/*
 * The qwerty_kbd_record structure consolates all the data/variables
 * specific to managing the single instance of the keyboard.
 */
struct qwerty_kbd_record {
	int     product_info;
	char    physinfo[QKBD_PHYSLEN];
	int     mclrpin;
	uint8_t cmd;
	uint8_t noargs;
	uint8_t cargs[2];
	uint8_t kybd_exists;
	uint8_t kybd_connected;
	uint8_t prefix;
	uint8_t pfxstk[2];
	uint8_t kcnt;
	uint8_t evcnt;
	struct delayed_work kb_cmdq;
	u32 (*xlf)(struct qwerty_kbd_record *kbdrec, s32 code,
			s32 *kstate, s32 *i2cerr);
	unsigned char pp2106m2_keycode[QKBD_IN_MXKYEVTS];
};


static struct input_dev *pp2106m2_kbd_dev;
static struct qwerty_kbd_record kbd_data;
static struct work_struct qkybd_irqwork;

extern void lg_enhanced_menukey_func(int value);
#define LG_MENUKEY_VALUE 0x8b
static int  __init qwerty_kbd_probe(struct platform_device *pdev);

__inline void P_ChipSendACK( void )
{
	QWERTY_SDA_OUTPUT_LOW();

	QWERTY_SCL_LOW();        // ACK Bit Upaate Pulse
	QWERTY_SCL_HIGH();

	QWERTY_SDA_INPUT();

	printk(" %s\n",__FUNCTION__);
}

/*=========================================================================
  FUNCTION  P_ChipGetData
  DESCRIPTION
  Function to read pysical scan code from qwerty keypad chip.
  DEPENDENCIES
  this function must be called after keypad h/w init function
  RETURN VALUE
  if user pressed keypad, return TRUE.
  or return FALSE;
  SIDE EFFECTS
  None
  ===========================================================================*/
__inline int P_ChipGetData( uint32_t *p_data  )
{
	int trigger_count;
	int first_bit_count = 0;

	QWERTY_SDA_OUTPUT_LOW();
	QWERTY_SCL_HIGH();
	QWERTY_SDA_LOW();
	udelay(25);
	printk(" %s\n",__FUNCTION__);

	for( trigger_count=QWERTY_START_BIT; trigger_count < QWERTY_MAX_BIT ; trigger_count++ )
	{
		if ( trigger_count == QWERTY_START_BIT )
		{
			QWERTY_SCL_LOW();

			// START bit
			QWERTY_SDA_INPUT();

			QWERTY_SCL_HIGH();
		}
		else if ( trigger_count >= QWERTY_1ST_BIT7&& trigger_count <= QWERTY_1ST_BIT0 )
		{
			QWERTY_SCL_LOW();

			// Data 1
			if( QWERTY_SDA_READ() )
			{
				*p_data |= 0x80 >>(first_bit_count);
			}
			first_bit_count++;

			QWERTY_SCL_HIGH();
		}
		else if ( trigger_count == QWERTY_ACK_BIT )
		{
			P_ChipSendACK();
		}

	}

	return 0;
}

// LGE_CHANGE_S [bluerti@lge.com] 2009-08-15 <LG_Enhanced_Menukey function >
extern void lg_enhanced_menukey_func(int value);
// LGE_CHANGE_E [bluerti@lge.com]

/* translate from scan code(s) to intermediate xlate-lookup code */
static inline void lge_xlscancode(void)
{
	struct input_dev *idev = pp2106m2_kbd_dev; /*kbdrec->i2ckbd_idev;*/
	u32 xlkcode = 0;
	u32 buf = 0;
	u8  keystate =0;  /*press = 1 , release = 0*/


	P_ChipGetData(&buf);
	//xlkcode = ScanCodeToShortVKeyTable[buf];
	keystate = (buf & 0x80) ? 0 : 1;

	if(keystate)
		xlkcode = pp2106m2_keycode[buf];
	if(!keystate)
		xlkcode = pp2106m2_keycode[(buf & 0x7f)];

	QDBG("+++++++++ Keypad : org <<0x%x>>  trans <<0x%x>>\n", buf, xlkcode);
	printk("+++++++++ Keypad : org <<0x%x>>  trans <<0x%x>>\n", buf, xlkcode);


	// we have a translated code to feed into the input system
	if (xlkcode) {
		input_report_key(idev, xlkcode, keystate);
		input_sync(idev);

		if (xlkcode == LG_MENUKEY_VALUE)
			lg_enhanced_menukey_func(0);

	}
}

static irqreturn_t qwerty_kbd_irqhandler(int irq, void *dev_id)
{
	QDBG(" %s\n",__FUNCTION__);
	schedule_work(&qkybd_irqwork);

	return IRQ_HANDLED;
}

static int qwerty_kbd_irqsetup(int kbd_irqpin)
{
	int rc = request_irq(MSM_GPIO_TO_INT(kbd_irqpin), &qwerty_kbd_irqhandler,
			IRQF_TRIGGER_FALLING, kbd_name, NULL);
	printk("%s\n",__FUNCTION__);

	if (rc < 0) {
		printk(KERN_ERR
				"Could not register for  %s interrupt "
				"(rc = %d)\n", kbd_name, rc);
		rc = -EIO;
	}

	rc = set_irq_wake(MSM_GPIO_TO_INT(kbd_irqpin), 1);
	return rc;
}

static int qwerty_kbd_release_gpio(struct qwerty_kbd_record *kbrec)
{
	int kbd_irqpin  = GPIO_IRQ_PP2106M2;
	int kbd_mclrpin = kbrec->mclrpin;
	printk("%s\n",__FUNCTION__);

	printk("releasing keyboard gpio pins %d,%d\n",
			kbd_irqpin, kbd_mclrpin);
	gpio_free(kbd_irqpin);
	gpio_free(kbd_mclrpin);
	return 0;
}

/* use gpio output pin to toggle keyboard external reset pin */
static void qwerty_kbd_hwreset(int kbd_mclrpin)
{
	//printk("++++++++++ KEY RESET!!!!\n");
	QDBG(" %s\n",__FUNCTION__);

	QWERTY_SCL_OUTPUT_HIGH();

	// Reset
	gpio_set_value(kbd_mclrpin, 1);
	msleep(25);
	gpio_set_value(kbd_mclrpin, 0);
	msleep(25);
	gpio_set_value(kbd_mclrpin, 1);
	msleep(25);
}

static void qwerty_kbd_fetchkeys(struct work_struct *work)
{       QDBG(" %s\n",__FUNCTION__);
	lge_xlscancode();
}

struct input_dev *qwerty_get_input_dev(void)
{
	return pp2106m2_kbd_dev;
}
EXPORT_SYMBOL(qwerty_get_input_dev);

static int qwerty_kbd_remove(struct platform_device *pdev)
{
	if (pp2106m2_kbd_dev) {
		input_unregister_device(pp2106m2_kbd_dev);
		kfree(pp2106m2_kbd_dev);
	}

	//      qwerty_kbd_shutdown(rd);
	//      qwerty_kbd_release_gpio(rd);
	return 0;
}

static int qwerty_kbd_suspend(struct platform_device *pdev, pm_message_t state)
{
	/* future capability*/
	return 0;
}

static int qwerty_kbd_resume(struct platform_device *pdev)
{
	/* future capability*/
	QWERTY_SDA_HIGH();
	QWERTY_SCL_HIGH();
	return 0;
}

static struct platform_driver qwerty_kbd_driver = {
	.driver = {
		.name = KEY_DRIVER_NAME,
		.owner = THIS_MODULE,
	},
	.probe   = qwerty_kbd_probe,
	.remove  = qwerty_kbd_remove,
	.suspend = qwerty_kbd_suspend,
	.resume  = qwerty_kbd_resume,
};

static int  __init qwerty_kbd_probe(struct platform_device *pdev)
{
	int                              rc = 0, err;
	struct qwerty_kbd_record           *rd = &kbd_data;
	int kidx = 0;
	printk("%s()\n", __FUNCTION__);

	INIT_WORK(&qkybd_irqwork, qwerty_kbd_fetchkeys);

	pp2106m2_kbd_dev = input_allocate_device();
	if (!pp2106m2_kbd_dev) {
		printk(KERN_ERR "amikbd: not enough memory for input device\n");
		err = -ENOMEM;
	}

	pp2106m2_kbd_dev->name = KEY_DRIVER_NAME;
	pp2106m2_kbd_dev->phys = "pp2106m2/input1";
	pp2106m2_kbd_dev->id.bustype = BUS_HOST;
	pp2106m2_kbd_dev->id.vendor = 0x0001;
	pp2106m2_kbd_dev->id.product = 0x0001;
	pp2106m2_kbd_dev->id.version = 0x0100;
	pp2106m2_kbd_dev->dev.parent = &pdev->dev;

	pp2106m2_kbd_dev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_REP);

	if(pp2106m2_kbd_dev){
		pp2106m2_kbd_dev->keycode = kbd_data.pp2106m2_keycode;
		pp2106m2_kbd_dev->keycodesize = sizeof(uint8_t);
		pp2106m2_kbd_dev->keycodemax = QKBD_IN_MXKYEVTS;

		pp2106m2_kbd_dev->evbit[0] = BIT(EV_KEY);
		memset(pp2106m2_kbd_dev->keycode, 0, QKBD_IN_MXKYEVTS);

		pp2106m2_kbd_dev->mscbit[0] = 0;

		// Support all keys
		for (kidx = 0; kidx <= KEY_DISPLAY_OFF; kidx++)
			set_bit(kidx, pp2106m2_kbd_dev->keybit);
		input_set_drvdata(pp2106m2_kbd_dev, rd);
	} else {
		printk("Failed to allocate input device for %s\n",      kbd_name);
	}


	gpio_request(GPIO_SDA_PP2106M2, "gpio_keybd_sda");
	gpio_request(GPIO_SCL_PP2106M2, "gpio_keybd_sdc");

	qwerty_kbd_hwreset(GPIO_RESET_PP2106M2);
	gpio_request(GPIO_RESET_PP2106M2, "gpio_keybd_reset");
	gpio_direction_output(GPIO_RESET_PP2106M2, 1);


	rc = request_irq(MSM_GPIO_TO_INT(GPIO_IRQ_PP2106M2), &qwerty_kbd_irqhandler,
			IRQF_TRIGGER_FALLING, kbd_name, NULL);
	if (rc < 0) {
		printk(KERN_ERR
				"Could not register for  %s interrupt "
				"(rc = %d)\n", kbd_name, rc);
		rc = -EIO;
	}

	/* wake the whole device on this irq */
	rc = set_irq_wake(MSM_GPIO_TO_INT(GPIO_IRQ_PP2106M2), 1);

	err = input_register_device(pp2106m2_kbd_dev);

	if (err)
		printk(" %s\n", __FUNCTION__);

	return rc;
}

static int __init qwerty_kbd_init(void)
{
	memset(&kbd_data, 0, sizeof(struct qwerty_kbd_record));

	return platform_driver_register(&qwerty_kbd_driver);
}


static void __exit qwerty_kbd_exit(void)
{
	platform_driver_unregister(&qwerty_kbd_driver);
}

module_init(qwerty_kbd_init);
module_exit(qwerty_kbd_exit);
