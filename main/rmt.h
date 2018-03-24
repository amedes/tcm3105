#ifndef __RMT_H__
#define __RMT_H__

#include <stdint.h>
#include "driver/rmt.h"

void rmt_tx_init(void);
//void send_packet(char data[], int len);
//void rmt_send_pulses(rmt_item32_t *item32, int item32_size);
//int send_data_to_modem(uint8_t data[], int len);
int is_modem_busy(void);
void rmt_tx_item(rmt_item32_t item32[], int size);

#endif /* __RMT_H__ */
