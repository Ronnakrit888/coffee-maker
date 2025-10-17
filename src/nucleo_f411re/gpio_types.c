#include "gpio_types.h"
#include "exti_handlers.h"
#include "stm32f4xx.h"
#include "stdbool.h"

void GPIO_Button_Init(GPIO_TypeDef *GPIOx, uint8_t pin_number, MY_GPIO_Pull_Type pull_config)
{

    // Clear MODER bits
    GPIOx->MODER &= ~(0b11 << (pin_number * 2));

    // Clear PUPDR bits
    GPIOx->PUPDR &= ~(0b11 << (pin_number * 2));

    // Set pull configuration
    GPIOx->PUPDR |= (pull_config << (pin_number * 2));
}

void toggle_LED1(void)
{
    GPIOB->ODR ^= GPIO_ODR_OD6;
}

void toggle_LED2(void)
{
    GPIOA->ODR ^= GPIO_ODR_OD7;
}

void toggle_LED3(void)
{
    GPIOA->ODR ^= GPIO_ODR_OD6;
}

void toggle_LED4(void)
{
    GPIOA->ODR ^= GPIO_ODR_OD5;
}

void onLED1(bool isOn)
{
    isOn ? (GPIOB->ODR |= GPIO_ODR_OD6) : (GPIOB->ODR &= ~GPIO_ODR_OD6);
}

void onLED2(bool isOn)
{
    isOn ? (GPIOA->ODR |= GPIO_ODR_OD7) : (GPIOA->ODR &= ~GPIO_ODR_OD7);
}

void onLED3(bool isOn)
{
    isOn ? (GPIOA->ODR |= GPIO_ODR_OD6) : (GPIOA->ODR &= ~GPIO_ODR_OD6);
}

void onLED4(bool isOn)
{
    isOn ? (GPIOA->ODR |= GPIO_ODR_OD5) : (GPIOA->ODR &= ~GPIO_ODR_OD5);
}

void blinkAllLEDs(void)
{
    toggle_LED1();
    toggle_LED2();
    toggle_LED3();
    toggle_LED4();
}
