#ifndef SENSOR_STM32_H
#define SENSOR_STM32_H

#include "stm32f4xx.h"
#include <stdio.h>
#include <stdint.h>

#define VREF 3.3f
#define VCC 3.3f
#define ADC_MAXES 4095.0f
#define RX 10000.0f
#define R0 10000.0f
#define T0 298.15f
#define BETA 3950.0f
#define SLOPE -0.6875f
#define OFFSET 5.1276f

// Temperature
int32_t calculateTemperature(void);
uint8_t checkRoastTemperatureSafety(void);

// Potentiometer
uint16_t readPotentiometer(void);
uint8_t getTampingLevel(uint16_t adc_value);
const char *getTampingDescription(uint8_t level);

// Light intensity
float readLightIntensity(void);

#endif