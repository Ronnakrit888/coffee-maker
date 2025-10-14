#ifndef SETUP_H
#define SETUP_H

#include <stdint.h>
#include "stm32f4xx.h"
#include "gpio_types.h"

void delay(void);

void setupFPU(void);

void setupButton(void);
void setupClock(void);
void setupLED(void);
void setupAnalog(void);
void setUpDisplaySegment(void);
void setupTemperature(void);
void setupOLED(void);
void setupPotentionmeter(void);
void setupLightSensor(void);

#endif