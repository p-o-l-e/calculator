#include <stdio.h>




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
    int r = 0;
    for(int i = 0; i < f; ++i)
    {
        c = period[i];
        if(!c) continue;
        for(int j = i + 1; j < f; ++j)
        {
            if(c == 1)
            {
                if(period[j] == 1) period[j] = 0;
                ++r;
            }
            else if((period[j]%c) == 0)
            {
                period[j] = 0;
                ++r;
            }
        }
    }
    int l = 0;
    for(int i = 0; i < f; i++)
    {
        if(period[i]) 
        {
            period[l] = period[i];
            if(l!=i)  period[i] = 0;
            ++l;
        }
    }
    return l;
}

int main()
{
    int ff[16];
                        //0b1111000011110000
    int l = sieve(ff, 16, 0b0);
    for(int i = 0; i < l; i++) printf("%2d ", ff[i]);

    return 0;
}