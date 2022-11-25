#pragma once
#include "74HC595.h"

uint16_t get_point(uint8_t x, uint8_t y, uint8_t c)
{
    uint16_t data = 0;
    if (c  < 3) data = ((1 << x) << (4*(c+1))) + 0b1111^(1 << y);
    else if (c == 3) data = ((1 << x) << 12) + ((1 << x) << 8) + 0b1111^(1 << y);
    else if (c == 4) data = ((1 << x) <<  8) + ((1 << x) << 4) + 0b1111^(1 << y); // Teal
    return data;
}

void set_bits(CD74HC595* sr, uint16_t a)
{
    for(int i = 0; i < 16; i++)
    {
        _74HC595_set(sr, i, a&0b1);
        a>>=1; 
    }
}

void pset(CD74HC595* sr, uint8_t x, uint8_t y, uint8_t c)
{   
    uint16_t data = 0;
    if (c  < 3) data = ((1 << x) << (4*(c+1))) + 0b1111^(1 << y);
    else if (c == 3) data = ((1 << x) << 12) + ((1 << x) << 8) + 0b1111^(1 << y);
    else if (c == 4) data = ((1 << x) <<  8) + ((1 << x) << 4) + 0b1111^(1 << y); // Teal
    for(int i = 0; i < 16; i++)
    {
        _74HC595_set(sr, i, data&0b1);
        data>>=1; 
    }
}

void pset_clr(CD74HC595* sr, uint8_t x, uint8_t y, uint8_t c)
{
    
    uint8_t cb = 0;
    for(int i = 12; i < 16; i++)
    {
        cb +=_74HC595_get(sr, i);
        cb <<= 1;
    }
    cb >>= 1;
    uint16_t data = ((1 << x) << (4*(c+1))) + cb^(1 << y);
    for(int i = 0; i < 16; i++)
    {
        _74HC595_set(sr, i, data&0b1);
        data>>=1; 
    }

}

void pset_rgb(CD74HC595* sr, uint8_t x, uint8_t y, uint32_t r, uint32_t g, uint32_t b)
{
    pset(sr, x, y, 0);
    sleep_us(b);
    pset(sr, x, y, 1);
    sleep_us(g);
    pset(sr, x, y, 2);
    sleep_us(r);
}