#include <linux/module.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/timer.h>
#include <linux/workqueue.h>

#include <linux/platform_device.h>

#include <mach/msm_rpcrouter.h>
#include <mach/vreg.h>
#include <linux/err.h>

#define DEBUG_INFO
#define DEBUG_FUNCTION

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

/* LGE_CHANGE_S [jinwoonam@lge.com] 2009.03.09 Add for rpc */
typedef enum
{
  PM_MPP__I_SINK__LEVEL_5mA,
  PM_MPP__I_SINK__LEVEL_10mA,
  PM_MPP__I_SINK__LEVEL_15mA,
  PM_MPP__I_SINK__LEVEL_20mA,
  PM_MPP__I_SINK__LEVEL_25mA,
  PM_MPP__I_SINK__LEVEL_30mA,
  PM_MPP__I_SINK__LEVEL_35mA,
  PM_MPP__I_SINK__LEVEL_40mA,
  PM_MPP__I_SINK__LEVEL_INVALID
}pm_mpp_i_sink_level_type;

#define PM_LIBPROG		0x30000061
#define PM_LIBVERS		0x00010001 /* 0x00010001 */

#define ONCRPC_PM_SECURE_MPP_CONFIG_I_SINK_PROC 6 
#define ONCRPC_PM_SET_LED_INTENSITY_PROC 16 

#define KEYLED_DEBUG 0
#if KEYLED_DEBUG
#define KDBG(fmt, args...) printk(fmt, ##args)
#else
#define KDBG(fmt, args...) do {} while (0)
#endif /* KEYLED_DEBUG */


#define JIFFIES_TO_MS(t) ((t) * 1000 / HZ)
#define MS_TO_JIFFIES(j) ((j * HZ) / 1000)

#define MAX_TIMEOUT_MS   (15000)

static int s_keyled;
static int s_touchled;
static int ledtimeout_value;

typedef enum
{
  PM_TOUCH_LED,
  PM_KBD_LED,
  PM_INVALID
}pm_led_intesity_type;

#define PM_MPP_7	6

static int pm_set_shift_led_intensity(int onoff)
{
	int rc = 0;
	static struct msm_rpc_endpoint *set_led_intensity_ep;
	static atomic_t pm_shift_led_intensity = ATOMIC_INIT(0);
	struct set_led_intensity_req {
		struct rpc_request_hdr hdr;
		uint32_t mpp;
		uint32_t level;
		uint32_t onoff;
	} req;

	if (atomic_inc_return(&pm_shift_led_intensity) != 1) {
		atomic_dec(&pm_shift_led_intensity);
		return -EBUSY;
	}

	set_led_intensity_ep = msm_rpc_connect(PM_LIBPROG, PM_LIBVERS, 0);
	if (IS_ERR(set_led_intensity_ep)) {
		printk(KERN_ERR "%s: msm_rpc_connect failed! rc = %ld\n",__func__, PTR_ERR(set_led_intensity_ep));
		return -EINVAL;
	}

	req.mpp = cpu_to_be32((int)PM_MPP_7);
	req.level = cpu_to_be32((int)PM_MPP__I_SINK__LEVEL_30mA);

	if (onoff)
		req.onoff = cpu_to_be32((int)1);
	else
		req.onoff = cpu_to_be32((int)0);

	rc = msm_rpc_call(set_led_intensity_ep,
						ONCRPC_PM_SECURE_MPP_CONFIG_I_SINK_PROC,
						&req, sizeof(req),
						5 * HZ);
	
	if (rc)
		printk(KERN_ERR"%s: msm_rpc_call failed! rc = %d\n", __func__, rc);

	if(set_led_intensity_ep)
		msm_rpc_close(set_led_intensity_ep);

	atomic_dec(&pm_shift_led_intensity);

	return rc;
}

static int pm_set_led_intensity(pm_led_intesity_type led_type, int value)
{
	int rc = 0;
	static struct msm_rpc_endpoint *set_led_intensity_ep;
	static atomic_t pm_led_intensity = ATOMIC_INIT(0);
	struct set_led_intensity_req {
		struct rpc_request_hdr hdr;
		uint32_t type;
		uint32_t val;
	} req;

	D_FUNC("%s() tyep : %d, value : %d\n", __FUNCTION__, led_type, value);

	if (atomic_inc_return(&pm_led_intensity) != 1) {
		atomic_dec(&pm_led_intensity);
		return -EBUSY;
	}

	set_led_intensity_ep = msm_rpc_connect(PM_LIBPROG, PM_LIBVERS, 0);
	if (IS_ERR(set_led_intensity_ep)) {
		printk(KERN_ERR "%s: msm_rpc_connect failed! rc = %ld\n",__func__, PTR_ERR(set_led_intensity_ep));
		return -EINVAL;
	}

	req.type = cpu_to_be32((int)led_type);
	req.val = cpu_to_be32(value);

	rc = msm_rpc_call(set_led_intensity_ep,
						ONCRPC_PM_SET_LED_INTENSITY_PROC,
						&req, sizeof(req),
						5 * HZ);
	
	if (rc)
		printk(KERN_ERR"%s: msm_rpc_call failed! rc = %d\n", __func__, rc);

	if(set_led_intensity_ep)
		msm_rpc_close(set_led_intensity_ep);

	atomic_dec(&pm_led_intensity);

	return rc;
}

static ssize_t keyled_enable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n",s_keyled );
}

static ssize_t keyled_enable_store(
		struct device *dev, struct device_attribute *attr, 
		const char *buf, size_t size)
{
	int value;
	int rc;

	sscanf(buf, "%d", &value);
	s_keyled = !!value;

	D_FUNC("%s(%d) \n.", __func__, value);
	/* ================ key-led =============== 
	 *  PM_LED_KBD_SETTING__LEVEL
      Value Current Units
        0   0       mA
        1   -10     mA
        2   -20     mA
        3   -30     mA
        4   -40     mA
        5   -50     mA
        6   -60     mA
        7   -70     mA
        8   -80     mA
        9   -90     mA
        10  -100    mA
        11  -110    mA
        12  -120    mA
        13  -130    mA
        14  -140    mA
        15  -150    mA
	 * =====================================*/

	if (s_keyled)
	{
		rc = pm_set_led_intensity(PM_KBD_LED, 14); // Set -140mA
	}
	else
	{
		rc = pm_set_led_intensity(PM_KBD_LED, s_keyled);
	}
	return size;
}

static ssize_t touchled_enable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	KDBG("diyu : %s \n ",__FUNCTION__);
	return sprintf(buf, "%d\n",s_touchled );
}

static ssize_t touchled_enable_store(
		struct device *dev, struct device_attribute *attr, 
		const char *buf, size_t size)
{
	int value;
	int rc;

	sscanf(buf, "%d", &value);
	s_touchled = !!value;

	D_FUNC("%s(%d) \n.", __func__, value);
	/* ========== Touch slide key =========
	 *  PM_LED_KBD_SETTING__LEVEL
      Value Current Units
        0   0       mA
        1   -10     mA
        2   -20     mA
        3   -30     mA
        4   -40     mA
        5   -50     mA
        6   -60     mA
        7   -70     mA
        8   -80     mA
        9   -90     mA
        10  -100    mA
        11  -110    mA
        12  -120    mA
        13  -130    mA
        14  -140    mA
        15  -150    mA
	 * =====================================*/

	if (s_touchled)
	{
		rc = pm_set_led_intensity(PM_TOUCH_LED, 1); //LCD_DRV_N connected to SLIDE_KPD_DRV_N
	}
	else
	{
		rc = pm_set_led_intensity(PM_TOUCH_LED, 0); //LCD_DRV_N connected to SLIDE_KPD_DRV_N
	}
	return size;
}

static ssize_t shiftled_enable_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int ret, onoff;
	sscanf(buf, "%d", &onoff);

	ret = pm_set_shift_led_intensity(onoff);

	return ret;
}

static DEVICE_ATTR(enable, 0666, keyled_enable_show, keyled_enable_store);
static DEVICE_ATTR(setting, 0666, touchled_enable_show, touchled_enable_store);
static DEVICE_ATTR(shift, 0666, NULL, shiftled_enable_store);

static int android_keyled_probe(struct platform_device *pdev)
{
	int err;
	KDBG( "android_keyled_probe \n");

	s_keyled = 0;
	s_touchled = 0;

	ledtimeout_value = 2000;

	err = device_create_file(&pdev->dev, &dev_attr_enable);
	if (err) {
		printk( "%s: keyled_probe: Failed: %d\n", __func__, err);
		return err;
	}

	err = device_create_file(&pdev->dev, &dev_attr_setting);
	if (err) {
		printk( "%s: keyled_probe: Failed: %d\n", __func__, err);
		return err;
	}

	err = device_create_file(&pdev->dev, &dev_attr_shift);
	if(err) {
		printk("%s() : shift key LED probed : Failed: %d\n", __func__, err);
		return err;
	}

	return 0;
}

static int android_keyled_remove(struct platform_device *pdev) 
{
	device_remove_file(&pdev->dev, &dev_attr_enable);
	device_remove_file(&pdev->dev, &dev_attr_setting);
	device_remove_file(&pdev->dev, &dev_attr_shift);

	return 0;
}


static struct platform_driver eve_keyled_driver = {
	.probe		= android_keyled_probe,
	.remove		= android_keyled_remove,
	.driver		= {
		.name		= "android-keyled",
		.owner		= THIS_MODULE,
	},
};

/* backlight of keyboard, home&back, shift */
static struct platform_device eve_keyled_device = {
    .name   = "android-keyled",
    .id = -1,
    .dev = {
        .platform_data = 0,
    },
};

static int __init eve_keyled_init(void)
{
	int ret;
	
	ret = platform_driver_register(&eve_keyled_driver);
	platform_device_register(&eve_keyled_device);

	return ret;
}

static void __exit eve_keyled_exit(void)
{
	platform_device_unregister(&eve_keyled_device);
	platform_driver_unregister(&eve_keyled_driver);
}

module_init(eve_keyled_init);
module_exit(eve_keyled_exit);

MODULE_AUTHOR("Dae il, yu <diyu@lge.com>");
MODULE_DESCRIPTION("EVE keyled driver for Android (KEY-LED / TOUCH-LED )");
MODULE_LICENSE("GPL");
