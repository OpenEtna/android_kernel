#
# Makefile fragment for Linux 2.6
# Broadcom DHD Driver
#
# Copyright (C) 2008, Broadcom Corporation
# All Rights Reserved.
# 
# THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
# KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
# SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
#
# $Id: makefile.26,v 1.1.144.1 2008/05/07 22:53:44 Exp $

obj-$(CONFIG_BCM4325) += wireless.o

obj-$(CONFIG_BRCM_USE_STATIC_BUF) += brcm_static_buf.o

wireless-objs = \
	dhd_linux.o \
	linux_osl.o \
	bcmutils.o \
	dhd_common.o \
	dhd_custom_gpio.o \
	siutils.o \
	sbutils.o \
	aiutils.o \
	hndpmu.o \
	wl_iw.o \
	bcmwifi.o \
	dhd_cdc.o \
	dhd_sdio.o \
	bcmsdh_sdmmc.o \
	bcmsdh.o \
	bcmsdh_linux.o \
	bcmsdh_sdmmc_linux.o \

SRCROOT = drivers/net/wireless/bcm4329
EXTRA_CFLAGS = \
	-DLINUX \
	-DBCMDRIVER \
	-DBCMDONGLEHOST \
	-DDHDTHREAD \
	-DBCMWPA2 \
	-DBCMWAPI_WPI \
	-DUNRELEASEDCHIP \
	-DBDC \
	-DTOE \
	-DDHD_BCMEVENTS \
	-DSHOW_EVENTS \
	-DBCMSDIO \
	-DDHD_GPL \
	-DBCMLXSDMMC \
	-DBCMPLATFORM_BUS \
	-DSDIO_ISR_THREAD \
	-Wall -Wstrict-prototypes \
	-I$(SRCROOT) \
	-I$(SRCROOT)/include \
	-I$(SRCROOT)/shared \
	-I$(SRCROOT)/dhd/sys \
	-I$(SRCROOT)/dongle \
	-I$(SRCROOT)/wl/sys \
	\
	-DIOCTL_RESP_TIMEOUT=1000 \
	-DSDIO_VENDOR_ID_BROADCOM=0x2d0 \
	-DSDIO_DEVICE_ID_BROADCOM_4325=0x4c6 \
	-DANDROID_SPECIFIC \
	-Wframe-larger-than=8192


#	\
	-fshort-wchar \
	-D__KERNEL__ \
	-Iinclude \
	-Iinclude/asm/mach-default \
	-DBCMEMBEDIMAGE="<dongle/rte/wl/builds/4325b0/sdio-g-cdc-reclaim-idsup-wme/rtecdc.h>" \

ifeq ($(CONFIG_BCM4325_DEBUG),y)
EXTRA_CFLAGS += -DDHD_DEBUG -DSDTEST -DSRCBASE="\"$(SRCROOT)\""
endif

ifeq ($(CONFIG_BRCM_USE_STATIC_BUF),y)
EXTRA_CFLAGS += -DDHD_USE_STATIC_BUF
endif

ifeq ($(CONFIG_MMC_MSM),y)
CONFIG_MMC_MSM7X00A = y
EXTRA_CFLAGS += -DCONFIG_MMC_MSM7X00A=1
endif

ifeq ($(CONFIG_MMC_MSM7X00A),y)
EXTRA_CFLAGS += -Dlinux
EXTRA_CFLAGS += -DDHD_SDALIGN=64 -DMAX_HDR_READ=64 -DDHD_FIRSTREAD=64
endif
