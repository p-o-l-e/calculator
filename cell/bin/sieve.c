// Based on code by Rodney DuPlessis
// https://github.com/rodneydup/Xenakis-Sieves

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#define COVERED -1

typedef struct
{
   int value; // value of the period
   int shift; // starting point

}  period_t;


unsigned sieve_xen(period_t* o, unsigned data)
{
    int crible[16];  // points of the crible
    int rest[16];    // points outside the periods
    int points = 0;
    int value;
    int periods = 0; // number of periods in the formula
    int f;
    period_t p;

    for(int i = 0; i < 16; ++i)
    {
        if((data>>i)&1)
        {
            crible[points] = i;
            ++points;
        }
    }
    memcpy(rest, crible, 16 * sizeof(int));

    for(int i = 0; i < points; ++i)
    {
        if(rest[i] == COVERED) continue;
        // compute a period starting at current point
        p.value = 0;
        do
        {
            ++p.value;
            p.shift  = crible[i] % p.value;
            for(f = 0, value = p.shift; f < points && value >= crible[f]; f++)
            {
                if (value == crible[f])
                {
                    value += p.value;
                }
            }
        }
        while (f < points);
        // check the redundancy of the period
        bool redundant = true;
        value = p.shift;
        for (f = 0; f < points; ++f)
        {
            if(value == crible[f])
            {
                if (value == rest[f])
                {
                    rest[f] = COVERED;
                    redundant = false;
                }
                value += p.value;
            }
        }
        if(!redundant) o[periods++] = p;
    }
    return periods;
}