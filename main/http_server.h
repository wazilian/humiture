/*
 * http_server.h
 *
 *  Created on: Mar 7, 2026
 *      Author: wazil
 */

#ifndef MAIN_HTTP_SERVER_H_
#define MAIN_HTTP_SERVER_H_

/**
 * Connection status for WiFi
 */
typedef enum http_server_wifi_connect_status {
	NONE = 0,
	HTTP_WIFI_STATUS_CONNECT_SUCCESS,
	HTTP_WIFI_STATUS_CONNECTING,
	HTTP_WIFI_STATUS_CONNECT_FAILED,
	HTTP_WIFI_STATUS_DISCONNECTED
} http_server_wifi_connect_message_e;

/**
 * Messages for the HTTP monitor
 */
typedef enum http_server_message {
	HTTP_MSG_WIFI_CONNECT_INIT = 0,
	HTTP_MSG_WIFI_CONNECT_SUCCESS,
	HTTP_MSG_WIFI_CONNECT_FAIL,
	HTTP_MSG_WIFI_HTTP_SERVER_DISCONNECT,
	HTTP_MSG_WIFI_UPDATE_SUCCESSFUL,
	HTTP_MSG_WIFI_UPDATE_FAILED
} http_server_message_e;

/**
 * OTA update status
 */
typedef enum http_ota_update_status {
	OTA_UPDATE_PENDING = 0,
	OTA_UPDATE_SUCCESSFUL,
	OTA_UPDATE_FAILED
} http_ota_update_status_e;

/**
 * Structure for the message queue
 */
typedef struct http_server_queue_message {
	http_server_message_e msgID;
} http_server_queue_message_t;

/*
 * Sends a message to the queue
 * @param msgID message ID from the http_server_message_e enum.
 * @return pdTRUE if an item was successfully sent to the queue, otherwise pdFALSE.
 * @note Expand as needed.
 */
int http_server_monitor_send_message(http_server_message_e msgID);

/*
 * Starts the HTTP server.
 */
void http_server_start(void);

/*
 * Stop the HTTP server.
 */
void http_server_stop(void);

/**
 * Timer callback function which calls esp_restart upon successful firmware update
 */
void http_server_fw_update_reset_callback(void *arg);

#endif /* MAIN_HTTP_SERVER_H_ */
