#ifndef OLED_DRIVER_H
#define OLED_DRIVER_H

#include "stm32f4xx.h"
#include <stdbool.h>

// --- Pin Definitions (STM32F4xx Register Access) ---
// SCL (SCK) -> PC8
#define OLED_SCK_PORT           GPIOC
#define OLED_SCK_PIN            8
#define OLED_SCK_SET()          (OLED_SCK_PORT->BSRR = (1U << OLED_SCK_PIN))
#define OLED_SCK_CLR()          (OLED_SCK_PORT->BSRR = (1U << (OLED_SCK_PIN + 16)))

// SDA (Data) -> PC6
#define OLED_SDA_PORT           GPIOC
#define OLED_SDA_PIN            6
#define OLED_SDA_SET()          (OLED_SDA_PORT->BSRR = (1U << OLED_SDA_PIN))
#define OLED_SDA_CLR()          (OLED_SDA_PORT->BSRR = (1U << (OLED_SDA_PIN + 16))) 
#define OLED_SDA_READ()         ((OLED_SDA_PORT->IDR & (1U << OLED_SDA_PIN)) != 0)

// SSD1306 Definitions
#define SSD1306_I2C_ADDR        (0x3C << 1) // 7-bit address 0x3C shifted left for R/W bit
#define SSD1306_WIDTH           128
#define SSD1306_HEIGHT          64

// Display Buffer (128 columns * 8 pages = 1024 bytes)
extern uint8_t SSD1306_Buffer[SSD1306_WIDTH * (SSD1306_HEIGHT / 8)];

// Driver Functions
void OLED_Init(void);
void OLED_Fill(uint8_t color);
void OLED_UpdateScreen(void);
void OLED_DrawPixel(uint8_t x, uint8_t y, bool color);

#endif // OLED_DRIVER_H
