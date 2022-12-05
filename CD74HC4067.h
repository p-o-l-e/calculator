#pragma once
#include "pico/stdlib.h"
#include <stdbool.h>

// GPIO PINS
#define S0 15
#define S1 14
#define S2 13
#define S3 12
#define SG 11

typedef struct 
{
    unsigned a : 1;
    unsigned b : 1;
    unsigned c : 1;
    unsigned d : 1;

} _4067_mode;

const _4067_mode __4067[] = 
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

void _4067_init()
{
    gpio_init(S0); 
    gpio_init(S1);
    gpio_init(S2);
    gpio_init(S3);
    gpio_init(SG);
    gpio_set_dir(S0, GPIO_OUT);
    gpio_set_dir(S1, GPIO_OUT);
    gpio_set_dir(S2, GPIO_OUT);
    gpio_set_dir(S3, GPIO_OUT);
    gpio_set_dir(SG, GPIO_IN);

}

void _4067_switch(int pin)
{  
    gpio_put(S0, __4067[pin].a);
    gpio_put(S1, __4067[pin].b);
    gpio_put(S2, __4067[pin].c);
    gpio_put(S3, __4067[pin].d);
}

bool _4067_get()
{
    return gpio_get(SG);
}
