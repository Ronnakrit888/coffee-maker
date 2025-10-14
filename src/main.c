#include <stdint.h>
#include <stdio.h>
#include "stm32f4xx.h"
#include "gpio_types.h"
#include "exti_handlers.h"
#include "setup.h"
#include "temperature.h"
#include "oled_driver.h"

void selectButton(void)
{
	/* Setup GPIO PA2, PA3*/
	GPIOA->MODER &= ~(GPIO_MODER_MODER2 | GPIO_MODER_MODER3);
	GPIOA->MODER |= (MY_GPIO_MODE_ALTERNATIVE << GPIO_MODER_MODER2_Pos) | (MY_GPIO_MODE_ALTERNATIVE << GPIO_MODER_MODER3_Pos);

	GPIOA->AFR[0] &= ~(GPIO_AFRL_AFRL2 | GPIO_AFRL_AFRL3);
	GPIOA->AFR[0] |= (0b0111 << GPIO_AFRL_AFSEL2_Pos) | (0b0111 << GPIO_AFRL_AFSEL3_Pos);

	/* USART2 Setup */
	USART2->CR1 |= USART_CR1_UE;
	USART2->CR1 &= ~USART_CR1_M;
	USART2->CR2 &= ~USART_CR2_STOP;
	USART2->BRR = 139;
	USART2->CR1 |= USART_CR1_TE | USART_CR1_RE;

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
	SYSCFG->EXTICR[1] |= (1 << SYSCFG_EXTICR2_EXTI4_Pos);

	NVIC_EnableIRQ(EXTI15_10_IRQn);
	NVIC_EnableIRQ(EXTI3_IRQn);
	NVIC_EnableIRQ(EXTI9_5_IRQn);
	NVIC_EnableIRQ(EXTI4_IRQn);

	NVIC_SetPriority(EXTI15_10_IRQn, 0);
	NVIC_SetPriority(EXTI3_IRQn, 0);
	NVIC_SetPriority(EXTI9_5_IRQn, 1);
	NVIC_SetPriority(EXTI4_IRQn, 1);
}

int main(void)
{

	setupClock();
	setupButton();
	setupAnalog();
	setupTemperature();
	setupPotentionmeter();
	setupLED();
	setupOLED();
	selectButton();

	// Delay to let UART stabilize
	delay();

	// Show welcome menu
	showWelcomeMenu();
	OLED_Init();

	while (1)
    {
		// Start ADC conversion for potentiometer reading
		ADC1->CR2 |= ADC_CR2_SWSTART;

		// Delay 2 seconds between conversions
		for (volatile uint32_t iter = 0; iter < 3200000; iter++)
			;

        // OLED countdown animation (commented out in original code)
        // char count_str[2];
		// uint8_t max_number = 4;

        // for (int8_t count = max_number; count >= 0; count--)
        // {
        //     OLED_Fill(0);

		// 	uint8_t percentage = (max_number - count) * 25;

		// 	OLED_DrawProgressBar(0, 56, SSD1306_WIDTH, 8, percentage);

        //     // Convert number to string
        //     snprintf(count_str, sizeof(count_str), "%d", count);

        //     // Calculate X position to approximately center a single 8x8 digit.
        //     // SSD1306_WIDTH is 128. FONT_WIDTH is 8.
        //     // Center X = (128 / 2) - (8 / 2) = 64 - 4 = 60
        //     uint8_t centered_x = 60;

        //     // Line 2: The actual countdown number (Starts at y=16, which is page-aligned)
        //     OLED_DrawString(centered_x, 24, count_str, 1);

        //     OLED_UpdateScreen();

        //     // if (count > 0)
        //     // {
        //     //     delay(); // Delay for 1 second
        //     // }
        // }
    }
}