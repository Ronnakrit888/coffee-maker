#include "setup.h"
#include "state_globals.h"

const uint8_t seven_seg_patterns[10] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09
};

const char *menu_names[MAX_MENUS] = {
    "Espresso", "Americano", "Cappuccino", "Latte", "Mocha", 
    "Macchiato", "Flat White", "Affogato", "Ristretto", "Lungo"
};

const char *temp_types[MAX_TEMP_TYPES] = {
    "Hot", "Cold", "Blended"
};

const char *bean_varieties[MAX_BEAN_TYPES] = {
    "Arabica", "Robusta", "Liberica", "Excelsa", "Geisha", "Bourbon"
};

const char *roast[MAX_ROAST_TYPE] = {
    "Light Roast", "Medium Roast", "Medium Dark Roast", "Dark Roast"
};

const char *tamping_levels[MAX_TAMPING] = {
    "Very Light", "Light", "Medium", "Strong", "Very Strong"
};

const char *state_names[MAX_STATE_NAME] = {
    "Menu Selection", "Temperature Selection", "Coffee Beans Selection", 
    "Tamping Level Selection", "Roast Level Selection", "Shot Quantity Selection", 
    "Final Order Summary", "Brewing"
};

volatile uint8_t counter = 0;
volatile uint8_t current_state = 0;
volatile uint8_t last_state = 99;

char stringOut[128];

volatile uint8_t bean_weights[MAX_BEAN_TYPES] = {5, 100, 100, 100, 100, 100};

// Temperature

volatile uint8_t current_temperature = 0;

// Potentiometer ADC value
volatile uint16_t adc_value = 0;
volatile uint8_t adc_ready = 0;

volatile uint32_t last_tamping_display_time = 0;
volatile uint8_t last_tamping_level = 0xFF; // Initialize to invalid value

// Error state for LED blinking
volatile uint8_t error_state_active = 0;
volatile uint32_t last_led_blink_time = 0;

volatile uint8_t state_selections[MAX_STATES] = {0};

const uint8_t state_max_limits[MAX_STATES] = {
	MAX_MENUS - 1,
	MAX_TEMP_TYPES - 1,
	MAX_BEAN_TYPES - 1,
	MAX_TAMPING - 1,
	MAX_ROAST_TYPE - 1,
    MAX_IS_SAFETY - 1,
	MAX_SHOTS - 1,
	1,
};

uint16_t water_level = 1000;    
uint16_t milk_level = 500;      
uint8_t bean_humidity = 12; 
uint8_t brewing_temp = 93; 