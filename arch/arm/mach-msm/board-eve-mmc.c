#include <linux/delay.h>
#include <linux/err.h>
#include <asm/gpio.h>
#include <asm/mach/mmc.h>
#include <mach/vreg.h>
#include <linux/platform_device.h>
#include "proc_comm.h"
#include "board-eve.h"

int __init msm_add_sdcc(unsigned int controller, struct mmc_platform_data *plat,
						unsigned int stat_irq, unsigned long stat_irq_flags);

static unsigned long vreg_sts, gpio_sts;
static struct vreg *vreg_mmc;

static unsigned sdcc_cfg_data[][6] = {
	/* SDC1 configs */
	{
		GPIO_CFG(51, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
		GPIO_CFG(52, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
		GPIO_CFG(53, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
		GPIO_CFG(54, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
		GPIO_CFG(55, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
		GPIO_CFG(56, 1, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_8MA),
	},
	/* SDC2 configs */
	{
		GPIO_CFG(62, 2, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_8MA),
		GPIO_CFG(63, 2, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
		GPIO_CFG(64, 2, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
		GPIO_CFG(65, 2, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
		GPIO_CFG(66, 2, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
		GPIO_CFG(67, 2, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	},
	{
		/* SDC3 configs */
	},
	/* SDC4 configs */
	{
		/* LGE_CHANGE [dojip.kim@lge.com] 2010-04-05, not used */
#if 0
		GPIO_CFG(19, 3, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
		GPIO_CFG(20, 3, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
		GPIO_CFG(21, 3, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
		GPIO_CFG(107, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
		GPIO_CFG(108, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
		GPIO_CFG(109, 1, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_8MA),
#endif
	}
};

static void msm_sdcc_setup_gpio(int dev_id, unsigned int enable)
{
	int i, rc;

	if (!(test_bit(dev_id, &gpio_sts)^enable))
		return;

	if (enable)
		set_bit(dev_id, &gpio_sts);
	else
		clear_bit(dev_id, &gpio_sts);

	for (i = 0; i < ARRAY_SIZE(sdcc_cfg_data[dev_id - 1]); i++) {
		rc = gpio_tlmm_config(sdcc_cfg_data[dev_id - 1][i],
				enable ? GPIO_ENABLE : GPIO_DISABLE);
		if (rc) {
			printk(KERN_ERR "%s: gpio_tlmm_config(%#x)=%d\n",
					__func__, sdcc_cfg_data[dev_id - 1][i], rc);
		}
	}
}

static uint32_t msm_sdcc_setup_power(struct device *dv, unsigned int vdd)
{
	int rc = 0;
	struct platform_device *pdev;

	pdev = container_of(dv, struct platform_device, dev);
	msm_sdcc_setup_gpio(pdev->id, !!vdd);

	if (vdd == 0) {
		if (!vreg_sts)
			return 0;

		clear_bit(pdev->id, &vreg_sts);

		if (!vreg_sts) {
			rc = vreg_disable(vreg_mmc);
			if (rc)
				printk(KERN_ERR "%s: return val: %d \n",
						__func__, rc);
		}
		return 0;
	}

	if (!vreg_sts) {
		rc = vreg_set_level(vreg_mmc, 2850);
		if (!rc)
			rc = vreg_enable(vreg_mmc);
		if (rc)
			printk(KERN_ERR "%s: return val: %d \n",
					__func__, rc);
	}
	set_bit(pdev->id, &vreg_sts);
	return 0;
}

static unsigned int eve_sdcc_slot_status(struct device *dev)
{
	return !gpio_get_value(GPIO_MMC_CD_N);
}
static unsigned int eve_sdcc_wlan_slot_status(struct device *dev)
{
	return gpio_get_value(BCM4325_GPIO_WL_RESET);
}

static struct mmc_platform_data eve_sdcc_wlan_data = {
	.ocr_mask   = MMC_VDD_30_31,
	.translate_vdd  = msm_sdcc_setup_power,
	.status     = eve_sdcc_wlan_slot_status,
};

static struct mmc_platform_data eve_sdcc_data = {
	.ocr_mask   = MMC_VDD_30_31,
	.translate_vdd  = msm_sdcc_setup_power,
	.status         = eve_sdcc_slot_status,
};

static void sdcc_gpio_init(void)
{
	int rc = 0;
	if (gpio_request(GPIO_MMC_CD_N, "sdc2_status_irq"))
		pr_err("failed to request gpio sdc2_status_irq\n");
	rc = gpio_tlmm_config(GPIO_CFG(GPIO_MMC_CD_N, 0, GPIO_INPUT, GPIO_PULL_UP,
				GPIO_2MA), GPIO_ENABLE);
	if (rc)
		printk(KERN_ERR "%s: Failed to configure GPIO %d\n",
				__func__, rc);

	/* SDC1 GPIOs */
	if (gpio_request(51, "sdc1_data_3"))
		pr_err("failed to request gpio sdc1_data_3\n");
	if (gpio_request(52, "sdc1_data_2"))
		pr_err("failed to request gpio sdc1_data_2\n");
	if (gpio_request(53, "sdc1_data_1"))
		pr_err("failed to request gpio sdc1_data_1\n");
	if (gpio_request(54, "sdc1_data_0"))
		pr_err("failed to request gpio sdc1_data_0\n");
	if (gpio_request(55, "sdc1_cmd"))
		pr_err("failed to request gpio sdc1_cmd\n");
	if (gpio_request(56, "sdc1_clk"))
		pr_err("failed to request gpio sdc1_clk\n");

	/* SDC2 GPIOs */
	if (gpio_request(62, "sdc2_clk"))
		pr_err("failed to request gpio sdc2_clk\n");
	if (gpio_request(63, "sdc2_cmd"))
		pr_err("failed to request gpio sdc2_cmd\n");
	if (gpio_request(64, "sdc2_data_3"))
		pr_err("failed to request gpio sdc2_data_3\n");
	if (gpio_request(65, "sdc2_data_2"))
		pr_err("failed to request gpio sdc2_data_2\n");
	if (gpio_request(66, "sdc2_data_1"))
		pr_err("failed to request gpio sdc2_data_1\n");
	if (gpio_request(67, "sdc2_data_0"))
		pr_err("failed to request gpio sdc2_data_0\n");
}

void __init eve_init_mmc(void)
{
	vreg_mmc = vreg_get(NULL, "mmc");
	if (IS_ERR(vreg_mmc)) {
		printk(KERN_ERR "%s: vreg get failed (%ld)\n",
				__func__, PTR_ERR(vreg_mmc));
		return;
	}

	sdcc_gpio_init();
	/* LGE_CHANGE_S [yoohoo@lge.com] 2009-03-05, BCM4325 control gpio */
	/* countrol output */
	gpio_direction_output(BCM4325_GPIO_WL_REGON, 0);
	gpio_direction_output(BCM4325_GPIO_WL_RESET, 0);
	mdelay(150);

	gpio_request(BCM4325_GPIO_WL_RESET, "wlan_cd");

	msm_add_sdcc(1, &eve_sdcc_wlan_data, gpio_to_irq(BCM4325_GPIO_WL_RESET), IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING);

	msm_add_sdcc(2, &eve_sdcc_data, MSM_GPIO_TO_INT(GPIO_MMC_CD_N),
			(IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING));
}
