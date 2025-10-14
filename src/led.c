#include "stm32f4xx.h"
#include "led.h"
#include "setup.h"

void alert_LED(void)
{
    
    GPIOB->ODR ^= GPIO_ODR_OD6;

}