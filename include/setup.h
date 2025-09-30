#ifndef SETUP_H
#define SETUP_H

#include <stdint.h>
#include "stm32f4xx.h"
#include "gpio_types.h"

void setupButton(void);
void setupClock(void);
void setupLED(void);
void setUpDisplaySegment(void);

#endif