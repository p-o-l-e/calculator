#pragma once
#include <stdint.h>
#include <stdbool.h>
/////////////////////////////////////////////////////////////////////
// Langtons Ant /////////////////////////////////////////////////////
// typedef enum { UP, RIGHT, DOWN, LEFT } direction;

#define UP    0
#define RIGHT 1
#define DOWN  2
#define LEFT  3

#define _LA_WIDTH  4
#define _LA_HEIGHT 4

typedef struct
{
    int current;

    bool field[_LA_WIDTH * _LA_HEIGHT];
    int w;
    int h;
    int l;
    int x;
    int y;
    int pos;

} langtons_ant;

void la_move(langtons_ant* la);
void la_clr(langtons_ant* la);
void la_init(langtons_ant* la, int width, int height);