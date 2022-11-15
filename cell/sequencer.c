#include "sequencer.h"


void init_sequence(sequencer* o, int step_length)
{
    o->current   = 0;
    o->bpm       = step_length * 16;
    o->direction = 0;
    for(int i = 0; i < _steps_; i++)
    {
        o->data[i].pitch    = 36;
        o->data[i].velocity = 0x7F;
        o->data[i].value    = 1;
        o->timestamp[i][0]  = step_length;
    }
}

void loop_forward(sequencer* o)
{
    o->current++;
    if(o->current >= _steps_) o->current = 0;
}

void loop_backward(sequencer* o)
{
    if(o->current > 0)
    o->current--;
    else o->current = _steps_ - 1;
}

void loop_pingpong(sequencer* o)
{
    static bool f;
    if(f)
    {
        o->current++;
        if(o->current >= _steps_) 
        {
            o->current = _steps_ - 2;
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

void loop_random(sequencer* o)
{
    o->current  = rand_in_range(0, _steps_ - 1);
}

note get_note(sequencer* o)
{
    return o->data[o->current];
}

void (*loop_sequence[])(sequencer*) = 
{
    loop_forward,
    loop_backward,
    loop_pingpong,
    loop_random
};