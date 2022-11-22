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
void track_init(track* o)
{
    o->current  = 0;
    o->mode     = 0;
    o->steps    = _steps;
    for(int i = 0; i < _steps; i++)
    {
        o->data[i].degree   = 0;
        o->data[i].octave   = 3;
        o->data[i].velocity = 0x7F;
        o->data[i].value    = 8;
        o->data[i].offset   = 0;
    }
}

void loop_forward(track* o)
{
    o->current++;
    if(o->current >= o->steps) o->current = 0;
}

void loop_backward(track* o)
{
    if(o->current > 0)
    o->current--;
    else o->current = o->steps - 1;
}

void loop_pingpong(track* o)
{
    static bool f;
    if(f)
    {
        o->current++;
        if(o->current >= o->steps) 
        {
            o->current = o->steps - 2;
            f = false;
        }
    }
    else
    {
        if(o->current > 0)
        o->current--;
        else 
        {
            f = true;
            o->current = 1;
        }
    }
}

void loop_random(track* o)
{
    o->current  = rand_in_range(0, o->steps - 1);
}

note get_note(track* o)
{
    return o->data[o->current];
}

void insert_bits(track* o, uint16_t bits)
{
    uint16_t s = bits;
    for(int i = _steps - 1; i >= 0; i--) 
    {
        o->trigger[i] = s & 1;
        s >>= 1;
    }
}

void (*loop_sequence[])(track*) = 
{
    loop_forward,
    loop_backward,
    loop_pingpong,
    loop_random
};

////////////////////////////////////////////////////////////////////////////////////
// Sequencer ///////////////////////////////////////////////////////////////////////
void reset_timestamp(sequencer* o, uint8_t track, uint16_t bpm)
{
    o->o[track].bpm  = bpm;
    o->o[track].beat = 60000/o->o[track].bpm;
    o->o[track].step = o->o[track].beat/4;
    o->o[track].atom = o->o[track].step/32;
}

void sequencer_init(sequencer* o, uint16_t bpm)
{
    o->state = PLAY;
    for(int i = 0; i < _tracks; i++)
    {
        track_init(&o->o[i]);
        reset_timestamp(o, i, bpm);
    }
}

uint32_t get_timeout(sequencer* o, uint8_t track)
{
    for(int i = 0; i < o->o[track].steps; i++)
    {
        int s = (i + o->o[track].current) % o->o[track].steps;
        if(o->o[track].trigger[s])
        {
            return (i + 1) * o->o[track].step + o->o[track].data[s].offset;
        }
    }
    return 0;
}

void sequencer_randomize(sequencer* o, uint8_t _track)
{
    uint16_t beat = rand_in_range(1, 0xFFFF);
    insert_bits(&o->o[_track], beat);
    for(int i = 0; i < _steps; i++)
    {
        o->o[_track].data[i].value    = rand_in_range(0,  0xFF);
        o->o[_track].data[i].offset   = rand_in_range(0,  0xFF);
        o->o[_track].data[i].degree   = rand_in_range(0,    11);
        o->o[_track].data[i].octave   = rand_in_range(0,     8);
        o->o[_track].data[i].velocity = rand_in_range(0,  0x7F);
        o->o[_track].scale.data       = rand_in_range(1, 0xFFF);
        o->o[_track].scale.root       = rand_in_range(0,    11);
        note_from_degree(&o->o[_track].scale, &o->o[_track].data[i]);
    }
}