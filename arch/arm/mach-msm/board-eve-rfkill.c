/*
 * Copyright (C) 2009 Google, Inc.
 * Copyright (C) 2009 HTC Corporation.
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

#include <linux/delay.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/rfkill.h>
#include <asm/gpio.h>
#include <asm/mach-types.h>
#include <mach/proc_comm.h>

#include "board-eve.h"

static struct rfkill *bt_rfk;
static const char bt_name[] = "bcm4329";

enum {
        BT_WAKE,
        BT_RFR,
        BT_CTS,
        BT_RX,
        BT_TX,
        BT_PCM_DOUT,
        BT_PCM_DIN,
        BT_PCM_SYNC,
        BT_PCM_CLK,
        BT_HOST_WAKE,
};
// shyoo modify
static unsigned bt_config_power_on[] = {
        GPIO_CFG(92, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),   /* WAKE */
        GPIO_CFG(43, 2, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),   /* RFR */
        GPIO_CFG(44, 2, GPIO_INPUT,  GPIO_NO_PULL, GPIO_2MA),   /* CTS */
        GPIO_CFG(45, 2, GPIO_INPUT,  GPIO_NO_PULL, GPIO_2MA),   /* Rx */
        GPIO_CFG(46, 3, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),   /* Tx */
        GPIO_CFG(68, 1, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),   /* PCM_DOUT */
        GPIO_CFG(69, 1, GPIO_INPUT,  GPIO_NO_PULL, GPIO_2MA),   /* PCM_DIN */
        GPIO_CFG(70, 2, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),   /* PCM_SYNC */
        GPIO_CFG(71, 2, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),   /* PCM_CLK */
        GPIO_CFG(83, 0, GPIO_INPUT,  GPIO_NO_PULL, GPIO_2MA),   /* HOST_WAKE */
};
static unsigned bt_config_power_off[] = {
        GPIO_CFG(92, 0, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA), /* WAKE */
        GPIO_CFG(43, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),  /* RFR */
        GPIO_CFG(44, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),  /* CTS */
        GPIO_CFG(45, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),  /* Rx */
        GPIO_CFG(46, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),  /* Tx */
        GPIO_CFG(68, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),  /* PCM_DOUT */
        GPIO_CFG(69, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),  /* PCM_DIN */
        GPIO_CFG(70, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),  /* PCM_SYNC */
        GPIO_CFG(71, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),  /* PCM_CLK */
        GPIO_CFG(83, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),  /* HOST_WAKE */
};

static int bluetooth_power(int on)
{
        int pin, rc;

        printk(KERN_DEBUG "%s(%d)\n", __func__, on);
        printk(KERN_DEBUG "on_off: %d\n", on);  //added by shyoo

        if (on) {
                for (pin = 0; pin < ARRAY_SIZE(bt_config_power_on); pin++) {
                        rc = gpio_tlmm_config(bt_config_power_on[pin],
                                        GPIO_ENABLE);
                        if (rc) {
                                printk(KERN_ERR
                                                "%s: gpio_tlmm_config(%#x)=%d\n",
                                                __func__, bt_config_power_on[pin], rc);
                                return -EIO;
                        }
                }

                printk(KERN_DEBUG       "*****wait for regulator turned-on, 150ms*****\n");

#if 1  //fm White
/*turn-on regulator*/
if(!gpio_get_value(GPIO_BT_REG_ON))
        gpio_set_value(GPIO_BT_REG_ON, 1);

/*wait for regulator turned-on, 200ms*/
msleep (200);

/*drive /RESET pin to LOW*/
gpio_set_value(GPIO_BT_RESET_N, 0);
/*reset assert for 120ms*/
msleep (120);

/*Power On Reset BCM4325D0*/
gpio_set_value(GPIO_BT_RESET_N, 1);
/*wait for 50ms for POR BCM2045*/
msleep (50);
#else
                /*turn-on regulator*/
                if(!gpio_get_value(GPIO_BT_REG_ON))
                        gpio_set_value(GPIO_BT_REG_ON, 1);

                /*drive /RESET pin to LOW*/
                gpio_set_value(GPIO_BT_RESET_N, 0);
                /*wait for regulator turned-on, 150ms*/
                msleep (150);
                /*Power On Reset BCM4325D0*/
                gpio_set_value(GPIO_BT_RESET_N, 1);
                /*wait for 20ms for POR BCM2045*/
                msleep (20);
#endif
        } else {
                for (pin = 0; pin < ARRAY_SIZE(bt_config_power_off); pin++) {
                        rc = gpio_tlmm_config(bt_config_power_off[pin],
                                        GPIO_ENABLE);
                        if (rc) {
                                printk(KERN_ERR
                                                "%s: gpio_tlmm_config(%#x)=%d\n",
                                                __func__, bt_config_power_off[pin], rc);
                                return -EIO;
                        }
                }
                gpio_set_value(GPIO_BT_RESET_N, 0);
                if(!gpio_get_value(GPIO_WL_RESET_N))
                        gpio_set_value(GPIO_BT_REG_ON, 0);
        }
        return 0;
}


static struct rfkill_ops eve_rfkill_ops = {
	.set_block = bluetooth_power,
};

static int eve_rfkill_probe(struct platform_device *pdev)
{
	int rc = 0;
	bool default_state = true;  /* off */

	rc = gpio_request(GPIO_BT_RESET_N, "bt_reset");
	if (rc)
		goto err_gpio_reset;

	bluetooth_power(default_state);

	bt_rfk = rfkill_alloc(bt_name, &pdev->dev, RFKILL_TYPE_BLUETOOTH,
				&eve_rfkill_ops, NULL);
	if (!bt_rfk) {
		rc = -ENOMEM;
	}

	rfkill_set_states(bt_rfk, default_state, false);

	/* userspace cannot take exclusive control */

	rc = rfkill_register(bt_rfk);
	if (rc)
		goto err_rfkill_reg;

	return 0;

err_rfkill_reg:
	rfkill_destroy(bt_rfk);
err_gpio_reset:
	return rc;
}

static int eve_rfkill_remove(struct platform_device *dev)
{
	rfkill_unregister(bt_rfk);
	rfkill_destroy(bt_rfk);
	gpio_free(GPIO_BT_RESET_N);

	return 0;
}

static struct platform_driver eve_rfkill_driver = {
	.probe = eve_rfkill_probe,
	.remove = eve_rfkill_remove,
	.driver = {
		.name = "eve_rfkill",
		.owner = THIS_MODULE,
	},
};

static int __init eve_rfkill_init(void)
{
	if (!machine_is_eve())
		return 0;

	return platform_driver_register(&eve_rfkill_driver);
}

static void __exit eve_rfkill_exit(void)
{
	platform_driver_unregister(&eve_rfkill_driver);
}

module_init(eve_rfkill_init);
module_exit(eve_rfkill_exit);
MODULE_DESCRIPTION("eve rfkill");
MODULE_AUTHOR("Nick Pelly <npelly@google.com>");
MODULE_LICENSE("GPL");
