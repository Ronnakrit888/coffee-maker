#include "check.h"
#include "state_globals.h"

uint8_t checkBeanAvailability(uint8_t bean_idx, uint8_t shots)
{
	// Base: 5 grams, Extra: 1 gram per additional shot
	uint8_t required_weight = BASE_BEAN_GRAMS + (shots - 1);

	if (bean_weights[bean_idx] >= required_weight)
	{
		return 1; // Available
	}
	return 0; // Not enough
}

void reduceBeanWeight(uint8_t bean_idx, uint8_t shots)
{
	// Base: 5 grams, Extra: 1 gram per additional shot
	uint8_t weight_to_reduce = BASE_BEAN_GRAMS + (shots - 1);

	if (bean_weights[bean_idx] >= weight_to_reduce)
	{
		bean_weights[bean_idx] -= weight_to_reduce;
	}
}

uint8_t checkWaterLevel(void)
{
	uint16_t required_water = 250; // ml per cup
	return (water_level >= required_water) ? 1 : 0;
}

uint8_t checkMilkLevel(void)
{
	uint16_t required_milk = 150; // ml per cup
	return (milk_level >= required_milk) ? 1 : 0;
}

uint8_t checkBeanHumidity(void)
{
	// Ideal humidity: 10-15%
	return (bean_humidity >= 10 && bean_humidity <= 15) ? 1 : 0;
}

uint8_t checkBrewingTemperature(void)
{
	// Ideal temperature: 90-96°C
	return (brewing_temp >= 90 && brewing_temp <= 96) ? 1 : 0;
}