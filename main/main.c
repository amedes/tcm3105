#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "lwip/tcp.h"

#include "wifi.h"
//#include "uart.h"
#include "ledc.h"
#include "gpio.h"
#include "dac.h"
#include "mcpwm.h"
#include "rmt.h"

#define MODEM_PORT 3105

EventGroupHandle_t task_event;
QueueHandle_t cap_queue;

#define TCP_WRITER_EXIT BIT0
#define TCP_READER_EXIT BIT1

#define BUF_SIZE 1436

static uint8_t wdata[BUF_SIZE];

void tcp_writer(void *arg)
{
    struct netconn *conn = *(struct netconn **)arg;
    err_t err;
    int top;
    uint32_t temp;

    while (1) {
	vTaskSuspend(NULL); // wait for resume

	// clear capture queue
	while (xQueueReceive(cap_queue, &temp, 0) == pdTRUE) {
	}

	xEventGroupClearBits(task_event, TCP_WRITER_EXIT);

	printf("tcp_writer() start\n");
	//vTaskDelay(1000 / portTICK_PERIOD_MS);

	conn = *(struct netconn **)arg;
	top = 0;
	while (1) {
	    if (xQueueReceive(cap_queue, &wdata[top], 1000 / portTICK_PERIOD_MS) == pdTRUE) {
		//printf("uart_read: %d byte\n", len);
		top += sizeof(uint32_t);
		if (top >= BUF_SIZE) {
		    err = netconn_write(conn, wdata, BUF_SIZE, NETCONN_COPY);
		    printf("netconn_write: %d byte\n", BUF_SIZE);
		    if (err != ERR_OK) break;
		    top = 0;
		}
	    } else {
		if (xEventGroupGetBits(task_event) & TCP_READER_EXIT) break;
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	    }
	}
	if (top > 0) {
	    err = netconn_write(conn, wdata, top, NETCONN_COPY);
	    printf("tcp_writer: %d byte write\n", top);
	}

	printf("tcp_writer() done\n");
	xEventGroupSetBits(task_event, TCP_WRITER_EXIT); // inform writer exit
    }
    vTaskDelete(NULL); // end of task
}

#define RMT_ITEM32_SIZE 2048
#define ID_TXD_DATA 0x0001
#define TXD_HEADER 1
#define TXD_DATA   2

void tcp_reader(void *arg)
{
    struct netconn *conn;
    struct netbuf *nbuf;
    err_t err;
    static rmt_item32_t item32[2048];
    uint8_t *item8 = (uint8_t *)item32;
    union {
	uint32_t val;
	struct {
	    uint32_t size:16;
	    uint32_t id:16;
	};
	uint8_t byte[sizeof(uint32_t)];
    } header;
    int index;
    int state;
    int item32_len = 0;

    while (1) {
	vTaskSuspend(NULL);
	xEventGroupClearBits(task_event, TCP_READER_EXIT);

	printf("tcp_reader() start\n");

	conn = *(struct netconn **)arg;
	//netconn_set_recvtimeout(conn, 1000);
	state = TXD_HEADER;
	index = 0;
	while ((err = netconn_recv(conn, &nbuf)) == ERR_OK) {
	    printf("netconn_recv: %d byte\n", netbuf_len(nbuf));
	    do {
		uint8_t *data;
		u16_t len;
		int i;

		netbuf_data(nbuf, (void **)&data, &len);
		for (i = 0; i < len; i++) {
		    switch (state) {
		    case TXD_HEADER:
		        header.byte[index++] = data[i];
			if (index < sizeof(header)) break;
			if (header.id != ID_TXD_DATA) {
			    printf("header ID not match: %u\n", header.id);
			    index = 0;
			    break;
			}
			state = TXD_DATA;
			index = 0;
			item32_len = header.size * sizeof(item32[0]);
			break;
		    case TXD_DATA:
			item8[index++] = data[i];
			if (index < item32_len) break;
			rmt_tx_item(item32, header.size);
			printf("rmt_tx_item: %d items\n", header.size);
			state = TXD_HEADER;
			index = 0;
		        break;
		    }
		}
	    } while (netbuf_next(nbuf) >= 0);
	    netbuf_delete(nbuf);
	}
	printf("netconn_recv error: %d\n", err);

	//uart_wait(); // wait for sending all data
	printf("tcp_reader() done\n");
	xEventGroupSetBits(task_event, TCP_READER_EXIT); // inform reader exit
    }
    vTaskDelete(NULL);
}

void callback(struct netconn *conn, enum netconn_evt event, u16_t len)
{
    const char *evt_name[] = {
	    "RCVPLUS",
	    "RCVMINUS",
	    "SENDPLUS",
	    "SENDMINUS",
	    "ERROR",
    };

    printf("event: %s, len: %d\n", evt_name[event], len);
}

void app_main()
{
    struct netconn *conn;
    struct netconn *newconn;
    err_t xErr;
    xTaskHandle read_task;
    xTaskHandle write_task;

    cap_queue = xQueueCreate(2048, sizeof(uint32_t));

    // initialize peripherals
    ledc_init(); // clock for TCM3105
    mcpwm_initialize(cap_queue); // capture RXD signal, TRS
    gpio_init(); // LED indicate RXD state
    //uart_init();
    dac_init(); // voltage for RXB
    
    rmt_tx_init(); // output pulse train to TXD

    wifi_connect_ap();
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

    printf("WiFi connected\n");

    //conn = netconn_new(NETCONN_TCP);
    conn = netconn_new_with_callback(NETCONN_TCP, callback);
    xErr = netconn_bind(conn, IP_ADDR_ANY, MODEM_PORT);
    printf("netconn_bind: %d\n", xErr);
    xErr = netconn_listen(conn);
    printf("netconn_listen: %d\n", xErr);

    task_event = xEventGroupCreate();

    if (xTaskCreate(tcp_writer, "tcp_writer", 4096, &newconn, tskIDLE_PRIORITY, &write_task) != pdPASS) {
	printf("tcp_writer task creation fail\n");
	vTaskDelete(NULL);
    }

    if (xTaskCreate(tcp_reader, "tcp_reader", 4096, &newconn, tskIDLE_PRIORITY, &read_task) !=  pdPASS) {
	printf("tcp_reader task creation fail\n");
	vTaskDelete(NULL);
    }

    while (1) {
	printf("wait for connection\n");
	xErr = netconn_accept(conn, &newconn);
	printf("netconn_accept: %d\n", xErr);
	if (xErr != ERR_OK) continue;

	//uart_clear(); // clear UART input buffer
	cap_queue_err = 0;

	vTaskResume(read_task);
	vTaskResume(write_task);

	// wait for end signal from task
	xEventGroupWaitBits(task_event,
			TCP_READER_EXIT | TCP_WRITER_EXIT,
			pdTRUE, // clear on exit
			pdTRUE, // wait for all bits
			portMAX_DELAY); // wait forever
	//printf("task_event: %u\n", xEventGroupGetBits(task_event));

	xErr = netconn_delete(newconn);
	printf("netconn_delete(): %d\n", xErr);
	printf("cap_queue_err: %d\n", cap_queue_err);
    }
}
