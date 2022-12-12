#include "utility.h"

uint16_t bjorklund(int steps, int pulses) 
{
    uint16_t result = 1;
    for (int i = 1; i < steps; i++) 
    {
        result <<= 1;
        result += (pulses*i)/steps - (pulses*(i-1))/steps;
    }
    return result;
}

