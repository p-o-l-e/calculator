#pragma once
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define MIN(a, b) ((b)>(a)?(a):(b))
#define MAX(a, b) ((a)>(b)?(a):(b))

inline int rand_in_range(int l, int r) 
{ 
    return (rand() % (r - l + 1)) + l;
}

inline unsigned rightrot12(unsigned x, unsigned n)
{
    return ((x >> n) | (x << (12 - n))) & 0xFFF;
}

inline uint16_t rightrot16(uint16_t x, uint16_t n)
{
    return ((x >> n) | (x << (16 - n))) & 0xFFFF;
}

inline unsigned leftrot12(unsigned x, unsigned n)
{
    return ((x << n) | (x >> (12 - n))) & 0xFFF;
}

inline unsigned bin_to_gray(unsigned n)
{
    return (n >> 1) ^ n;
}

inline unsigned gray_to_bin(unsigned n)
{
    unsigned r = n;
    while(n)
    {
        n >>= 1;
        r ^= n;
    }
    return r;
}

inline void swap(int* a, int* b)
{
    int t = *a;
    *a = *b;
    *b = t;
}

inline unsigned lhca(unsigned x, unsigned rule, unsigned mask) // Linear Hybrid Cellular Automaton.
{
    unsigned r = rule & x;
    unsigned t = (x>>1) ^ (x<<1);
    t ^= r;
    t &= mask;
    return  t;
}

uint16_t bjorklund(int steps, int pulses);