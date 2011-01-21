/* drivers/misc/hall_bu52031.c
    reference : vibrator_eve.c
 *
 * (C) 2008 LGE, Inc.
 * (C) 2007 Google, Inc.
 *
 * Android Hall ic driver 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#include <linux/module.h>
#include <linux/input.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/timer.h>
#include <mach/gpio.h>
#include <asm/io.h>
#include <mach/msm_iomap.h>
#include <linux/at_kpd_eve.h> //LGE_CHANGE [antispoon@lge.com,diyu@lge.com] 2009-07-17 for AT+GKPD
#include <linux/jiffies.h>

#define GPIO_HALL_IC_IRQ  	18
#define SLIDE_DEBUG 		0

#if SLIDE_DEBUG
#define SDBG(fmt, args...) printk(fmt, ##args)
#else
#define SDBG(fmt, args...) do {} while (0)
#endif /* SLIDE_DEBUG */

static int s_hall_ic_gpio; 			//HALL_IC_IRQ
static int hall_ic_enable;
static int hall_ic_on = -1; 
static int last_state;
static int hall_ic_timer = 2500; 	// default 1 sec

extern void lg_slideon_event_func(void);	//LGE_CHANGE [bluerti@lge.com] 2009-10-16
struct hall_ic_device {
	struct input_dev *input_dev;
	struct timer_list timer;
	spinlock_t	lock;
};

static struct hall_ic_device *_dev = NULL;

enum {
	SLIDE_CLOSE = 0,
	SLIDE_OPEN,
};

int is_slide_open(void)
{
	SDBG("\n%s \n", __FUNCTION__);
	return hall_ic_on;
}

void set_slide_open(int value)
{
	struct hall_ic_device *dev = _dev;
	hall_ic_on = value;
	disable_irq(s_hall_ic_gpio);

	if(	hall_ic_on == 0){
		//LGE_CHANGE [antispoon@lge.com,diyu@lge.com] 2009-07-17 steal folder open ASCII(73) for AT+GKPD
		write_gkpd_value (73); 
		input_report_switch(dev->input_dev, SW_LID, 0);
		input_sync(dev->input_dev);
		SDBG("Hall-ic - -> Open \n");
	}else if(hall_ic_on == 1){
		//LGE_CHANGE [antispoon@lge.com,diyu@lge.com] 2009-07-17 steal folder closed ASCII(74) for AT+GKPD
		write_gkpd_value (74);  
		input_report_switch(dev->input_dev, SW_LID, 1);
		input_sync(dev->input_dev);
		SDBG("Hall-ic - -> Close \n");
	}

	enable_irq(s_hall_ic_gpio);
}

EXPORT_SYMBOL(is_slide_open);
EXPORT_SYMBOL(set_slide_open);

static void hall_ic_report_event(int state) 
{
	if (state == SLIDE_OPEN) {
		hall_ic_on = 0;
		SDBG("Hall-ic -> Open \n");
	} else {
		hall_ic_on = 1;
		SDBG("Hall-ic -> Close \n");
	}

	input_report_switch(_dev->input_dev, SW_LID, hall_ic_on);
	input_sync(_dev->input_dev);

	return;
}

static void hall_timer(unsigned long arg)
{
	struct hall_ic_device *dev = (struct hall_ic_device *)_dev;
	int current_state;

	spin_lock(&dev->lock);
	if (hall_ic_enable)
		goto timer_end;

	current_state = gpio_get_value(GPIO_HALL_IC_IRQ);
	if (current_state == last_state) {
		hall_ic_enable = 1;
	} else {
		hall_ic_report_event(current_state);

		last_state = current_state;
		mod_timer(&dev->timer, jiffies + (hall_ic_timer * HZ / 1000));
	}

timer_end:
	spin_unlock(&dev->lock);

	return;
}

static int hall_ic_irq_handler(int irq, void *dev_id)
{
	struct hall_ic_device *dev = dev_id;
	
	SDBG("\n\nCheck point : %s\n", __FUNCTION__);
	//LGE_CHANGE_S [bluerti@lge.com] 2009-10-16,
	if (gpio_get_value(GPIO_HALL_IC_IRQ) == SLIDE_OPEN)
		lg_slideon_event_func();
	//LGE_CHANGE_E [bluerti@lge.com] 2009-10-16
	
	spin_lock(&dev->lock);
	if (!hall_ic_enable)
		goto irq_handler_end;

	//disable_irq(s_hall_ic_gpio);
	disable_irq_nosync(s_hall_ic_gpio); //L&T: Fix synchronize_irq deadlock
	last_state = gpio_get_value(GPIO_HALL_IC_IRQ);
	hall_ic_report_event(last_state);

	hall_ic_enable = 0;
	mod_timer(&dev->timer, jiffies + (hall_ic_timer * HZ / 1000));
	enable_irq(s_hall_ic_gpio);

irq_handler_end:
	spin_unlock(&dev->lock);

	return IRQ_HANDLED;
}

static ssize_t hall_ic_enable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	if(gpio_get_value(GPIO_HALL_IC_IRQ)==1){ //OPEN
		hall_ic_on = 0;
	} else { //CLOSE
		hall_ic_on = 1;
	}
	return sprintf(buf, "%d\n", hall_ic_on );
}

static ssize_t hall_ic_enable_store(
		struct device *dev, struct device_attribute *attr, 
		const char *buf, size_t size)
{
	int value;
	int current_state;

	sscanf(buf, "%d", &value);

	/* value == 0  : stop hall ic	 
	 * value == 1  : start hall ic
	 */
	value = !!value;
	spin_lock(&_dev->lock);
	if (value == hall_ic_enable)
		goto enable_store_end;
	
	hall_ic_enable = value;
	current_state = gpio_get_value(GPIO_HALL_IC_IRQ);
	
	if (!hall_ic_enable) {
		last_state = current_state;
		SDBG("\nandroid-hall-ic: hall_ic disbled\n");
	} else {
		if (current_state != last_state) {
			hall_ic_report_event(current_state);
			last_state = current_state;
			SDBG("\nandroid-hall-ic: hall_ic enabled\n");
		}
	}
		
enable_store_end:
	spin_unlock(&_dev->lock);

	return size;
}

static ssize_t hall_ic_show_timer(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", hall_ic_timer);
}

static ssize_t hall_ic_set_timer(struct device *dev, struct device_attribute *attr,	
								 const char *buf, size_t size)
{
	int value;

	sscanf(buf, "%d", &value);

	hall_ic_timer = value;

	return size;
}

static DEVICE_ATTR(enable, S_IRUGO | S_IWUSR, hall_ic_enable_show, hall_ic_enable_store);
static DEVICE_ATTR(timer, S_IRUGO | S_IWUSR, hall_ic_show_timer , hall_ic_set_timer);

static int android_hall_ic_probe(struct platform_device *pdev)
{
	struct hall_ic_device *dev;
	struct input_dev *input_dev;
	int ret, err;

	dev = kzalloc(sizeof(struct hall_ic_device), GFP_KERNEL);
	input_dev = input_allocate_device();
	if (!dev|| !input_dev) {
		ret = -ENOMEM;
		printk("diyu input_allocate_device failed\n");
		goto err_input_register_device;
	}
	
	platform_set_drvdata(pdev, dev);
	_dev = dev; /* for miscdevice */
	
	input_dev->name = "Slide Hall-ic";
	input_dev->phys = "hallic/input0";
	input_dev->dev.parent = &pdev->dev;
	input_dev->evbit[0] =  BIT_MASK(EV_SW);
	
	set_bit(SW_LID, input_dev->swbit);

	ret = input_register_device(input_dev);
	if (ret) {
		printk("diyu input_allocate_device failed\n");
		goto err_input_register_device;
	}
	dev->input_dev = input_dev;
	spin_lock_init(&dev->lock);
		
	setup_timer(&dev->timer, hall_timer, (unsigned long)dev);
	
	// hall_ic_on 
	hall_ic_enable = 1; 
	s_hall_ic_gpio = gpio_to_irq(GPIO_HALL_IC_IRQ);//Do Not need. To assign at board_eve_gpio_i2c.c

	/* hall_ic_on = 1 (open) :  hall_ic_on = 0 (close) :  
	 * HIGH : Open  //LOW : Close
	 */
	ret = request_irq(s_hall_ic_gpio, hall_ic_irq_handler,
			IRQF_TRIGGER_RISING|IRQF_TRIGGER_FALLING,
			"hall-ic", dev); 
	
	if (ret) {
		printk("\nHALL IC IRQ Check-fail\n pdev->client->irq %d\n",s_hall_ic_gpio);
		goto err_request_irq;
	}

	err = set_irq_wake(s_hall_ic_gpio, 1);
	if (err) 
		pr_err("hall-ic: set_irq_wake failed for gpio %d, "
				"irq %d\n", GPIO_HALL_IC_IRQ, s_hall_ic_gpio);
	
	ret = device_create_file(&pdev->dev, &dev_attr_enable);
	if (ret) {
		printk( "android-hall-ic: hall-ic_probe: Fail\n");
		device_remove_file(&pdev->dev, &dev_attr_enable);
		goto err_request_irq;
	}
	
	ret = device_create_file(&pdev->dev, &dev_attr_timer);
	if (ret) {
		printk( "android-hall-ic: hall-ic_probe: Fail\n");
		device_remove_file(&pdev->dev, &dev_attr_timer);
		goto err_request_irq;
	}
	
	last_state = gpio_get_value(GPIO_HALL_IC_IRQ);
	if (last_state == SLIDE_OPEN) {
		hall_ic_on = 0;
		input_report_switch(dev->input_dev, SW_LID, 0);
		input_sync(dev->input_dev);
		SDBG("Hall-ic - -> Open \n");
	} else {
		hall_ic_on = 1;
		input_report_switch(dev->input_dev, SW_LID, 1);
		input_sync(dev->input_dev);
		SDBG("Hall-ic - -> Close \n");
	}
	
	printk(KERN_ERR "android-hall_ic: hall_ic_probe: Done\n");
	return 0;

err_request_irq:
	input_unregister_device(input_dev);
err_input_register_device:
	input_free_device(input_dev);
	kfree(dev);
	return ret;

	
}

static int android_hall_ic_remove(struct platform_device *pdev) 
{
	struct hall_ic_device *dev = (struct hall_ic_device *)_dev;

	device_remove_file(&pdev->dev, &dev_attr_enable);
	device_remove_file(&pdev->dev, &dev_attr_timer);
	del_timer_sync(&dev->timer);
	
	return 0;
}

static struct platform_driver android_hall_ic_driver = {
	.probe		= android_hall_ic_probe,
	.remove		= android_hall_ic_remove,
	.driver		= {
		.name		= "hall-ic",
		.owner		= THIS_MODULE,
	},
};

static int __init android_hall_ic_init(void)
{
	SDBG( "android_hall_ic: init\n");

	return platform_driver_register(&android_hall_ic_driver);
}

static void __exit android_hall_ic_exit(void)
{
	SDBG( "android_hall_ic: exit\n");
 
	platform_driver_unregister(&android_hall_ic_driver);
}

module_init(android_hall_ic_init);
module_exit(android_hall_ic_exit);

MODULE_AUTHOR("Dae il, yu <diyu@lge.com>");
MODULE_DESCRIPTION("EVE hall ic driver for Android");
MODULE_LICENSE("GPL");
