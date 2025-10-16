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

void send_summary_state(void)
{

	uint8_t menu_idx = state_selections[STATE_SELECT_MENUS];
	uint8_t temp_idx = state_selections[STATE_SELECT_TEMP];
	uint8_t bean_idx = state_selections[STATE_SELECT_BEAN];
	uint8_t tamping_idx = state_selections[STATE_SELECT_TAMPING];
	uint8_t is_safety_idx = state_selections[STATE_CHECK_ROAST_TEMP];
	uint8_t roast_idx = state_selections[STATE_SELECT_ROAST];
	uint8_t shots = state_selections[STATE_SELECT_SHOTS] + 1;
	vdg_UART_TxString("[SUMMARYSTART]");

	// Updated format string to include spaces as requested in your last query
	sprintf(stringOut, "%d, %d, %d, %d, %d, %d, %d",
			(int)menu_idx, (int)temp_idx, (int)bean_idx,
			(int)tamping_idx, (int)roast_idx, (int)is_safety_idx, (int)shots);

	vdg_UART_TxString(stringOut);
	vdg_UART_TxString("[SUMMARYEND]");
}

static void handle_confirm(void)
{

	// Check if it state Check Temp and
	if (current_state == STATE_CHECK_ROAST_TEMP && safety_halt_released == 0)
	{
		safety_halt_released = 1;
		vdg_UART_TxString("\r\n--- HALT ACKNOWLEDGED: PROCEEDING TO SHOT QUANTITY SELECTION (State 6) ---\r\n");

		onLED1(false);
		onLED2(false);
		onLED3(false);
		onLED4(false);

		current_state++;
		counter = state_selections[current_state];
		showStateOptions(current_state);
		display(counter);
		send_current_state_via_uart();
		return;
	}

	if (current_state == STATE_SELECT_TAMPING) // Tamping state
	{
		// Use current ADC value to get tamping level
		uint8_t tamping_level = getTampingLevel(adc_value); // Use the global ADC value updated by interrupt
		state_selections[current_state] = tamping_level;

		vdg_UART_TxString("\r\n========================================\r\n");
		sprintf(stringOut, "CONFIRMED: %s - %s\r\n", tamping_levels[tamping_level], getTampingDescription(tamping_level));
		vdg_UART_TxString(stringOut);
		vdg_UART_TxString("========================================\r\n");

		// Stop continuous ADC conversion when leaving tamping state
		ADC1->CR2 &= ~ADC_CR2_CONT;
	}
	else if (current_state != STATE_CHECK_ROAST_TEMP)
	{
		state_selections[current_state] = counter;
	}

	if (current_state == STATE_SUMMARY)
	{
		vdg_UART_TxString("\r\n--- ORDER CONFIRMED: STARTING BREWING PROCESS ---\r\n");
		vdg_UART_TxString("[STARTBREW]"); // Send the start brewing command
	}

	if (current_state < MAX_STATES) // 0,1,2,3,4,5,6,7
	{
		current_state++; // 1,2,3,4,5,6,7,8
	}

	const uint8_t prev_state = current_state - 1;

	counter = 0;

	// Check for safety halt BEFORE showing options
	if (current_state == STATE_CHECK_ROAST_TEMP)
	{
		if (checkRoastTemperatureSafety() == 1)
		{
			safety_halt_released = 0;
			display(counter); // This will show the safety halt message
			state_selections[STATE_CHECK_ROAST_TEMP] = 1;
			send_current_state_via_uart();
			return;
		}
		else
		{
			vdg_UART_TxString("\r\n--- TEMP CHECK PASSED: SKIPPING HALT & PROCEEDING TO SHOT QUANTITY (State 6) ---\r\n");
			current_state = STATE_SELECT_SHOTS;
			state_selections[STATE_CHECK_ROAST_TEMP] = 0;
			counter = state_selections[current_state];
		}
	}

	if (prev_state == STATE_SELECT_SHOTS)
	{
		vdg_UART_TxString("\r\n--- SELECTION COMPLETE: GENERATING ORDER SUMMARY ---\r\n");
		send_summary_state();
	}

	if (current_state <= STATE_SELECT_SHOTS)
	{
		showStateOptions(current_state);
	}

	if (current_state != STATE_SELECT_TAMPING)
	{
		display(counter);
		send_current_state_via_uart();
	}
}

static void handle_back(void)
{

	if (current_state == STATE_CHECK_ROAST_TEMP && safety_halt_released == 0 && checkRoastTemperatureSafety() == 1)
	{

		vdg_UART_TxString("\r\n--- HALT BYPASSED: Returning to Roast Selection (State 4) ---\r\n");
		onLED1(false);
		onLED2(false);
		onLED3(false);
		onLED4(false);

		current_state = STATE_SELECT_ROAST;
		safety_halt_released = 1;
		counter = state_selections[current_state];
		showStateOptions(current_state);
		display(counter);
		send_current_state_via_uart();
		return;
	}

	if (current_state > 0)
	{
		// Stop ADC conversion if leaving tamping state
		if (current_state == STATE_SELECT_TAMPING)
		{
			ADC1->CR2 &= ~ADC_CR2_CONT;
		}

		// Show going back message
		vdg_UART_TxString("\r\n========================================\r\n");
		sprintf(stringOut, "Going back from [%s] to [%s]\r\n",
				state_names[current_state], state_names[current_state - 1]);
		vdg_UART_TxString(stringOut);
		vdg_UART_TxString("========================================\r\n");

		current_state--;

		if (current_state == STATE_CHECK_ROAST_TEMP)
		{
			if (checkRoastTemperatureSafety() == 0)
			{
				vdg_UART_TxString("\r\n--- TEMP CHECK PASSED: SKIPPING HALT & RETURNING TO ROAST (State 4) ---\r\n");
				current_state = STATE_SELECT_ROAST;
			}
			else
			{
				safety_halt_released = 0;
			}
		}

		if (current_state <= STATE_SELECT_SHOTS)
		{
			showStateOptions(current_state);
		}
	}

	// Restore selection for the new state
	counter = state_selections[current_state];
	display(counter);
	send_current_state_via_uart();
}

// ADC interrupt handler - read potentiometer value
void ADC_IRQHandler(void)
{
	if ((ADC1->SR & ADC_SR_EOC) != 0)
	{
		// Just read the value - DO NOT print here to avoid blocking
		adc_value = ADC1->DR;
		adc_ready = 1;

		// Printing will be handled by a periodic task in main loop or by button events
		// This keeps the interrupt handler fast and prevents system hang
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
			send_current_state_via_uart();
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
			send_current_state_via_uart();
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

	for (uint8_t i = 0; i < MAX_MENUS; i++)
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
	case STATE_SELECT_MENUS:
		showWelcomeMenu();
		break;
	case STATE_SELECT_TEMP:
		vdg_UART_TxString("\r\n========================================\r\n");
		vdg_UART_TxString("    TEMPERATURE SELECTION\r\n");
		vdg_UART_TxString("========================================\r\n");
		for (uint8_t i = 0; i < MAX_TEMP_TYPES; i++)
		{
			sprintf(stringOut, "%d: %s\r\n", i, temp_types[i]);
			vdg_UART_TxString(stringOut);
		}
		vdg_UART_TxString("========================================\r\n\r\n");
		break;
	case STATE_SELECT_BEAN:
		vdg_UART_TxString("\r\n========================================\r\n");
		vdg_UART_TxString("    COFFEE BEANS SELECTION\r\n");
		vdg_UART_TxString("========================================\r\n");
		for (uint8_t i = 0; i < MAX_BEAN_TYPES; i++)
		{
			sprintf(stringOut, "%d: %s (%dg available)\r\n", i, bean_varieties[i], bean_weights[i]);
			vdg_UART_TxString(stringOut);
		}
		vdg_UART_TxString("========================================\r\n\r\n");
		break;
	case STATE_SELECT_TAMPING:
		vdg_UART_TxString("\r\n========================================\r\n");
		vdg_UART_TxString("    TAMPING LEVEL SELECTION\r\n");
		vdg_UART_TxString("========================================\r\n");
		vdg_UART_TxString("Rotate potentiometer to adjust tamping\r\n");
		vdg_UART_TxString("Press PB5 to confirm your selection\r\n");
		vdg_UART_TxString("Press PB4 to go back\r\n");
		vdg_UART_TxString("========================================\r\n");
		vdg_UART_TxString("Tamping Levels & Descriptions:\r\n");
		for (uint8_t i = 0; i < MAX_TAMPING; i++)
		{
			sprintf(stringOut, "  %d: %s - %s\r\n",
					i, tamping_levels[i], getTampingDescription(i));
			vdg_UART_TxString(stringOut);
		}
		vdg_UART_TxString("========================================\r\n");
		vdg_UART_TxString("Real-time updates shown every 1 second\r\n");
		vdg_UART_TxString("or when level changes...\r\n");
		vdg_UART_TxString("========================================\r\n\r\n");

		// Reset display timer when entering tamping state
		last_tamping_display_time = 0;
		last_tamping_level = 0xFF;

		// Start continuous ADC conversion for potentiometer
		ADC1->CR2 |= ADC_CR2_CONT;
		ADC1->CR2 |= ADC_CR2_SWSTART;
		break;
	case STATE_SELECT_ROAST:
		vdg_UART_TxString("\r\n========================================\r\n");
		vdg_UART_TxString("    ROAST LEVEL SELECTION\r\n");
		vdg_UART_TxString("========================================\r\n");
		for (uint8_t i = 0; i < MAX_ROAST_TYPE; i++)
		{
			sprintf(stringOut, "%d: %s\r\n", i, roast[i]);
			vdg_UART_TxString(stringOut);
		}
		vdg_UART_TxString("========================================\r\n\r\n");
		break;
	case STATE_CHECK_ROAST_TEMP:
		vdg_UART_TxString("\r\n[SYSTEM CHECK] Entering Roast Safety Check (State 5)...\r\n");
		break;
	case STATE_SELECT_SHOTS:
		vdg_UART_TxString("\r\n========================================\r\n");
		vdg_UART_TxString("    SHOT QUANTITY SELECTION\r\n");
		vdg_UART_TxString("========================================\r\n");
		for (uint8_t i = 0; i < MAX_SHOTS; i++)
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
	case STATE_SELECT_MENUS:
		sprintf(stringOut, "[Menu Selection] %d: %s\r\n",
				(uint16_t)counter, menu_names[counter]);
		break;
	case STATE_SELECT_TEMP:
		sprintf(stringOut, "[Temperature] %d: %s\r\n",
				(uint16_t)counter, temp_types[counter]);
		break;
	case STATE_SELECT_BEAN:
		sprintf(stringOut, "[Coffee Beans] %d: %s\r\n",
				(uint16_t)counter, bean_varieties[counter]);
		break;
	case STATE_SELECT_TAMPING:
	{
		// Tamping state - display current value from ADC interrupt
		// The ADC interrupt handler updates the value automatically
		uint8_t tamping_level = getTampingLevel(adc_value);
		const char *tamping_desc = getTampingDescription(tamping_level);

		// Note: Display is rate-limited by ADC_IRQHandler to prevent flooding
		sprintf(stringOut, "[Tamping] Current: %s - %s (ADC: %d)\r\n",
				tamping_levels[tamping_level], tamping_desc, adc_value);
		break;
	}
	case STATE_SELECT_ROAST:
		sprintf(stringOut, "[Roast] %s\r\n",
				roast[counter]);
		break;
	case STATE_CHECK_ROAST_TEMP: // NEW State 5: Roast Safety Check
		if (checkRoastTemperatureSafety() == 1 && safety_halt_released == 0)
		{
			// Safety halt active
			sprintf(stringOut, "!!! SAFETY HALT !!! Temp too high for %s. Acknowledge to proceed.\r\n", roast[state_selections[4]]);
			vdg_UART_TxString(stringOut);
			vdg_UART_TxString("========================================\r\n");
			vdg_UART_TxString("  PB5  - Confirm/Proceed to Shots\r\n");
			vdg_UART_TxString("  PB4  - Back to Roast\r\n");
			vdg_UART_TxString("========================================\r\n\r\n");
			// Turn on visual alerts
			onLED1(true);
			onLED2(true);
			onLED3(true);
			onLED4(true);
			return;
		}
		else
		{
			// Should be handled by skip logic in handle_confirm, but just in case
			sprintf(stringOut, "[Safety Check] PASSED or HALT ACKNOWLEDGED. Proceeding...\r\n");
		}
		break;
	case STATE_SELECT_SHOTS:
		onLED1(false);
		onLED2(false);
		onLED3(false);
		onLED4(false);
		sprintf(stringOut, "[Strength/Quantity] Shots: %d\r\n",
				(uint16_t)counter + 1);
		break;
	case STATE_SUMMARY:
	{
		uint8_t menu_idx = state_selections[STATE_SELECT_MENUS];
		uint8_t temp_idx = state_selections[STATE_SELECT_TEMP];
		uint8_t bean_idx = state_selections[STATE_SELECT_BEAN];
		uint8_t tamping_idx = state_selections[STATE_SELECT_TAMPING];
		uint8_t roast_idx = state_selections[STATE_SELECT_ROAST];
		uint8_t is_safety_idx = state_selections[STATE_CHECK_ROAST_TEMP];
		uint8_t shots = state_selections[STATE_SELECT_SHOTS] + 1;

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
		sprintf(stringOut, "SAFETY: %d\r\n", is_safety_idx);
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
	case STATE_BREW_COFFEE:
		brewCoffee();
		break;
	default:
		sprintf(stringOut, "System Error: Unknown State\r\n");
		break;
	}

	if (current_state != STATE_SUMMARY)
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

	for (uint8_t i = 0; i < MAX_BEAN_TYPES; i++)
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
	uint8_t bean_idx = state_selections[STATE_SELECT_BEAN];
	uint8_t shots = state_selections[STATE_SELECT_SHOTS] + 1;

	vdg_UART_TxString("\r\n========================================\r\n");
	vdg_UART_TxString("      BREWING SYSTEM CHECKS\r\n");
	vdg_UART_TxString("========================================\r\n");

	// 1. Check bean availability
	vdg_UART_TxString("1. Checking bean availability... ");
	if (!checkBeanAvailability(bean_idx, shots))
	{
		vdg_UART_TxString("[CHECKBEANSTART]");
		vdg_UART_TxString("FAILED\r\n");
		vdg_UART_TxString("ERROR: Insufficient beans.\r\n");
		vdg_UART_TxString("Reason: Not enough coffee beans in inventory.\r\n");
		vdg_UART_TxString("Returning to menu in 3 seconds...\r\n");
		vdg_UART_TxString("[CHECKBEANEND]");

		delay(3000);

		current_state = 0;
		counter = 0;
		showWelcomeMenu();
		return;
	}
	vdg_UART_TxString("Weight pass\r\n");
	vdg_UART_TxString("[CHECKBEANSTART_STATUS]PASS[CHECKBEANEND_STATUS]");

	// 2. Check water level
	sprintf(stringOut, "2. Checking water level (%dml)... ", water_level);
	vdg_UART_TxString(stringOut);
	if (!checkWaterLevel())
	{
		vdg_UART_TxString("[CHECKWATERSTART]");
		vdg_UART_TxString("FAILED\r\n");
		vdg_UART_TxString("ERROR: Insufficient water.\r\n");
		sprintf(stringOut, "Reason: Water tank has %dml, need at least 250ml.\r\n", water_level);
		vdg_UART_TxString(stringOut);
		vdg_UART_TxString("Returning to menu in 3 seconds...\r\n");
		vdg_UART_TxString("[CHECKWATEREND]");

		delay(3000);

		current_state = 0;
		counter = 0;
		showWelcomeMenu();
		return;
	}
	vdg_UART_TxString("Water pass\r\n");
	vdg_UART_TxString("[CHECKWATERSTART_STATUS]PASS[CHECKWATEREND_STATUS]");

	// 3. Check milk level
	sprintf(stringOut, "3. Checking milk level (%dml)... ", milk_level);
	vdg_UART_TxString(stringOut);
	if (!checkMilkLevel())
	{
		vdg_UART_TxString("[CHECKMILKSTART]");
		vdg_UART_TxString("FAILED\r\n");
		vdg_UART_TxString("ERROR: Insufficient milk.\r\n");
		sprintf(stringOut, "Reason: Milk container has %dml, need at least 150ml.\r\n", milk_level);
		vdg_UART_TxString(stringOut);
		vdg_UART_TxString("Returning to menu in 3 seconds...\r\n");
		vdg_UART_TxString("[CHECKMILKEND]");

		delay(3000);

		current_state = 0;
		counter = 0;
		showWelcomeMenu();
		return;
	}
	vdg_UART_TxString("Milk pass\r\n");
	vdg_UART_TxString("[CHECKMILKSTART_STATUS]PASS[CHECKMILKEND_STATUS]");

	// 4. Check bean humidity
	sprintf(stringOut, "4. Checking bean humidity (%d%%)... ", bean_humidity);
	vdg_UART_TxString(stringOut);
	if (!checkBeanHumidity())
	{
		vdg_UART_TxString("[CHECKBEANHUDIMITYSTART]");
		vdg_UART_TxString("FAILED\r\n");
		vdg_UART_TxString("ERROR: Bean humidity not ideal.\r\n");
		sprintf(stringOut, "Reason: Humidity is %d%%, ideal range is 10-15%%.\r\n", bean_humidity);
		vdg_UART_TxString(stringOut);
		vdg_UART_TxString("Returning to menu in 3 seconds...\r\n");
		vdg_UART_TxString("[CHECKBEANHUDIMITYEND]");

		delay(3000);

		current_state = 0;
		counter = 0;
		showWelcomeMenu();
		return;
	}
	vdg_UART_TxString("Humidity pass\r\n");
	vdg_UART_TxString("[CHECKBEANHUMIDITYSTART_STATUS]PASS[CHECKBEANHUMIDITYEND_STATUS]");

	// 5. Check brewing temperature
	sprintf(stringOut, "5. Checking brewing temperature (%d°C)... ", brewing_temp);
	vdg_UART_TxString(stringOut);
	if (!checkBrewingTemperature())
	{
		vdg_UART_TxString("[CHECKBREWTEMPSTART]");
		vdg_UART_TxString("FAILED\r\n");
		vdg_UART_TxString("ERROR: Temperature not ideal.\r\n");
		sprintf(stringOut, "Reason: Temperature is %d°C, ideal range is 90-96°C.\r\n", brewing_temp);
		vdg_UART_TxString(stringOut);
		vdg_UART_TxString("Returning to menu in 3 seconds...\r\n");
		vdg_UART_TxString("[CHECKBREWTEMPEND]");

		delay(3000);

		current_state = 0;
		counter = 0;
		showWelcomeMenu();
		return;
	}
	vdg_UART_TxString("Temperature pass\r\n");
	vdg_UART_TxString("[CHECKBREWTEMPSTART_STATUS]PASS[CHECKBREWTEMPEND_STATUS]");

	vdg_UART_TxString("========================================\r\n");
	vdg_UART_TxString("All checks passed!\r\n");
	vdg_UART_TxString("========================================\r\n\r\n");

	// Brewing process
	vdg_UART_TxString(">> Grinding coffee beans...\r\n");
	delay(1000); // Shorter delay for grinding

	vdg_UART_TxString(">> Brewing coffee...\r\n");
	// char count_str[5];
	uint8_t max_number = 10;

	for (int8_t count = max_number; count >= 0; count--)
	{
		// Calculate percentage: 0, 10, 20... 100
		uint8_t percentage = (max_number - count) * (100 / max_number);

		// Send the percentage via UART with new tags
		sprintf(stringOut, "[BREWINGPROGRESSSTART]%d[BREWINGPROGRESSEND]", percentage);
		vdg_UART_TxString(stringOut);

		// Simulate brewing time
		delay(50000);
	}

	vdg_UART_TxString(">> Coffee brewing complete!\r\n\r\n");
	// Ensure 100% is sent one last time
	sprintf(stringOut, "[BREWINGPROGRESSSTART]%d[BREWINGPROGRESSEND]", 100);
	vdg_UART_TxString(stringOut);

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

	// New tag to indicate process completion
	vdg_UART_TxString("[BREW_COMPLETE]");

	vdg_UART_TxString("\r\nReturning to menu in 5 seconds...\r\n");

	delay(5000);

	// Reset to menu
	current_state = 0;
	counter = 0;
	showWelcomeMenu();
}
