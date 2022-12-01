#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "midi.h"

typedef struct
{
    uint_fast8_t  degree[12]; // Semitones above root
    uint_fast8_t  root;       // C == 0
    uint_fast16_t data;       // Bit representation
    uint_fast8_t  width;      // Number of degrees

} scale_t;


void set_scale(scale_t* scale);
void note_from_degree(scale_t* scale, note* o);

extern const char* chromatic[];
extern const char* chromatic_lr[];