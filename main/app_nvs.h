/*
 * app_nvs.h
 *
 *  Created on: Jun 17, 2026
 *      Author: wazil
 */

#ifndef MAIN_APP_NVS_H_
#define MAIN_APP_NVS_H_

#include <stdbool.h>
#include "esp_err.h"

/**
 * NVS update groups
 */
typedef enum app_nvs_settings_group {
	APP_NVS_SETTINGS_SYSTEM = 1,
	APP_NVS_SETTINGS_WIFI = 2,
	APP_NVS_SETTINGS_ALL = 4
} app_nvs_settings_group_e;

/**
 * Saves system settings to NVS
 * @return ESP_OK if successful
 */
esp_err_t app_nvs_save_system_settings(app_nvs_settings_group_e group);

/**
 * Loads the previously saved system settings from NVS
 * @return true if previously saved settings were found
 */
bool app_nvs_load_system_settings(void);

#endif /* MAIN_APP_NVS_H_ */
