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


int sieve(int* period, int steps, unsigned data)
{
    int f = 0;
    for(int i = 0; i < steps; ++i)
    {
        if((data>>i)&1)
        {
            period[f] = i;
            ++f;
        }
    }
    int c = period[0];
    for(int i = 0; i < f; ++i)
    {
        c = period[i];
        if(!c) continue;
        for(int j = 0; j < f; ++j)
        {
            if(i == j) continue;
            if(c == 1)
            {
                if(period[j] == 1) period[j] = 0;
            }
            else if((period[j]%c) == 0) period[j] = 0;
        }
    }
    c = 0;
    for(int i = 0; i < f; i++)
    {
        if(period[i]) 
        {
            period[c] = period[i];
            if(c != i)  period[i] = 0;
            ++c;
        }
    }
    return c;
}
