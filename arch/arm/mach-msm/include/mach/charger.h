#ifndef CHARGER_H_trimmed
#define CHARGER_H_trimmed


/* List of available charger IRQs. */
typedef enum                                          
{
    CHG_CHARGER_IRQ__WALL_VALID,
    CHG_CHARGER_IRQ__WALL_INVALID,
    CHG_CHARGER_IRQ__USB_VALID,           
    CHG_CHARGER_IRQ__USB_INVALID,                
    CHG_CHARGER_IRQ__INVALID
}chg_charger_irq_type;

/*=============================================================================
  @file  charger.h

  ---------------------------------------------------------------------------
  Copyright (c) 2008 QUALCOMM Incorporated. 
  ---------------------------------------------------------------------------

  $Id: //source/qcom/qct/platform/linux/targets/main/latest/msm8650/kernel/remote_apis/charger/inc/charger.h#3 $

  Notes: 
     ==== Auto-Generated File, do not edit manually ====
     Generated from build type: SDCAALBH 
     #defines containing SDCAALBH replaced with ________
=============================================================================*/
#ifndef CHARGER_H
#define CHARGER_H
/*===========================================================================

    B A T T E R Y   C H A R G E R   H E A D E R   F I L E

DESCRIPTION
  This header file contains all of the definitions necessary for other
  modules to interface with the battery charger utilities.

REFERENCES
  None

===========================================================================*/

/* LGE_CHANGE_S [ybw75@lge.com] 2009-01-21, Add Charger MtoA (modem to app) RPC routine */
//#define FEATURE_MTOA_CHARGER_RPC
/* LGE_CHANGE_E [ybw75@lge.com] 2009-01-21 */

/* LGE_CHANGE_S [ybw75@lge.com] 2009-03-19, Add test mode for changing GPIO setting during sleep */
#define FEATURE_TEST_FOR_SLEEP_GPIO_SETTING  	1
/* LGE_CHANGE_E [ybw75@lge.com] 2009-03-19 */

/* LGE_CHANGE_S [ybw75@lge.com] 2008-05-22, Add RPC for sharing battery information */
typedef enum 
{
  CHG_IDLE_ST,                  /* 0:Charger state machine entry point.       */
  CHG_WALL_IDLE_ST,             /* 1:Wall charger state machine entry point.  */
  CHG_WALL_TRICKLE_ST,          /* 2:Wall charger low batt charging state.    */
  CHG_WALL_NO_TX_FAST_ST,       /* 3:Wall charger high I charging state.      */
  CHG_WALL_FAST_ST,             /* 4:Wall charger high I charging state.      */
  CHG_WALL_TOPOFF_ST,           /* 5:Wall charger top off charging state.     */
  CHG_WALL_MAINT_ST,            /* 6:Wall charger maintance charging state.   */
  CHG_WALL_TX_WAIT_ST,          /* 7:Wall charger TX WAIT charging state.     */
  CHG_WALL_ERR_WK_BAT_WK_CHG_ST,/* Wall CHG ERR: weak batt and weak charger.*/
  CHG_WALL_ERR_WK_BAT_BD_CHG_ST,/* Wall CHG ERR: weak batt and bad charger. */
  CHG_WALL_ERR_GD_BAT_BD_CHG_ST,/* Wall CHG ERR: good batt and bad charger. */
  CHG_WALL_ERR_GD_BAT_WK_CHG_ST,/* Wall CHG ERR: good batt and weak charger.*/
  CHG_WALL_ERR_BD_BAT_GD_CHG_ST,/* Wall CHG ERR: Bad batt and good charger. */
  CHG_WALL_ERR_BD_BAT_WK_CHG_ST,/* Wall CHG ERR: Bad batt and weak charger. */
  CHG_WALL_ERR_BD_BAT_BD_CHG_ST,/* Wall CHG ERR: Bad batt and bad charger.  */
  CHG_WALL_ERR_GD_BAT_BD_BTEMP_CHG_ST,/* Wall CHG ERR: GD batt and BD batt temp */
  CHG_WALL_ERR_WK_BAT_BD_BTEMP_CHG_ST,/* Wall CHG ERR: WK batt and BD batt temp */
  CHG_USB_IDLE_ST,              /* 17:USB charger state machine entry point.   */
  CHG_USB_TRICKLE_ST,           /* 18:USB charger low batt charging state.     */
  CHG_USB_NO_TX_FAST_ST,        /* 19:USB charger high I charging state.       */ 
  CHG_USB_FAST_ST,              /* 20:USB charger high I charging state.       */ 
  CHG_USB_TOPOFF_ST,            /* 21:USB charger top off state charging state.*/ 
  CHG_USB_DONE_ST,              /* 22:USB charger Done charging state.         */ 
  CHG_USB_ERR_WK_BAT_WK_CHG_ST, /* USB CHG ERR: weak batt and weak charger. */
  CHG_USB_ERR_WK_BAT_BD_CHG_ST, /* USB CHG ERR: weak batt and bad charger.  */
  CHG_USB_ERR_GD_BAT_BD_CHG_ST, /* USB CHG ERR: good batt and bad charger.  */
  CHG_USB_ERR_GD_BAT_WK_CHG_ST, /* USB CHG ERR: good batt and weak charger. */
  CHG_USB_ERR_BD_BAT_GD_CHG_ST, /* USB CHG ERR: Bad batt and good charger.  */
  CHG_USB_ERR_BD_BAT_WK_CHG_ST, /* USB CHG ERR: Bad batt and weak charger.  */
  CHG_USB_ERR_BD_BAT_BD_CHG_ST, /* USB CHG ERR: Bad batt and bad charger.   */
  CHG_USB_ERR_GD_BAT_BD_BTEMP_CHG_ST,/* USB CHG ERR: GD batt and BD batt temp */
  CHG_USB_ERR_WK_BAT_BD_BTEMP_CHG_ST,/* USB CHG ERR: WK batt and BD batt temp */
  CHG_VBATDET_CAL_ST,           /* 32:VBATDET calibration state*/
  CHG_INVALID_ST
} chg_state_type;

#if 0
typedef struct
{
    unsigned int voltage;				/*gpStatus->sps.BatteryVoltage*/
    unsigned int current0;			/*gpStatus->sps.BatteryCurrent			[mA] */
    unsigned int avgcurrent;			/*gpStatus->sps.BatteryAverageCurrent  average value during 29sec [mA]*/
    unsigned int temperature;		/*gpStatus->sps.BatteryTemperature    298 --> 29.8'C*/
    unsigned int acr;						/*gpStatus->sps.BackupBatteryLifeTime*/
	unsigned char batt_status;		/*gpStatus->sps.BackupBatteryFlag*/
	unsigned char as;						/*gpStatus->sps.BackupBatteryVoltage*/
	unsigned char lifepercentRSRC;/*gpStatus->sps.BatteryLifeTime*/
	unsigned char lifepercentRARC;
	unsigned int lifeRSAC;				/*gpStatus->sps.BatteryFullLifeTime*/
	unsigned int lifeRAAC;				/*gpStatus->sps.BackupBatteryFullLifeTime*/
	unsigned int fullcapa;				/*gpStatus->sps.BatterymAHourConsumed*/
	unsigned char valid_flag;		// good --> SM_RET_NO_ERR
	chg_state_type	chg_curr_state_flag;
}smart_battery_info_type;
#endif

typedef struct
{
	unsigned int  batt_remaining;		/* 0~100 % */
	unsigned int  batt_voltage;		/* mV */
	int  batt_current;		/* mA */
	int  batt_temperature;	/* 0.1 Celcious degree */
	unsigned int  batt_flag;
	unsigned int  chg_status;
	unsigned int  charger_type;
	unsigned int  present;
} lge_battery_info_type;

typedef enum 
{
  	BATT_VOLTAGE,
	BATT_CURRENT,
	BATT_CAPACITY, 
	BATT_TEMPERATURE,
	BATT_FLAGS,
	CHG_STATUS,
} lge_battery_info_parameter;

typedef enum 
{
  	SB_VOLTAGE,
	SB_CURRENT,
	SB_AVGCURRENT, 
	SB_TEMPERATURE,
	SB_ACR,
	SB_BATT_STATUS,
	SB_AS,
	SB_LIFEPERCENTRSRC,
	SB_LIFEPERCENTRARC,
	SB_LIFERSAC,
	SB_LIFERAAC,
	SB_FULLCAPA,
	SB_VALID_FLAG,
	SB_CHG_CURR_STATE_FLAG,
} sb_info_type;

typedef enum {
	UNKNOWN_BATT,
	SMART_BATT,
	DUMMY_BATT
} battery_type;

#define SM_RET_NO_ERR 1
#define SM_RET_ERR 0
/* LGE_CHANGE_E [ybw75@lge.com] 2008-05-22 */

/* LGE_CHANGE_S [ybw75@lge.com] 2009-04-01, Add shutdonw test code */
typedef enum
{
    TEST_SHUT_DOWN 			= 0,
    TEST_BATT_SWITCH_OFF	= 1,
    TEST_BATT_SWITCH_ON		= 2,
    TEST_USB_SWITCH_OFF		= 3,
    TEST_USB_SWITCH_ON		= 4,
    TEST_AC_SWITCH_OFF		= 5,
    TEST_AC_SWITCH_ON		= 6,
    TEST_CALL_SLEEP_GPIO_SET= 7,
    TEST_RPC_MAX = 20,
    TEST_SMEM_SHUT_DOWN = 21,
    TEST_SMEM = 22,
} lge_test_type;
/* LGE_CHANGE_E [ybw75@lge.com] 2009-04-01 */

extern void lge_battery_update_charger(void);

#ifndef REX_H
#define REX_H
/*==========================================================================

                      R E X    H E A D E R    F I L E

DESCRIPTION
   API for the REX - Real Time Executive


===========================================================================*/

#ifndef COMDEF_H
#define COMDEF_H
/*===========================================================================

                   S T A N D A R D    D E C L A R A T I O N S

DESCRIPTION
  This header file contains general types and macros that are of use
  to all modules.  The values or definitions are dependent on the specified
  target.  T_WINNT specifies Windows NT based targets, otherwise the
  default is for ARM targets.

       T_WINNT  Software is hosted on an NT platforn, triggers macro and
                type definitions, unlike definition above which triggers
                actual OS calls

DEFINED TYPES

       Name      Definition
       -------   --------------------------------------------------------
       byte      8  bit unsigned value
       word      16 bit unsigned value
       dword     32 bit unsigned value

       uint1     byte
       uint2     word
       uint4     dword

       uint8     8  bit unsigned value
       uint16    16 bit unsigned value
       uint32    32 bit unsigned value
       uint64    64 bit unsigned value

       int8      8  bit signed value
       int16     16 bit signed value
       int32     32 bit signed value
       int 64    64 bit signed value

       sint31    32 bit signed value
       sint15    16 bit signed value
       sint7     8  bit signed value

       int1      8  bit signed value
       int2      16 bit signed value
       int4      32 bit signed value

       boolean   8 bit boolean value

DEFINED CONSTANTS

       Name      Definition
       -------   --------------------------------------------------------
       TRUE      Asserted boolean condition (Logical 1)
       FALSE     Deasserted boolean condition (Logical 0)

       ON        Asserted condition
       OFF       Deasserted condition

       NULL      Pointer to nothing

       PACKED    Used to indicate structures which should use packed
                 alignment

       INLINE    Used to inline functions for compilers which support this

===========================================================================*/

#ifndef TARGET_H
#define TARGET_H
/*===========================================================================

      T A R G E T   C O N F I G U R A T I O N   H E A D E R   F I L E

DESCRIPTION
  All the declarations and definitions necessary for general configuration
  of the DMSS software for a given target environment.

===========================================================================*/


/* All featurization starts from customer.h which includes the appropriate
**    cust*.h and targ*.h
*/
#ifdef CUST_H
#ifndef CUSTOMER_H
#define CUSTOMER_H
/*===========================================================================

                   C U S T O M E R    H E A D E R    F I L E

DESCRIPTION
  This header file provides customer specific information for the current
  build.  It expects the compile time switch /DCUST_H=CUSTxxxx.H.  CUST_H
  indicates which customer file is to be used during the current build.
  Note that cust_all.h contains a list of ALL the option currently available.
  The individual CUSTxxxx.H files define which options a particular customer
  has requested.


===========================================================================*/


/* Make sure that CUST_H is defined and then include whatever file it
** specifies.
*/
#ifdef CUST_H
#ifndef CUST_________H
#define CUST_________H
/* ========================================================================
FILE: CUST________

=========================================================================== */

#ifndef TARG_________H
#ifndef TARG_________H
#define TARG_________H
/* ========================================================================
FILE: TARG________

=========================================================================== */

#define T_MSM8650B


#endif /* TARG_________H */
#endif

#define FEATURE_GSM 
#define FEATURE_WCDMA 
#define FEATURE_NATIVELINUX 
#define FEATURE_EXPORT_7K_APIS 
#define FEATURE_SMD 
#define FEATURE_SMD_BRIDGE 
#define FEATURE_L4 
#define FEATURE_AUDIO_CONFIGURATION_VALUE_ADD 
#define FEATURE_SD20 
#define FEATURE_SSPR_ENHANCEMENTS 
#define FEATURE_MULTIPROCESSOR 
#define FEATURE_NV_RPC_SUPPORT 
#define FEATURE_GSM 
#define FEATURE_WCDMA 
#define FEATURE_HSDPA_ACCESS_STRATUM 
#define FEATURE_UIM 
#define FEATURE_MMGSDI_NO_TCB_PTR_OR_CRIT_SEC 
#define FEATURE_HS_USB 
#define FEATURE_USB_CDC_ACM 
#ifdef IMAGE_APPS_PROC
   #define FEATURE_USB_DIAG 
#endif
#ifdef IMAGE_APPS_PROC
   #define FEATURE_USB_DIAG_NMEA 
#endif
#define FEATURE_CGPS 
#define FEATURE_CGPS_USES_UMTS 
#define FEATURE_DS 
#define FEATURE_HTORPC_METACOMMENTS 

#ifndef CUSTREMOTEAPIS_H
#define CUSTREMOTEAPIS_H
/*===========================================================================

                      C U S T R E M O T E A P I S . H

DESCRIPTION
  This file defines which APIs are remoted via ONCRPC.

===========================================================================*/

#ifndef CUSTDRIVERS_H
#define CUSTDRIVERS_H
/*===========================================================================

                           C U S T D R I V E R S

DESCRIPTION
  Customer definition file for drivers.


===========================================================================*/ 


/* Enable F3 trace feature */
#if !defined(BOOT_LOADER) && !defined(BUILD_HOSTDL) && !defined(FEATURE_WINCE)
  #define FEATURE_SAVE_DEBUG_TRACE
  #define FEATURE_ERR_EXTENDED_STORE
#endif


#endif /* CUSTDRIVERS_H */

#ifdef FEATURE_EXPORT_7K_APIS

  #if defined(FEATURE_DS) && (defined(FEATURE_WCDMA) || defined(FEATURE_GSM))
#ifndef CUSTMODEMDATA_H
#define CUSTMODEMDATA_H
/*===========================================================================

DESCRIPTION
  Configuration for NVM.

===========================================================================*/


/*===========================================================================

                      EDIT HISTORY FOR FILE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

$Header: //source/qcom/qct/platform/linux/targets/main/latest/msm8650/kernel/remote_apis/charger/inc/charger.h#3 $
$DateTime: 2008/11/18 13:31:44 $ $Author: sjhajhri $

when       who     what, where, why
--------   ---     ------------------------------------------------------------
04/14/08   hs      Created new file for multimode targets
===========================================================================*/
/* -----------------------------------------------------------------------
** Data Services
** ----------------------------------------------------------------------- */
#define FEATURE_DS

#if defined(FEATURE_GSM_GPRS) || defined(FEATURE_WCDMA)
#ifndef CUSTDATAUMTS_H
#define CUSTDATAUMTS_H

/*===========================================================================

DESCRIPTION
  Configuration for NVM.

===========================================================================*/


/*===========================================================================

                      EDIT HISTORY FOR FILE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

$Header: //source/qcom/qct/platform/linux/targets/main/latest/msm8650/kernel/remote_apis/charger/inc/charger.h#3 $ 

$DateTime: 2008/11/18 13:31:44 $ $Author: sjhajhri $

when       who     what, where, why
--------   ---     ------------------------------------------------------------
04/14/08   hs      Created new file for multimode targets
===========================================================================*/

  #define FEATURE_DATA

/* ---  GSM  --- */
/* Turn on GCSD stack */
#ifdef FEATURE_DATA
  #define FEATURE_DATA_GCSD
#endif

/* Enable ETSI specific functionality in PS 
*/
#define FEATURE_DATA_WCDMA_PS

#ifdef FEATURE_WCDMA
/* Enable WCDMA Circuit-Switched Data 
*/
#define FEATURE_DATA_WCDMA_CS
#endif /* FEATURE_WCDMA */

#if (!defined(FEATURE_DATA_ON_APPS) && defined(IMAGE_MODEM_PROC)) 
  /* Enable UCSD API for VT calls (WindowsCE builds)
  */
  #define FEATURE_DATA_UCSD_SCUDIF_API
#endif

#ifdef FEATURE_NATIVELINUX
  #define FEATURE_DATA_UCSD_UNIFIED_API
#endif


/*  Enable UCSD API SIO port notificaiton */
#define FEATURE_DATA_UCSD_API_SIOPORT

#define FEATURE_SECONDARY_PDP

#define FEATURE_PS_DORMANT_PWR_SAVE

#endif /* CUSTDATAUMTS_H */
#endif /* FEATURE_GSM_GPRS || FEATURE_WCDMA*/


#endif /* CUSTMODEMDATA_H */
#ifndef CUSTUMTSDATA_H
#define CUSTUMTSDATA_H
/*===========================================================================

DESCRIPTION
  Configuration for UMTS DATA.

===========================================================================*/


/* ---  GSM  --- */
/* Turn on GCSD stack */
#ifdef FEATURE_DATA
  #define FEATURE_DATA_GCSD
#endif

/* Enable ETSI specific functionality in PS
*/
#define FEATURE_DATA_WCDMA_PS

/* Enable WCDMA Circuit-Switched Data
*/
#define FEATURE_DATA_WCDMA_CS

#if (!defined(FEATURE_DATA_ON_APPS) && defined(IMAGE_MODEM_PROC)) 
  /* Enable UCSD API for VT calls (WindowsCE builds)
  */
  #define FEATURE_DATA_UCSD_SCUDIF_API
#endif

#ifdef FEATURE_NATIVELINUX
  #define FEATURE_DATA_UCSD_UNIFIED_API
#endif


/*  Enable UCSD API SIO port notificaiton */
#define FEATURE_DATA_UCSD_API_SIOPORT

#define FEATURE_SECONDARY_PDP

#define FEATURE_PS_DORMANT_PWR_SAVE

#endif /* CUSTUMTSDATA_H */
  #endif


  #if defined(FEATURE_DATA_ON_APPS) && defined(FEATURE_WCDMA)
    #define FEATURE_EXPORT_UMTS_DATA
  #endif /*FEATURE_DATA_ON_APPS*/

  #ifdef FEATURE_WCDMA
     #ifdef  FEATURE_EXPORT_UMTS_DATA
     #else
       #if (!defined(FEATURE_DATA_ON_APPS) && defined(IMAGE_MODEM_PROC))
         /* Remoted UCSD API (for non-AMSS modem build only) */
         #define FEATURE_EXPORT_DSUCSDAPPIF_APIS
       #endif
     #endif/*FEATURE_EXPORT_UMTS_DATA*/
  #endif


#endif /* FEATURE_EXPORT_7K_APIS */
#endif /* CUSTREMOTEAPIS_H */
#ifndef CUSTL4_H
#define CUSTL4_H
/*===========================================================================

            " C u s t - L 4 "   H E A D E R   F I L E

DESCRIPTION
  Configuration for L4 Feature.

===========================================================================*/


/*===========================================================================

                      EDIT HISTORY FOR FILE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

$Header:
$DateTime: 2008/11/18 13:31:44 $ $Author:

when       who     what, where, why
--------   ---     ----------------------------------------------------------
03/10/06   hn      Disabled FEATURE_L4_CPUSLEEP for 7200
12/07/05   ddh     Enabled FEATURE_L4_CPUSLEEP
11/17/05   dlb'    Enabling Rex Profiling for modem processor..
10/27/05   AMW     Added BUILD_JFLASH exception
09/21/05   AMW     Added test for building boot and tools

===========================================================================*/


#if !defined(BUILD_BOOT_CHAIN) && \
    !defined(BUILD_JNAND) && \
    !defined(BUILD_JFLASH) && \
    !defined(BUILD_HOSTDL) && \
    defined(FEATURE_L4)

  #define FEATURE_QUARTZ_135
#else
#endif /* !BUILD_BOOT_CHAIN & !BUILD_JNAND & FEATURE_L4 */

#endif /* CUSTL4_H */
#ifndef CUSTSIO_H
#define CUSTSIO_H
/*===========================================================================

DESCRIPTION
  Configuration for SIO

===========================================================================*/
                                        

/*Enable to open MMC port*/
#ifdef FEATURE_BUILD_MMC
   #define FEATURE_MMC
#endif


/* Enable new SIO code - required for MSM3100 s/w release
*/
#define FEATURE_NEW_SIO

#ifdef FEATURE_NEW_SIO

/* Enables Serial Device Mapper.  Must define
** FEATURE_NEW_SIO for this to be effective.
*/
  #define FEATURE_SERIAL_DEVICE_MAPPER

#ifdef FEATURE_SERIAL_DEVICE_MAPPER
  /* Runtime Device Mapper services.  These are mutually exclusive
  ** with FEATURE_SDEVMAP_UI_MENU & FEATURE_SDEVMAP_MENU_ITEM_NV.
  */
  #define FEATURE_RUNTIME_DEVMAP

  /* Enables UI selection of port mapping. The port map is stored in
  ** NV and retrieved - requires power cycle for changes to take effect.
  ** These should be defined or undefined together.
  */
  #ifndef FEATURE_RUNTIME_DEVMAP
    #define FEATURE_SDEVMAP_MENU_ITEM_NV
  #endif

  #ifdef FEATURE_DS
    /* Enables UI selection of DS baud rate and the DS baud to be stored in
    ** and retrieved from NV.  These should be defined or undefined together.
    */
    #define  FEATURE_DS_DEFAULT_BITRATE_NV
  #endif /* FEATURE_DS */

   #define FEATURE_DIAG_DEFAULT_BITRATE_NV

#endif  /* FEATURE_SERIAL_DEVICE_MAPPER */

#endif  /* FEATURE_NEW_SIO */

#endif /* CUSTSIO_H */
#ifndef CUSTMMODE_H
#define CUSTMMODE_H
/*========================================================================

DESCRIPTION

  Configuration CUST file for all of Multi Mode code

========================================================================*/


  /* Enable FEATURE_MM_SUPERSET for targets that use RPCs and thus
  ** need remote API files.
  ** This flag being on results in uniform remote API files for MMODE
  ** as even 1X-only targets will then pull in NAS files that CM uses in
  ** its API
  */
  #ifdef FEATURE_MULTIPROCESSOR
  #define FEATURE_MM_SUPERSET
  #endif


  /*
  ** Multi Mode features that are WCDMA or GSM specific
  */
  #if (defined FEATURE_WCDMA) || (defined FEATURE_GSM)

    /* Feature to deactivate secondary PDP before sending
    ** setup response for incoming call.
    */
    #define FEATURE_CM_DELAY_SETUPRES
    
    /* For CR42797: Allow UE goes to power save mode when PS call is dormant.
    ** This feature is required for GSM/UMTS capable configuration including 
    ** multimode configuration.
    */
    #define FEATURE_PS_DORMANT_PWR_SAVE

  #endif /* (defined FEATURE_WCDMA) || (defined FEATURE_GSM) */

#endif /* CUSTMMODE_H */
#ifndef CUSTAVS_H
#define CUSTAVS_H
/*===========================================================================

DESCRIPTION
  Configuration for AVS

===========================================================================*/


/*===========================================================================

                      EDIT HISTORY FOR FILE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

$Header: //source/qcom/qct/platform/linux/targets/main/latest/msm8650/kernel/remote_apis/charger/inc/charger.h#3 $ $DateTime: 2008/11/18 13:31:44 $ $Author: sjhajhri $

when       who     what, where, why
--------   ---     ----------------------------------------------------------
03/23/07    aw     Enable QDSP_PACKET_SNIFFER only for vocoder 1x which have
                   FEATURE_DIAG_PACKET_COUPLING enabled in custdiag.h
01/22/07    aw     Enabled FEATURE_AUDIO_NEW_AUX_CODEC_HW for 7600, 7200A 
17/01/07   kan     Enabled  QDSP_PACKET_SNIFFER for 1X vocoder packet log.
01/16/06    ac     Enabled FEATURE_AVS_I2SSBC
                           FEATURE_EXTERNAL_SDAC 
                           FEATURE_EXTERNAL_SADC
11/16/06    aw     Enable FEATURE_IIR_FILTER
10/23/06    aw     Added FEATURE_AVS_NEXTGEN_EC
07/19/06    aw     Added FEATURE_VOC_PCM_INTERFACE
04/07/06    aw     Added FEATURE_INTERNAL_SADC
02/04/06   bam     Added FEATURE_AUDFMT_AMR
12/28/05   bam     Added FEATURE_VOICE_RECORD.
12/28/05    aw     Enabled FEATURE_VOICE_PLAYBACK
12/20/05    aw     Added FEATURE_VOC_PACKET_INTERFACE
12/12/04    aw     Added FEATURE_MVS
11/10/04    ro     Added FEATURE_QDSP_RTOS.
02/19/03    st     Added FEATURE_GRAPH_ADPCM.
02/03/03   cah     Added FEATURE_MVS for GSM.
11/11/02   cah     Added GSM features.
09/06/02    st     Enabled FEATURE_TTY.
08/23/02    st     Updated defines for SDAC.
05/01/02   jct     Created
===========================================================================*/
#if defined(FEATURE_AUDIO_CONFIGURATION_VALUE_ADD) || \
    defined(FEATURE_AUDIO_CONFIGURATION_STANDARD)

  #ifndef FEATURE_AUDIO_CONFIGURATION_MINIMAL
    #define FEATURE_AUDIO_CONFIGURATION_MINIMAL
  #endif

#endif /* FEATURE_AUDIO_CONFIGURATION_VALUE_ADD || 
          FEATURE_AUDIO_CONFIGURATION_STANDARD */

#ifdef FEATURE_AUDIO_CONFIGURATION_MINIMAL

  /**************************************************************************
  [FEATURE NAME]:      FEATURE_TTY
  [SCOPE]:             AVS
  [DESCRIPTION]:       Enable TTY Support
  [OEM CAN ENABLE/DISABLE]: YES.
  **************************************************************************/
  #define FEATURE_TTY

  /**************************************************************************
  [FEATURE NAME]:      FEATURE_EXTERNAL_SDAC
  [SCOPE]:             AVS
  [DESCRIPTION]:       Allows use of the internal codec's Stereo DAC 
  [OEM CAN ENABLE/DISABLE]: YES.
  **************************************************************************/
  #define FEATURE_EXTERNAL_SDAC

  #if defined(FEATURE_INTERNAL_SDAC) || defined(FEATURE_EXTERNAL_SDAC)
    /* Adds calibration values and provides common code between 
    ** FEATURE_INTERNAL_SDAC and FEATURE_EXTERNAL_SDAC.
    */
    #define FEATURE_STEREO_DAC
  #endif

  
#endif /* FEATURE_AUDIO_CONFIGURATION_MINIMAL */

#endif /* CUSTAVS_H */
#ifndef CUSTWMSAPP_H
#define CUSTWMSAPP_H
/*===========================================================================

DESCRIPTION
  Configuration for WMS Application

===========================================================================*/

#ifndef CUSTWMS_H
#define CUSTWMS_H
/*===========================================================================

DESCRIPTION
  Configuration for WMS

  Copyright (c) 2002,2003,2004, 2005, 2006 by QUALCOMM Incorporated. 
===========================================================================*/

#ifndef CUSTGSM_H
#define CUSTGSM_H
/*=========================================================================== 

                           C U S T G S M

DESCRIPTION
  Customer file for GSM.

===========================================================================*/


/*===========================================================================

                      EDIT HISTORY FOR FILE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

$Header: //source/qcom/qct/platform/linux/targets/main/latest/msm8650/kernel/remote_apis/charger/inc/charger.h#3 $

when       who     what, where, why
--------   ---     ----------------------------------------------------------
02/10/08	ps	   Enable AMR-WB for LCU	 	
18/09/08    rc     Removed FEATURE_USB_WITH_ARM_HALT, Enabled FEATURE_GAN dependent features & FEATURE_LAPDM_REESTABLISHMENT
04/09/08    rc     Enabled FEATURE_GSM_RGS_MULTIPASS
01/09/08    rc     Enable R-SACCH for LCU
04/08/08    rc     Disable AMR-WB and R-SACCH feature in LCU for now.
08/07/08    ws     Changes for raptor2 Target, Disable AMR-WB and define FEATURE_GSM_8K_QDSP6_AUDIO
13/06/08    rc     Added 2H07 GERAN Feature AMR-WB
07/05/08    rc     Added 2H07 GERAN Features R-SACCH & PS HO
07/05/08    rc     Added FEATURE_GPRS_FAST_RACH 
30/04/08    ws     Moved ESAIC feature to cutgsmdrv.h
30/04/08    ws     Enable ESAIC for LCU
06/02/08    ws     Added FEATURE_GSM_MDSP_ESAIC
05/02/08    ws     Added FEATURE_GSM_L1_IMPLEMENT_LPM
22/01/08    ws     Added T_QSC6270 specific featurisation
22/10/07    ws     Addede Enable ACQ database feature
14/09/07    rc     Added FEATURE_GPRS_COMP_ENUM_TYPES to re-use SNDCP VU that support IPHC with other layers that does not support IPHC
14/08/07    rc     Replaced FEATURE_GSM_SNDCP_IPHC_ROHC with FEATURE_GSM_SNDCP_IPHC
                   Added FEATURE_GSM_L1_OOS_SEARCH_TIME_ENH
09/07/07    rc     Removed FEATURE_CSN_REFACTORED
31/05/07    ws     Enabled FEATURE_GSM_L1_CONTROL_SET_BAND
16/04/07    npr    Enabled 2H06 features: EDTM, RFACCH, IPA, CSN
19/01/07    rc     Enabled FEATURE_GSM_GPRS_LLC_GEA_NV_SUPPORT
21/11/06    rc     Enabled FEATURE_GSM_L1_HFREQ_ERR_NCELL_SCH_DECODE
10/02/06    ws     Enabled DTM,EDA and PFC
09/06/05    ws     Removed FEATURE_GSM_DTM  since new build Id's use them
08/26/05    ws     Merged from Raven Branch features DISABLE_XFER_SNR_RESELECTION
                   and FEATURE_GPRS_GRR_GMM_PCCCH_PAGING_COORD
08/25/05    ws     Enabled FEATURE_GSM_DTM
06/23/05   src     Enabled FEATURE_GSM_GPRS_PEMR
05/24/05    gw     Enable WCDMA ID searches in transfer mode via
                   FEATURE_GTOW_RESELECTION_XFER_CELL_ID
04/24/05    bk     Temp disabled SNR based reselection in transfer via DISABLE_XFER_SNR_RESELECTION
11/19/04   gfr     Moved DEBUG_HW_SELECT_STAGE2_OUTPUT_CHANNEL_FILTER to
                   custgsmdrv.h
13/05/04   npr     Initial revision.
===========================================================================*/

#ifdef FEATURE_GSM

/***************************************************************************
                                 GSM
****************************************************************************/
#define FEATURE_GSM_EXT_SPEECH_PREF_LIST


/* Enable GPRS as Standard */
#define FEATURE_GSM_GPRS

#if ( !defined(T_MSM8650B) )
/* Don't enable W-AMR on Raptor2 for now */
#define FEATURE_GSM_AMR_WB
#endif


/*-------------------------------------------------------------------------*/
#endif /* FEATURE_GSM */
#endif /* CUSTGSM_H */

/*---------------------------------------------------------------------------
                         Wireless Messaging Services
---------------------------------------------------------------------------*/
#ifdef FEATURE_CDMA
  #define FEATURE_CDSMS
#endif

#ifdef FEATURE_WCDMA
  #define FEATURE_GWSMS
#endif

/* Common Features across CDMA and GW Modes */
#if defined(FEATURE_CDSMS) || defined(FEATURE_GWSMS)
  
  /* Enables user data headers to support voice and email address.
  */
  #define FEATURE_SMS_UDH

#endif /*defined(FEATURE_CDSMS) || defined(FEATURE_GWSMS)*/

#endif /* CUSTWMS_H */

#endif /* CUSTWMSAPP_H */
#ifndef CUSTNVM_H
#define CUSTNVM_H
/*===========================================================================

DESCRIPTION
  Configuration for NVM.

===========================================================================*/


/* Phone's NV memory will support one NAM of 625 bytes, and the one-time
   subsidy lock item
*/
#define  FEATURE_NV_ONE_NAM_RL_LARGE

/* This feature configures NV to have two small roaming lists for a two NAM phone,
** as opposed to having one large roaming list for a one NAM phone.
*/
#undef  FEATURE_NV_TWO_NAMS_RL_SMALL

/* Define to enable the use of a trimode rf calibration file to derive
** NV items.  Also triggers the NV section to support this feature.
*/
#define FEATURE_TRIMODE_ITEMS

/* Enable reading of BTF value from NV
*/
#define FEATURE_ENC_BTF_IN_NV

#endif /* CUSTNVM_H */
#ifndef CUSTRF_H
#define CUSTRF_H
/*===========================================================================

DESCRIPTION
  Configuration for RF

===========================================================================*/


/* Enable NV item to calculate the PA warmup delay used for puncturing during
** transmit.
*/
#define FEATURE_SUBPCG_PA_WARMUP_DELAY

/* CRF3100-PA and QRF3300-1 uses RFR3100
** Enables reading of Forward Mode Rx CAGC RF Cal items.
*/
#define FEATURE_RFR3100

/* Features to use the improved TX AGC supported by CRF3100-PA
** Features enable reading of Forward Mode Tx CAGC RF Cal items.
*/

#define FEATURE_PA_RANGE_TEMP_FREQ_COMP
#define FEATURE_4PAGE_TX_LINEARIZER


#endif /* CUSTRF_H */
#ifndef CUSTUSURF_H
#define CUSTUSURF_H
/*=========================================================================== 

                           C U S T U S U R F

DESCRIPTION
  Customer file for the MSM6280 UMTS (GSM + WCDMA) SURF build.

===========================================================================*/


// ---  GSM  ---
/* Enable GSM code
*/
#define FEATURE_GSM

// --- WCDMA ---
/* Enable WCDMA code
*/
#define FEATURE_WCDMA

/* Must be defined for SIO services.
*/
#define FEATURE_NEW_SIO

/* Must be defined to use the baud change abilities for Diag and DS.
*/
#define FEATURE_SERIAL_DEVICE_MAPPER

/* Must be defined to use the Runtime Device Mapper.
*/
#define FEATURE_RUNTIME_DEVMAP

/* Must be defined for SIO services.
*/
#define FEATURE_NEW_SIO

/* Must be defined to use the baud change abilities for Diag and DS.
*/
#define FEATURE_SERIAL_DEVICE_MAPPER

/* Must be defined to use the Runtime Device Mapper.
*/
#define FEATURE_RUNTIME_DEVMAP
#define FEATURE_MMGSDI_GSM 
#define FEATURE_MMGSDI_UMTS

#define FEATURE_ALS

/* Enable Data Services
*/
#define FEATURE_DATA

/* Enable WCDMA Circuit-Switched Data
*/
#define FEATURE_DATA_WCDMA_CS

// ---  GSM  ---
/* Turn on GCSD stack */
#ifdef FEATURE_DATA
  #define FEATURE_DATA_GCSD
#endif


/* Enable ETSI specific functionality in PS
*/
#define FEATURE_DATA_WCDMA_PS

/* Enable feature to read DS baud info from NV
*/
#define FEATURE_DS_DEFAULT_BITRATE_NV

/* Enable Factory Test Mode support
*/
#define FEATURE_FACTORY_TESTMODE
#define FEATURE_IS683A_PRL

/* Support for Multiple PDP contexts */
#define FEATURE_MULTIPLE_PRIMARY_PDP

#define FEATURE_UUS

#define FEATURE_CCBS

#endif /* CUSTUSURF_H */


/*****************************************************************************/
#ifndef CUSTWCDMA_H
#define CUSTWCDMA_H
/*=========================================================================== 

                           C U S T    W C D M A 

DESCRIPTION
  Customer file for WCDMA

===========================================================================*/


/* #define FEATURE_TMC_TCXOMGR */

#ifdef FEATURE_HSDPA_ACCESS_STRATUM
  #define FEATURE_REL5
  /* Make CQI estimation less pessimistic */
#endif


#endif /* CUSTWCDMA_H */
#ifndef CUSTUIM_H
#define CUSTUIM_H
/*===========================================================================

            " C u s t - R U I M "   H E A D E R   F I L E

DESCRIPTION
  Configuration for RUIM Feature.

===========================================================================*/


/* UIM Task Support
*/
#define FEATURE_UIM
/* #define FEATURE_UIM2 */

/* Support for ONCHIP SIM to allow for factory testing */
#define FEATURE_MMGSDI_ONCHIP_SIM

/* Switch to New SIM Lock Architecture */
#define FEATURE_MMGSDI_PERSONALIZATION_ENGINE

/*SIM Lock for GSM/UMTS only*/
#define FEATURE_PERSO_SIM

/* CDMA part of MMGSDI */
/* #define FEATURE_MMGSDI_CDMA */

#ifdef FEATURE_GSM
/* GSM part of MMGSDI */
#define FEATURE_MMGSDI_GSM
#endif /* FEATURE_GSM */

/* #define FEATURE_DOG */


/* MMGSDI supports multiple clients */
#define FEATURE_GSDI_MULTICLIENT_SUPPORT

/* Enable the GSDI/MMGSDI Interface to provide Detailed Error Events */
#define FEATURE_MMGSDI_CARD_ERROR_INFO

/*---------------------------------------------------------------------------
                            Debug Features
---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
                       GSM SIM support
---------------------------------------------------------------------------*/
#ifdef FEATURE_GSM

  /* The feature enables support for GSM specific UIM commands and responses.
  */
  #define  FEATURE_UIM_GSM

  /* Enables support for the USIM-specific UIM commands and responses.
  */
  #define  FEATURE_UIM_USIM

#endif /* FEATURE_GSM */

/* #define FEATURE_GPRS_CALLCONTROL */

/* Enables support for the USIM-specific UIM commands and responses.
*/
#define  FEATURE_UIM_USIM

/* Enables the support for Multiclient in GSDI
 */
#define FEATURE_GSDI_MULTICLIENT_SUPPORT

/* Enable MM GSDI
*/
#define FEATURE_MMGSDI_UMTS
#define FEATURE_CAT_REL6


/* Enables the UIM sub-configuration to use the BIO lines for the MSM5200
** SURF board.
*/
#define  FEATURE_UIM

/* The feature enables support for GSM specific UIM commands and responses.
*/
#define  FEATURE_UIM_GSM

/* Enables support for the USIM-specific UIM commands and responses.
*/
#define  FEATURE_UIM_USIM

/* ISIM Support */
#define FEATURE_UIM_ISIM

/* Feature for dual processor build to prevent these members from being accessed */
#define FEATURE_MMGSDI_NO_TCB_PTR_OR_CRIT_SEC

/* Features for 1000 PBM */
#define FEATURE_USIM_1000_PBM

#endif /* CUSTUIM_H */
#ifndef CUSTCGPS_H
#define CUSTCGPS_H
/*===========================================================================

            " C u s t - C G P S "   H E A D E R   F I L E

DESCRIPTION
  Configuration for GPS Feature.

  Copyright (c) 2006 - 2007

===========================================================================*/


/*---------------------------------------------------------------------------
                            gpsOne Features
---------------------------------------------------------------------------*/

#define FEATURE_CGPS

#ifdef FEATURE_CGPS

  #define FEATURE_GPSONE 

  #ifdef FEATURE_CGPS_USES_UMTS

    #define FEATURE_CM_LCS

  #endif  /* FEATURE_CGPS_USES_UMTS */

#endif /* FEATURE_CGPS */
#endif /* CUSTCGPS_H */
#ifndef CUSTDATACOMMON_H
#define CUSTDATACOMMON_H
/*===========================================================================

DESCRIPTION
  Configuration for DATACOMMON SU

===========================================================================*/


/*---------------------------------------------------------------------------
  Tier specific features
---------------------------------------------------------------------------*/
#ifdef FEATURE_MODEM_DATACOMMON_MODEM
#elif defined FEATURE_MODEM_DATACOMMON_ULC
#elif defined FEATURE_MODEM_DATACOMMON_QSC_LOW_TIER
#elif defined FEATURE_MODEM_DATACOMMON_8650_THINUI
#elif defined FEATURE_THIN_UI
#elif defined FEATURE_THIN_UI_NO_DATA_APPS
#else
  /*-------------------------------------------------------------------------
    Standard features for all high end targets
  -------------------------------------------------------------------------*/

  #if defined(FEATURE_GSM_GPRS) || defined(FEATURE_WCDMA)
    #define FEATURE_PS_DORMANT_PWR_SAVE
  #endif /* (GSM || WCDMA) */


#endif    /* else EITHER ... FEATURE_MODEM_DATACOMMON_ULC or
	     FEATURE_MODEM_DATACOMMON_MODEM or target-specific 8650..6800 */

  /* Unused features
   *#define FEATURE_DATA_BCAST_MCAST_DEPRECATED
   *#define FEATURE_DATA_RM_NET_MULTI
   *#define FEATURE_DATA_PS_IWLAN
   *#define FEATURE_DATA_PS_IWLAN_3GPP2
    #define FEATURE_PS_TCP_NO_DELAYED_ACK
    #define FEATURE_STA_QOS
   */
#endif /* CUSTDATACOMMON_H */

#endif /* CUST_________H */
#else
#endif

/* Now perform certain Sanity Checks on the various options and combinations
** of option.  Note that this list is probably NOT exhaustive, but just
** catches the obvious stuff.
*/


#endif /* CUSTOMER_H */
#endif

#endif /* TARGET_H */

#if defined FEATURE_L4  && !defined FEATURE_L4_KERNEL && \
    !defined BUILD_BOOT_CHAIN && !defined BUILD_BOOT_CHAIN_OEMSBL
  #ifndef _ARM_ASM_
    #ifdef FEATURE_QUARTZ_135
/*===========================================================================

     QUALCOMM Confidential and Proprietary
===========================================================================*/

#ifndef __MSM_SYSCALL_H__
#define __MSM_SYSCALL_H__
/*===========================================================================

              M S M    S Y S C A L L    H E A D E R    F I L E

DESCRIPTION
   This file contains the functions in assembly, which are equivelent
   to those inline assembly functions defined in msm_syscall.h

===========================================================================*/

#ifndef KXCACHE_H
#define KXCACHE_H

/*====================================================================
  FILE:         KxCache.h

  SERVICES:     Quartz cache management

  DESCRIPTION:  This API gives access to the cache functionality
                allowing various operations such as flushing and
                invalidating the cache at various granularity levels.


  Copyright (c) 2006 QUALCOMM Incorporated.

 =====================================================================
 *$Header: //source/qcom/qct/platform/linux/targets/main/latest/msm8650/kernel/remote_apis/charger/inc/charger.h#3 $ */

 
/*
 * stdlib.h declares four types, several general purpose functions,
 * and defines several macros.
 */

#ifndef __stdlib_h
#define __stdlib_h


  #ifndef __STDLIB_DECLS
  #define __STDLIB_DECLS


#undef NULL
#define NULL 0                   /* see <stddef.h> */


   /*
    * Defining __USE_ANSI_EXAMPLE_RAND at compile time switches to
    * the example implementation of rand() and srand() provided in
    * the ANSI C standard. This implementation is very poor, but is
    * provided for completeness.
    */
#ifdef __USE_ANSI_EXAMPLE_RAND
#define rand _ANSI_rand
#else
#endif
   /*
    * RAND_MAX: an integral constant expression, the value of which
    * is the maximum value returned by the rand function.
    */
  #endif /* __STDLIB_DECLS */


#endif
/* end of stdlib.h */

#endif /* #ifdef KXCACHE_H */

#endif  /* __MSM_SYSCALL_H__ */
    #else /* FEATURE_QUARTZ_135 */
    #endif /* FEATURE_QUARTZ_135 */
  #endif /* _ARM_ASM_ */
#endif /* FEATURE_L4 && !FEATURE_L4_KERNEL &&
          !FEATURE_L4_KERNEL && !BUILD_BOOT_CHAIN_OEMSBL */

/* ------------------------------------------------------------------------
** Constants
** ------------------------------------------------------------------------ */

#ifdef TRUE
#undef TRUE
#endif

#ifdef FALSE
#undef FALSE
#endif

#define TRUE   1   /* Boolean true value. */
#define FALSE  0   /* Boolean false value. */

#define  ON   1    /* On value. */
#define  OFF  0    /* Off value. */

#ifdef _lint
  #define NULL 0
#endif

#ifndef NULL
  #define NULL  0
#endif

/* -----------------------------------------------------------------------
** Standard Types
** ----------------------------------------------------------------------- */

/* The following definitions are the same accross platforms.  This first
** group are the sanctioned types.
*/
#ifndef _ARM_ASM_
typedef  unsigned char      boolean;     /* Boolean value type. */

typedef  unsigned long int  uint32;      /* Unsigned 32 bit value */
typedef  unsigned short     uint16;      /* Unsigned 16 bit value */
typedef  unsigned char      uint8;       /* Unsigned 8  bit value */

typedef  signed long int    int32;       /* Signed 32 bit value */
typedef  signed short       int16;       /* Signed 16 bit value */
typedef  signed char        int8;        /* Signed 8  bit value */

/* This group are the deprecated types.  Their use should be
** discontinued and new code should use the types above
*/
typedef  unsigned char     byte;         /* Unsigned 8  bit value type. */
typedef  unsigned short    word;         /* Unsinged 16 bit value type. */
typedef  unsigned long     dword;        /* Unsigned 32 bit value type. */

typedef  unsigned char     uint1;        /* Unsigned 8  bit value type. */
typedef  unsigned short    uint2;        /* Unsigned 16 bit value type. */
typedef  unsigned long     uint4;        /* Unsigned 32 bit value type. */

typedef  signed char       int1;         /* Signed 8  bit value type. */
typedef  signed short      int2;         /* Signed 16 bit value type. */
typedef  long int          int4;         /* Signed 32 bit value type. */

typedef  signed long       sint31;       /* Signed 32 bit value */
typedef  signed short      sint15;       /* Signed 16 bit value */
typedef  signed char       sint7;        /* Signed 8  bit value */

#if (! defined T_WINNT) && (! defined TARGET_OS_SOLARIS)
#ifndef SWIG  /* The SWIG preprocessor gets confused by these */
  /* Non WinNT Targets
  */
  typedef  signed long long   int64;       /* Signed 64 bit value */
  typedef  unsigned long long uint64;      /* Unsigned 64 bit value */

#if defined(__ARMCC_VERSION) 
  #define PACKED __packed
#else  /* __GNUC__ */
#endif /* defined (__GNUC__) */

#endif /* SWIG */

#else /* T_WINNT || TARGET_OS_SOLARIS */
#endif /* T_WINNT */
#endif // #ifndef _ARM_ASM_

/* ----------------------------------------------------------------------
** Lint does not understand __packed, so we define it away here.  In the
** past we did this:
**   This helps us catch non-packed pointers accessing packed structures,
**   for example, (although lint thinks it is catching non-volatile pointers
**   accessing volatile structures).
**   This does assume that volatile is not being used with __packed anywhere
**   because that would make Lint see volatile volatile (grrr).
** but found it to be more trouble than it was worth as it would emit bogus
** errors
** ---------------------------------------------------------------------- */
#ifdef _lint
  #define __packed
#endif

/* ----------------------------------------------------------------------
**                          STANDARD MACROS
** ---------------------------------------------------------------------- */

#ifndef SWIG /* these confuse the SWIG preprocessor and aren't needed for it */

/*===========================================================================

MACRO MAX
MACRO MIN

DESCRIPTION
  Evaluate the maximum/minimum of 2 specified arguments.

PARAMETERS
  x     parameter to compare to 'y'
  y     parameter to compare to 'x'

DEPENDENCIES
  'x' and 'y' are referenced multiple times, and should remain the same
  value each time they are evaluated.

RETURN VALUE
  MAX   greater of 'x' and 'y'
  MIN   lesser of 'x' and 'y'

SIDE EFFECTS
  None

===========================================================================*/
#ifndef MAX
   #define  MAX( x, y ) ( ((x) > (y)) ? (x) : (y) )
#endif

#ifndef MIN
   #define  MIN( x, y ) ( ((x) < (y)) ? (x) : (y) )
#endif


#endif /* SWIG */

#endif  /* COMDEF_H */
typedef dword   rex_sigs_type;
#ifndef TIMETICK_H
#define TIMETICK_H
/*=============================================================================

                   T I M E   S E R V I C E   S U B S Y S T E M

GENERAL DESCRIPTION
  Implements time-keeping functions using the slow clock.

  Slow Clock Frequency          Granularity     Timer Range
    30.000kHz (lower limit)      33.3 us         39.7 hrs
    32.768kHz (nominal freq)     30.5 us         36.4 hrs
    60.000kHz (upper limit)      16.6 us         19.8 hrs


REGIONAL FUNCTIONS

  timetick_set_modem_app_sclk_offset(delta)
    Sets the modem/app sclk counter difference.  Call only from time*.c


EXTERNALIZED FUNCTIONS

  timetick_get
    Returns the current SLEEP_XTAL_TIMETICK counter's count.

  timetick_get_safe
    Returns the current SLEEP_XTAL_TIMETICK counter's count.
    Must be called from an INTLOCK'd context.

  timetick_get_diff
    Determines the time between two timeticks, in slow clocks, milliseconds,
    seconds, etc.

  timetick_get_elapsed
    Determines the time between a past timetick and now, in slow clocks,
    milliseconds, seconds, etc.

  timetick_get_ms
    Returns a monotonically increasing millisecond count, that is not
    related to system time or time-of-day.  Speed, not accuracy, is
    the focus of this function.

  timetick_cvt_to_sclk
    Converts a time value from seconds, milliseconds, etc to slow clocks

  timetick_cvt_from_sclk
    Converts a time value from slow clocks to seconds, milliseconds, etc

  timetick_sclk_to_prec_us
    Converts sclks to microseconds with precision and full range

INITIALIZATION AND SEQUENCING REQUIREMENTS
    None



=============================================================================*/


/*-----------------------------------------------------------------------------
  Time, in slow clock counts, from 0 to 0xFFFFFFFF (~1.5 days)
-----------------------------------------------------------------------------*/

typedef uint32                    timetick_type;

#endif /* TIMETICK_H */

#endif /* END REX_H */
#ifndef PMAPP_USB_H
#define PMAPP_USB_H

/*===========================================================================

              P M   A P P - U S B   A P P   H E A D E R   F I L E

DESCRIPTION
  This file contains functions prototypes and variable/type/constant 
  declarations for HS-USB application.
  
===========================================================================*/


/* =========================================================================
                         TYPE DEFINITIONS
========================================================================= */
// Remote A-dev type
typedef enum
{
   PM_APP_OTG_A_DEV_TYPE__USB_HOST,       // Standard USB host like a PC
   PM_APP_OTG_A_DEV_TYPE__USB_CARKIT,     // USB carkit as specified in CEA-936-A
   PM_APP_OTG_A_DEV_TYPE__USB_CHARGER,    // USB charger as specified in CEA-936-A
   PM_APP_OTG_A_DEV_TYPE__INVALID
} pm_app_otg_a_dev_type;

#endif /* PMAPP_USB_H */

/*===========================================================================

                      TYPE DEFINITIONS

===========================================================================*/
typedef enum
{
    CHG_UI_EVENT__IDLE,           /* Starting point, no charger.        */
    CHG_UI_EVENT__NO_POWER,       /* No/Weak Battery + Weak Charger.    */
    CHG_UI_EVENT__VERY_LOW_POWER, /* No/Weak Battery + Strong Charger.  */
    CHG_UI_EVENT__LOW_POWER,      /* Low Battery     + Strong Charger.  */
    CHG_UI_EVENT__NORMAL_POWER,   /* Enough Power for most applications.*/
    CHG_UI_EVENT__DONE,           /* Done charging, batt full.          */
    CHG_UI_EVENT__INVALID,
#ifdef FEATURE_L4LINUX
    CHG_UI_EVENT_MAX32 = 0x7fffffff  /* Pad enum to 32-bit int */
#endif
}chg_ui_event_type;

/* Generic type definition used to enable/disable charger functions */
typedef enum
{
    CHG_CMD_DISABLE,
    CHG_CMD_ENABLE,
    CHG_CMD_INVALID,
#ifdef FEATURE_L4LINUX
    CHG_CMD_MAX32 = 0x7fffffff  /* Pad enum to 32-bit int */
#endif
}chg_cmd_type;

/*===========================================================================

FUNCTION chg_is_charger_valid

DESCRIPTION
      This function returns TRUE/FALSE depending if a valid charger accessory 
    connected or not.
    
PARAMETERS
    None.    
        
DEPENDENCIES
  The following functions need to be called before this function:
  1) chg_init().
 
RETURN VALUE
  Return type: boolean.
  - TRUE: Valid charger accessory attached.
  - FALSE: Invalid or no charger accessory connected.
  
SIDE EFFECTS
    None.
===========================================================================*/
extern boolean chg_is_charger_valid(void);

/*===========================================================================

FUNCTION chg_is_battery_valid

DESCRIPTION
    This function returns if we have a valid battery connected or not.
    
PARAMETERS
    None.    
        
DEPENDENCIES
    Need to initialize the charger hardware driver and call chg_init() 
  before calling this function.
 
RETURN VALUE
    TRUE if the battery is connected.
  
SIDE EFFECTS
    None.
===========================================================================*/
extern boolean chg_is_battery_valid(void);

/*===========================================================================

FUNCTION chg_ui_event_read

DESCRIPTION
    This function returns the last charger UI event. The charger UI events
  indicate how much is available for the system.  
        
PARAMETERS
    None.    
        
DEPENDENCIES
    Need to initialize the charger hardware driver and call chg_init() 
  before calling this function.
 
RETURN VALUE
    CHG_UI_EVENT__IDLE           = Starting point, no charger.        
    CHG_UI_EVENT__NO_POWER       = No/Weak Battery + Weak Charger.       
    CHG_UI_EVENT__VERY_LOW_POWER = No/Weak Battery + Strong Charger.  
    CHG_UI_EVENT__LOW_POWER      = Low Battery     + Strong Charger.  
    CHG_UI_EVENT__NORMAL_POWER   = Enough Power for most applications.
    CHG_UI_EVENT__DONE           = Done charging, batt full.          
  
SIDE EFFECTS
    None.
===========================================================================*/
chg_ui_event_type chg_ui_event_read(void);

/*===========================================================================

FUNCTION chg_usb_charger_switch

DESCRIPTION
    This function enables/disables usb charging.  
        
PARAMETERS
    1) Parameter name: cmd.
     - Enable/disable USB charging.

     Parameter type: chg_cmd_type (enum).
     - Valid Inputs:
         CHG_CMD_DISABLE
         CHG_CMD_ENABLE
   
        
DEPENDENCIES
    Need to initialize the charger hardware driver and call chg_init() 
  before calling this function.
 
RETURN VALUE
    None.        
  
SIDE EFFECTS
    Interrupts are LOCKED while within this function.
===========================================================================*/
extern void chg_usb_charger_switch(chg_cmd_type cmd);

/*===========================================================================

FUNCTION  chg_usb_charger_connected

DESCRIPTION
    This function is a call back function registered with the PMIC USB
  driver code that is called by the SB Transceiver driver when a valid
  USB charger accessory is connected.
    
PARAMETERS
    1) Parameter Name: otg_dev.
       - USB charger accessory type.
     
       Parameter type: pm_app_otg_a_dev_type

DEPENDENCIES
  The function functions need to be called before calling this function:
  1) pm_init()
  2) chg_init()
  3) chg_usb_irq_switch()
  
RETURN VALUE
  None
  
SIDE EFFECTS
  None.
===========================================================================*/
extern void chg_usb_charger_connected(pm_app_otg_a_dev_type otg_dev);

/*===========================================================================

FUNCTION chg_usb_charger_disconnected

DESCRIPTION
    This function is called by the USB Transceiver driver. It indicates
  that the USB devise has been removed.

PARAMETERS
  None.
      
DEPENDENCIES
  The function functions need to be called before calling this function:
  1) chg_init()
  
RETURN VALUE
  None
  
SIDE EFFECTS
  None.
===========================================================================*/
extern void chg_usb_charger_disconnected(void);

/*===========================================================================

FUNCTION chg_usb_i_is_available

DESCRIPTION
    This function is a call back function registered with the USB 
  Transceiver driver that is called when we are allowed to draw current 
  from the USB charger accessory.
    
PARAMETERS
    1) Parameter Name: i_ma.
     - How much current we are allowed to draw from the USB charger 
       accessory.

     Parameter type: uint32
     - Valid Inputs:
       current in mA.

DEPENDENCIES
  The function functions need to be called before calling this function:
  1) pm_init()
  2) chg_init()
  3) chg_usb_irq_switch()
  
RETURN VALUE
  None
  
SIDE EFFECTS
  None.
===========================================================================*/
extern void chg_usb_i_is_available(uint32 i_ma);

/*===========================================================================

FUNCTION chg_usb_i_is_not_available

DESCRIPTION
    This function is called by the USB Transceiver driver. It indicates
  that the USB devise can not supply current. Because it is not able 
  (ex; suspend mode) or is not attached.
    
PARAMETERS
  None.

DEPENDENCIES
  The function functions need to be called before calling this function:
  1) chg_init()
  
RETURN VALUE
  None
  
SIDE EFFECTS
  None.
===========================================================================*/
extern void chg_usb_i_is_not_available(void);

/* LGE_CHANGE_S [ybw75@lge.com] 2008-05-22, Add RPC for sharing battery information */
//extern void chg_get_sb_info(smart_battery_info_type *sb_all_data);
/* LGE_CHANGE_E [ybw75@lge.com] 2008-05-22 */

#endif/* CHARGER_H */

#endif /* ! CHARGER_H_trimmed */
