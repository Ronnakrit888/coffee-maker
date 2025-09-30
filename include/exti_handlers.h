#ifndef EXTI_HANDLERS_H
#define EXTI_HANDLERS_H

#include <stdint.h>
#include "stm32f4xx.h"

extern volatile uint8_t counter;

void EXTI15_10_IRQHandler(void);
void EXTI3_IRQHandler(void);
void EXTI9_5_IRQHandler(void);
void EXTI4_IRQHandler(void);

#endif