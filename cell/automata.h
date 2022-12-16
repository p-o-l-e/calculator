#pragma once
#include <stdint.h>
#include <stdbool.h>

/////////////////////////////////////////////////////////////////////
// Langtons Ant /////////////////////////////////////////////////////
#define ROWS  4
#define COLS  4

typedef struct
{
    uint16_t field;
    int current;
    int rule[20];
    int pos[2];

} automata_t;


void automata_evolve(automata_t* restrict o);
void automata_init(automata_t* restrict o);
void automata_rand(automata_t* restrict o);