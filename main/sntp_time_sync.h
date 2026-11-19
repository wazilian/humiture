/*
 * sntp_time_sync.h
 *
 *  Created on: Jun 25, 2026
 *      Author: wazil
 */

#ifndef MAIN_SNTP_TIME_SYNC_H_
#define MAIN_SNTP_TIME_SYNC_H_

#include <stdint.h>

/**
 * Get system uptime in seconds
 */
void get_system_uptime(int64_t *uptime);

/**
 * Get UTC time in ISO 8601 format
 */
void get_system_time_iso_8601(char *time_buf, int buf_len);

/**
 * Starts the NTP server synchronization task
 */
void sntp_time_sync_start(void);

#endif /* MAIN_SNTP_TIME_SYNC_H_ */
