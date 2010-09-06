/* -*- mode: c; c-basic-offset: 8; -*-
 * vim: noexpandtab sw=8 ts=8 sts=0:
 *
 * Copyright (c) 2008-2009 QUALCOMM USA, INC.
 * Copyright (c) 2009 LG Electronics, INC.
 * 
 * All source code in this file is licensed under the following license
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can find it at http://www.fsf.org
 */

#include <linux/delay.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <media/msm_camera.h>
#include <mach/vreg.h>
#include <mach/gpio.h>
#include <linux/syscalls.h>
#include "mv9319.h"

#define FEATURE_MIMIC_ISP_JPG_SIZE 0
#define FEATURE_DEBUG_CAPTURE_TIMING 0
#define FEATURE_CONTROL_MCLK 0
#define FEATURE_NEW_POWER_ON_OFF_SEQUENCE 1

/* LGE_CHANGE [dojip.kim@lge.com] 2010-04-27, blocked the debugging message */
//#define CAMERA_DEBUG 1
#define CAMERA_DEBUG 0
#if (CAMERA_DEBUG)
#define LDBG(fmt, args...) printk(fmt,##args)
#define LDBG1(fmt, args...) do {} while(0) //printk(fmt,##args)
#else
#define LDBG(args...) do {} while(0)
#define LDBG1(args...) do {} while(0)
#endif

// GPIO Mapping
#define GPIO_CAM_RESET 0
#define GPIO_CAM_MCLK 15

#if FEATURE_DEBUG_CAPTURE_TIMING
#define GPIO_RES_CHG_BB 82
#endif

#define MV9319_SET_DELAY_MSEC 10

static unsigned cam_mclk_config[2] = {
#if 1
	GPIO_CFG(GPIO_CAM_MCLK, 0, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA),
	GPIO_CFG(GPIO_CAM_MCLK, 1, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),
#else
	/* Qualcomm's default setting */
	GPIO_CFG(GPIO_CAM_MCLK, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),
	GPIO_CFG(GPIO_CAM_MCLK, 1, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_16MA),
#endif
};

static int flag_update_firmware = 0;
static unsigned char firmware_ver_major, firmware_ver_minor;
static int flickerless_hz = MV9319_FLICKERLESS_60HZ;

#define MV9319_DEFAULT_CLOCK_RATE  24000000

#define MV9319_FIRMWARE_FILE_PATH "/sdcard/MV9319Plus_EVE.bin"

struct mv9319_work_t {
	struct work_struct work;
};

static struct mv9319_work_t *mv9319_sensorw;
static struct i2c_client *mv9319_client;
static struct i2c_client *mv9319_fw_client;

struct mv9319_ctrl_t {
	struct msm_camera_sensor_info *sensordata;
	enum sensor_mode_t sensormode;
	enum camera_flash_mode_t flashmode;

	unsigned char qfactor;
	unsigned char jpg_buf_size;

	/* for Video Camera */
	enum camera_effect_t effect;
	enum camera_wb_t wb;
	enum camera_scene_mode_t scene;
	unsigned char brightness;
	unsigned char zoom;
	int video_flash;
	int fps_control;
	int af_enabled;
#if FEATURE_MIMIC_ISP_JPG_SIZE
	/*enum camera_img_quality_t*/ int img_quality;
	/*enum sensor_resolution_t*/ int res;
#endif
};

struct mv9319_i2c_reg_conf {
	unsigned short waddr;
	unsigned short wdata;
};

static struct mv9319_ctrl_t *mv9319_ctrl;
static DECLARE_WAIT_QUEUE_HEAD(mv9319_wait_queue);
DECLARE_MUTEX(mv9319_sem);

int eve_flash_set_led_state(int led_state);

#define LDO_NO_5M	1
#define LDO_NO_5M_AVDD	3

#ifdef CONFIG_BACKLIGHT_EVE
int bd6083_set_ldo_power(int ldo_no, int on);
int bd6083_set_ldo_vout(int ldo_no, unsigned vout_mv);
int bd6083_set_camera_on_mode(void);
int bd6083_set_camera_off_mode(void);
#endif

static int mv9319_set_vreg_avdd(int on)
{
#ifdef CONFIG_BACKLIGHT_EVE
	if (on) {
		if (bd6083_set_ldo_vout(LDO_NO_5M_AVDD, 2800))
			goto failure;
		if (bd6083_set_ldo_power(LDO_NO_5M_AVDD, 1))
			goto failure;
	} else {
		if (bd6083_set_ldo_power(LDO_NO_5M_AVDD, 0))
			goto failure;
	}
	return 0;
failure:
#endif
	printk("mv9319: failed to set vreg_avdd\n");
	return -1;
}

static int mv9319_set_vreg_5m(int on)
{
#ifdef CONFIG_BACKLIGHT_EVE
	if (on) {
		if (bd6083_set_ldo_vout(LDO_NO_5M, 2800))
			goto failure;
		if (bd6083_set_ldo_power(LDO_NO_5M, 1))
			goto failure;
	} else {
		if (bd6083_set_ldo_power(LDO_NO_5M, 0))
			goto failure;
	}
	return 0;
failure:
#endif
	printk("mv9319: failed to set vreg_5m\n");
	return -1;
}

static int mv9319_i2c_write_buffer(struct i2c_client *write_client,
				   unsigned char *str, unsigned short length)
{
	struct i2c_msg msg[] = {
		{
		 .addr = write_client->addr,
		 .flags = 0,
		 .len = length,
		 .buf = str,
		 },
	};

	if (i2c_transfer(write_client->adapter, msg, 1) < 0) {
		printk("mv9319: write i2c buffer failed\n");
		return -EIO;
	}

	return 0;
}

static int mv9319_i2c_write_byte(struct i2c_client *write_client,
				 unsigned short waddr, unsigned short wdata)
{
	unsigned char str[2];
	struct i2c_msg msg[1];

	memset(str, 0, sizeof(str));

	str[0] = waddr;
	str[1] = wdata;

	msg[0].addr = write_client->addr;
	msg[0].flags = 0;
	msg[0].len = 2;
	msg[0].buf = str;

	if (i2c_transfer(write_client->adapter, msg, 1) < 0) {
		printk("mv9319: write i2c byte failed\n");
		return -EIO;
	}

	return 0;
}

static int mv9319_i2c_write_byte_table(struct i2c_client *write_client,
				       struct mv9319_i2c_reg_conf const
				       *reg_conf_tbl, int num_of_items_in_table)
{
	int i;
	int rc = -EFAULT;

	for (i = 0; i < num_of_items_in_table; i++) {
		rc = mv9319_i2c_write_byte(write_client,
					   reg_conf_tbl->waddr,
					   reg_conf_tbl->wdata);
		if (rc < 0)
			break;
		reg_conf_tbl++;
	}

	return rc;
}

static int mv9319_i2c_read_byte(struct i2c_client *read_client,
				unsigned char raddr, unsigned char *rdata)
{
	int ret = 0;

	struct i2c_msg msgs[2] = {
		{
		 .addr = read_client->addr,
		 .flags = 0,
		 .len = 1,
		 .buf = &raddr,
		 },
		{
		 .addr = read_client->addr,
		 .flags = I2C_M_RD,
		 .len = 1,
		 .buf = rdata,
		 },
	};

	ret = i2c_transfer(read_client->adapter, msgs, 2);
	if (ret < 0) {
		printk("mv9319: read i2c byte failed!, %d\n", ret);
		return -EIO;
	}

	return 0;
}

#if FEATURE_NEW_POWER_ON_OFF_SEQUENCE
static int mv9319_power_on_1(void)
{
	struct vreg *vreg_5m_dvdd;
	struct vreg *vreg_cam_af;
	int rc = -1;

	LDBG("mv9319: %s called\n", __func__);

	// RESET to Low
	gpio_set_value(GPIO_CAM_RESET, 0);

	mdelay(10);

	mv9319_set_vreg_5m(1);

	vreg_5m_dvdd = vreg_get(0, "rftx");
	vreg_enable(vreg_5m_dvdd);
	rc = vreg_set_level(vreg_5m_dvdd, 1800);
	if (rc != 0) {
		printk("mv9319: 5m_dvdd failed\n");
		return -1;
	}
	mv9319_set_vreg_avdd(1);

	vreg_cam_af = vreg_get(0, "rfrx2");
	vreg_enable(vreg_cam_af);
	rc = vreg_set_level(vreg_cam_af, 2800);
	if (rc != 0) {
		printk("mv9319: cam_af failed\n");
		return -1;
	}

	mdelay(5);
	return 0;
}

static int mv9319_power_on_2(void)
{
	LDBG("mv9319: %s called\n", __func__);
	mdelay(10);

	// RESET to HIGH
	gpio_set_value(GPIO_CAM_RESET, 1);

	mdelay(5);

	return 0;
}

int mv9319_power_off_1(void)
{
	int rc = 0;
	LDBG("mv9319: %s called\n", __func__);
	// Reset to Low
	gpio_set_value(GPIO_CAM_RESET, 0);

	mdelay(10);
	
	// MCLK to Low
	rc = gpio_tlmm_config(cam_mclk_config[0], GPIO_ENABLE);
	if (rc != 0) {
		printk("mv9319: cam_mclk_config_0 error %d\n", rc);
		return -1;
	}
	gpio_configure(GPIO_CAM_MCLK, GPIOF_DRIVE_OUTPUT);
	gpio_set_value(GPIO_CAM_MCLK, 0);
	mdelay(1);

	return rc;
}
EXPORT_SYMBOL(mv9319_power_off_1);

static void mv9319_power_off_2(void)
{
	struct vreg *vreg_5m_dvdd;
	struct vreg *vreg_cam_af;

	LDBG("mv9319: %s called\n", __func__);
	mdelay(5);

	// Disable Power
	vreg_5m_dvdd = vreg_get(0, "rftx");
	vreg_disable(vreg_5m_dvdd);

	mv9319_set_vreg_5m(0);
	mv9319_set_vreg_avdd(0);

	vreg_cam_af = vreg_get(0, "rfrx2");
	vreg_disable(vreg_cam_af);
}
#else /* FEATURE_NEW_POWER_ON_OFF_SEQUENCE */
static int mv9319_power_on(void)
{
	struct vreg *vreg_5m_dvdd;
	struct vreg *vreg_cam_af;
	int rc = -1;

	LDBG("mv9319: %s called\n", __func__);

	// RESET to Low
	gpio_set_value(GPIO_CAM_RESET, 0);

#if FEATURE_CONTROL_MCLK
	// MCLK to Low
	rc = gpio_tlmm_config(cam_mclk_config[0], GPIO_ENABLE);
	if (rc != 0) {
		printk("mv9319: cam_mclk_config_0 error\n");
		return -1;
	}
	gpio_configure(GPIO_CAM_MCLK, GPIOF_DRIVE_OUTPUT);
	gpio_set_value(GPIO_CAM_MCLK, 0);
#endif

	// Enable Power
	vreg_5m_dvdd = vreg_get(0, "rftx");
	vreg_enable(vreg_5m_dvdd);
	rc = vreg_set_level(vreg_5m_dvdd, 1800);
	if (rc != 0) {
		printk("mv9319: 5m_dvdd failed\n");
		return -1;
	}

	mv9319_set_vreg_avdd(1);
	mv9319_set_vreg_5m(1);

	vreg_cam_af = vreg_get(0, "rfrx2");
	vreg_enable(vreg_cam_af);
	rc = vreg_set_level(vreg_cam_af, 2800);
	if (rc != 0) {
		printk("mv9319: cam_af failed\n");
		return -1;
	}

	msleep(1);

#if FEATURE_CONTROL_MCLK
	// Enable MCLK
	rc = gpio_tlmm_config(cam_mclk_config[1], GPIO_ENABLE);
	if (rc != 0) {
		printk("mv9319: cam_mclk_config_1 error\n");
		return -1;
	}
#endif

	msleep(10);

	// RESET to HIGH
	gpio_set_value(GPIO_CAM_RESET, 1);

	msleep(1);

	return 0;
}

static void mv9319_power_off(void)
{
	struct vreg *vreg_5m_dvdd;
	struct vreg *vreg_cam_af;
	int rc = -1;

	LDBG("mv9319: %s called\n", __func__);

	// Reset to Low
	gpio_set_value(GPIO_CAM_RESET, 0);

	msleep(5);

#if FEATURE_CONTROL_MCLK
	// MCLK to Low
	rc = gpio_tlmm_config(cam_mclk_config[0], GPIO_ENABLE);
	if (rc != 0) {
		printk("mv9319: cam_mclk_config_0 error %d\n", rc);
		return;
	}
	gpio_configure(GPIO_CAM_MCLK, GPIOF_DRIVE_OUTPUT);
	gpio_set_value(GPIO_CAM_MCLK, 0);
	msleep(1);
#endif

	// Disable Power
	vreg_5m_dvdd = vreg_get(0, "rftx");
	vreg_disable(vreg_5m_dvdd);

	mv9319_set_vreg_5m(0);
	mv9319_set_vreg_avdd(0);

	vreg_cam_af = vreg_get(0, "rfrx2");
	vreg_disable(vreg_cam_af);
}
#endif /* FEATURE_NEW_POWER_ON_OFF_SEQUENCE */


static int mv9319_configure_pins(void)
{
	int rc;

	rc = gpio_request(GPIO_CAM_RESET, "mv9319_reset");
	if (rc != 0) {
		printk("mv9319: mv9319_reset gpio_request failed\n");
		return -EIO;
	}

	gpio_configure(GPIO_CAM_RESET, GPIOF_DRIVE_OUTPUT);

	rc = gpio_request(GPIO_CAM_MCLK, "mv9319_mclk");
	if (rc != 0) {
		printk("mv9319: mv9319_mclk gpio_request failed\n");
		gpio_free(GPIO_CAM_RESET);
		return -EIO;
	}

#if FEATURE_DEBUG_CAPTURE_TIMING
	LDBG("##### configure GPIO_RES_CHG_BB\n");
	rc = gpio_request(GPIO_RES_CHG_BB, "mv9319_debug_capture");
	if (rc != 0) {
		printk("mv9319: mv9319_debug_capture gpio_request failed\n");
		return -EIO;
	}

	gpio_configure(GPIO_RES_CHG_BB, GPIOF_DRIVE_OUTPUT);
	gpio_set_value(GPIO_RES_CHG_BB, 0);
#endif
	return 0;
}

int mv9319_is_fw_downloaded(void)
{
	int rc;

	struct mv9319_i2c_reg_conf const normal_pll_setup_tbl[] = {
		{0xD0, 0x20},
		{0x40, 0x60},	/* Output Divider Value: Input / 2^6 */
		{0x42, 0x04},	/* PLL Filter Range: 21~42MHz */
		{0x49, 0x1F},	/* Feedback Divider Value: Input / (31+1) */
		{0x47, 0x01},	/* PLL Change Strobe */
		{0x80, 0x03},	// Start Cmd
	};

	/* PLL Setup Start */
	rc = mv9319_i2c_write_byte_table(mv9319_client,
					 &normal_pll_setup_tbl[0],
					 ARRAY_SIZE(normal_pll_setup_tbl));

	if (rc < 0)
		return rc;
	/* PLL Setup End   */

	msleep(10); /* PLL Set time */

	mv9319_i2c_read_byte(mv9319_client, MV9319_CMD_VER_MAJOR, &firmware_ver_major);
	mv9319_i2c_read_byte(mv9319_client, MV9319_CMD_VER_MINOR, &firmware_ver_minor);

	LDBG("mv9319: Is firmware 0x%x.0x%x downloaded ? ", 
			MV9319_FIRMWARE_VER_MAJOR, MV9319_FIRMWARE_VER_MINOR);

	if ((firmware_ver_major != MV9319_FIRMWARE_VER_MAJOR)
	    || (firmware_ver_minor != MV9319_FIRMWARE_VER_MINOR)) {
		LDBG("NO. major=0x%x,minor=0x%x\n", 
				firmware_ver_major, firmware_ver_minor);
		return 0;
	}

	LDBG("YES. major=0x%x, minor=0x%x\n", firmware_ver_major, firmware_ver_minor);

	return 1;
}

static int mv9319_burn_firmware(void)
{
	int rc;
	unsigned char cmd_buf[2][3] = {
		{0x00, 0x00, 0x02},
		{0x00, 0x00, 0x50},
	};

	struct mv9319_i2c_reg_conf const pll_setup_tbl[] = {
		{0xD0, 0x20},
		{0x40, 0x60},
		{0x42, 0x04},
		{0x49, 0x1F},
		{0x47, 0x01},
		{0x80, 0x02},	// Start Cmd
	};

	printk("mv9319: burning firwmare....\n");

	/* PLL Setup Start */
	rc = mv9319_i2c_write_byte_table(mv9319_client,
					 &pll_setup_tbl[0],
					 ARRAY_SIZE(pll_setup_tbl));
	if (rc < 0)
		return rc;
	/* PLL Setup End   */

	msleep(10); /* PLL Set time */

	rc = mv9319_i2c_write_byte(mv9319_client, 0x5B, 0x01);
	if (rc < 0)
		return rc;

	rc = mv9319_i2c_write_buffer(mv9319_fw_client, cmd_buf[0], 3);
	if (rc < 0) 
		return rc;

	msleep(150);

	rc = mv9319_i2c_write_buffer(mv9319_fw_client, cmd_buf[1], 3);
	if (rc < 0)
		return rc;

	LDBG1("%s: ARRAY_SIZE(mv9319_img_buf) = %d\n",
	      __func__, ARRAY_SIZE(mv9319_img_buf));

	rc = mv9319_i2c_write_buffer(mv9319_fw_client,
				     (unsigned char *)mv9319_img_buf,
				     ARRAY_SIZE(mv9319_img_buf));
	if (rc < 0)
		return rc;

	rc = mv9319_i2c_write_byte(mv9319_client, 0x5B, 0x00);
	if (rc < 0)
		return rc;

	LDBG("mv9319: firmware buring end\n");

	return 0;
}

static int mv9319_burn_firmware_from_file(void)
{
	int rc = 0;
	unsigned char cmd_buf[2][3] = {
		{0x00, 0x00, 0x02},
		{0x00, 0x00, 0x50},
	};

	struct mv9319_i2c_reg_conf const pll_setup_tbl[] = {
		{0xD0, 0x20},
		{0x40, 0x60},
		{0x42, 0x04},
		{0x49, 0x1F},
		{0x47, 0x01},
		{0x80, 0x02},	// Start Cmd
	};

	int fd, i;
	unsigned count;
	unsigned char data;

	mm_segment_t old_fs = get_fs();
	set_fs(KERNEL_DS);

	printk("mv9319: burning firwmare from %s....\n", MV9319_FIRMWARE_FILE_PATH);

	fd = sys_open(MV9319_FIRMWARE_FILE_PATH, O_RDONLY, 0);
	if (fd < 0) {
		printk(KERN_WARNING "%s: Can not open %s[%d]\n",
			__func__, MV9319_FIRMWARE_FILE_PATH, fd);
		return -ENOENT;
	}

	count = (unsigned)sys_lseek(fd, (off_t)0, 2);
	if (count == 0) {
		rc = -EIO;
		sys_close(fd);
		set_fs(old_fs);
		return rc;
	}
	sys_lseek(fd, (off_t)0, 0);

	LDBG("mv9319: firmware file size %u\n", count);

	// XXX: MUST: count <= MV9319_ISP_BINARY_BUF_SIZE
	
	for (i = 0; i < count; i++) {
		if ((unsigned)sys_read(fd, (char *)&data, 1) != 1) {
			rc = -EIO;
			sys_close(fd);
			set_fs(old_fs);
			return rc;
		}
		mv9319_img_buf[i] = data;
		//LDBG1("%s: [%d] read data 0x%x\n", __func__, i, data);
	}
	printk("mv9319: success in reading firmware\n");

	sys_close(fd);
	set_fs(old_fs);

	/* PLL Setup Start */
	rc = mv9319_i2c_write_byte_table(mv9319_client,
					 &pll_setup_tbl[0],
					 ARRAY_SIZE(pll_setup_tbl));
	if (rc < 0)
		return rc;
	/* PLL Setup End   */

	msleep(10); /* PLL Set time */

	rc = mv9319_i2c_write_byte(mv9319_client, 0x5B, 0x01);
	if (rc < 0)
		return rc;

	rc = mv9319_i2c_write_buffer(mv9319_fw_client, cmd_buf[0], 3);
	if (rc < 0) 
		return rc;

	msleep(150);

	rc = mv9319_i2c_write_buffer(mv9319_fw_client, cmd_buf[1], 3);
	if (rc < 0) 
		return rc;

	printk("mv9319: firmware buring start\n");
	rc = mv9319_i2c_write_buffer(mv9319_fw_client,
			(unsigned char *)mv9319_img_buf, count);
	if (rc < 0)
		return rc;

	rc = mv9319_i2c_write_byte(mv9319_client, 0x5B, 0x00);
	if (rc < 0)
		return rc;

	printk("mv9319: firmware buring end\n");
	return rc;
}

static int mv9319_verify_firmware(void)
{
	int rc;
	unsigned char init_status = 0;
	unsigned retry = 0;
	unsigned char checksum_msb, checksum_lsb;

	struct mv9319_i2c_reg_conf const pll_setup_tbl[] = {
		{0xD0, 0x20},
		{0x40, 0x60},
		{0x42, 0x04},
		{0x49, 0x1F},
		{0x47, 0x01},
		{0x80, 0x03},	// Start Cmd
	};

#if FEATURE_NEW_POWER_ON_OFF_SEQUENCE
#else
	mv9319_power_off();
	msleep(10);
	mv9319_power_on();
#endif

	/* Enable binay checksum verification */
	rc = mv9319_i2c_write_byte(mv9319_client, 0x95, 0x00);
	if (rc < 0)
		return rc;

	/* PLL Setup Start */
	rc = mv9319_i2c_write_byte_table(mv9319_client,
					 &pll_setup_tbl[0],
					 ARRAY_SIZE(pll_setup_tbl));

	if (rc < 0)
		return rc;
	/* PLL Setup End   */

	msleep(10); /* PLL Set time */

	/* Wait for Initialization */
	do {
		rc = mv9319_i2c_read_byte(mv9319_client, 0x01, &init_status);
		if (rc < 0)
			return rc;

		LDBG1("%s: Checking Status 0x%x\n", __func__, init_status);
		retry++;
		msleep(10);
	} while ((init_status != 0x19) && (retry < 500));

	if (init_status != 0x19) {
		printk("%s: Failed 0x%x\n", __func__, init_status);
		return -1;
	}

	mv9319_i2c_read_byte(mv9319_client,
			     MV9319_CMD_FW_CHECKSUM_MSB, &checksum_msb);
	mv9319_i2c_read_byte(mv9319_client,
			     MV9319_CMD_FW_CHECKSUM_LSB, &checksum_lsb);

	if ((checksum_msb != MV9319_FIRMWARE_CHECKSUM_MSB)
	    || (checksum_lsb != MV9319_FIRMWARE_CHECKSUM_LSB)) {
		printk("mv9319: Veficiation Failed: 0x%x:msb=0x%x, 0x%x:lsb=0x%x\n",
				MV9319_FIRMWARE_CHECKSUM_MSB,
				checksum_msb,
				MV9319_FIRMWARE_CHECKSUM_LSB,
				checksum_lsb);
		return 1;
	}

	LDBG("Firmware checksum check succeed. msb=0x%x, lsb=0x%x\n",
	      checksum_msb, checksum_lsb);

	return 0;
}

static long mv9319_normal_boot(void)
{
	unsigned char init_status = 0;
	unsigned retry = 0;
	long rc;

	struct mv9319_i2c_reg_conf const normal_pll_setup_tbl[] = {
		{0xD0, 0x20},
		{0x40, 0x60},	/* Output Divider Value: Input / 2^6 */
		{0x42, 0x04},	/* PLL Filter Range: 21~42MHz */
		{0x49, 0x1F},	/* Feedback Divider Value: Input / (31+1) */
		{0x47, 0x01},	/* PLL Change Strobe */
		{0x80, 0x03},	// Start Cmd
	};

	LDBG("%s: start[%lu]\n", __func__, jiffies);

	/* Disable binay checksum verification */
	rc = mv9319_i2c_write_byte(mv9319_client, 0x95, 0xAA);
	if (rc < 0)
		return rc;

	/* PLL Setup Start */
	rc = mv9319_i2c_write_byte_table(mv9319_client,
					 &normal_pll_setup_tbl[0],
					 ARRAY_SIZE(normal_pll_setup_tbl));

	if (rc < 0)
		return rc;
	/* PLL Setup End   */

	msleep(10); /* PLL Set time */

	/* Wait for Initialization */
	do {
		rc = mv9319_i2c_read_byte(mv9319_client, 0x01, &init_status);
		if (rc < 0)
			return rc;

		LDBG1("%s: Checking Status 0x%x\n", __func__, init_status);
		retry++;
		msleep(10);
	} while ((init_status != 0x19) && (retry < 500));

	if (init_status != 0x19) {
		printk("%s: Failed 0x%x\n", __func__, init_status);
		return -1;
	}

	/* Change Image Orientation */
	rc = mv9319_i2c_write_byte(mv9319_client, MV9319_CMD_ORIENTATION, 0x00);
	if (rc < 0)
		return rc;

	msleep(MV9319_SET_DELAY_MSEC);

	/* Flickerless Control */
	rc = mv9319_i2c_write_byte(mv9319_client,
			MV9319_CMD_FLICKERLESS_CONTROL, flickerless_hz);
	if (rc < 0)
		return rc;

	msleep(MV9319_SET_DELAY_MSEC);

	LDBG("%s: Succeed[%lu]\n", __func__, jiffies);

	return 0;
}

static unsigned char current_luminance = 0;

static int mv9319_enable_flash_led_strobe(int is_pre_flash)
{
	long rc = 0;

	LDBG("%s: called [is_pre_flash:%d, af_enabled: %d]\n",
			__func__, is_pre_flash, mv9319_ctrl->af_enabled);

	if (mv9319_ctrl->flashmode == CAMERA_FLASH_AUTO) {
		if (is_pre_flash || !mv9319_ctrl->af_enabled) {
			rc = mv9319_i2c_read_byte(mv9319_client,
					MV9319_CMD_CURRENT_LUMINANCE,
					&current_luminance);
			if (rc < 0)
				return rc;
		}
		LDBG("mv9319: current luminance 0x%x\n", current_luminance);

		if (current_luminance < 2) { /* 50 Lux */ 
			if (is_pre_flash) {
				eve_flash_set_led_state(7);

#if 0 /* 2009/09/25 too dark */
				rc = mv9319_i2c_write_byte(mv9319_client,
						MV9319_CMD_AF_CONTROL, 0x02);
				if (rc < 0)
					return rc;
#endif
			}
			else
				eve_flash_set_led_state(0);
		}
	} else if (mv9319_ctrl->flashmode == CAMERA_FLASH_ON) {
		if (is_pre_flash || !mv9319_ctrl->af_enabled) {
			rc = mv9319_i2c_read_byte(mv9319_client,
					MV9319_CMD_CURRENT_LUMINANCE,
					&current_luminance);
			if (rc < 0)
				return rc;
			LDBG("mv9319: current luminance 0x%x\n", current_luminance);
		}

		if (is_pre_flash) {
			if (current_luminance < 2) {/* 50 Lux */ 
				eve_flash_set_led_state(7);
#if 0 /* 2009/09/25 too dark */
				rc = mv9319_i2c_write_byte(mv9319_client,
						MV9319_CMD_AF_CONTROL, 0x02);
				if (rc < 0)
					return rc;
#endif
			}
		} else
			eve_flash_set_led_state(0);
	}

	return 0;
}

static int32_t mv9319_suspend_auto_focus(void)
{
	long rc = 0;
	unsigned retry = 0;
	unsigned char data = 0;

	LDBG("%s: called\n", __func__);

	eve_flash_set_led_state(3);

	mv9319_ctrl->af_enabled = 0;

	// Suspend Focus
	rc = mv9319_i2c_write_byte(mv9319_client, MV9319_CMD_AF_STATUS, 0xCC);
	if (rc < 0)
		return rc;

	do {
		rc = mv9319_i2c_read_byte(mv9319_client, MV9319_CMD_AF_STATUS, &data);
		if (rc < 0)
			return rc;

		LDBG1("%s: Auto Focus status 0xCC ?= 0x%x\n", __func__,
		      data);

		retry++;
		msleep(10);
	} while ((data != 0xCC) && (retry < 500));

	return 0;
}

static int32_t mv9319_set_auto_focus(int macro_focus)
{
	long rc = 0;
	unsigned retry = 0;
	unsigned char data = 0;

	LDBG("%s: called, macro [%d]\n", __func__, macro_focus);

	rc = mv9319_enable_flash_led_strobe(1);
	if (rc < 0)
		return rc;

	mv9319_ctrl->af_enabled = 1;

	// Auto Focus
	rc = mv9319_i2c_write_byte(mv9319_client, MV9319_CMD_AF_STATUS, 0x00);
	if (rc < 0)
		return rc;

	do {
		rc = mv9319_i2c_read_byte(mv9319_client, MV9319_CMD_AF_STATUS, &data);
		if (rc < 0)
			return rc;

		LDBG1("%s: Auto Focus status 0x%x\n", __func__, data);

		retry++;
		msleep(10);
	} while ((data != 0x00) && (retry < 500));

	if (macro_focus) {
		rc = mv9319_i2c_write_byte(mv9319_client, MV9319_CMD_AF_CONTROL, 0x01);
		if (rc < 0)
			return rc;
	} else {
		rc = mv9319_i2c_write_byte(mv9319_client, MV9319_CMD_AF_CONTROL, 0x00);
		if (rc < 0)
			return rc;
	}

	retry = 0;
	do {
		rc = mv9319_i2c_read_byte(mv9319_client, MV9319_CMD_AF_STATUS, &data);
		if (rc < 0)
			return rc;

		LDBG1("%s: Auto Focus Change Finish Check 0x%x\n", __func__,
		      data);

		retry++;
		msleep(10);

		if (data == 0xBB)  {
			LDBG("mv9319: good focus\n");
			eve_flash_set_led_state(3);
			return 0;
		}
		else if (data == 0xCB) {
			LDBG("mv9319: auto focus failed\n");
			eve_flash_set_led_state(3);
			return 1;
		}
	} while (retry < 500);

	return 1;
}

static int mv9319_check_resolution_change(enum sensor_mode_t mode)
{
	long rc = 0;
	unsigned char data = 0;
	int retry = 0;

	do {
		rc = mv9319_i2c_read_byte(mv9319_client, 0x2F, &data);
		if (rc < 0)
			return rc;

		LDBG1("%s: Resolution Change Finish Check 0x%x\n", __func__,
		      data);

		retry++;
		msleep(10);
	} while ((data != 0xBB) && (retry < 500));
	if (data != 0xBB) {
		printk("%s: resolution change was not finished\n", __func__);
		return -1;
	} else {
		LDBG("mv9319: snapshot succeed\n");
	}

#if FEATURE_DEBUG_CAPTURE_TIMING
	LDBG("##### set GPIO_RES_CHG_BB to HIGH\n");
	gpio_set_value(GPIO_RES_CHG_BB, 1);
	mdelay(10);
	LDBG("##### set GPIO_RES_CHG_BB to LOW\n");
	gpio_set_value(GPIO_RES_CHG_BB, 0);
#endif

	LDBG("%s: current fps 0x%x\n", __func__, mv9319_ctrl->fps_control);
	rc = mv9319_i2c_write_byte(
			mv9319_client, MV9319_CMD_FPS, mv9319_ctrl->fps_control);
	if (rc < 0)
		return rc;

	/* Turn flash off */
#if 0
	if (mv9319_ctrl->flashmode == CAMERA_FLASH_AUTO ||
			mv9319_ctrl->flashmode == CAMERA_FLASH_ON)
		eve_flash_set_led_state(1);
#endif

	mv9319_ctrl->sensormode = mode;

	//mdelay(700); // XXX: For TEST

	return 0;
}

static int32_t mv9319_video_config(enum sensor_mode_t mode,
				   enum sensor_resolution_t res)
{
	long rc;
	unsigned char image_format_cmd;

	LDBG("%s: called, resolution: %d\n", __func__, res);

	/* We uses only VGA resolution. VFE will crop for QVGA */
#if 0
	switch (res) {
	case SENSOR_QVGA_SIZE:
		break;
	case SENSOR_VGA_SIZE:
		image_format_cmd = MV9319_F_VGA;
		break;
	case SENSOR_QUADVGA_SIZE:
	case SENSOR_UXGA_SIZE:
	case SENSOR_QXGA_SIZE:
	case SENSOR_QSXGA_SIZE:
		break;
	default:
		printk("mv9319: wrong capture image size, set VGA\n");
		image_format_cmd = MV9319_F_VGA;
	}
#endif
	image_format_cmd = MV9319_F_VGA;

	rc = mv9319_i2c_write_byte(mv9319_client,
				   MV9319_CMD_STATUS_SET_FORMAT, 0x00);
	if (rc < 0)
		return rc;

	LDBG("mv9319: image format cmd 0x%x\n", image_format_cmd);

	rc = mv9319_i2c_write_byte(mv9319_client,
				   MV9319_CMD_FORMAT, image_format_cmd);
	if (rc < 0)
		return rc;

	return mv9319_check_resolution_change(mode);
}

static int32_t mv9319_snapshot_config(enum sensor_mode_t mode,
		enum sensor_resolution_t res)
{
	long rc;
	unsigned char image_format_cmd;

	LDBG("%s: called, Input resolution[%d]\n", __func__, res);

	switch (res) {
		case SENSOR_QVGA_SIZE:
			image_format_cmd = MV9319_F_QVGA
				| MV9319_SET_CAPTURE_MODE;
			break;
		case SENSOR_VGA_SIZE:
			image_format_cmd = MV9319_F_VGA
				| MV9319_SET_CAPTURE_MODE;
			break;
		case SENSOR_QUADVGA_SIZE:
			image_format_cmd = MV9319_F_QUADVGA
				| MV9319_SET_CAPTURE_MODE;
			break;
		case SENSOR_UXGA_SIZE:
			image_format_cmd = MV9319_F_UXGA
				| MV9319_SET_CAPTURE_MODE;
			break;
		default:
			printk("mv9319: wrong capture image size\n");
			return -EINVAL;
	}

	/* Image Size Change & Mode Selection */
	rc = mv9319_i2c_write_byte(mv9319_client, 
			MV9319_CMD_STATUS_SET_FORMAT, 0x00);
	if (rc < 0)
		return rc;

	LDBG("mv9319: image format cmd 0x%x\n", image_format_cmd);

	rc = mv9319_enable_flash_led_strobe(0);
	if (rc < 0)
		return rc;

	rc = mv9319_i2c_write_byte(mv9319_client,
			MV9319_CMD_FORMAT, image_format_cmd);
	if (rc < 0)
		return rc;

	return mv9319_check_resolution_change(mode);
}

static long mv9319_set_jpg_control(void)
{
	long rc;

#if CAMERA_DEBUG
	unsigned char dbg_temp = 0;
#endif
	rc = mv9319_i2c_write_byte(mv9319_client,
				   MV9319_CMD_JPG_Q_FACTOR,
				   mv9319_ctrl->qfactor);
	if (rc < 0)
		return rc;

	msleep(MV9319_SET_DELAY_MSEC);

#if CAMERA_DEBUG
	rc = mv9319_i2c_read_byte(mv9319_client,
			MV9319_CMD_JPG_Q_FACTOR, &dbg_temp);
	if (rc < 0)
		return rc;
	LDBG("##### %s: QFactor 0x%x\n", __func__, dbg_temp);
#endif

	rc = mv9319_i2c_write_byte(mv9319_client,
				   MV9319_CMD_JPG_BUFFER_SIZE, 
				   mv9319_ctrl->jpg_buf_size);
	if (rc < 0)
		return rc;

	msleep(MV9319_SET_DELAY_MSEC);

#if CAMERA_DEBUG
	dbg_temp = 0;
	rc = mv9319_i2c_read_byte(mv9319_client,
				  MV9319_CMD_JPG_BUFFER_SIZE, &dbg_temp);
	if (rc < 0)
		return rc;
	LDBG("##### %s: JPEG Buffer size 0x%x\n", __func__, dbg_temp);
#endif

	return rc;
}

static int32_t mv9319_raw_snapshot_config(enum sensor_mode_t mode,
					  enum sensor_resolution_t res)
{

	long rc = 0;
	unsigned char image_format_cmd;

	LDBG("%s: called, Input resolution[%d]\n", __func__, res);

#if FEATURE_MIMIC_ISP_JPG_SIZE
	mv9319_ctrl->res = res;
#endif

	switch (res) {
		case SENSOR_QUADVGA_SIZE:
			image_format_cmd = MV9319_F_QUADVGA
				| MV9319_SET_CAPTURE_MODE | MV9319_SET_FORMAT_JPG ;
			break;
		case SENSOR_UXGA_SIZE:
			image_format_cmd = MV9319_F_UXGA
				| MV9319_SET_CAPTURE_MODE | MV9319_SET_FORMAT_JPG ;
			break;
		case SENSOR_QXGA_SIZE:
			image_format_cmd = MV9319_F_QXGA
				| MV9319_SET_CAPTURE_MODE | MV9319_SET_FORMAT_JPG ;
			break;
		case SENSOR_QSXGA_SIZE:
			image_format_cmd = MV9319_F_QSXGA
				| MV9319_SET_CAPTURE_MODE | MV9319_SET_FORMAT_JPG ;
			break;
		default:
			printk("mv9319: wrong capture image size\n");
			return -EINVAL;
	}

	rc = mv9319_set_jpg_control();
	if (rc < 0)
		return rc;

	rc = mv9319_i2c_write_byte(mv9319_client,
				   MV9319_CMD_STATUS_SET_FORMAT, 0x00);
	if (rc < 0)
		return rc;

	LDBG("mv9319: image format cmd 0x%x\n", image_format_cmd);

	rc = mv9319_enable_flash_led_strobe(0);
	if (rc < 0)
		return rc;

	rc = mv9319_i2c_write_byte(mv9319_client,
				   MV9319_CMD_FORMAT, image_format_cmd);
	if (rc < 0)
		return rc;

	return mv9319_check_resolution_change(mode);
}

static int mv9319_probe_init_done(struct msm_camera_sensor_info *data)
{
	LDBG("%s: called\n", __func__);
	gpio_direction_output(GPIO_CAM_RESET, 0);

#if FEATURE_DEBUG_CAPTURE_TIMING
	LDBG("##### set GPIO_RES_CHG_BB to output low\n");
	gpio_direction_output(GPIO_RES_CHG_BB, 0);
#endif

	return 0;
}

/* 1. Configure pins
 * 2. Reset and read sensor ID */
static int mv9319_probe_init_sensor(struct msm_camera_sensor_info *data)
{
	int32_t rc;
	unsigned char chipid;

	LDBG("%s: called[%lu]\n", __func__, jiffies);

	if (mv9319_configure_pins() != 0) {
		printk("mv9319_configure_pins failed failed\n");
		rc = -EIO;
		goto init_fail;
	}

#if FEATURE_NEW_POWER_ON_OFF_SEQUENCE
	mv9319_power_on_2();
#else
	mv9319_power_on();
#endif

#if 1
	/* 3. Read sensor Model ID: */
	rc = mv9319_i2c_read_byte(mv9319_client, MV9319_PID_REG, &chipid);
	if (rc < 0) {
		printk("mv9319: fail to read Model ID\n");
		rc = -EIO;
		goto init_fail;
	}

	LDBG("mv9319 model_id:product_id = 0x%x", chipid);

	rc = mv9319_i2c_read_byte(mv9319_client, MV9319_VER_REG, &chipid);
	if (rc < 0) {
		printk("mv9319: fail to read Product ID\n");
		rc = -EIO;
		goto init_fail;
	}
	LDBG(":0x%x\n", chipid);
#endif

	if (unlikely(flag_update_firmware)) {
		LDBG1("%s: fw check start[%lu]\n", __func__, jiffies);
		switch (flag_update_firmware) {
			case 1:
				if (!mv9319_is_fw_downloaded()) {
					if (mv9319_burn_firmware()) {
						printk("mv9319: fail to burn firmware\n");
						rc = -EIO;
						goto init_fail;
					}
				}
				break;
			case 2:
				if (mv9319_burn_firmware_from_file()) {
					printk("mv9319: fail to burn firmware\n");
					rc = -EIO;
					goto init_fail;
				}
				mv9319_is_fw_downloaded();
				break;
			default:
				printk("mv9319: wrong firmware update flag\n");
		}
		if (mv9319_verify_firmware() == 1) {
			printk("mv9391: binary checksum failed. Re-burning...\n");
			if (mv9319_burn_firmware()) {
				printk("mv9319: fail to burn firmware\n");
				rc = -EIO;
				goto init_fail;
			}
			if (mv9319_verify_firmware()) {
				rc = -EIO;
				goto init_fail;
			}
		}
		LDBG1("%s: fw check end[%lu]\n", __func__, jiffies);
	} else {
		bd6083_set_camera_on_mode(); // To disable ALC mode
	}

	if (rc >= 0)
		goto init_done;

init_fail:
	return rc;
init_done:
	LDBG("%s: init success [%lu]\n", __func__, jiffies);
	return rc;
}

static int mv9319_sensor_open_init(struct msm_camera_sensor_info *data)
{
	int32_t rc;

	LDBG("%s: called[%lu]\n", __func__, jiffies);

	mv9319_ctrl = kzalloc(sizeof(struct mv9319_ctrl_t), GFP_KERNEL);
	if (!mv9319_ctrl) {
		printk("mv9319_init failed!\n");
		rc = -ENOMEM;
		goto init_done;
	}

	if (data)
		mv9319_ctrl->sensordata = data;

	mv9319_ctrl->qfactor = 0x50;
	mv9319_ctrl->jpg_buf_size = 30;
	mv9319_ctrl->fps_control = MV9319_FPS_AUTO;
	mv9319_ctrl->af_enabled = 0;

	/* enable mclk first */
	msm_camio_clk_rate_set(MV9319_DEFAULT_CLOCK_RATE);
	msleep(5); // XXX: 5?

	msm_camio_camif_pad_reg_reset();
	//msleep(20); // XXX: redundant?

	rc = mv9319_probe_init_sensor(data);
	if (rc < 0)
		goto init_fail1;

	if (rc >= 0) 
		rc = mv9319_normal_boot();
	else
		goto init_fail1;


	LDBG("mv9319_open: normal_boot rc = %d\n", rc);

	if (rc >= 0)
		goto init_done;

init_fail1:
	mv9319_probe_init_done(data);
	kfree(mv9319_ctrl);
init_done:
	LDBG("%s: end[%lu]\n", __func__, jiffies);
	return rc;
}

static int mv9319_init_client(struct i2c_client *client)
{
	LDBG("%s: called\n", __func__);

	/* Initialize the MSM_CAMI2C Chip */
	init_waitqueue_head(&mv9319_wait_queue);
	return 0;
}

static int32_t mv9319_set_sensor_mode(enum sensor_mode_t mode,
				      enum sensor_resolution_t res)
{
	int32_t rc = 0;

	LDBG("%s: called\n", __func__);
	switch (mode) {
	case SENSOR_PREVIEW_MODE:
		rc = mv9319_video_config(mode, res);
		break;

	case SENSOR_SNAPSHOT_MODE:
		rc = mv9319_snapshot_config(mode, res);
		break;

	case SENSOR_RAW_SNAPSHOT_MODE:
		rc = mv9319_raw_snapshot_config(mode, res);
		break;

	default:
		rc = -EINVAL;
		break;
	}

	msleep(MV9319_SET_DELAY_MSEC);

	return rc;
}

static long mv9319_set_effect(enum sensor_mode_t mode, int8_t effect)
{
	long rc = 0;

	LDBG("%s: called, %d\n", __func__, effect);

	switch ((enum camera_effect_t)effect) {
	case CAMERA_EFFECT_OFF:
		rc = mv9319_i2c_write_byte(mv9319_client,
					   MV9319_CMD_EFFECT_CONTORL,
					   MV9319_EFFECT_NORMAL);
		if (rc < 0)
			return rc;
		break;
	case CAMERA_EFFECT_MONO:
		rc = mv9319_i2c_write_byte(mv9319_client,
					   MV9319_CMD_EFFECT_CONTORL,
					   MV9319_EFFECT_BNW);
		if (rc < 0)
			return rc;
		break;
	case CAMERA_EFFECT_NEGATIVE:
		rc = mv9319_i2c_write_byte(mv9319_client,
					   MV9319_CMD_EFFECT_CONTORL,
					   MV9319_EFFECT_NNP);
		if (rc < 0)
			return rc;

		break;
	case CAMERA_EFFECT_SOLARIZE:
		rc = mv9319_i2c_write_byte(mv9319_client,
					   MV9319_CMD_EFFECT_CONTORL,
					   MV9319_EFFECT_SOLARIZE);
		if (rc < 0)
			return rc;
		break;
	case CAMERA_EFFECT_SEPIA:
		rc = mv9319_i2c_write_byte(mv9319_client,
					   MV9319_CMD_EFFECT_CONTORL,
					   MV9319_EFFECT_SEPIA);
		if (rc < 0)
			return rc;

		break;
	case CAMERA_EFFECT_AQUA:
		rc = mv9319_i2c_write_byte(mv9319_client,
					   MV9319_CMD_EFFECT_CONTORL,
					   MV9319_EFFECT_AQUA);
		if (rc < 0)
			return rc;

		break;
	case CAMERA_EFFECT_SKETCH:
		rc = mv9319_i2c_write_byte(mv9319_client,
					   MV9319_CMD_EFFECT_CONTORL,
					   MV9319_EFFECT_SKETCH);
		if (rc < 0)
			return rc;

		break;
	case CAMERA_EFFECT_EMBOSS:
		rc = mv9319_i2c_write_byte(mv9319_client,
					   MV9319_CMD_EFFECT_CONTORL,
					   MV9319_EFFECT_EMBOSS);
		if (rc < 0)
			return rc;

		break;
	case CAMERA_EFFECT_RED:
		rc = mv9319_i2c_write_byte(mv9319_client,
					   MV9319_CMD_EFFECT_CONTORL,
					   MV9319_EFFECT_RED);
		if (rc < 0)
			return rc;

		break;
	case CAMERA_EFFECT_GREEN:
		rc = mv9319_i2c_write_byte(mv9319_client,
					   MV9319_CMD_EFFECT_CONTORL,
					   MV9319_EFFECT_GREEN);
		if (rc < 0)
			return rc;

		break;
	default:
		return -EFAULT;
	}

	mv9319_ctrl->effect = effect;

	msleep(MV9319_SET_DELAY_MSEC);
	msleep(10);

	return rc;
}

static long mv9319_set_wb(int8_t wb)
{
	long rc;
	unsigned char new_wb;
	LDBG("%s: called, new wb: %d\n", __func__, wb);

	switch ((enum camera_wb_t)wb) {
	case CAMERA_WB_AUTO:
		new_wb = MV9319_WB_AUTO_START;
		break;
	case CAMERA_WB_INCANDESCENT:
		new_wb = MV9319_WB_INCANDESCENT;
		break;
	case CAMERA_WB_FLUORESCENT:
		new_wb = MV9319_WB_FLUORESCENT;
		break;
	case CAMERA_WB_DAYLIGHT:
		new_wb = MV9319_WB_DAYLIGHT;
		break;
	case CAMERA_WB_CLOUDY_DAYLIGHT:
		new_wb = MV9319_WB_CLOUDY;
		break;
	default:
		printk("mv9319: wrong white balance value\n");
		return -EFAULT;
	}

	rc = mv9319_i2c_write_byte(mv9319_client,
				   MV9319_CMD_WB, MV9319_WB_AUTO_STOP);

	if (rc < 0)
		return rc;

	rc = mv9319_i2c_write_byte(mv9319_client, MV9319_CMD_WB, new_wb);

	mv9319_ctrl->wb = wb;
	
	msleep(MV9319_SET_DELAY_MSEC);

	return rc;
}

static long mv9319_set_iso(int8_t iso)
{
	long rc;
	unsigned char new_iso;
	LDBG("%s: called: new iso %d\n", __func__, iso);

	switch ((enum camera_iso_t)iso) {
	case CAMERA_ISO_AUTO:
		new_iso = MV9319_ISO_AUTO;
		break;
	case CAMERA_ISO_100:
		new_iso = MV9319_ISO_100;
		break;
	case CAMERA_ISO_200:
		new_iso = MV9319_ISO_200;
		break;
	case CAMERA_ISO_400:
		new_iso = MV9319_ISO_400;
		break;
	default:
		printk("mv9319: wrong iso value\n");
		return -EINVAL;
	}

	rc = mv9319_i2c_write_byte(mv9319_client, MV9319_CMD_ISO, new_iso);

	msleep(MV9319_SET_DELAY_MSEC);

	return rc;
}

static long mv9319_set_image_quality(int8_t quality)
{
	LDBG("%s: called: quality %u\n", __func__, quality);

	switch (quality) {
	case CAMERA_IMG_QUALITY_NORMAL:
		mv9319_ctrl->qfactor = 0xb0;
		break;
	case CAMERA_IMG_QUALITY_FINE:
		mv9319_ctrl->qfactor = 0x90;
		break;
	case CAMERA_IMG_QUALITY_SUPER_FINE:
		mv9319_ctrl->qfactor = 0x70;
		break;
	default:
		mv9319_ctrl->qfactor = 0x70;
		printk("mv9319: wrong jpeg quality value. set the normal\n");
		//return -EINVAL;
	}

	if (mv9319_ctrl->effect == CAMERA_EFFECT_SKETCH)
		mv9319_ctrl->qfactor += 0x20;

#if FEATURE_MIMIC_ISP_JPG_SIZE
	mv9319_ctrl->img_quality = quality;
#endif

	/* XXX: We use static JPG buffer size, unit: 100KB */
	mv9319_ctrl->jpg_buf_size = 30;

	return 0;
}

static long mv9319_set_scene_mode(int8_t mode)
{
	long rc;
	unsigned char new_scene_mode;

	LDBG("%s: called: scene mode %u\n", __func__, mode);

	switch ((enum camera_scene_mode_t)mode) {
	case CAMERA_SCENE_NORMAL:
		new_scene_mode = MV9319_SCENE_NORMAL;
		break;
	case CAMERA_SCENE_NIGHT:
		new_scene_mode = MV9319_SCENE_NIGHT;
		break;
	case CAMERA_SCENE_BACKLIGHT:
		new_scene_mode = MV9319_SCENE_BACKLIGHT;
		break;
	case CAMERA_SCENE_LANDSCAPE:
		new_scene_mode = MV9319_SCENE_LANDSCAPE;
		break;
	case CAMERA_SCENE_PORTRAIT:
		new_scene_mode = MV9319_SCENE_PORTRAIT;
		break;
	case CAMERA_SCENE_NIGHT_PORTRAIT:
		new_scene_mode = MV9319_SCENE_NIGHT_PORTRAIT;
		break;
	case CAMERA_SCENE_BEACH:
		new_scene_mode = MV9319_SCENE_BEACH;
		break;
	case CAMERA_SCENE_PARTY:
		new_scene_mode = MV9319_SCENE_PARTY;
		break;
	case CAMERA_SCENE_SPORT:
		new_scene_mode = MV9319_SCENE_SPORT;
		break;
	default:
		printk("mv9319: wrong scene mode value, set to the normal\n");
		new_scene_mode = MV9319_SCENE_NORMAL;
	}

	rc = mv9319_i2c_write_byte(mv9319_client,
				   MV9319_CMD_SCENE_MODE, new_scene_mode);

	mv9319_ctrl->scene = mode;

	msleep(MV9319_SET_DELAY_MSEC);

	return rc;
}

static long mv9319_set_exposure_value(int8_t ev)
{
	long rc;
	unsigned char new_ev;

	LDBG("%s: called: ev %u\n", __func__, ev);

	switch (ev) {
	case 0:
		new_ev = MV9319_EV_N_6;
		break;
	case 1:
		new_ev = MV9319_EV_N_5;
		break;
	case 2:
		new_ev = MV9319_EV_N_4;
		break;
	case 3:
		new_ev = MV9319_EV_N_3;
		break;
	case 4:
		new_ev = MV9319_EV_N_2;
		break;
	case 5:
		new_ev = MV9319_EV_N_1;
		break;
	case 6:
		new_ev = MV9319_EV_DEFAULT;
		break;
	case 7:
		new_ev = MV9319_EV_P_1;
		break;
	case 8:
		new_ev = MV9319_EV_P_2;
		break;
	case 9:
		new_ev = MV9319_EV_P_3;
		break;
	case 10:
		new_ev = MV9319_EV_P_4;
		break;
	case 11:
		new_ev = MV9319_EV_P_5;
		break;
	case 12:
		new_ev = MV9319_EV_P_6;
		break;
	default:
		printk("mv9319: wrong ev value, set to the default\n");
		new_ev = MV9319_EV_DEFAULT;
	}

	rc = mv9319_i2c_write_byte(mv9319_client, MV9319_CMD_EV, new_ev);

	mv9319_ctrl->brightness = ev;

	msleep(MV9319_SET_DELAY_MSEC);

	return rc;
}

static long mv9319_set_zoom(int8_t zoom)
{
	long rc;

	LDBG("%s: called: zoom %u\n", __func__, zoom);

	if (zoom < 0 || zoom > 0xA) {
		printk("mv9319: wrong zoom value\n");
		return -EINVAL;
	}

	rc = mv9319_i2c_write_byte(mv9319_client, MV9319_CMD_ZOOM, zoom);
	if (rc < 0)
		return rc;

	mv9319_ctrl->zoom = zoom;
	
	msleep(MV9319_SET_DELAY_MSEC);

	return rc;
}

static long mv9319_set_flash_mode(int8_t mode)
{
	LDBG("%s: called: flash mode %u\n", __func__, mode);

	if (mode < 0 || mode >= CAMERA_FLASH_MAX) {
		printk("mv9319: wrong flash mode\n");
		return -EINVAL;
	}

	/* This controls only flash in Camera mode, not Camcorder 
	 * Camcorder uses sysfs interface to control flash.*/
	mv9319_ctrl->flashmode = mode;

	return 0;
}

static int32_t mv9319_move_focus(enum sensor_move_focus_t direction,
		int32_t num_steps)
{
	long rc = 0;

	LDBG("%s: called, step %d\n", __func__, num_steps);

	rc = mv9319_i2c_write_byte(mv9319_client,
				  MV9319_CMD_MF_CONTROL, num_steps);
	if (rc < 0)
		return rc;

	msleep(MV9319_SET_DELAY_MSEC);

	return 0;
}

static long mv9319_set_flicker(int8_t hz)
{
	long rc;

	LDBG("%s: called: flicker HZ %u\n", __func__, hz);
	switch (hz) {
		case 50:
			flickerless_hz = MV9319_FLICKERLESS_50HZ;
			break;
		case 60:
			flickerless_hz = MV9319_FLICKERLESS_60HZ;
			break;
		default:
			printk("mv9319: invalid input\n");
			return 0;
	}

	/* Flickerless Control */
	rc = mv9319_i2c_write_byte(mv9319_client,
			MV9319_CMD_FLICKERLESS_CONTROL, flickerless_hz);
	if (rc < 0)
		return rc;

	msleep(MV9319_SET_DELAY_MSEC);

	return 0;
}

static long mv9319_set_recording_mode(int8_t is_recording_mode)
{
	long rc;

	LDBG("%s: called: is recording mode? %u\n", __func__, is_recording_mode);

	if (is_recording_mode) {
		rc = mv9319_i2c_write_byte(
				mv9319_client, MV9319_CMD_FPS, MV9319_FPS_30);
		if (rc < 0)
			return rc;

		mv9319_ctrl->fps_control = MV9319_FPS_30;
	} else {
		rc = mv9319_i2c_write_byte(
				mv9319_client, MV9319_CMD_FPS, MV9319_FPS_AUTO);
		if (rc < 0)
			return rc;

		mv9319_ctrl->fps_control = MV9319_FPS_AUTO;
	}

	msleep(MV9319_SET_DELAY_MSEC);

	return 0;
}

static uint32_t mv9319_get_jpg_image_size(void)
{
	uint32_t isp_jpg_size = 0;

	LDBG("%s: called\n", __func__);
#if FEATURE_MIMIC_ISP_JPG_SIZE
	LDBG("%s: res %d, current quality %d\n", __func__,
			mv9319_ctrl->res,
			mv9319_ctrl->img_quality);

	switch (mv9319_ctrl->res) {
		case SENSOR_QUADVGA_SIZE:
			isp_jpg_size = 700000;
			break;
		case SENSOR_UXGA_SIZE:
			isp_jpg_size = 800000;
			break;
		case SENSOR_QXGA_SIZE:
			isp_jpg_size = 900000;
			break;
		case SENSOR_QSXGA_SIZE:
			isp_jpg_size = 1000000;
			break;
		default:
			printk("mv9319: wrong capture image size\n");
			return -EINVAL;
	}

	LDBG("%s: jpg size %u\n", __func__, isp_jpg_size);

	switch (mv9319_ctrl->img_quality) {
		case CAMERA_IMG_QUALITY_NORMAL:
			isp_jpg_size -= (50000 * 2);
			break;
		case CAMERA_IMG_QUALITY_FINE:
			isp_jpg_size -= (50000 * 1);
			break;
		case CAMERA_IMG_QUALITY_SUPER_FINE:
			break;
	}
#endif

	LDBG("%s: jpg size %u\n", __func__, isp_jpg_size);
	return isp_jpg_size;
}

int mv9319_sensor_config(void __user * argp)
{
	struct sensor_cfg_data_t cdata;
	long rc = 0;

	LDBG1("%s: called [%lu]\n", __func__, jiffies);

	if (copy_from_user(&cdata,
			   (void *)argp, sizeof(struct sensor_cfg_data_t)))
		return -EFAULT;

	down(&mv9319_sem);

	LDBG1("%s: cfgtype=%d\n", __func__, cdata.cfgtype);

	switch (cdata.cfgtype) {
	case CFG_SET_MODE:
		rc = mv9319_set_sensor_mode(cdata.mode, cdata.rs);
		break;

	case CFG_SET_DEFAULT_FOCUS:
		if (cdata.cfg.focus.steps == 5)
			cdata.cfg.focus.steps = mv9319_suspend_auto_focus();
		else {
			cdata.cfg.focus.steps = mv9319_set_auto_focus(cdata.cfg.focus.dir);
		}
		if (copy_to_user
		    ((void *)argp, &cdata, sizeof(struct sensor_cfg_data_t)))
			rc = -EFAULT;
		break;
	case CFG_MOVE_FOCUS:
		rc = mv9319_move_focus(
				cdata.cfg.focus.dir,
				cdata.cfg.focus.steps);
		break;

	case CFG_SET_EFFECT:
		rc = mv9319_set_effect(cdata.mode, cdata.cfg.effect);
		break;
	case CFG_SET_WB:
		rc = mv9319_set_wb(cdata.cfg.wb);
		break;
	case CFG_SET_ISO:
		rc = mv9319_set_iso(cdata.cfg.iso);
		break;
	case CFG_SET_IMG_QUALITY:
		rc = mv9319_set_image_quality(cdata.cfg.img_quality);
		break;
	case CFG_SET_SCENE_MODE:
		rc = mv9319_set_scene_mode(cdata.cfg.scene_mode);
		break;
	case CFG_SET_EXPOSURE_VALUE:
		rc = mv9319_set_exposure_value(cdata.cfg.ev);
		break;
	case CFG_SET_ZOOM:
		rc = mv9319_set_zoom(cdata.cfg.zoom);
		break;
	case CFG_SET_FLASH_MODE:
		rc = mv9319_set_flash_mode(cdata.cfg.flash);
		break;
	case CFG_GET_JPG_IMAGE_SIZE:
		cdata.cfg.jpg_size = mv9319_get_jpg_image_size();
		if (copy_to_user
		    ((void *)argp, &cdata, sizeof(struct sensor_cfg_data_t)))
			rc = -EFAULT;
		break;
	case CFG_SET_FLICKER:
		rc = mv9319_set_flicker(cdata.cfg.flicker);
		break;
	case CFG_SET_RECORDING_MODE:
		rc = mv9319_set_recording_mode(cdata.cfg.is_recording_mode);
		break;
	default:
		rc = -EFAULT;
		break;
	}
	
	if (rc < 0)
		printk("mv9319: ERROR in sensor_config, %ld\n", rc);

	LDBG1("%s: end [%lu]\n", __func__, jiffies);
	up(&mv9319_sem);
	return rc;
}

static ssize_t mv9319_brightness_show(struct device* dev, struct device_attribute* attr, char* buf)
{
	unsigned char val;

	if (mv9319_ctrl == NULL)
		return sprintf(buf,"%x\n", 0); 

	val = mv9319_ctrl->brightness;
	return sprintf(buf,"%x\n", val);

}
static ssize_t mv9319_brightness_store(struct device* dev, struct device_attribute* attr, const char* buf, size_t n)
{
	int val;
	long rc;

	if (mv9319_ctrl == NULL)
		return 0;

	sscanf(buf,"%d",&val);

	rc = mv9319_set_exposure_value(val);
	if (rc < 0)
		printk("mv9319: failed to set brightness\n");

	return n;
}
static DEVICE_ATTR(brightness, S_IRUGO|S_IWUGO, mv9319_brightness_show, mv9319_brightness_store);

static ssize_t mv9319_flash_show(struct device* dev, struct device_attribute* attr, char* buf)
{
	unsigned char val;

	if (mv9319_ctrl == NULL)
		return sprintf(buf,"%x\n", 0); 

	val = mv9319_ctrl->video_flash;
	return sprintf(buf,"%d\n", val);

}
static ssize_t mv9319_flash_store(struct device* dev, struct device_attribute* attr, const char* buf, size_t n)
{
	int val;

	if (mv9319_ctrl == NULL)
		return 0;

	sscanf(buf,"%x",&val);

	if (val)
		eve_flash_set_led_state(2);
	else
		eve_flash_set_led_state(3);

	mv9319_ctrl->video_flash = val;

	return n;
}
static DEVICE_ATTR(flash, S_IRUGO|S_IWUGO, mv9319_flash_show, mv9319_flash_store);

static ssize_t mv9319_scene_show(struct device* dev, struct device_attribute* attr, char* buf)
{
	unsigned char val;

	if (mv9319_ctrl == NULL)
		return sprintf(buf,"%x\n", 0); 

	val = mv9319_ctrl->scene;
	return sprintf(buf,"%x\n", val);

}
static ssize_t mv9319_scene_store(struct device* dev, struct device_attribute* attr, const char* buf, size_t n)
{
	int val;
	long rc;

	if (mv9319_ctrl == NULL)
		return 0;

	sscanf(buf,"%x",&val);

	rc = mv9319_set_scene_mode(val);

	switch (val) {
		case CAMERA_SCENE_NIGHT:
		case CAMERA_SCENE_NIGHT_PORTRAIT:
			mv9319_set_recording_mode(0);
			break;
		default:
			mv9319_set_recording_mode(1);
	}
	if (rc < 0)
		printk("mv9319: failed to set scene\n");

	return n;
}
static DEVICE_ATTR(scene, S_IRUGO|S_IWUGO, mv9319_scene_show, mv9319_scene_store);

static ssize_t mv9319_zoom_show(struct device* dev, struct device_attribute* attr, char* buf)
{
	unsigned char val;

	if (mv9319_ctrl == NULL)
		return sprintf(buf,"%x\n", 0); 

	val = mv9319_ctrl->zoom;
	return sprintf(buf,"%x\n", val);

}
static ssize_t mv9319_zoom_store(struct device* dev, struct device_attribute* attr, const char* buf, size_t n)
{
	int val;
	long rc;

	if (mv9319_ctrl == NULL)
		return 0;

	sscanf(buf,"%x",&val);

	if(val < 0x0 || val > 0xA) {
		printk("mv9319: invalid zoom input\n");
		return 0;
	}

	rc = mv9319_set_zoom(val);
	if (rc < 0)
		printk("mv9319: failed to set zoom\n");

	return n;
}
static DEVICE_ATTR(zoom, S_IRUGO|S_IWUGO, mv9319_zoom_show, mv9319_zoom_store);

static ssize_t mv9319_wb_show(struct device* dev, struct device_attribute* attr, char* buf)
{
	unsigned char val;

	if (mv9319_ctrl == NULL)
		return sprintf(buf,"%x\n", 0); 

	val = mv9319_ctrl->wb;
	return sprintf(buf,"%x\n", val);

}
static ssize_t mv9319_wb_store(struct device* dev, struct device_attribute* attr, const char* buf, size_t n)
{
	int val;
	long rc;

	if (mv9319_ctrl == NULL)
		return 0;

	sscanf(buf,"%x",&val);

	if(val < CAMERA_WB_AUTO || val > CAMERA_WB_MAX) {
		printk("mv9319: invalid white balance input\n");
		return 0;
	}

	rc = mv9319_set_wb(val);
	if (rc < 0)
		printk("mv9319: failed to set white balance\n");

	return n;
}
static DEVICE_ATTR(wb, S_IRUGO|S_IWUGO, mv9319_wb_show, mv9319_wb_store);

static ssize_t mv9319_effect_show(struct device* dev, struct device_attribute* attr, char* buf)
{
	unsigned char val;

	if (mv9319_ctrl == NULL)
		return sprintf(buf,"%x\n", 0); 

	val = mv9319_ctrl->effect;
	return sprintf(buf,"%x\n", val);

}
static ssize_t mv9319_effect_store(struct device* dev, struct device_attribute* attr, const char* buf, size_t n)
{
	int val;
	long rc;

	if (mv9319_ctrl == NULL)
		return 0;

	sscanf(buf,"%x",&val);

	if(val < CAMERA_EFFECT_OFF || val > CAMERA_EFFECT_MAX) {
		printk("mv9319: invalid effect input\n");
		return 0;
	}

	rc = mv9319_set_effect(SENSOR_PREVIEW_MODE, val);
	if (rc < 0)
		printk("mv9319: failed to set effect\n");

	return n;
}
static DEVICE_ATTR(effect, S_IRUGO|S_IWUGO, mv9319_effect_show, mv9319_effect_store);

static ssize_t mv9319_firmware_show(struct device* dev, struct device_attribute* attr, char* buf)
{
	return sprintf(buf,"Current version %d.%d, update: %d\n", 
			firmware_ver_major, firmware_ver_minor,
			flag_update_firmware);

}
static ssize_t mv9319_firmware_store(struct device* dev, struct device_attribute* attr, const char* buf, size_t n)
{
	int val;

	sscanf(buf,"%x",&val);

	switch (val) {
		case 1:
			flag_update_firmware = 1;
			break;
		case 2:
			flag_update_firmware = 2;
			break;
		default:
			printk("mv9319: invalid input\n");
			return 0;

	}

	return n;
}
static DEVICE_ATTR(firmware, S_IRUGO|S_IWUGO, mv9319_firmware_show, mv9319_firmware_store);

static ssize_t mv9319_flickerless_show(struct device* dev, struct device_attribute* attr, char* buf)
{
	int hz = 0;
	switch (flickerless_hz) {
		case MV9319_FLICKERLESS_50HZ:
			hz = 50;
			break;
		case MV9319_FLICKERLESS_60HZ:
			hz = 60;
			break;
	}

	return sprintf(buf, "Current flickerless setting: %d HZ\n", hz);
}
static ssize_t mv9319_flickerless_store(struct device* dev, struct device_attribute* attr, const char* buf, size_t n)
{
	int val;

	sscanf(buf,"%d",&val);

	switch (val) {
		case 50:
			flickerless_hz = MV9319_FLICKERLESS_50HZ;
			break;
		case 60:
			flickerless_hz = MV9319_FLICKERLESS_60HZ;
			break;
		default:
			printk("mv9319: invalid input\n");
			return 0;
	}

	LDBG("mv9319: new flickerless settings: %d:0x%x\n", val, flickerless_hz);
	return n;
}
static DEVICE_ATTR(flickerless, S_IRUGO|S_IWUGO, mv9319_flickerless_show, mv9319_flickerless_store);

static ssize_t mv9319_camcorder_mode_show(struct device* dev, struct device_attribute* attr, char* buf)
{
	switch (mv9319_ctrl->fps_control) {
		case MV9319_FPS_AUTO:
			return sprintf(buf, "Camera Mode\n");
			break;
		case MV9319_FPS_30:
			return sprintf(buf, "Camcorder Mode\n");
			break;
		default:
			return sprintf(buf, "Wrong FPS value\n");
	}

}
static ssize_t mv9319_camcorder_mode_store(struct device* dev, struct device_attribute* attr, const char* buf, size_t n)
{
	int val;

	sscanf(buf,"%d",&val);

	mv9319_set_recording_mode(val);

	return n;
}
static DEVICE_ATTR(camcorder_mode, S_IRUGO|S_IWUGO, mv9319_camcorder_mode_show, mv9319_camcorder_mode_store);

static struct attribute* mv9319_sysfs_attrs[] = {
	&dev_attr_effect.attr,
	&dev_attr_wb.attr,
	&dev_attr_zoom.attr,
	&dev_attr_scene.attr,
	&dev_attr_flash.attr,
	&dev_attr_brightness.attr,
	&dev_attr_firmware.attr,
	&dev_attr_flickerless.attr,
	&dev_attr_camcorder_mode.attr,
	NULL
};

static void mv9319_sysfs_add(struct kobject* kobj)
{
	int i, n, ret;
	n = ARRAY_SIZE(mv9319_sysfs_attrs);
	for(i = 0; i < n; i++){
		if(mv9319_sysfs_attrs[i]){
			ret = sysfs_create_file(kobj, mv9319_sysfs_attrs[i]);
			if(ret < 0)
				printk("mv9319 sysfs is not created\n");
		}
	}
}

static void mv9319_sysfs_rm(struct kobject* kobj)
{
	int i, n;

	n = ARRAY_SIZE(mv9319_sysfs_attrs);
	for(i = 0; i < n; i++){
		if(mv9319_sysfs_attrs[i])
			sysfs_remove_file(kobj,mv9319_sysfs_attrs[i]);
	}
}

int mv9319_sensor_release(void)
{
	int rc = -EBADF;

	LDBG("%s: called\n", __func__);

	down(&mv9319_sem);

	eve_flash_set_led_state(3);

#if FEATURE_NEW_POWER_ON_OFF_SEQUENCE
	mv9319_power_off_2();
#else
	mv9319_power_off();
#endif

	// TODO: ??? GPIO Free
	gpio_direction_output(GPIO_CAM_RESET, 0);
	gpio_free(GPIO_CAM_RESET);
	gpio_direction_output(GPIO_CAM_MCLK, 0);
	gpio_free(GPIO_CAM_MCLK);

#if FEATURE_DEBUG_CAPTURE_TIMING
	LDBG("##### set GPIO_RES_CHG_BB to output low\n");
	gpio_direction_output(GPIO_RES_CHG_BB, 0);
	gpio_free(GPIO_RES_CHG_BB);
#endif

	kfree(mv9319_ctrl);
	mv9319_ctrl = NULL;

	if (!flag_update_firmware) 
		bd6083_set_camera_off_mode();
	flag_update_firmware = 0;

	up(&mv9319_sem);
	return rc;
}

static int mv9319_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	int rc = 0;
	LDBG("mv9319_probe called!\n");

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		printk("i2c_check_functionality failed\n");
		goto probe_failure;
	}

	LDBG("%s: device = %s, client addr = 0x%x, adapter = 0x%p\n", __func__,
			id->name, (client->addr << 1), client->adapter);

	if (!strcmp(id->name, "mv9319_firmware")) {
		// XXX: This is dangerous. But, works.
		// "camera" i2c device was already probed, so mv9319_ctrl->sensorw
		// was allocated.
		if (mv9319_sensorw == NULL) {
			printk("mv9319: probing firmware i2c client failed\n");
			rc = -EIO;
			goto probe_failure;
		}
		i2c_set_clientdata(client, mv9319_sensorw);
		mv9319_fw_client = client;

		return 0;
	}

	mv9319_sensorw = kzalloc(sizeof(struct mv9319_work_t), GFP_KERNEL);
	if (!mv9319_sensorw) {
		printk("mv9319: kzalloc failed.\n");
		rc = -ENOMEM;
		goto probe_failure;
	}

	i2c_set_clientdata(client, mv9319_sensorw);
	mv9319_init_client(client);
	mv9319_client = client;

	mv9319_sysfs_add(&client->dev.kobj);

	LDBG("mv9319_probe successed! rc = %d\n", rc);
	return 0;

probe_failure:
	kfree(mv9319_sensorw);
	mv9319_sensorw = NULL;
	printk("mv9319_probe failed! rc = %d\n", rc);
	return rc;
}

static int __exit mv9319_remove(struct i2c_client *client)
{
	/* TODO: this function is called twice. Handle It! */

	struct mv9319_work_t *sensorw = i2c_get_clientdata(client);

	LDBG("%s: called\n", __func__);

	mv9319_sysfs_rm(&client->dev.kobj);
	free_irq(client->irq, sensorw);
	i2c_detach_client(client);
	mv9319_client = NULL;
	mv9319_fw_client = NULL;
	kfree(sensorw);
	return 0;
}

static const struct i2c_device_id mv9319_id[] = {
	{"mv9319", 0},
	{"mv9319_firmware", 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, mv9319__id);

static struct i2c_driver mv9319_driver = {
	.id_table = mv9319_id,
	.probe = mv9319_probe,
	.remove = __exit_p(mv9319_remove),
	.driver = {
		.name = "mv9319",
	},
};

static int32_t mv9319_init(void)
{
	int32_t rc = 0;

	printk("mv9319: init\n");

	rc = i2c_add_driver(&mv9319_driver);
	if (IS_ERR_VALUE(rc))
		goto init_fail;
	return rc;

init_fail:
	printk("mv9319_init failed, rc = %d\n", rc);
	return rc;

}

void mv9319_exit(void)
{
	LDBG("%s: called\n", __func__);
	i2c_del_driver(&mv9319_driver);
}

int mv9319_probe_init(void *dev, void *ctrl)
{
	int rc = 0;
#if 0	/* Disabled because GPIO I2C driver initialized
	 * after msm_camera driver */

	struct msm_camera_sensor_info *info =
		(struct msm_camera_sensor_info *)dev;
#endif

	struct msm_sensor_ctrl_t *s = (struct msm_sensor_ctrl_t *)ctrl;

	LDBG("%s: called\n", __func__);

	rc = mv9319_init();	/* Just add I2C driver */
	if (rc < 0)
		goto probe_done;

	msm_camio_clk_rate_set(MV9319_DEFAULT_CLOCK_RATE);

#if 0	/* Disabled because GPIO I2C driver initialized
	 * after msm_camera driver */
	rc = mv9319_probe_init_sensor(info);
	if (rc < 0)
		goto probe_done;
#endif

	s->s_init = mv9319_sensor_open_init;
	s->s_release = mv9319_sensor_release;
	s->s_config = mv9319_sensor_config;
#if FEATURE_NEW_POWER_ON_OFF_SEQUENCE
	s->s_power_on = mv9319_power_on_1;
	s->s_power_off = mv9319_power_off_1;
#else
	s->s_power_on = NULL;
	s->s_power_off = NULL;
#endif

probe_done:
	printk("%s %s:%d\n", __FILE__, __func__, __LINE__);
	return rc;
}

