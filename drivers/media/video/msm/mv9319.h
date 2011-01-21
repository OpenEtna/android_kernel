/*
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

#ifndef CAMSENSOR_MV9319_SONY
#define CAMSENSOR_MV9319_SONY

#include <mach/board.h>
#include <mach/camera.h>
#include "mv9319_firmware_4_0_2.h"

#define MV9319_PID_REG  0x00
#define MV9319_VER_REG  0x01
#define MV9319_PID      0x93
#define MV9319_VER      0x19

#define MV9319_FORMAT_YUV_422 1
#define MV9319_FORMAT_RGB565  2
#define MV9319_FORMAT_JPG     3

/* MV9319 MCU REGISTER ADDRESS */
#define MV9319_CMD_VER_MAJOR            0x02
#define MV9319_CMD_VER_MINOR            0x03
#define MV9319_CMD_FORMAT               0x07
#define MV9319_CMD_ZOOM                 0x1B
#define MV9319_CMD_JPG_BUFFER_SIZE      0x2C
#define MV9319_CMD_JPG_Q_FACTOR         0x2B
#define MV9319_CMD_STATUS_SET_FORMAT    0x2F
#define MV9319_CMD_AF_CONTROL           0x19
#define MV9319_CMD_AF_STATUS            0x30
#define MV9319_CMD_MF_CONTROL           0x1A
#define MV9319_CMD_FPS                  0x0F
#define MV9319_CMD_WB                   0x14
#define MV9319_CMD_EV                   0x0E
#define MV9319_CMD_FLASH                0xA
#define MV9319_CMD_EFFECT_CONTORL       0x1F
#define MV9319_CMD_SCENE_MODE           0x25
#define MV9319_CMD_ISO                  0x0D
#define MV9319_CMD_ORIENTATION          0x09
#define MV9319_CMD_PWDN                 0xE7
#define MV9319_CMD_CURRENT_LUMINANCE    0x97
#define MV9319_CMD_FW_CHECKSUM_MSB      0x93
#define MV9319_CMD_FW_CHECKSUM_LSB      0x94
#define MV9319_CMD_FLICKERLESS_CONTROL  0x11

/* MV9319 CAMERA MODE SIZE FORMAT */
#define MV9319_PREVIEW_MODE         0x2
#define MV9319_CAPTURE_MODE         0x3
#define MV9319_CAMERA_MODE_SHIFT    0x6
#define MV9319_SET_PREVIEW_MODE ((MV9319_PREVIEW_MODE & 0x3) << MV9319_CAMERA_MODE_SHIFT)
#define MV9319_SET_CAPTURE_MODE ((MV9319_CAPTURE_MODE & 0x3) << MV9319_CAMERA_MODE_SHIFT)
#define MV9319_SET_FORMAT_YUV       (0x00 << 4)
#define MV9319_SET_FORMAT_JPG       (0x01 << 4)
#define MV9319_SET_FORMAT_RGB565    (0x02 << 4)
#define MV9319_CAMERA_MODE_MASK     0x3F
#define MV9319_CAMERA_FORMAT_MASK   0xCF
#define MV9319_CAMERA_SIZE_MASK     0xF0

//#define MV9319_F_DEFAULT    0x01
#define MV9319_F_QVGA       0x05
#define MV9319_F_HVGA       0x06
#define MV9319_F_VGA        0x07
//#define MV9319_F_WVGA       0x08
#define MV9319_F_QUADVGA    0x0B
#define MV9319_F_UXGA       0x0D
#define MV9319_F_QXGA       0x0E
#define MV9319_F_QSXGA      0x0F

/* MV9319 FORMAT SUCCESS OR FAIL */
#define MV9319_FORMAT_SUCCESS   0xBB

/* MV9319 AF */
#define MV9319_MACRO_AF     0x00
#define MV9319_AUTO_AF      0x01
#define MV9319_AF_SUCCESS   0xBB
#define MV9319_AF_FAIL      0xCB

/* MV9319 Effect */
#define MV9319_EFFECT_NORMAL    0x00
#define MV9319_EFFECT_SKETCH    0x01
#define MV9319_EFFECT_EMBOSS    0x02
#define MV9319_EFFECT_SEPIA     0x04
#define MV9319_EFFECT_RED       0x05
#define MV9319_EFFECT_GREEN     0x06
#define MV9319_EFFECT_NNP       0x08
#define MV9319_EFFECT_BNW       0x09
#define MV9319_EFFECT_AQUA      0x0B
#define MV9319_EFFECT_SOLARIZE  0x0D

/* MV9319 WHITEBALANCE */
#define MV9319_WB_DAYLIGHT      0x01
#define MV9319_WB_FLUORESCENT   0x02
#define MV9319_WB_INCANDESCENT  0x04
#define MV9319_WB_CLOUDY        0x06
#define MV9319_WB_AUTO_START    0xC0
#define MV9319_WB_AUTO_STOP     0x00

/*MV9319 EXPOSURE */
#define MV9319_EV_DEFAULT 0x0
#define MV9319_EV_P_1 0x01
#define MV9319_EV_P_2 0x02
#define MV9319_EV_P_3 0x03
#define MV9319_EV_P_4 0x04
#define MV9319_EV_P_5 0x05
#define MV9319_EV_P_6 0x06
#define MV9319_EV_N_1 0x81
#define MV9319_EV_N_2 0x82
#define MV9319_EV_N_3 0x83
#define MV9319_EV_N_4 0x84
#define MV9319_EV_N_5 0x85
#define MV9319_EV_N_6 0x86

/* MV9319_ISO */
#define MV9319_ISO_AUTO 0x00
#define MV9319_ISO_100  0x01
#define MV9319_ISO_200  0x02
#define MV9319_ISO_400  0x03

/* MV9319 Scene mode */
#define MV9319_SCENE_NORMAL         0x00
#define MV9319_SCENE_NIGHT          0x02
#define MV9319_SCENE_BACKLIGHT      0x03
#define MV9319_SCENE_LANDSCAPE      0x04
#define MV9319_SCENE_PORTRAIT       0x05
#define MV9319_SCENE_NIGHT_PORTRAIT 0x06
#define MV9319_SCENE_BEACH          0x07
#define MV9319_SCENE_PARTY          0x08
#define MV9319_SCENE_SPORT          0x09

#define MV9319_FLICKERLESS_60HZ     0x01
#define MV9319_FLICKERLESS_50HZ     0x02

#define MV9319_FPS_AUTO 0x01
#define MV9319_FPS_7    0x0B
#define MV9319_FPS_10   0x0C
#define MV9319_FPS_15   0x0D
#define MV9319_FPS_30   0x0A
#define MV9319_FPS_60   0x0E
#define MV9319_FPS_90   0x0F

/*MV9319 POWER DOWN */
#define MV9319_POWER_DOWN 0x01

#define swab16_new(x) ((unsigned short)((((unsigned short)(x) & (unsigned short)0x00ff)<<8) | (((unsigned short)(x) & (unsigned short)0xff00)>>8)))

#endif /* CAMSENSOR_MV9319_SONY */
