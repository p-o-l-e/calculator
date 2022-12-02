#include "automata.h"
#include "utility.h"

void automata_evolve(automata_t* o)
{
    for(int i = 0; i < o->iterations; ++i)
    {
    int xy = o->pos[0] + COLS * o->pos[1];
    if (o->field[xy] == true)
    {
        o->field[xy] = false;
        switch (o->current)
        {
            case 0b00:
                o->pos[((o->rule[8]>>1)&0b1)]+=o->step[0]; 
                o->current = o->rule[0]; 
                break;
            case 0b01:
                o->pos[((o->rule[8])&0b1)]+=o->step[1]; 
                o->current = o->rule[1]; 
                break;
            case 0b10:
                o->pos[((o->rule[9]>>1)&0b1)]+=o->step[2]; 
                o->current = o->rule[2]; 
                break;
            case 0b11:
                o->pos[(o->rule[9]&0b1)]+=o->step[3];
                o->current = o->rule[3]; 
                break;
            default:
                break;
        }
    }
    else
    {
        o->field[xy] = true;
        switch(o->current)
        {
            case 0b00:
                o->pos[((o->rule[10]>>1)&0b1)]+=o->step[4];
                o->current = o->rule[4]; 
                break;
            case 0b01:
                o->pos[((o->rule[10])&0b1)]+=o->step[5];
                o->current = o->rule[5]; 
                break;
            case 0b10:
                o->pos[((o->rule[11]>>1)&0b1)]+=o->step[6];
                o->current = o->rule[6]; 
                break;
            case 0b11:
                o->pos[((o->rule[11])&0b1)]+=o->step[7];
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
}

void automata_clr(automata_t* o)
{
    for(int i = 0; i < (ROWS*COLS); i++) o->field[i] = false;
}


void automata_init(automata_t* o)
{
    o->step[ 0] =  1;
    o->step[ 1] =  1;
    o->step[ 2] = -1;
    o->step[ 3] = -1;
    o->step[ 4] = -1;
    o->step[ 5] = -1;
    o->step[ 6] =  1;
    o->step[ 7] =  1;

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
    
    o->iterations = 1;
    automata_clr(o);
}

void automata_rand(automata_t* o)
{
    o->pos[0]   = rand_in_range(0, COLS-1);
    o->pos[1]   = rand_in_range(0, ROWS-1);
    o->current  = rand_in_range(0, 0b11);
    for(int i = 0; i < 12; ++i) o->rule[i] = rand_in_range( 0, 3);
    for(int i = 0; i <  8; ++i) o->step[i] = rand_in_range(-3, 3);
}