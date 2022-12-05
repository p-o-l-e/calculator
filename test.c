#include <stdio.h>
#include <limits.h>

unsigned rightrot12(unsigned x, unsigned n)
{
  return ((x >> n) | (x << (12 - n)))&0xFFF; /* CHAR_BIT is defined in limits.h */
}

unsigned leftrot12(unsigned x, unsigned n)
{
  return ((x << n) | (x >> (12 - n)))&0xFFF;
}
int c = 0b011010001000;
int r = 0b100001101000;
int m = 0xFFF;
int main()
{
    printf("%d", rightrot12(c, 4));
}