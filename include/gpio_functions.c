#include "gpio_types.h"
#include "stm32f4xx.h"

void GPIO_Button_Init(GPIO_TypeDef* GPIOx, uint8_t pin_number, GPIO_Pull_Type pull_config) {

    // Clear MODER bits
    GPIOx->MODER &= ~(0b11 << (pin_number * 2));

    // Clear PUPDR bits
    GPIOx->PUPDR &= ~(0b11 << (pin_number * 2));

    // Set pull configuration
    GPIOx->PUPDR |= (pull_config << (pin_number * 2));

}

