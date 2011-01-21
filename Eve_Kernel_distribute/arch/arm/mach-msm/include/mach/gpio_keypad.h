/* arch/gpio_keypad.h
**
** Copyright (C) 2007 Google, Inc.
*/

#ifndef _ARCH_GPIO_KEYPAD_H
#define _ARCH_GPIO_KEYPAD_H

enum gpio_keypad_flags {
	GPIOKPF_ACTIVE_HIGH              = 1U << 0, // unset: drive active output low, set: drive active output high
	GPIOKPF_DEBOUNCE                 = 1U << 1,
	GPIOKPF_REMOVE_SOME_PHANTOM_KEYS = 1U << 2,
	GPIOKPF_REMOVE_PHANTOM_KEYS      = GPIOKPF_REMOVE_SOME_PHANTOM_KEYS | GPIOKPF_DEBOUNCE,
	GPIOKPF_DRIVE_INACTIVE           = 1U << 3,
	GPIOKPF_LEVEL_TRIGGERED_IRQ      = 1U << 4,
	GPIOKPF_PRINT_UNMAPPED_KEYS      = 1U << 16,
	GPIOKPF_PRINT_MAPPED_KEYS        = 1U << 17,
	GPIOKPF_PRINT_PHANTOM_KEYS       = 1U << 18,
};


struct gpio_keypad_platform_data {
	const char *name;
	const unsigned short *keymap; // size must be ninputs * noutputs
	unsigned int *input_gpios;
	unsigned int *output_gpios;
	unsigned int ninputs;
	unsigned int noutputs;
	ktime_t settle_time; // time to wait before reading inputs after driving each output
	ktime_t debounce_delay; // time to wait before scanning the keypad a second time
	ktime_t poll_time;
	unsigned flags;
};

extern void gpio_keypad_send_keycode(unsigned int code, int value);

extern void msm_keyled_send(void);  //keyled_pm.c
extern void msm_touchled_send(void);//keyled_pm.c
extern int is_touch_pressed(void);//for touch-key, touch-window

//extern void msm_keyled_send(int value);


#endif
