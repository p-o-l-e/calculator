/* automaton.c
* Generates a cellular automaton with ROWS rows and COLS cols given a rule. */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#define ROWS 16
#define COLS 16



typedef struct
{
	uint64_t field[ROWS][COLS];
	uint64_t rule;
    int64_t  nbrs;

} element;




void element_init(element* o)
{
    o->rule = 90;
    for(int i = 0; i < ROWS; i++) 
	{
        for(int j = 0; j < COLS; j++) 
		{
            o->field[i][j] = 1;
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

/* Prints out the cellular automaton */
void print_grid(element* o)
{
    for(int i = 0; i < ROWS; i++) 
    {
        for (int j = 0; j < COLS; j++) 
        {
            printf("%d ", o->field[i][j]);
        }
        printf("\n");
    }
}

element el;
int main(int argc, char* argv[])
{   
    element_init(&el);

    
    for (int i = 0; i < COLS; i++) 
    {
        el.rule++;
        evolve(&el);
        print_grid(&el);
    }

    // grid[0][COLS/2] = 1; // initial grid configuration

    
    return 0;
}