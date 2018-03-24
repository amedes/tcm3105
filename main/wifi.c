#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_clk.h"
#include "rom/ets_sys.h"
#include "nvs_flash.h"

#include "lwip/api.h"
#include "lwip/ip_addr.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "lwip/netbuf.h"

#include "wifi.h"

#define TX_POWER -4 // WiFi TX power, 127 is level 0

#define WIFI_SSID	CONFIG_ESP_WIFI_SSID
#define WIFI_PASS	CONFIG_ESP_WIFI_PASSWORD

static const char tag[] = "wifi", *TAG = tag;

EventGroupHandle_t wifi_event_group; // event for wifi

//const int CONNECTED_BIT = BIT0;
//const int DISCONNECTED_BIT = BIT1;

esp_err_t event_handler(void *ctx, system_event_t *event)
{
	switch (event->event_id) {
	case SYSTEM_EVENT_STA_START:
		ESP_LOGW(TAG, "SYSTEM_EVENT_STA_START");
		ESP_ERROR_CHECK(esp_wifi_set_max_tx_power(TX_POWER)); // -3dBm?
		ESP_ERROR_CHECK(esp_wifi_connect());
		break;
	case SYSTEM_EVENT_STA_GOT_IP:
		ESP_LOGW(TAG, "SYSTEM_EVENT_STA_GOT_IP");
		xEventGroupClearBits(wifi_event_group, DISCONNECTED_BIT);
		xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
		break;
	case SYSTEM_EVENT_STA_CONNECTED:
		ESP_LOGW(TAG, "SYSTEM_EVENT_STA_CONNECTED");
		break;
	case SYSTEM_EVENT_STA_DISCONNECTED:
		xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
		xEventGroupSetBits(wifi_event_group, DISCONNECTED_BIT);
		ESP_LOGW(TAG, "SYSTEM_EVENT_STA_DISCONNECTED");
		system_event_sta_disconnected_t *e = &event->event_info.disconnected;
		if (e->ssid_len < 32) {
			e->ssid[e->ssid_len] = 0;
		} else {
			e->ssid[31] = 0;
		}
		ESP_LOGI(TAG, "ssid: %s", e->ssid);
		ESP_LOGI(TAG, "bssid: %02x:%02x:%02x:%02x:%02x:%02x",
			e->bssid[0], e->bssid[1], e->bssid[2], e->bssid[3], e->bssid[4], e->bssid[5]);
		ESP_LOGI(TAG, "reason: %u", e->reason);
		ESP_LOGI(TAG, "esp_wifi_connect()");
		vTaskDelay(10000 / portTICK_PERIOD_MS); // wait 10 sec
		ESP_ERROR_CHECK(esp_wifi_connect());
		break;
	case SYSTEM_EVENT_STA_STOP:
		ESP_LOGW(TAG, "SYSTEM_EVENT_STA_STOP");
		break;
	default:
		ESP_LOGI(TAG, "event_handler: %d", event->event_id);
		break;
	}
	return ESP_OK;
}

void wifi_connect_ap(void)
{
	ESP_LOGI(TAG, "wifi_connect_ap()");

	nvs_flash_init();
	tcpip_adapter_init();

	wifi_event_group = xEventGroupCreate();
	ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	//ESP_ERROR_CHECK(esp_wifi_set_country(WIFI_COUNTRY_JP));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	wifi_config_t wifi_config = {
	    .sta = {
		.ssid = WIFI_SSID,
		.password = WIFI_PASS,
	    },
	};
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());
}
