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

#define ROWS  4
#define COLS  4
#define LANGTONS_ANT 0b00000000010101010110110011000110
typedef struct
{
    int current;
    bool field[ROWS * COLS];
    int step[8];
    uint32_t rule;
    int pos[2];

} ant;


typedef struct
{
	uint64_t field[ROWS][COLS];
	uint64_t rule;
    int64_t  nbrs;

} element;

void element_init(element* o);
void evolve(element* o);

void ant_evolve(ant* o);
void ant_clr(ant* o);
void ant_init(ant* o);