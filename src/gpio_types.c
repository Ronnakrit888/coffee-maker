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

void onLED1(bool isOn)
{
    if (isOn)
    {
        GPIOB->ODR |= GPIO_ODR_OD6;
    }
    else
    {
        GPIOB->ODR &= ~GPIO_ODR_OD6;
    }
}

void onLED2(bool isOn)
{
    if (isOn)
    {
        GPIOA->ODR |= GPIO_ODR_OD7;
    }
    else
    {
        GPIOA->ODR &= ~GPIO_ODR_OD7;
    }
}

void onLED3(bool isOn)
{
    if (isOn)
    {
        GPIOA->ODR |= GPIO_ODR_OD6;
    }
    else
    {
        GPIOA->ODR &= ~GPIO_ODR_OD6;
    }
}

void onLED4(bool isOn)
{
    if (isOn)
    {
        GPIOA->ODR |= GPIO_ODR_OD5;
    }
    else
    {
        GPIOA->ODR &= ~GPIO_ODR_OD5;
    }
}
