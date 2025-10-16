#ifndef SETUP_H
#define SETUP_H

#include <stdint.h>
#include "stm32f4xx.h"
#include "gpio_types.h"

#define MAX_STATES 7
#define MAX_MENUS 10
#define MAX_TEMP_TYPES 3
#define MAX_BEAN_TYPES 6
#define MAX_ROAST_TYPE 4
#define MAX_TAMPING 5
#define MAX_SHOTS 9
#define MAX_STATE_NAME 8
#define BASE_BEAN_GRAMS 5

extern volatile uint8_t state_selections[MAX_STATES];
extern const uint8_t state_max_limits[MAX_STATES];
const uint8_t seven_seg_patterns[10];
const char *menu_names[MAX_MENUS];
const char *temp_types[MAX_TEMP_TYPES];
const char *bean_varieties[MAX_BEAN_TYPES];
const char *roast[MAX_ROAST_TYPE];
const char *tamping_levels[MAX_TAMPING];
const char *state_names[MAX_STATE_NAME];

void delay(uint32_t ms);

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

#endif