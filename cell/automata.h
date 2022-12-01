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
    int_fast8_t current;
    int_fast8_t rule[16];
    int_fast8_t step[8];
    int_fast8_t pos[2];
    int_fast8_t iterations;

} automata_t;


void automata_evolve(automata_t* o);
void automata_clr(automata_t* o);
void automata_init(automata_t* o);
void automata_rand(automata_t* o);