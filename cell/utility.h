#pragma once
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

inline int rand_in_range(int l, int r) 
{ 
    return (rand() % (r - l + 1)) + l;
}

inline unsigned rightrot12(unsigned x, unsigned n)
{
  return ((x >> n) | (x << (12 - n)))&0xFFF;
}

inline unsigned leftrot12(unsigned x, unsigned n)
{
  return ((x << n) | (x >> (12 - n)))&0xFFF;
}