#include "automata.h"
#include "utility.h"

void automata_evolve(automata_t* o)
{
    int xy = o->pos[0] + COLS * o->pos[1];
    if ((o->field>>xy)&1)
    {
        o->field ^= 1<<xy;
        switch (o->current)
        {
            case 0b00:
                o->pos[((o->rule[8]>>1)&0b1)]+=o->rule[12]; 
                o->current = o->rule[0]; 
                break;
            case 0b01:
                o->pos[((o->rule[8])&0b1)]+=o->rule[13]; 
                o->current = o->rule[1]; 
                break;
            case 0b10:
                o->pos[((o->rule[9]>>1)&0b1)]+=o->rule[14]; 
                o->current = o->rule[2]; 
                break;
            case 0b11:
                o->pos[(o->rule[9]&0b1)]+=o->rule[15];
                o->current = o->rule[3]; 
                break;
            default:
                break;
        }
    }
    else
    {
        o->field ^= 1<<xy;
        switch(o->current)
        {
            case 0b00:
                o->pos[((o->rule[10]>>1)&0b1)]+=o->rule[16];
                o->current = o->rule[4]; 
                break;
            case 0b01:
                o->pos[((o->rule[10])&0b1)]+=o->rule[17];
                o->current = o->rule[5]; 
                break;
            case 0b10:
                o->pos[((o->rule[11]>>1)&0b1)]+=o->rule[18];
                o->current = o->rule[6]; 
                break;
            case 0b11:
                o->pos[((o->rule[11])&0b1)]+=o->rule[19];
                o->current = o->rule[7]; 
                break;
            default:
                break;
        }
    }
    if(o->pos[0] >= COLS) o->pos[0] &= (COLS-1);
    else if(o->pos[0] < 0) o->pos[0] = COLS - (o->pos[0]&(COLS-1));
    if(o->pos[1] >= ROWS) o->pos[1] &= (ROWS-1);
    else if(o->pos[1] < 0) o->pos[1] = ROWS - (o->pos[1]&(ROWS-1));
}



void automata_init(automata_t* o)
{
    o->rule[ 0] = 0b01;
    o->rule[ 1] = 0b10;
    o->rule[ 2] = 0b11;
    o->rule[ 3] = 0b00;

    o->rule[ 4] = 0b11;
    o->rule[ 5] = 0b00;
    o->rule[ 6] = 0b01;
    o->rule[ 7] = 0b10;

    o->rule[ 8] = 0b01;
    o->rule[ 9] = 0b01;
    o->rule[10] = 0b01; 
    o->rule[11] = 0b01;

    o->rule[12] =  1;
    o->rule[13] =  1;
    o->rule[14] = -1;
    o->rule[15] = -1;
    o->rule[16] = -1;
    o->rule[17] = -1;
    o->rule[18] =  1;
    o->rule[19] =  1;
    
    o->field = 0;
}

void automata_rand(automata_t* o)
{
    o->pos[0]   = rand_in_range(0, COLS-1);
    o->pos[1]   = rand_in_range(0, ROWS-1);
    o->current  = rand_in_range(0, 0b11);
    for(int i = 0; i < 12; ++i) o->rule[i] = rand_in_range( 0, 3);
    for(int i = 0; i <  8; ++i) o->rule[12 + i] = rand_in_range(-3, 3);
}