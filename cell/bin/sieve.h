// Xenakis sieve
// Based on code by Rodney DuPlessis
// https://github.com/rodneydup/Xenakis-Sieves

#pragma once
#include <string.h>
#include <stdbool.h>
#define COVERED -1

typedef struct
{
   int value; // value of the period
   int shift; // starting point

}  period_t;

int sieve_xen(period_t* o, unsigned data);