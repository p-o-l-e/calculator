#include "sequencer.h"

////////////////////////////////////////////////////////////////////////////////////
// Track ///////////////////////////////////////////////////////////////////////////
void track_init(track* o, uint32_t step_length)
{
    o->current  = 0;
    o->mode     = 0;
    
    for(int i = 0; i < steps; i++)
    {
        o->data[i].pitch    = 36;
        o->data[i].velocity = 0x7F;
        o->data[i].value    = 1;
        o->step = step_length;
    }
}

void loop_forward(track* o)
{
    o->current++;
    if(o->current >= steps) o->current = 0;
}

void loop_backward(track* o)
{
    if(o->current > 0)
    o->current--;
    else o->current = steps - 1;
}

void loop_pingpong(track* o)
{
    static bool f;
    if(f)
    {
        o->current++;
        if(o->current >= steps) 
        {
            o->current = steps - 2;
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
    o->current  = rand_in_range(0, steps - 1);
}

note get_note(track* o)
{
    return o->data[o->current];
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
void reset_timestamp(sequencer* o, uint8_t _track, uint16_t bpm)
{
    o->o[_track].bpm  = bpm;
    o->o[_track].beat = 60000/o->o[_track].bpm;
    o->o[_track].step = o->o[_track].beat/4;
}

void sequencer_init(sequencer* o, uint16_t bpm)
{
    for(int i = 0; i < tracks; i++)
    {
        reset_timestamp(o, i, bpm);
    }
}