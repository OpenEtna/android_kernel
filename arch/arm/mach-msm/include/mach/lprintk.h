/*
 *       Filename:  lprintk.h
 * 
 *    Description:  
 * 
 *         Author:  Jugwan Eom, zugwan@lge.com
 *        Company:  MES Group, LG Electronics, Inc.
 */

#ifndef __ASM_ARCH_MSM_LPRINTK_H
#define __ASM_ARCH_MSM_LPRINTK_H

#define D_KERNEL    0 
#define D_SD        1
#define D_KEY       2
#define D_FS        3
#define D_LCD       4
#define D_CAMERA    5
#define D_AUDIO     6
#define D_TOUCH     7
#define D_USB       8
#define D_BATT      9
#define D_POWER     10
#define D_WIFI      11
#define D_SENSOR    12
#define D_BT        13
#define D_FM        14
#define D_GPIO      15
#define D_MES       16
#define D_MAX       17

#ifdef CONFIG_LPRINTK
struct lprintk_info_struct {
        char *name;
        int enable;
};

extern struct lprintk_info_struct lge_debug[];

#define lprintk(type, fmt, args...) do { \
        if (!lge_debug[type].enable) \
                break; \
        printk("[%5s] : ", lge_debug[type].name); \
        printk(fmt, ##args); \
} while (0)
#else
#define lprintk(type, arg...) ((void)0)  /* do { } while(0) */
#endif

#define LPRINTK(X...) {lprintk(D_KERNEL,"%s:",__FUNCTION__); if (lge_debug[D_KERNEL].enable) printk(X);}
#define _LTRACE 	lprintk(D_KERNEL,"%s\n",__FUNCTION__)

#endif   /* ----- #ifndef __ASM_ARCH_MSM_LPRINTK_H ----- */

