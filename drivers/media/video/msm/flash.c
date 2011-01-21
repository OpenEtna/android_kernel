/* Copyright (c) 2009-2010, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */
#include <linux/kernel.h>
#include <linux/errno.h>
#include <mach/pmic.h>
#include <mach/camera.h>

int32_t flash_set_led_state(enum msm_camera_led_state_t led_state)
{
	int32_t rc;

  CDBG("flash_set_led_state: %d\n", led_state);
  switch (led_state) {
  case MSM_LED_OFF:
    rc = pmic_flash_led_set_current(0);
    break;

  case MSM_LED_LOW:
    rc = pmic_flash_led_set_current(30);
    break;

  case MSM_LED_HIGH:
    rc = pmic_flash_led_set_current(100);
    break;

  default:
    rc = -EFAULT;
    break;
  }
  CDBG("flash_set_led_state: return %d\n", rc);

  return rc;
}
