#ifndef __WIFI_H__
#define __WIFI_H__

#include <stdint.h>
#include <lwip/api.h>
#include <freertos/event_groups.h>

#define CONNECTED_BIT BIT0
#define DISCONNECTED_BIT BIT1

extern EventGroupHandle_t wifi_event_group;

void wifi_connect_ap(void);
#endif /* __WIFI_H__ */
