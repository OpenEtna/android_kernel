/* linux/arch/arm/mach-msm/gpio.c
 *
 * Copyright (C) 2007 Google, Inc.
 * Copyright (c) 2009, Code Aurora Forum. All rights reserved.
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

#include <asm/io.h>
#include <asm/gpio.h>
#include <linux/irq.h>
#include <linux/module.h>
#include <linux/debugfs.h>
#include "gpio_chip.h"
#include "gpio_hw.h"
#include "proc_comm.h"

#include "smd_private.h"

/* LGE_CHANGE_S [ybw75@lge.com] 2009-03-19, Add test mode for changing GPIO setting during sleep */
#include <mach/charger.h>	// for feature definition : FEATURE_TEST_FOR_SLEEP_GPIO_SETTING

int    test_get_gpio_num;
uint32 test_get_gpio_sleep_config;
int    test_set_gpio_num;
uint32 test_set_gpio_sleep_config;

extern int msm_chg_lge_get_test_sleep_gpio_config( int gpio_number );						// chg_rpc_function
extern int msm_chg_lge_set_test_sleep_gpio_config( int gpio_number, int gpio_config );		// chg_rpc_function
/* LGE_CHANGE_E [ybw75@lge.com] 2009-03-19 */
enum {
	GPIO_DEBUG_SLEEP = 1U << 0,
};
static int msm_gpio_debug_mask = 0;
module_param_named(debug_mask, msm_gpio_debug_mask, int, S_IRUGO | S_IWUSR | S_IWGRP);

#define MSM_GPIO_BROKEN_INT_CLEAR 1

/* private gpio_configure flags */
#define MSM_GPIOF_ENABLE_INTERRUPT      0x10000000
#define MSM_GPIOF_DISABLE_INTERRUPT     0x20000000
#define MSM_GPIOF_ENABLE_WAKE           0x40000000
#define MSM_GPIOF_DISABLE_WAKE          0x80000000

static int msm_gpio_configure(struct gpio_chip *chip, unsigned int gpio, unsigned long flags);
static int msm_gpio_get_irq_num(struct gpio_chip *chip, unsigned int gpio, unsigned int *irqp, unsigned long *irqnumflagsp);
static int msm_gpio_read(struct gpio_chip *chip, unsigned n);
static int msm_gpio_write(struct gpio_chip *chip, unsigned n, unsigned on);
static int msm_gpio_read_detect_status(struct gpio_chip *chip, unsigned int gpio);
static int msm_gpio_clear_detect_status(struct gpio_chip *chip, unsigned int gpio);

/* LGE_CHANGE [dojip.kim@lge.com] 2010-03-23, definitions  */
#if defined(CONFIG_MACH_EVE)
static int gpio_check_range(u64 number);
#endif

struct msm_gpio_regs
{
	void __iomem *out;
	void __iomem *in;
	void __iomem *int_status;
	void __iomem *int_clear;
	void __iomem *int_en;
	void __iomem *int_edge;
	void __iomem *int_pos;
	void __iomem *oe;
};

struct msm_gpio_chip {
	struct gpio_chip        chip;
	struct msm_gpio_regs    regs;
#if MSM_GPIO_BROKEN_INT_CLEAR
	unsigned                int_status_copy;
#endif
	unsigned int            both_edge_detect;
	unsigned int            int_enable[2]; /* 0: awake, 1: sleep */
};

struct msm_gpio_chip msm_gpio_chips[] = {
	{
		.regs = {
			.out =         GPIO_OUT_0,
			.in =          GPIO_IN_0,
			.int_status =  GPIO_INT_STATUS_0,
			.int_clear =   GPIO_INT_CLEAR_0,
			.int_en =      GPIO_INT_EN_0,
			.int_edge =    GPIO_INT_EDGE_0,
			.int_pos =     GPIO_INT_POS_0,
			.oe =          GPIO_OE_0,
		},
		.chip = {
			.start = 0,
			.end = 15,
			.configure = msm_gpio_configure,
			.get_irq_num = msm_gpio_get_irq_num,
			.read = msm_gpio_read,
			.write = msm_gpio_write,
			.read_detect_status = msm_gpio_read_detect_status,
			.clear_detect_status = msm_gpio_clear_detect_status
		}
	},
	{
		.regs = {
			.out =         GPIO_OUT_1,
			.in =          GPIO_IN_1,
			.int_status =  GPIO_INT_STATUS_1,
			.int_clear =   GPIO_INT_CLEAR_1,
			.int_en =      GPIO_INT_EN_1,
			.int_edge =    GPIO_INT_EDGE_1,
			.int_pos =     GPIO_INT_POS_1,
			.oe =          GPIO_OE_1,
		},
		.chip = {
			.start = 16,
#if defined(CONFIG_ARCH_MSM7X30)
			.end = 43,
#else
			.end = 42,
#endif
			.configure = msm_gpio_configure,
			.get_irq_num = msm_gpio_get_irq_num,
			.read = msm_gpio_read,
			.write = msm_gpio_write,
			.read_detect_status = msm_gpio_read_detect_status,
			.clear_detect_status = msm_gpio_clear_detect_status
		}
	},
	{
		.regs = {
			.out =         GPIO_OUT_2,
			.in =          GPIO_IN_2,
			.int_status =  GPIO_INT_STATUS_2,
			.int_clear =   GPIO_INT_CLEAR_2,
			.int_en =      GPIO_INT_EN_2,
			.int_edge =    GPIO_INT_EDGE_2,
			.int_pos =     GPIO_INT_POS_2,
			.oe =          GPIO_OE_2,
		},
		.chip = {
#if defined(CONFIG_ARCH_MSM7X30)
			.start = 44,
#else
			.start = 43,
#endif
			.end = 67,
			.configure = msm_gpio_configure,
			.get_irq_num = msm_gpio_get_irq_num,
			.read = msm_gpio_read,
			.write = msm_gpio_write,
			.read_detect_status = msm_gpio_read_detect_status,
			.clear_detect_status = msm_gpio_clear_detect_status
		}
	},
	{
		.regs = {
			.out =         GPIO_OUT_3,
			.in =          GPIO_IN_3,
			.int_status =  GPIO_INT_STATUS_3,
			.int_clear =   GPIO_INT_CLEAR_3,
			.int_en =      GPIO_INT_EN_3,
			.int_edge =    GPIO_INT_EDGE_3,
			.int_pos =     GPIO_INT_POS_3,
			.oe =          GPIO_OE_3,
		},
		.chip = {
			.start = 68,
			.end = 94,
			.configure = msm_gpio_configure,
			.get_irq_num = msm_gpio_get_irq_num,
			.read = msm_gpio_read,
			.write = msm_gpio_write,
			.read_detect_status = msm_gpio_read_detect_status,
			.clear_detect_status = msm_gpio_clear_detect_status
		}
	},
	{
		.regs = {
			.out =         GPIO_OUT_4,
			.in =          GPIO_IN_4,
			.int_status =  GPIO_INT_STATUS_4,
			.int_clear =   GPIO_INT_CLEAR_4,
			.int_en =      GPIO_INT_EN_4,
			.int_edge =    GPIO_INT_EDGE_4,
			.int_pos =     GPIO_INT_POS_4,
			.oe =          GPIO_OE_4,
		},
		.chip = {
			.start = 95,
#if defined(CONFIG_ARCH_QSD8X50)
			.end = 103,
#else
			.end = 106,
#endif
			.configure = msm_gpio_configure,
			.get_irq_num = msm_gpio_get_irq_num,
			.read = msm_gpio_read,
			.write = msm_gpio_write,
			.read_detect_status = msm_gpio_read_detect_status,
			.clear_detect_status = msm_gpio_clear_detect_status
		}
	},
	{
		.regs = {
			.out =         GPIO_OUT_5,
			.in =          GPIO_IN_5,
			.int_status =  GPIO_INT_STATUS_5,
			.int_clear =   GPIO_INT_CLEAR_5,
			.int_en =      GPIO_INT_EN_5,
			.int_edge =    GPIO_INT_EDGE_5,
			.int_pos =     GPIO_INT_POS_5,
			.oe =          GPIO_OE_5,
		},
		.chip = {
#if defined(CONFIG_ARCH_QSD8X50)
			.start = 104,
			.end = 121,
#elif defined(CONFIG_ARCH_MSM7X30)
			.start = 107,
			.end = 133,
#else
			.start = 107,
			.end = 132,
#endif
			.configure = msm_gpio_configure,
			.get_irq_num = msm_gpio_get_irq_num,
			.read = msm_gpio_read,
			.write = msm_gpio_write,
			.read_detect_status = msm_gpio_read_detect_status,
			.clear_detect_status = msm_gpio_clear_detect_status
		}
	},
#if defined(CONFIG_ARCH_MSM_SCORPION)
	{
		.regs = {
			.out =         GPIO_OUT_6,
			.in =          GPIO_IN_6,
			.int_status =  GPIO_INT_STATUS_6,
			.int_clear =   GPIO_INT_CLEAR_6,
			.int_en =      GPIO_INT_EN_6,
			.int_edge =    GPIO_INT_EDGE_6,
			.int_pos =     GPIO_INT_POS_6,
			.oe =          GPIO_OE_6,
		},
		.chip = {
#if defined(CONFIG_ARCH_MSM7X30)
			.start = 134,
			.end = 150,
#else
			.start = 122,
			.end = 152,
#endif
			.configure = msm_gpio_configure,
			.get_irq_num = msm_gpio_get_irq_num,
			.read = msm_gpio_read,
			.write = msm_gpio_write,
			.read_detect_status = msm_gpio_read_detect_status,
			.clear_detect_status = msm_gpio_clear_detect_status
		}
	},
	{
		.regs = {
			.out =         GPIO_OUT_7,
			.in =          GPIO_IN_7,
			.int_status =  GPIO_INT_STATUS_7,
			.int_clear =   GPIO_INT_CLEAR_7,
			.int_en =      GPIO_INT_EN_7,
			.int_edge =    GPIO_INT_EDGE_7,
			.int_pos =     GPIO_INT_POS_7,
			.oe =          GPIO_OE_7,
		},
		.chip = {
#if defined(CONFIG_ARCH_MSM7X30)
			.start = 151,
			.end = 181,
#else
			.start = 153,
			.end = 164,
#endif
			.configure = msm_gpio_configure,
			.get_irq_num = msm_gpio_get_irq_num,
			.read = msm_gpio_read,
			.write = msm_gpio_write,
			.read_detect_status = msm_gpio_read_detect_status,
			.clear_detect_status = msm_gpio_clear_detect_status
		}
	},
#endif
};

static void msm_gpio_update_both_edge_detect(struct msm_gpio_chip *msm_chip)
{
	int loop_limit = 100;
	unsigned pol, val, val2, intstat;
	do {
		val = readl(msm_chip->regs.in);
		pol = readl(msm_chip->regs.int_pos);
		pol = (pol & ~msm_chip->both_edge_detect) | (~val & msm_chip->both_edge_detect);
		writel(pol, msm_chip->regs.int_pos);
		intstat = readl(msm_chip->regs.int_status);
		val2 = readl(msm_chip->regs.in);
		if (((val ^ val2) & msm_chip->both_edge_detect & ~intstat) == 0)
			return;
	} while (loop_limit-- > 0);
	printk(KERN_ERR "msm_gpio_update_both_edge_detect, failed to reach stable state %x != %x\n", val, val2);
}

static int msm_gpio_write(struct gpio_chip *chip, unsigned n, unsigned on)
{
	struct msm_gpio_chip *msm_chip = container_of(chip, struct msm_gpio_chip, chip);
	unsigned b = 1U << (n - chip->start);
	unsigned v;

	v = readl(msm_chip->regs.out);
	if (on) {
		writel(v | b, msm_chip->regs.out);
	} else {
		writel(v & (~b), msm_chip->regs.out);
	}
	return 0;
}

static int msm_gpio_read(struct gpio_chip *chip, unsigned n)
{
	struct msm_gpio_chip *msm_chip = container_of(chip, struct msm_gpio_chip, chip);
	unsigned b = 1U << (n - chip->start);

	return (readl(msm_chip->regs.in) & b) ? 1 : 0;
}

static int msm_gpio_read_detect_status(struct gpio_chip *chip, unsigned int gpio)
{
	struct msm_gpio_chip *msm_chip = container_of(chip, struct msm_gpio_chip, chip);
	unsigned b = 1U << (gpio - chip->start);
	unsigned v;

	v = readl(msm_chip->regs.int_status);
#if MSM_GPIO_BROKEN_INT_CLEAR
	v |= msm_chip->int_status_copy;
#endif
	return (v & b) ? 1 : 0;
}

static int msm_gpio_clear_detect_status(struct gpio_chip *chip, unsigned int gpio)
{
	struct msm_gpio_chip *msm_chip = container_of(chip, struct msm_gpio_chip, chip);
	unsigned b = 1U << (gpio - chip->start);

#if MSM_GPIO_BROKEN_INT_CLEAR
	/* Save interrupts that already triggered before we loose them. */
	/* Any interrupt that triggers between the read of int_status */
	/* and the write to int_clear will still be lost though. */
	msm_chip->int_status_copy |= readl(msm_chip->regs.int_status);
	msm_chip->int_status_copy &= ~b;
#endif
	writel(b, msm_chip->regs.int_clear);
	msm_gpio_update_both_edge_detect(msm_chip);
	return 0;
}

int msm_gpio_configure(struct gpio_chip *chip, unsigned int gpio, unsigned long flags)
{
	struct msm_gpio_chip *msm_chip = container_of(chip, struct msm_gpio_chip, chip);
	unsigned b = 1U << (gpio - chip->start);
	unsigned v;

	if (flags & (GPIOF_OUTPUT_LOW | GPIOF_OUTPUT_HIGH))
		msm_gpio_write(chip, gpio, flags & GPIOF_OUTPUT_HIGH);

	if (flags & (GPIOF_INPUT | GPIOF_DRIVE_OUTPUT)) {
		v = readl(msm_chip->regs.oe);
		if (flags & GPIOF_DRIVE_OUTPUT) {
			writel(v | b, msm_chip->regs.oe);
		} else {
			writel(v & (~b), msm_chip->regs.oe);
		}
	}

	if (flags & (IRQF_TRIGGER_MASK | GPIOF_IRQF_TRIGGER_NONE)) {
		v = readl(msm_chip->regs.int_edge);
		if (flags & (IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING)) {
			writel(v | b, msm_chip->regs.int_edge);
			irq_desc[MSM_GPIO_TO_INT(gpio)].handle_irq = handle_edge_irq;
		} else {
			writel(v & (~b), msm_chip->regs.int_edge);
			irq_desc[MSM_GPIO_TO_INT(gpio)].handle_irq = handle_level_irq;
		}
		if ((flags & (IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING)) == (IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING)) {
			msm_chip->both_edge_detect |= b;
			msm_gpio_update_both_edge_detect(msm_chip);
		} else {
			msm_chip->both_edge_detect &= ~b;
			v = readl(msm_chip->regs.int_pos);
			if (flags & (IRQF_TRIGGER_RISING | IRQF_TRIGGER_HIGH)) {
				writel(v | b, msm_chip->regs.int_pos);
			} else {
				writel(v & (~b), msm_chip->regs.int_pos);
			}
		}
	}

	/* used by msm_gpio_irq_mask and msm_gpio_irq_unmask */
	if (flags & (MSM_GPIOF_ENABLE_INTERRUPT | MSM_GPIOF_DISABLE_INTERRUPT)) {
		v = readl(msm_chip->regs.int_edge);
		/* level triggered interrupts are also latched */
		if (!(v & b))
			msm_gpio_clear_detect_status(chip, gpio);
		if (flags & MSM_GPIOF_ENABLE_INTERRUPT) {
			msm_chip->int_enable[0] |= b;
		} else {
			msm_chip->int_enable[0] &= ~b;
		}
		writel(msm_chip->int_enable[0], msm_chip->regs.int_en);
	}

	if (flags & (MSM_GPIOF_ENABLE_WAKE | MSM_GPIOF_DISABLE_WAKE)) {
		if (flags & MSM_GPIOF_ENABLE_WAKE)
			msm_chip->int_enable[1] |= b;
		else
			msm_chip->int_enable[1] &= ~b;
	}

	return 0;
}

static int msm_gpio_get_irq_num(struct gpio_chip *chip, unsigned int gpio, unsigned int *irqp, unsigned long *irqnumflagsp)
{
	*irqp = MSM_GPIO_TO_INT(gpio);
	if (irqnumflagsp)
		*irqnumflagsp = 0;
	return 0;
}


static void msm_gpio_irq_ack(unsigned int irq)
{
	gpio_clear_detect_status(irq - NR_MSM_IRQS);
}

static void msm_gpio_irq_mask(unsigned int irq)
{
	gpio_configure(irq - NR_MSM_IRQS, MSM_GPIOF_DISABLE_INTERRUPT);
}

static void msm_gpio_irq_unmask(unsigned int irq)
{
	gpio_configure(irq - NR_MSM_IRQS, MSM_GPIOF_ENABLE_INTERRUPT);
}

static int msm_gpio_irq_set_wake(unsigned int irq, unsigned int on)
{
	return gpio_configure(irq - NR_MSM_IRQS, on ? MSM_GPIOF_ENABLE_WAKE : MSM_GPIOF_DISABLE_WAKE);
}


static int msm_gpio_irq_set_type(unsigned int irq, unsigned int flow_type)
{
	return gpio_configure(irq - NR_MSM_IRQS, flow_type);
}

static void msm_gpio_irq_handler(unsigned int irq, struct irq_desc *desc)
{
	int i, j, m;
	unsigned v;

	for (i = 0; i < ARRAY_SIZE(msm_gpio_chips); i++) {
		struct msm_gpio_chip *msm_chip = &msm_gpio_chips[i];
		v = readl(msm_chip->regs.int_status);
		v &= msm_chip->int_enable[0];
		while (v) {
			m = v & -v;
			j = fls(m) - 1;
			/* printk("msm_gpio_irq_handler %08x %08x bit %d gpio %d irq %d\n", v, m, j, msm_chip->chip.start + j, NR_MSM_IRQS + msm_chip->chip.start + j); */
			v &= ~m;
			generic_handle_irq(NR_MSM_IRQS + msm_chip->chip.start + j);
		}
	}
	desc->chip->ack(irq);
}

static struct irq_chip msm_gpio_irq_chip = {
	.name      = "msmgpio",
	.ack       = msm_gpio_irq_ack,
	.mask      = msm_gpio_irq_mask,
	.unmask    = msm_gpio_irq_unmask,
	.set_wake  = msm_gpio_irq_set_wake,
	.set_type  = msm_gpio_irq_set_type,
};

#define NUM_GPIO_SMEM_BANKS 6
#define GPIO_SMEM_NUM_GROUPS 2
#define GPIO_SMEM_MAX_PC_INTERRUPTS 8
struct tramp_gpio_smem
{
	uint16_t num_fired[GPIO_SMEM_NUM_GROUPS];
	uint16_t fired[GPIO_SMEM_NUM_GROUPS][GPIO_SMEM_MAX_PC_INTERRUPTS];
	uint32_t enabled[NUM_GPIO_SMEM_BANKS];
	uint32_t detection[NUM_GPIO_SMEM_BANKS];
	uint32_t polarity[NUM_GPIO_SMEM_BANKS];
};

static void msm_gpio_sleep_int(unsigned long arg)
{
	int i, j;
	struct tramp_gpio_smem *smem_gpio;

	BUILD_BUG_ON(NR_GPIO_IRQS > NUM_GPIO_SMEM_BANKS * 32);

	smem_gpio = smem_alloc(SMEM_GPIO_INT, sizeof(*smem_gpio)); 
	if (smem_gpio == NULL)
		return;

	local_irq_disable();
	for(i = 0; i < GPIO_SMEM_NUM_GROUPS; i++) {
		int count = smem_gpio->num_fired[i];
		for(j = 0; j < count; j++) {
			/* TODO: Check mask */
			generic_handle_irq(MSM_GPIO_TO_INT(smem_gpio->fired[i][j]));
		}
	}
	local_irq_enable();
}

static DECLARE_TASKLET(msm_gpio_sleep_int_tasklet, msm_gpio_sleep_int, 0);

void msm_gpio_enter_sleep(int from_idle)
{
	int i;
	struct tramp_gpio_smem *smem_gpio;

	smem_gpio = smem_alloc(SMEM_GPIO_INT, sizeof(*smem_gpio)); 

	if (smem_gpio) {
		for (i = 0; i < ARRAY_SIZE(smem_gpio->enabled); i++) {
			smem_gpio->enabled[i] = 0;
			smem_gpio->detection[i] = 0;
			smem_gpio->polarity[i] = 0;
		}
	}

	for (i = 0; i < ARRAY_SIZE(msm_gpio_chips); i++) {
		writel(msm_gpio_chips[i].int_enable[!from_idle], msm_gpio_chips[i].regs.int_en);
		if (smem_gpio) {
			uint32_t tmp;
			int start, index, shiftl, shiftr;
			start = msm_gpio_chips[i].chip.start;
			index = start / 32;
			shiftl = start % 32;
			shiftr = 32 - shiftl;
			tmp = msm_gpio_chips[i].int_enable[!from_idle];
			smem_gpio->enabled[index] |= tmp << shiftl;
			smem_gpio->enabled[index+1] |= tmp >> shiftr;
			smem_gpio->detection[index] |= readl(msm_gpio_chips[i].regs.int_edge) << shiftl;
			smem_gpio->detection[index+1] |= readl(msm_gpio_chips[i].regs.int_edge) >> shiftr;
			smem_gpio->polarity[index] |= readl(msm_gpio_chips[i].regs.int_pos) << shiftl;
			smem_gpio->polarity[index+1] |= readl(msm_gpio_chips[i].regs.int_pos) >> shiftr;
		}
	}

	if (smem_gpio) {
		if (msm_gpio_debug_mask & GPIO_DEBUG_SLEEP)
			for (i = 0; i < ARRAY_SIZE(smem_gpio->enabled); i++) {
				printk("msm_gpio_enter_sleep gpio %d-%d: enable"
				       " %08x, edge %08x, polarity %08x\n",
				       i * 32, i * 32 + 31,
				       smem_gpio->enabled[i],
				       smem_gpio->detection[i],
				       smem_gpio->polarity[i]);
			}
		for(i = 0; i < GPIO_SMEM_NUM_GROUPS; i++)
			smem_gpio->num_fired[i] = 0;
	}
}

void msm_gpio_exit_sleep(void)
{
	int i;
	struct tramp_gpio_smem *smem_gpio;

	smem_gpio = smem_alloc(SMEM_GPIO_INT, sizeof(*smem_gpio)); 

	for (i = 0; i < ARRAY_SIZE(msm_gpio_chips); i++) {
		writel(msm_gpio_chips[i].int_enable[0], msm_gpio_chips[i].regs.int_en);
	}

	if (smem_gpio && (smem_gpio->num_fired[0] || smem_gpio->num_fired[1])) {
		if (msm_gpio_debug_mask & GPIO_DEBUG_SLEEP)
			printk(KERN_INFO "gpio: fired %x %x\n",
			      smem_gpio->num_fired[0], smem_gpio->num_fired[1]);
		tasklet_schedule(&msm_gpio_sleep_int_tasklet);
	}
}

static int __init msm_init_gpio(void)
{
	int i;

	for (i = NR_MSM_IRQS; i < NR_MSM_IRQS + NR_GPIO_IRQS; i++) {
		set_irq_chip(i, &msm_gpio_irq_chip);
		set_irq_handler(i, handle_edge_irq);
		set_irq_flags(i, IRQF_VALID);
	}

	for (i = 0; i < ARRAY_SIZE(msm_gpio_chips); i++) {
		writel(0, msm_gpio_chips[i].regs.int_en);
		register_gpio_chip(&msm_gpio_chips[i].chip);
	}

	set_irq_chained_handler(INT_GPIO_GROUP1, msm_gpio_irq_handler);
	set_irq_chained_handler(INT_GPIO_GROUP2, msm_gpio_irq_handler);
	set_irq_wake(INT_GPIO_GROUP1, 1);
	set_irq_wake(INT_GPIO_GROUP2, 2);
	return 0;
}

postcore_initcall(msm_init_gpio);

int gpio_tlmm_config(unsigned config, unsigned disable)
{
	/* LGE_CHANGE [dojip.kim@lge.com] 2010-03-23, check the valild gpio */
#if defined(CONFIG_MACH_EVE)
	int gpio;

	gpio = GPIO_PIN(config);

	if (gpio_check_range(gpio) < 0) {
		return 0;
	}
#endif
	return msm_proc_comm(PCOM_RPC_GPIO_TLMM_CONFIG_EX, &config, &disable);
}
EXPORT_SYMBOL(gpio_tlmm_config);

int msm_gpios_request_enable(const struct msm_gpio *table, int size)
{
	int rc = msm_gpios_request(table, size);
	if (rc)
		return rc;
	rc = msm_gpios_enable(table, size);
	if (rc)
		msm_gpios_free(table, size);
	return rc;
}
EXPORT_SYMBOL(msm_gpios_request_enable);

void msm_gpios_disable_free(const struct msm_gpio *table, int size)
{
	msm_gpios_disable(table, size);
	msm_gpios_free(table, size);
}
EXPORT_SYMBOL(msm_gpios_disable_free);

int msm_gpios_request(const struct msm_gpio *table, int size)
{
	int rc;
	int i;
	const struct msm_gpio *g;
	for (i = 0; i < size; i++) {
		g = table + i;
		rc = gpio_request(GPIO_PIN(g->gpio_cfg), g->label);
		if (rc) {
			pr_err("gpio_request(%d) <%s> failed: %d\n",
			       GPIO_PIN(g->gpio_cfg), g->label ?: "?", rc);
			goto err;
		}
	}
	return 0;
err:
	msm_gpios_free(table, i);
	return rc;
}
EXPORT_SYMBOL(msm_gpios_request);

void msm_gpios_free(const struct msm_gpio *table, int size)
{
	int i;
	const struct msm_gpio *g;
	for (i = size-1; i >= 0; i--) {
		g = table + i;
		gpio_free(GPIO_PIN(g->gpio_cfg));
	}
}
EXPORT_SYMBOL(msm_gpios_free);

int msm_gpios_enable(const struct msm_gpio *table, int size)
{
	int rc;
	int i;
	const struct msm_gpio *g;
	for (i = 0; i < size; i++) {
		g = table + i;
		rc = gpio_tlmm_config(g->gpio_cfg, GPIO_ENABLE);
		if (rc) {
			pr_err("gpio_tlmm_config(0x%08x, GPIO_ENABLE)"
			       " <%s> failed: %d\n",
			       g->gpio_cfg, g->label ?: "?", rc);
			pr_err("pin %d func %d dir %d pull %d drvstr %d\n",
			       GPIO_PIN(g->gpio_cfg), GPIO_FUNC(g->gpio_cfg),
			       GPIO_DIR(g->gpio_cfg), GPIO_PULL(g->gpio_cfg),
			       GPIO_DRVSTR(g->gpio_cfg));
			goto err;
		}
	}
	return 0;
err:
	msm_gpios_disable(table, i);
	return rc;
}
EXPORT_SYMBOL(msm_gpios_enable);

void msm_gpios_disable(const struct msm_gpio *table, int size)
{
	int rc;
	int i;
	const struct msm_gpio *g;
	for (i = size-1; i >= 0; i--) {
		g = table + i;
		rc = gpio_tlmm_config(g->gpio_cfg, GPIO_DISABLE);
		if (rc) {
			pr_err("gpio_tlmm_config(0x%08x, GPIO_DISABLE)"
			       " <%s> failed: %d\n",
			       g->gpio_cfg, g->label ?: "?", rc);
			pr_err("pin %d func %d dir %d pull %d drvstr %d\n",
			       GPIO_PIN(g->gpio_cfg), GPIO_FUNC(g->gpio_cfg),
			       GPIO_DIR(g->gpio_cfg), GPIO_PULL(g->gpio_cfg),
			       GPIO_DRVSTR(g->gpio_cfg));
		}
	}
}
EXPORT_SYMBOL(msm_gpios_disable);

#if defined(CONFIG_DEBUG_FS)

static int msm_gpio_debug_result = 1;

static int gpio_enable_set(void *data, u64 val)
{
	msm_gpio_debug_result = gpio_tlmm_config(val, 0);
	return 0;
}
static int gpio_disable_set(void *data, u64 val)
{
	msm_gpio_debug_result = gpio_tlmm_config(val, 1);
	return 0;
}

static int gpio_debug_get(void *data, u64 *val)
{
	unsigned int result = msm_gpio_debug_result;
	msm_gpio_debug_result = 1;
	if (result)
		*val = 1;
	else
		*val = 0;
	return 0;
}

/* LGE_CHANGE_S [ybw75@lge.com] 2009-03-19, Add test mode for changing GPIO setting during sleep */
#if defined(FEATURE_TEST_FOR_SLEEP_GPIO_SETTING)

static int gpio_debug_get_sleep_config_get(void *data, u64 *val)
{
	*val = (u64)(test_get_gpio_sleep_config);
	return 0;
}

static int gpio_debug_get_sleep_config_set(void *data, u64 val)
{
	test_get_gpio_sleep_config = (uint32)val;
	return 0;
}

static int gpio_debug_set_sleep_config_get(void *data, u64 *val)
{
	*val = (u64)(test_set_gpio_sleep_config);
	return 0;
}

static int gpio_debug_set_sleep_config_set(void *data, u64 val)
{
	test_set_gpio_sleep_config = (uint32)val;
	return 0;
}

static int gpio_debug_get_config_gpio_num_get(void *data, u64 *val)
{
	*val = (u64)(test_get_gpio_num);
	return 0;
}

static int gpio_debug_get_config_gpio_num_set(void *data, u64 val)
{

	test_get_gpio_num = (int)val;
	test_get_gpio_sleep_config = msm_chg_lge_get_test_sleep_gpio_config(test_get_gpio_num);
	return 0;
}

static int gpio_debug_set_config_gpio_num_get(void *data, u64 *val)
{
	*val = (u64)(test_set_gpio_num);
	return 0;
}

static int gpio_debug_set_config_gpio_num_set(void *data, u64 val)
{

	test_set_gpio_num = (int)val;
	msm_chg_lge_set_test_sleep_gpio_config(test_set_gpio_num, test_set_gpio_sleep_config);
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(gpio_get_sleep_gpio_num_fops, gpio_debug_get_config_gpio_num_get, gpio_debug_get_config_gpio_num_set, "%llu\n");
DEFINE_SIMPLE_ATTRIBUTE(gpio_set_sleep_gpio_num_fops, gpio_debug_set_config_gpio_num_get, gpio_debug_set_config_gpio_num_set, "%llu\n");
DEFINE_SIMPLE_ATTRIBUTE(gpio_get_gpio_sleep_config_fops, gpio_debug_get_sleep_config_get, gpio_debug_get_sleep_config_set, "%llu\n");
DEFINE_SIMPLE_ATTRIBUTE(gpio_set_gpio_sleep_config_fops, gpio_debug_set_sleep_config_get, gpio_debug_set_sleep_config_set, "%llu\n");

#endif //#if defined(FEATURE_TEST_FOR_SLEEP_GPIO_SETTING)
/* LGE_CHANGE_E [ybw75@lge.com] 2009-03-19 */

/* LGE_CHANGE_S [jinwoonam@lge.com] 2009.03.05 */
/* LGE_CHANGE [dojip.kim@lge.com] 2010-03-23, set the value as u64 */
#if defined(CONFIG_MACH_EVE)
#define NUM_OF_GPIOS		123ULL // 0~122
#define INVALID_GPIO_VALUE      9999ULL
#define SLEEP_GPIO_GET_OFFSET   0x10ULL

// Add GPIO Setting functions

static int msm_gpio_debug_number = INVALID_GPIO_VALUE;

static int gpio_check_range(u64 number)
{
	if (number >= NUM_OF_GPIOS || number < 0) {
		printk("[ GPIO] : %llu is not a valid GPIO\n", number);
		return -1;
	}

	return 0;
}

/* LGE_CHANGE [dojip.kim@lge.com] 2010-03-23, fix the wrong pointer: *val -> val */
static int gpio_enable_value_set(void *data, u64 val)
{
	if (0 == gpio_check_range(val)) {
		msm_gpio_debug_number = val;
	} else {
		msm_gpio_debug_number = INVALID_GPIO_VALUE;
	}
	return 0;
}

static int gpio_enable_value_get(void *data, u64 *val)
{
	uint param2;
	uint config;

	param2 = msm_gpio_debug_number + SLEEP_GPIO_GET_OFFSET;

	if (0 == msm_proc_comm(PCOM_RPC_GPIO_TLMM_CONFIG_EX, &config, &param2)) {
		// To debug value
		uint nGpioNumber;
		uint func_val;
		uint dir_val;
		uint drvstr_val;
		uint pull_val;

		nGpioNumber = (((config) >> 8) & 0xFF);
		func_val =    ( (config) & 0xF);
		dir_val =     (((config) >> 16) & 0x1);
		pull_val =    (((config) >> 17) & 0x3);
		drvstr_val =  (((config) >> 19) & 0x7);

		printk("%u is 0x%x\n", msm_gpio_debug_number, config);
		printk("Number:%u, Func:%u, Dir:%u, Pull:%u, DrvStrength:%u\n", \
			nGpioNumber, func_val, dir_val, pull_val, drvstr_val);

		*val = config;
	} else {    // Fail
		printk("[err] Fail to get GPIO_%d value via RPC\n", msm_gpio_debug_number);
	}
	return 0;
}

static int gpio_config_number_set(void *data, u64 val)
{
	if (0 == gpio_check_range(val)) {
		msm_gpio_debug_number = val;
	} else {
		msm_gpio_debug_number = INVALID_GPIO_VALUE;
	}
	return 0;
}

static int gpio_config_number_get(void *data, u64 *val)
{
	*val = msm_gpio_debug_number;
	return 0;
}

static int gpio_config_value_set(void *data, u64 val)
{
	if (0 == gpio_check_range(msm_gpio_debug_number)) {
		//gpio_set_value(msm_gpio_debug_number, (val ? 1 : 0));
		gpio_direction_output(msm_gpio_debug_number, (val ? 1 : 0));
	}
	return 0;
}

static int gpio_config_value_get(void *data, u64 *val)
{
	if (0 == gpio_check_range(msm_gpio_debug_number)) {
		*val = gpio_get_value(msm_gpio_debug_number);
		printk("[ GPIO] : %u is %llu\n", msm_gpio_debug_number, *val);
	} else {
		*val = INVALID_GPIO_VALUE;
	}
	return 0;
}

static int gpio_config_showall_set(void *data, u64 val)
{
	return 0;
}

static int gpio_config_showall_get(void *data, u64 *val)
{
	unsigned i;

	for (i = 0; i < NUM_OF_GPIOS; i++) {
		printk("%u : %d,\t", i, gpio_get_value(i));
		if (i % 5 == 4) printk("\n");
	}
	*val = 0;
	return 0;
}
#endif /* CONFIG_MACH_EVE */
/* LGE_CHANGE_E [jinwoonam@lge.com] 2009.03.05 */

DEFINE_SIMPLE_ATTRIBUTE(gpio_enable_fops, gpio_debug_get,
						gpio_enable_set, "%llu\n");
DEFINE_SIMPLE_ATTRIBUTE(gpio_disable_fops, gpio_debug_get,
						gpio_disable_set, "%llu\n");
/* LGE_CHANGE_S [jinwoonam@lge.com] 2009.03.05 */
#if defined(CONFIG_MACH_EVE)
DEFINE_SIMPLE_ATTRIBUTE(gpio_value_fops, gpio_enable_value_get, gpio_enable_value_set, "%llu\n");

DEFINE_SIMPLE_ATTRIBUTE(gpio_config_number_fops, gpio_config_number_get, gpio_config_number_set, "%llu\n");
DEFINE_SIMPLE_ATTRIBUTE(gpio_config_value_fops, gpio_config_value_get, gpio_config_value_set, "%llu\n");
DEFINE_SIMPLE_ATTRIBUTE(gpio_config_showall_fops, gpio_config_showall_get, gpio_config_showall_set, "%llu\n");
#endif /* CONFIG_MACH_EVE */
/* LGE_CHANGE_E [jinwoonam@lge.com] 2009.03.05 */

static int __init gpio_debug_init(void)
{
	struct dentry *dent;

	dent = debugfs_create_dir("gpio", 0);
	if (IS_ERR(dent))
		return 0;

	debugfs_create_file("enable", 0644, dent, 0, &gpio_enable_fops);
	debugfs_create_file("disable", 0644, dent, 0, &gpio_disable_fops);
/* LGE_CHANGE_S [ybw75@lge.com] 2009-03-19, Add test mode for changing GPIO setting during sleep */
#if defined(FEATURE_TEST_FOR_SLEEP_GPIO_SETTING)
	debugfs_create_file("get_sleep_gpio_num", 0666, dent, 0, &gpio_get_sleep_gpio_num_fops);
	debugfs_create_file("set_sleep_gpio_num", 0666, dent, 0, &gpio_set_sleep_gpio_num_fops);
	debugfs_create_file("get_gpio_sleep_config", 0666, dent, 0, &gpio_get_gpio_sleep_config_fops);
	debugfs_create_file("set_gpio_sleep_config", 0666, dent, 0, &gpio_set_gpio_sleep_config_fops);
#endif //#if defined(FEATURE_TEST_FOR_SLEEP_GPIO_SETTING)
/* LGE_CHANGE_E [ybw75@lge.com] 2009-03-19 */

/* LGE_CHANGE_S [jinwoonam@lge.com] 2009.03.05 */
// Add debug filesystem for setting GPIO
#if defined(CONFIG_MACH_EVE)
	debugfs_create_file("value", 0644, dent, 0, &gpio_value_fops);

	dent = debugfs_create_dir("gpio_config", 0);
	if (IS_ERR(dent))
		return 0;

	debugfs_create_file("gpio_number", 0644, dent, 0, &gpio_config_number_fops);
	debugfs_create_file("gpio_value", 0644, dent, 0, &gpio_config_value_fops);
	debugfs_create_file("show_all", 0444, dent, 0, &gpio_config_showall_fops);
#endif /* CONFIG_MACH_EVE */
/* LGE_CHANGE_E [jinwoonam@lge.com] 2009.03.05 */

	return 0;
}

device_initcall(gpio_debug_init);
#endif

