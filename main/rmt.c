#include <string.h>
#include "esp_log.h"
#include "esp_err.h"
#include "driver/rmt.h"
#include "driver/gpio.h"

//#include "cw.h"

#define RMT_TX_CHANNEL RMT_CHANNEL_0
#define RMT_TX_GPIO 2

#define RMT_RX_CHANNEL RMT_CHANNEL_1
#define RMT_RX_GPIO 32

#define RMT_CLK_DIV 16
#define RMT_DURATION ((80*1000*1000 / RMT_CLK_DIV + 600) / 1200) // 1200bps

#define GPIO_TRIGGER_PIN 12

/*
 * Initialize the RMT Tx channel
 */
void rmt_tx_init(void)
{
    rmt_config_t config = {
    	.rmt_mode = RMT_MODE_TX,
    	.channel = RMT_TX_CHANNEL,
    	.gpio_num = RMT_TX_GPIO,
    	.mem_block_num = 1,
    	.tx_config.loop_en = 0,
    	.tx_config.carrier_en = 0,
    	.tx_config.idle_output_en = 1,
    	.tx_config.idle_level = 1,
    	.clk_div = RMT_CLK_DIV
    };
    ESP_ERROR_CHECK(rmt_config(&config));
    ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));
}

#define MAX_PACKET_SIZE 1024
#define ITEM32_SIZE (MAX_PACKET_SIZE*32/2)
#define NUM_BUF 2
#define TX_STATE_HEADER 1
#define TX_STATE_DATA 2

// double buffering
static rmt_item32_t txbuf[NUM_BUF][ITEM32_SIZE];

void rmt_tx_item(rmt_item32_t item32[], int size)
{
    static int num = 0;
    int buf_size = (size < ITEM32_SIZE) ? size : ITEM32_SIZE;

    memcpy(txbuf[num], item32, buf_size * sizeof(rmt_item32_t));

    ESP_ERROR_CHECK(rmt_wait_tx_done(RMT_TX_CHANNEL, portMAX_DELAY));
#ifdef GPIO_TRIGGER_PIN
    gpio_set_level(GPIO_TRIGGER_PIN, 1);
#endif
    ESP_ERROR_CHECK(rmt_write_items(RMT_TX_CHANNEL, txbuf[num], buf_size, false));
#ifdef GPIO_TRIGGER_PIN
    gpio_set_level(GPIO_TRIGGER_PIN, 0);
#endif
    num = (num + 1) % NUM_BUF;
}

int is_modem_busy(void)
{
    return rmt_wait_tx_done(RMT_TX_CHANNEL, 0);
}
