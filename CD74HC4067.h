#pragma once
#include "pico/stdlib.h"

// GPIO PINS
#define S0 20
#define S1 21
#define S2 22
#define S3 23

typedef struct 
{
    unsigned a : 1;
    unsigned b : 1;
    unsigned c : 1;
    unsigned d : 1;

} mode_4067;

mode_4067 _mode_4067[] = 
{
    {.a = 0, .b = 0, .c = 0, .d = 0}, //  0
    {.a = 1, .b = 0, .c = 0, .d = 0}, //  1
    {.a = 0, .b = 1, .c = 0, .d = 0}, //  2
    {.a = 1, .b = 1, .c = 0, .d = 0}, //  3
    {.a = 0, .b = 0, .c = 1, .d = 0}, //  4
    {.a = 1, .b = 0, .c = 1, .d = 0}, //  5
    {.a = 0, .b = 1, .c = 1, .d = 0}, //  6
    {.a = 1, .b = 1, .c = 1, .d = 0}, //  7
    {.a = 0, .b = 0, .c = 0, .d = 1}, //  8
    {.a = 1, .b = 0, .c = 0, .d = 1}, //  9
    {.a = 0, .b = 1, .c = 0, .d = 1}, //  10
    {.a = 1, .b = 1, .c = 0, .d = 1}, //  11
    {.a = 0, .b = 0, .c = 1, .d = 1}, //  12
    {.a = 1, .b = 0, .c = 1, .d = 1}, //  13
    {.a = 0, .b = 1, .c = 1, .d = 1}, //  14
    {.a = 1, .b = 1, .c = 1, .d = 1}  //  15

};

void Switch4067(int pin, unsigned lag)
{  
    gpio_put(S0, _mode_4067[pin].a);
    gpio_put(S1, _mode_4067[pin].b);
    gpio_put(S2, _mode_4067[pin].c);
    gpio_put(S3, _mode_4067[pin].d);
    sleep_us(lag);
}
