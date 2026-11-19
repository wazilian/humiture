/*
 * app_nvs.c
 *
 *  Created on: Jun 17, 2026
 *      Author: wazil
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "esp_err.h"
#include "esp_log.h"
#include "nvs.h"

#include "app_nvs.h"
#include "system_settings.h"

// Tag for logging to the monitor
static const char TAG[] = "nvs";

// NVS name space for system settings
const char app_nvs_system_settings_namespace[] = "system_settings";

esp_err_t app_nvs_save_system_settings(app_nvs_settings_group_e group) {
	nvs_handle handle;
	esp_err_t esp_err;
	ESP_LOGI(TAG, "app_nvs_save_system_settings: Saving system settings to flash.");

	esp_err = nvs_open(app_nvs_system_settings_namespace, NVS_READWRITE, &handle);

	if (esp_err != ESP_OK) {
		printf("app_nvs_save_system_settings: Error (%s) opening NVS handle\n", esp_err_to_name(esp_err));
		return(esp_err);
	}

	if (group & APP_NVS_SETTINGS_SYSTEM) {
		// Set system name
		esp_err = nvs_set_blob(handle, "system_name", system_settings->system_name, MAX_NAME_LENGTH);

		if (esp_err != ESP_OK) {
			printf("app_nvs_save_system_settings: Error (%s) setting system name to NVS.\n", esp_err_to_name(esp_err));
			return esp_err;
		}

		// Set use LED
		esp_err = nvs_set_u8(handle, "use_led", (uint8_t) system_settings->use_led);

		if (esp_err != ESP_OK) {
			printf("app_nvs_save_system_settings: Error (%s) setting use LED to NVS.\n", esp_err_to_name(esp_err));
			return esp_err;
		}
	}

	if (group & APP_NVS_SETTINGS_WIFI) {
		// Set SSID
		esp_err = nvs_set_blob(handle, "ssid", system_settings->ssid, MAX_SSID_LENGTH);

		if (esp_err != ESP_OK) {
			printf("app_nvs_save_system_settings: Error (%s) setting SSID to NVS.\n", esp_err_to_name(esp_err));
			return esp_err;
		}

		// Set Password
		esp_err = nvs_set_blob(handle, "wifi_password", system_settings->wifi_password, MAX_PASSWORD_LENGTH);

		if (esp_err != ESP_OK) {
			printf("app_nvs_save_system_settings: Error (%s) setting Password to NVS.\n", esp_err_to_name(esp_err));
			return esp_err;
		}
	}

	// Commit settings to NVS
	esp_err = nvs_commit(handle);

	if (esp_err != ESP_OK) {
		printf("app_nvs_save_system_settings: Error (%s) committing settings to NVS.\n", esp_err_to_name(esp_err));
		return esp_err;
	}

	nvs_close(handle);
	ESP_LOGI(TAG, "app_nvs_save_system_settings: wrote settings to NVS.");

	printf("app_nvs_save_system_settings: returned ESP_OK\n");
	return ESP_OK;
}

bool app_nvs_load_system_settings(void) {
	nvs_handle handle;
	esp_err_t esp_err;

	ESP_LOGI(TAG, "app_nvs_load_system_settings: Loading system settings from flash.");

	if (nvs_open(app_nvs_system_settings_namespace, NVS_READONLY, &handle) == ESP_OK) {
		size_t str_size;
		uint8_t tmp_uint8_t = 0;

		// Clear buffers
		memset(system_settings->system_name, 0x00, MAX_NAME_LENGTH);

		// Load system name
		str_size = sizeof(system_settings->system_name);
		esp_err = nvs_get_blob(handle, "system_name", system_settings->system_name, &str_size);

		if (esp_err != ESP_OK) {
			printf("app_nvs_load_system_settings: (%s) no system name found in NVS.\n", esp_err_to_name(esp_err));
			return false;
		}

		// Load use LED
		esp_err = nvs_get_u8(handle, "use_led", &tmp_uint8_t);

		if (esp_err != ESP_OK) {
			printf("app_nvs_load_system_settings: (%s) no use LED found in NVS.\n", esp_err_to_name(esp_err));
			return false;
		} else {
			system_settings->use_led = (bool) tmp_uint8_t;
		}

		// Load SSID
		str_size = sizeof(system_settings->ssid);
		esp_err = nvs_get_blob(handle, "ssid", system_settings->ssid, &str_size);

		if (esp_err != ESP_OK) {
			printf("app_nvs_load_system_settings: (%s) no SSID found in NVS.\n", esp_err_to_name(esp_err));
			return false;
		}

		// Load WiFi password
		str_size = sizeof(system_settings->wifi_password);
		esp_err = nvs_get_blob(handle, "wifi_password", system_settings->wifi_password, &str_size);

		if (esp_err != ESP_OK) {
			printf("app_nvs_load_system_settings: (%s) no WiFi password found in NVS.\n", esp_err_to_name(esp_err));
			return false;
		}

		nvs_close(handle);

		printf("app_nvs_load_system_settings: Name: %s Use LED: %d SSID: %s WiFi Password: %s\n",
				system_settings->system_name,
				system_settings->use_led,
				system_settings->ssid,
				system_settings->wifi_password);
		return true;
	} else {
		return false;
	}
}
