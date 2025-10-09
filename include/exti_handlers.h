#ifndef EXTI_HANDLERS_H
#define EXTI_HANDLERS_H

#include <stdint.h>
#include "stm32f4xx.h"

#define MAX_STATES 6

extern volatile uint8_t counter;
extern volatile uint8_t current_state;
extern char stringOut[100];

extern volatile uint8_t state_selections[MAX_STATES];
extern const uint8_t state_max_limits[MAX_STATES];

void EXTI15_10_IRQHandler(void);
void EXTI3_IRQHandler(void);
void EXTI9_5_IRQHandler(void);
void EXTI4_IRQHandler(void);
void display(uint8_t);
void vdg_UART_TxString(char[]);
void showWelcomeMenu(void);
void showStateOptions(uint8_t state);

// Bean weight management functions
uint8_t checkBeanAvailability(uint8_t bean_idx, uint8_t shots);
void reduceBeanWeight(uint8_t bean_idx, uint8_t shots);
void displayBeanWeights(void);

// Brewing system checks
uint8_t checkWaterLevel(void);
uint8_t checkMilkLevel(void);
uint8_t checkBeanHumidity(void);
uint8_t checkBrewingTemperature(void);
uint16_t calculateCaffeine(uint8_t shots);
void brewCoffee(void);

#endif