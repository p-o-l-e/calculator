// Based on code by Rodney DuPlessis
// https://github.com/rodneydup/Xenakis-Sieves

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct
{
   int modulus; // modulus of the period
   int initial; // starting point
   int covered; // number of covered points

}  period_t;

unsigned lcd_euclide(const unsigned x, const unsigned y)  // computation of the LCD
{
    unsigned a = x;
    unsigned b = y;
    unsigned t;
    while ((t = a % b) != 0)
    {
        a = b;
        b = t;
    }
    return b;
}

#define POINTS         16        // number of points in the sieve
#define COVERED       -1L

unsigned sieve_xenakis(period_t* o, unsigned data)
{
    int crible[POINTS];      // points of the crible
    int rest[POINTS];        // points outside the periods
    int points = 0;
    int value;
    int periods = 0;        // number of periods in the formula
    int ptnb;
    period_t per;

    for(int i = 0; i < POINTS; ++i)
    {
        if((data>>i)&1)
        {
            crible[points] = i;
            ++points;
        }
    }
    memcpy(rest, crible, POINTS * sizeof(int));

    for(int i = 0; i < points; ++i)
    {
        if(rest[i] == COVERED) continue;
        // compute a period starting at current point
        per.modulus = 0;
        do
        {
            per.modulus++;
            per.initial  = crible[i] % per.modulus;
            per.covered = 0;
            for(ptnb = 0, value = per.initial; ptnb < points && value >= crible[ptnb]; ptnb++)
            {
                if (value == crible[ptnb])
                {
                    per.covered++;
                    value += per.modulus;
                }
            }
        }
        while (ptnb < points);
        // check the redundancy of the period
        bool redundant = true;
        value = per.initial;
        for (ptnb = 0; ptnb < points; ptnb++)
        {
            if(value == crible[ptnb])
            {
                if (value == rest[ptnb])
                {
                    rest[ptnb] = COVERED;
                    redundant = false;
                }
                value += per.modulus;
            }
        }
        if(!redundant) o[periods++] = per;
    }
    return periods;
}


unsigned compute_period(period_t* o, int n) // compute the period of the sieve
{
    unsigned percrib = o[0].modulus;
    for (int i = 1; i < n; ++i)
    {
        if (o[i].modulus >= percrib) percrib *= o[i].modulus / lcd_euclide(o[i].modulus, percrib);
        else percrib *= o[i].modulus / lcd_euclide(percrib, o[i].modulus);
    }
    return percrib;
}

period_t period[POINTS]; // periods of the sieve


int main()
{      
     //0b1111000011110000
    printf("Periods: %d\n", sieve_xenakis(period, 0b00000101011001));
    for(int i = 0; i < POINTS; i++)
    {
        printf("(%2d, %2d, %2d)\n", period[i].initial, period[i].modulus, period[i].covered);
    }

    return 0;
}