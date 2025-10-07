#include <stdint.h>
#include <stdio.h>
#include "stm32f4xx.h"
#include "gpio_types.h"
#include "exti_handlers.h"
#include "temperature.h"

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

const char *menu_names[10] = {
	"Espresso",
	"Americano",
	"Cappuccino",
	"Latte",
	"Mocha",
	"Macchiato",
	"Flat White",
	"Affogato",
	"Ristretto",
	"Lungo"};

const char *temp_types[3] = {
	"Hot",
	"Cold",
	"Blended"};

const char *bean_varieties[6] = {
	"Arabica",
	"Robusta",
	"Liberica",
	"Excelsa",
	"Geisha",
	"Bourbon"};

const char *roast[4] = {
	"Light Roast",
	"Medium Roast",
	"Medium Dark Roast",
	"Dark Roast"};

volatile uint8_t counter = 0;
volatile uint8_t current_state = 0;

char stringOut[100];

volatile uint8_t bean_weights[6] = {5, 5, 5, 5, 5, 5};

volatile uint8_t state_selections[MAX_STATES];
const uint8_t state_max_limits[MAX_STATES] = {
	9,
	2,
	5,
	3,
	8,
	1,
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
			if (current_state < MAX_STATES - 1)
			{
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
			if (current_state != 0)
			{
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

void showWelcomeMenu(void)
{
	vdg_UART_TxString("\r\n========================================\r\n");
	vdg_UART_TxString("    COFFEE MAKER - MENU SELECTION\r\n");
	vdg_UART_TxString("========================================\r\n");

	for (uint8_t i = 0; i < 10; i++)
	{
		sprintf(stringOut, "%d: %s\r\n", i, menu_names[i]);
		vdg_UART_TxString(stringOut);
	}

	vdg_UART_TxString("========================================\r\n");
	vdg_UART_TxString("Controls:\r\n");
	vdg_UART_TxString("  PA10 - Next option\r\n");
	vdg_UART_TxString("  PB3  - Previous option\r\n");
	vdg_UART_TxString("  PB5  - Confirm\r\n");
	vdg_UART_TxString("  PB4  - Back\r\n");
	vdg_UART_TxString("========================================\r\n\r\n");

	display(counter);
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

	// Display message based on current state
	switch (current_state)
	{
	case 0:
		sprintf(stringOut, "[Menu Selection] %d: %s\r\n",
				(uint16_t)counter, menu_names[counter]);
		break;
	case 1:
		sprintf(stringOut, "[Temperature] %d: %s\r\n",
				(uint16_t)counter, temp_types[counter]);
		break;
	case 2:
		sprintf(stringOut, "[Coffee Beans] %d: %s\r\n",
				(uint16_t)counter, bean_varieties[counter]);
		break;
	case 3:
		sprintf(stringOut, "[Roast] %s\r\n",
				roast[counter]);
		break;
	case 4:
		if (checkRoastTemperatureSafety() == 1)
		{
			sprintf(stringOut, "!!! SAFETY HALT !!! Temp too high for %s. Back to Menu\r\n", roast[state_selections[3]]);
			vdg_UART_TxString(stringOut);
			current_state = 0;
			counter = 0;
			display(counter);
			return;
		}
		else
		{
			// SAFE TO PROCEED: Display the current selection for State 4 (Strength/Quantity)
			sprintf(stringOut, "[Strength/Quantity] Shots: %d\r\n",
					(uint16_t)counter + 1);
		}
		break;
	case 5:
	{
		uint8_t menu_idx = state_selections[0];
		uint8_t temp_idx = state_selections[1];
		uint8_t bean_idx = state_selections[2];
		uint8_t roast_idx = state_selections[3];
		uint8_t shots = state_selections[4] + 1;

		vdg_UART_TxString("\r\n========================================\r\n");
		vdg_UART_TxString("         FINAL ORDER SUMMARY\r\n");
		vdg_UART_TxString("========================================\r\n");
		sprintf(stringOut, "DRINK: %s\r\n", menu_names[menu_idx]);
		vdg_UART_TxString(stringOut);
		sprintf(stringOut, "TEMP: %s\r\n", temp_types[temp_idx]);
		vdg_UART_TxString(stringOut);
		sprintf(stringOut, "BEANS: %s\r\n", bean_varieties[bean_idx]);
		vdg_UART_TxString(stringOut);
		sprintf(stringOut, "ROAST: %s\r\n", roast[roast_idx]);
		vdg_UART_TxString(stringOut);
		sprintf(stringOut, "SHOTS: %d\r\n", shots);
		vdg_UART_TxString(stringOut);
		vdg_UART_TxString("\r\n========================================\r\n");
		uint8_t required_weight = 5 + (shots - 1);
		sprintf(stringOut, "REQUIRED BEANS: %dg\r\n", required_weight);
		vdg_UART_TxString(stringOut);
		sprintf(stringOut, "AVAILABLE: %dg\r\n", bean_weights[bean_idx]);
		vdg_UART_TxString(stringOut);

		if (checkBeanAvailability(bean_idx, shots))
		{
			sprintf(stringOut, "Ready to Brew. Press CONFIRM (PB5).\r\n");
			vdg_UART_TxString(stringOut);
		}
		else
		{
			sprintf(stringOut, "!!! INSUFFICIENT BEANS !!! Need %dg, Have %dg\r\n",
					required_weight, bean_weights[bean_idx]);
			vdg_UART_TxString(stringOut);
			vdg_UART_TxString("Returning to menu...\r\n");
			vdg_UART_TxString("\r========================================\r\n");
			current_state = 0;
			counter = 0;
			display(counter);
			return;
		}
	}
	break;
	case 6:
	{
		uint8_t bean_idx = state_selections[2];
		uint8_t shots = state_selections[4] + 1;

		// Check again before brewing
		if (checkBeanAvailability(bean_idx, shots))
		{
			// Reduce bean weight
			reduceBeanWeight(bean_idx, shots);

			vdg_UART_TxString("\r\n========================================\r\n");
			vdg_UART_TxString("        BREWING COFFEE...\r\n");
			vdg_UART_TxString("========================================\r\n");
			sprintf(stringOut, "Coffee brewed successfully!\r\n");
			vdg_UART_TxString(stringOut);
			sprintf(stringOut, "%s beans remaining: %dg\r\n",
					bean_varieties[bean_idx], bean_weights[bean_idx]);
			vdg_UART_TxString(stringOut);

			// Display inventory
			displayBeanWeights();

			// Reset to initial state
			current_state = 0;
			counter = 0;
			display(counter);
		}
		else
		{
			vdg_UART_TxString("ERROR: Not enough beans. Returning to menu.\r\n");
			current_state = 0;
			counter = 0;
			display(counter);
		}
	}
	break;
	default:
		sprintf(stringOut, "System Error: Unknown State\r\n");
		break;
	}

	if (current_state != 5)
	{
		vdg_UART_TxString(stringOut);
	}
}

// Check if enough beans are available for the order
// Returns 1 if available, 0 if not enough
uint8_t checkBeanAvailability(uint8_t bean_idx, uint8_t shots)
{
	// Base: 5 grams, Extra: 1 gram per additional shot
	uint8_t required_weight = 5 + (shots - 1);

	if (bean_weights[bean_idx] >= required_weight)
	{
		return 1; // Available
	}
	return 0; // Not enough
}

// Reduce bean weight after making coffee
void reduceBeanWeight(uint8_t bean_idx, uint8_t shots)
{
	// Base: 5 grams, Extra: 1 gram per additional shot
	uint8_t weight_to_reduce = 5 + (shots - 1);

	if (bean_weights[bean_idx] >= weight_to_reduce)
	{
		bean_weights[bean_idx] -= weight_to_reduce;
	}
}

// Display current bean weights
void displayBeanWeights(void)
{
	vdg_UART_TxString("\r\n========================================\r\n");
	vdg_UART_TxString("      BEAN INVENTORY (grams)\r\n");
	vdg_UART_TxString("========================================\r\n");

	for (uint8_t i = 0; i < 6; i++)
	{
		sprintf(stringOut, "%s: %dg\r\n", bean_varieties[i], bean_weights[i]);
		vdg_UART_TxString(stringOut);
	}

	vdg_UART_TxString("========================================\r\n");
}
