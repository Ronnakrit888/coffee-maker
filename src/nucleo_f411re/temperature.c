#include "temperature.h"
#include "exti_handlers.h"
#include "gpio_types.h"
#include "stm32f4xx.h"
#include <stdio.h>
#include <stdint.h>
#include <math.h>

#define LIGHT_ROAST_INDEX 0
#define MEDIUM_ROAST_INDEX 1
#define MEDIUM_DARK_ROAST_INDEX 2
#define DARK_ROAST_INDEX 3

#define MAX_SAFE_TEMP_C 20

int32_t calculateTemperature(void)
{
    // Start ADC Conversion and wait for it to complete

    ADC1->CR2 |= ADC_CR2_SWSTART;
    while ((ADC1->SR & ADC_SR_EOC) == 0)
        ;

    // Read ADC and apply NTC formula
    float adc_voltage = (ADC1->DR * VREF / ADC_MAXES);
    float r_ntc = RX * adc_voltage / (VCC - adc_voltage);
    float temperature = ((BETA * T0) / (T0 * log(r_ntc / R0) + BETA)) - 273.15f;

    // Return the float temperature value cast to the required int32_t type
    return (int32_t)temperature;
}

uint8_t checkRoastTemperatureSafety(void)
{

    uint8_t selected_roast_index = state_selections[3];

    if (selected_roast_index == MEDIUM_DARK_ROAST_INDEX ||
        selected_roast_index == DARK_ROAST_INDEX)
    {

        int32_t current_temp = calculateTemperature();

        if (current_temp > MAX_SAFE_TEMP_C)
        {
            return 1;
        }
    }

    return 0;
}