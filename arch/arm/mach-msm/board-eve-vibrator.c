/* drivers/android/vibrator.c
 *
 * (C) 2008 LGE, Inc.
 * (C) 2007 Google, Inc.
 *
 * Android vibrator driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/timer.h>
#include <mach/gpio.h>
#include <linux/workqueue.h>
#include <linux/spinlock.h>
#include <asm/io.h>
#include <mach/msm_iomap.h>
#include <mach/vreg.h>
#include <linux/i2c.h>
#include <../../../drivers/staging/android/timed_output.h>
#include "board-eve.h"

#define TRUE		1
#define FALSE		0
#define I2C_NO_REG	0xFF


#define VIBRATOR_DEBUG 0
#if VIBRATOR_DEBUG
#define VDBG(fmt, args...) printk(fmt, ##args)
#else
#define VDBG(fmt, args...) do {} while (0)
#endif /* VIBRATOR_DEBUG */

#define GPIO_LIN_MOTOR_PWM		28
#define GPIO_LIN_MOTOR_EN		57

#define GP_MN_CLK_MDIV			0x004C
#define GP_MN_CLK_NDIV			0x0050
#define GP_MN_CLK_DUTY			0x0054

#define GPMN_M_DEFAULT			21
#define GPMN_N_DEFAULT			4500
#define GPMN_D_DEFAULT			2250
#define PWM_MULTIPLIER			4394

#define GPMN_M_MASK				0x01FF
#define GPMN_N_MASK				0x1FFF
#define GPMN_D_MASK				0x1FFF

#define REG_WRITEL(value, reg)	writel(value, (MSM_WEB_BASE+reg))

#define JIFFIES_TO_MS(t) ((t) * 1000 / HZ)
#define MS_TO_JIFFIES(j) ((j * HZ) / 1000)

#define MAX_TIMEOUT_MS   (15000)

static void vibrator_work_func(struct work_struct *work);
DECLARE_WORK(vibrator_work, vibrator_work_func);

static void vibrator_timeout(unsigned long arg);
static atomic_t s_vibrator = ATOMIC_INIT(0);
static struct timer_list s_timer = TIMER_INITIALIZER(vibrator_timeout, 0, 0);
static atomic_t s_amp = ATOMIC_INIT(1);
static int s_vibstate = 0;
static atomic_t s_vibstate_work_check =  ATOMIC_INIT(0);
static uint8_t vibrator_start_first = FALSE;

#define MOTOR_VOLT_LEVEL	3000 /* 3000mV */
#define LDO_NO_MOTOR		2

int bd6083_set_ldo_power(int ldo_no, int on);
int bd6083_set_ldo_vout(int ldo_no, unsigned vout_mv);

int bd6083_ldo_poweron(void)
{
	if (bd6083_set_ldo_power(LDO_NO_MOTOR, 1)) {
		printk("android_vibrator: failed to set motor poweron\n");
		return -1;
	}
	return 0;
}

int bd6083_ldo_poweroff(void)
{
	if (bd6083_set_ldo_power(LDO_NO_MOTOR, 0)) {
		printk("android_vibrator: failed to set motor poweroff\n");
		return -1;
	}
	return 0;
}

static int vibrator_motor_power_switch(int on)
{
	if (on)
		return bd6083_ldo_poweron();
	else
		return bd6083_ldo_poweroff();
}

static int vibrator_motor_set_level(int mvolt)
{
	if (bd6083_set_ldo_vout(LDO_NO_MOTOR, mvolt)) {
		printk("android_vibrator: failed to set motor poweron\n");
		return -1;
	}
	return 0;
}

static void vibrator_disable(void)
{

	gpio_set_value(GPIO_LIN_MOTOR_EN, 0);
	gpio_set_value(GPIO_LIN_MOTOR_PWM, 0);

	vibrator_motor_power_switch(0);

	s_vibstate = 0;
}

static void vibrator_enable(void)
{

	REG_WRITEL((GPMN_M_DEFAULT & GPMN_M_MASK), GP_MN_CLK_MDIV);
	REG_WRITEL((~( GPMN_N_DEFAULT - GPMN_M_DEFAULT )&GPMN_N_MASK), GP_MN_CLK_NDIV);

	gpio_set_value(GPIO_LIN_MOTOR_EN, 1);
	gpio_set_value(GPIO_LIN_MOTOR_PWM, 1);

	vibrator_motor_power_switch(1);

	s_vibstate = 1;
}

static void vibrator_set(int amp)
{
	int gain;

	VDBG("android-vibrator: vibrator_set amp=%d\n",amp);

	if(amp == 0) {
		if(s_vibstate)
			vibrator_disable();
	}
	else {
		if(!s_vibstate)
			vibrator_enable();

		if(!vibrator_start_first){
			vibrator_motor_set_level(MOTOR_VOLT_LEVEL);
			vibrator_start_first = TRUE;
		}

		gain = ((PWM_MULTIPLIER * amp) >> 8) + GPMN_D_DEFAULT ;
		REG_WRITEL((gain & GPMN_D_MASK), GP_MN_CLK_DUTY);
	}
}

static int is_vib_state(void)
{
	return s_vibstate;
}

static void vibrator_work_func(struct work_struct *work)
{
	vibrator_set(0);
}

static void vibrator_timeout(unsigned long arg __attribute__((unused)))
{
	vibrator_set(0);
	atomic_set(&s_vibstate_work_check ,0);
}

static void to_vibrator_enable(struct timed_output_dev *sdev, int timeout) {

	VDBG("vibrator_enable(%d)\n",timeout);
	atomic_set(&s_amp, 120);
	atomic_set(&s_vibrator, timeout);

	/*
	 * value == -1 : infinite vibration
	 * value == 0 : stop vibration
	 * value > 0 : vibrate during <value> miliseconds
	 */
	if(atomic_read(&s_vibrator)>0){
		if (atomic_read(&s_vibstate_work_check) ==0){
			del_timer(&s_timer);
			vibrator_set(atomic_read(&s_amp));
			s_timer.expires = jiffies + MS_TO_JIFFIES(atomic_read(&s_vibrator));
			add_timer(&s_timer);
			atomic_set(&s_vibstate_work_check ,1);
		}
	}
	else if ( atomic_read(&s_vibrator) == 0){
		if (atomic_read(&s_vibstate_work_check) ==1){
			del_timer(&s_timer);

			vibrator_set(0);
			atomic_set(&s_vibstate_work_check ,0);
		}
	}
}

static int to_vibrator_get_time(struct timed_output_dev *sdev) {
	VDBG("vibrator_get_time\n");
	return atomic_read(&s_vibrator);
}

static struct timed_output_dev pmic_vibrator = {
	.name = "vibrator",
	.get_time = to_vibrator_get_time,
	.enable = to_vibrator_enable,
};


static int __init android_vibrator_init(void)
{
	printk( "android-vibrator: init\n");

	gpio_request(GPIO_LIN_MOTOR_EN, "gpio_lin_motor_en");
	gpio_request(GPIO_LIN_MOTOR_PWM, "gpio_lin_motor_pwm");

	/* Off Enable */
	gpio_set_value(GPIO_LIN_MOTOR_EN, 0);

	REG_WRITEL((GPMN_M_DEFAULT & GPMN_M_MASK), GP_MN_CLK_MDIV);
	REG_WRITEL((~( GPMN_N_DEFAULT - GPMN_M_DEFAULT )&GPMN_N_MASK), GP_MN_CLK_NDIV);
	gpio_set_value(GPIO_LIN_MOTOR_PWM, 0);

	return timed_output_dev_register(&pmic_vibrator);
}

module_init(android_vibrator_init);

MODULE_AUTHOR("Dae il, yu <diyu@lge.com>");
MODULE_DESCRIPTION("Eve vibrator driver for Android");
MODULE_LICENSE("GPL");
