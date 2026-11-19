/*
 * rgb_led.h
 *
 *  Created on: Mar 4, 2026
 *      Author: wazil
 */

#ifndef MAIN_RGB_LED_H_
#define MAIN_RGB_LED_H_

#include <stdint.h>

// RGB LED GPIOs
#define RGB_LED_RED_GPIO			21
#define RGB_LED_GREEN_GPIO			22
#define RGB_LED_BLUE_GPIO			23

// RGB LED color mix channels
#define RGB_LED_CHANNEL_NUM			3

// RGB LED configuration
typedef struct {
	int channel;
	int gpio;
	int mode;
	int timer_index;
} ledc_info_t;

/**
 * Toggle RGB LED with provided RGB LED color define.
 */
void rgb_led_toggle(uint8_t *rgb);

/**
 * Color to indicate system error has occurred.
 */
void rgb_led_system_error_occurred(void);

/**
 * Color to indicate system warning has occurred.
 */
void rgb_led_system_warning_occurred(void);

/**
 * Color to indicate system OK.
 */
void rgb_led_system_ok(void);

/**
 * Starts RGB LED system boot task.
 */
void rgb_led_system_boot_task_start(void);

/**
 * Stops RGB LED system boot task.
 */
void rgb_led_system_boot_task_stop(void);

#endif /* MAIN_RGB_LED_H_ */
