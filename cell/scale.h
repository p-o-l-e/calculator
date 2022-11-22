#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "midi.h"

typedef struct
{
    uint8_t  degree[12]; // Semitones above root
    uint8_t  root;       // C == 0
    uint16_t data;       // Bit representation
    uint8_t  width;      // Number of degrees

} scale_t;


void set_scale(scale_t* scale, uint16_t gaps, uint8_t root);
void note_from_degree(scale_t* scale, note* o);
uint16_t shift_to_root(scale_t* scale);

extern const char* chromatic[];