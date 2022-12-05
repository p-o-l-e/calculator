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

#include "sequencer.h"
////////////////////////////////////////////////////////////////////////////////////
// Track ///////////////////////////////////////////////////////////////////////////
void track_init(track_t* o)
{
    o->current  = 0;
    o->mode     = 0;
    o->steps    = STEPS;
    o->revolutions = 0;
    o->freerun  = false;
    o->regenerate[0] = 0;
    o->regenerate[1] = 0;
    o->regenerate[2] = 0;
    o->drift[0] = 0;
    o->drift[1] = 0;
    o->drift[2] = 0;
    o->drift[3] = 0;
    for(int i = 0; i < STEPS; ++i)
    {
        o->data[i].degree   = 0;
        o->data[i].octave   = 3;
        o->data[i].velocity = 0x7F;
        o->data[i].value    = 8;
        o->data[i].offset   = 0;
        o->trigger[i] = false;
    }
}

void loop_forward(track_t* o)
{
    ++o->current;
    if(o->current >= o->steps) 
    {
        o->current = 0;
        ++o->revolutions;
    }
}

void loop_backward(track_t* o)
{
    --o->current;
    if(o->current < 0)
    {
        o->current = o->steps - 1;
        ++o->revolutions;
    }
}

void loop_pingpong(track_t* o)
{
    static bool f;
    if(f)
    {
        ++o->current;
        if(o->current >= o->steps) 
        {
            o->current = o->steps - 2;
            f = false;
            ++o->revolutions;
        }
    }
    else
    {
        --o->current;
        if(o->current < 0) 
        {
            f = true;
            o->current = 1;
            ++o->revolutions;
        }
    }
}

void loop_random(track_t* o)
{
    static int r;
    o->current  = rand_in_range(0, o->steps - 1);
    ++r;
    if(r>o->steps)
    {
        r = 0;
        ++o->revolutions;
    }
}

note get_note(track_t* o)
{
    return o->data[o->current];
}

void insert_bits(track_t* o, uint16_t bits)
{
    uint16_t s = bits;
    for(int i = STEPS - 1; i >= 0; --i) 
    {
        o->trigger[i] = s & 1;
        s >>= 1;
    }
}

void (*loop_sequence[])(track_t*) = 
{
    loop_forward,
    loop_backward,
    loop_pingpong,
    loop_random
};

void reset_timestamp(sequencer* o, int track, int bpm)
{
    if(bpm > 800) bpm = 800;
    else if(bpm < 1) bpm = 1;
    o->o[track].bpm  = bpm;
    o->o[track].beat = 60000/o->o[track].bpm;
    o->o[track].step = o->o[track].beat/4; // /4
    o->o[track].atom = o->o[track].step/32; // /32
    if(o->o[track].atom <= 0) o->o[track].atom = 1;
}

void sequencer_init(sequencer* o, int bpm)
{
    o->state = STOP;
    for(int i = 0; i < TRACKS; ++i)
    {
        track_init(&o->o[i]);
        o->o[i].channel = i;
        automata_init(&o->automata[i]);
        reset_timestamp(o, i, bpm);
    }
}

void sequencer_rand(sequencer* o, int track)
{
    // uint16_t beat = rand_in_range(1, 0xFFFF);
    // insert_bits(&o->o[track], beat);
    o->o[track].scale.data = rand_in_range(1, 0xFFF);
    o->o[track].scale.root = rand_in_range(0,    11);
    set_scale(&o->o[track].scale);
    for(int i = 0; i < STEPS; ++i)
    {
        o->o[track].data[i].value    = rand_in_range(1, 32);
        o->o[track].data[i].offset   = 0;//rand_in_range(-0x7F,  0x7F);
        o->o[track].data[i].degree   = rand_in_range(0, 11);
        o->o[track].data[i].octave   = rand_in_range(0, 8);
        o->o[track].data[i].velocity = rand_in_range(0, 0x7F);
        note_from_degree(&o->o[track].scale, &o->o[track].data[i]);
    }
}

void recount_all(sequencer* o, int track)
{
    for(int i = 0; i < STEPS; ++i)
    {
        note_from_degree(&o->o[track].scale, &o->o[track].data[i] );
    }
}