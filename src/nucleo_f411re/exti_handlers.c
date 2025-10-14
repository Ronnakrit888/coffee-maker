#include <stdint.h>
#include <stdio.h>
#include "stm32f4xx.h"
#include "gpio_types.h"
#include "exti_handlers.h"
#include "temperature.h"
#include "led.h"
#include "setup.h"

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

const char *state_names[7] = {
	"Menu Selection",
	"Temperature Selection",
	"Coffee Beans Selection",
	"Roast Level Selection",
	"Shot Quantity Selection",
	"Final Order Summary",
	"Brewing"};

volatile uint8_t counter = 0;
volatile uint8_t current_state = 0;

char stringOut[100];

volatile uint8_t bean_weights[6] = {5, 5, 5, 5, 5, 5};

// Brewing system resources (hardcoded for testing)
volatile uint16_t water_level = 500; // ml
volatile uint16_t milk_level = 300;	 // ml
volatile uint8_t bean_humidity = 12; // percentage (ideal: 10-15%)
volatile uint8_t brewing_temp = 92;	 // celsius (ideal: 90-96°C)

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
			if (current_state < 6)
			{
				current_state++;
				// Show options for the new state (1-4 only)
				if (current_state >= 0 && current_state <= 4)
				{
					showStateOptions(current_state);
				}
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
				// Show going back message
				vdg_UART_TxString("\r\n========================================\r\n");
				sprintf(stringOut, "Going back from [%s] to [%s]\r\n",
						state_names[current_state], state_names[current_state - 1]);
				vdg_UART_TxString(stringOut);
				vdg_UART_TxString("========================================\r\n");

				current_state--;
				// Show options when going back
				if (current_state >= 0 && current_state <= 4)
				{
					showStateOptions(current_state);
				}
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

void showStateOptions(uint8_t state)
{
	switch (state)
	{
	case 0:
		showWelcomeMenu();
		break;
	case 1:
		vdg_UART_TxString("\r\n========================================\r\n");
		vdg_UART_TxString("    TEMPERATURE SELECTION\r\n");
		vdg_UART_TxString("========================================\r\n");
		for (uint8_t i = 0; i <= 2; i++)
		{
			sprintf(stringOut, "%d: %s\r\n", i, temp_types[i]);
			vdg_UART_TxString(stringOut);
		}
		vdg_UART_TxString("========================================\r\n\r\n");
		break;
	case 2:
		vdg_UART_TxString("\r\n========================================\r\n");
		vdg_UART_TxString("    COFFEE BEANS SELECTION\r\n");
		vdg_UART_TxString("========================================\r\n");
		for (uint8_t i = 0; i <= 5; i++)
		{
			sprintf(stringOut, "%d: %s (%dg available)\r\n", i, bean_varieties[i], bean_weights[i]);
			vdg_UART_TxString(stringOut);
		}
		vdg_UART_TxString("========================================\r\n\r\n");
		break;
	case 3:
		vdg_UART_TxString("\r\n========================================\r\n");
		vdg_UART_TxString("    ROAST LEVEL SELECTION\r\n");
		vdg_UART_TxString("========================================\r\n");
		for (uint8_t i = 0; i <= 3; i++)
		{
			sprintf(stringOut, "%d: %s\r\n", i, roast[i]);
			vdg_UART_TxString(stringOut);
		}
		vdg_UART_TxString("========================================\r\n\r\n");
		break;
	case 4:
		vdg_UART_TxString("\r\n========================================\r\n");
		vdg_UART_TxString("    SHOT QUANTITY SELECTION\r\n");
		vdg_UART_TxString("========================================\r\n");
		for (uint8_t i = 0; i <= 8; i++)
		{
			sprintf(stringOut, "%d: %d shot(s) (%dg beans)\r\n", i, i + 1, 1 + i);
			vdg_UART_TxString(stringOut);
		}
		vdg_UART_TxString("========================================\r\n\r\n");
		break;
	default:
		break;
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

			for (uint8_t iter = 0; iter < 10; iter++)
			{
				alert_LED();
			}
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
		uint8_t required_weight = 1 + (shots - 1);
		sprintf(stringOut, "REQUIRED BEANS: %dg\r\n", required_weight);
		vdg_UART_TxString(stringOut);
		sprintf(stringOut, "AVAILABLE: %dg\r\n", bean_weights[bean_idx]);
		vdg_UART_TxString(stringOut);
		vdg_UART_TxString("\r========================================\r\n");

		// Wait 2 seconds before showing result
		for (volatile uint32_t i = 0; i < 3200000; i++)
			;

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
			vdg_UART_TxString("\r\nReason: Not enough coffee beans to complete this order.\r\n");
			vdg_UART_TxString("Restart in 3 seconds...\r\n");
			vdg_UART_TxString("\r========================================\r\n");

			// Wait 3 seconds
			for (volatile uint32_t i = 0; i < 5000000; i++)
				;

			current_state = 0;
			counter = 0;
			showWelcomeMenu();
			return;
		}
	}
	break;
	case 6:
		brewCoffee();
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

// Check water level (returns 1 if sufficient, 0 if not)
uint8_t checkWaterLevel(void)
{
	uint16_t required_water = 250; // ml per cup
	return (water_level >= required_water) ? 1 : 0;
}

// Check milk level (returns 1 if sufficient, 0 if not)
uint8_t checkMilkLevel(void)
{
	uint16_t required_milk = 150; // ml per cup
	return (milk_level >= required_milk) ? 1 : 0;
}

// Check bean humidity (returns 1 if ideal, 0 if not)
uint8_t checkBeanHumidity(void)
{
	// Ideal humidity: 10-15%
	return (bean_humidity >= 10 && bean_humidity <= 15) ? 1 : 0;
}

// Check brewing temperature (returns 1 if ideal, 0 if not)
uint8_t checkBrewingTemperature(void)
{
	// Ideal temperature: 90-96°C
	return (brewing_temp >= 90 && brewing_temp <= 96) ? 1 : 0;
}

// Calculate caffeine content in mg
uint16_t calculateCaffeine(uint8_t shots)
{
	// Approximately 64mg caffeine per shot
	return (uint16_t)(64 * shots);
}

// Main brewing process
void brewCoffee(void)
{
	uint8_t bean_idx = state_selections[2];
	uint8_t shots = state_selections[4] + 1;

	vdg_UART_TxString("\r\n========================================\r\n");
	vdg_UART_TxString("      BREWING SYSTEM CHECKS\r\n");
	vdg_UART_TxString("========================================\r\n");

	// 1. Check bean availability
	vdg_UART_TxString("1. Checking bean availability... ");
	if (!checkBeanAvailability(bean_idx, shots))
	{
		vdg_UART_TxString("FAILED\r\n");
		vdg_UART_TxString("ERROR: Insufficient beans.\r\n");
		vdg_UART_TxString("Reason: Not enough coffee beans in inventory.\r\n");
		vdg_UART_TxString("Returning to menu in 3 seconds...\r\n");

		// Wait 3 seconds
		for (volatile uint32_t i = 0; i < 5000000; i++)
			;

		current_state = 0;
		counter = 0;
		showWelcomeMenu();
		return;
	}
	vdg_UART_TxString("Weight pass\r\n");

	// 2. Check water level
	sprintf(stringOut, "2. Checking water level (%dml)... ", water_level);
	vdg_UART_TxString(stringOut);
	if (!checkWaterLevel())
	{
		vdg_UART_TxString("FAILED\r\n");
		vdg_UART_TxString("ERROR: Insufficient water.\r\n");
		sprintf(stringOut, "Reason: Water tank has %dml, need at least 250ml.\r\n", water_level);
		vdg_UART_TxString(stringOut);
		vdg_UART_TxString("Returning to menu in 3 seconds...\r\n");

		// Wait 3 seconds
		for (volatile uint32_t i = 0; i < 5000000; i++)
			;

		current_state = 0;
		counter = 0;
		showWelcomeMenu();
		return;
	}
	vdg_UART_TxString("Water pass\r\n");

	// 3. Check milk level
	sprintf(stringOut, "3. Checking milk level (%dml)... ", milk_level);
	vdg_UART_TxString(stringOut);
	if (!checkMilkLevel())
	{
		vdg_UART_TxString("FAILED\r\n");
		vdg_UART_TxString("ERROR: Insufficient milk.\r\n");
		sprintf(stringOut, "Reason: Milk container has %dml, need at least 150ml.\r\n", milk_level);
		vdg_UART_TxString(stringOut);
		vdg_UART_TxString("Returning to menu in 3 seconds...\r\n");

		// Wait 3 seconds
		for (volatile uint32_t i = 0; i < 5000000; i++)
			;

		current_state = 0;
		counter = 0;
		showWelcomeMenu();
		return;
	}
	vdg_UART_TxString("Milk pass\r\n");

	// 4. Check bean humidity
	sprintf(stringOut, "4. Checking bean humidity (%d%%)... ", bean_humidity);
	vdg_UART_TxString(stringOut);
	if (!checkBeanHumidity())
	{
		vdg_UART_TxString("FAILED\r\n");
		vdg_UART_TxString("ERROR: Bean humidity not ideal.\r\n");
		sprintf(stringOut, "Reason: Humidity is %d%%, ideal range is 10-15%%.\r\n", bean_humidity);
		vdg_UART_TxString(stringOut);
		vdg_UART_TxString("Returning to menu in 3 seconds...\r\n");

		// Wait 3 seconds
		for (volatile uint32_t i = 0; i < 5000000; i++)
			;

		current_state = 0;
		counter = 0;
		showWelcomeMenu();
		return;
	}
	vdg_UART_TxString("Humidity pass\r\n");

	// 5. Check brewing temperature
	sprintf(stringOut, "5. Checking brewing temperature (%d°C)... ", brewing_temp);
	vdg_UART_TxString(stringOut);
	if (!checkBrewingTemperature())
	{
		vdg_UART_TxString("FAILED\r\n");
		vdg_UART_TxString("ERROR: Temperature not ideal.\r\n");
		sprintf(stringOut, "Reason: Temperature is %d°C, ideal range is 90-96°C.\r\n", brewing_temp);
		vdg_UART_TxString(stringOut);
		vdg_UART_TxString("Returning to menu in 3 seconds...\r\n");

		// Wait 3 seconds
		for (volatile uint32_t i = 0; i < 5000000; i++)
			;

		current_state = 0;
		counter = 0;
		showWelcomeMenu();
		return;
	}
	vdg_UART_TxString("Temperature pass\r\n");

	vdg_UART_TxString("========================================\r\n");
	vdg_UART_TxString("All checks passed!\r\n");
	vdg_UART_TxString("========================================\r\n\r\n");

	// Brewing process
	vdg_UART_TxString(">> Grinding coffee beans...\r\n");
	for (volatile uint32_t i = 0; i < 1000000; i++)
		; // Delay

	vdg_UART_TxString(">> Brewing coffee...\r\n");
	for (volatile uint32_t i = 0; i < 2000000; i++)
		; // Delay

	vdg_UART_TxString(">> Coffee brewing complete!\r\n\r\n");

	// Calculate caffeine
	uint16_t caffeine = calculateCaffeine(shots);
	sprintf(stringOut, "Caffeine content: %dmg\r\n", caffeine);
	vdg_UART_TxString(stringOut);

	// Reduce resources
	reduceBeanWeight(bean_idx, shots);
	water_level -= 250;
	milk_level -= 150;

	vdg_UART_TxString("\r\n========================================\r\n");
	vdg_UART_TxString("      COMPLETED!\r\n");
	vdg_UART_TxString("========================================\r\n");
	sprintf(stringOut, "%s beans remaining: %dg\r\n", bean_varieties[bean_idx], bean_weights[bean_idx]);
	vdg_UART_TxString(stringOut);
	sprintf(stringOut, "Water remaining: %dml\r\n", water_level);
	vdg_UART_TxString(stringOut);
	sprintf(stringOut, "Milk remaining: %dml\r\n", milk_level);
	vdg_UART_TxString(stringOut);

	vdg_UART_TxString("\r\nReturning to menu in 5 seconds...\r\n");

	// Wait 5 seconds (approximate delay)
	for (volatile uint32_t i = 0; i < 8000000; i++)
		;

	// Reset to menu
	current_state = 0;
	counter = 0;
	showWelcomeMenu();
}
