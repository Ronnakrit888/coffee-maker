#include "sensor_stm32.h"
#include "exti_handlers.h"
#include "gpio_types.h"
#include "stm32f4xx.h"
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include "state_globals.h"

#define LIGHT_ROAST_INDEX 0
#define MEDIUM_ROAST_INDEX 1
#define MEDIUM_DARK_ROAST_INDEX 2
#define DARK_ROAST_INDEX 3

#define MAX_SAFE_TEMP_C 5

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

    current_temperature = 0;
    uint8_t selected_roast_index = state_selections[4];

    if (selected_roast_index == MEDIUM_DARK_ROAST_INDEX ||
        selected_roast_index == DARK_ROAST_INDEX)
    {

        current_temperature = calculateTemperature();

        if (current_temperature > MAX_SAFE_TEMP_C)
        {
            return 1;
        }
    }

    return 0;
}

uint16_t readPotentiometer(void)
{
    return adc_value;
}

uint8_t getTampingLevel(uint16_t adc_value)
{
    if (adc_value < 820)
        return 0;
    else if (adc_value < 1639)
        return 1;
    else if (adc_value < 2458)
        return 2;
    else if (adc_value < 3277)
        return 3;
    else
        return 4;
}

const char *getTampingDescription(uint8_t level)
{

    const char *descriptions[5] = {
        "Mild, Delicate, Fruity Notes",
        "Smooth, Balanced, Light Body",
        "Rich, Full-bodied, Balanced",
        "Bold, Intense, Strong Flavor",
        "Very Bold, Robust, Maximum Extraction"};

    if (level > 4)
        level = 4;
    return descriptions[level];
}

float readLightIntensity(void)
{
	// Configure ADC to read from channel 1 (PA1)
	ADC1->SQR3 &= ~(ADC_SQR3_SQ1);
	ADC1->SQR3 |= (1 << ADC_SQR3_SQ1_Pos);
	ADC1->SQR1 &= ~(ADC_SQR1_L);
	ADC1->SQR1 |= (1 << ADC_SQR1_L_Pos);
	ADC1->SMPR2 |= ADC_SMPR2_SMP1;

	// Start conversion
	ADC1->CR2 |= ADC_CR2_SWSTART;

	// Wait for conversion to complete
	while ((ADC1->SR & ADC_SR_EOC) == 0)
		;

	// Read ADC value
	uint16_t adc_raw = ADC1->DR;

	// Calculate voltage
	float adc_voltage = (adc_raw * VREF) / ADC_MAXES;

	// Calculate LDR resistance
	float r_ldr = RX * (adc_voltage / (VREF - adc_voltage));

	// Calculate light intensity using logarithmic formula
	float log_resistance = log10(r_ldr);
	float x = (log_resistance - OFFSET) / SLOPE;
	float light_intensity = powf(10.0f, x);

	return light_intensity;
}