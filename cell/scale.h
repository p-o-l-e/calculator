#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "utility.h"
#include "midi.h"

typedef struct
{
    unsigned degree[12]; // Semitones above root
    unsigned root;       // C == 0
    unsigned data;       // Bit representation
    unsigned width;      // Number of degrees

} scale_t;

void set_scale(scale_t* restrict scale);
void transpose_root(scale_t* restrict scale);
void note_from_degree(scale_t* restrict scale, note* restrict o);