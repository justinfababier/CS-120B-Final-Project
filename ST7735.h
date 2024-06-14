#ifndef ST7735_H
#define ST7735_H

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h> // Include delay header
#include "spiAVR.h"
#include "helper.h"

const unsigned int MAX_X = 129; // Screen width
const unsigned int MAX_Y = 129; // Screen height

// Hardware Reset
void HardwareReset() {
    PORTD = SetBit(PORTD, 7, 0);    // setResetPinToLow
    _delay_ms(200);
    PORTD = SetBit(PORTD, 7, 1);    // setResetPinToHigh
    _delay_ms(200);
}

// Invoke CASET
void columnSet(void) {
    Send_Command(0x2A);
    Send_Data(0x00);
    Send_Data(0x00);
    Send_Data(0x00);
    Send_Data(MAX_X);
}

// Invoke RASET
void rowSet(void) {
    Send_Command(0x2B);
    Send_Data(0x00);
    Send_Data(0x00);
    Send_Data(0x00);
    Send_Data(MAX_Y);
}

// Function to set the address window
void setAddrWindow(int x0, int y0, int x1, int y1) {
    Send_Command(0x2A); // CASET (Column Address Set)
    Send_Data(0x00);    // High byte of x0
    Send_Data(x0);      // Low byte of x0
    Send_Data(0x00);    // High byte of x1
    Send_Data(x1);      // Low byte of x1

    Send_Command(0x2B); // RASET (Row Address Set)
    Send_Data(0x00);    // High byte of y0
    Send_Data(y0);      // Low byte of y0
    Send_Data(0x00);    // High byte of y1
    Send_Data(y1);      // Low byte of y1

    Send_Command(0x2C); // RAMWR (Memory Write)
}

// Initialize ST7735
void ST7735_init(void) {
    HardwareReset();
    Send_Command(0x01); // Software reset
    _delay_ms(150);
    Send_Command(0x11); // Sleep out
    _delay_ms(200);
    Send_Command(0x3A); // Set color mode
    Send_Data(0x05);    // 16-bit color
    _delay_ms(10);
    Send_Command(0x29); // Display on
    _delay_ms(200);
}

// Function to send 16-bit color data and clear the screen with that color
// White - 0xFFFF
void Clear_Screen_With_Color(int color) {
    // Set the address window to the full screen
    setAddrWindow(0, 0, MAX_X, MAX_Y);

    // Fill the screen with the specified color
    for (unsigned int i = 0; i < MAX_X * MAX_Y; i++) {
        Send_Data(color >> 8);    // Send high byte
        Send_Data(color & 0xFF);  // Send low byte
    }
}

#endif // ST7735_H
