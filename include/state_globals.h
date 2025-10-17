#ifndef STATE_GLOBALS_H
#define STATE_GLOBALS_H

#include <stdint.h>

extern volatile uint8_t counter;
extern volatile uint8_t current_state;
extern char stringOut[];

extern volatile uint8_t bean_weights[];
extern volatile uint8_t current_temperature;
extern volatile uint16_t adc_value;
extern volatile uint8_t adc_ready;
extern volatile uint8_t state_selections[];

// Tamping display control - for limiting update rate
extern volatile uint32_t last_tamping_display_time;
extern volatile uint8_t last_tamping_level;

extern volatile uint8_t safety_halt_released;

// Error state for LED blinking
extern volatile uint8_t error_state_active;
extern volatile uint32_t last_led_blink_time;

extern uint16_t water_level;
extern uint16_t milk_level;
extern uint8_t bean_humidity;
extern uint8_t brewing_temp; 

#endif

