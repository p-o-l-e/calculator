#include "keypad.h"

uint8_t _matrix[4][2]; // [0] Columns - [1] Rows

void keypad_init() 
{
    for (int i = 0; i < 4; i++) 
    {
        gpio_init(_matrix[i][0]);
        gpio_init(_matrix[i][1]);

        gpio_set_dir(_matrix[i][1], GPIO_IN);
        gpio_set_dir(_matrix[i][0], GPIO_OUT);
    }
}


uint8_t keypad_get() 
{
    for(int y = 0; y < 4; y++)
    {
        gpio_put(_matrix[y][0], 1);
        for(int x = 0; x < 4; x++)
        {
            if(gpio_get(_matrix[x][1])) 
            {
                return y * 4 + x + 1;
            }
        }
        gpio_put(_matrix[y][0], 0);
    }
    return 0;
}
