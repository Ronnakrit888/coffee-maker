#ifndef EXTI_HANDLERS_H
#define EXTI_HANDLERS_H

#include <stdint.h>
#include "stm32f4xx.h"
#include "setup.h"

void EXTI15_10_IRQHandler(void);
void ADC_IRQHandler(void);
void EXTI3_IRQHandler(void);
void EXTI9_5_IRQHandler(void);
void EXTI4_IRQHandler(void);
void display(uint8_t);
void vdg_UART_TxString(char[]);
void recommendMenuByLight(void);
void showWelcomeMenu(void);
void showStateOptions(uint8_t state);
void displayBeanWeights(void);
uint16_t calculateCaffeine(uint8_t);
void brewCoffee(void);

#endif