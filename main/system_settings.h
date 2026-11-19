/*
 * settings.h
 *
 *  Created on: Nov 4, 2026
 *      Author: wazil
 */

#ifndef MAIN_SYSTEM_SETTINGS_H_
#define MAIN_SYSTEM_SETTINGS_H_

#include <stdint.h>

// System settings string lengths
#define MAX_NAME_LENGTH					32
#define MAX_SSID_LENGTH					32
#define MAX_PASSWORD_LENGTH				64
#define IP_ADDR_LENGTH					16
#define MAC_ADDR_LENGTH					18
#define ISO_8601_LENGTH					21

// Default system settings
#define SYSTEM_NAME_DEFAULT				"Node_"

#define WIFI_SSID_DEFAULT				""
#define WIFI_PASSWORD_DEFAULT			""

#define SNTP_TIME_SERVER_DEFAULT		"pool.ntp.org"

/**
 * Global device settings structure
 */
typedef struct {
	char system_name[MAX_NAME_LENGTH];				// Name of the system, later set as system host name
	bool use_led;									// Use status LED during normal operation

	char fw_version[MAX_NAME_LENGTH];
	char mac_address[MAC_ADDR_LENGTH];				// ASCII string with semicolons, not 6 bytes/octets
	char iso_8601_boot[22];							// boot time in ISO 8601 format

	char ssid[MAX_SSID_LENGTH];
	char wifi_password[MAX_PASSWORD_LENGTH];
} system_settings_t;

extern system_settings_t *system_settings;

/**
 * Initializes global system settings typedef
 */
void system_settings_init(void);

/**
 * Destroy and free up the current system settings
 */
 void system_settings_destroy(void);

/**
 * Set default values to system settings
 */
void system_settings_default(void);

#endif /* MAIN_SYSTEM_SETTINGS_H_ */
