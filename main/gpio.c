#include "driver/gpio.h"

#define GPIO_RXD_PIN 32		// input RXD signal
#define GPIO_CDT_PIN 14		// input CDT signal
#define GPIO_LED_PIN 19		// output RXD CDT state to LED
//#define GPIO_CLK_PIN 27		// input CLK signal
//#define GPIO_TRIGGER_PIN 12	// trigger pluse for test
#define ESP_INTR_FLAG_DEFAULT 0

#ifdef GPIO_CLK_PIN
#define GPIO_TRS_PIN 12		// output TRS signal
#endif

int cdt_state = 1;

static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    gpio_set_level(GPIO_LED_PIN, cdt_state && gpio_get_level(gpio_num));
}

static void IRAM_ATTR gpio_cdt_isr_handler(void *arg)
{
    cdt_state = gpio_get_level(GPIO_CDT_PIN);
}

#ifdef GPIO_CLK_PIN
static void IRAM_ATTR gpio_clk_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    gpio_set_level(GPIO_TRS_PIN, !gpio_get_level(gpio_num));
}
#endif

void gpio_init(void)
{
    // RXD
    gpio_config_t io_conf = {
	.intr_type = GPIO_INTR_ANYEDGE,
	.mode = GPIO_MODE_INPUT,
	.pin_bit_mask = (1ULL << GPIO_RXD_PIN),
	.pull_down_en = 0,
	.pull_up_en = 0
    };
    gpio_config(&io_conf);
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    gpio_isr_handler_add(GPIO_RXD_PIN, gpio_isr_handler, (void *)GPIO_RXD_PIN);
    gpio_intr_enable(GPIO_RXD_PIN);

    // CDT
    io_conf.pin_bit_mask = (1ULL << GPIO_CDT_PIN);
    gpio_config(&io_conf);
    gpio_isr_handler_add(GPIO_CDT_PIN, gpio_cdt_isr_handler, (void *)GPIO_CDT_PIN);
    gpio_intr_enable(GPIO_CDT_PIN);

    // CLK
#ifdef GPIO_CLK_PIN
    io_conf.pin_bit_mask = (1ULL << GPIO_CLK_PIN);
    gpio_config(&io_conf);
    gpio_isr_handler_add(GPIO_CLK_PIN, gpio_clk_isr_handler, (void *)GPIO_CLK_PIN);
    gpio_intr_enable(GPIO_CLK_PIN);
#endif

    // LED
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << GPIO_LED_PIN);
    gpio_config(&io_conf);
    gpio_set_level(GPIO_LED_PIN, gpio_get_level(GPIO_RXD_PIN));

    // TRIGGER
#ifdef GPIO_TRIGGER_PIN
    io_conf.pin_bit_mask = (1ULL << GPIO_TRIGGER_PIN);
    gpio_config(&io_conf);
    gpio_set_level(GPIO_TRIGGER_PIN, 0);
#endif

    // TRS
#ifdef GPIO_TRS_PIN
    io_conf.pin_bit_mask = (1ULL << GPIO_TRS_PIN);
    gpio_config(&io_conf);
#endif
}
