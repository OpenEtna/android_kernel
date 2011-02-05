#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <asm/mach-types.h>
#include <mach/camera.h>
#include <mach/msm_iomap.h>
#include <mach/msm_flashlight.h>
#include <mach/gpio.h>
#include "proc_comm.h"
#include "board-eve.h"

/* LGE_CHANGE [cleaneye@lge.com] 2009-02-24, temporary blocking */
/* the following are codes about qualcomm surf specific camera device driver
 * so these codes may not work well.
 * moreover, there is possibility of side-effect related to GPIO setting.
 */
static uint32_t camera_off_gpio_table[] = {
	/* parallel CAMERA interfaces */
	PCOM_GPIO_CFG(4,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT0 */
	PCOM_GPIO_CFG(5,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT1 */
	PCOM_GPIO_CFG(6,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT2 */
	PCOM_GPIO_CFG(7,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT3 */
	PCOM_GPIO_CFG(8,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT4 */
	PCOM_GPIO_CFG(9,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT5 */
	PCOM_GPIO_CFG(10, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT6 */
	PCOM_GPIO_CFG(11, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT7 */
	PCOM_GPIO_CFG(12, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* PCLK */
	PCOM_GPIO_CFG(13, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* HSYNC_IN */
	PCOM_GPIO_CFG(14, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* VSYNC_IN */
	//TODO: why is this not disabled? bravo does it
	//PCOM_GPIO_CFG(15, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), /* MCLK */
};

static uint32_t camera_on_gpio_table[] = {
	/* parallel CAMERA interfaces */
	PCOM_GPIO_CFG(4,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT0 */
	PCOM_GPIO_CFG(5,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT1 */
	PCOM_GPIO_CFG(6,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT2 */
	PCOM_GPIO_CFG(7,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT3 */
	PCOM_GPIO_CFG(8,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT4 */
	PCOM_GPIO_CFG(9,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT5 */
	PCOM_GPIO_CFG(10, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT6 */
	PCOM_GPIO_CFG(11, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT7 */
	PCOM_GPIO_CFG(12, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_16MA), /* PCLK */
	PCOM_GPIO_CFG(13, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* HSYNC_IN */
	PCOM_GPIO_CFG(14, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* VSYNC_IN */
	PCOM_GPIO_CFG(15, 1, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_16MA), /* MCLK */
};

static void config_camera_on_gpios(void)
{
	printk("msm_camera: config camera gpio ON\n");
	config_gpio_table(camera_on_gpio_table,
			ARRAY_SIZE(camera_on_gpio_table));
}

static void config_camera_off_gpios(void)
{
	printk("msm_camera: config camera gpio OFF\n");
	config_gpio_table(camera_off_gpio_table,
			ARRAY_SIZE(camera_off_gpio_table));
}

#ifdef NEW_MSM_CAMERA
static struct msm_camera_device_platform_data msm_camera_device_data = {
    .camera_gpio_on  = config_camera_on_gpios,
    .camera_gpio_off = config_camera_off_gpios,
    .ioext.mdcphy = MSM_MDC_PHYS,
    .ioext.mdcsz  = MSM_MDC_SIZE,
    .ioext.appphy = MSM_CLK_CTL_PHYS,
    .ioext.appsz  = MSM_CLK_CTL_SIZE,
};

int flashlight_control(int mode) {
	printk("%s: mode = %d\n",__func__,mode);
	return 0;
}

static struct camera_flash_cfg msm_camera_sensor_flash_cfg = {
    .camera_flash       = flashlight_control,
    .num_flash_levels   = FLASHLIGHT_NUM,
    .low_temp_limit     = 5,
    .low_cap_limit      = 15,
};

static struct msm_camera_sensor_info msm_camera_sensor = {
	.sensor_reset = 0,
	.sensor_pwd   = 0,
	.vcm_pwd      = 0,
	.sensor_name  = "mv9319_sony",
	.flash_type   = MSM_CAMERA_FLASH_NONE,
	.pdata      = &msm_camera_device_data,
//	.resource   = msm_camera_resources,
//  .num_resources  = ARRAY_SIZE(msm_camera_resources),
    .flash_cfg  = &msm_camera_sensor_flash_cfg,
};

static struct platform_device msm_camera_sensor_mv9391 = {
	.name   = "msm_camera_mv9319",
	.dev    = {
		.platform_data = &msm_camera_sensor,
	},
};

static int __init eve_init_camera(void)
{
	if (!machine_is_eve())
		return 0;

	config_camera_off_gpios();
	platform_device_register(&msm_camera_sensor_mv9391);
	return 0;
}

device_initcall(eve_init_camera);
#else

#define MSM_PROBE_INIT(name) name##_probe_init
static struct msm_camera_sensor_info msm_camera_sensor[] = {
    {
        .sensor_reset = 0,
        .sensor_pwd   = 0,
        .vcm_pwd      = 0,
        .sensor_name  = "mv9319_sony",
        .flash_type     = MSM_CAMERA_FLASH_NONE,
#ifdef CONFIG_MSM_CAMERA
        .sensor_probe = MSM_PROBE_INIT(mv9319),
#endif
    },
};
#undef MSM_PROBE_INIT

static struct msm_camera_device_platform_data msm_camera_device_data = {
    .camera_gpio_on  = config_camera_on_gpios,
    .camera_gpio_off = config_camera_off_gpios,
    .snum = ARRAY_SIZE(msm_camera_sensor),
    .sinfo = &msm_camera_sensor[0],
    .ioext.mdcphy = MSM_MDC_PHYS,
    .ioext.mdcsz  = MSM_MDC_SIZE,
    .ioext.appphy = MSM_CLK_CTL_PHYS,
    .ioext.appsz  = MSM_CLK_CTL_SIZE,
};

static struct platform_device msm_camera_device = {
    .name   = "msm_camera",
    .id = 0,
};

static struct platform_device eve_camera_flashlight_device = {
        .name   = "flashlight",
        .id     = 0,
        .dev = {
                .platform_data = 0,
          },
};

static void __init eve_init_camera(void)
{
	msm_camera_device.dev.platform_data = &msm_camera_device_data;

    platform_device_register(&eve_camera_flashlight_device);
	platform_device_register(&msm_camera_device);

    config_camera_off_gpios();
}

device_initcall(eve_init_camera);
#endif
