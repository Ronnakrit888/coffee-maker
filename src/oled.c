#include "oled_driver.h"
#include <string.h>

// Display Buffer definition
uint8_t SSD1306_Buffer[SSD1306_WIDTH * (SSD1306_HEIGHT / 8)];

// Dummy delay function (essential for bit-banging I2C)
// In a real application, replace this with a SysTick or timer delay.
static void I2C_Delay(void)
{
    volatile uint32_t i;
    for (i = 0; i < 100; i++)
        ; // Adjust based on your clock speed
}

// --- Low-level I2C Bit-Banging Functions ---

// Set SDA pin to input (floating) to check for ACK
static void SDA_Input_Mode()
{
    OLED_SDA_PORT->MODER &= ~(3U << (OLED_SDA_PIN * 2)); // Clear mode bits
    // Input mode (00) is set by clearing the bits
}

// Set SDA pin to output (open-drain) for driving the line
static void SDA_Output_Mode()
{
    OLED_SDA_PORT->MODER &= ~(3U << (OLED_SDA_PIN * 2)); // Clear mode bits
    OLED_SDA_PORT->MODER |= (1U << (OLED_SDA_PIN * 2));  // Set to Output mode (01)
}

// Generates I2C Start Condition
static void I2C_Start(void)
{
    SDA_Output_Mode();
    OLED_SDA_SET();
    OLED_SCK_SET();
    I2C_Delay();
    OLED_SDA_CLR(); // SDA goes low while SCK is high
    I2C_Delay();
    OLED_SCK_CLR();
    I2C_Delay();
}

// Generates I2C Stop Condition
static void I2C_Stop(void)
{
    SDA_Output_Mode();
    OLED_SDA_CLR();
    OLED_SCK_SET();
    I2C_Delay();
    OLED_SDA_SET(); // SDA goes high while SCK is high
    I2C_Delay();
}

// Writes a single byte over I2C and checks for ACK
static bool I2C_WriteByte(uint8_t data)
{
    uint8_t i;
    bool ack;

    for (i = 0; i < 8; i++)
    {
        if ((data << i) & 0x80)
        {
            OLED_SDA_SET();
        }
        else
        {
            OLED_SDA_CLR();
        }
        I2C_Delay();
        OLED_SCK_SET(); // Clock high (data read)
        I2C_Delay();
        OLED_SCK_CLR(); // Clock low (data prepared for next bit)
    }

    // Read ACK
    OLED_SDA_SET();   // Release SDA line
    SDA_Input_Mode(); // Set SDA to input to read ACK bit
    I2C_Delay();
    OLED_SCK_SET();
    I2C_Delay();
    ack = OLED_SDA_READ(); // ACK is '0' (FALSE)
    OLED_SCK_CLR();
    SDA_Output_Mode(); // Return SDA to output mode
    I2C_Delay();

    return !ack; // Return true if ACK (0) was received
}

// Writes a command or data to the SSD1306 controller
static void OLED_Write(uint8_t type, uint8_t data)
{
    I2C_Start();

    // 1. Send device address + Write bit
    I2C_WriteByte(SSD1306_I2C_ADDR);

    // 2. Send control byte (0x00 for Command, 0x40 for Data)
    I2C_WriteByte(type);

    // 3. Send data/command
    I2C_WriteByte(data);

    I2C_Stop();
}

// Function to write commands
#define OLED_WriteCommand(command) OLED_Write(0x00, (command))
// Function to write data
#define OLED_WriteData(data) OLED_Write(0x40, (data))

// --- SSD1306 Driver Functions ---

// Initializes the SSD1306 display
void OLED_Init(void)
{
    // Wait for display to power up (optional, but good practice)
    for (volatile int i = 0; i < 100000; i++)
        ;

    // Initial sequence of commands for a 128x64 display
    OLED_WriteCommand(0xAE); // Display OFF
    OLED_WriteCommand(0xD5); // Set Display Clock Divide Ratio / Oscillator Frequency
    OLED_WriteCommand(0x80);
    OLED_WriteCommand(0xA8); // Set Multiplex Ratio (height-1)
    OLED_WriteCommand(0x3F); // For 64px height
    OLED_WriteCommand(0xD3); // Set Display Offset
    OLED_WriteCommand(0x00);
    OLED_WriteCommand(0x40); // Set Start Line
    OLED_WriteCommand(0x8D); // Charge Pump Setting
    OLED_WriteCommand(0x14); // Enable charge pump
    OLED_WriteCommand(0x20); // Set Memory Addressing Mode
    OLED_WriteCommand(0x00); // Horizontal Addressing Mode
    OLED_WriteCommand(0xA1); // Segment Remap (0xA0 or 0xA1)
    OLED_WriteCommand(0xC8); // Com scan direction (0xC0 or 0xC8)
    OLED_WriteCommand(0xDA); // COM Pins hardware configuration
    OLED_WriteCommand(0x12); // Sequential COM pin configuration
    OLED_WriteCommand(0x81); // Contrast Control
    OLED_WriteCommand(0xCF);
    OLED_WriteCommand(0xD9); // Set Pre-charge period
    OLED_WriteCommand(0xF1);
    OLED_WriteCommand(0xDB); // Set VCOMH Deselect level
    OLED_WriteCommand(0x40);
    OLED_WriteCommand(0xA4); // Entire Display ON/OFF (Resume to RAM content)
    OLED_WriteCommand(0xA6); // Set Normal Display (0xA7 for Inverse)
    OLED_WriteCommand(0x2E); // Deactivate Scroll
    OLED_WriteCommand(0xAF); // Display ON

    // Clear the screen buffer
    OLED_Fill(0);
    OLED_UpdateScreen();
}

// Fills the whole buffer with a color (0=Black, 1=White)
void OLED_Fill(uint8_t color)
{
    memset(SSD1306_Buffer, (color == 1) ? 0xFF : 0x00, sizeof(SSD1306_Buffer));
}

// Sends the buffer content to the display RAM
void OLED_UpdateScreen(void)
{
    uint16_t i;
    for (i = 0; i < 8; i++)
    {
        OLED_WriteCommand(0xB0 + i); // Set page start address
        OLED_WriteCommand(0x00);     // Set lower column address
        OLED_WriteCommand(0x10);     // Set higher column address

        I2C_Start();
        I2C_WriteByte(SSD1306_I2C_ADDR); // Device Address + Write
        I2C_WriteByte(0x40);             // Data mode

        // Send 128 bytes of data for the current page
        for (uint8_t j = 0; j < SSD1306_WIDTH; j++)
        {
            I2C_WriteByte(SSD1306_Buffer[i * SSD1306_WIDTH + j]);
        }
        I2C_Stop();
    }
}

// Draws a single pixel at (x,y)
void OLED_DrawPixel(uint8_t x, uint8_t y, bool color)
{
    if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT)
    {
        return; // Coordinates out of bounds
    }

    // SSD1306 uses Page Addressing (8 rows per byte)
    // Page = y / 8
    // Bit = y % 8
    if (color)
    {
        SSD1306_Buffer[x + (y / 8) * SSD1306_WIDTH] |= (1 << (y % 8)); // Set bit (White)
    }
    else
    {
        SSD1306_Buffer[x + (y / 8) * SSD1306_WIDTH] &= ~(1 << (y % 8)); // Clear bit (Black)
    }
}