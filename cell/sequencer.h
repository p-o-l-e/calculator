// MIT License

// Copyright (c) 2022 unmanned

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once
#include <stdbool.h>
#include "utility.h"
#include "scale.h"
#include "midi.h"
#include "automata.h"

#define TRACKS  8       // Number of tracks
#define STEPS   16      // Maximum number of steps


typedef struct
{
    note data[STEPS];   // Note data
    scale_t scale;      // Note set
    uint16_t trigger;   // NoteON bits
    int revolutions;    // Beat counter
    int beat;           // Beat length
    int step;           // Step length
    int atom;           // Minimal note length
    int bpm;            // Beats per minute
    int current;        // Current step
    int channel;        // Track channel
    int steps;          // Steps count
    int mode;           // Loop mode
    bool euclidean;     // Bresenham
    bool regenerate[4]; // [0]Beat [1]Note [2]Octave [3]Velocity
    bool reset;         // Recount timestamp
    bool freerun;       // Sync

} track_t;

#define PLAY  1
#define PAUSE 2
#define STOP  0

typedef struct 
{
    automata_t automata[TRACKS];
    track_t o[TRACKS];
    int  state;

} sequencer;

///////////////////////////////////////////////////////////////
// Track routines /////////////////////////////////////////////
void track_init     (track_t* restrict o);
void loop_forward   (track_t* restrict o);
void loop_backward  (track_t* restrict o);
void loop_pingpong  (track_t* restrict o);
void loop_random    (track_t* restrict o);
void insert_bits    (track_t* restrict o, uint16_t bits);
note get_note       (track_t* restrict o);
extern void (*loop_sequence[])(track_t* restrict);

///////////////////////////////////////////////////////////////
// Sequencer routines /////////////////////////////////////////
void reset_timestamp(sequencer* restrict o, int track, int bpm);
void sequencer_init (sequencer* restrict o, int bpm);
void sequencer_run  (sequencer* restrict o);
void sequencer_stop (sequencer* restrict o);
void sequencer_pause(sequencer* restrict o);
void sequencer_rand (sequencer* restrict o, int track);
void recount_all    (sequencer* restrict o, int track);
uint32_t get_timeout(sequencer* restrict o, int track); // Time to the next step - NULL if timeline is clear
void sag_degree(sequencer* restrict o, int track, uint16_t data);   // Sheep and Goats - degree
void siv_degree(sequencer* restrict o, int track, uint16_t data);   // Sieve shift - degree
void prm_degree(sequencer* restrict o, int track, uint16_t data);   // Sieve shift - degree
void sag_octave(sequencer* restrict o, int track, uint16_t data);   // Sheep and Goats - octave
void sag_velocity(sequencer* restrict o, int track, uint16_t data); // Sheep and Goats - octave
void rlf_velocity(sequencer* restrict o, int track, uint16_t data); // Sheep and Goats - octave
void xlr_velocity(sequencer* restrict o, int track, uint16_t data);
void rrl_velocity(sequencer* restrict o, int track, uint16_t data);
void irl_velocity(sequencer* restrict o, int track, uint16_t data);

extern void (*mutate[])(sequencer* restrict, int, uint16_t); 