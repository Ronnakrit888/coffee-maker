#include "stm32f4xx.h"
#include "led.h"
#include "setup.h"

void alert_LED(void)
{
    toggle_LED1();
    toggle_LED2();
    toggle_LED3();
    toggle_LED4();
}