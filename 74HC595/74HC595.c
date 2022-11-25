/*
    CD74HC595.h - Library for simplified control of 74HC595 shift registers.
    RP2040 Port by Devnol and contributors, developed and maintained since Jul 2021.
    Original Arduino Version Developed and maintained by Timo Denk and contributors, since Nov 2014.
    Additional information is available at https://timodenk.com/blog/shift-register-arduino-library/
    For usage see /example/example.cpp in this repository
    C translation by unmanned.
*/
#include "74HC595.h"

// CD74HC595 constructor
// _74HC595_SIZE is the number of shiftregisters stacked in serial
void shift_register_74HC595_init(CD74HC595* sr, spi_inst_t *spiPort, uint8_t sdiPin, uint8_t sckPin, uint8_t latchPin)
{
    // set spi pins
    gpio_set_function(sdiPin, GPIO_FUNC_SPI);
    gpio_set_function(sckPin, GPIO_FUNC_SPI);
    // set attributes
    sr->_spi_port  = spiPort;
    sr->_latch_pin = latchPin;

    spi_init(spiPort, 500 * 1000);
    gpio_init(latchPin);
    gpio_set_dir(latchPin, GPIO_OUT);
    gpio_put(latchPin, 0);

    // allocates the specified number of bytes and initializes them to zero
    memset(sr->_digital_values, 0, _74HC595_SIZE * sizeof(uint8_t));
    _74HC595_update_registers(sr);       // reset shift register
}

// Set all pins of the shift registers at once.
// digitalVAlues is a uint8_t array where the length is equal to the number of shift registers.
void _74HC595_set_all(CD74HC595* sr, const uint8_t * digitalValues)
{
    memcpy(sr->_digital_values, digitalValues, _74HC595_SIZE);   // dest, src, size
    _74HC595_update_registers(sr);
}

// Set a specific pin to either HIGH (1) or LOW (0).
// The pin parameter is a positive, zero-based integer, indicating which pin to set.
void _74HC595_set(CD74HC595* sr, const uint8_t pin, const uint8_t value)
{
    _74HC595_set_no_update(sr, pin, value);
    _74HC595_update_registers(sr);
}

// Retrieve all states of the shift registers' output pins.
// The returned array's length is equal to the number of shift registers.
uint8_t* _74HC595_get_all(CD74HC595* sr)
{
    return sr->_digital_values; 
}

// Updates the shift register pins to the stored output values.
// This is the function that actually writes data into the shift registers of the 74HC595.
void _74HC595_update_registers(CD74HC595* sr)
{   
    spi_write_blocking(sr->_spi_port, sr->_digital_values, sizeof(sr->_digital_values));
    gpio_put(sr->_latch_pin, 1); 
    gpio_put(sr->_latch_pin, 0); 
}

// Equivalent to set(int pin, uint8_t value), except the physical shift register is not updated.
// Should be used in combination with _74HC595_update_registers(sr).
void _74HC595_set_no_update(CD74HC595* sr, const uint8_t pin, uint8_t value)
{
    value ? (sr->_digital_values[pin / 8] |= (1 << pin % 8)) : (sr->_digital_values[pin /8] &= ~(1 << pin % 8));
}

// Returns the state of the given pin.
// Either HIGH (1) or LOW (0)
uint8_t _74HC595_get(CD74HC595* sr, const uint8_t pin)
{
    return (sr->_digital_values[pin / 8] >> (pin % 8)) & 1;
}

// Sets all pins of all shift registers to HIGH (1).
void _74HC595_set_all_high(CD74HC595* sr)
{
    for (int i = 0; i < _74HC595_SIZE; i++) {
        sr->_digital_values[i] = 255;
    }
    _74HC595_update_registers(sr);
}

// Sets all pins of all shift registers to LOW (0).
void _74HC595_set_all_low(CD74HC595* sr)
{
    for (int i = 0; i < _74HC595_SIZE; i++) {
        sr->_digital_values[i] = 0;
    }
    _74HC595_update_registers(sr);
}
