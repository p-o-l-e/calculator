/*
  ShiftRegister74HC595.h - Library for simplified control of 74HC595 shift registers.
  RP2040 Port by Devnol and contributors, developed and maintained since Jul 2021.
  Original Arduino Version Developed and maintained by Timo Denk and contributors, since Nov 2014.
  Additional information is available at https://timodenk.com/blog/shift-register-arduino-library/
  For usage see /example/example.cpp in this repository
  C translation by unmanned.
*/
#pragma once

#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"

#define _74HC595_SIZE 2


typedef struct 
{
    spi_inst_t* _spi_port;
    uint8_t     _latch_pin;
    uint8_t     _digital_values[_74HC595_SIZE];

} ShiftRegister74HC595;

void shift_register_74HC595_init(ShiftRegister74HC595* sr, spi_inst_t *spiPort, uint8_t sdiPin, uint8_t sckPin, uint8_t latchPin);
    
void _74HC595_set_all(ShiftRegister74HC595* sr, const uint8_t * digitalValues);
uint8_t* _74HC595_get_all(ShiftRegister74HC595* sr); 
void _74HC595_set(ShiftRegister74HC595* sr, const uint8_t pin, const uint8_t value);
void _74HC595_set_no_update(ShiftRegister74HC595* sr, const uint8_t pin, uint8_t value);
void _74HC595_update_registers(ShiftRegister74HC595* sr);
void _74HC595_set_all_low(ShiftRegister74HC595* sr);
void _74HC595_set_all_high(ShiftRegister74HC595* sr); 
uint8_t _74HC595_get(ShiftRegister74HC595* sr, const uint8_t pin);

