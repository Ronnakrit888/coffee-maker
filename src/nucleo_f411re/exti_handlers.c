#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include "stm32f4xx.h"
#include "gpio_types.h"
#include "exti_handlers.h"
#include "sensor_stm32.h"
#include "setup.h"
#include "check.h"
#include "state_globals.h"
#include "oled_driver.h"

volatile uint8_t safety_halt_released = 0;

static void handle_confirm(void)
{
	if (checkRoastTemperatureSafety() == 1 && current_state == 5 && safety_halt_released == 0)
	{

		safety_halt_released = 1;
		vdg_UART_TxString("\r\n--- HALT ACKNOWLEDGED: PROCEEDING TO QUANTITY SELECTION (State 5) ---\r\n");

		onLED1(false);
		onLED2(false);
		onLED3(false);
		onLED4(false);

		counter = 0;
		showStateOptions(current_state);
		display(counter);
		return;
	}

	if (current_state == 3) // Tamping state
	{
		// Use current ADC value to get tamping level
		uint8_t tamping_level = getTampingLevel(adc_value); // Use the global ADC value updated by interrupt
		state_selections[current_state] = tamping_level;

		vdg_UART_TxString("\r\n========================================\r\n");
		sprintf(stringOut, "CONFIRMED: %s - %s\r\n", tamping_levels[tamping_level], getTampingDescription(tamping_level));
		vdg_UART_TxString(stringOut);
		vdg_UART_TxString("========================================\r\n");
	}
	else
	{
		state_selections[current_state] = counter;
	}

	if (current_state < 7)
	{
		current_state++;
	}

	counter = 0;

	// Check for safety halt BEFORE showing options
	if (current_state == 5 && checkRoastTemperatureSafety() == 1)
	{
		// Temperature is unsafe - trigger safety halt
		safety_halt_released = 0;
		display(counter);  // This will show the safety halt message
		return;
	}

	if (current_state <= 5)
	{
		showStateOptions(current_state);
	}

	if (current_state != 3)
	{
		display(counter);
	}
}

static void handle_back(void)
{
	if (checkRoastTemperatureSafety() == 1 && current_state == 5)
	{
		if (safety_halt_released == 0)
		{
			safety_halt_released = 1;
			vdg_UART_TxString("\r\n--- HALT ACKNOWLEDGED: Returning to Roast Selection (State 4) ---\r\n");

			// Clear visual alerts
			onLED1(false);
			onLED2(false);
			onLED3(false);
			onLED4(false);

			current_state = 4;
			// Restore selection for state 4
			counter = state_selections[current_state];
			showStateOptions(current_state);
			display(counter);
			return;
		}
	}

	if (current_state > 0)
	{
		// Show going back message
		vdg_UART_TxString("\r\n========================================\r\n");
		sprintf(stringOut, "Going back from [%s] to [%s]\r\n",
				state_names[current_state], state_names[current_state - 1]);
		vdg_UART_TxString(stringOut);
		vdg_UART_TxString("========================================\r\n");

		current_state--;

		// Show options when going back (0-5)
		if (current_state <= 5)
		{
			showStateOptions(current_state);
		}
	}

	// Restore selection for the new state
	counter = state_selections[current_state];
	display(counter);
}

// ADC interrupt handler - read potentiometer value
void ADC_IRQHandler(void)
{
	if ((ADC1->SR & ADC_SR_EOC) != 0)
	{
		adc_value = ADC1->DR;
		adc_ready = 1;

		// Display real-time value when in tamping state
		if (current_state == 3)
		{
			uint8_t tamping_level = getTampingLevel(adc_value);
			const char *tamping_desc = getTampingDescription(tamping_level);

			sprintf(stringOut, "[Tamping] ADC: %d | Level: %s | Taste: %s\r\n",
					adc_value, tamping_levels[tamping_level], tamping_desc);
			vdg_UART_TxString(stringOut);
		}
	}
}

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
			handle_confirm();
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
			handle_back();
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

void send_current_state_via_uart(void)
{

	vdg_UART_TxString("[DATASTART]");
	sprintf(stringOut, "%d,%d,%d", (int)current_state, (int)counter, (int)safety_halt_released);
	vdg_UART_TxString(stringOut);
	vdg_UART_TxString("[DATAEND]\n");
}

void recommendMenuByLight(void)
{
	float light_lux = readLightIntensity();

	vdg_UART_TxString("\r\n========================================\r\n");
	vdg_UART_TxString("    SMART COFFEE RECOMMENDATION\r\n");
	vdg_UART_TxString("========================================\r\n");

	sprintf(stringOut, "Light Intensity: %d Lux\r\n", (uint16_t)light_lux);
	vdg_UART_TxString(stringOut);

	if (light_lux < 50.0f)
	{
		vdg_UART_TxString("Time: Early Morning / Late Night\r\n");
		vdg_UART_TxString("Recommended:\r\n");
		vdg_UART_TxString("  - Espresso (Strong kick start)\r\n");
		vdg_UART_TxString("  - Ristretto (Intense flavor)\r\n");
		vdg_UART_TxString("  - Americano (Classic energy boost)\r\n");
	}
	else if (light_lux < 200.0f)
	{
		vdg_UART_TxString("Time: Morning / Evening\r\n");
		vdg_UART_TxString("Recommended:\r\n");
		vdg_UART_TxString("  - Cappuccino (Balanced & smooth)\r\n");
		vdg_UART_TxString("  - Latte (Creamy comfort)\r\n");
		vdg_UART_TxString("  - Flat White (Rich & velvety)\r\n");
	}
	else if (light_lux < 500.0f)
	{
		vdg_UART_TxString("Time: Mid-Morning / Afternoon\r\n");
		vdg_UART_TxString("Recommended:\r\n");
		vdg_UART_TxString("  - Mocha (Sweet & energizing)\r\n");
		vdg_UART_TxString("  - Macchiato (Light & flavorful)\r\n");
		vdg_UART_TxString("  - Latte (Perfect for relaxing)\r\n");
	}
	else if (light_lux < 1000.0f)
	{
		vdg_UART_TxString("Time: Bright Day\r\n");
		vdg_UART_TxString("Recommended:\r\n");
		vdg_UART_TxString("  - Cold Americano (Refreshing)\r\n");
		vdg_UART_TxString("  - Iced Latte (Cool & smooth)\r\n");
		vdg_UART_TxString("  - Cold Cappuccino (Light & fresh)\r\n");
		vdg_UART_TxString("Tip: Consider 'Cold' temperature option!\r\n");
	}
	else
	{
		vdg_UART_TxString("Time: Very Bright / Sunny Day\r\n");
		vdg_UART_TxString("Recommended:\r\n");
		vdg_UART_TxString("  - Blended Coffee (Icy & refreshing)\r\n");
		vdg_UART_TxString("  - Cold Mocha (Sweet & cool)\r\n");
		vdg_UART_TxString("  - Affogato (Ice cream + espresso)\r\n");
		vdg_UART_TxString("Tip: Select 'Blended' temperature!\r\n");
	}

	vdg_UART_TxString("========================================\r\n");
	vdg_UART_TxString("Loading menu in 3 seconds...\r\n");
	vdg_UART_TxString("========================================\r\n\r\n");

	delay(3000);
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
		vdg_UART_TxString("    TAMPING LEVEL SELECTION\r\n");
		vdg_UART_TxString("========================================\r\n");
		vdg_UART_TxString("Rotate potentiometer to adjust tamping\r\n");
		vdg_UART_TxString("Press PB5 to confirm your selection\r\n");
		vdg_UART_TxString("========================================\r\n");
		vdg_UART_TxString("Tamping Levels:\r\n");
		for (uint8_t i = 0; i < 5; i++)
		{
			sprintf(stringOut, "  %d: %s\r\n", i, tamping_levels[i]);
			vdg_UART_TxString(stringOut);
		}
		vdg_UART_TxString("========================================\r\n\r\n");
		break;
	case 4:
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
	case 5:
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
	{
		// Tamping state - read potentiometer and display real-time
		uint16_t adc_value = readPotentiometer();
		uint8_t tamping_level = getTampingLevel(adc_value);
		const char *tamping_desc = getTampingDescription(tamping_level);

		sprintf(stringOut, "[Tamping] ADC: %d | Level: %s | Taste: %s\r\n",
				adc_value, tamping_levels[tamping_level], tamping_desc);
		break;
	}
	case 4:
		sprintf(stringOut, "[Roast] %s\r\n",
				roast[counter]);
		break;
	case 5:
		if (checkRoastTemperatureSafety() == 1 && safety_halt_released == 0)
		{
			toggle_LED1();
            toggle_LED2();
            toggle_LED3();
            toggle_LED4();

			sprintf(stringOut, "!!! SAFETY HALT !!! Temp too high for %s. Acknowledge to proceed.\r\n", roast[state_selections[4]]);
			vdg_UART_TxString(stringOut);
			vdg_UART_TxString("========================================\r\n");
			vdg_UART_TxString("  PB5  - Confirm/Proceed\r\n");
			vdg_UART_TxString("  PB4  - Back to Roast\r\n");
			vdg_UART_TxString("========================================\r\n\r\n");
			return;
		}
		else
		{
			// SAFE TO PROCEED: Display the current selection for State 5 (Strength/Quantity)
			onLED1(false);
			onLED2(false);
			onLED3(false);
			onLED4(false);
			sprintf(stringOut, "[Strength/Quantity] Shots: %d\r\n",
					(uint16_t)counter + 1);
		}
		break;
	case 6:
	{
		uint8_t menu_idx = state_selections[0];
		uint8_t temp_idx = state_selections[1];
		uint8_t bean_idx = state_selections[2];
		uint8_t tamping_idx = state_selections[3];
		uint8_t roast_idx = state_selections[4];
		uint8_t shots = state_selections[5] + 1;

		vdg_UART_TxString("\r\n========================================\r\n");
		vdg_UART_TxString("         FINAL ORDER SUMMARY\r\n");
		vdg_UART_TxString("========================================\r\n");
		sprintf(stringOut, "DRINK: %s\r\n", menu_names[menu_idx]);
		vdg_UART_TxString(stringOut);
		sprintf(stringOut, "TEMP: %s\r\n", temp_types[temp_idx]);
		vdg_UART_TxString(stringOut);
		sprintf(stringOut, "BEANS: %s\r\n", bean_varieties[bean_idx]);
		vdg_UART_TxString(stringOut);
		sprintf(stringOut, "TAMPING: %s - %s\r\n", tamping_levels[tamping_idx], getTampingDescription(tamping_idx));
		vdg_UART_TxString(stringOut);
		sprintf(stringOut, "ROAST: %s\r\n", roast[roast_idx]);
		vdg_UART_TxString(stringOut);
		sprintf(stringOut, "SHOTS: %d\r\n", shots);
		vdg_UART_TxString(stringOut);
		vdg_UART_TxString("\r\n========================================\r\n");
		uint8_t required_weight = BASE_BEAN_GRAMS + (shots - 1);
		sprintf(stringOut, "REQUIRED BEANS: %dg\r\n", required_weight);
		vdg_UART_TxString(stringOut);
		sprintf(stringOut, "AVAILABLE: %dg\r\n", bean_weights[bean_idx]);
		vdg_UART_TxString(stringOut);
		vdg_UART_TxString("\r========================================\r\n");

		delay(2000);

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

			delay(3000);

			current_state = 0;
			counter = 0;
			showWelcomeMenu();
			return;
		}
	}
	break;
	case 7:
		brewCoffee();
		break;
	default:
		sprintf(stringOut, "System Error: Unknown State\r\n");
		break;
	}

	if (current_state != 6)
	{
		vdg_UART_TxString(stringOut);
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
	uint8_t shots = state_selections[5] + 1;

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

		delay(3000);

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

		delay(3000);

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

		delay(3000);

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

		delay(3000);

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

		delay(3000);

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
	delay(5000);

	vdg_UART_TxString(">> Brewing coffee...\r\n");
	char count_str[2];
	uint8_t max_number = 10;

	for (int8_t count = max_number; count >= 0; count--)
	{
		OLED_Fill(0);

		uint8_t percentage = (max_number - count) * (100 / max_number);

		OLED_DrawProgressBar(0, 56, SSD1306_WIDTH, 8, percentage);

		snprintf(count_str, sizeof(count_str), "%d", count);

		// Calculate X position to approximately center a single 8x8 digit.
		// SSD1306_WIDTH is 128. FONT_WIDTH is 8.
		// Center X = (128 / 2) - (8 / 2) = 64 - 4 = 60
		uint8_t centered_x = 60;

		// Line 2: The actual countdown number (Starts at y=16, which is page-aligned)
		OLED_DrawString(centered_x, 24, count_str, 1);

		OLED_UpdateScreen();

		if (count > 0)
		{
			delay(1000); // Delay for 1 second
		}
	}

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

	delay(5000);

	// Reset to menu
	current_state = 0;
	counter = 0;
	showWelcomeMenu();
}
