#ifndef OLED_DRIVER_H
#define OLED_DRIVER_H

#include "stm32f4xx.h"
#include <stdbool.h>

// --- Pin Definitions (STM32F4xx Register Access) ---
// SCL (SCK) -> PC8
#define OLED_SCK_PORT GPIOC
#define OLED_SCK_PIN 8
#define OLED_SCK_SET() (OLED_SCK_PORT->BSRR = (1U << OLED_SCK_PIN))
#define OLED_SCK_CLR() (OLED_SCK_PORT->BSRR = (1U << (OLED_SCK_PIN + 16)))

// SDA (Data) -> PC6
#define OLED_SDA_PORT GPIOC
#define OLED_SDA_PIN 6
#define OLED_SDA_SET() (OLED_SDA_PORT->BSRR = (1U << OLED_SDA_PIN))
#define OLED_SDA_CLR() (OLED_SDA_PORT->BSRR = (1U << (OLED_SDA_PIN + 16)))
#define OLED_SDA_READ() ((OLED_SDA_PORT->IDR & (1U << OLED_SDA_PIN)) != 0)

// SSD1306 Definitions
#define SSD1306_I2C_ADDR (0x3C << 1) // 7-bit address 0x3C shifted left for R/W bit
#define SSD1306_WIDTH 128
#define SSD1306_HEIGHT 64

#define FONT_8X8_WIDTH    8
#define FONT_8X8_HEIGHT   8
#define FONT_PAGES        1 // 8 pixels tall = 1 page
#define CHAR_BYTES        8 // 8 columns * 1 page


// Note: FONT_WIDTH and FONT_HEIGHT definitions were removed
// as the 8x8 dimensions are handled internally by the C file.

// Display Buffer (128 columns * 8 pages = 1024 bytes)
extern uint8_t SSD1306_Buffer[SSD1306_WIDTH * (SSD1306_HEIGHT / 8)];

// Driver Functions
void OLED_Init(void);
void OLED_Fill(uint8_t color);
void OLED_UpdateScreen(void);
void OLED_DrawPixel(uint8_t x, uint8_t y, bool color);
void OLED_DrawChar(uint8_t x, uint8_t y, char ch, bool color);
void OLED_DrawString(uint8_t x, uint8_t y, const char* str, bool color);
void OLED_DrawProgressBar(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t percentage);

#endif // OLED_DRIVER_H