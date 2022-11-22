#include "scale.h"


void set_scale(scale_t* scale, uint16_t gaps, uint8_t root)
{
    scale->data  = gaps;
    scale->root  = root;
    uint8_t s = 0;
    for(int i = 0; i < 12; i++)
    {
        if(gaps & (0x800 >> i))
        {
            scale->degree[s] = i;
            s++;
        }
    }
    scale->width = s;
}

void note_from_degree(scale_t* scale, note* o)
{
    o->chroma = scale->root + scale->degree[o->degree % scale->width] + 12 * o->octave;
}



const char* chromatic[] = 
{
    "C ",
    "C#",
    "D ",
    "D#",
    "E ",
    "F ",
    "F#",
    "G ",
    "G#",
    "A ",
    "A#",
    "B "
};