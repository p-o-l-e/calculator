#pragma once
#include <stdbool.h>
#include "utility.h"
#include "midi.h"

#define _tracks_ 16  // Number of tracks
#define _steps_  16 // Number of steps


typedef struct
{
    uint8_t  channel; // Track channel
    uint8_t  current; // Current step
    uint8_t  direction;
    uint16_t bpm;     // Beats per minute
    uint32_t timestamp[_steps_][2]; // [0]-Note ONs [1]-Note OFFs
    note     data[_steps_];

} sequencer;


void    init_sequence(sequencer* o, int step_length);

void    loop_forward (sequencer* o);
void    loop_backward(sequencer* o);
void    loop_pingpong(sequencer* o);
void    loop_random  (sequencer* o);

note    get_note     (sequencer* o);

extern void (*loop_sequence[])(sequencer*);
