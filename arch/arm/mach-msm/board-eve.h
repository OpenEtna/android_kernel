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
#define MSM_PMEM_MDP_SIZE         (12*SZ_1M)

#define MSM_PMEM_ADSP_BASE        (MSM_PMEM_MDP_BASE + MSM_PMEM_MDP_SIZE)
#define MSM_PMEM_ADSP_SIZE        (8*SZ_1M)

#define MSM_PMEM_GPU1_BASE        (MSM_PMEM_ADSP_BASE + MSM_PMEM_ADSP_SIZE)
#define MSM_PMEM_GPU1_SIZE        (8*SZ_1M)

#define MSM_LINUX_BASE            MSM_EBI_BASE
#define MSM_LINUX_SIZE            (223*SZ_1M - MSM_PMEM_MDP_SIZE-MSM_PMEM_ADSP_SIZE-MSM_PMEM_GPU1_SIZE)

#define MSM_RAM_CONSOLE_BASE      (MSM_PMEM_GPU1_SIZE+MSM_PMEM_GPU1_BASE)
#define MSM_RAM_CONSOLE_SIZE      (256 * SZ_1K)

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

/* Camera */
#define GPIO_CAMERA_I2C_SDA		61
#define GPIO_CAMERA_I2C_SCL		60

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

/* vibrator */
extern void eve_vibrator_set(int timeout);
#define MSM_WEB_BASE          0xE100C000
#define MSM_WEB_PHYS          0xA9D00000
#define MSM_WEB_SIZE          SZ_4K

/* panel */
#define GPIO_LCD_RESET_N            100
#define GPIO_LCD_VSYNC_O            97

/* mmc */
#define GPIO_MMC_CD_N 30

/* wifi */
#define BCM4325_GPIO_BT_RESET		93
#define GPIO_BT_HOST_WAKE			83
#define GPIO_BT_WAKE				92
#define BCM4325_GPIO_WL_RESET		35
#define BCM4325_GPIO_WL_WAKE		153
#define BCM4325_GPIO_WL_REGON		21
#define BCM4325_GPIO_WL_HOSTWAKEUP	94

#define EVE_GPIO_WIFI_IRQ BCM4325_GPIO_WL_WAKE

#define CUSTOMER_CMD2_BATT_GET_HW_REV	0x0010
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

#define GPIO_BT_RESET_N         93
#define GPIO_BT_WAKEUP          92
#define GPIO_BT_HOST_WAKEUP     83
#define GPIO_BT_REG_ON          21 //19
#define GPIO_WL_RESET_N         35

/* battery/charger */
void notify_usb_connected(int online);

#endif //__ARCH_ARM_MACH_MSM_BOARD_EVE_H

int eve_backlight_init(void);
