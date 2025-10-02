#ifndef TEMPERATURE_H
#define TEMPERATURE_H

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

int32_t calculateTemperature(void);
uint8_t checkRoastTemperatureSafety(void);

#endif