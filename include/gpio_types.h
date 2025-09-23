#ifndef GPIO_TYPES_H
#define GPIO_TYPES_H

#include <stdint.h>

// GPIO Pull Configuration enumuration
typedef enum {
    GPIO_PULL_NONE = 0b00,
    GPIO_PULL_UP = 0b01,
    GPIO_PULL_DOWN = 0b10,
    GPIO_PULL_RESERVED = 0b11
} GPIO_Pull_Type;

// GPIO Mode enumuration
typedef enum {
    GPIO_MODE_INPUT = 0b00,
    GPIO_MODE_OUTPUT = 0b01,
    GPIO_MODE_ALTERNATIVE = 0b10,
    GPIO_MODE_ANALOG = 0b11
} GPIO_Mode_Type;

// GPIO Output Type enumuration
typedef enum {
    GPIO_OTYPE_PUSH_PULL  = 0,
    GPIO_OTYPE_OPEN_DRAIN = 1
} GPIO_OutputType_Type;

// GPIO Speed enumeration
typedef enum {
    GPIO_SPEED_LOW    = 0b00,
    GPIO_SPEED_MEDIUM = 0b01,
    GPIO_SPEED_HIGH   = 0b10,
    GPIO_SPEED_VERY_HIGH = 0b11
} GPIO_Speed_Type;

// Button state enumeration
typedef enum {
    BUTTON_RELEASED = 0,
    BUTTON_PRESSED  = 1,
    BUTTON_DEBOUNCE = 2
} Button_State_Type;

void GPIO_Button_Init(GPIO_TypeDef* GPIOx, uint8_t pin_number, GPIO_Pull_Type pull_config);

#endif