#include "automata.h"
#include "utility.h"


void ant_evolve(ant* o)
{
    int xy = o->pos[0] + COLS * o->pos[1];
    if (o->field[xy] == true)
    {
        o->field[xy] = false;
        // 0b 00000000 01010101 01101100 11000110
        if(o->current == 0b00) 
        { 
            o->pos[((o->rule>>23)&0b1)]+=o->step[0]; 
            o->current = ((o->rule>>14)&0b11); 
        }
        else if(o->current == 0b01) 
        { 
            o->pos[((o->rule>>22)&0b1)]+=o->step[1]; 
            o->current = ((o->rule>>12)&0b11); 
        }
        else if(o->current == 0b10) 
        { 
            o->pos[((o->rule>>21)&0b1)]+=o->step[2]; 
            o->current = ((o->rule>>10)&0b11); 
        }
        else if(o->current == 0b11) 
        { 
            o->pos[((o->rule>>20)&0b1)]+=o->step[3];
            o->current = ((o->rule>> 8)&0b11); 
        }
    }
    else
    {
        o->field[xy] = true;
             if(o->current == 0b00) 
        { 
            o->pos[((o->rule>>19)&0b1)]+=o->step[4];
            o->current = ((o->rule>>6)&0b11); 
        }
        else if(o->current == 0b01) 
        { 
            o->pos[((o->rule>>18)&0b1)]+=o->step[5];
            o->current = ((o->rule>>4)&0b11); 
        }
        else if(o->current == 0b10) 
        { 
            o->pos[((o->rule>>17)&0b1)]+=o->step[6];
            o->current = ((o->rule>>2)&0b11); 
        }
        else if(o->current == 0b11) 
        { 
            o->pos[((o->rule>>16)&0b1)]+=o->step[7];
            o->current = ((o->rule   )&0b11); 
        }
    }
    if(o->pos[0] >= COLS) o->pos[0] &= (COLS-1);
    if(o->pos[1] >= ROWS) o->pos[1] &= (ROWS-1);
    if(o->pos[0] < 0) o->pos[0] = COLS - (o->pos[0]&(COLS-1));
    if(o->pos[1] < 0) o->pos[1] = ROWS - (o->pos[1]&(ROWS-1));
}

void ant_clr(ant* o)
{
    for(int i = 0; i < (ROWS*COLS); i++) o->field[i] = false;
}


void ant_init(ant* o)
{
    o->pos[0]   = rand_in_range(0, COLS-1);
    o->pos[1]   = rand_in_range(0, ROWS-1);
    o->current  = rand_in_range(0, 0b11);
    o->rule     = LANGTONS_ANT;
    // o->rule = rand_in_range(1, 0xFFF);
    o->step[0] =  1;
    o->step[1] =  1;
    o->step[2] = -1;
    o->step[3] = -1;
    o->step[4] = -1;
    o->step[5] = -1;
    o->step[6] =  1;
    o->step[7] =  1;

    // for(int i = 0; i < 8; i++) o->step[i] = rand_in_range(-3, 3);
    ant_clr(o);
}




void element_init(element* o)
{
    o->rule = 105;
    for(int i = 0; i < ROWS-1; i++) 
	{
        for(int j = 1; j < COLS-1; j++) 
		{
            o->field[i][j] = 0;
        }
    }
}

void evolve(element* o)
{
    for (int i = 0; i < ROWS-1; i++) 
	{
        for (int j = 1; j < COLS-1; j++) 
		{
            o->nbrs = (o->field[i][j - 1] << 2) | (o->field[i][j] << 1) | (o->field[i][j + 1]);
            o->field[i+1][j] = (o->rule >> o->nbrs) & 0x1;
        }
    }
}

