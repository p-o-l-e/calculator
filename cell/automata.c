#include "automata.h"


void la_move(langtons_ant* la)
{

    la->pos = la->x + la->w * la->y;

    if (la->field[la->pos] == true)
    {
        la->field[la->pos] = false;
        if(la->current == UP) // Up
        { la->x++; la->current = RIGHT; }
        else if(la->current == RIGHT) // Right
        { la->y++; la->current = DOWN; }
        else if(la->current == DOWN) // Down
        { la->x--; la->current = LEFT; }
        else if(la->current == LEFT) // Left
        { la->y--; la->current = UP; }            
    }
    else
    {
        la->field[la->pos] = true;
        if(la->current == UP) // Up
        { la->x--; la->current = LEFT; }
        else if(la->current == RIGHT) // Right
        { la->y--; la->current = UP; }
        else if(la->current == DOWN) // Down
        { la->x++; la->current = RIGHT; }
        else if(la->current == LEFT) // Left
        { la->y++; la->current = DOWN; }        
    }
    if(la->x >= la->w) la->x = 0;
    if(la->y >= la->h) la->y = 0;
    if(la->x < 0) la->x = la->w - 1;
    if(la->y < 0) la->y = la->h - 1;
}

void la_clr(langtons_ant* la)
{
    for(int i = 0; i < (_LA_HEIGHT*_LA_WIDTH); i++) la->field[i] = false;
}


void la_init(langtons_ant* la, int width, int height)
{
    la->w = _LA_WIDTH;
    la->h = _LA_HEIGHT;
    la_clr(la);
}