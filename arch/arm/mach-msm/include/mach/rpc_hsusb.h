/* linux/include/mach/rpc_hsusb.h
 *
 * Copyright (c) 2008-2009, Code Aurora Forum. All rights reserved.
 *
 * All source code in this file is licensed under the following license except
 * where indicated.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can find it at http://www.fsf.org
 */

#ifndef __ASM_ARCH_MSM_RPC_HSUSB_H
#define __ASM_ARCH_MSM_RPC_HSUSB_H

#include <mach/msm_rpcrouter.h>
#include <mach/msm_otg.h>
#include <mach/msm_hsusb.h>

int msm_hsusb_rpc_connect(void);
int msm_hsusb_phy_reset(void);
int msm_hsusb_vbus_powerup(void);
int msm_hsusb_vbus_shutdown(void);
int msm_hsusb_send_productID(uint32_t product_id);
int msm_hsusb_send_serial_number(char *serial_number);
int msm_hsusb_is_serial_num_null(uint32_t val);
int msm_hsusb_reset_rework_installed(void);
int msm_hsusb_enable_pmic_ulpidata0(void);
int msm_hsusb_disable_pmic_ulpidata0(void);
int msm_hsusb_rpc_close(void);

#ifdef CONFIG_USB_GADGET_MSM_72K
int hsusb_chg_init(int connect);
void hsusb_chg_vbus_draw(unsigned mA);
void hsusb_chg_connected(enum chg_type chgtype);
#endif


int msm_fsusb_rpc_init(struct msm_otg_ops *ops);
int msm_fsusb_init_phy(void);
int msm_fsusb_reset_phy(void);
int msm_fsusb_suspend_phy(void);
int msm_fsusb_resume_phy(void);
int msm_fsusb_rpc_close(void);
int msm_fsusb_remote_dev_disconnected(void);
int msm_fsusb_set_remote_wakeup(void);
void msm_fsusb_rpc_deinit(void);
/* LnT-S fix for ADB device name */
typedef struct { 
	/* International Mobile Equipment Identity */
	unsigned char ue_imei[9];
} __attribute__((packed)) nv_ue_imei_type;
int msm_nv_imei_get(unsigned char* nv_imei_ptr);

  typedef enum {
    NV_READ_F,          /* Read item */
    NV_WRITE_F,         /* Write item */
    NV_PEEK_F,          /* Peek at a location */
    NV_POKE_F,          /* Poke into a location */
    NV_FREE_F,          /* Free an nv item's memory allocation */
    NV_CHKPNT_DIS_F,    /* Disable cache checkpointing for glitch recovery */
    NV_CHKPNT_ENA_F,    /* Enable cache checkpointing for glitch recovery */
    NV_OTASP_COMMIT_F,  /* Commit (write) OTASP parameters to nv */
    NV_REPLACE_F,       /* Replace (overwrite) a dynamic pool item */
    NV_INCREMENT_F,     /* Increment the rental timer item */
    NV_FUNC_ENUM_PAD = 0x7FFF     /* Pad to 16 bits on ARM */
#ifdef FEATURE_RPC
    , NV_FUNC_ENUM_MAX = 0x7fffffff /* Pad to 32 bits */
#endif

  } nv_func_enum_type;

typedef enum {
  NV_ESN_I							= 0,
  NV_UE_IMEI_I						= 550,
#ifdef FEATURE_NV_RPC_SUPPORT
    NV_ITEMS_ENUM_MAX			= 0x7fffffff
#endif
} nv_items_enum_type;

  typedef enum {
    NV_DONE_S,          /* Request completed okay */
    NV_BUSY_S,          /* Request is queued */
    NV_BADCMD_S,        /* Unrecognizable command field */
    NV_FULL_S,          /* The NVM is full */
    NV_FAIL_S,          /* Command failed, reason other than NVM was full */
    NV_NOTACTIVE_S,     /* Variable was not active */
    NV_BADPARM_S,       /* Bad parameter in command block */
    NV_READONLY_S,      /* Parameter is write-protected and thus read only */
    NV_BADTG_S,         /* Item not valid for Target */
    NV_NOMEM_S,         /* free memory exhausted */
    NV_NOTALLOC_S,      /* address is not a valid allocation */
    NV_STAT_ENUM_PAD = 0x7FFF     /* Pad to 16 bits on ARM */
#ifdef FEATURE_RPC
     ,     NV_STAT_ENUM_MAX = 0x7FFFFFFF     /* Pad to 16 bits on ARM */
#endif /* FEATURE_RPC */

  } nv_stat_enum_type;
  /* LGE_CHANGE_E [ljmblueday@lge.com] 2009-08-29 */
  /* LnT-E fix for ADB device name */
#endif
