#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_attr.h"
#include "driver/mcpwm.h"
#include "driver/gpio.h"
#include "soc/mcpwm_reg.h"
#include "soc/mcpwm_struct.h"
#include "esp_sleep.h"

#define CAP0_INT_EN BIT(27)  //Capture 0 interrupt bit
#define CAP1_INT_EN BIT(28)  //Capture 1 interrupt bit
#define CAP2_INT_EN BIT(29)  //Capture 2 interrupt bit

#define GPIO_PWM0A_OUT 12   //Set GPIO 19 as PWM0A
#define GPIO_PWM0B_OUT 13   //Set GPIO 18 as PWM0B
#define GPIO_PWM1A_OUT 17   //Set GPIO 17 as PWM1A
#define GPIO_PWM1B_OUT 16   //Set GPIO 16 as PWM1B
#define GPIO_PWM2A_OUT 15   //Set GPIO 15 as PWM2A
#define GPIO_PWM2B_OUT 14   //Set GPIO 14 as PWM2B
#define GPIO_CAP0_IN   32   //Set GPIO 23 as  CAP0
#define GPIO_CAP1_IN   32   //Set GPIO 25 as  CAP1
#define GPIO_CAP2_IN   26   //Set GPIO 26 as  CAP2
#define GPIO_SYNC0_IN  27   //Set GPIO 02 as SYNC0
#define GPIO_SYNC1_IN   4   //Set GPIO 04 as SYNC1
#define GPIO_SYNC2_IN   5   //Set GPIO 05 as SYNC2
#define GPIO_FAULT0_IN 32   //Set GPIO 32 as FAULT0
#define GPIO_FAULT1_IN 34   //Set GPIO 34 as FAULT1
#define GPIO_FAULT2_IN 34   //Set GPIO 34 as FAULT2


static xQueueHandle cap_queue;

static mcpwm_dev_t *MCPWM[2] = {&MCPWM0, &MCPWM1};

int cap_queue_err = 0;

/**
 * @brief this is ISR handler function, here we check for interrupt that triggers rising edge on CAP0 signal and according take action
 */
static void IRAM_ATTR mcpwm_isr_handler()
{
    uint32_t mcpwm_intr_status;
    uint32_t ts;
    //capture evt;

    mcpwm_intr_status = MCPWM[MCPWM_UNIT_0]->int_st.val; //Read interrupt status

    if (mcpwm_intr_status & CAP0_INT_EN) { //Check for interrupt on rising edge on CAP0 signal
	ts = mcpwm_capture_signal_get_value(MCPWM_UNIT_0, MCPWM_SELECT_CAP0);
	ts &= (~1); // clear LSB for indicate positive edge
	if (xQueueSendFromISR(cap_queue, &ts, NULL) != pdTRUE) cap_queue_err++;
    }

    if (mcpwm_intr_status & CAP1_INT_EN) { //Check for interrupt on falling edge on CAP1 signal
	ts = mcpwm_capture_signal_get_value(MCPWM_UNIT_0, MCPWM_SELECT_CAP1);
	ts |= 1; // set LSB for indicate negative edge
	if (xQueueSendFromISR(cap_queue, &ts, NULL) != pdTRUE) cap_queue_err++;
    }

    MCPWM[MCPWM_UNIT_0]->int_clr.val = mcpwm_intr_status;
}

void mcpwm_initialize(xQueueHandle queue)
{
    mcpwm_config_t pwm_config = {
    	.frequency = 19110,   //frequency = 19110Hz
    	.cmpr_a = 50.0,       //duty cycle of PWMxA = 50.0%
    	.cmpr_b = 50.0,       //duty cycle of PWMxb = 50.0%
    	.counter_mode = MCPWM_UP_COUNTER,
    	.duty_mode = MCPWM_DUTY_MODE_1,
    };
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);

    cap_queue = queue;

#ifdef GPIO_PWM0A_OUT
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, GPIO_PWM0A_OUT);
#endif
#ifdef GPIO_SYNC0_IN
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM_SYNC_0, GPIO_SYNC0_IN);
#endif
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM_CAP_0, GPIO_CAP0_IN);
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM_CAP_1, GPIO_CAP1_IN);

    // enable sync input
#ifdef GPIO_SYNC0_IN
    mcpwm_sync_enable(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_SELECT_SYNC0, 0); // phase_val 50.0%
#endif

#define ESP_INTR_FLAG_DEFAULT (0)

    mcpwm_isr_register(MCPWM_UNIT_0, mcpwm_isr_handler, NULL, ESP_INTR_FLAG_DEFAULT, NULL);

#define CAP_PRESCALE (0) // prescale 1/(value + 1)

    // use two capture channel to detect both positive and negative edge
    mcpwm_capture_enable(MCPWM_UNIT_0, MCPWM_SELECT_CAP0, MCPWM_POS_EDGE, CAP_PRESCALE);
    mcpwm_capture_enable(MCPWM_UNIT_0, MCPWM_SELECT_CAP1, MCPWM_NEG_EDGE, CAP_PRESCALE);

    // enable interrupt
    MCPWM[MCPWM_UNIT_0]->int_ena.val = CAP0_INT_EN | CAP1_INT_EN;
}
