#include <stdint.h>
#include <stdio.h>
#include "stm32f4xx.h"
#include "gpio_types.h"
#include "setup.h"
#include "oled_driver.h"

// Millisecond counter for timekeeping
volatile uint32_t systick_millis = 0;

void delay(uint32_t ms)
{
	uint32_t threshold = ms * 133.333f;
	for (uint32_t iter = 0; iter < threshold; iter++)
		;
}

uint32_t millis(void)
{
	return systick_millis;
}

void setupSysTick(void)
{
	// Configure SysTick to trigger every 1ms
	// Assuming 16MHz system clock (adjust if different)
	SysTick->LOAD = 16000 - 1;  // 16MHz / 1000 = 16000 for 1ms
	SysTick->VAL = 0;
	SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
}

// SysTick interrupt handler
void SysTick_Handler(void)
{
	systick_millis++;
}

void setupFPU(void)
{
	SCB->CPACR |= (0b1111 << 20);
	__asm volatile("dsb");
	__asm volatile("isb");
}

void setUpDisplaySegment(void)
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

void setupButton(void)
{

	GPIO_Button_Init(GPIOA, 10, MY_GPIO_PULL_UP);
	GPIO_Button_Init(GPIOB, 3, MY_GPIO_PULL_UP);
	GPIO_Button_Init(GPIOB, 5, MY_GPIO_PULL_UP);
	GPIO_Button_Init(GPIOB, 4, MY_GPIO_PULL_UP);
	setUpDisplaySegment();
}

void setupClock(void)
{

	RCC->AHB1ENR |= (RCC_AHB1ENR_GPIOAEN + RCC_AHB1ENR_GPIOBEN + RCC_AHB1ENR_GPIOCEN);
	RCC->APB2ENR |= (RCC_APB2ENR_SYSCFGEN + RCC_APB2ENR_ADC1EN);
	RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
}

void setupLED(void)
{

	GPIOA->MODER &= ~(GPIO_MODER_MODER5 | GPIO_MODER_MODER6 | GPIO_MODER_MODER7);
	GPIOB->MODER &= ~(GPIO_MODER_MODER6);

	GPIOA->MODER |= (MY_GPIO_MODE_OUTPUT << GPIO_MODER_MODER5_Pos) | (MY_GPIO_MODE_OUTPUT << GPIO_MODER_MODER6_Pos) | (MY_GPIO_MODE_OUTPUT << GPIO_MODER_MODER7_Pos);
	GPIOB->MODER |= (MY_GPIO_MODE_OUTPUT << GPIO_MODER_MODER6_Pos);
}

void setupAnalog(void)
{

	// Setup Temperature Sensor
	GPIOA->MODER &= ~(GPIO_MODER_MODER0);
	GPIOA->MODER |= (MY_GPIO_MODE_ANALOG << GPIO_MODER_MODER0_Pos);
}

void setupOLED(void)
{
	GPIOC->MODER &= ~(GPIO_MODER_MODER6 | GPIO_MODER_MODER8);
	GPIOC->MODER |= ((MY_GPIO_MODE_OUTPUT << GPIO_MODER_MODER6_Pos) | (MY_GPIO_MODE_OUTPUT << GPIO_MODER_MODER8_Pos));
	GPIOC->OTYPER |= (GPIO_OTYPER_OT6 | GPIO_OTYPER_OT8);
	GPIOC->PUPDR &= ~(GPIO_PUPDR_PUPD6 | GPIO_PUPDR_PUPD8);
	GPIOC->PUPDR |= ((MY_GPIO_PULL_UP << GPIO_PUPDR_PUPD6_Pos) | (MY_GPIO_PULL_UP << GPIO_PUPDR_PUPD8_Pos));

	OLED_SCK_SET();
	OLED_SDA_SET();
}

void setupTemperature(void)
{

	ADC1->CR2 |= ADC_CR2_ADON;
	ADC1->SMPR2 |= ADC_SMPR2_SMP0;
	ADC1->SQR1 &= ~(ADC_SQR1_L);
	ADC1->SQR1 |= (1 << ADC_SQR1_L_Pos);
	ADC1->SQR3 &= ~(ADC_SQR3_SQ1);
	ADC1->SQR3 |= (0 << ADC_SQR3_SQ1_Pos);

	setupFPU();
}

void setupPotentionmeter(void)
{
	GPIOA->MODER &= ~(GPIO_MODER_MODER5);
	GPIOA->MODER |= (MY_GPIO_MODE_OUTPUT << GPIO_MODER_MODER5_Pos);
	GPIOA->OTYPER &= (GPIO_OTYPER_OT5);
	GPIOA->OSPEEDR &= (GPIO_OSPEEDR_OSPEED5);

	GPIOA->MODER &= ~(GPIO_MODER_MODER4);
	GPIOA->MODER |= (MY_GPIO_MODE_ANALOG << GPIO_MODER_MODER4_Pos);

	// ADC Configuration for Potentiometer (Channel 4)
	ADC1->CR2 |= ADC_CR2_ADON;
	ADC1->SMPR2 |= ADC_SMPR2_SMP4;
	ADC1->SQR1 &= ~(ADC_SQR1_L);
	ADC1->SQR3 &= ~(ADC_SQR3_SQ1);
	ADC1->SQR3 |= (4 << ADC_SQR3_SQ1_Pos);

	// Enable ADC interrupt
	ADC1->CR1 |= ADC_CR1_EOCIE;

	// Note: Continuous mode and conversion start will be controlled by tamping state
	// to avoid interfering with other ADC channels (temperature, light sensor)

	NVIC_EnableIRQ(18);
	NVIC_SetPriority(18, 0);
}

void setupLightSensor(void)
{
	// Setup PA1 as analog input for light sensor (ADC1 channel 1)
	GPIOA->MODER &= ~(GPIO_MODER_MODER1);
	GPIOA->MODER |= (MY_GPIO_MODE_ANALOG << GPIO_MODER_MODER1_Pos);

	// ADC configuration for light sensor
	// Note: ADC1 is already enabled by setupPotentionmeter()
	// We just need to configure the channel when reading
}

void setupTrackingSensor(void)
{
	// Setup PC9 as digital input with pull-up for tracking sensor
	GPIOC->MODER &= ~(GPIO_MODER_MODER9);
	GPIOC->PUPDR &= ~(GPIO_PUPDR_PUPD9);
	GPIOC->PUPDR |= (MY_GPIO_PULL_UP << GPIO_PUPDR_PUPD9_Pos);  // Pull-up

	// Note: PA7 (Red LED) and PB6 (Green LED) are already configured by setupLED()
}