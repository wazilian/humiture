# See the build system documentation in IDF programming guide
# for more information about component CMakeLists.txt files.

idf_component_register(
    SRCS main.c rgb_led.c wifi_app.c http_server.c DHT22.c app_nvs.c sntp_time_sync.c system_settings.c
    INCLUDE_DIRS "."
    EMBED_FILES www/index.html www/settings.html www/app.css www/ota.js www/settings.js www/status.js www/wifi.js www/error.ico www/favicon.ico www/info.ico www/success.ico
)
