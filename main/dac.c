#include "driver/gpio.h"
#include "driver/dac.h"

//#define TCM3105_RXB_VAL ((int)(2.725/5.0*255+0.5)) // 2.7V @5.0V
#define TCM3105_RXB_VAL 137
#define TCM3105_CDL_VAL ((int)(3.3/5.0*255+0.5)) // 3.3V @5.0V

void dac_init(void)
{
    dac_output_enable(DAC_CHANNEL_1); // GPIO25
    dac_output_voltage(DAC_CHANNEL_1, TCM3105_RXB_VAL);
#if 0
    dac_output_enable(DAC_CHANNEL_2); // GPIO26
    dac_output_voltage(DAC_CHANNEL_2, TCM3105_CDL_VAL);
#endif
}
