#pragma once
#include "ShiftRegister74HC595.h"

uint16_t get_point(uint8_t x, uint8_t y, uint8_t c)
{
    return ((1 << x) << (4*(c+1))) + 0b1111^(1 << y);
}

void set_bits(ShiftRegister74HC595* sr, uint16_t a)
{
    for(int i = 0; i < 16; i++)
    {
        _74HC595_set(sr, i, a&0b1);
        a>>=1; 
    }
}

void pset(ShiftRegister74HC595* sr, uint8_t x, uint8_t y, uint8_t c)
{
    uint16_t data = ((1 << x) << (4*(c+1))) + 0b1111^(1 << y);
    for(int i = 0; i < 16; i++)
    {
        _74HC595_set(sr, i, data&0b1);
        data>>=1; 
    }
}

void pset_rgb(ShiftRegister74HC595* sr, uint8_t x, uint8_t y, uint32_t r, uint32_t g, uint32_t b)
{
    pset(sr, x, y, 0);
    sleep_us(b);
    pset(sr, x, y, 1);
    sleep_us(g);
    pset(sr, x, y, 2);
    sleep_us(r);
}