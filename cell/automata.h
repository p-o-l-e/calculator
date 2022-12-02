#pragma once
#include <stdint.h>
#include <stdbool.h>

/////////////////////////////////////////////////////////////////////
// Langtons Ant /////////////////////////////////////////////////////
#define ROWS  4
#define COLS  4

typedef struct
{
    bool field[ROWS * COLS];
    int current;
    int rule[16];
    int step[8];
    int pos[2];
    int iterations;

} automata_t;


void automata_evolve(automata_t* o);
void automata_clr(automata_t* o);
void automata_init(automata_t* o);
void automata_rand(automata_t* o);