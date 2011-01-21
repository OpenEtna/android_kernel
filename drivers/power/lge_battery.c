/*
 * Driver for batteries of lge.
 *
 */

#include <linux/module.h>
#include <linux/param.h>
#include <linux/jiffies.h>
#include <linux/workqueue.h>
#include <linux/pm.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/miscdevice.h>

#include <mach/charger.h>
/* LGE_CHANGE [dojip.kim@lge.com] 2010-03-19 */
//#include <mach/rpc_hsusb.h>
#include <mach/rpc_charger.h>
#include <mach/lprintk.h>

// Select Test Mode interface
#define SYSFS_LGE_BATT_TEST_MODE
#define DEBUGFS_LGE_BATT_TEST_MODE

// Select RPC method
//#define LGE_BATT_MTOA_RPC_MODE
#define LGE_BATT_ATOM_RPC_MODE

/* LGE_CHANGE_S [ybw75@lge.com] 2009-01-21, Add Charger MtoA (modem to app) RPC routine */
#include <mach/msm_rpcrouter.h>
/* LGE_CHANGE_E [ybw75@lge.com] 2009-01-21 */

/* LGE_CHANGE_S [ybw75@lge.com] 2009-02-04, Add Debugfs feature for test mode */
#include <linux/debugfs.h>
/* LGE_CHANGE_E [ybw75@lge.com] 2009-02-04  */

#define LGE_BATT_DEFAULT_VOLTAGE_MV		3800		// mV
#define LGE_BATT_DEFAULT_CAPACITY		50		// %
#define LGE_BATT_DEFAULT_TEMPERATURE		300		// 0.1 Celsius degree

//#define DEBUG_INFO
//#define DEBUG_FUNCTION

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

enum charger_type {
	CHG_HOST_PC,
	CHG_WALL = 2,
	CHG_UNDEFINED,
};
//extern int msm_chg_lge_get_sb_info(smart_battery_info_type* sb_info);

/* LGE_CHANGE_S [ybw75@lge.com] 2009-02-20, Modify battery infomation structure */
//extern int msm_chg_lge_batt_info(lge_battery_info_parameter sb_info);
/* LGE_CHANGE_E [ybw75@lge.com] 2009-02-20 */

/* LGE_CHANGE_S [ybw75@lge.com] 2009-03-02, checking if MtoA is initialized */
#ifdef FEATURE_MTOA_CHARGER_RPC
extern int msm_chg_mtoa_init_ok( int mtoa_init_ok);
#endif
/* LGE_CHANGE_E [ybw75@lge.com] 2009-03-02, checking if MtoA is initialized */

extern void msm_chg_pm_test( int test_order);

/* LGE_CHANGE_S [ybw75@lge.com] 2009-03-06, Workaround of detecting AC or USB  */
#ifdef CONFIG_USB_FUNCTION_MSM_HSUSB
extern int get_charger_type( void );
#endif
/* LGE_CHANGE_E [ybw75@lge.com] 2009-03-06  */

enum {
	BEFORE_INITIALIZATION = 0,
	AFTER_INITIALIZATION = 1,
};

static int lge_battery_get_property(struct power_supply *psy, enum power_supply_property psp, union power_supply_propval *val);
static void lge_battery_external_power_changed(struct power_supply *psy);
static int lge_power_get_property(struct power_supply *psy, enum power_supply_property psp, union power_supply_propval *val);

static lge_battery_info_type new_batt_info, batt_info;

//static smart_battery_info_type sbat_info;

int work_queue_init_ok=BEFORE_INITIALIZATION;

struct lge_battery_device_info {
	struct device *dev;

	unsigned long update_time;	/* jiffies when data read */
	struct workqueue_struct *monitor_wqueue;
	struct delayed_work monitor_work;
};

static struct lge_battery_device_info lge_batt;

enum lge_power_type {
	BATTERY=0,
	USB = 1,
	AC = 2,
};
static char *supply_list[] = {
	"battery",
};
static enum power_supply_property lge_battery_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_PRESENT,
};

static enum power_supply_property lge_power_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
};
static struct power_supply lge_power_supplies[] = {
	{	/* battery */
		.name = "battery",
		.type	   = POWER_SUPPLY_TYPE_BATTERY,
		.properties	   = lge_battery_props,
		.num_properties = ARRAY_SIZE(lge_battery_props),
		.get_property   = lge_battery_get_property,
		.external_power_changed = lge_battery_external_power_changed,
	},
	{	/* USB charger */
		.name	   = "usb",
		.type	   = POWER_SUPPLY_TYPE_USB,
		.properties	   = lge_power_props,
		.num_properties = ARRAY_SIZE(lge_power_props),
		.supplied_to = supply_list,
		.num_supplicants = ARRAY_SIZE(supply_list),
		.get_property   = lge_power_get_property,
		.external_power_changed = NULL,
	},
	{	/* AC charger */
		.name    = "ac",
		.type    = POWER_SUPPLY_TYPE_MAINS,
		.properties	  = lge_power_props,
		.num_properties = ARRAY_SIZE(lge_power_props),
		.supplied_to = supply_list,
		.num_supplicants = ARRAY_SIZE(supply_list),
		.get_property   = lge_power_get_property,
		.external_power_changed = NULL,
	},
};

#if defined(DEBUGFS_LGE_BATT_TEST_MODE) || defined(SYSFS_LGE_BATT_TEST_MODE)
enum {
	BATT_TEST_MODE = 0,
	BATT_TEST_CAPACITY,
	BATT_TEST_VOLT,
	BATT_TEST_TEMP,
	BATT_TEST_PM,
};
enum {
	LGE_BATT_TEST_MODE_OFF = 0,
	LGE_BATT_TEST_MODE_ON = 1,
};
int	lge_battery_test_mode=LGE_BATT_TEST_MODE_OFF;
#endif

struct lge_battery_device_info *pg_lge_batt_info;

#if defined(SYSFS_LGE_BATT_TEST_MODE)
static int lge_battery_test_create_attrs(struct device * dev);
static ssize_t lge_battery_test_show_property(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t lge_battery_test_store_property(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
#endif
#if defined(DEBUGFS_LGE_BATT_TEST_MODE)
void lge_battery_debug_init(void);
#endif

static unsigned int cache_time = 1000;
module_param(cache_time, uint, 0644);
MODULE_PARM_DESC(cache_time, "cache time in milliseconds");


int lge_battery_info_changed(void)
{
	if ( new_batt_info.batt_remaining 		!= batt_info.batt_remaining
		|| new_batt_info.batt_voltage	!= batt_info.batt_voltage
		|| new_batt_info.batt_temperature!= batt_info.batt_temperature
		|| new_batt_info.chg_status		!= batt_info.chg_status
		|| new_batt_info.charger_type	!= batt_info.charger_type
		|| new_batt_info.present		!= batt_info.present)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

void lge_battery_info_update(void)
{
	batt_info.batt_remaining 	= new_batt_info.batt_remaining;
	batt_info.batt_voltage		= new_batt_info.batt_voltage;
	batt_info.batt_temperature	= new_batt_info.batt_temperature;
	batt_info.chg_status		= new_batt_info.chg_status;
	batt_info.charger_type		= new_batt_info.charger_type;
	batt_info.present			= new_batt_info.present;
}

static int lge_battery_read(void)
{
	int rc=0;

	D_FUNC("%s()\n", __func__ );

	// If TestMode is ON, Don't update battery info.
#if defined(DEBUGFS_LGE_BATT_TEST_MODE) || defined(SYSFS_LGE_BATT_TEST_MODE)
	if (LGE_BATT_TEST_MODE_ON == lge_battery_test_mode )
	{
		return 0;
	}
#endif

#if defined(LGE_BATT_ATOM_RPC_MODE)
	rc = msm_chg_get_lge_batt_info(&new_batt_info);
	if (rc>0)
	{
		/*printk("%s() %d%%-%dmV-%ddegC-%d(stat)-%d(chg)-%d(pre)\n", __func__, new_batt_info.batt_remaining, 	\
			new_batt_info.batt_voltage, new_batt_info.batt_temperature, \
			new_batt_info.chg_status, new_batt_info.charger_type, new_batt_info.present);
		*/

		if ( lge_battery_info_changed() || new_batt_info.batt_remaining < 4)
		{
			lge_battery_info_update();
			power_supply_changed(&lge_power_supplies[BATTERY]);
		}
	}
#endif // #if defined(LGE_BATT_ATOM_RPC_MODE)

	return rc;
}

static int lge_battery_read_status(void)
{
	int ret=0;

	/* if (not first time) && ( too short time passed after reading data) then return */
	if (lge_batt.update_time && time_before(jiffies, lge_batt.update_time +
					   msecs_to_jiffies(cache_time)))
		return 0;

	ret = lge_battery_read();
	if (ret < 0) {
		printk(KERN_ERR "[BAT] lge_battery_read() fail \n");
		return 1;
	}

	lge_batt.update_time = jiffies;

	return 0;
}

#if defined(LGE_BATT_ATOM_RPC_MODE)
void lge_battery_update_charger(void)
{
	D_FUNC("%s()\n", __func__);

	if( BEFORE_INITIALIZATION == work_queue_init_ok )	// if work queue is not initialized yet
	{
		return;
	}

#if defined(LGE_BATT_ATOM_RPC_MODE)
	cancel_delayed_work(&lge_batt.monitor_work);
	//queue_delayed_work(lge_batt.monitor_wqueue, &lge_batt.monitor_work,  HZ * 30);
  #ifdef CONFIG_USB_FUNCTION_MSM_HSUSB
	if (CHG_UNDEFINED == get_charger_type())
	{
		new_batt_info.charger_type = CHG_CHARGER_IRQ__WALL_INVALID;
		power_supply_changed(&lge_power_supplies[BATTERY]);
		queue_delayed_work(lge_batt.monitor_wqueue, &lge_batt.monitor_work, HZ * 30);
	}
	else
  #endif
	{
		queue_delayed_work(lge_batt.monitor_wqueue, &lge_batt.monitor_work, HZ * 1);
	}
#endif
}

static void lge_battery_work(struct work_struct *work)
{
	//const int interval = HZ * 60;

	const int interval = HZ * 30;

	if( BEFORE_INITIALIZATION == work_queue_init_ok )	// if work queue is not initialized yet
	{
		return;
	}

	lge_battery_read_status();

#if defined(LGE_BATT_ATOM_RPC_MODE)
	queue_delayed_work(lge_batt.monitor_wqueue, &lge_batt.monitor_work, interval);
#endif
}
#endif //#if defined(LGE_BATT_ATOM_RPC_MODE)

static void lge_battery_external_power_changed(struct power_supply *psy)
{
	lprintk(D_BATT, "%s() start !!	\n", __func__ );

	if( BEFORE_INITIALIZATION == work_queue_init_ok )	// if work queue is not initialized yet
	{
		return;
	}

#if defined(LGE_BATT_ATOM_RPC_MODE)
	cancel_delayed_work(&lge_batt.monitor_work);
	queue_delayed_work(lge_batt.monitor_wqueue, &lge_batt.monitor_work, HZ/10);
#endif // #if defined(LGE_BATT_ATOM_RPC_MODE)

}

static int lge_battery_get_charging_status(void)
{
	int ret = POWER_SUPPLY_STATUS_UNKNOWN;

	lprintk(D_BATT, "%s()\n", __func__);

	// To Do ::
	switch (new_batt_info.charger_type) {
	case CHG_CHARGER_IRQ__WALL_INVALID:
	case CHG_CHARGER_IRQ__USB_INVALID:
		ret = POWER_SUPPLY_STATUS_NOT_CHARGING;
		break;
	case CHG_CHARGER_IRQ__WALL_VALID:
	case CHG_CHARGER_IRQ__USB_VALID:
		if (100 == batt_info.batt_remaining)
			ret = POWER_SUPPLY_STATUS_FULL;
		else
			ret = POWER_SUPPLY_STATUS_CHARGING;
		break;
	default:
		ret = POWER_SUPPLY_STATUS_UNKNOWN;
	}

	return ret;
}

static int lge_battery_get_property(struct power_supply *psy,
				       enum power_supply_property psp,
				       union power_supply_propval *val)
{
	D_FUNC("%s() power_supply_property=%d	\n", __func__, psp );

	//lge_battery_read_status();		// Take too long time So use buffered data

	switch (psp) {
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = batt_info.batt_voltage;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = batt_info.batt_remaining;
		break;
	//case POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN:
	//	val->intval = di->rated_capacity;
	//	break;
	//case POWER_SUPPLY_PROP_CHARGE_FULL:
	//	val->intval = di->full_active_uAh;
	//	break;
	//case POWER_SUPPLY_PROP_CHARGE_EMPTY:
	//	val->intval = di->empty_uAh;
	//	break;
	//case POWER_SUPPLY_PROP_CHARGE_NOW:
	//	val->intval = di->accum_current_uAh;
	//	break;
	case POWER_SUPPLY_PROP_TEMP:
		val->intval = batt_info.batt_temperature;
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = POWER_SUPPLY_HEALTH_GOOD;
		break;
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = lge_battery_get_charging_status();
		lprintk(D_BATT, "%s() :: power_supply_property->POWER_SUPPLY_PROP_STATUS :: val->intval = %d	\n", __func__, val->intval );
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = batt_info.present;
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static int lge_power_get_property(struct power_supply *psy,
				       enum power_supply_property psp,
				       union power_supply_propval *val)
{
	D_FUNC("%s() power_supply_property=%d	\n", __func__, psp );

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		if (psy->type == POWER_SUPPLY_TYPE_MAINS)
		{
			/* LGE_CHANGE_S [ybw75@lge.com] 2009-03-06, Workaround of detecting AC or USB  */
			// To Do ::
			if ( new_batt_info.charger_type == CHG_CHARGER_IRQ__WALL_VALID )
			{
				val->intval = 1;
			}
			else
			{
				val->intval = 0;
			}
			D("%s() :: POWER_SUPPLY_PROP_ONLINE :: MAINS = %d \n", __func__, val->intval );
			/* LGE_CHANGE_E [ybw75@lge.com] 2009-03-06 */
		}
		else if (psy->type == POWER_SUPPLY_TYPE_USB)
		{
			/* LGE_CHANGE_S [ybw75@lge.com] 2009-03-06, Workaround of detecting AC or USB  */
			if ( new_batt_info.charger_type == CHG_CHARGER_IRQ__USB_VALID )
			{
				val->intval = 1;
			}
			else
			{
				val->intval = 0;
			}
			D("%s() :: POWER_SUPPLY_PROP_ONLINE :: USB = %d \n", __func__, val->intval );
			/* LGE_CHANGE_E [ybw75@lge.com] 2009-03-06  */
		}
		else
		{
			val->intval = 0;
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int lge_battery_probe(struct platform_device *pdev)
{
	int i, retval = 0;

	lprintk(D_BATT, "%s() start !!  \n", __func__ );

	lge_batt.dev = &pdev->dev;

#if defined(LGE_BATT_ATOM_RPC_MODE)
	// Define & Init work queue for battery polling
	INIT_DELAYED_WORK(&lge_batt.monitor_work, lge_battery_work);
	//lge_batt.monitor_wqueue = create_singlethread_workqueue(pdev->dev.bus_id);

	/* LGE_CHANGE [pranav.s@lge.com] */
	lge_batt.monitor_wqueue = create_singlethread_workqueue(pdev->name);
	if (!lge_batt.monitor_wqueue) {
		retval = -ESRCH;
		goto workqueue_failed;
	}
#endif // #if defined(LGE_BATT_ATOM_RPC_MODE)

	// Initialize Battery property with default values
	batt_info.batt_voltage = LGE_BATT_DEFAULT_VOLTAGE_MV;
	batt_info.batt_remaining = LGE_BATT_DEFAULT_CAPACITY;
	batt_info.batt_temperature = LGE_BATT_DEFAULT_TEMPERATURE;

	/* init power supplier framework */
	for (i = 0; i < ARRAY_SIZE(lge_power_supplies); i++)
	{
		retval = power_supply_register(&pdev->dev, &lge_power_supplies[i]);
		if (retval)
		{
			printk("Failed to register power supply (%d)\n", retval);
			goto batt_failed;
		}
	}

#if defined(LGE_BATT_ATOM_RPC_MODE)
	// Battery polling start
	queue_delayed_work(lge_batt.monitor_wqueue, &lge_batt.monitor_work, HZ * 1);
#endif // #if defined(LGE_BATT_ATOM_RPC_MODE)

#if defined(SYSFS_LGE_BATT_TEST_MODE)
	lge_battery_test_create_attrs(lge_batt.dev);
#endif
#if defined(DEBUGFS_LGE_BATT_TEST_MODE)
	lge_battery_debug_init();
#endif

	work_queue_init_ok = AFTER_INITIALIZATION;

	goto success;

batt_failed:
	for (i = 0; i < ARRAY_SIZE(lge_power_supplies); i++) {
		power_supply_unregister(&lge_power_supplies[i]);
	}
workqueue_failed:
	destroy_workqueue(lge_batt.monitor_wqueue);
success:
#if defined(FEATURE_MTOA_CHARGER_RPC)
	msm_chg_mtoa_init_ok( (int)TRUE );
#endif // #if defined(FEATURE_MTOA_CHARGER_RPC)

	return retval;
}

static int lge_battery_remove(struct platform_device *pdev)
{
	int i;

	lprintk(D_BATT, "%s() start !!	\n", __func__ );

#if defined(LGE_BATT_ATOM_RPC_MODE)
	cancel_rearming_delayed_workqueue(lge_batt.monitor_wqueue,
					  &lge_batt.monitor_work);
	destroy_workqueue(lge_batt.monitor_wqueue);
#endif // #if defined(LGE_BATT_ATOM_RPC_MODE)

#if defined(FEATURE_MTOA_CHARGER_RPC)
	msm_chg_mtoa_init_ok( (int)FALSE );
#endif // #if defined(FEATURE_MTOA_CHARGER_RPC)

	for (i = 0; i < ARRAY_SIZE(lge_power_supplies); i++) {
		power_supply_unregister(&lge_power_supplies[i]);
	}

	return 0;
}

#ifdef CONFIG_PM

static int lge_battery_suspend(struct platform_device *pdev,
				  pm_message_t state)
{
	lprintk(D_BATT, "%s() start !!	\n", __func__ );

#if defined(FEATURE_MTOA_CHARGER_RPC)
	msm_chg_mtoa_init_ok( (int)FALSE );		// To Do : is it right that turning off MtoA rpc during suspend mode ?
#endif // #if defined(FEATURE_MTOA_CHARGER_RPC)

	return 0;
}

static int lge_battery_resume(struct platform_device *pdev)
{

	lprintk(D_BATT, "%s() start !!	\n", __func__ );
	D_FUNC("%s()\n", __func__ );

	power_supply_changed(&lge_power_supplies[BATTERY]);
	power_supply_changed(&lge_power_supplies[AC]);
	power_supply_changed(&lge_power_supplies[BATTERY]);

	if( BEFORE_INITIALIZATION == work_queue_init_ok )	// if work queue is not initialized yet
	{
		return 1;
	}

#if defined(LGE_BATT_ATOM_RPC_MODE)
	cancel_delayed_work(&lge_batt.monitor_work);
	queue_delayed_work(lge_batt.monitor_wqueue, &lge_batt.monitor_work, HZ);
#endif // #if defined(LGE_BATT_ATOM_RPC_MODE)

#if defined(FEATURE_MTOA_CHARGER_RPC)
	msm_chg_mtoa_init_ok( (int)TRUE );
#endif // #if defined(FEATURE_MTOA_CHARGER_RPC)

	return 0;
}

#else

#endif /* CONFIG_PM */

/* LGE_CHANGE_S [ybw75@lge.com] 2009-01-21, Add Charger MtoA (modem to app) RPC routine */
#if defined(FEATURE_MTOA_CHARGER_RPC)

#define CHG_MTOA_PROG				0x30100000
#define CHG_MTOA_VERS				0
#define RPC_CHG_MTOA_NULL			0
#define RPC_CHG_MTOA_BATT_LEVEL_UPDATE_PROC		1
#define RPC_CHG_MTOA_CABLE_STATUS_UPDATE_PROC	2
#define RPC_CHG_MTOA_BATT_STATUS_UPDATE_PROC	3

struct rpc_chg_mtoa_batt_level_udate_args {
	uint32 battery_level;
};

struct rpc_chg_mtoa_cable_status_update_args {
	uint32 chg_irq;
};

struct rpc_chg_mtoa_batt_status_udate_args {
	struct lge_battery_info_type lge_batt_info;
};

static int handle_chg_call(struct msm_rpc_server *server,
			       struct rpc_request_hdr *req, unsigned len)
{
	lprintk("%s()-%d\n", __func__, req->procedure);
	D_FUNC("%s()-%d\n", __func__, req->procedure);

#if defined(DEBUGFS_LGE_BATT_TEST_MODE) || defined(SYSFS_LGE_BATT_TEST_MODE)
	if (LGE_BATT_TEST_MODE_ON == lge_battery_test_mode )
	{
		return 0;
	}
#endif

	switch (req->procedure) {
	case RPC_CHG_MTOA_NULL:
		return 0;

	case RPC_CHG_MTOA_BATT_LEVEL_UPDATE_PROC: {
		struct rpc_chg_mtoa_batt_level_udate_args *args;
		args = (struct rpc_chg_mtoa_batt_level_udate_args *)(req + 1);
		args->battery_level = be32_to_cpu(args->battery_level);
		lprintk(D_BATT, " RPC_CHG_MTOA_BATT_LEVEL_UPDATE_PROC:  Batt_level = %d	\n", (int)(args->battery_level) );
		// To Do ::
		return 0;
	}
	case RPC_CHG_MTOA_CABLE_STATUS_UPDATE_PROC: {
		lprintk(D_BATT, " RPC_CHG_MTOA_CABLE_STATUS_UPDATE_PROC: 000 "  );
		struct rpc_chg_mtoa_cable_status_update_args *args;
		args = (struct rpc_chg_mtoa_cable_status_update_args *)(req + 1);
		args->chg_irq = be32_to_cpu(args->chg_irq);
		lprintk(D_BATT, " RPC_CHG_MTOA_CABLE_STATUS_UPDATE_PROC: 111 , irq = %d ", args->chg_irq  );
		switch ( (chg_charger_irq_type)(args->chg_irq) )
		{
			case CHG_CHARGER_IRQ__WALL_VALID:
			case CHG_CHARGER_IRQ__WALL_INVALID:
			case CHG_CHARGER_IRQ__USB_VALID:
			case CHG_CHARGER_IRQ__USB_INVALID:
				new_batt_info.charger_type = (chg_charger_irq_type)(args->chg_irq);
				break;
			default:
				new_batt_info.charger_type = CHG_CHARGER_IRQ__WALL_INVALID;
				break;
		}
		lprintk(D_BATT, " RPC_CHG_MTOA_CABLE_STATUS_UPDATE_PROC: 222 "  );
		power_supply_changed(&lge_power_supplies[USB]);
		power_supply_changed(&lge_power_supplies[AC]);
		power_supply_changed(&lge_power_supplies[BATTERY]);
		return 0;
	}
	case RPC_CHG_MTOA_BATT_STATUS_UPDATE_PROC: {
		struct rpc_chg_mtoa_batt_status_udate_args *args;
		args = (struct rpc_chg_mtoa_batt_status_udate_args *)(req + 1);

#if defined(DEBUGFS_LGE_BATT_TEST_MODE) || defined(SYSFS_LGE_BATT_TEST_MODE)
		if (LGE_BATT_TEST_MODE_ON == lge_battery_test_mode )
		{
			return 0;
		}
#endif
		new_batt_info.batt_remaining = be32_to_cpu(args->lge_batt_info.batt_remaining);
		new_batt_info.batt_voltage = be32_to_cpu(args->lge_batt_info.batt_voltage);
		new_batt_info.batt_current = be32_to_cpu(args->lge_batt_info.batt_current);
		new_batt_info.batt_temperature = be32_to_cpu(args->lge_batt_info.batt_temperature);
		new_batt_info.batt_flag = be32_to_cpu(args->lge_batt_info.batt_flag);
		new_batt_info.chg_status = be32_to_cpu(args->lge_batt_info.chg_status);
		new_batt_info.charger_type = be32_to_cpu(args->lge_batt_info.charger_type);
		new_batt_info.present = be32_to_cpu(args->lge_batt_info.present);
		lprintk(D_BATT, " RPC_CHG_MTOA_BATT_STATUS_UPDATE_PROC:  batt_remaining = %d	\n", new_batt_info.batt_remaining);
		lprintk(D_BATT, " RPC_CHG_MTOA_BATT_STATUS_UPDATE_PROC:  batt_voltage = %d	\n", new_batt_info.batt_voltage);
		lprintk(D_BATT, " RPC_CHG_MTOA_BATT_STATUS_UPDATE_PROC:  batt_current = %d	\n", new_batt_info.batt_current);
		lprintk(D_BATT, " RPC_CHG_MTOA_BATT_STATUS_UPDATE_PROC:  batt_temperature = %d	\n", new_batt_info.batt_temperature);
		lprintk(D_BATT, " RPC_CHG_MTOA_BATT_STATUS_UPDATE_PROC:  batt_flag = %d	\n", new_batt_info.batt_flag);
		lprintk(D_BATT, " RPC_CHG_MTOA_BATT_STATUS_UPDATE_PROC:  chg_status = %d  \n", new_batt_info.chg_status);
		lprintk(D_BATT, " RPC_CHG_MTOA_BATT_STATUS_UPDATE_PROC:  charger_type = %d  \n", new_batt_info.charger_type);
		/*
		printk("%s() %d%%-%dmV-%dmA-%ddegC-%d(flag)-%d(stat)-%d(chg)-%d(pre)\n", __func__, new_batt_info.batt_remaining, 	\
			new_batt_info.batt_voltage, new_batt_info.batt_current, new_batt_info.batt_temperature, \
			new_batt_info.batt_flag, new_batt_info.chg_status, new_batt_info.charger_type, new_batt_info.present);
		*/

		if (lge_battery_info_changed())
		{
			lge_battery_info_update();
			//printk("call power_supply_changed() -  charter_type :%d\n", new_batt_info.charger_type);
			power_supply_changed(&lge_power_supplies[BATTERY]);
			power_supply_changed(&lge_power_supplies[USB]);
			power_supply_changed(&lge_power_supplies[AC]);
		}

		return 0;
	}

	default:
		printk(KERN_ERR "%s: program 0x%08x:%d: unknown procedure %d\n",
		       __FUNCTION__, req->prog, req->vers, req->procedure);
		return -ENODEV;
	}
}

static struct msm_rpc_server chg_mtoa_server = {
	.prog = CHG_MTOA_PROG,
	.vers = CHG_MTOA_VERS,
	.rpc_call = handle_chg_call,
};
#endif // #if defined(FEATURE_MTOA_CHARGER_RPC)
/* LGE_CHANGE_E [ybw75@lge.com] 2009-01-21 */

#if defined(SYSFS_LGE_BATT_TEST_MODE)

#define LGE_BATTERY_TEST_ATTR(_name)							\
{										\
	.attr = { .name = #_name, .mode = (S_IRUGO|S_IWUGO), .owner = THIS_MODULE },	\
	.show = lge_battery_test_show_property,					\
	.store = lge_battery_test_store_property,								\
}

static struct device_attribute lge_battery_test_attrs[] = {
	LGE_BATTERY_TEST_ATTR(TestMode),
	LGE_BATTERY_TEST_ATTR(TestCapacity),
	LGE_BATTERY_TEST_ATTR(TestVoltage),
	LGE_BATTERY_TEST_ATTR(TestTemperature),
};

static int lge_battery_test_create_attrs(struct device * dev)
{
	int i, rc;

	lprintk(D_BATT, "%s() start !!	\n", __func__ );

	for (i = 0; i < ARRAY_SIZE(lge_battery_test_attrs); i++) {
		rc = device_create_file(dev, &lge_battery_test_attrs[i]);
		if (rc)
		{
			while (i--)
			device_remove_file(dev, &lge_battery_test_attrs[i]);
		}
	}

	return rc;
}

static ssize_t lge_battery_test_show_property(struct device *p_dev,
					 struct device_attribute *attr,
					 char *buf)
{
	int i = 0;
	const ptrdiff_t off = attr - lge_battery_test_attrs;

	switch (off) {
		case BATT_TEST_MODE:
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", lge_battery_test_mode);
			lprintk(D_BATT, " BATT_TEST_MODE writing, success_number = %d	\n", i );
			break;
		case BATT_TEST_CAPACITY:
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", batt_info.batt_remaining);
			lprintk(D_BATT, " BATT_TEST_CAPACITY writing, success_number = %d	\n", i );
			break;
		case BATT_TEST_VOLT:
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", batt_info.batt_voltage*1000);
			lprintk(D_BATT, " BATT_TEST_VOLT writing, success_number = %d	\n", i );
			break;
		case BATT_TEST_TEMP:
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", batt_info.batt_temperature);
			lprintk(D_BATT, " BATT_TEST_TEMP writing, success_number = %d	\n", i );
			break;
		default:
			i = -EINVAL;
	}
	return i;
}

static ssize_t lge_battery_test_store_property(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	unsigned long x;
	const ptrdiff_t off = attr - lge_battery_test_attrs;

	lprintk(D_BATT, "%s() start !!	\n", __func__ );

	switch (off)  {
	case BATT_TEST_MODE:
		if (sscanf(buf, "%ld\n", &x) == 1)
		{
			lge_battery_test_mode = x ;
			lprintk(D_BATT, " BATT_TEST_MODE reading = %d	\n", (int)x );
		}
		break;
	case BATT_TEST_CAPACITY:
		if (sscanf(buf, "%ld\n", &x) == 1)
		{
			lprintk(D_BATT, " BATT_TEST_CAPACITY reading = %d	\n", (int)x );
			batt_info.batt_remaining = x;
		}
		break;
	case BATT_TEST_VOLT:
		if (sscanf(buf, "%ld\n", &x) == 1)
		{
			lprintk(D_BATT, " BATT_TEST_VOLT reading = %d	\n", (int)x );
			batt_info.batt_voltage = (unsigned int)(x/1000);
		}

		break;
	case BATT_TEST_TEMP:
		if (sscanf(buf, "%ld\n", &x) == 1)
		{
			lprintk(D_BATT, " BATT_TEST_TEMP reading = %d	\n", (int)x );
			batt_info.batt_temperature = x;
		}
		break;
	default:
		count = -EINVAL;
	}

	power_supply_changed(&lge_power_supplies[BATTERY]);
	power_supply_changed(&lge_power_supplies[AC]);
	power_supply_changed(&lge_power_supplies[BATTERY]);

	return count;
}

#endif	// #if defined(SYSFS_LGE_BATT_TEST_MODE)

#if defined(DEBUGFS_LGE_BATT_TEST_MODE)

struct lge_batt_test_reg {
	const char *name;
	unsigned int code;
};

#define LGE_BATT_TEST_REG(_name, _code) { .name = _name, .code = _code}

static struct lge_batt_test_reg lge_batt_test_regs[] = {
	LGE_BATT_TEST_REG("TestMode",  	 BATT_TEST_MODE),
	LGE_BATT_TEST_REG("TestCapacity",    BATT_TEST_CAPACITY),
	LGE_BATT_TEST_REG("TestVoltage",    BATT_TEST_VOLT),
	LGE_BATT_TEST_REG("TestTemperature", BATT_TEST_TEMP),
	LGE_BATT_TEST_REG("PmTest", BATT_TEST_PM),
};

static int lge_battery_debug_set(void *data, u64 val)
{
	struct lge_batt_test_reg *p_lge_batt_test_reg = data;
	int rtn = 0;

	lprintk(D_BATT, "%s() : p_lge_batt_test_reg->code = %d	\n", __func__ , p_lge_batt_test_reg->code );

	switch (p_lge_batt_test_reg->code)
	{
		case BATT_TEST_MODE:
			lge_battery_test_mode = (int)val;
			break;
		case BATT_TEST_CAPACITY:
			batt_info.batt_remaining = (unsigned int)val;
			break;
		case BATT_TEST_VOLT:
			batt_info.batt_voltage = (unsigned int)((unsigned int)val/1000);
			break;
		case BATT_TEST_TEMP :
			batt_info.batt_temperature = (int)val;
			break;
		case BATT_TEST_PM:
			msm_chg_pm_test( (int)val );
			break;
		default:
			rtn = -EINVAL;
			break;
	}

	power_supply_changed(&lge_power_supplies[BATTERY]);
	power_supply_changed(&lge_power_supplies[AC]);
	power_supply_changed(&lge_power_supplies[BATTERY]);

	return rtn;

}

static int lge_battery_debug_get(void *data, u64 *val)
{
	struct lge_batt_test_reg *p_lge_batt_test_reg = data;
	int rtn = 0;

	lprintk(D_BATT, "%s() : p_lge_batt_test_reg->code = %d	\n", __func__ , p_lge_batt_test_reg->code );
	switch (p_lge_batt_test_reg->code)
	{
		case BATT_TEST_MODE:
			*val = (u64)lge_battery_test_mode;
			break;
		case BATT_TEST_CAPACITY:
			*val = (u64)(batt_info.batt_remaining);
			break;
		case BATT_TEST_VOLT:
			*val = (u64)(batt_info.batt_voltage*1000);
			break;
		case BATT_TEST_TEMP:
			*val = (u64)(batt_info.batt_temperature);
			break;
		case BATT_TEST_PM:
			*val = 0;
			break;
		default:
			rtn = -EINVAL;
			break;
	}

	return rtn;
}

DEFINE_SIMPLE_ATTRIBUTE(lge_battery_test_fops, lge_battery_debug_get, lge_battery_debug_set, "%llu\n");

void lge_battery_debug_init(void)
{
	struct dentry *dent;
	int n;

	dent = debugfs_create_dir("lge_battery_test", 0);
	if (IS_ERR(dent))
		lprintk(D_BATT, "%s() : debugfs_create_dir() Error !!!! 	\n", __func__ );

	for (n = 0; n < ARRAY_SIZE(lge_batt_test_regs); n++)
	{
		debugfs_create_file(lge_batt_test_regs[n].name, 0666, dent, \
			               &lge_batt_test_regs[n],	&lge_battery_test_fops);
	}
}

#endif // #if defined(DEBUGFS_LGE_BATT_TEST_MODE)

static struct platform_driver lge_battery_driver = {
	.probe = lge_battery_probe,
	.remove   = lge_battery_remove,
	.suspend  = lge_battery_suspend,
	.resume	  = lge_battery_resume,
	.driver = {
		.name = "lge_battery",
		.owner = THIS_MODULE,
	},
};

static int __init lge_battery_init(void)
{
	lprintk(D_BATT, "%s() start !!  \n", __func__ );

#if defined(FEATURE_MTOA_CHARGER_RPC)
	msm_rpc_create_server(&chg_mtoa_server);
	// msm_chg_mtoa_init_ok( (int)TRUE );	-> enable after finishing probe()
#endif // #if defined(FEATURE_MTOA_CHARGER_RPC)

	return platform_driver_register(&lge_battery_driver);
}

static void __exit lge_battery_exit(void)
{
	platform_driver_unregister(&lge_battery_driver);
}

module_init(lge_battery_init);
module_exit(lge_battery_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Szabolcs Gyurko <szabolcs.gyurko@tlt.hu>, "
	      "Matt Reimer <mreimer@vpop.net>, "
	      "Anton Vorontsov <cbou@mail.ru>");
MODULE_DESCRIPTION("lge_battery battery driver");
