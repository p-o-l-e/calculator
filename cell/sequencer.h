#pragma once
#include <stdbool.h>
#include "utility.h"
#include "midi.h"

#define tracks 8  // Number of tracks
#define steps  16 // Number of steps


// Beat length (16 steps) = quarter = 60000ms/BPM
// Step length = Beat/16 (1/64 note)

typedef struct
{
    uint8_t  channel;   // Track channel
    uint8_t  current;   // Current step
    uint8_t  mode;      // Loop mode
    uint64_t timestamp[steps];  // Trigger gaps
    note     data[steps];       // Pitches
    bool     reset;     // Recount timestamp
    bool     freerun;
    bool     on;

} track;

typedef struct 
{
    track o[tracks];
    uint16_t atom;    // Minimal step lenght
    uint16_t bpm;     // Beats per minute

} sequencer;

///////////////////////////////////////////////////////////////
// Track routines /////////////////////////////////////////////

void    track_init(track* o, uint32_t step_length);

void    loop_forward (track* o);
void    loop_backward(track* o);
void    loop_pingpong(track* o);
void    loop_random  (track* o);

note    get_note     (track* o);

extern void (*loop_sequence[])(track*);


///////////////////////////////////////////////////////////////
// Sequencer routines /////////////////////////////////////////
void    reset_timestamp(sequencer* o, uint8_t _track);
    
void    esq_init(sequencer* o, uint16_t bpm);
void    esq_run(sequencer* o);
void    esq_stop(sequencer* o);
void    esq_pause(sequencer* o);