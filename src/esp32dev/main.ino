#include <Arduino.h>
// Define the pins for the hardware UART used for communication (Serial2)
// NOTE: Adjust these pins based on your specific ESP32 board
#define RXD2 16 // GPIO pin used to receive data from STM32
#define TXD2 17 // GPIO pin used to transmit data (not used in this example)

const long BAUD_RATE = 115200;

void setup() {
    // 1. Initialize the Debug Serial port (Serial0) to see output on the PC monitor
    Serial.begin(BAUD_RATE);
    Serial.println("\n--- ESP32 UART Receiver Initialized ---");
    Serial.print("Listening for data on Serial2 (RX Pin: ");
    Serial.print(RXD2);
    Serial.println(")...");

    // 2. Initialize the Communication Serial port (Serial2)
    // Parameters: Baud Rate, Configuration (8 data bits, no parity, 1 stop bit), RX Pin, TX Pin
    Serial2.begin(BAUD_RATE, SERIAL_8N1, RXD2, TXD2);
}

void loop() {
    // Check if any data is available to read from the STM32 (on Serial2)
    if (Serial2.available() > 0) {
        // Read all available bytes
        String receivedData = Serial2.readString();

        // Echo the received data to the Debug Serial monitor
        Serial.print("Received: ");
        Serial.print(receivedData); // Use print() to keep the formatting (e.g., newlines)
    }

    // A small delay to keep the loop from running too fast
    delay(10);
}