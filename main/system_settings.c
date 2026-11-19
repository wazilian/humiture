/*
 * system_settings.c
 *
 *  Created on: Nov 8, 2026
 *      Author: wazil
 */

#include <string.h>

#include "esp_app_desc.h"

#include "system_settings.h"
#include "wifi_app.h"

// Description about application
const esp_app_desc_t *app_desc;

// Global device settings typedef structure (system_setting.h)
system_settings_t *system_settings = NULL;

/*
 * Get system firmware version
 */
void get_fw_version(char *version) {
	app_desc = esp_app_get_description();
	sprintf(version, "%s", (char *) app_desc->version);
}

void system_settings_init(void) {
	if (system_settings != NULL) {
		free(system_settings);
		system_settings = NULL;
	}

	char mac_address[MAC_ADDR_LENGTH] = {0};
	char fw_version[MAX_NAME_LENGTH] = {0};

	wifi_app_get_mac_address(mac_address, MAC_ADDR_LENGTH);
	get_fw_version(fw_version);

	system_settings = (system_settings_t *) malloc(sizeof(system_settings_t));
	memset(system_settings, 0x00, sizeof(system_settings_t));

	sprintf(system_settings->mac_address, "%s", mac_address);
	sprintf(system_settings->fw_version, "%s", fw_version);

	memset(system_settings->iso_8601_boot, 0x00, sizeof(system_settings->iso_8601_boot));
}

void system_settings_destroy(void) {
	if (system_settings != NULL) {
		free(system_settings);
		system_settings = NULL;
	}
}

void system_settings_default(void) {
	char mac_address[MAC_ADDR_LENGTH] = {0};

	wifi_app_get_mac_address(mac_address, MAC_ADDR_LENGTH / 2);		// Only need last 3 octets here

	sprintf(system_settings->system_name, "%s%s", SYSTEM_NAME_DEFAULT, mac_address);
	system_settings->use_led = true;
	sprintf(system_settings->ssid, "%s", WIFI_SSID_DEFAULT);
	sprintf(system_settings->wifi_password, "%s", WIFI_PASSWORD_DEFAULT);
}
