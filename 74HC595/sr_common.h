#pragma once
#include "74HC595.h"

uint_fast16_t get_point(uint_fast8_t x, uint_fast8_t y, uint_fast8_t c)
{
    uint_fast16_t data = 0;
    if (c  < 3) data = ((1 << x) << (4*(c+1))) + 0b1111^(1 << y);
    else if (c == 3) data = ((1 << x) << 12) + ((1 << x) << 8) + 0b1111^(1 << y);
    else if (c == 4) data = ((1 << x) <<  8) + ((1 << x) << 4) + 0b1111^(1 << y); // Teal
    return data;
}

void set_bits(CD74HC595* sr, uint_fast16_t a)
{
    for(int i = 0; i < 16; i++)
    {
        _74HC595_set(sr, i, a&0b1);
        a>>=1; 
    }
}

void pset(CD74HC595* sr, uint_fast8_t x, uint_fast8_t y, uint_fast8_t c)
{   
    uint_fast16_t data = 0;
    if (c  < 3) data = ((1 << x) << (4*(c+1))) + 0b1111^(1 << y);
    else if (c == 3) data = ((1 << x) << 12) + ((1 << x) << 8) + 0b1111^(1 << y);
    else if (c == 4) data = ((1 << x) <<  8) + ((1 << x) << 4) + 0b1111^(1 << y); // Teal
    for(int i = 0; i < 16; i++)
    {
        _74HC595_set(sr, i, data&0b1);
        data>>=1; 
    }
}

void pset_clr(CD74HC595* sr, uint_fast8_t x, uint_fast8_t y, uint_fast8_t c)
{
    
    uint_fast8_t cb = 0;
    for(int i = 12; i < 16; i++)
    {
        cb +=_74HC595_get(sr, i);
        cb <<= 1;
    }
    cb >>= 1;
    uint_fast16_t data = ((1 << x) << (4*(c+1))) + cb^(1 << y);
    for(int i = 0; i < 16; i++)
    {
        _74HC595_set(sr, i, data&0b1);
        data>>=1; 
    }

}
