#include <stdint.h>
#include <stdio.h>
#include "stm32f4xx.h"
#include "gpio_types.h"
#include "exti_handlers.h"
#include "setup.h"
#include "oled_driver.h"
#include "state_globals.h"
#include "sensor_stm32.h"

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
	setupSysTick();
	setupButton();
	setupAnalog();
	setupTemperature();
	setupPotentionmeter();
	setupLightSensor();
	setupLED();
	setupOLED();
	selectButton();

	// Delay to let UART stabilize
	delay(1000);

	// Read ambient light and recommend menu
	recommendMenuByLight();

	// Show welcome menu
	showWelcomeMenu();
	OLED_Init();

	// Main loop - handle periodic tamping display
	while (1)
	{
		// Check if we're in tamping state and need to update display
		if (current_state == STATE_SELECT_TAMPING)
		{
			uint32_t current_time = millis();

			// Display ONLY every 1 second (not when value changes)
			if (current_time - last_tamping_display_time >= 1000)
			{
				uint8_t tamping_level = getTampingLevel(adc_value);
				const char *tamping_desc = getTampingDescription(tamping_level);

				sprintf(stringOut, "[Tamping] ADC: %d | Level: %s | Taste: %s\r\n",
						adc_value, tamping_levels[tamping_level], tamping_desc);
				vdg_UART_TxString(stringOut);

				// Update display time and level
				last_tamping_display_time = current_time;
				last_tamping_level = tamping_level;
			}
		}

		// Small delay to prevent CPU hogging (optional)
		// Can be removed if you want maximum responsiveness
	}
}
