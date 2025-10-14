#ifndef CHECK_H
#define CHECK_H

#include <stdint.h>
#include "stm32f4xx.h"
#include "setup.h"

uint8_t checkBeanAvailability(uint8_t bean_idx, uint8_t shots);
void reduceBeanWeight(uint8_t bean_idx, uint8_t shots);
uint8_t checkWaterLevel(void);
uint8_t checkMilkLevel(void);
uint8_t checkBeanHumidity(void);
uint8_t checkBrewingTemperature(void);

#endif