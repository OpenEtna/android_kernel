/*
 * Copyright (C) 2008 Google, Inc.
 * Author: Nick Pelly <npelly@google.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __ASM_ARCH_MSM_SERIAL_HS_H
#define __ASM_ARCH_MSM_SERIAL_HS_H

#include<linux/serial_core.h>

// BEGIN: 0002333 chpark9@lge.com 2009-12-26
// ADD: 0002333: [SWIFT][BT] Bluetooth Sleep. 
#ifndef FEATURE_USE_BTLA
#define FEATURE_USE_BTLA
#endif/*FEATURE_USE_BTLA*/
// END: 0002333 chpark9@lge.com 2009-12-26

/* Optional platform device data for msm_serial_hs driver.
 * Used to configure low power wakeup */
struct msm_serial_hs_platform_data {
	int wakeup_irq;  /* wakeup irq */
	/* bool: inject char into rx tty on wakeup */
	unsigned char inject_rx_on_wakeup;
	char rx_to_inject;
};

unsigned int msm_hs_tx_empty(struct uart_port *uport);
void msm_hs_request_clock_off(struct uart_port *uport);
void msm_hs_request_clock_on(struct uart_port *uport);
void msm_hs_set_mctrl_locked(struct uart_port *uport,
				    unsigned int mctrl);

// BEGIN: 0002333 chpark9@lge.com 2009-12-26
// ADD: 0002333: [SWIFT][BT] Bluetooth Sleep. 
#ifdef FEATURE_USE_BTLA
struct uart_port * msm_hs_get_bt_uport(unsigned int line);
#endif/*FEATURE_USE_BTLA*/
// END: 0002333 chpark9@lge.com 2009-12-26

#endif
