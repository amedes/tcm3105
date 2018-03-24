#ifndef __UART_H__
#define __UART_H__

#include <stdint.h>

void uart_init(void);
int uart_read(uint8_t *buf, uint32_t len);
int uart_write(uint8_t *buf, uint32_t len);
int uart_clear(void);
int uart_wait(void);

#endif /* __UART_H__ */
