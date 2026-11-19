/*
 * tasks_common.h
 *
 *  Created on: Mar 5, 2026
 *      Author: wazil
 */

#ifndef MAIN_TASKS_COMMON_H_
#define MAIN_TASKS_COMMON_H_

// RGB LED system boot task
#define RGB_LED_SYSTEM_BOOT_TASK_STACK_SIZE		2048
#define RGB_LED_SYSTEM_BOOT_TASK_PRIORITY		5
#define RGB_LED_SYSTEM_BOOTP_TASK_CORE_ID		0

// WiFi application task
#define WIFI_APP_TASK_STACK_SIZE				4096
#define WIFI_APP_TASK_PRIORITY					5
#define WIFI_APP_TASK_CORE_ID					0

// HTTP Server task
#define HTTP_SERVER_TASK_STACK_SIZE				8192
#define HTTP_SERVER_TASK_PRIORITY				4
#define HTTP_SERVER_TASK_CORE_ID				0

// HTTP Server Monitor task
#define HTTP_SERVER_MONITOR_STACK_SIZE			4096
#define HTTP_SERVER_MONITOR_PRIORITY			3
#define HTTP_SERVER_MONITOR_CORE_ID				0

// DHT22 Sensor task
#define DHT22_TASK_STACK_SIZE					4096
#define DHT22_TASK_PRIORITY						5
#define DHT22_TASK_CORE_ID						1

#endif /* MAIN_TASKS_COMMON_H_ */
