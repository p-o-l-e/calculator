#pragma once
#include "pico/stdlib.h"
#include <stdint.h>


typedef struct 
{
    int pin_a;
    int pin_b;
    volatile long value;
    volatile int  prior;

} encoder;

void encoder_read(encoder* o);
void encoder_init(encoder* o, int pin_a, int pin_b); 