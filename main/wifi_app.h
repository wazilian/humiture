/*
 * wifi_app.h
 *
 *  Created on: Mar 5, 2026
 *      Author: wazil
 */

#ifndef MAIN_WIFI_APP_H_
#define MAIN_WIFI_APP_H_

#include "esp_wifi_types_generic.h"

// Callback typedef
typedef void (*wifi_connected_event_callback_t)(void);

// WiFi application settings
#define WIFI_STA_POWER_SAVE			WIFI_PS_NONE
#define MAX_CONNECTION_RETRIES		5

/**
 * Message IDs for the WiFi application task
 * @note expand upon this as needed
 */
typedef enum wifi_app_message {
	WIFI_APP_MSG_START_HTTP_SERVER = 0,
	WIFI_APP_MSG_STA_CONNECTED_GOT_IP,
	WIFI_APP_MSG_HTTP_SERVER_REQUESTED_STA_DISCONNECT,
	WIFI_APP_MSG_USER_LOAD_SAVED_CREDENTIALS,
	WIFI_APP_MSG_STA_DISCONNECTED
} wifi_app_message_e;

/**
 * Structure for the message queue
 * @note expand upon this as needed
 */
typedef struct wifi_app_queue_message {
	wifi_app_message_e msgID;
} wifi_app_queue_message_t;

/**
 * Sends a message to the queue
 * @param msgID message ID from the wifi_app_message_e enum
 * @return pdTRUE if an item was successfully sent to the queue, otherwise pdFASLE
 * @note expand upon this as needed
 */
int wifi_app_send_message(wifi_app_message_e msgID);

/**
 * Set the system host name
 */
void wifi_app_set_hostname(void);

/**
 * Gets the WiFi MAC address
 */
void wifi_app_get_mac_address(char *mac_address, int size);

/**
 * Gets the current IP address
 */
void wifi_app_get_ip_address(char *ip);

/**
 * Gets the RSSI value
 */
void wifi_app_get_rssi(int8_t *rssi);

/**
 * Sets the callback function
 */
void wifi_app_set_callback(wifi_connected_event_callback_t cb);

/**
 * Calls the callback function
 */
void wifi_app_call_callback(void);

/**
 * Starts the WiFI RTOS task
 */
void wifi_app_start(void);

/**
 * Gets the WiFi configuration
 */
wifi_config_t *wifi_app_get_wifi_config(void);

#endif /* MAIN_WIFI_APP_H_ */
