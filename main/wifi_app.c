/*
 * wifi_app.c
 *
 *  Created on: Mar 5, 2026
 *      Author: wazil
 */

#include "esp_event.h"
#include "esp_event_base.h"
#include "esp_log_level.h"
#include "esp_netif_types.h"
#include "esp_wifi_types_generic.h"
#include "freertos/event_groups.h"
#include "freertos/idf_additions.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_mac.h"

#include "system_settings.h"
#include "app_nvs.h"
#include "portmacro.h"
#include "tasks_common.h"
#include "wifi_app.h"
#include "http_server.h"

// Tag used for ESP serial console message
static const char TAG[] = "wifi_app";

// WiFi application callback
static wifi_connected_event_callback_t wifi_connected_event_cb;

// Used for returning the WiFi configuration
wifi_config_t *wifi_config = NULL;

// Used to track the number of retries when a connection attempt fails
static int g_retry_number;

/**
 * WiFi application event group handle and status bits
 */
static EventGroupHandle_t wifi_app_event_group;
const int WIFI_APP_CONNECTING_USING_SAVED_CREDS_BIT				= BIT0;
const int WIFI_APP_CONNECTING_FROM_HTTP_SERVER_BIT				= BIT1;
const int WIFI_APP_HTTP_SERVER_REQUESTED_STA_DISCONNECT_BIT		= BIT2;

// Queue handle used to manipulate the main queue of events
static QueueHandle_t wifi_app_queue_handle;

// netif objects for the WiFi station
esp_netif_t *esp_netif_sta = NULL;


/**
 * WiFi application WiFI credentials loader
 * @return true if previously saved credentials were found
 */
static bool wifi_app_load_wifi_creds(void) {
	size_t wifi_config_size;

	// Allocate memory for the WiFi configuration
	wifi_config = (wifi_config_t *) malloc(sizeof(wifi_config_t));
	memset(wifi_config, 0x00, sizeof(wifi_config_t));

	// Load SSID
	wifi_config_size = sizeof(wifi_config->sta.ssid);
	memcpy(wifi_config->sta.ssid, system_settings->ssid, wifi_config_size);

	// Load Password
	wifi_config_size = sizeof(wifi_config->sta.password);
	memcpy(wifi_config->sta.password, system_settings->wifi_password, wifi_config_size);

	// Set auth mode to use either WPA2 or WPA3
//	wifi_config->sta.threshold.authmode = WIFI_AUTH_WPA2_WPA3_PSK;

	return wifi_config->sta.ssid[0] != '\0';
}

/**
 * WiFI application event handler
 * @param arg data, aside from event data, that is passed to the handler when it is called
 * @param event_base the base id of the event to register the handler for
 * @param event_id the id of the event to register the handler for
 * @param event_data event data
 */
static void wifi_app_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
	if (event_base == WIFI_EVENT) {
		switch (event_id) {
			case WIFI_EVENT_STA_START:
				ESP_LOGI(TAG, "WIFI_EVENT_STA_START");
				break;

			case WIFI_EVENT_STA_CONNECTED:
				ESP_LOGI(TAG, "WIFI_EVENT_STA_CONNECTED");
				break;

			case WIFI_EVENT_STA_DISCONNECTED:
				ESP_LOGI(TAG, "WIFI_EVENT_STA_DISCONNECTED");

				wifi_event_sta_disconnected_t *wifi_event_sta_disconnected = (wifi_event_sta_disconnected_t *) malloc(sizeof(wifi_event_sta_disconnected_t));
				*wifi_event_sta_disconnected = *((wifi_event_sta_disconnected_t *) event_data);
				printf("WIFI_EVENT_STA_DISCONNECTED, reason code %d\n", wifi_event_sta_disconnected->reason);

				if (g_retry_number < MAX_CONNECTION_RETRIES) {
					ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, wifi_app_get_wifi_config()));
					ESP_ERROR_CHECK(esp_wifi_connect());
					g_retry_number++;
				} else {
					wifi_app_send_message(WIFI_APP_MSG_STA_DISCONNECTED);
				}

				break;

			default:
				break;
		}
	} else if (event_base == IP_EVENT) {
		switch (event_id) {
			case IP_EVENT_STA_GOT_IP:
				ESP_LOGI(TAG, "IP_EVENT_STA_GOT_IP");

				g_retry_number = 0;

				wifi_app_send_message(WIFI_APP_MSG_STA_CONNECTED_GOT_IP);

				break;

			default:
				break;
		}
	}
}

/**
 * Initializes the WiFi application event handler for WiFi and IP events
 */
static void wifi_app_event_handler_init(void) {
	// Event loop for the WiFi driver
	ESP_ERROR_CHECK(esp_event_loop_create_default());

	// Create the event handler for the connection
	esp_event_handler_instance_t instance_wifi_event;
	esp_event_handler_instance_t instance_ip_event;
	ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_app_event_handler, NULL, &instance_wifi_event));
	ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &wifi_app_event_handler, NULL, &instance_ip_event));	
}

static void wifi_app_default_wifi_init(void) {
	// Initialize the TCP stack
	ESP_ERROR_CHECK(esp_netif_init());

	// Default WiFI config - operation must be in this order!
	wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	esp_netif_sta = esp_netif_create_default_wifi_sta();
}

/**
 * Connects the ESP32 to an external AP using the updated station configuration
 */
static void wifi_app_connect_sta(void) {
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, wifi_app_get_wifi_config()));
	ESP_ERROR_CHECK(esp_wifi_connect());
}

/**
 * Disconnects the ESP32 from an external AP
*/
static void wifi_app_disconnect_sta(void) {
	ESP_ERROR_CHECK(esp_wifi_disconnect());
}

/**
 * Main task for the WiFi application
 * @param pvParameters parameter which can be passed to the task
 */
static void wifi_app_task(void *pvParameters) {
	wifi_app_queue_message_t msg;
	EventBits_t eventBits;

	// Initialize the event handler
	wifi_app_event_handler_init();

	// Initialize the TCP/IP stack and WiFi config
	wifi_app_default_wifi_init();

	// Start WiFi
	ESP_ERROR_CHECK(esp_wifi_start());

	// Send first event message
	wifi_app_send_message(WIFI_APP_MSG_USER_LOAD_SAVED_CREDENTIALS);

	for (;;) {
		if (xQueueReceive(wifi_app_queue_handle, &msg, portMAX_DELAY)) {
			switch (msg.msgID) {
				case WIFI_APP_MSG_USER_LOAD_SAVED_CREDENTIALS:
					ESP_LOGI(TAG, "WIFI_APP_MSG_USER_LOAD_SAVED_CREDENTIALS");

					// Load WiFi credentials
					wifi_app_load_wifi_creds();

					// Connect to AP
					wifi_app_connect_sta();
					xEventGroupSetBits(wifi_app_event_group, WIFI_APP_CONNECTING_USING_SAVED_CREDS_BIT);

					// Next, start the web server
					wifi_app_send_message(WIFI_APP_MSG_START_HTTP_SERVER);

					break;

				case WIFI_APP_MSG_START_HTTP_SERVER:
					ESP_LOGI(TAG, "WIFI_APP_MSG_START_HTTP_SERVER");

					http_server_start();

					break;

                case WIFI_APP_MSG_STA_CONNECTED_GOT_IP:
                	ESP_LOGI(TAG, "WIFI_APP_MSG_STA_CONNECTED_GOT_IP");

                	http_server_monitor_send_message(HTTP_MSG_WIFI_CONNECT_SUCCESS);

					// TODO: Save WiFi credentials to system_settings here instead of in http_app AND after a connection success?

                	eventBits = xEventGroupGetBits(wifi_app_event_group);
                	if (eventBits & WIFI_APP_CONNECTING_USING_SAVED_CREDS_BIT) {
						// Clear the bits, in case we want to disconnect and reconnect, and start again
						xEventGroupClearBits(wifi_app_event_group, WIFI_APP_CONNECTING_USING_SAVED_CREDS_BIT);
					} else {
						app_nvs_save_system_settings(APP_NVS_SETTINGS_WIFI);
					}

					if (eventBits & WIFI_APP_MSG_HTTP_SERVER_REQUESTED_STA_DISCONNECT) {
						xEventGroupClearBits(wifi_app_event_group, WIFI_APP_HTTP_SERVER_REQUESTED_STA_DISCONNECT_BIT);
					}

					// Check for connection callback
					if (wifi_connected_event_cb) {
						wifi_app_call_callback();
					}

                	break;

                case WIFI_APP_MSG_HTTP_SERVER_REQUESTED_STA_DISCONNECT:
                	ESP_LOGI(TAG, "WIFI_APP_MSG_HTTP_SERVER_REQUESTED_STA_DISCONNECT");

                	xEventGroupSetBits(wifi_app_event_group, WIFI_APP_HTTP_SERVER_REQUESTED_STA_DISCONNECT_BIT);

					g_retry_number = 0;
                	wifi_app_disconnect_sta();

                	break;

                case WIFI_APP_MSG_STA_DISCONNECTED:
                	ESP_LOGI(TAG, "WIFI_APP_MSG_STA_DISCONNECTED");

                	eventBits = xEventGroupGetBits(wifi_app_event_group);

                	if (eventBits & WIFI_APP_CONNECTING_USING_SAVED_CREDS_BIT) {
						ESP_LOGI(TAG, "WIFI_APP_MSG_STA_DISCONNECTED: Attempt using saved credentials.");
						xEventGroupClearBits(wifi_app_event_group, WIFI_APP_CONNECTING_USING_SAVED_CREDS_BIT);
						// TODO: keep trying to connect?
					} else if (eventBits & WIFI_APP_MSG_HTTP_SERVER_REQUESTED_STA_DISCONNECT) {
						ESP_LOGI(TAG, "WIFI_APP_MSG_STA_DISCONNECTED: User requested disconnection.");
						xEventGroupClearBits(wifi_app_event_group, WIFI_APP_HTTP_SERVER_REQUESTED_STA_DISCONNECT_BIT);
						http_server_monitor_send_message(HTTP_MSG_WIFI_HTTP_SERVER_DISCONNECT);
					} else {
						ESP_LOGI(TAG, "WIFI_APP_MSG_STA_DISCONNECTED: Attempt failed, check WiFi access point availability");
						// TODO: keep trying to connect?
					}

                	break;

                default:
                	break;
             }
         }
	}
}

int wifi_app_send_message(wifi_app_message_e msgID) {
	wifi_app_queue_message_t msg;
	msg.msgID = msgID;

	return xQueueSend(wifi_app_queue_handle, &msg, portMAX_DELAY);
}

wifi_config_t *wifi_app_get_wifi_config(void) {
	return wifi_config;
}

void wifi_app_set_hostname(void) {
	esp_err_t esp_err = esp_netif_set_hostname(esp_netif_sta, system_settings->system_name);

	if (esp_err != ESP_OK) {
		ESP_LOGE(TAG, "Failed to set host name, error: %s", esp_err_to_name(esp_err));
	}
}

void wifi_app_get_mac_address(char *mac_address, int size) {
	uint8_t tmp_mac[6];

    esp_read_mac(tmp_mac, ESP_MAC_WIFI_STA);

	// Return either full MAC address with semicolons (ASCII string) or the last 3 octets with no semicolons
	if (size == MAC_ADDR_LENGTH) {
		sprintf(mac_address, "%02X:%02X:%02X:%02X:%02X:%02X", tmp_mac[0], tmp_mac[1], tmp_mac[2], tmp_mac[3], tmp_mac[4], tmp_mac[5]);
	} else if (size == (MAC_ADDR_LENGTH / 2)) {
		sprintf(mac_address, "%02x%02x%02x", tmp_mac[3], tmp_mac[4], tmp_mac[5]);
	}
}

void wifi_app_get_ip_address(char *ip) {
	esp_netif_ip_info_t ip_info;

	ESP_ERROR_CHECK(esp_netif_get_ip_info(esp_netif_sta, &ip_info));
	esp_ip4addr_ntoa(&ip_info.ip, ip, IP4ADDR_STRLEN_MAX);
}

void wifi_app_get_rssi(int8_t *rssi) {
	esp_err_t esp_err = esp_wifi_sta_get_rssi((int *) rssi);

    if (esp_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get RSSI, error: %s", esp_err_to_name(esp_err));
    }
}

void wifi_app_set_callback(wifi_connected_event_callback_t cb) {
	wifi_connected_event_cb = cb;
}

void wifi_app_call_callback(void) {
	wifi_connected_event_cb();
}

void wifi_app_start(void) {
	ESP_LOGI(TAG, "STARTING WIFI APPLICATION");

	// Disable default WiFi logging messages
	esp_log_level_set("wifi", ESP_LOG_NONE);

	// Create message queue
	wifi_app_queue_handle = xQueueCreate(3, sizeof(wifi_app_queue_message_t));

	// Create WiFi application event group
	wifi_app_event_group = xEventGroupCreate();

	// Start the WiFi application task
	xTaskCreatePinnedToCore(&wifi_app_task, "wifi_app_task", WIFI_APP_TASK_STACK_SIZE, NULL, WIFI_APP_TASK_PRIORITY, NULL, WIFI_APP_TASK_CORE_ID);
}
