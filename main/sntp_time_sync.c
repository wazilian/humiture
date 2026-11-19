/*
 * sntp_time_sync.c
 *
 *  Created on: Jun 25, 2026
 *      Author: wazil
 */

#include <stdint.h>
#include <time.h>

#include "esp_log.h"
#include "esp_netif_sntp.h"
#include "esp_timer.h"

#include "sntp_time_sync.h"
#include "system_settings.h"

static const char TAG[] = "sntp_time_sync";

void get_system_uptime(int64_t *uptime) {
	*uptime = esp_timer_get_time() / 1000000;
}

/**
 * Get boot UTC time in ISO 8601 format
 */
static void get_system_boot_time_iso_8601(struct timeval *tv) {
	int64_t uptime = esp_timer_get_time() / 1000000;
	time_t now = 0, boot_time = 0;
	struct tm time_info = {0};

	time(&now);
	boot_time = now - uptime;
	localtime_r(&boot_time, &time_info);

	strftime(system_settings->iso_8601_boot, ISO_8601_LENGTH, "%Y-%m-%dT%H:%M:%SZ", &time_info);
}

void get_system_time_iso_8601(char *time_buf, int buf_len) {
	time_t now = 0;
	struct tm time_info = {0};

	time(&now);
	localtime_r(&now, &time_info);

	if (time_info.tm_year < (2016 - 1900)) {
		ESP_LOGI(TAG, "Time is not set yet.");
		sprintf(time_buf, "0000-00-00T00:00:00Z");
	} else {
		strftime(time_buf, buf_len, "%Y-%m-%dT%H:%M:%SZ", &time_info);
	}
}

void sntp_time_sync_start(void) {
	// Set up SNTP service config
	esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG(SNTP_TIME_SERVER_DEFAULT);
	config.sync_cb = get_system_boot_time_iso_8601;

	// Set time zone
	setenv("TZ", "UTC0", 1);
	tzset();

	esp_netif_sntp_init(&config);
}
