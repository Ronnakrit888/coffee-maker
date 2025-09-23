#include <stdint.h>
#define STM32F411xE
#include <stdio.h>
#include "stm32f4xx.h"

#include "gpio_types.h"

volatile uint8_t counter = 0;
volatile uint32_t button_delay = 0;
char stringOut[50];

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

	sprintf(stringOut, "Current Number = %d \n", (uint16_t)counter);
	vdg_UART_TxString(stringOut);
}

void EXTI15_10_IRQHandler(void)
{
	if ((EXTI->PR & EXTI_PR_PR10) != 0)
	{
		if ((GPIOA->IDR & GPIO_IDR_ID10) == 0)
		{
			counter++;
			if (counter > 9)
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
			if (counter == 0)
			{
				counter = 9;
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
			counter = 0;
			display(counter);
		}
		EXTI->PR |= EXTI_PR_PR5;
	}
}

void EXTI4_IRQHandler(void)
{
	if ((EXTI->PR & EXTI_PR_PR5) != 0)
	{
		if ((GPIOB->IDR & GPIO_IDR_ID5) == 0)
		{
			counter = 0;
			display(counter);
		}
		EXTI->PR |= EXTI_PR_PR5;
	}
}

void setupButton(void)
{

	GPIO_Button_Init(GPIOA, 10, MY_GPIO_PULL_UP);
	GPIO_Button_Init(GPIOB, 3, MY_GPIO_PULL_UP);
	GPIO_Button_Init(GPIOB, 5, MY_GPIO_PULL_UP);
	GPIO_Button_Init(GPIOB, 4, MY_GPIO_PULL_UP);
}
int main(void)
{

	RCC->AHB1ENR |= (RCC_AHB1ENR_GPIOAEN + RCC_AHB1ENR_GPIOBEN + RCC_AHB1ENR_GPIOCEN);
	RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
	RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
}

void displaySegment(void)
{

	GPIOC->MODER &= ~(GPIO_MODER_MODER7);
	GPIOA->MODER &= ~(GPIO_MODER_MODER8) | (GPIO_MODER_MODER9);
	GPIOB->MODER &= ~(GPIO_MODER_MODER10);

	GPIOC->MODER |= (MY_GPIO_MODE_OUTPUT << GPIO_MODER_MODER7_Pos);
	GPIOA->MODER |= (MY_GPIO_MODE_OUTPUT << GPIO_MODER_MODER8_Pos) | (MY_GPIO_MODE_OUTPUT << GPIO_MODER_MODER9_Pos);
	GPIOB->MODER |= (MY_GPIO_MODE_OUTPUT << GPIO_MODER_MODER10_Pos);

	GPIOC->OTYPER &= ~GPIO_OTYPER_OT7;
	GPIOA->OTYPER &= ~(GPIO_OTYPER_OT8 | GPIO_OTYPER_OT9);
	GPIOB->OTYPER &= ~(GPIO_OTYPER_OT10);
}

uint8_t selectMenu(void)
{
	GPIOB->OTYPER &= ~(GPIO_OTYPER_OT10);

	/* Setup GPIO PA2, PA3*/
	GPIOA->MODER &= ~(GPIO_MODER_MODER2 | GPIO_MODER_MODER3);
	GPIOA->MODER |= (0b10 << GPIO_MODER_MODER2_Pos) | (0b10 << GPIO_MODER_MODER3_Pos);

	GPIOA->AFR[0] &= ~(GPIO_AFRL_AFRL2 | GPIO_AFRL_AFRL3);
	GPIOA->AFR[0] |= (0b0111 << GPIO_AFRL_AFSEL2_Pos) | (0b0111 << GPIO_AFRL_AFSEL3_Pos);

	/* USART2 Setup */
	USART2->CR1 |= USART_CR1_UE;
	USART2->CR1 &= ~USART_CR1_M;
	USART2->CR2 &= ~USART_CR2_STOP;
	USART2->BRR = 139;
	USART2->CR1 |= USART_CR1_TE | USART_CR1_RE;

	/* Setup PA4 PA5*/
	GPIOA->MODER &= ~GPIO_MODER_MODER5;
	GPIOA->MODER |= (0b01 << GPIO_MODER_MODER5_Pos);

	GPIOA->MODER &= ~GPIO_MODER_MODER4;
	GPIOA->MODER |= (0b11 << GPIO_MODER_MODER4_Pos);

	EXTI->IMR |= EXTI_IMR_MR10;
	EXTI->RTSR |= (1 << EXTI_RTSR_TR10_Pos);
	EXTI->FTSR |= (1 << EXTI_FTSR_TR10_Pos);
	SYSCFG->EXTICR[2] &= ~(SYSCFG_EXTICR3_EXTI10);
	SYSCFG->EXTICR[2] |= (0 << SYSCFG_EXTICR3_EXTI10_Pos);

	EXTI->IMR |= EXTI_IMR_MR3;
	EXTI->RTSR |= (1 << EXTI_RTSR_TR3_Pos);
	EXTI->FTSR |= (1 << EXTI_FTSR_TR3_Pos);
	SYSCFG->EXTICR[0] &= ~(SYSCFG_EXTICR1_EXTI3);
	SYSCFG->EXTICR[0] |= (1 << SYSCFG_EXTICR1_EXTI3_Pos);

	EXTI->IMR |= EXTI_IMR_MR5;
	EXTI->RTSR |= (1 << EXTI_RTSR_TR5_Pos);
	EXTI->FTSR |= (1 << EXTI_FTSR_TR5_Pos);
	SYSCFG->EXTICR[1] &= ~(SYSCFG_EXTICR2_EXTI5);
	SYSCFG->EXTICR[1] |= (1 << SYSCFG_EXTICR2_EXTI5_Pos);

	EXTI->IMR |= EXTI_IMR_MR4;
	EXTI->RTSR |= (1 << EXTI_RTSR_TR4_Pos);
	EXTI->FTSR |= (1 << EXTI_FTSR_TR4_Pos);
	SYSCFG->EXTICR[0] &= ~(SYSCFG_EXTICR2_EXTI4_Pos);
	SYSCFG->EXTICR[0] |= (1 << SYSCFG_EXTICR2_EXTI4_Pos);
}

int main(void)
{

	RCC->AHB1ENR |= (RCC_AHB1ENR_GPIOAEN + RCC_AHB1ENR_GPIOBEN + RCC_AHB1ENR_GPIOCEN);
	RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

	setupButton();

	NVIC_EnableIRQ(EXTI15_10_IRQn);
	NVIC_EnableIRQ(EXTI3_IRQn);
	NVIC_EnableIRQ(EXTI9_5_IRQn);
	NVIC_EnableIRQ(EXTI4_IRQn);

	NVIC_SetPriority(EXTI15_10_IRQn, 0);
	NVIC_SetPriority(EXTI3_IRQn, 0);
	NVIC_SetPriority(EXTI9_5_IRQn, 1);
	NVIC_SetPriority(EXTI4_IRQn, 1);

	/* ADC */
	SCB->CPACR |= (0b1111 << 20);
	__asm volatile("dsb");
	__asm volatile("isb");

	sprintf(stringOut, "Choose Menu: \n 1. \n 2. \n");
	vdg_UART_TxString(stringOut);

	display(counter);

	while (1)
	{

		for (uint32_t iter = 0; iter < 133333; iter++)
			;
	}
}