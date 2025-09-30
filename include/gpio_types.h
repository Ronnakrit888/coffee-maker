#ifndef GPIO_TYPES_H
#define GPIO_TYPES_H

#include "stm32f4xx.h"

#include <stdint.h>

// GPIO Pull Configuration enumuration
typedef enum {
    MY_GPIO_PULL_NONE = 0b00,
    MY_GPIO_PULL_UP = 0b01,
    MY_GPIO_PULL_DOWN = 0b10,
    MY_GPIO_PULL_RESERVED = 0b11
} MY_GPIO_Pull_Type;

// GPIO Mode enumuration
typedef enum {
    MY_GPIO_MODE_INPUT = 0b00,
    MY_GPIO_MODE_OUTPUT = 0b01,
    MY_GPIO_MODE_ALTERNATIVE = 0b10,
    MY_GPIO_MODE_ANALOG = 0b11
} MY_GPIO_Mode_Type;

// GPIO Output Type enumuration
typedef enum {
    MY_GPIO_OTYPE_PUSH_PULL  = 0,
    MY_GPIO_OTYPE_OPEN_DRAIN = 1
} MY_GPIO_OutputType_Type;

// GPIO Speed enumeration
typedef enum {
    MY_GPIO_SPEED_LOW    = 0b00,
    MY_GPIO_SPEED_MEDIUM = 0b01,
    MY_GPIO_SPEED_HIGH   = 0b10,
    MY_GPIO_SPEED_VERY_HIGH = 0b11
} MY_GPIO_Speed_Type;

// Button state enumeration
typedef enum {
    MY_BUTTON_RELEASED = 0,
    MY_BUTTON_PRESSED  = 1,
    MY_BUTTON_DEBOUNCE = 2
} MY_Button_State_Type;

void GPIO_Button_Init(GPIO_TypeDef* GPIOx, uint8_t pin_number, MY_GPIO_Pull_Type pull_config);

#endif