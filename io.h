#pragma once
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/binary_info.h"
#include "SSD1306/ssd1306.h"
#include "74HC595/sr_common.h"
#include "quadrature-decoder/quadrature_decoder.h"
#include "bsp/board.h"
#include "tusb.h"

#define SPI_PORT    spi0 
#define CLOCK_595   18
#define DATA_595    19
#define LATCH_595   16
CD74HC595 sr;

#define SDA_PIN     2
#define SCL_PIN     3
#define RESET_PIN  -1
#define OLED_WIDTH  128
#define OLED_HEIGHT 64
ssd1306_t oled;

#define PGLFT   4   // Page Left
#define PGRGT   5   // Page Right
#define ALTGR   6   // Upper alt
#define ENCDR   7   // Encoder button
#define BTNUP   8
#define BTNCT   9
#define BTNDW   10
#define SHIFT   11


#define N4067   12  // 4067 Inputs
// Encoder //////////////////////////
#define NCODER_A    9
#define NCODER_B    10
quadrature_decoder ncoder;
int32_t ncoder_index;
// Button matrix columns: GPIOs ////
#define MCOL0   17
#define MCOL1   20
#define MCOL2   21
#define MCOL3   22
// Button matrix rows: 4067 ////////
#define MROW0   0
#define MROW1   1
#define MROW2   2
#define MROW3   3

volatile uint8_t _4067_iterator = 4;
const uint8_t _matrix[4] = { MCOL0, MCOL1, MCOL2, MCOL3};

void keypad_init() 
{
    for(int i = 0; i < 4; i++)
    {
        gpio_init(_matrix[i]);
        gpio_set_dir(_matrix[i], GPIO_OUT);
    }
}

uint8_t keypad_switch()
{
    static uint8_t f;
    static uint8_t column;
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

int32_t quad_encoder_init()
{
    gpio_init(NCODER_A);
    gpio_init(NCODER_B);
    gpio_pull_down(NCODER_A);
    gpio_pull_down(NCODER_B);
    quadrature_decoder_init(&ncoder, pio0);
    return add_quadrature_decoder(&ncoder, NCODER_A);
}