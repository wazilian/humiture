
/**
 * Application entry point.
 */

#include "nvs_flash.h"

#include "system_settings.h"
#include "app_nvs.h"
#include "rgb_led.h"
#include "sntp_time_sync.h"
#include "wifi_app.h"
#include "DHT22.h"

bool boot = true;

void wifi_application_connected_events(void) {
	if (boot) {
		// Set system host name
		wifi_app_set_hostname();

		// Start SNTP time sync task
		sntp_time_sync_start();

		// Stop RGB LED boot sequence
		rgb_led_system_boot_task_stop();

		// system has booted
		boot = false;
	}
}

void app_main(void) {
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
//	ret = ESP_ERR_NVS_NEW_VERSION_FOUND;	// Un-comment for development to erase NVS

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();

		system_settings_init();
		system_settings_default();
		app_nvs_save_system_settings(APP_NVS_SETTINGS_ALL);
		system_settings_destroy();
	}

	ESP_ERROR_CHECK(ret);

	// RGB LED boot sequence
	rgb_led_system_boot_task_start();

	// Initialize system settings
	system_settings_init();

	// Get system settings
	app_nvs_load_system_settings();

	// Start WiFi
	wifi_app_start();

	// Start DHT22 Sensor task
	DHT22_task_start();

	// Set WiFi connected event callback
	wifi_app_set_callback(&wifi_application_connected_events);
}
