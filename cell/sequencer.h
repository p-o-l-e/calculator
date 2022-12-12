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

#define TRACKS  4       // Number of tracks
#define STEPS   16      // Maximum number of steps


typedef struct
{
    note data[STEPS];   // Note data
    scale_t scale;      // Note set
    int revolutions;    // Beat counter
    int beat;           // Beat length
    int step;           // Step length
    int atom;           // Minimal note length
    int bpm;            // Beats per minute
    int current;        // Current step
    int drift[4];       // Drift amount of: [0] Velocity [1] Offset
    int channel;        // Track channel
    int steps;          // Steps count
    int mode;           // Loop mode
    int theta;          // Rotation
    uint16_t trigger;   // NoteON bits
    bool euclidean;     // Bresenham
    bool regenerate[4]; // [0] Beat [1] Notes [3] Set scale
    bool reset;         // Recount timestamp
    bool freerun;       // Sync
    bool on;            // On-Off switch

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
void track_init     (track_t* o);
void loop_forward   (track_t* o);
void loop_backward  (track_t* o);
void loop_pingpong  (track_t* o);
void loop_random    (track_t* o);
void insert_bits    (track_t* o, uint16_t bits);
note get_note       (track_t* o);
extern void (*loop_sequence[])(track_t*);

///////////////////////////////////////////////////////////////
// Sequencer routines /////////////////////////////////////////
void reset_timestamp(sequencer* o, int track, int bpm);
void sequencer_init (sequencer* o, int bpm);
void sequencer_run  (sequencer* o);
void sequencer_stop (sequencer* o);
void sequencer_pause(sequencer* o);
void sequencer_rand (sequencer* o, int track);
void sequencer_sag  (sequencer* o, int track, int dest); // Sheep and Goats
void recount_all    (sequencer* o, int track);
uint32_t get_timeout(sequencer* o, int track); // Time to the next step - NULL if timeline is clear
