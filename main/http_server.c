/*
 * http_server.c
 *
 *  Created on: Mar 7, 2026
 *      Author: wazil
 */

#include "esp_err.h"
#include "esp_http_server.h"
#include "esp_netif_ip_addr.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_wifi_types_generic.h"
#include "sys/param.h"
#include "freertos/idf_additions.h"
#include "system_settings.h"
#include "time.h"

#include "app_nvs.h"
#include "DHT22.h"
#include "http_server.h"
#include "sntp_time_sync.h"
#include "tasks_common.h"
#include "wifi_app.h"

// Tag used for ESP serial console messages
static const char TAG[] = "http_server";

// WiFi connect status
static int g_wifi_connect_status = NONE;

// Firmware update status
static int g_fw_update_status = OTA_UPDATE_PENDING;

// Description about application
//const esp_app_desc_t *app_desc;

// End of HTTP header string
static char http_header_end[] = "\r\n\r\n";

// HTTP server task handle
static httpd_handle_t http_server_handle = NULL;

// HTTP server monitor task handle
static TaskHandle_t task_http_server_monitor = NULL;

// Queue handle used to manipulate the main queue of events
static QueueHandle_t http_server_monitor_queue_handle;

/**
 * ESP32 timer configuration passed to esp_timer_create.
 */
const esp_timer_create_args_t fw_update_reset_args = {
	.callback = &http_server_fw_update_reset_callback,
	.arg = NULL,
	.dispatch_method = ESP_TIMER_TASK,
	.name = "fw_update_reset"
};
esp_timer_handle_t fw_update_reset;

// Embedded web files: <see www folder>
extern const uint8_t index_html_start[]			asm("_binary_index_html_start");
extern const uint8_t index_html_end[]			asm("_binary_index_html_end");
extern const uint8_t settings_html_start[]		asm("_binary_settings_html_start");
extern const uint8_t settings_html_end[]		asm("_binary_settings_html_end");
extern const uint8_t app_css_start[]			asm("_binary_app_css_start");
extern const uint8_t app_css_end[]				asm("_binary_app_css_end");

extern const uint8_t ota_js_start[]				asm("_binary_ota_js_start");
extern const uint8_t ota_js_end[]				asm("_binary_ota_js_end");
extern const uint8_t settings_js_start[]		asm("_binary_settings_js_start");
extern const uint8_t settings_js_end[]			asm("_binary_settings_js_end");
extern const uint8_t status_js_start[]			asm("_binary_status_js_start");
extern const uint8_t status_js_end[]			asm("_binary_status_js_end");
extern const uint8_t wifi_js_start[]			asm("_binary_wifi_js_start");
extern const uint8_t wifi_js_end[]				asm("_binary_wifi_js_end");

extern const uint8_t error_ico_start[]			asm("_binary_error_ico_start");
extern const uint8_t error_ico_end[]			asm("_binary_error_ico_end");
extern const uint8_t favicon_ico_start[]		asm("_binary_favicon_ico_start");
extern const uint8_t favicon_ico_end[]			asm("_binary_favicon_ico_end");
extern const uint8_t info_ico_start[]			asm("_binary_info_ico_start");
extern const uint8_t info_ico_end[]				asm("_binary_info_ico_end");
extern const uint8_t success_ico_start[]		asm("_binary_success_ico_start");
extern const uint8_t success_ico_end[]			asm("_binary_success_ico_end");

/**
 * Checks the g_fw_update_status and creates the fw_update_reset timer if g_fw_update_status is true
 */
static void http_server_fw_update_reset_timer(void) {
	if (g_fw_update_status == OTA_UPDATE_SUCCESSFUL) {
		ESP_LOGI(TAG, "http_server_fw_update_reset_timer: FW updated successful starting FW update reset timer");

		// Give the web page a chance to receive an acknowledge back and initialize the timer
		ESP_ERROR_CHECK(esp_timer_create(&fw_update_reset_args, &fw_update_reset));
		ESP_ERROR_CHECK(esp_timer_start_once(fw_update_reset, 8000000));
	} else {
		ESP_LOGI(TAG, "http_server_fw_update_reset_timer: FW update was unsuccessful");
	}
}

/**
 * HTTP server monitor task used to track events of the HTTP server.
 * @param pvParameters parameter which can be passed to the task.
 * @return ESP_OK
 */
static void http_server_monitor(void *parameter) {
	http_server_queue_message_t msg;

	for (;;) {
		if (xQueueReceive(http_server_monitor_queue_handle, &msg, portMAX_DELAY)) {
			switch (msg.msgID) {
				case HTTP_MSG_WIFI_CONNECT_SUCCESS:
					ESP_LOGI(TAG, "HTTP_MSG_WIFI_CONNECT_SUCCESS");
					g_wifi_connect_status = HTTP_WIFI_STATUS_CONNECT_SUCCESS;
					break;

				case HTTP_MSG_WIFI_CONNECT_INIT:
					ESP_LOGI(TAG, "HTTP_MSG_WIFI_CONNECT_INIT");
					g_wifi_connect_status = HTTP_WIFI_STATUS_CONNECTING;
					break;

				case HTTP_MSG_WIFI_CONNECT_FAIL:
					ESP_LOGI(TAG, "HTTP_MSG_WIFI_CONNECT_FAIL");
					g_wifi_connect_status = HTTP_WIFI_STATUS_CONNECT_FAILED;
					break;

				case HTTP_MSG_WIFI_HTTP_SERVER_DISCONNECT:
					ESP_LOGI(TAG, "HTTP_MSG_WIFI_USER_DISCONNECT");
					g_wifi_connect_status = HTTP_WIFI_STATUS_DISCONNECTED;
					break;

				case HTTP_MSG_WIFI_UPDATE_SUCCESSFUL:
					ESP_LOGI(TAG, "HTTP_MSG_WIFI_UPDATE_SUCCESSFUL");
					g_fw_update_status = OTA_UPDATE_SUCCESSFUL;
					http_server_fw_update_reset_timer();
					break;

				case HTTP_MSG_WIFI_UPDATE_FAILED:
					ESP_LOGI(TAG, "HTTP_MSG_WIFI_UPDATE_FAILED");
					g_fw_update_status = OTA_UPDATE_FAILED;
					break;

				default:
					break;
			}
		}
	}
}

/**
 * Sends the index.html page.
 * @param req HTTP request for which the URI needs to be handled.
 * @return ESP_OK
 */
static esp_err_t http_server_index_html_handler(httpd_req_t *req) {
	ESP_LOGI(TAG, "index.html requested");

	httpd_resp_set_type(req, "text/html");
	httpd_resp_send(req, (const char *) index_html_start, index_html_end - index_html_start);

	return ESP_OK;
}

/**
 * Sends the settings.html page.
 * @param req HTTP request for which the URI needs to be handled.
 * @return ESP_OK
 */
static esp_err_t http_server_settings_html_handler(httpd_req_t *req) {
	ESP_LOGI(TAG, "settings.html requested");

	httpd_resp_set_type(req, "text/html");
	httpd_resp_send(req, (const char *) settings_html_start, settings_html_end - settings_html_start);

	return ESP_OK;
}

/**
 * app.css get handler is requested when accessing the web page.
 * @param req HTTP request for which the URI needs to be handled.
 * @return ESP_OK
 */
static esp_err_t http_server_app_css_handler(httpd_req_t *req) {
	ESP_LOGI(TAG, "app.css requested");

	httpd_resp_set_type(req, "text/css");
	httpd_resp_send(req, (const char *) app_css_start, app_css_end - app_css_start);

	return ESP_OK;
}

/**
 * ota.js get handler is requested when accessing the web page.
 * @param req HTTP request for which the URI needs to be handled.
 * @return ESP_OK
 */
static esp_err_t http_server_ota_js_handler(httpd_req_t *req) {
	ESP_LOGI(TAG, "ota.js requested");

	httpd_resp_set_type(req, "application/javascript");
	httpd_resp_send(req, (const char *) ota_js_start, ota_js_end - ota_js_start);

	return ESP_OK;
}

/**
 * settings.js get handler is requested when accessing the web page.
 * @param req HTTP request for which the URI needs to be handled.
 * @return ESP_OK
 */
static esp_err_t http_server_settings_js_handler(httpd_req_t *req) {
	ESP_LOGI(TAG, "settings.js requested");

	httpd_resp_set_type(req, "application/javascript");
	httpd_resp_send(req, (const char *) settings_js_start, settings_js_end - settings_js_start);

	return ESP_OK;
}

/**
 * status.js get handler is requested when accessing the web page.
 * @param req HTTP request for which the URI needs to be handled.
 * @return ESP_OK
 */
static esp_err_t http_server_status_js_handler(httpd_req_t *req) {
	ESP_LOGI(TAG, "status.js requested");

	httpd_resp_set_type(req, "application/javascript");
	httpd_resp_send(req, (const char *) status_js_start, status_js_end - status_js_start);

	return ESP_OK;
}

/**
 * wifi.js get handler is requested when accessing the web page.
 * @param req HTTP request for which the URI needs to be handled.
 * @return ESP_OK
 */
static esp_err_t http_server_wifi_js_handler(httpd_req_t *req) {
	ESP_LOGI(TAG, "wifi.js requested");

	httpd_resp_set_type(req, "application/javascript");
	httpd_resp_send(req, (const char *) wifi_js_start, wifi_js_end - wifi_js_start);

	return ESP_OK;
}

/**
 * Sends the error.ico (icon) file when accessing the web page.
 * @param req HTTP request for which the URI needs to be handled.
 * @return ESP_OK
 */
static esp_err_t http_server_error_ico_handler(httpd_req_t *req) {
	ESP_LOGI(TAG, "error.ico requested");

	httpd_resp_set_type(req, "image/x-icon");
	httpd_resp_send(req, (const char *) error_ico_start, error_ico_end - error_ico_start);

	return ESP_OK;
}

/**
 * Sends the favicon.ico (icon) file when accessing the web page.
 * @param req HTTP request for which the URI needs to be handled.
 * @return ESP_OK
 */
static esp_err_t http_server_favicon_ico_handler(httpd_req_t *req) {
	ESP_LOGI(TAG, "favicon.ico requested");

	httpd_resp_set_type(req, "image/x-icon");
	httpd_resp_send(req, (const char *) favicon_ico_start, favicon_ico_end - favicon_ico_start);

	return ESP_OK;
}

/**
 * Sends the info.ico (icon) file when accessing the web page.
 * @param req HTTP request for which the URI needs to be handled.
 * @return ESP_OK
 */
static esp_err_t http_server_info_ico_handler(httpd_req_t *req) {
	ESP_LOGI(TAG, "info.ico requested");

	httpd_resp_set_type(req, "image/x-icon");
	httpd_resp_send(req, (const char *) info_ico_start, info_ico_end - info_ico_start);

	return ESP_OK;
}

/**
 * Sends the success.ico (icon) file when accessing the web page.
 * @param req HTTP request for which the URI needs to be handled.
 * @return ESP_OK
 */
static esp_err_t http_server_success_ico_handler(httpd_req_t *req) {
	ESP_LOGI(TAG, "success.ico requested");

	httpd_resp_set_type(req, "image/x-icon");
	httpd_resp_send(req, (const char *) success_ico_start, success_ico_end - success_ico_start);

	return ESP_OK;
}

/**
 * Receives the .bin file via the web page and handles the firmware update
 * @param reg HTTP request for which the URI needs to be handled.
 * @return ESP_OK, otherwise ESP_FAIL if timeout occurs and the update cannot be started.
 */
esp_err_t http_server_OTA_update_handler(httpd_req_t *req) {
	esp_ota_handle_t ota_handle;
	char ota_buf[1024];
	char ota_JSON[100];
	int content_length = req->content_len;
	int content_received = 0;
	int recv_len;
	bool is_req_body_started = false;
	bool flash_successful = false;

	const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
	const esp_partition_t *running_partition = esp_ota_get_running_partition();

	if (content_length < running_partition->size) {
		do {
			// Read the data for the request
			if ((recv_len = httpd_req_recv(req, ota_buf, MIN(content_length, sizeof(ota_buf)))) < 0) {
				// Check if timeout occured
				if (recv_len == HTTPD_SOCK_ERR_TIMEOUT) {
					ESP_LOGI(TAG, "http_server_OTA_update_handler: Socket Timeout");
					continue;	///> Retry receiving if timeout occurred
				}

				ESP_LOGI(TAG, "http_server_OTA_update_handler: OTA other Error %d", recv_len);
				return ESP_FAIL;
			}

			printf("http_server_OTA_update_handler: OTA Rx: %d of %d\r", content_received, content_length);

			// Is this the first data we are receiving
			// If so, it will have the information in the header we need.
			if (!is_req_body_started) {
				is_req_body_started = true;

				// Get the location of the .bin file content (remove the web form data)
				char *body_start_p = strstr(ota_buf, http_header_end) + strlen(http_header_end);
				int body_part_len = recv_len - (body_start_p - ota_buf);

				printf("http_server_OTA_update_handler: OTA file size: %d\r\n", content_length);

				esp_err_t err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &ota_handle);
				if (err != ESP_OK) {
					printf("http_server_OTA_update_handler: Error with OTA begin, cancelling OTA with error: %d\r\n", err);
					return ESP_FAIL;
				} else {
					printf("http_server_OTA_update_handler: Writing to partition subtype %d at offset 0x%lx\r\n", update_partition->subtype, (unsigned long) update_partition->address);
				}

				// Write this first part of the data
				esp_ota_write(ota_handle, body_start_p, body_part_len);
				content_received += body_part_len;
			} else {
				// Write OTA data
				esp_ota_write(ota_handle, ota_buf, recv_len);
				content_received += recv_len;
			}
		} while (recv_len > 0 && content_received < content_length);

		if (esp_ota_end(ota_handle) == ESP_OK) {
			// Lets update the partition
			if (esp_ota_set_boot_partition(update_partition) == ESP_OK) {
				const esp_partition_t *boot_partition = esp_ota_get_boot_partition();
				ESP_LOGI(TAG, "http_server_OTA_update_handler: Next boot partition subtype %d at offset %lx\r\n", boot_partition->subtype, (unsigned long) boot_partition->address);
				flash_successful = true;
			} else {
				ESP_LOGI(TAG, "http_server_OTA_update_handler: esp_ota_end ERROR!!!");
			}
		} else {
			ESP_LOGI(TAG, "http_server_OTA_update_handler: FLASHED ERROR!!!");
		}
	}

	// Don't update the global variables throughout this source file, but send the message about the status
	if (flash_successful) {
		http_server_monitor_send_message(HTTP_MSG_WIFI_UPDATE_SUCCESSFUL);
	} else {
		http_server_monitor_send_message(HTTP_MSG_WIFI_UPDATE_FAILED);
	}

	// We need to let the web page know success/fail
	sprintf(ota_JSON, "{\"ota_update_status\":%d}", flash_successful);
	httpd_resp_set_type(req, "application/json");
	httpd_resp_send(req, ota_JSON, strlen(ota_JSON));

	return ESP_OK;
}

/**
 * System status JSON handler responds with the system status data
 * such as system and network status and DHT22 sensor data.
 * @param req HTTP request for which the URI needs to be handled
 * @return ESP_OK
 */
static esp_err_t http_server_system_status_json_handler(httpd_req_t *req) {
	ESP_LOGI(TAG, "/systemStatus requested");

	char sysStatusJSON[512] = {0};
	char iso_8601[ISO_8601_LENGTH] = {0};
	int64_t uptime = 0;
	char ip[IP4ADDR_STRLEN_MAX] = {0};
	int8_t rssi = 0;
	uint16_t temperature = 0;
	uint8_t humidity = 0;

	get_system_time_iso_8601(iso_8601, ISO_8601_LENGTH);
	get_system_uptime(&uptime);
	wifi_app_get_ip_address(ip);
	wifi_app_get_rssi(&rssi);
	temperature = getTemperature();
	humidity = getHumidity();

	sprintf(sysStatusJSON,
			"{\"system_name\":\"%s\",\"fw_version\":\"%s\",\"iso_8601\":\"%s\",\"uptime\":%lld,\"iso_8601_boot\":\"%s\"," \
			"\"ip\":\"%s\",\"mac\":\"%s\",\"ssid\":\"%s\",\"rssi\":%d,\"temperature\":%d,\"humidity\":%d}",
			system_settings->system_name,
			system_settings->fw_version,
			iso_8601,
			uptime,
			system_settings->iso_8601_boot,
			ip,
			system_settings->mac_address,
			system_settings->ssid,
			rssi,
			temperature,
			humidity);

	httpd_resp_set_type(req, "application/json");
	httpd_resp_send(req, sysStatusJSON, strlen(sysStatusJSON));

	return ESP_OK;
}

/**
 * System settings JSON handler responds with the system settings data
 * @param req HTTP request for which the URI needs to be handled
 * @return ESP_OK
 */
static esp_err_t http_server_system_settings_json_handler(httpd_req_t *req) {
	ESP_LOGI(TAG, "/systemSettings requested");

	char sysSettingsJSON[256];

	sprintf(sysSettingsJSON,
			"{\"system_name\":\"%s\",\"use_led\":%d,\"ssid\":\"%s\"}",
			system_settings->system_name,
			system_settings->use_led,
			system_settings->ssid);

	httpd_resp_set_type(req, "application/json");
	httpd_resp_send(req, sysSettingsJSON, strlen(sysSettingsJSON));

	return ESP_OK;
}

/**
 * System update settings handler receives the system settings entered by the user
 * @param req HTTP request for which the URI needs to be handled.
 * @return ESP_OK
 */
static esp_err_t http_server_update_system_settings_json_handler(httpd_req_t *req) {
	ESP_LOGI(TAG, "/updateSystemSettings: requested");

	size_t len_name = 0, len_use_led = 0;
	char *tmp_name = NULL, *tmp_use_led = NULL;
	char responseJSON[15];
	esp_err_t esp_err;
	uint8_t status = 0;

	// Get system name (max length 31 plus null terminator)
	len_name = httpd_req_get_hdr_value_len(req, "system_name") + 1;

	if (len_name > 0 && len_name < 33) {
		tmp_name = malloc(len_name);

		if (httpd_req_get_hdr_value_str(req, "system_name", tmp_name, len_name) == ESP_OK) {
			ESP_LOGI(TAG, "http_server_update_system_settings_json_handler: Found header => system_name: %s size: %d", tmp_name, len_name);			
		} else {
			status++;
		}
	} else {
		status++;
	}

	// Get use LED (max length 1 plus null terminator)
	len_use_led = httpd_req_get_hdr_value_len(req, "use_led") + 1;

	if (len_use_led == 2) {
		tmp_use_led = malloc(len_use_led);

		if (httpd_req_get_hdr_value_str(req, "use_led", tmp_use_led, len_use_led) == ESP_OK) {
			ESP_LOGI(TAG, "http_server_update_system_settings_json_handler: Found header => use_led: %s size: %d", tmp_use_led, len_use_led);
		} else {
			status++;
		}
	} else {
		status++;
	}

	if (status == 0) {
		// Update system_settings
		memset(system_settings->system_name, 0x00, sizeof(system_settings->system_name));
		memcpy(system_settings->system_name, tmp_name, len_name);
		system_settings->use_led = (bool) atoi(tmp_use_led);

		// Save new settings to NVS
		esp_err = app_nvs_save_system_settings(APP_NVS_SETTINGS_SYSTEM);

		// Set host name from system name. TODO: Needs IF_NET to reconnect?
		wifi_app_set_hostname();

		if (esp_err != ESP_OK) {
			status++;
		}
	}

	free(tmp_name);
	free(tmp_use_led);

	sprintf(responseJSON, "{\"response\":%d}", status);
	httpd_resp_set_type(req, "application/json");
	httpd_resp_send(req, responseJSON, strlen(responseJSON));

	return ESP_OK;
}

/**
 * wifiConnect handler is invoked after the connect button is pressed
 * and handles the receiving the Network/SSID and password entered by the user
 * @param req HTTP request for which the URI needs to be handled.
 * @return ESP_OK
 */
static esp_err_t http_server_wifi_connect_json_handler(httpd_req_t *req) {
	ESP_LOGI(TAG, "/wifiConnect: requested");

	size_t len_ssid = 0, len_pwd = 0;
	char *ssid_str = NULL, *pwd_str = NULL;
	char responseJSON[15];
	uint8_t status = 0;

	// Get SSID header
	len_ssid = httpd_req_get_hdr_value_len(req, "ssid") + 1;

	if (len_ssid > 1 && len_ssid < MAX_SSID_LENGTH) {
		ssid_str = malloc(len_ssid);

		if (httpd_req_get_hdr_value_str(req, "ssid", ssid_str, len_ssid) == ESP_OK) {
			ESP_LOGI(TAG, "http_server_wifi_connect_json_handler: Found header => ssid: %s", ssid_str);
		} else {
			status++;
		}
	} else {
		status++;
	}

	// Get Password header
	len_pwd = httpd_req_get_hdr_value_len(req, "password") + 1;

	if (len_pwd > 1 && len_pwd < MAX_PASSWORD_LENGTH) {
		pwd_str = malloc(len_pwd);

		if (httpd_req_get_hdr_value_str(req, "password", pwd_str, len_pwd) == ESP_OK) {
			ESP_LOGI(TAG, "http_server_wifi_connect_json_handler: Found header => password: %s", pwd_str);
		} else {
			status++;
		}
	} else {
		status++;
	}

	// Set the status for the JSON response here. We send it later at the appropriate juncture
	sprintf(responseJSON, "{\"response\":%d}", status);
	httpd_resp_set_type(req, "application/json");

	if (status == 0) {
		// Update the WiFi network config & system settings (we save to NVS later after successful WiFi association)
		wifi_config_t *wifi_config = wifi_app_get_wifi_config();
		memset(wifi_config, 0x00, sizeof(wifi_config_t));
		memcpy(wifi_config->sta.ssid, ssid_str, len_ssid);
		memcpy(wifi_config->sta.password, pwd_str, len_pwd);
		sprintf(system_settings->ssid, "%s", ssid_str);
		sprintf(system_settings->wifi_password, "%s", pwd_str);

		// Send response back to browser BEFORE we do a delay AND WiFI disconnect
		httpd_resp_send(req, responseJSON, strlen(responseJSON));

		// Let the WiFi application know after a brief delay
		vTaskDelay(150 / portTICK_PERIOD_MS);	// TODO: don't love this, toggle LED instead?
		wifi_app_send_message(WIFI_APP_MSG_HTTP_SERVER_REQUESTED_STA_DISCONNECT);
	} else {
		httpd_resp_send(req, responseJSON, strlen(responseJSON));
	}

	free(ssid_str);
	free(pwd_str);

	return ESP_OK;
}

/**
 * wifiConnectStatus handler updates the connection status for the web page.
 * @param req HTTP request for which the URI needs to be handled.
 * @return ESP_OK
 */
 static esp_err_t http_server_wifi_connect_status_json_handler(httpd_req_t *req) {
	ESP_LOGI(TAG, "/wifiConnectStatus requested");

	char statusJSON[26];

	sprintf(statusJSON, "{\"wifi_connect_status\":%d}", g_wifi_connect_status);
	httpd_resp_set_type(req, "application/json");
	httpd_resp_send(req, statusJSON, strlen(statusJSON));

	return ESP_OK;
}

/**
  * Sets up the default httpd server configuration
 * @return http_server instance handle if successful, NULL otherwise
 */
static httpd_handle_t http_server_configure(void) {
	// Generate the default configuration
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();

	// Create HTTP server monitor task
	xTaskCreatePinnedToCore(&http_server_monitor, "http_server_monitor", HTTP_SERVER_MONITOR_STACK_SIZE, NULL, HTTP_SERVER_TASK_PRIORITY, &task_http_server_monitor, HTTP_SERVER_TASK_CORE_ID);

	// Create the message queue
	http_server_monitor_queue_handle = xQueueCreate(3, sizeof(http_server_queue_message_t));

	// The core the HTTP server will run on
	config.core_id = HTTP_SERVER_TASK_CORE_ID;

	// Adjust the default priority to 1 less than the WiFi application task
	config.task_priority = HTTP_SERVER_TASK_PRIORITY;

	// Bump up the stack size (default is 4096)
	config.stack_size = HTTP_SERVER_TASK_STACK_SIZE;

	// Increase URI handlers
	config.max_uri_handlers = 20;

	// Increase the timeout limits (seconds)
	config.recv_wait_timeout = 10;
	config.send_wait_timeout = 10;

	ESP_LOGI(TAG,
			"http_server_configure(): Starting server on port: '%d' with task priority: '%d'",
			config.server_port,
			config.task_priority);

	// Start the HTTPD server
	if (httpd_start(&http_server_handle, &config) == ESP_OK) {
		ESP_LOGI(TAG, "http_server_configure(): Registration URI handlers");

		// register index.html handler
		httpd_uri_t index_html = {
			.uri = "/index.html",
			.method = HTTP_GET,
			.handler = http_server_index_html_handler,
			.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &index_html);

		// register settings.html handler
		httpd_uri_t settings_html = {
			.uri = "/settings.html",
			.method = HTTP_GET,
			.handler = http_server_settings_html_handler,
			.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &settings_html);

		// register app.css handler
		httpd_uri_t app_css = {
			.uri = "/app.css",
			.method = HTTP_GET,
			.handler = http_server_app_css_handler,
			.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &app_css);

		// register ota.js handler
		httpd_uri_t ota_js = {
			.uri = "/ota.js",
			.method = HTTP_GET,
			.handler = http_server_ota_js_handler,
			.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &ota_js);

		// register settings.js handler
		httpd_uri_t settings_js = {
			.uri = "/settings.js",
			.method = HTTP_GET,
			.handler = http_server_settings_js_handler,
			.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &settings_js);

		// register status.js handler
		httpd_uri_t status_js = {
			.uri = "/status.js",
			.method = HTTP_GET,
			.handler = http_server_status_js_handler,
			.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &status_js);

		// register wifi.js handler
		httpd_uri_t wifi_js = {
			.uri = "/wifi.js",
			.method = HTTP_GET,
			.handler = http_server_wifi_js_handler,
			.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &wifi_js);

		// register favicon.ico handler
		httpd_uri_t favicon_ico = {
			.uri = "/favicon.ico",
			.method = HTTP_GET,
			.handler = http_server_favicon_ico_handler,
			.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &favicon_ico);

		// register error.ico handler
		httpd_uri_t error_ico = {
			.uri = "/error.ico",
			.method = HTTP_GET,
			.handler = http_server_error_ico_handler,
			.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &error_ico);

		// register success.ico handler
		httpd_uri_t success_ico = {
			.uri = "/success.ico",
			.method = HTTP_GET,
			.handler = http_server_success_ico_handler,
			.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &success_ico);

		// register info.ico handler
		httpd_uri_t info_ico = {
			.uri = "/info.ico",
			.method = HTTP_GET,
			.handler = http_server_info_ico_handler,
			.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &info_ico);

		// register OTAupdate handler
		httpd_uri_t OTA_update = {
			.uri = "/OTAupdate",
			.method = HTTP_POST,
			.handler = http_server_OTA_update_handler,
			.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &OTA_update);

		// register systemStatus handler
		httpd_uri_t system_status_json = {
			.uri = "/systemStatus",
			.method = HTTP_GET,
			.handler = http_server_system_status_json_handler,
			.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &system_status_json);

		// register systemSettings handler
		httpd_uri_t system_settings_json = {
			.uri = "/systemSettings",
			.method = HTTP_GET,
			.handler = http_server_system_settings_json_handler,
			.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &system_settings_json);

		// register /updateSystemSettings handler
		httpd_uri_t update_system_settings_json = {
			.uri = "/updateSystemSettings",
			.method = HTTP_POST,
			.handler = http_server_update_system_settings_json_handler,
			.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &update_system_settings_json);

		// register wifiConnect handler
		httpd_uri_t wifi_connect_json = {
			.uri = "/wifiConnect",
			.method = HTTP_POST,
			.handler = http_server_wifi_connect_json_handler,
			.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &wifi_connect_json);

		// register wifiConnectStatus handler
		httpd_uri_t wifi_connect_status_json = {
			.uri = "/wifiConnectStatus",
			.method = HTTP_GET,
			.handler = http_server_wifi_connect_status_json_handler,
			.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &wifi_connect_status_json);

		return http_server_handle;
	}

	return NULL;
}

void http_server_start(void) {
	if (http_server_handle == NULL) {
		http_server_handle = http_server_configure();
	}
}

void http_server_stop(void) {
	if (http_server_handle) {
		httpd_stop(http_server_handle);
		ESP_LOGI(TAG, "http_server_stop: stopping HTTP server");
		http_server_handle = NULL;
	}

	if (task_http_server_monitor) {
		vTaskDelete(task_http_server_monitor);
		ESP_LOGI(TAG, "http_server_stop: stopping HTTP server monitor");
		task_http_server_monitor = NULL;
	}
}

int http_server_monitor_send_message(http_server_message_e msgID) {
	http_server_queue_message_t msg;
	msg.msgID = msgID;
	return xQueueSend(http_server_monitor_queue_handle, &msg, portMAX_DELAY);
}

void http_server_fw_update_reset_callback(void *arg) {
	ESP_LOGI(TAG, "http_server_fw_update_reset_callback: Timer timed-out, restarting the device");
	esp_restart();
}
