#pragma once
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/clocks.h"
#include "cell/midi_uart.h"
#include "SSD1306/ssd1306.h"
#include "CD74HC4067.h"
#include "SSD1306/ssd1306.h"
#include "74HC595/sr_common.h"
#include "quadrature-decoder/quadrature_decoder.h"
#include "bsp/board.h"
#include "tusb.h"

#define SPI_PORT    spi0 
#define CLOCK_595   18
#define DATA_595    19
#define LATCH_595   16

#define SDA_PIN     2
#define SCL_PIN     3
#define RESET_PIN  -1
#define OLED_WIDTH  128
#define OLED_HEIGHT 64
////////////////////////////////////////
#define PGLFT       4   // Page Left
#define PGRGT       5   // Page Right
#define ALTGR       6   // Upper alt
#define ENCDR       7   // Encoder button
#define BTNUP       8   // Upper button
#define BTNCT       9   // Center button
#define BTNDW       10  // Lower button
#define SHIFT       11  // Shift

#define N4067       12  // 4067 Inputs
// Encoder ////////////////////////////
#define NCODER_A    9
#define NCODER_B    10
// Button matrix columns: GPIOs ///////
#define MCOL0       17
#define MCOL1       20
#define MCOL2       21
#define MCOL3       22
// Button matrix rows: 4067 ///////////
#define MROW0       0
#define MROW1       1
#define MROW2       2
#define MROW3       3

#define cable_num   0

static const int _matrix[4] = { MCOL0, MCOL1, MCOL2, MCOL3 };

void keypad_init() 
{
    for(int i = 0; i < 4; i++)
    {
        gpio_init(_matrix[i]);
        gpio_set_dir(_matrix[i], GPIO_OUT);
    }
}

int keypad_switch()
{
    static int f;
    static int column;
    if(f > 3) { f = 0; column++; }
    if(column > 3) column = 0;
    for(int i = 0; i < 4; i++)
    {
        if(i == column) gpio_put(_matrix[i], 1);
        else gpio_put(_matrix[i], 0);
    }
    f++;
    return column;
}

int32_t quad_encoder_init(quadrature_decoder *ncoder)
{
    gpio_init(NCODER_A);
    gpio_init(NCODER_B);
    gpio_pull_down(NCODER_A);
    gpio_pull_down(NCODER_B);
    quadrature_decoder_init(ncoder, pio0);
    return add_quadrature_decoder(ncoder, NCODER_A);
}