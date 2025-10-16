#ifndef SETUP_H
#define SETUP_H

#include <stdint.h>
#include "stm32f4xx.h"
#include "gpio_types.h"

#define MAX_STATES 8
#define MAX_MENUS 10
#define MAX_TEMP_TYPES 3
#define MAX_BEAN_TYPES 6
#define MAX_ROAST_TYPE 4
#define MAX_IS_SAFETY 2
#define MAX_TAMPING 5
#define MAX_SHOTS 9
#define MAX_STATE_NAME 8
#define BASE_BEAN_GRAMS 5

#define STATE_SELECT_MENUS 0
#define STATE_SELECT_TEMP 1
#define STATE_SELECT_BEAN 2
#define STATE_SELECT_TAMPING 3
#define STATE_SELECT_ROAST 4
#define STATE_CHECK_ROAST_TEMP 5
#define STATE_SELECT_SHOTS 6
#define STATE_SUMMARY 7
#define STATE_BREW_COFFEE 8

extern volatile uint8_t state_selections[MAX_STATES];
extern const uint8_t state_max_limits[MAX_STATES];
const uint8_t seven_seg_patterns[10];
const char *menu_names[MAX_MENUS];
const char *temp_types[MAX_TEMP_TYPES];
const char *bean_varieties[MAX_BEAN_TYPES];
const char *roast[MAX_ROAST_TYPE];
const char *safety[MAX_IS_SAFETY];
const char *tamping_levels[MAX_TAMPING];
const char *state_names[MAX_STATE_NAME];

void delay(uint32_t ms);
uint32_t millis(void);
void setupSysTick(void);

void setupFPU(void);

void setupButton(void);
void setupClock(void);
void setupLED(void);
void setupAnalog(void);
void setUpDisplaySegment(void);
void setupTemperature(void);
void setupOLED(void);
void setupPotentionmeter(void);
void setupLightSensor(void);
void setupFlameSensor(void);
void setupTrackingSensor(void);

#endif