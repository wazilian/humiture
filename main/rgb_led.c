/*
 * rgb_led.c
 *
 *  Created on: Mar 4, 2026
 *      Author: wazil
 */

#include <stdbool.h>
#include <stdint.h>

#include "driver/ledc.h"
#include "hal/ledc_types.h"
#include "freertos/idf_additions.h"
#include "freertos/task.h"

#include "tasks_common.h"
#include "system_settings.h"
#include "rgb_led.h"

// RGB LED color defines
uint8_t RGB_LED_OFF[] =				{ 0, 0, 0 };		// Black technically, but off for RGB LED
uint8_t RGB_LED_SYSTEM_ERROR[] =	{ 255, 0, 0 };		// Red
uint8_t RGB_LED_SYSTEM_WARNING[] =	{ 255, 255, 0 };	// Yellow
uint8_t RGB_LED_SYSTEM_OK[] =		{ 0, 255, 0 };		// Green
uint8_t RGB_LED_SYSTEM_BOOT[] =		{ 0, 0, 255 };		// Blue
uint8_t RGB_LED_OTA_UPDATE[] =		{ 128, 0, 128 };	// Purple

// RGB LED Configuration Array
ledc_info_t ledc_ch[RGB_LED_CHANNEL_NUM];

// handle for rgb_led_pwm_init
bool g_pwm_init_handle = false;

// handle for rgb led system boot task
TaskHandle_t rgb_led_system_boot_task_handle = NULL;

/**
 * Initializes the RGB LED settings per channel, including
 * the GPIO for each color, mode, timer configuration.
 */
static void rgb_led_pwm_init(void) {
	int rgb_ch;

	// Red
	ledc_ch[0].channel		= LEDC_CHANNEL_0;
	ledc_ch[0].gpio			= RGB_LED_RED_GPIO;
	ledc_ch[0].mode			= LEDC_HIGH_SPEED_MODE;
	ledc_ch[0].timer_index	= LEDC_TIMER_0;

	// Green
	ledc_ch[1].channel		= LEDC_CHANNEL_1;
	ledc_ch[1].gpio			= RGB_LED_GREEN_GPIO;
	ledc_ch[1].mode			= LEDC_HIGH_SPEED_MODE;
	ledc_ch[1].timer_index	= LEDC_TIMER_0;

	// Blue
	ledc_ch[2].channel		= LEDC_CHANNEL_2;
	ledc_ch[2].gpio			= RGB_LED_BLUE_GPIO;
	ledc_ch[2].mode			= LEDC_HIGH_SPEED_MODE;
	ledc_ch[2].timer_index	= LEDC_TIMER_0;

	// Configure timer zero
	ledc_timer_config_t ledc_timer = {
		.duty_resolution	= LEDC_TIMER_8_BIT,
		.freq_hz			= 100,
		.speed_mode			= LEDC_HIGH_SPEED_MODE,
		.timer_num			= LEDC_TIMER_0
	};
	ledc_timer_config(&ledc_timer);

	// Configure channels
	for (rgb_ch = 0; rgb_ch < RGB_LED_CHANNEL_NUM; rgb_ch++) {
		ledc_channel_config_t ledc_channel = {
			.channel	= ledc_ch[rgb_ch].channel,
			.duty		= 0,
			.hpoint		= 0,
			.gpio_num	= ledc_ch[rgb_ch].gpio,
			.speed_mode	= ledc_ch[rgb_ch].mode,
			.timer_sel	= ledc_ch[rgb_ch].timer_index,
		};
		ledc_channel_config(&ledc_channel);
	}

	g_pwm_init_handle = true;
}

/*
 * Sets the RGB LED color.
 */
static void rgb_led_set_color(uint8_t *rgb) {
	uint8_t red = 	rgb[0];
	uint8_t green = rgb[1];
	uint8_t blue = 	rgb[2];

	ledc_set_duty(ledc_ch[0].mode, ledc_ch[0].channel, 255 - red);
	ledc_update_duty(ledc_ch[0].mode, ledc_ch[0].channel);

	ledc_set_duty(ledc_ch[1].mode, ledc_ch[1].channel, 255 - green);
	ledc_update_duty(ledc_ch[1].mode, ledc_ch[1].channel);

	ledc_set_duty(ledc_ch[2].mode, ledc_ch[2].channel, 255 - blue);
	ledc_update_duty(ledc_ch[2].mode, ledc_ch[2].channel);
 }

void rgb_led_toggle(uint8_t *rgb) {
	if (g_pwm_init_handle == false) {
		rgb_led_pwm_init();
	}

	rgb_led_set_color(rgb);
}

/**
 * Blink RGB LED with provided color. Should come from a task start.
 */
static void rgb_led_blink(void *rgb_color) {
	if (g_pwm_init_handle == false) {
		rgb_led_pwm_init();
	}

	while (1) {
		rgb_led_set_color(rgb_color);
		vTaskDelay(500 / portTICK_PERIOD_MS);
		rgb_led_set_color(RGB_LED_OFF);
		vTaskDelay(500 / portTICK_PERIOD_MS);
	}
}

void rgb_led_system_boot_task_start(void) {
	xTaskCreatePinnedToCore(&rgb_led_blink, "rgb_led_blink", RGB_LED_SYSTEM_BOOT_TASK_STACK_SIZE, RGB_LED_SYSTEM_BOOT, RGB_LED_SYSTEM_BOOT_TASK_PRIORITY, &rgb_led_system_boot_task_handle, RGB_LED_SYSTEM_BOOTP_TASK_CORE_ID);
}

void rgb_led_system_boot_task_stop(void) {
	vTaskDelete(rgb_led_system_boot_task_handle);
	vTaskDelay(500 / portTICK_PERIOD_MS);	// TODO: improve this. check if/when task was deleted?

	if (system_settings->use_led) {
		rgb_led_set_color(RGB_LED_SYSTEM_OK);	// Set system OK for now
	} else {
		rgb_led_set_color(RGB_LED_OFF);			// User requests to turn off status LED
	}
}
