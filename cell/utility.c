#include "utility.h"

int rand_in_range(int l, int r) 
{ 
    return (rand() % (r - l + 1)) + l;
}