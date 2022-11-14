#include "sequencer.h"
#include "utility.h"

void init_sequence(sequencer* o, int l)
{
    o->departed = 0;
    o->current  = 0;
    o->length   = l;
}

void process_sequence(sequencer* o)
{
    o->departed++;
    if(o->departed >= o->length) 
    {
        o->current++;
        if(o->current == STEPS) o->current = 0;
        o->departed = 0;
        o->out = o->note[o->current];
        ar_init(&o->env);
    }
}


float get_note(sequencer* o)
{
    return o->note[o->current];
}


void genRand(sequencer* o)
{
    for(int i = 0; i < STEPS; i++)
    {
        o->note[i] = (float)rand_in_range(0, 24)/24.0f;
    }
}