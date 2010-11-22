#ifndef __ARCH_ARM_MACH_MSM_BOARD_EVE_H
#define __ARCH_ARM_MACH_MSM_BOARD_EVE_H

#define MSM_SMI_BASE        0x00100000
#define MSM_SMI_SIZE        0x00800000

#define MSM_PMEM_GPU0_BASE  MSM_SMI_BASE
#define MSM_PMEM_GPU0_SIZE  (7*SZ_1M)

//#define MSM_PMEM_GPU0_BASE      (0x10000000 + 64*SZ_1M)
//#define MSM_PMEM_GPU0_SIZE      0x800000

#define MSM_EBI_BASE            0x10000000
#define MSM_EBI_SIZE            0x06E00000

#define MSM_FB_BASE               (MSM_PMEM_GPU0_BASE+MSM_PMEM_GPU0_SIZE)
#define MSM_FB_SIZE               (1*SZ_1M)

#define MSM_PMEM_MDP_BASE         (MSM_LINUX_BASE + MSM_LINUX_SIZE)
#define MSM_PMEM_MDP_SIZE         (8*SZ_1M)

#define MSM_PMEM_ADSP_BASE        (MSM_PMEM_MDP_BASE + MSM_PMEM_MDP_SIZE)
#define MSM_PMEM_ADSP_SIZE        (13*SZ_1M)

#define MSM_PMEM_CAMERA_BASE      (MSM_PMEM_ADSP_BASE + MSM_PMEM_ADSP_SIZE)
#define MSM_PMEM_CAMERA_SIZE      (11*SZ_1M)

#define MSM_PMEM_GPU1_BASE        (MSM_PMEM_CAMERA_BASE + MSM_PMEM_CAMERA_SIZE)
#define MSM_PMEM_GPU1_SIZE        (8*SZ_1M)

#define MSM_LINUX_BASE            MSM_EBI_BASE
#define MSM_LINUX_SIZE            (223*SZ_1M - MSM_PMEM_MDP_SIZE-MSM_PMEM_ADSP_SIZE-MSM_PMEM_CAMERA_SIZE-MSM_PMEM_GPU1_SIZE)

#define MSM_RAM_CONSOLE_BASE      (MSM_PMEM_GPU1_SIZE+MSM_PMEM_GPU1_BASE)
#define MSM_RAM_CONSOLE_SIZE      (128 * SZ_1K)

/* I2C Bus Num */
#define I2C_BUS_NUM_MOTION		1
#define I2C_BUS_NUM_BACKLIGHT	2
#define I2C_BUS_NUM_CAMERA		3
#define I2C_BUS_NUM_AMP			4
#define I2C_BUS_NUM_TOUCH		5
#define I2C_BUS_NUM_PROX		6
#define I2C_BUS_NUM_COMPASS		7

/* For Backlight */
#define GPIO_BL_I2C_SCL			84
#define GPIO_BL_I2C_SDA			85

/* Proximity Sensor */
#define GPIO_PROX_I2C_SDA		89
#define GPIO_PROX_I2C_SCL		90
#define GPIO_PROX_IRQ			88

/* Capacitive Touch (Menu/Back button)*/
#define GPIO_TOUCH_IRQ			20
#define GPIO_TOUCH_I2C_SDA		41
#define GPIO_TOUCH_I2C_SCL		42

/*Motion Sensor*/
#define GPIO_MOTION_I2C_SDA		23
#define GPIO_MOTION_I2C_SCL		33
#define GPIO_MOTION_IRQ			49

#define GPIO_CAMERA_I2C_SDA		61
#define GPIO_CAMERA_I2C_SCL		60

#define GPIO_AMP_I2C_SCL		27
#define GPIO_AMP_I2C_SDA		17

#define GPIO_COMPASS_I2C_SCL	1
#define GPIO_COMPASS_I2C_SDA	2
#define GPIO_COMPASS_IRQ		80
#define GPIO_COMPASS_RESET	91
#define COMPASS_LAYOUTS {                \
    { {-1,  0, 0}, { 0, -1,  0}, {0, 0,  1} }, \
    { { 0, -1, 0}, { 1,  0,  0}, {0, 0, -1} }, \
    { { 0, -1, 0}, { 1,  0,  0}, {0, 0,  1} }, \
    { {-1,  0, 0}, { 0,  0, -1}, {0, 1,  0} }  \
}

/* qwerty keyboard */
#define GPIO_PP2106M2_I2C_SDA	38
#define GPIO_PP2106M2_I2C_SCL	37
#define GPIO_PP2106M2_RESET		36
#define GPIO_PP2106M2_IRQ		39

/* panel */
#define GPIO_LCD_RESET_N            100
#define GPIO_LCD_VSYNC_O            97

#endif //__ARCH_ARM_MACH_MSM_BOARD_EVE_H
