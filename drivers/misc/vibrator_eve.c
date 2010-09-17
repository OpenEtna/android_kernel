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
#include <linux/spinlock.h> //spinlock for variable of  &s_amp
#include <asm/io.h>
#include <mach/msm_iomap.h>
#include <mach/vreg.h>
/*++ ViBrator setting *///diyu@lge.com
#include <linux/i2c.h>
//LGE_CHANGE [antispoon@lge.com,diyu@lge.com] 2009-07-17 for AT+MOT,GKPD, FKPD
#include <linux/at_kpd_eve.h> 

#define TRUE 		1
#define FALSE 		0
#define I2C_NO_REG 	0xFF

/*-- ViBrator setting *///diyu@lge.com

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
static int s_vibrator_gpio;
static atomic_t s_amp = ATOMIC_INIT(1);
static int s_vibstate = 0;
static atomic_t s_vibstate_work_check =  ATOMIC_INIT(0);
static uint8_t vibrator_start_first = FALSE;
//static int s_vibstate_work = 0; /*0: not to work, 1: working on*/


#define MOTOR_VOLT_LEVEL	3000 /* 3000mV */
#define LDO_NO_MOTOR 		2

#ifdef CONFIG_BACKLIGHT_EVE
int bd6083_set_ldo_power(int ldo_no, int on);
int bd6083_set_ldo_vout(int ldo_no, unsigned vout_mv);
#endif

int bd6083_ldo_poweron(void)
{
#ifdef CONFIG_BACKLIGHT_EVE
    if (bd6083_set_ldo_power(LDO_NO_MOTOR, 1)) {
         printk("android_vibrator: failed to set motor poweron\n");
         return -1;
    }
#endif
    return 0;
}

int bd6083_ldo_poweroff(void)
{
#ifdef CONFIG_BACKLIGHT_EVE
    if (bd6083_set_ldo_power(LDO_NO_MOTOR, 0)) {
        printk("android_vibrator: failed to set motor poweroff\n");
        return -1;
    }
#endif
    return 0;
}

static int vibrator_motor_power_switch(int on)
{
#ifdef CONFIG_BACKLIGHT_EVE
    if (on) 
	    return bd6083_ldo_poweron();
    else
        return bd6083_ldo_poweroff();
#else
    return 0;
#endif
}

static int vibrator_motor_set_level(int mvolt)
{
#ifdef CONFIG_BACKLIGHT_EVE
    if (bd6083_set_ldo_vout(LDO_NO_MOTOR, mvolt)) {
        printk("android_vibrator: failed to set motor poweron\n");
        return -1;
    }
#endif
    return 0;
}

//static //LGE_CHANGE [antispoon@lge.com,diyu@lge.com] 2009-07-17 comment out "static" for AT+MOT 
void vibrator_disable(void)
{

	gpio_set_value(GPIO_LIN_MOTOR_EN, 0);
	gpio_set_value(GPIO_LIN_MOTOR_PWM, 0);

	vibrator_motor_power_switch(0);

	s_vibstate = 0;
}

//static //LGE_CHANGE [antispoon@lge.com,diyu@lge.com] 2009-07-17 comment out "static" for AT+MOT 
void vibrator_enable(void)
{

	REG_WRITEL((GPMN_M_DEFAULT & GPMN_M_MASK), GP_MN_CLK_MDIV);
	REG_WRITEL((~( GPMN_N_DEFAULT - GPMN_M_DEFAULT )&GPMN_N_MASK), GP_MN_CLK_NDIV);

	gpio_set_value(GPIO_LIN_MOTOR_EN, 1);
	gpio_set_value(GPIO_LIN_MOTOR_PWM, 1);
	
	vibrator_motor_power_switch(1);

	s_vibstate = 1;
}

//static //LGE_CHANGE [antispoon@lge.com,diyu@lge.com] 2009-07-17 comment out "static" for AT+MOT 
void vibrator_set(int amp)
{
	int gain;

	printk(KERN_ERR "android-vibrator: vibrator_set amp=%d\n",amp);

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

//LGE_CHANGE [antispoon@lge.com,diyu@lge.com] 2009-07-17 for AT+MOT
int is_vib_state(void) //antispoon0713 diyu@lge.com
{
	return s_vibstate;
}

static void vibrator_work_func(struct work_struct *work)
{
	vibrator_set(0);
}

static void vibrator_timeout(unsigned long arg __attribute__((unused))) 
{
//	schedule_work(&vibrator_work);
	vibrator_set(0);
	atomic_set(&s_vibstate_work_check ,0);
//	s_vibstate_work =0;
}

static ssize_t vibrator_enable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int timeout = atomic_read(&s_vibrator);
	return sprintf(buf, "%d\n", JIFFIES_TO_MS(timeout));
}

spinlock_t vibrator_lock;

static ssize_t vibrator_enable_store(
		struct device *dev, struct device_attribute *attr, 
		const char *buf, size_t size)
{
	int value;
	sscanf(buf, "%d", &value);

	/* LGE_CHANGE [dojip.kim@lge.com] 2010-05-13, fixed the lockup issue:
	 * not use the spinclock
	 */
	//spin_lock(&vibrator_lock);
	atomic_set(&s_vibrator, value);

	/* 
	 * value == -1 : infinite vibration
	 * value == 0 : stop vibration	 
	 * value > 0 : vibrate during <value> miliseconds
	 */
	 VDBG("%s -1",__FUNCTION__);
	if(atomic_read(&s_vibrator)>0){
		if (atomic_read(&s_vibstate_work_check) ==0){
			del_timer(&s_timer);
			VDBG("%s -2",__FUNCTION__);
			vibrator_set(atomic_read(&s_amp));
			s_timer.expires = jiffies + MS_TO_JIFFIES(atomic_read(&s_vibrator));
//			s_timer.expires = jiffies + MS_TO_JIFFIES(value);
			add_timer(&s_timer);
			atomic_set(&s_vibstate_work_check ,1);
//			s_vibstate_work =1;
		}
	}
	else if ( atomic_read(&s_vibrator) == 0){		
		if (atomic_read(&s_vibstate_work_check) ==1){
			del_timer(&s_timer);
			VDBG("%s -3",__FUNCTION__);
			
			vibrator_set(0);
			atomic_set(&s_vibstate_work_check ,0);
//			s_vibstate_work =0;
		}
	}
	/* LGE_CHANGE [dojip.kim@lge.com] 2010-05-13, fixed the lockup issue:
	 * not use the spinclock
	 */
	//spin_unlock(&vibrator_lock);

	return size;
}
static ssize_t vibrator_amp_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int amp = atomic_read(&s_amp);
	return sprintf(buf, "%d\n", amp);
}

static ssize_t vibrator_amp_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int value;
	VDBG("%s -2",__FUNCTION__);

	sscanf(buf, "%d", &value);
	atomic_set(&s_amp, value);

	return size;
}

static DEVICE_ATTR(enable, S_IRUGO | S_IWUSR, vibrator_enable_show, vibrator_enable_store);
static DEVICE_ATTR(amp, S_IRUGO | S_IWUSR, vibrator_amp_show, vibrator_amp_store);

static int  android_vibrator_suspend(struct platform_device *pdev, pm_message_t state)
{
	/* future capability*/
	//LGE_CHANGE [antispoon@lge.com,diyu@lge.com] 2009-10-24 Add Voltage Setting one more
	vibrator_start_first = FALSE; 
	printk("%s()\n",__FUNCTION__);
	vibrator_disable();
	return 0;
}

static int  android_vibrator_resume(struct platform_device *pdev)
{
	/* future capability*/
	return 0;
}

static int android_vibrator_probe(struct platform_device *pdev)
{
	int err;
	VDBG( "%s -1 Fail\n",__FUNCTION__);

	s_vibrator_gpio = (int)pdev->dev.platform_data;
	err = device_create_file(&pdev->dev, &dev_attr_enable);
	if (err) {
		printk( "android-vibrator: vibrator_probe: Fail\n");
		return err;
	}
	
	VDBG( "%s -2 Fail\n",__FUNCTION__);
	err = device_create_file(&pdev->dev, &dev_attr_amp);
	if (err) {
		printk( "android-vibrator: vibrator_probe: Fail\n");
		device_remove_file(&pdev->dev, &dev_attr_enable);
		return err;
	}
	
	VDBG( "%s -3 Fail\n",__FUNCTION__);
	gpio_request(GPIO_LIN_MOTOR_EN, "gpio_lin_motor_en");
	gpio_request(GPIO_LIN_MOTOR_PWM, "gpio_lin_motor_pwm");

	/* Off Enable */
	gpio_set_value(GPIO_LIN_MOTOR_EN, 0);
	
	VDBG( "%s -4 Fail\n",__FUNCTION__);

	/* Set PWM frequency */
	#if 1
	REG_WRITEL((GPMN_M_DEFAULT & GPMN_M_MASK), GP_MN_CLK_MDIV);
	REG_WRITEL((~( GPMN_N_DEFAULT - GPMN_M_DEFAULT )&GPMN_N_MASK), GP_MN_CLK_NDIV);
	gpio_set_value(GPIO_LIN_MOTOR_PWM, 0);
	#endif //temp
	VDBG( "%s -5 Fail\n",__FUNCTION__);

        /* XXX: this will be done in the charge pump driver
         * because of the kernel panic */
	//vibrator_motor_set_level(MOTOR_VOLT_LEVEL);
	VDBG( "%s -6 Fail\n",__FUNCTION__);

	/* LGE_CHANGE [dojip.kim@lge.com] 2010-05-13, fixed the lockup issue:
	 * not use the spinclock
	 */
	//spin_lock_init(&vibrator_lock);

	printk(KERN_ERR "android-vibrator: vibrator_probe: Done\n");
	
	return 0;
}

static int android_vibrator_remove(struct platform_device *pdev) 
{
	device_remove_file(&pdev->dev, &dev_attr_enable);
	device_remove_file(&pdev->dev, &dev_attr_amp);

	REG_WRITEL((GPMN_M_DEFAULT & GPMN_M_MASK), GP_MN_CLK_MDIV);
	REG_WRITEL((~( GPMN_N_DEFAULT - GPMN_M_DEFAULT )&GPMN_N_MASK), GP_MN_CLK_NDIV);
	REG_WRITEL((GPMN_D_DEFAULT & GPMN_D_MASK), GP_MN_CLK_DUTY);
	vibrator_motor_power_switch(0);
	vibrator_motor_set_level(0);

	return 0;
}

static struct platform_driver android_vibrator_driver = {
	.probe		= android_vibrator_probe,
	.remove		= android_vibrator_remove,
	.suspend	= android_vibrator_suspend,
	.resume		= android_vibrator_resume,
	.driver		= {
		.name		= "android-vibrator",
		.owner		= THIS_MODULE,
	},
};

static int __init android_vibrator_init(void)
{
	printk( "android-vibrator: init\n");
	return platform_driver_register(&android_vibrator_driver);
}

static void __exit android_vibrator_exit(void)
{
	printk( "android-vibrator: exit\n");
 	platform_driver_unregister(&android_vibrator_driver);
}

module_init(android_vibrator_init);
module_exit(android_vibrator_exit);

MODULE_AUTHOR("Dae il, yu <diyu@lge.com>");
MODULE_DESCRIPTION("Eve vibrator driver for Android");
MODULE_LICENSE("GPL");
