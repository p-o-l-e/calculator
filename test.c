#include <stdio.h>




int sieve(int* period, int steps, unsigned data)
{
    int f = 0;
    for(int i = 0; i < steps; ++i)
    {
        if((data>>i)&1)
        {
            period[f] = i%12;
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
            if(i==j) continue;
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
            if(c!=i)  period[i] = 0;
            ++c;
        }
    }
    return c;
}

int main()
{
    int ff[16];
                        //0b1111000011110000
    int lmax = 0;
    int n7 = 0;
    for(unsigned j = 0; j < __UINT16_MAX__; j++)
    {
        int l = sieve(ff, 16, j);
        if(l > lmax) lmax = l;
        for(int i = 0; i < l; i++) printf("%2d ", ff[i]);
        printf("\n");
        if(l == 7)
        {
            ++n7;
            printf(" ------------------------------------------------------------------------------------- %d\n", n7);
        }
    }
    printf("\n%d\n%d", lmax, n7);

    return 0;
}