#include <stdio.h>
#include <stdint.h>
#include "driver/uart.h"
#include "esp_err.h"

//#define UART_INPUT_PIN GPIO_NUM_32
#define UART_INPUT_PIN UART_PIN_NO_CHANGE
#define UART_OUTPUT_PIN GPIO_NUM_2
#define UART_PORT UART_NUM_1

#define BAUD_RATE 1200 // 1200 bps
#define BUF_SIZE 256

void uart_init(void)
{
    uart_config_t uart_config = {
	    .baud_rate = BAUD_RATE,
	    .data_bits = UART_DATA_8_BITS,
	    .parity = UART_PARITY_DISABLE,
	    .stop_bits = UART_STOP_BITS_1,
	    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
	    .use_ref_tick = false
    };

    uart_param_config(UART_PORT, &uart_config);
    uart_set_pin(UART_PORT, UART_OUTPUT_PIN, UART_INPUT_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_PORT, BUF_SIZE, BUF_SIZE, 0, NULL, 0);
    uart_set_tx_idle_num(UART_PORT, 0);
}

int uart_read(void *buf, uint32_t len)
{
    return uart_read_bytes(UART_PORT, buf, len, 1000 / portTICK_PERIOD_MS); // no wait
}

int uart_write(void *buf, uint32_t len)
{
    return uart_write_bytes(UART_PORT, buf, len);
}

int uart_clear(void)
{
    return uart_flush_input(UART_PORT);
}

int uart_wait(void)
{
    return uart_wait_tx_done(UART_PORT, portMAX_DELAY);
}
