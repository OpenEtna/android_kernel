/* -*- mode: c; c-basic-offset: 8; -*-
 * vim: noexpandtab sw=8 ts=8 sts=0:
 */
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <mach/gpio.h>

#undef DEBUG
#define DEBUG 1
#if DEBUG
#define DBG(fmt, args...) printk(fmt,##args)
#else
#define DBG(fmt, args...) do {} while(0)
#endif

#define FEATURE_DEVICE_FILE_INTERFACE 0
#define FEATURE_MOVIE_MODE_STROBE 0
#define FEATURE_STROBE_PREFLASH 1

#define GPIO_FLASH_LED_EN	78
#define GPIO_FLASH_LED_STROBE	79
#define GPIO_FLASH_LED_INH	81

#if FEATURE_DEVICE_FILE_INTERFACE
unsigned int movie_mode_on_status=0;
#endif

static struct timer_list strobe_timer;

static int eve_flash_led_configure_pins(void)
{
	int ret=0;

	ret = gpio_request(GPIO_FLASH_LED_EN, "eve_flash_led_en_set");
	if(ret != 0)
	{
		printk("eve_flash_led_en_set gpio_request failed\n");
		return -1;
	}

	gpio_configure(GPIO_FLASH_LED_EN, GPIOF_DRIVE_OUTPUT);
	gpio_set_value(GPIO_FLASH_LED_EN, 0);

	ret = gpio_request(GPIO_FLASH_LED_STROBE, "eve_flash_led_flen");
	if(ret != 0)
	{
		printk("eve_flash_led_flen gpio_request failed\n");
		return -1;
	}

	gpio_configure(GPIO_FLASH_LED_STROBE, GPIOF_DRIVE_OUTPUT);
	gpio_set_value(GPIO_FLASH_LED_STROBE, 0);

	ret = gpio_request(GPIO_FLASH_LED_INH, "eve_flash_led_flinh");
	if(ret != 0)
	{
		printk("eve_flash_led_flinh gpio_request failed\n");
		return -1;
	}

	gpio_configure(GPIO_FLASH_LED_INH, GPIOF_DRIVE_OUTPUT);
	gpio_set_value(GPIO_FLASH_LED_INH, 0);

	return 0;
}

/*
 * reduced level: the movie mode current at a reduced level
 * 1: 100 %
 * 2: 89 %
 * 3: 79 %
 * 4: 71 %
 * 5: 63 %
 * 6: 56 %
 * 7: 50 %
 * 8: 44.7 %
 * 9: 39.8 %
 * 10: 35.5 %
 * 11: 31.6 %
 * 12: 28.2 %
 * 13: 25.1 %
 * 14: 22.4 %
 * 15: 20 %
 * 16: 0 %
 *
 * -> We use,
 *  - 1 as a real movie mode level
 *  - 1 as a pre-flash mode level during auto focus
 */
#define EVE_FLASH_MOVIE_CURRENT 1
#define EVE_FLASH_PRE_SNAPSHOT_CURRENT 1
static void eve_flash_led_movie_mode_on(unsigned int reduced_level)
{
	unsigned int i;

	if (16 < reduced_level) 
		reduced_level = 16;

	for(i = 0; i < reduced_level; i++)
	{
		gpio_set_value(GPIO_FLASH_LED_EN, 0);
		udelay(1);

		gpio_set_value(GPIO_FLASH_LED_EN, 1);
		ndelay(75);
	}

	udelay(525);
}

static void eve_flash_led_movie_mode_off(void)
{
	gpio_set_value(GPIO_FLASH_LED_EN, 0);
	udelay(525);
}

static void eve_flash_led_strobe_on(void)
{
	gpio_set_value(GPIO_FLASH_LED_STROBE, 1);
}

static void eve_flash_led_strobe_off(void)
{
	gpio_set_value(GPIO_FLASH_LED_STROBE, 0);
}

static void eve_flash_led_set_inhibition(void)
{
	gpio_set_value(GPIO_FLASH_LED_INH, 1);
}

static void eve_flash_led_unset_inhibition(void)
{
	gpio_set_value(GPIO_FLASH_LED_INH, 0);
}

#if FEATURE_DEVICE_FILE_INTERFACE
static ssize_t eve_flash_led_movie_mode_on_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "eve_flash_led_movie_mode_on status: %d\n",
			movie_mode_on_status);
}

static ssize_t eve_flash_led_movie_mode_on_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int cmd;
	sscanf(buf, "%d", &cmd);

	DBG("%s: %d\n", __FUNCTION__, cmd);

	if(cmd == 1)
	{
		DBG("eve_flash_led_movie_mode_on\n");
		eve_flash_led_movie_mode_on(EVE_FLASH_MOVIE_CURRENT);
		movie_mode_on_status=1;
	}
	else
	{
		DBG("eve_flash_led_movie_mode_off\n");
		eve_flash_led_movie_mode_off();
		movie_mode_on_status=0;
	}

	return size;
}

static DEVICE_ATTR(movie_mode_on, S_IRUGO | S_IWUGO, eve_flash_led_movie_mode_on_show, eve_flash_led_movie_mode_on_store);
#endif

static void eve_flash_led_strobe_timer(unsigned long arg)
{
	DBG("********* eve_flash_led: turn off the strobe ************\n");
#if FEATURE_MOVIE_MODE_STROBE
	eve_flash_led_movie_mode_off();
#else
	eve_flash_led_strobe_off();
#endif
}


int eve_flash_set_led_state(int led_state)
{
	DBG("eve_flash_led: new state, %d\n", led_state);

	switch (led_state) {
		case 0: // Snapshot flash on
			eve_flash_led_strobe_off();
			eve_flash_led_movie_mode_off();
#if FEATURE_MOVIE_MODE_STROBE
			eve_flash_led_movie_mode_on(EVE_FLASH_MOVIE_CURRENT);
#else
			eve_flash_led_strobe_on();
#endif
			// msecs_to_jiffies
			mod_timer(&strobe_timer, jiffies + HZ * 2);
			break;
		case 1: // Snapshot flash off
			eve_flash_led_strobe_off();
			break;
		case 2: // Movie flash on
			eve_flash_led_strobe_off();
			eve_flash_led_movie_mode_off();
			eve_flash_led_movie_mode_on(EVE_FLASH_MOVIE_CURRENT);
			break;
		case 3: // Movie flash off
			eve_flash_led_movie_mode_off();
			break;
		case 4:
			eve_flash_led_set_inhibition();
			break;
		case 5:
			eve_flash_led_unset_inhibition();
			break;
		case 7: /* Snapshot Pre-flash */
			eve_flash_led_strobe_off();
			eve_flash_led_movie_mode_off();
#if FEATURE_STROBE_PREFLASH
			eve_flash_led_strobe_on();
			mdelay(800);
			eve_flash_led_strobe_off();
			eve_flash_led_movie_mode_on(EVE_FLASH_PRE_SNAPSHOT_CURRENT);
#else
			eve_flash_led_movie_mode_on(EVE_FLASH_PRE_SNAPSHOT_CURRENT);
			mdelay(800);
#endif /* FEATURE_STROBE_PREFLASH */
			break;
		default:
			printk("eve_flash_led: wrong led state\n");
			return -EINVAL;
	}
	
	return 0;
}
EXPORT_SYMBOL(eve_flash_set_led_state);

static int eve_flash_led_probe(struct platform_device *pdev)
{
	int err;

#if FEATURE_DEVICE_FILE_INTERFACE
	err = device_create_file(&pdev->dev, &dev_attr_movie_mode_on);
	if(err)
	{
		printk("eve_flash_led: failed to create device file\n");
		return err;
	}
#endif

	err = eve_flash_led_configure_pins();
	if(err)
	{
		printk("eve_flash_led: failed to configure pins\n");
		return err;
	}

	setup_timer(&strobe_timer, eve_flash_led_strobe_timer, (unsigned long)NULL);

	return 0;
}

static int eve_flash_led_remove(struct platform_device *pdev)
{
#if FEATURE_DEVICE_FILE_INTERFACE
	device_remove_file(&pdev->dev, &dev_attr_movie_mode_on);
#endif
	del_timer_sync(&strobe_timer);

	gpio_free(GPIO_FLASH_LED_EN);
	gpio_free(GPIO_FLASH_LED_STROBE);
	gpio_free(GPIO_FLASH_LED_INH);

	return 0;
}

static struct platform_driver eve_flash_led_driver =
{
	.probe = eve_flash_led_probe,
	.remove = eve_flash_led_remove,
	.driver =
	{
		.name = "camera_flash_led",
		.owner = THIS_MODULE,
	}
};


static int __init eve_flash_led_init(void)
{
	return platform_driver_register(&eve_flash_led_driver);
}

static void __exit eve_flash_led_exit(void)
{
	platform_driver_unregister(&eve_flash_led_driver);
}

module_init(eve_flash_led_init);
module_exit(eve_flash_led_exit);
