#pragma once
#include <stdint.h>
#include <stdbool.h>

/////////////////////////////////////////////////////////////////////
// Langtons Ant /////////////////////////////////////////////////////
#define ROWS  4
#define COLS  4

typedef struct
{
    int current;
    bool field[ROWS * COLS];
    int8_t rule[16];
    int8_t step[8];
    int pos[2];
    int iterations;

} automata;


typedef struct
{
	uint64_t field[ROWS][COLS];
	uint64_t rule;
    int64_t  nbrs;

} element;

void element_init(element* o);
void evolve(element* o);

void automata_evolve(automata* o);
void automata_clr(automata* o);
void automata_init(automata* o);
void automata_rand(automata* o);