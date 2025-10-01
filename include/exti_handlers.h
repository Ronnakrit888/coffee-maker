#ifndef EXTI_HANDLERS_H
#define EXTI_HANDLERS_H

#include <stdint.h>
#include "stm32f4xx.h"

#define MAX_STATES 5

extern volatile uint8_t counter;
extern volatile uint8_t current_state;
extern char stringOut[50];

extern volatile uint8_t state_selections[MAX_STATES];
extern const uint8_t state_max_limits[MAX_STATES];

void EXTI15_10_IRQHandler(void);
void EXTI3_IRQHandler(void);
void EXTI9_5_IRQHandler(void);
void EXTI4_IRQHandler(void);
void display(uint8_t);
void vdg_UART_TxString(char[]);
void showWelcomeMenu(void);

#endif