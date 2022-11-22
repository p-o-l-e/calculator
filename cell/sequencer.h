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
#include "midi.h"

#define _tracks 2  // Number of tracks
#define _steps  16 // Maximum umber of steps


// Beat length (16 steps) = quarter = 60000ms/BPM
// Step length = Beat/16 (1/64 note)

typedef struct
{
    note     data[_steps];
    uint32_t beat;      // Beat length
    uint32_t step;      // Step length
    uint32_t atom;      // Minimal note length
    uint16_t bpm;       // Beats per minute
    uint8_t  channel;   // Track channel
    uint8_t  steps;     // Steps count
    uint8_t  current;   // Current step
    uint8_t  mode;      // Loop mode
    bool     trigger[_steps]; // NoteON bits
    bool     regenerate[2]; // [0] Beat [1] Notes
    bool     reset;     // Recount timestamp
    bool     freerun;
    bool     on;

} track;

#define PLAY  1
#define PAUSE 2
#define STOP  0

typedef struct 
{
    track o[_tracks];
    uint8_t state;

} sequencer;

///////////////////////////////////////////////////////////////
// Track routines /////////////////////////////////////////////

void    track_init(track* o);

void    loop_forward (track* o);
void    loop_backward(track* o);
void    loop_pingpong(track* o);
void    loop_random  (track* o);

note    get_note     (track* o);

extern void (*loop_sequence[])(track*);


///////////////////////////////////////////////////////////////
// Sequencer routines /////////////////////////////////////////
void    reset_timestamp(sequencer* o, uint8_t _track, uint16_t bpm);
void    sequencer_init(sequencer* o, uint16_t bpm);
void    sequencer_run(sequencer* o);
void    sequencer_stop(sequencer* o);
void    sequencer_pause(sequencer* o);

uint32_t get_timeout(sequencer* o, uint8_t track); // Time to the next step - NULL if timeline is clear
