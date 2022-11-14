#pragma once
#define STEPS 12
#include <math.h>
#include "envelope.h"

typedef struct
{
    ar     env;
    int    departed;      // current time in samples
    int    length;        // step duration in samples
    int    current;       // current step
    float* note;
    float  out;
    bool*  on;

} sequencer;

void init_sequence(sequencer* o, int l);

void process_sequence(sequencer* o);

float get_note(sequencer* o);

void genRand(sequencer* o);