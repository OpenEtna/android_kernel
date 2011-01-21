/* drivers/input/touchscreen/msm_touch.c
 *
 * Copyright (c) 2008-2009, Code Aurora Forum. All rights reserved.
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
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/platform_device.h>
#include <linux/jiffies.h>
#include <linux/io.h>

#include <mach/msm_touch.h>

/* the data to be read from registers */
/* LGE_CHANGE_S [dojip.kim@lge.com] 2010-03-24, from cupcake */
#if defined(CONFIG_MACH_EVE)
#define TS_X_MAX	2752/*690*/	/*1024 */
#define TS_X_MIN	1324/*330*/	/*1024 */
#define TS_Y_MAX	3314	/*1024 */
#define TS_Y_MIN	748	/*1024 */
#else
#define TS_X_MAX	690
#define TS_X_MIN	330
#define TS_Y_MAX	820
#define TS_Y_MIN	160
#endif
/* LGE_CHANGE_E [dojip.kim@lge.com] 2010-03-24 */
#define TS_X_OFFSET 	20		/*15 */
#define TS_Y_OFFSET 	15
/* HW register map */
#define TSSC_CTL_REG      0x100
#define TSSC_SI_REG       0x108
#define TSSC_OPN_REG      0x104
#define TSSC_STATUS_REG   0x10C
#define TSSC_AVG12_REG    0x110

/* status bits */
#define TSSC_STS_OPN_SHIFT 0x6
#define TSSC_STS_OPN_BMSK  0x1C0
#define TSSC_STS_NUMSAMP_SHFT 0x1
#define TSSC_STS_NUMSAMP_BMSK 0x3E

/* CTL bits */
#define TSSC_CTL_EN		(0x1 << 0)
#define TSSC_CTL_SW_RESET	(0x1 << 2)
#define TSSC_CTL_MASTER_MODE	(0x3 << 3)
#define TSSC_CTL_AVG_EN		(0x1 << 5)
#define TSSC_CTL_DEB_EN		(0x1 << 6)
#define TSSC_CTL_DEB_12_MS	(0x2 << 7)	/* 1.2 ms */
#define TSSC_CTL_DEB_16_MS	(0x3 << 7)	/* 1.6 ms */
#define TSSC_CTL_DEB_2_MS	(0x4 << 7)	/* 2 ms */
#define TSSC_CTL_DEB_3_MS	(0x5 << 7)	/* 3 ms */
#define TSSC_CTL_DEB_4_MS	(0x6 << 7)	/* 4 ms */
#define TSSC_CTL_DEB_6_MS	(0x7 << 7)	/* 6 ms */
#define TSSC_CTL_INTR_FLAG1	(0x1 << 10)
#define TSSC_CTL_DATA		(0x1 << 11)
#define TSSC_CTL_SSBI_CTRL_EN	(0x1 << 13)

/* LGE_CHANGE [dojip.kim@lge.com] 2010-03-24, from cupcake */
//TSSC_CTL_DEB_12_MS 
/* control reg's default state */
#define TSSC_CTL_STATE	  ( \
		TSSC_CTL_DEB_4_MS| \
		TSSC_CTL_DEB_EN | \
		TSSC_CTL_AVG_EN | \
		TSSC_CTL_MASTER_MODE | \
		TSSC_CTL_EN)

/* LGE_CHANGE_S [dojip.kim@lge.com] 2010-03-24, from cupcake */
#define FEATURE_TSSC_D1_FILTER 	1
#define TBSIZE 			2

#define TSSC_SI_STATE 		0x05

#define TSSC_AVG34_REG     	0x114
#define TSHK_ADC_4WIRE_X_OP	0x01
#define TSHK_ADC_4WIRE_Y_OP	0x02
#define TSHK_ADC_4WIRE_Z1_OP	0x03
#define TSHK_ADC_4WIRE_Z2_OP	0x04
/* LGE_CHANGE_E [dojip.kim@lge.com] 2010-03-24 */

/* LGE_CHANGE_S [munyoung@lge.com] change default sampling rate for tssc */
#ifdef CONFIG_MACH_EVE
#define TSSC_DEFAULT_SAMPLING_INTERVAL  10
#endif
/* LGE_CHANGE_E */

/* LGE_CHANGE [munyoung@lge.com] change number of operation due to z axis */
//#define TSSC_NUMBER_OF_OPERATIONS 2
#define TSSC_NUMBER_OF_OPERATIONS 4
/* LGE_CHANGE_S [diyu@lge.com] 2009-04-15*/
#if defined(CONFIG_MACH_EVE)
// princlee change 120msec -> 60msec, and send downEvent Every 3 ea.
/* LGE_CHANGE_S [luckyjun77@lge.com] 2009-10-17, touch tunning*/
//#define TS_PENUP_TIMEOUT_MS 40 /*150: This value is slow to move*/ /*100: This value is to shake touch */     /*150 */
#define TS_PENUP_TIMEOUT_MS 28
/* LGE_CHANGE_E [luckyjun77@lge.com] 2009-10-17*/
#else
#define TS_PENUP_TIMEOUT_MS 20
#endif
/* LGE_CHANGE_E [diyu@lge.com] 2009-04-15*/

#define TS_DRIVER_NAME "msm_touchscreen"

/* LGE_CHANGE_S [diyu@lge.com] 2009-04-15*/
#if 1
/* LGE_CHANGE_S [luckyjun77@lge.com] 2009-10-14*/
enum touch_prereject_count {
	TOUCH_REJECTCOUNT0 = 0,
	TOUCH_REJECTCOUNT1,
	TOUCH_REJECTCOUNT2,
	TOUCH_REJECTCOUNT3,
	TOUCH_REJECTCOUNT4,
	TOUCH_REJECTCOUNT5,
	TOUCH_REJECTCOUNT6,
	TOUCH_REJECTCOUNT7,
};
/* LGE_CHANGE_E [luckyjun77@lge.com] 2009-10-14*/
#endif

#if defined(CONFIG_MACH_EVE)
#define X_MAX	690		/*1024 */
#define Y_MAX	820		/*1024 */
#define P_MAX	256
#else
#define X_MAX	790		/*1024 */
#define Y_MAX	825		/*1024 */
#define P_MAX	256
#endif

struct ts {
	struct input_dev *input;
	struct timer_list timer;
	int irq;
	unsigned int x_max;
	unsigned int y_max;
	//LGE_CHANGE_S by cleaneye@lge.com, For performance, 2009.8.26
	unsigned int count;
	int x_lastpt;
	int y_lastpt;
	//LGE_CHANGE_E by cleaneye@lge.com
};
//LGE_CHANGE_S by cleaneye@lge.com,  2009.8.26
#define X_DISTANCE  20
#define Y_DISTANCE  21
//LGE_CHANGE_E by cleaneye@lge.com
/* LGE_CHANGE_S, [munyoung@lge.com] touch tuning */
static int s_sampling_int = TSSC_DEFAULT_SAMPLING_INTERVAL;
static int s_penup_time = TS_PENUP_TIMEOUT_MS;
/* LGE_CHANGE_E */

/* LGE_CHANGE_S, [luckyjun77@lge.com] 2009-10-17, touch tuning */
static int PreRejectTouchCount = 0;
static int ReRejectTouchCount = 0;

#define DISTANCE_VALUE	40
static int preRejectValue = TOUCH_REJECTCOUNT0;
//static int postRejectValue = TOUCH_REJECTCOUNT0;
static int distanceValue = DISTANCE_VALUE;
/* LGE_CHANGE_E, [luckyjun77@lge.com]  2009-10-17*/
/* LGE_CHANGE_S [dojip.kim@lge.com] 2010-03-24, from cupcake */
static int SkipToggle = 0;
static int TouchWindowPress = 1;	//  touch  1 : press,  0: release
/* LGE_CHANGE_E [dojip.kim@lge.com] 2010-03-24 */

static void __iomem *virt;
#define TSSC_REG(reg) (virt + TSSC_##reg##_REG)

/* LGE_CHANGE [cleaneye@lge.com] 2009-02-24, for debug */
#if defined(CONFIG_MACH_EVE)
//#define DEBUG
#if defined(DEBUG)
#define DBG(fmt, args...) printk(fmt, ##args)
#else
#define DBG(fmt, args...) 	do {} while(0);
#endif
#endif /* CONFIG_MACH_EVE */

#ifdef FEATURE_TSSC_D1_FILTER
typedef struct {
	int x;
	int y;
} point;

static int xbuf[2] = { 0 };
static int ybuf[2] = { 0 };

//static point res;
#endif

/* LGE_CHANGE_S [bluerti@lge.com] 2009-09-01 */
struct timer_list lg_enhanced_touch;
extern void lg_block_touch_event_func(int value);
extern int msm_touch_timer_value;
extern int msm_touch_option;

static void lg_enhanced_touch_timer(unsigned long arg)
{
	lg_block_touch_event_func(0);	// Free Touch Event
}

/* LGE_CHANGE_E [bluerti@lge.com] 2009-09-01 */

/* LGE_CHANGE_S, [munyoung@lge.com] touch tunning */
#ifdef CONFIG_MACH_EVE
static ssize_t penup_time_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", s_penup_time);
}

static ssize_t penup_time_store(struct device *dev,
				struct device_attribute *attr, const char *buf,
				size_t size)
{
	int value;

	sscanf(buf, "%d", &value);
	s_penup_time = value;

	return size;
}

static ssize_t sampling_int_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", s_sampling_int);
}

static ssize_t sampling_int_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t size)
{
	int value;

	sscanf(buf, "%d", &value);
	if (value < 1)
		value = 1;
	if (value > 31)
		value = 31;

	s_sampling_int = value;

	return size;
}

static DEVICE_ATTR(sampling_int, S_IRUGO | S_IWUSR, sampling_int_show,
		   sampling_int_store);
static DEVICE_ATTR(penup_time, S_IRUGO | S_IWUSR, penup_time_show,
		   penup_time_store);
#endif
/* LGE_CHANGE_E */

/* LGE_CHANGE [dojip.kim@lge.com] 2010-03-24, from cupcake */
#ifdef FEATURE_TSSC_D1_FILTER
static int dist(point * p1, point * p2)
{
	return abs(p1->x - p2->x) + abs(p1->y - p2->y);
}

static point **sort(point * p1, point * p2, point * p3)
{
	static point *sorted[2];
	int d_1_2, d_1_3, d_2_3;

	d_1_2 = dist(p1, p2);
	d_1_3 = dist(p1, p3);
	d_2_3 = dist(p2, p3);

	if (d_1_2 < d_1_3) {
		if (d_1_2 < d_2_3) {
			sorted[0] = p1;
			sorted[1] = p2;
		} else {
			sorted[0] = p2;
			sorted[1] = p3;
		}
	} else {
		if (d_1_3 < d_2_3) {
			sorted[0] = p1;
			sorted[1] = p3;
		} else {
			sorted[0] = p2;
			sorted[1] = p3;
		}
	}
	return sorted;
}

static point pbwa(point points[])
{
	point result;
	point **sorted = sort(&points[0], &points[1], &points[2]);
	result.x = (sorted[0]->x * 3 + sorted[1]->x) >> 2;
	result.y = (sorted[0]->y * 3 + sorted[1]->y) >> 2;
	return result;
}

#endif

/* LGE_CHANGE_S, [munyoung@lge.com] */
static void block_touch_key(void)
{
	// LGE_CHANGE_S [bluerti@lge.com] 2009-09-01 
	if (msm_touch_timer_value) {
		if (msm_touch_option != 0)
			lg_block_touch_event_func(1);	//Block touch event 

		mod_timer(&lg_enhanced_touch,
			  jiffies + (msm_touch_timer_value * HZ / 1000));
	} else
		lg_block_touch_event_func(0);	// Free Touch Event

	// LGE_CHANGE_S [bluerti@lge.com] 2009-09-01 
}

/* LGE_CHANGE_E */

//LGE_CHANGE_S by cleaneye@lge.com for performance,  2009.8.26
static int ts_check_region(struct ts *ts, int x, int y, int pressure)
{
	int update_event = false;
	int x_axis, y_axis;
	int x_diff, y_diff;
	x_axis = x;
	y_axis = y;

	if (ts->count == 0) {
		ts->x_lastpt = x_axis;
		ts->y_lastpt = y_axis;

		update_event = true;
		TouchWindowPress = 1;
		ts->count = ts->count + 1;
	}

	x_diff = x_axis - ts->x_lastpt;
	if (x_diff < 0)
		x_diff = x_diff * -1;
	y_diff = y_axis - ts->y_lastpt;
	if (y_diff < 0)
		y_diff = y_diff * -1;

	/* LGE_CHANGE_S, [luckyjun77@lge.com] 2009-10-17, touch tuning */
	//if( (x_diff < X_DISTANCE) && (y_diff < Y_DISTANCE) ){    //original code
	if ((x_diff < distanceValue) && (y_diff < distanceValue)) {
	/* LGE_CHANGE_E, [luckyjun77@lge.com] 2009-10-17*/
		x_axis = ts->x_lastpt;
		y_axis = ts->y_lastpt;
	} else {
		ts->x_lastpt = x_axis;
		ts->y_lastpt = y_axis;

		x = x_axis;
		y = y_axis;

		update_event = true;
		TouchWindowPress = 1;
	}

	return update_event;
}

//LGE_CHANGE_E by cleaneye@lge.com  

static void ts_update_pen_state(struct ts *ts, int x, int y, int pressure)
{
	if (pressure) {
		/* LGE_CHANGE_S [dojip.kim@lge.com] 2010-03-24, from cupcake */
		if (ts_check_region(ts, x, y, pressure) == false)
			return;
		/* LGE_CHANGE_E [dojip.kim@lge.com] 2010-03-24 */
		input_report_abs(ts->input, ABS_X, x);
		input_report_abs(ts->input, ABS_Y, y);
		input_report_abs(ts->input, ABS_PRESSURE, pressure);
		input_report_key(ts->input, BTN_TOUCH, !!pressure);
		/* LGE_CHANGE [dojip.kim@lge.com] 2010-03-24, from cupcake */
		input_sync(ts->input);
	} else {
		input_report_abs(ts->input, ABS_PRESSURE, 0);
		input_report_key(ts->input, BTN_TOUCH, 0);
		/* LGE_CHANGE_S [dojip.kim@lge.com] 2010-03-24, from cupcake */
		input_sync(ts->input);
		ts->count = 0;
		/* LGE_CHANGE_E [dojip.kim@lge.com] 2010-03-24 */
	}

	/* LGE_CHANGE [dojip.kim@lge.com] 2010-03-24, from cupcake: no sync */
	//input_sync(ts->input);
}

static void ts_timer(unsigned long arg)
{
	struct ts *ts = (struct ts *)arg;

	ts_update_pen_state(ts, 0, 0, 0);
	/* LGE_CHANGE_S [dojip.kim@lge.com] 2010-03-24, from cupcake */
	ts->count = 0;
	TouchWindowPress = 0;
	ReRejectTouchCount = 0;
	PreRejectTouchCount = 0;
	/* LGE_CHANGE_E [dojip.kim@lge.com] 2010-03-24 */
}

/* LGE_CHANGE [dojip.kim@lge.com] 2010-03-24, from cupcake */
int is_touch_pressed(void)
{
	DBG("\n%s TouchWindowPress %d\n", __FUNCTION__, TouchWindowPress);
	return TouchWindowPress;
}

/* LGE_CHANGE_S, [munyoung@lge.com] setup operation for z axis */
static int ts_set_op(void)
{
	uint32_t tmp;

	/* LGE_CHANGE_S, [luckyjun77@lge.com] 2009-10-17, touch tuning */
#if 0				//original code
	/* op1 - measure X, 8 sample, 12bit resolution */
	tmp = (TSHK_ADC_4WIRE_X_OP << 16) | (2 << 8) | (2 << 0);
	/* op2 - measure Y, 8 sample, 12bit resolution */
	tmp |= (TSHK_ADC_4WIRE_Y_OP << 20) | (2 << 10) | (2 << 2);
	/* op3 - measure Z1, 8 sample, 8bit resolution */
	tmp |= (TSHK_ADC_4WIRE_Z1_OP << 24) | (2 << 12) | (0 << 4);
	/* op4 - measure Z2, 8 sample, 8bit resolution */
	tmp |= (TSHK_ADC_4WIRE_Z2_OP << 28) | (2 << 12) | (0 << 4);
#else //added by jykim
	/* op1 - measure X, 4 sample, 12bit resolution */
	tmp = (TSHK_ADC_4WIRE_X_OP << 16) | (1 << 8) | (2 << 0);
	/* op2 - measure Y, 4 sample, 12bit resolution */
	tmp |= (TSHK_ADC_4WIRE_Y_OP << 20) | (1 << 10) | (2 << 2);
	/* op3 - measure Z1, 4 sample, 8bit resolution */
	tmp |= (TSHK_ADC_4WIRE_Z1_OP << 24) | (1 << 12) | (0 << 4);
	/* op4 - measure Z2, 4 sample, 8bit resolution */
	tmp |= (TSHK_ADC_4WIRE_Z2_OP << 28) | (1 << 12) | (0 << 4);
#endif
	/* LGE_CHANGE_E, [luckyjun77@lge.com] 2009-10-17*/

	writel(tmp, TSSC_REG(OPN));

	return 0;
}

/* LGE_CHANGE_E */

static irqreturn_t ts_interrupt(int irq, void *dev_id)
{
	/* LGE_CHANGE [dojip.kim@lge.com] 2010-03-24, from cupcake */
	//u32 avgs, x, y, lx, ly;
	u32 avgs, x, y, lx = 0, ly = 0;
	u32 num_op, num_samp;
	u32 status;
	/* LGE_CHANGE [dojip.kim@lge.com] 2010-03-24, from cupcake */
	u32 avgs34, z1, z2, lz;

	struct ts *ts = dev_id;

	status = readl(TSSC_REG(STATUS));
	avgs = readl(TSSC_REG(AVG12));
	x = avgs & 0xFFFF;
	y = avgs >> 16;

	/* LGE_CHANGE_S [dojip.kim@lge.com] 2010-03-24, from cupcake */
	avgs34 = readl(TSSC_REG(AVG34));
	z1 = avgs34 & 0xFFFF;
	z2 = avgs34 >> 16;
	/* LGE_CHANGE_E [dojip.kim@lge.com] 2010-03-24 */

	/* For pen down make sure that the data just read is still valid.
	 * The DATA bit will still be set if the ARM9 hasn't clobbered
	 * the TSSC. If it's not set, then it doesn't need to be cleared
	 * here, so just return.
	 */
	if (!(readl(TSSC_REG(CTL)) & TSSC_CTL_DATA))
		goto out;

	/* Data has been read, OK to clear the data flag */
	writel(TSSC_CTL_STATE, TSSC_REG(CTL));

	/* LGE_CHANGE [munyoung@lge.com] setup op for z axis */
	ts_set_op();
	/* LGE_CHAGE [munyoung@lge.com] change sampling interval(20ms) */
	writel(s_sampling_int, TSSC_REG(SI));

	/* Valid samples are indicated by the sample number in the status
	 * register being the number of expected samples and the number of
	 * samples collected being zero (this check is due to ADC contention).
	 */
	num_op = (status & TSSC_STS_OPN_BMSK) >> TSSC_STS_OPN_SHIFT;
	num_samp = (status & TSSC_STS_NUMSAMP_BMSK) >> TSSC_STS_NUMSAMP_SHFT;

	if ((num_op == TSSC_NUMBER_OF_OPERATIONS) && (num_samp == 0)) {
		/* TSSC can do Z axis measurment, but driver doesn't support
		 * this yet.
		 */

		/*
		 * REMOVE THIS:
		 * These x, y co-ordinates adjustments will be removed once
		 * Android framework adds calibration framework.
		 */
/* LGE_CHANGE [cleaneye@lge.com] 2009-02-24, invert y axis */
#if defined(CONFIG_MACH_EVE)
		lx = TS_X_MIN + TS_X_MAX - x;
		ly = TS_Y_MIN + TS_Y_MAX - y;
		lz = 255 * z1 / z2;
		//ly = y; //REV.A

		/* LGE_CHANGE_S, [luckyjun77@lge.com] 2009-10-17, touch tunning*/
		//if( PreRejectTouchCount > 4/**/ ){
		if (PreRejectTouchCount > preRejectValue /**/) {
		/* LGE_CHANGE_E, [luckyjun77@lge.com] 2009-10-17*/
			ts_update_pen_state(ts, lx, ly, lz);
		} else {
			PreRejectTouchCount++;
		}
		/* kick pen up timer - to make sure it expires again(!) */
		/* LGE_CHANGE_S, [luckyjun77@lge.com] 2009-10-23, 
		 * move the mod timer to the 'if' statement outside
		 */
		//mod_timer(&ts->timer,
		//	jiffies + msecs_to_jiffies(s_penup_time));
		/* LGE_CHANGE_E, [luckyjun77@lge.com] 2009-10-23*/
		block_touch_key();
	} 
	else {
#ifdef DEBUG
		printk(KERN_INFO "Ignored interrupt: {%3d, %3d},"
		       " op = %3d samp = %3d\n", x, y, num_op, num_samp);
#endif
	}
	/* LGE_CHANGE_S, [luckyjun77@lge.com] 2009-10-23, 
	 * always update the penup timer when the pen is down
	 */
	mod_timer(&ts->timer, jiffies + msecs_to_jiffies(s_penup_time));
	/* LGE_CHANGE_E, [luckyjun77@lge.com] 2009-10-23*/

#else /* qualcomm or google */
#ifdef CONFIG_ANDROID_TOUCHSCREEN_MSM_HACKS
		//lx = ts->x_max - x;
		//ly = ts->y_max - y;
		lx = TS_X_MAX - x;
		ly = TS_Y_MAX - y;
#else
		lx = x;
		ly = y;
#endif
		ts_update_pen_state(ts, lx, ly, 255);
		/* kick pen up timer - to make sure it expires again(!) */
		mod_timer(&ts->timer,
			  jiffies + msecs_to_jiffies(TS_PENUP_TIMEOUT_MS));

	} else
		printk(KERN_INFO "Ignored interrupt: {%3d, %3d},"
		       " op = %3d samp = %3d\n", x, y, num_op, num_samp);

#endif /* CONFIG_MACH_EVE */
out:
	return IRQ_HANDLED;
}

static int __devinit ts_probe(struct platform_device *pdev)
{
	int result;
	struct input_dev *input_dev;
	struct resource *res, *ioarea;
	struct ts *ts;
	unsigned int x_max, y_max, pressure_max;
	struct msm_ts_platform_data *pdata = pdev->dev.platform_data;
	/* LGE_CHANGE_S [dojip.kim@lge.com] 2010-03-24, from cupcake */
	struct touchbutton *touchbutton;
	u32 test_status;
	/* LGE_CHANGE_E [dojip.kim@lge.com] 2010-03-24 */

	/* The primary initialization of the TS Hardware
	 * is taken care of by the ADC code on the modem side
	 */

	/* LGE_CHANGE_S, [munyoung@lge.com] touch tunning */
#ifdef CONFIG_MACH_EVE
	result = device_create_file(&pdev->dev, &dev_attr_sampling_int);
	if (result) {
		printk(KERN_ERR "%s: device_create_file failed\n", __func__);
		return result;
	}

	result = device_create_file(&pdev->dev, &dev_attr_penup_time);
	if (result) {
		printk(KERN_ERR "%s: device_create_file failed\n", __func__);
		device_remove_file(&pdev->dev, &dev_attr_sampling_int);
		return result;
	}
#endif
	/* LGE_CHANGE_E */

	ts = kzalloc(sizeof(struct ts), GFP_KERNEL);
	input_dev = input_allocate_device();
	if (!input_dev || !ts) {
		result = -ENOMEM;
		goto fail_alloc_mem;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "Cannot get IORESOURCE_MEM\n");
		result = -ENOENT;
		goto fail_alloc_mem;
	}

	ts->irq = platform_get_irq(pdev, 0);
	if (!ts->irq) {
		dev_err(&pdev->dev, "Could not get IORESOURCE_IRQ\n");
		result = -ENODEV;
		goto fail_alloc_mem;
	}

	ioarea = request_mem_region(res->start, resource_size(res), pdev->name);
	if (!ioarea) {
		dev_err(&pdev->dev, "Could not allocate io region\n");
		result = -EBUSY;
		goto fail_alloc_mem;
	}

	virt = ioremap(res->start, resource_size(res));
	if (!virt) {
		dev_err(&pdev->dev, "Could not ioremap region\n");
		result = -ENOMEM;
		goto fail_ioremap;
	}

	input_dev->name = TS_DRIVER_NAME;
	input_dev->phys = "msm_touch/input0";
	input_dev->id.bustype = BUS_HOST;
	input_dev->id.vendor = 0x0001;
	input_dev->id.product = 0x0002;
	input_dev->id.version = 0x0100;
	input_dev->dev.parent = &pdev->dev;

	/* LGE_CHANGE [dojip.kim@lge.com] 2010-03-24, from cupcake */
	//input_dev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
	input_dev->evbit[0] =
	    BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS) | BIT_MASK(EV_REP);;
	input_dev->absbit[0] = BIT(ABS_X) | BIT(ABS_Y) | BIT(ABS_PRESSURE);
	input_dev->absbit[BIT_WORD(ABS_MISC)] = BIT_MASK(ABS_MISC);
	input_dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);

	if (pdata) {
		x_max = pdata->x_max ? : X_MAX;
		y_max = pdata->y_max ? : Y_MAX;
		pressure_max = pdata->pressure_max ? : P_MAX;
	} else {
		x_max = X_MAX;
		y_max = Y_MAX;
		pressure_max = P_MAX;
	}

	ts->x_max = x_max;
	ts->y_max = y_max;

/*LGE_CHANGE_S diyu@lge.com. To calibration touch screen */
#if defined(CONFIG_MACH_EVE)
	input_set_abs_params(input_dev, ABS_X, TS_X_MIN, TS_X_MAX, 0, 0);
	input_set_abs_params(input_dev, ABS_Y, TS_Y_MIN, TS_Y_MAX, 0, 0);
	input_set_abs_params(input_dev, ABS_PRESSURE, 0, pressure_max, 0, 0);
#else /*original */
	input_set_abs_params(input_dev, ABS_X, 0, x_max, 0, 0);
	input_set_abs_params(input_dev, ABS_Y, 0, y_max, 0, 0);
	input_set_abs_params(input_dev, ABS_PRESSURE, 0, pressure_max, 0, 0);
#endif
/*LGE_CHANGE_E diyu@lge.com. To calibration touch screen */
	result = input_register_device(input_dev);
	if (result)
		goto fail_ip_reg;

	ts->input = input_dev;

	setup_timer(&ts->timer, ts_timer, (unsigned long)ts);
	// LGE_CHANGE [bluerti@lge.com] 2009-09-01 
	setup_timer(&lg_enhanced_touch, lg_enhanced_touch_timer, 0);
	result = request_irq(ts->irq, ts_interrupt, IRQF_TRIGGER_RISING,
			     "touchscreen", ts);
	if (result)
		goto fail_req_irq;

	platform_set_drvdata(pdev, ts);

	return 0;

fail_req_irq:
	input_unregister_device(input_dev);
	input_dev = NULL;
fail_ip_reg:
	iounmap(virt);
fail_ioremap:
	release_mem_region(res->start, resource_size(res));
fail_alloc_mem:
	input_free_device(input_dev);
	kfree(ts);
	return result;
}

static int __devexit ts_remove(struct platform_device *pdev)
{
	struct resource *res;
	struct ts *ts = platform_get_drvdata(pdev);

	free_irq(ts->irq, ts);
	del_timer_sync(&ts->timer);

	input_unregister_device(ts->input);
	iounmap(virt);
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	release_mem_region(res->start, resource_size(res));
	platform_set_drvdata(pdev, NULL);
	kfree(ts);

	return 0;
}

static struct platform_driver ts_driver = {
	.probe		= ts_probe,
	.remove		= __devexit_p(ts_remove),
	.driver		= {
		.name = TS_DRIVER_NAME,
		.owner = THIS_MODULE,
	},
};

static int __init ts_init(void)
{
	return platform_driver_register(&ts_driver);
}
module_init(ts_init);

static void __exit ts_exit(void)
{
	platform_driver_unregister(&ts_driver);
}
module_exit(ts_exit);

MODULE_DESCRIPTION("MSM Touch Screen driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:msm_touchscreen");
