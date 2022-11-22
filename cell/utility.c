/////////////////////////////////////////////////////////////////////////////////////////
// Utilities
// V.0.3.8 2022-07-22
// MIT License
// Copyright (c) 2022 unmanned
/////////////////////////////////////////////////////////////////////////////////////////
#include "utility.h"

int _gate(gator* o, int in, int width)
{
    if(abs(in - o->eax) > width)
    {
        o->eax = in;
        return o->eax;
    }
    else
    {
        return o->eax;
    }
}

int rand_in_range(int l, int r) 
{ 
    return (rand() % (r - l + 1)) + l;
}

void snh_init(snh* o)
{
    o->t = 0;
    o->value = 0.0f;
}

int snh_process(snh* o, int input, int time)
{
    if (o->t>time)
    {
        o->t = 0;
        o->value = input;
    }
    o->t++;
    return o->value;
}
