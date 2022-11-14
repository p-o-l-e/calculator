#pragma once
#include <stdbool.h>
#include "midi.h"
#include "utility.h"

#define _tracks_ 4  // Number of tracks
#define _steps_  16 // Number of steps


typedef struct
{
    uint8_t  channel;
    uint8_t  current;       // Current step
    uint32_t departed;      // Current time in samples
    uint32_t length;        // Step duration in samples
    note     data[_steps_];

} sequencer;


void    init_sequence(sequencer* o, int l);

void    loop_forward (sequencer* o);
void    loop_backward(sequencer* o);
void    loop_pingpong(sequencer* o);
void    loop_random  (sequencer* o);


UMP32   get_message (sequencer* o);
note    get_note    (sequencer* o);