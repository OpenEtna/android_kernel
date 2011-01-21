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
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <asm/gpio.h>
#include <mach/msm_i2ckbd.h>
#include <mach/gpio_keypad.h> //for keyled //keyled_pm.c
#include <linux/delay.h>
//#include <asm/arch/lprintk.h>
#include <linux/at_kpd_eve.h> 
#include <mach/msm_rpcrouter.h> 

#define DRIVER_VERSION "v1.0"
#define GPIO_RESET_PP2106M2 36
#define GPIO_IRQ_PP2106M2 	39
#define GPIO_SDA_PP2106M2 	38
#define GPIO_SCL_PP2106M2 	37
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

#define KEY_UPLEFT      247
#define KEY_UPRIGHT     248
#define KEY_DOWNLEFT    249
#define KEY_DOWNRIGHT   250
/* extra key: hold key */
#define KEY_HOLD        251
#define KEY_HOLDLONG    253
/* extra key:camer key */
#define KEY_CAMERAFOCUS 252
#define KEY_SYMBOL 		254

/* constants relating to keyboard i2c transactions */
enum qkbd_protocol {
	QKBD_REG_CNTRL  = 0,		/* control register offset */
	QKBD_REG_STATUS = 1,		/* status register offset  */
	QKBD_CMD_RESET  = 0xFF,		/* keyboard reset command  */
	QKBD_CMD_ENABLE = 0xF4,		/* keyboard enable command */
	QKBD_CMD_SREP   = 0xF3,		/* set keyboard autorepeat */
	QKBD_CMD_SLSET  = 0xF0,		/* select scan set         */
	QKBD_CMD_READID = 0xF2,		/* read keyboard id info   */
	QKBD_RSP_ACK    = 0xFA		/* keyboard acknowledge    */
};

/* constants relating to events sent into the input core */
enum kbd_inevents {
	QKBD_IN_KEYPRESS        = 1,
	QKBD_IN_KEYRELEASE      = 0,
	QKBD_IN_MXKYEVTS        = 256
};

/* keyboard status register bit definitions */
enum kbd_statusbits {
	QKBD_SRBIT_CMDIP	= 1<<0,	/* cmd in progress      */
	QKBD_SRBIT_KDATA	= 1<<1,	/* key code available   */
	QKBD_SRBIT_RSPRDY	= 1<<2,	/* response ready       */
	QKBD_SRBIT_SCANEN	= 1<<3,	/* keybd enabled        */
	QKBD_SRBIT_SSET		= 1<<4	/* scan set             */
};

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
#if 0
static unsigned char pp2106m2_keycode[256]  = {
	0xff, KEY_CAMERA, KEY_SPACE, KEY_LEFTALT, KEY_MENU, KEY_FN, KEY_BACK, KEY_CAMERA, /* 0x00~0x07 */
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, KEY_M, KEY_B, KEY_C,  KEY_V,  KEY_X,  KEY_N, KEY_F1,  /* 0x10~0x17 */
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, KEY_K,  KEY_H,  KEY_F,  KEY_G,  KEY_D,  KEY_J,  KEY_F2,  /* 0x20~0x27 */
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, KEY_I,  KEY_Y,  KEY_R,  KEY_T,  KEY_VOLUMEDOWN,  KEY_VOLUMEUP,  KEY_VOLUMEDOWN,  /* 0x30~0x37 */
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, KEY_F1, KEY_A,  KEY_LEFTCTRL, KEY_Z,  KEY_LEFTSHIFT, KEY_VOLUMEUP, KEY_VOLUMEUP, /* 0x40~0x47 */
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, KEY_COMMA,  KEY_L,  KEY_W,  KEY_ENTER, KEY_S,  KEY_DOT,  KEY_MENU, /* 0x50~0x57 */
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, KEY_O,  KEY_P,  KEY_Q,  0xff, 0xff, 0xff, 0xff, /* 0x60~0x67 */
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0
};
#else /*EVE Rev.B*/
static unsigned char pp2106m2_keycode[256]  = {
0xff, KEY_LEFT	, KEY_SPACE , KEY_MENU  , KEY_SEARCH/*KEY_BACK*/, KEY_LEFTSHIFT, KEY_DOT	, KEY_8, KEY_PROG3,/* 0x00~0x08 */
0xff, 0xff		, 0xff		, 0xff		, 0xff	  , 0xff  		 , 0xff	  	, 
0xff, KEY_M		, KEY_B		, KEY_C		, KEY_V   , KEY_X 		 , KEY_N  	, KEY_5, KEY_VIDEO_PREV, /* 0x10~0x18 */
0xff, 0xff		, 0xff		, 0xff		, 0xff    , 0xff  		 , 0xff   	, 
0xff, KEY_K		, KEY_H		, KEY_F		, KEY_G   , KEY_D 		 , KEY_J  	, KEY_0, KEY_CAMERA, /* 0x20~0x28 */
0xff, 0xff		, 0xff		, 0xff		, 0xff    , 0xff  		 , 0xff   	, 
0xff, KEY_I		, KEY_Y		, KEY_R		, KEY_T   , KEY_E 		 , KEY_U  	, KEY_7, KEY_MENU ,  /* 0x30~0x38 */
0xff, 0xff		, 0xff		, 0xff		, 0xff    , 0xff  		 , 0xff   	,  
0xff, KEY_DOWN	, KEY_A		, KEY_1		, KEY_Z   , KEY_FN		 , KEY_RIGHT, KEY_6, /* 0x40~0x47 */
0xff, 0xff		, 0xff		, 0xff		, 0xff    , 0xff  		 , 0xff   	, 0xff, 
0xff, KEY_UP	, KEY_L		, KEY_W		, KEY_BACKSPACE , KEY_S  , KEY_ENTER, KEY_2, /* 0x50~0x57 */
0xff, 0xff		, 0xff		, 0xff		, 0xff    , 0xff  		 , 0xff     , 0xff, 
0xff, KEY_O		, KEY_P		, KEY_Q		, KEY_4   , KEY_3 		 , KEY_9    , 0xff, /* 0x60~0x67 */
0xff, 0xff		, 0xff		, 0xff		, 0xff    , 0xff  		 , 0xff     , 0xff,
0
};
static unsigned char at_gkpd_keycode[256]  = { //Encoder from AT cmd ASCII to Android key map code for GKPD
0xff, 76	, 0xff , 67  , 0xff/*KEY_BACK*/, 0xff, 0xff	, 56, 0xff,/* 0x00~0x08 */
0xff, 0xff		, 0xff		, 0xff		, 0xff	  , 0xff  		 , 0xff	  	, 
0xff, 0xff		, 0xff		, 0xff		, 0xff   , 0xff 		 , 0xff  	, 53, 91, /* 0x10~0x18 */
0xff, 0xff		, 0xff		, 0xff		, 0xff    , 0xff  		 , 0xff   	, 
0xff, 0xff		, 0xff		, 0xff		, 0xff   , 0xff 		 , 0xff  	, 48, 65, /* 0x20~0x28 */
0xff, 0xff		, 0xff		, 0xff		, 0xff    , 0xff  		 , 0xff   	, 
0xff, 0xff		, 0xff		, 0xff		, 0xff   , 0xff 		 , 0xff  	, 55, 67 ,  /* 0x30~0x38 */
0xff, 0xff		, 0xff		, 0xff		, 0xff    , 0xff  		 , 0xff   	,  
0xff, 86	, 0xff		, 49		, 0xff   , 0xff		 , 82, 54, /* 0x40~0x47 */
0xff, 0xff		, 0xff		, 0xff		, 0xff    , 0xff  		 , 0xff   	, 0xff, 
0xff, 94	, 0xff		, 0xff		, 89 , 0xff  , 0xff, 50, /* 0x50~0x57 */
0xff, 0xff		, 0xff		, 0xff		, 0xff    , 0xff  		 , 0xff     , 0xff, 
0xff, 0xff		, 0xff		, 0xff		, 52   , 51 		 , 57    , 0xff, /* 0x60~0x67 */
0xff, 0xff		, 0xff		, 0xff		, 0xff    , 0xff  		 , 0xff     , 0xff,
0
};

static unsigned char at_fkpd_keycode[42]  = { //Encoder from AT cmd ASCII to Android key map code for GKPD
KEY_0,	KEY_1,	KEY_2,	KEY_3,	KEY_4,	KEY_5,	KEY_6,	KEY_7,	KEY_8,	KEY_9,	/*#*/228,	227/***/, KEY_SEND, END_CALL,
48,			49,			50,			51,			52,			53,			54,			55,			56,			57,			35,			42,			83,				69,
41,			33,			64,			126,			36,			37,			60,			38,			62,			40,			63,			124,			115,				101
};
#if 0

static unsigned char pp2106m2_keycode_symbol[256]  = {
0xff, KEY_LEFT	, KEY_SYMBOL , KEY_MENU  , KEY_SEARCH/*KEY_BACK*/, KEY_LEFTSHIFT, KEY_DOT	, KEY_8, KEY_ENTER,/* 0x00~0x08 */
0xff, 0xff		, 0xff		, 0xff		, 0xff	  , 0xff  		 , 0xff	  	, 
0xff, KEY_M		, KEY_B		, KEY_C		, KEY_V   , KEY_X 		 , KEY_N  	, KEY_5, KEY_VIDEO_PREV, /* 0x10~0x18 */
0xff, 0xff		, 0xff		, 0xff		, 0xff    , 0xff  		 , 0xff   	, 
0xff, KEY_K		, KEY_H		, KEY_F		, KEY_G   , KEY_D 		 , KEY_J  	, KEY_0, KEY_CAMERA, /* 0x20~0x28 */
0xff, 0xff		, 0xff		, 0xff		, 0xff    , 0xff  		 , 0xff   	, 
0xff, KEY_I		, KEY_Y		, KEY_R		, KEY_T   , KEY_E 		 , KEY_U  	, KEY_7, KEY_MENU ,  /* 0x30~0x38 */
0xff, 0xff		, 0xff		, 0xff		, 0xff    , 0xff  		 , 0xff   	,  
0xff, KEY_DOWN	, KEY_A		, KEY_1		, KEY_Z   , KEY_FN		 , KEY_RIGHT, KEY_6, /* 0x40~0x47 */
0xff, 0xff		, 0xff		, 0xff		, 0xff    , 0xff  		 , 0xff   	, 0xff, 
0xff, KEY_UP	, KEY_L		, KEY_W		, KEY_BACKSPACE , KEY_S  , KEY_ENTER, KEY_2, /* 0x50~0x57 */
0xff, 0xff		, 0xff		, 0xff		, 0xff    , 0xff  		 , 0xff     , 0xff, 
0xff, KEY_O		, KEY_P		, KEY_Q		, KEY_4   , KEY_3 		 , KEY_9    , 0xff, /* 0x60~0x67 */
0xff, 0xff		, 0xff		, 0xff		, 0xff    , 0xff  		 , 0xff     , 0xff,
0
};	

{
0xff, 0xff		, KEY_S		, 0xff		, 0xff	    , KEY_LEFTSHIFT , KEY_QUESTION, KEY_APOSTROPHE, 0xff, /* 0x00~0x08 */
0xff, 0xff		, 0xff		, 0xff		, 0xff	    , 0xff		    , 0xff		  , 
0xff, 0xff		, KEY_COMMA , KEY_APOSTROPHE, KEY_SLASH , KEY_APOSTROPHE, 0xff	  , KEY_P			,0xff, /* 0x10~0x18 */
0xff, 0xff		, 0xff		, 0xff		, 0xff	    , 0xff			, 0xff		  , 
0xff, KEY_LEFTBRACE, KEY_S  , KEY_S		, KEY_S	    , KEY_S/**/		, 0xff		  , KEY_S/**/		,KEY_S/**/,  /* 0x20~0x28 */
0xff, 0xff		, 0xff		, 0xff		, 0xff	    , 0xff			, 0xff		  ,  
0xff, KEY_EQUAL , KEY_MINUS , KEY_S		, KEY_S     , 0xff			, KEY_QUESTION, KEY_S/**/		,KEY_S	,/* 0x30~0x38 */
0xff, 0xff		, 0xff		, 0xff		, 0xff	    , 0xff			, 0xff		  , 
0xff, KEY_PAGEDOWN,0xff		, KEY_S		, 0xff	    , KEY_FN/**/	, 0xff		  , KEY_S/**/		,/* 0x40~0x47 */
0xff, 0xff		, 0xff		, 0xff		, 0xff	    , 0xff			, 0xff		  , 0xff			, 
0xff, KEY_PAGEUP, KEY_RIGHTBRACE,  0xff , KEY_D     , 0xff			, 0xff		  , KEY_S/**/		,/* 0x50~0x57 */
0xff, 0xff		, 0xff		, 0xff		, 0xff	    , 0xff			, 0xff		  , 0xff			, 
0xff, KEY_COMMA , KEY_SEMICOLON, KEY_TAB, KEY_DOLLAR, KEY_S			, KEY_S		  , 0xff			,/* 0x60~0x67 */
0xff, 0xff		, 0xff		, 0xff		, 0xff		, 0xff	    	, 0xff		  , 0xff			,
0
};
#endif

#endif
#define KBDIRQNO(kbdrec)  MSM_GPIO_TO_INT(GPIO_IRQ_PP2106M2) /*kbdrec->mykeyboard->irq)*/
#define QKBD_PHYSLEN 128

#define KEY_SCL_PIN		GPIO_SCL_PP2106M2 /*13*/
#define KEY_SDA_PIN		GPIO_SDA_PP2106M2 /*14*/

#define QWERTY_SDA_OUTPUT()	{ gpio_configure(KEY_SDA_PIN,GPIOF_DRIVE_OUTPUT); udelay(25);}
#define QWERTY_SDA_HIGH()	{ gpio_set_value(KEY_SDA_PIN,GPIO_HIGH_VALUE); udelay(25); }
#define QWERTY_SDA_LOW()	{ gpio_set_value(KEY_SDA_PIN,GPIO_LOW_VALUE); udelay(25); }
#define QWERTY_SDA_INPUT()	{ gpio_configure(KEY_SDA_PIN,GPIOF_INPUT); udelay(25);}
#define QWERTY_SDA_READ()	gpio_get_value(KEY_SDA_PIN)
#define QWERTY_SCL_OUTPUT()	{ gpio_configure(KEY_SCL_PIN,GPIOF_DRIVE_OUTPUT); udelay(25); }
#define QWERTY_SCL_HIGH()	{ gpio_set_value(KEY_SCL_PIN,GPIO_HIGH_VALUE); udelay(25); }
#define QWERTY_SCL_LOW()	{ gpio_set_value(KEY_SCL_PIN,GPIO_LOW_VALUE); udelay(25); }
/*
 * The qwerty_kbd_record structure consolates all the data/variables
 * specific to managing the single instance of the keyboard.
 */
struct qwerty_kbd_record {
	struct i2c_client *mykeyboard;
	//struct input_dev *i2ckbd_idev;
	//struct input_dev *kbd_idev;
	int	product_info;
	char	physinfo[QKBD_PHYSLEN];
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
	//unsigned char xltable[QKBD_IN_MXKYEVTS];
};


static struct input_dev *pp2106m2_kbd_dev;
static struct qwerty_kbd_record kbd_data;
static struct work_struct qkybd_irqwork;
#if 0  /*Rev.B with Function key*/
static int keysympressed ;
#endif

extern void lg_enhanced_menukey_func(int value);
#define LG_MENUKEY_VALUE 0x8b
static int  __init qwerty_kbd_probe(struct platform_device *pdev);

static void at_fkpd_fetchkeys (struct work_struct *work); //antispoon test
DECLARE_WORK(fkpd_work, at_fkpd_fetchkeys); //antispoon test

// static int fkpd_press_delay, fkpd_release_delay, fkpd_work_index =0,
static int gkpd_state, gkpd_last_index, gkpd_value[21],fkpd_temp_index=0,  fkpd_type=0;
static int fkpd_buffer[41] = { 200, 200, 200, 200, 200,   200, 200, 200, 200, 200,   200, 200, 200, 200, 200,   200, 200, 200, 200, 200, 
	                  200, 200, 200, 200, 200,   200, 200, 200, 200, 200,   200, 200, 200, 200, 200,   200, 200, 200, 200, 200, 200};
	                  
int at_gkpd_cfg(unsigned int type, int value) //0,0 : READ STATE, 0,1 : READ LEFT VALUE, 1,0 : WRITE DISABLE VALUE, 1,1 : WRITE ENABLE VALUE
{
	int i,retval;
	if (type == 1) {
		gkpd_value[0] = 0;
		gkpd_last_index = 0;
		retval = 0;
		if (value == 1) {
			gkpd_state = 1;				
		}		
		else {
			gkpd_state = 0;	
		}
	}
	else {
		if (value == 0) {
			retval = gkpd_state;
		}
		else {
			retval = gkpd_value[0];			
			for (i = 0; i < gkpd_last_index; i++){
				gkpd_value[i] = gkpd_value[i+1];
			}
			gkpd_value[gkpd_last_index] = 0;
			if ( retval == 0 ) gkpd_last_index = 0;			
		}
	}
	return retval;
}
void write_gkpd_value(int value)
{
	int i;
	if (gkpd_state == 1) {
		if ( value != 0xff ) { 
			if (gkpd_last_index == 20) {
				gkpd_value[gkpd_last_index] = value;
				for ( i = 0; i < 20 ; i++) {
					gkpd_value[i] = gkpd_value[i+1];
				}			
				gkpd_value[gkpd_last_index] = 0;
			}
			else {
				gkpd_value[gkpd_last_index] = value;
				gkpd_value[gkpd_last_index+1] = 0;
				gkpd_last_index ++;
			}
		}		
	}	
}

int fkpd_xlkcode (int value)
{
	int i,index = 99;
	for ( i = 14; i < 42; i++){
			if ( value == at_fkpd_keycode[i] ){
				index = i;
			}
		}
	return index;	
}
void manual_fkpd_input (struct input_dev *dev, int value)
{		
		int index;		
		index = fkpd_xlkcode(value);
		//printk("auto fkpd %d %c\n",at_fkpd_keycode[index-14], value);
		if ( index < 28 ) {
			input_report_key(dev, at_fkpd_keycode[index-14], QKBD_IN_KEYPRESS);
			input_sync(dev);
			mdelay (100);
		}
		else if ( index >27 && index < 42 ) {
			input_report_key(dev, at_fkpd_keycode[index-28], QKBD_IN_KEYRELEASE);
			input_sync(dev);
			mdelay (100);
		}
		else {
			//printk("manual input error%d\n",value);
		}				
}

void auto_fkpd_input (struct input_dev *dev, int value, int time1, int time2)
{
		//struct input_dev *idev = pp2106m2_kbd_dev; 
		int index;
		index = fkpd_xlkcode(value);
		//printk("auto fkpd %d %c\n",at_fkpd_keycode[index-14], value);
		if ( index < 28 ) {
			input_report_key(dev, at_fkpd_keycode[index-14], QKBD_IN_KEYPRESS);
			input_sync(dev);
			mdelay (time1*100);
			input_report_key(dev, at_fkpd_keycode[index-14], QKBD_IN_KEYRELEASE);
			input_sync(dev);
			mdelay (time2*100);
		}
		else {
			//printk("auto input error%d\n",value);
		}				
}

static void at_fkpd_fetchkeys (struct work_struct *work)
{		
	struct input_dev *idev = pp2106m2_kbd_dev; 
	int i, press_delay=0, release_delay=0, type = 0;
	//printk (" antispoon \n");
	//mdelay(800);
	//for (i=0;i<41;i++) printk("[%d]",fkpd_buffer[i]); //test antispoon
	//printk (" %dst antispoon \n",i);
	//for ( j =0; j < 2 ; j++) {
		for ( i = 0; i < 40 ; i++ ) { //printk ("type[%d]buffer[%d]\n",type,fkpd_buffer[i]);
			if ( fkpd_buffer[i] != 200) {
				if ( type == 0 ) {
					press_delay = fkpd_buffer[i];
					fkpd_buffer[i] = 200;
					type =1;
				}
				else if ( type == 1 ) {
					release_delay = fkpd_buffer[i];
					fkpd_buffer[i] = 200;
					type = 2;
				}
				else if ( type == 2 ) {
					if ((press_delay == 0) && (release_delay == 0)) {
						fkpd_buffer[i] = 200;
						type = 3;	
					}
					else {
						fkpd_buffer[i] = 200;
						type = 4;	
					}
				}
				else if (type == 3) {
					manual_fkpd_input ( idev, fkpd_buffer[i] );	
					fkpd_buffer[i] = 200;
					type = 5;
				}
				else if (type ==4) {
					if ( fkpd_buffer[i] == 34 ){
						fkpd_buffer[i] = 200;
						type =0;	
					}
					else {
						auto_fkpd_input ( idev, fkpd_buffer[i], press_delay, release_delay );
						fkpd_buffer[i] = 200;
						type = 4;
					}
				}
				else if (type == 5) {
					fkpd_buffer[i] = 200;
					type = 0;	
				}
				else {
					//printk("out of gkpd\n");
					//fkpd_buffer[i] = 200;
					//type = 0;
				}
			}	
		}			
	//for (i=0;i<41;i++) printk("[%d]",fkpd_buffer[i]); //test antispoon
	//}
}

int at_fkpd_cfg ( unsigned int type, int value)
{	
	//printk (" type=%d value=%d fk_index=%d fk_type=%d\n",type,value,fkpd_temp_index,fkpd_type); //antispoon test
	fkpd_buffer[fkpd_temp_index] = value;
	fkpd_buffer[fkpd_temp_index+1] = 200;
	if (type == 0 ) {
		fkpd_type = 0;
		fkpd_temp_index = 0;
		fkpd_buffer[0]=200;
	}
	else {			
		if ( (value == 34) && (fkpd_type == 0) ) {
			//set_slide_open(0);  //antispoon
			fkpd_type = 1;	
		}
		else if ( (value == 34) && (fkpd_type == 1) ) {
			//fkpd_buffer[fkpd_temp_index] = 199;
			fkpd_type = 0;	
			schedule_work(&fkpd_work);  //antispoon0717_test
		}
		else {
			//fkpd_type = 1; 
			if ( value == 199 ){
				fkpd_type = 0;	
				fkpd_buffer[fkpd_temp_index] = 200;
				fkpd_temp_index = 0; //antispoon
				
			}
		}		
	
		if ( fkpd_temp_index < 39 ) {
			fkpd_temp_index++;
		}
		else {
			fkpd_temp_index = 0;	
		}
	}
	return 1; 
}	

__inline void P_ChipSendACK( void )
{
    QWERTY_SDA_OUTPUT();
    QWERTY_SDA_LOW();        // ACK Bit Set

	QWERTY_SCL_LOW();        // ACK Bit Upaate Pulse
	QWERTY_SCL_HIGH();

    QWERTY_SDA_INPUT();
	
	//printk(" %s\n",__FUNCTION__);
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

    QWERTY_SDA_OUTPUT();
    QWERTY_SCL_HIGH();
	QWERTY_SDA_LOW();
	udelay(25);
	//printk(" %s\n",__FUNCTION__);

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

extern void lg_enhanced_menukey_func(int value);

/* translate from scan code(s) to intermediate xlate-lookup code */
static inline void lge_xlscancode(void)
{
	//struct qwerty_kbd_record *kbdrec = &kbd_data;
	struct input_dev *idev = pp2106m2_kbd_dev; /*kbdrec->i2ckbd_idev;*/
	u32 xlkcode = 0;
	u32 buf = 0;
	u8  keystate =0;  /*press = 1 , release = 0*/
	

	P_ChipGetData(&buf);
	//xlkcode = ScanCodeToShortVKeyTable[buf];
	keystate = (buf & 0x80) ? QKBD_IN_KEYRELEASE : QKBD_IN_KEYPRESS;
#if 1 /*REV.A, B*/
	if(keystate == QKBD_IN_KEYPRESS)
		xlkcode = pp2106m2_keycode[buf];
	if(keystate == QKBD_IN_KEYRELEASE)
		xlkcode = pp2106m2_keycode[(buf & 0x7f)];
#else /*Rev.B with Function key*/
	if(keystate == QKBD_IN_KEYPRESS){
		if(keysympressed == 0){
			xlkcode = pp2106m2_keycode[buf];
		}else if(keysympressed == 1 ){
			xlkcode = pp2106m2_keycode_symbol[buf];
		}
	}
		
	if(keystate == QKBD_IN_KEYRELEASE){
		if(keysympressed == 0){
			xlkcode = pp2106m2_keycode[buf & 0x7f];
		}else if(keysympressed == 1 ){
			xlkcode = pp2106m2_keycode_symbol[buf & 0x7f];
		}

		if(xlkcode == 208 /*KEY_FN : 0x1d0*/ && keysympressed == 0){
			keysympressed = 1;
		}else if(xlkcode == 208 /*KEY_FN : 0x1d0*/ && keysympressed == 1){
			keysympressed = 0;
		}
	}	
#endif	

    QDBG("+++++++++ Keypad : org <<0x%x>>  trans <<0x%x>>\n", buf, xlkcode);

	// we have a translated code to feed into the input system 
	if (xlkcode) 
	{


#if 1
	
		input_report_key(idev, xlkcode, keystate);
		if (keystate == 0) {
			write_gkpd_value(at_gkpd_keycode[(buf & 0x7f)]/*xlkcode*/);  //Steal Key Input value for AT%GKPD
		}
		input_sync(idev);
#ifdef CONFIG_KEYLED
//		msm_keyled_send(); //keyled on //code is in keyled_pm.c
#endif
#else
		// [+] 2009.2.9 (KIM KUNWOO, MAS1 SW) : =====================================
		input_report_key(idev, xlkcode, QKBD_IN_KEYPRESS);
		input_sync(idev);
		input_report_key(idev, xlkcode, QKBD_IN_KEYRELEASE);
		input_sync(idev);
		// [-] 2009.2.9 (KIM KUNWOO, MAS1 SW) : =====================================
#endif		
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
// Detect Falling edge
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
	int kbd_irqpin  = kbrec->mykeyboard->irq;
	int kbd_mclrpin = kbrec->mclrpin;
	printk("%s\n",__FUNCTION__);

	dev_info(&kbrec->mykeyboard->dev,
		 "releasing keyboard gpio pins %d,%d\n",
		 kbd_irqpin, kbd_mclrpin);
	gpio_free(kbd_irqpin);
	gpio_free(kbd_mclrpin);
	return 0;
}

/*
 * Configure the (2) external gpio pins connected to the keyboard.
 * interrupt(input), reset(output).
 */
static int qwerty_kbd_config_gpio(struct qwerty_kbd_record *kbrec)
{
	int rc;
	struct device *kbdev = &kbrec->mykeyboard->dev;
//	int kbd_irqpin  = kbrec->mykeyboard->irq;
//	int kbd_mclrpin = kbrec->mclrpin;



QDBG(" %s\n",__FUNCTION__);

//	rc = gpio_request(kbd_irqpin, "gpio_keybd_irq");
#if 0
	rc = gpio_request(GPIO_IRQ_PP2106M2, "gpio_keybd_irq");
	if (rc) {
		dev_err(kbdev, "gpio_request failed on pin %d (rc=%d)\n",
			kbd_irqpin, rc);
		goto err_gpioconfig;
	}
#endif


//	rc = gpio_request(kbd_mclrpin, "gpio_keybd_reset");
	rc = gpio_request(GPIO_RESET_PP2106M2, "gpio_keybd_reset");
	if (rc) {
		dev_err(kbdev, "gpio_request failed on pin %d (rc=%d)\n",
			GPIO_RESET_PP2106M2, rc);
			//kbd_mclrpin, rc);
		goto err_gpioconfig;
	}
#if 0

//	rc = gpio_direction_input(kbd_irqpin);
	rc = gpio_direction_input(GPIO_IRQ_PP2106M2);
	if (rc) {
		dev_err(kbdev, "gpio_direction_input failed on "
		       "pin %d (rc=%d)\n", GPIO_IRQ_PP2106M2, rc);
		goto err_gpioconfig;
	}
#endif

//	rc = gpio_direction_output(kbd_mclrpin, 1);
	rc = gpio_direction_output(GPIO_RESET_PP2106M2, 1);
	if (rc) {
		dev_err(kbdev, "gpio_direction_output failed on "
		       "pin %d (rc=%d)\n", GPIO_IRQ_PP2106M2, rc);
		goto err_gpioconfig;
	}


	
	return rc;

err_gpioconfig:
	qwerty_kbd_release_gpio(kbrec);
	return rc;
}

/* use gpio output pin to toggle keyboard external reset pin */
static void qwerty_kbd_hwreset(int kbd_mclrpin)
{
    //printk("++++++++++ KEY RESET!!!!\n");
	QDBG(" %s\n",__FUNCTION__);

    QWERTY_SCL_HIGH();
    QWERTY_SCL_OUTPUT();

	// Reset
	gpio_set_value(kbd_mclrpin, 1);
	msleep(25);
	gpio_set_value(kbd_mclrpin, 0);
	msleep(25);
	gpio_set_value(kbd_mclrpin, 1);
	msleep(25);
}

static void qwerty_kbd_fetchkeys(struct work_struct *work)
{	QDBG(" %s\n",__FUNCTION__);
	lge_xlscancode();
}

static void qwerty_kbd_shutdown(struct qwerty_kbd_record *rd)
{	
	QDBG(" %s\n",__FUNCTION__);
	if (rd->kybd_connected) {
		dev_info(&rd->mykeyboard->dev, "disconnecting keyboard\n");
		free_irq(KBDIRQNO(rd), NULL);
		qwerty_kbd_hwreset(rd->mclrpin);
		rd->kybd_connected = 0;
	}
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

//	qwerty_kbd_shutdown(rd);
//	qwerty_kbd_release_gpio(rd);
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
	.probe	 = qwerty_kbd_probe,
	.remove	 = qwerty_kbd_remove,
	.suspend = qwerty_kbd_suspend,
	.resume  = qwerty_kbd_resume,
	//.command = qwerty_kbd_command,
};

static int  __init qwerty_kbd_probe(struct platform_device *pdev)
{
//	struct msm_i2ckbd_platform_data *setup_data;
	int                              rc = 0, err;
	struct qwerty_kbd_record           *rd = &kbd_data;
	int kidx = 0;
	QDBG(" %s\n",__FUNCTION__);

	pp2106m2_kbd_dev = input_allocate_device();
	if (!pp2106m2_kbd_dev) {
		printk(KERN_ERR "amikbd: not enough memory for input device\n");
		err = -ENOMEM;
		//goto fail1;
	}
	
/*
	if (rd->mykeyboard) {
		dev_err(&client->dev,
			"only a single i2c keyboard is supported\n");
		return -ENODEV;
	}

	if (!client->dev.platform_data) {
		dev_err(&client->dev,
			"keyboard platform device data is required\n");
		return -ENODEV;
	}
	
*/	
#if 0  /*Rev.B with Function key*/
	keysympressed=0;
#endif

	pp2106m2_kbd_dev->name = KEY_DRIVER_NAME;
    pp2106m2_kbd_dev->phys = "pp2106m2/input1";
    pp2106m2_kbd_dev->id.bustype = BUS_HOST;
    pp2106m2_kbd_dev->id.vendor = 0x0001;
    pp2106m2_kbd_dev->id.product = 0x0001;
    pp2106m2_kbd_dev->id.version = 0x0100;
    pp2106m2_kbd_dev->dev.parent = &pdev->dev;
	
	pp2106m2_kbd_dev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_REP);

	if(pp2106m2_kbd_dev){
//		idev->open = qwerty_kbd_opencb;
//		idev->close = qwerty_kbd_closecb;
	pp2106m2_kbd_dev->keycode = kbd_data.pp2106m2_keycode;
	pp2106m2_kbd_dev->keycodesize = sizeof(uint8_t);
	pp2106m2_kbd_dev->keycodemax = QKBD_IN_MXKYEVTS;

	pp2106m2_kbd_dev->evbit[0] = BIT(EV_KEY);
	memset(pp2106m2_kbd_dev->keycode, 0, QKBD_IN_MXKYEVTS);

	pp2106m2_kbd_dev->mscbit[0] = 0;

	// Support all keys 
	for (kidx = 0; kidx <= KEY_DISPLAY_OFF; kidx++)
		set_bit(kidx, pp2106m2_kbd_dev->keybit);
//	input_set_drvdata(pp2106m2_kbd_dev, kbdrec);
	input_set_drvdata(pp2106m2_kbd_dev, rd);

	} else {
		printk("Failed to allocate input device for %s\n",	kbd_name);
	}


/*
	client->driver = &qwerty_kbd_driver;
	i2c_set_clientdata(client, &kbd_data);
	rd->mykeyboard    = client;
	setup_data        = client->dev.platform_data;
	rd->mclrpin       = setup_data->gpioreset;
*/
	// Reset
	//qwerty_kbd_hwreset(setup_data->gpioreset);
	qwerty_kbd_hwreset(GPIO_RESET_PP2106M2);

	rc = qwerty_kbd_config_gpio(rd);
	/*
	if (!rc) {
		rc = testfor_keybd(client, 0);
		if (rc)
			qwerty_kbd_release_gpio(rd);
	}
*/

	INIT_WORK(&qkybd_irqwork, qwerty_kbd_fetchkeys);
	qwerty_kbd_irqsetup(GPIO_IRQ_PP2106M2);

	err = input_register_device(pp2106m2_kbd_dev);
	if (err)
		printk(" %s\n", __FUNCTION__);
//		goto fail3;
	
	return rc;
}

static int __init qwerty_kbd_init(void)
{
	memset(&kbd_data, 0, sizeof(struct qwerty_kbd_record));
	QDBG(" - 0418 %s\n",__FUNCTION__);

	return platform_driver_register(&qwerty_kbd_driver);
}


static void __exit qwerty_kbd_exit(void)
{
	QDBG(" - 0418 %s\n",__FUNCTION__);
	platform_driver_unregister(&qwerty_kbd_driver);
}

module_init(qwerty_kbd_init);
module_exit(qwerty_kbd_exit);
