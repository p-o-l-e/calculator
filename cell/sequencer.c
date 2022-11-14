#include "sequencer.h"


void init_sequence(sequencer* o, int l)
{
    o->departed = 0;
    o->current  = 0;
    o->length   = l;
    for(int i = 0; i < _steps_; i++)
    {
        o->data[i].pitch     = 0;
        o->data[i].detune    = 0;
        o->data[i].velocity  = 0;
        o->data[i].duration  = 0;
    }
}

void loop_forward(sequencer* o)
{
    o->departed++;
    if(o->departed >= o->length) 
    {
        o->current++;
        if(o->current >= _steps_) o->current = 0;
        o->departed = 0;
    }
}

void loop_backward(sequencer* o)
{
    o->departed++;
    if(o->departed >= o->length) 
    {
        if(o->current > 0)
        o->current--;
        else o->current = _steps_ - 1;
        o->departed = 0;
    }
}

void loop_pingpong(sequencer* o)
{
    static bool f;
    o->departed++;
    if(o->departed >= o->length) 
    {
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
        o->departed = 0;
    }
}

void loop_random(sequencer* o)
{
    o->departed++;
    if(o->departed >= o->length) 
    {
        o->current  = rand_in_range(0, _steps_ - 1);
        o->departed = 0;
    }
}

note get_note(sequencer* o)
{
    return o->data[o->current];
}

UMP32 get_message(sequencer* o)
{
    // return o->sequence[track][o->current];
}