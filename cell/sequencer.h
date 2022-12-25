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
#include <string.h>
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
    int sieve[8];       // Sieve
    int gaps;           // Sieve gaps
    int median;			// Sieve center
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
    bool regenerate;	// Regenerate sieve
    bool permute[6];    // [0]Degree [1]Octave [2]Velocity [3]Duration [4]Offset [5]Sieve
    bool sift[5];		// [0]Degree [1]Octave [2]Velocity [3]Duration [4]Offset
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
    int state;			// Play / Pause / Stop
    int brush; 			// Brush width
    int form;			// Brush type

} sequencer;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Track routines /////////////////////////////////////////////////////////////////////////////////////////////////////////////
void track_init     (track_t* restrict o);
void loop_forward   (track_t* restrict o);
void loop_backward  (track_t* restrict o);
void loop_pingpong  (track_t* restrict o);
void loop_random    (track_t* restrict o);
void insert_bits    (track_t* restrict o, uint16_t bits);
note get_note       (track_t* restrict o);
extern void (*loop_sequence[])(track_t* restrict);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sequencer routines /////////////////////////////////////////////////////////////////////////////////////////////////////////
void reset_timestamp(sequencer* restrict o, int track, int bpm);
void sequencer_init (sequencer* restrict o, int bpm);
void sequencer_run  (sequencer* restrict o);
void sequencer_stop (sequencer* restrict o);
void sequencer_pause(sequencer* restrict o);
void sequencer_rand (sequencer* restrict o, int track);
void recount_all    (sequencer* restrict o, int track);
uint32_t get_timeout(sequencer* restrict o, int track); // Time to the next step - NULL if timeline is clear

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Section setters ////////////////////////////////////////////////////////////////////////////////////////////////////////////
void velocity_rise (sequencer* restrict o, int track, int center, int range, int value, int incr);
void velocity_fall (sequencer* restrict o, int track, int center, int range, int value, int incr);
void velocity_wave (sequencer* restrict o, int track, int center, int range, int value, int incr);
void velocity_rect (sequencer* restrict o, int track, int center, int range, int value, int incr);
void duration_rise (sequencer* restrict o, int track, int center, int range, int value, int incr);
void duration_fall (sequencer* restrict o, int track, int center, int range, int value, int incr);
void duration_wave (sequencer* restrict o, int track, int center, int range, int value, int incr);
void offset_rise   (sequencer* restrict o, int track, int center, int range, int value, int incr);
void offset_fall   (sequencer* restrict o, int track, int center, int range, int value, int incr);
void offset_wave   (sequencer* restrict o, int track, int center, int range, int value, int incr);

extern void (*set_section[])(sequencer* restrict, int, int, int, int, int);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sheep and Goats - Permutations /////////////////////////////////////////////////////////////////////////////////////////////
void sag_degree		(sequencer* restrict o, int track, uint16_t data);   
void sag_octave		(sequencer* restrict o, int track, uint16_t data);
void sag_velocity	(sequencer* restrict o, int track, uint16_t data);
void sag_duration	(sequencer* restrict o, int track, uint16_t data);
void sag_offset		(sequencer* restrict o, int track, uint16_t data);
void sag_sieve		(sequencer* restrict o, int track, uint16_t data);

extern void (*mutate[])(sequencer* restrict, int, uint16_t); 

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sieve //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void regenerate_sieve(sequencer* restrict o, int track, uint16_t data);
void sift_degree	(sequencer* restrict o, int track);
void sift_octave	(sequencer* restrict o, int track);
void sift_velocity	(sequencer* restrict o, int track);
void sift_duration	(sequencer* restrict o, int track);
void sift_offset	(sequencer* restrict o, int track);

extern void (*sift[])(sequencer* restrict, int);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline void fit_velocity(sequencer* restrict o, int track, int position)
{
	if(o->o[track].data[position].velocity > 0x7F) o->o[track].data[position].velocity = 0x7F;
	else if(o->o[track].data[position].velocity < 1) o->o[track].data[position].velocity = 1;
}

inline void fit_duration(sequencer* restrict o, int track, int position)
{
	if(o->o[track].data[position].value > 0xFF) o->o[track].data[position].value = 0xFF;
    else if(o->o[track].data[position].value < 1) o->o[track].data[position].value = 1;
}

inline void fit_offset(sequencer* restrict o, int track, int position)
{
	if(o->o[track].data[position].offset > 0x20) o->o[track].data[position].offset = 0x20;
    else if(o->o[track].data[position].offset < 0) o->o[track].data[position].offset = 0;
}
