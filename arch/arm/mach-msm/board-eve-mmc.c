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

static struct vreg *vreg_mmc;

static void (*wifi_status_cb)(int card_present, void *dev_id);
static void *wifi_status_cb_devid;

static int eve_wifi_cd = 0;

static int eve_wifi_status_register(
            void (*callback)(int card_present, void *dev_id),
            void *dev_id)
{
    if (wifi_status_cb)
        return -EAGAIN;
    wifi_status_cb = callback;
    wifi_status_cb_devid = dev_id;
    return 0;
}

int eve_wifi_set_carddetect(int val)
{
    eve_wifi_cd = val;
    if (wifi_status_cb) {
        wifi_status_cb(val, wifi_status_cb_devid);
    } else
        pr_warning("%s: Nobody to notify\n", __func__);
    return 0;
}

static int eve_wifi_power_state;

static int eve_wifi_reset_state;

int eve_wifi_reset(int on)
{
    printk("%s: do nothing\n", __func__);
    eve_wifi_reset_state = on;
    return 0;
}

static void config_gpio_table(uint32_t *table, int len)
{
    int n;
    unsigned id;
    for(n = 0; n < len; n++) {
        id = table[n];
        msm_proc_comm(PCOM_RPC_GPIO_TLMM_CONFIG_EX, &id, 0);
    }
}

static uint32_t wifi_on_gpio_table[] = {
    PCOM_GPIO_CFG(51, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), /* DAT3 */
    PCOM_GPIO_CFG(52, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), /* DAT2 */
    PCOM_GPIO_CFG(53, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), /* DAT1 */
    PCOM_GPIO_CFG(54, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), /* DAT0 */
    PCOM_GPIO_CFG(55, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), /* CMD */
    PCOM_GPIO_CFG(56, 1, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_8MA), /* CLK */
    PCOM_GPIO_CFG(EVE_GPIO_WIFI_IRQ, 0, GPIO_INPUT, GPIO_NO_PULL, GPIO_8MA),  /* WLAN IRQ */
};

static uint32_t wifi_off_gpio_table[] = {
    PCOM_GPIO_CFG(51, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_8MA), /* DAT3 */
    PCOM_GPIO_CFG(52, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_8MA), /* DAT2 */
    PCOM_GPIO_CFG(53, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_8MA), /* DAT1 */
    PCOM_GPIO_CFG(54, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_8MA), /* DAT0 */
    PCOM_GPIO_CFG(55, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_8MA), /* CMD */
    PCOM_GPIO_CFG(56, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_8MA), /* CLK */
    PCOM_GPIO_CFG(EVE_GPIO_WIFI_IRQ, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_8MA),  /* WLAN IRQ */
};

static uint32_t sdcard_on_gpio_table[] = {
    PCOM_GPIO_CFG(62, 2, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_8MA), /* CLK */
    PCOM_GPIO_CFG(63, 2, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), /* CMD */
    PCOM_GPIO_CFG(64, 2, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), /* DAT3 */
    PCOM_GPIO_CFG(65, 2, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), /* DAT2 */
    PCOM_GPIO_CFG(66, 2, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), /* DAT1 */
    PCOM_GPIO_CFG(67, 2, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), /* DAT0 */
};

static uint32_t sdcard_off_gpio_table[] = {
    PCOM_GPIO_CFG(62, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_8MA), /* CLK */
    PCOM_GPIO_CFG(63, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_8MA), /* CMD */
    PCOM_GPIO_CFG(64, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_8MA), /* DAT3 */
    PCOM_GPIO_CFG(65, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_8MA), /* DAT2 */
    PCOM_GPIO_CFG(66, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_8MA), /* DAT1 */
    PCOM_GPIO_CFG(67, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_8MA), /* DAT0 */
};

int eve_wifi_power(int on)
{
    printk("%s: %d\n", __func__, on);

    if (on) {
        config_gpio_table(wifi_on_gpio_table,
                  ARRAY_SIZE(wifi_on_gpio_table));
        mdelay(50);
    } else {
        config_gpio_table(wifi_off_gpio_table,
                  ARRAY_SIZE(wifi_off_gpio_table));
    }

    mdelay(100);
	gpio_set_value(BCM4325_GPIO_WL_REGON, on);
	mdelay(100);
    gpio_set_value(BCM4325_GPIO_WL_RESET, on);
	mdelay(200);

    eve_wifi_power_state = on;
    return 0;
}

static uint32_t msm_sdcc_setup_power(int on)
{
	int rc;
	if (!on) {
		rc = vreg_disable(vreg_mmc);
		if (rc)
			printk(KERN_ERR "%s: return val: %d \n",
					__func__, rc);
	} else {

		rc = vreg_set_level(vreg_mmc, 2850);
		if (!rc)
			rc = vreg_enable(vreg_mmc);
		if (rc)
			printk(KERN_ERR "%s: return val: %d \n",
					__func__, rc);
	}
	return 0;
}

static unsigned int eve_sdcc_slot_status(struct device *dev)
{
	return !gpio_get_value(GPIO_MMC_CD_N);
}
static unsigned int eve_sdcc_wlan_slot_status(struct device *dev)
{
	printk("%s: gpio: %d, cd: %d\n", __func__, gpio_get_value(BCM4325_GPIO_WL_RESET), eve_wifi_cd);
	return eve_wifi_cd;
}

static unsigned int wlan_transalate_vdd(int vdd) {
	printk("%s: %d\n", __func__, vdd);
	return 0;
}

static struct mmc_platform_data eve_sdcc_wlan_data = {
	.ocr_mask   = MMC_VDD_30_31,
	.status     = eve_sdcc_wlan_slot_status,
	.built_in 	= 1,
	.register_status_notify = eve_wifi_status_register,
};

static struct mmc_platform_data eve_sdcc_data = {
	.ocr_mask   = MMC_VDD_30_31,
	.status         = eve_sdcc_slot_status,
};

void __init eve_init_mmc(void)
{
	vreg_mmc = vreg_get(NULL, "mmc");
	if (IS_ERR(vreg_mmc)) {
		printk(KERN_ERR "%s: vreg get failed (%ld)\n",
				__func__, PTR_ERR(vreg_mmc));
		return;
	}

	gpio_request(BCM4325_GPIO_WL_RESET, "wlan_cd");
	gpio_request(BCM4325_GPIO_WL_RESET, "wlan_regon");
	gpio_direction_output(BCM4325_GPIO_WL_REGON, 0);
	gpio_direction_output(BCM4325_GPIO_WL_RESET, 0);
	mdelay(150);

	config_gpio_table(sdcard_on_gpio_table,
                  ARRAY_SIZE(sdcard_on_gpio_table));

	msm_sdcc_setup_power(1);

	msm_add_sdcc(1, &eve_sdcc_wlan_data, 0, 0);
	msm_add_sdcc(2, &eve_sdcc_data, 0, 0);
}
