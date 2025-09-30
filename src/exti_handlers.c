#include <stdint.h>
#include <stdio.h>
#include "stm32f4xx.h"
#include "gpio_types.h"
#include "exti_handlers.h"

const uint8_t seven_seg_patterns[10] = {
	0x00, // 0: segments pattern (masked to 4 bits)
	0x01, // 1
	0x02, // 2
	0x03, // 3
	0x04, // 4
	0x05, // 5
	0x06, // 6
	0x07, // 7
	0x08, // 8
	0x09  // 9
};

volatile uint8_t counter = 0;
volatile uint8_t current_state = 0;

char stringOut[50];

volatile uint8_t state_selections[MAX_STATES];
const uint8_t state_max_limits[MAX_STATES] = {
	9, 2, 9, 9, 2, 
};

void EXTI15_10_IRQHandler(void)
{
	if ((EXTI->PR & EXTI_PR_PR10) != 0)
	{
		if ((GPIOA->IDR & GPIO_IDR_ID10) == 0)
		{

			uint8_t max_limit = state_max_limits[current_state];

			counter++;
			if (counter > max_limit)
			{
				counter = 0;
			}
			display(counter);

		}

		EXTI->PR |= EXTI_PR_PR10;
	}
}

void EXTI3_IRQHandler(void)
{
	if ((EXTI->PR & EXTI_PR_PR3) != 0)
	{
		if ((GPIOB->IDR & GPIO_IDR_ID3) == 0)
		{

			uint8_t max_limit = state_max_limits[current_state];

			if (counter == 0)
			{
				counter = max_limit;
			}
			else
			{
				counter--;
			}
			display(counter);
		}
		EXTI->PR |= EXTI_PR_PR3;
	}
}

void EXTI9_5_IRQHandler(void)
{
	if ((EXTI->PR & EXTI_PR_PR5) != 0)
	{
		if ((GPIOB->IDR & GPIO_IDR_ID5) == 0)
		{
			
			state_selections[current_state] = counter;
			if (current_state < MAX_STATES - 1) {
				current_state++;
			}
			counter = 0;
			display(counter);

		}
		EXTI->PR |= EXTI_PR_PR5;
	}
}

void EXTI4_IRQHandler(void)
{
	if ((EXTI->PR & EXTI_PR_PR4) != 0)
	{
		if ((GPIOB->IDR & GPIO_IDR_ID4) == 0)
		{
			
			state_selections[current_state] = counter;
			if (current_state != 0) {
				current_state--;
			}

			counter = state_selections[current_state];
			display(counter);
		}
		EXTI->PR |= EXTI_PR_PR4;
	}
}

void vdg_UART_TxString(char strOut[])
{
	for (uint8_t idx = 0; strOut[idx] != '\0'; idx++)
	{
		while ((USART2->SR & USART_SR_TXE) == 0)
			;
		USART2->DR = strOut[idx];
	}
}

void display(uint8_t num)
{

	uint8_t pattern = seven_seg_patterns[num];

	// Debug: Force all pins LOW first
	GPIOC->ODR &= ~GPIO_ODR_OD7;
	GPIOA->ODR &= ~(GPIO_ODR_OD8 | GPIO_ODR_OD9);
	GPIOB->ODR &= ~GPIO_ODR_OD10;

	// Then set according to pattern
	if (pattern & 0x01)
		GPIOC->ODR |= GPIO_ODR_OD7;
	if (pattern & 0x02)
		GPIOA->ODR |= GPIO_ODR_OD8;
	if (pattern & 0x04)
		GPIOB->ODR |= GPIO_ODR_OD10;
	if (pattern & 0x08)
		GPIOA->ODR |= GPIO_ODR_OD9;

	 sprintf(stringOut, "State %d | Counter: %d (Max: %d) \n", 
        (uint16_t)current_state, 
        (uint16_t)counter, 
        (uint16_t)state_max_limits[current_state]);
    vdg_UART_TxString(stringOut);
}
