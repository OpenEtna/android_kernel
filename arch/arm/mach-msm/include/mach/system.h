/* arch/arm/mach-msm/include/mach/system.h
 *
 * Copyright (C) 2007 Google, Inc.
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

#include <mach/hardware.h>

/* LGE_UPDATE_S [jinwoonam@lge.com] 2009.8.31 */
#if defined(CONFIG_MACH_EVE)
typedef enum {
	LGE_PCB_VER_UNKNOWN = 0,
	LGE_PCB_VER_A = 1,
	LGE_PCB_VER_B,
	LGE_PCB_VER_C,
	LGE_PCB_VER_D,
	LGE_PCB_VER_E,
	LGE_PCB_VER_F,
	LGE_PCB_VER_G,
	LGE_PCB_VER_1_0,
	LGE_PCB_VER_1_1,
	LGE_PCB_VER_1_2,
	LGE_PCB_VER_1_3,
	LGE_PCB_VER_1_4,
	LGE_PCB_VER_MAX
} lg_hw_rev_type;

int msm_proc_comm_get_hw_rev(int *hw_rev);
#endif
/* LGE_UPDATE_E [jinwoonam@lge.com] 2009.8.31 */
void arch_idle(void);

static inline void arch_reset(char mode)
{
	for (;;) ;  /* depends on IPC w/ other core */
}

/* low level hardware reset hook -- for example, hitting the
 * PSHOLD line on the PMIC to hard reset the system
 */
extern void (*msm_hw_reset_hook)(void);

void msm_set_i2c_mux(bool gpio, int *gpio_clk, int *gpio_dat);

