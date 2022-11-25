// MIT License

// Copyright (c) 2021 David Schramm
// Copyright (c) 2022 Unmanned

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.


#pragma once
#include <pico/stdlib.h>
#include <hardware/i2c.h>
#include <stdint.h>
#include <stdbool.h>
#include "font.h"


typedef enum 
{
    SET_CONTRAST 		= 0x81,
    SET_ENTIRE_ON 		= 0xA4,
    SET_NORM_INV 		= 0xA6,
    SET_DISP 			= 0xAE,
    SET_MEM_ADDR 		= 0x20,
    SET_COL_ADDR 		= 0x21,
    SET_PAGE_ADDR 		= 0x22,
    SET_DISP_START_LINE = 0x40,
    SET_SEG_REMAP 		= 0xA0,
    SET_MUX_RATIO 		= 0xA8,
    SET_COM_OUT_DIR 	= 0xC0,
    SET_DISP_OFFSET 	= 0xD3,
    SET_COM_PIN_CFG 	= 0xDA,
    SET_DISP_CLK_DIV 	= 0xD5,
    SET_PRECHARGE 		= 0xD9,
    SET_VCOM_DESEL 		= 0xDB,
    SET_CHARGE_PUMP 	= 0x8D

} ssd1306_command_t;


typedef struct 
{
    uint8_t width; 		/**< width of display */
    uint8_t height; 	/**< height of display */
    uint8_t pages;		/**< stores pages of display (calculated on initialization*/
    uint8_t address; 	/**< i2c address of display*/
    i2c_inst_t *i2c_i; 	/**< i2c connection instance */
    bool external_vcc; 	/**< whether display uses external vcc */ 
    uint8_t *buffer;	/**< display buffer */
    size_t bufsize;		/**< buffer size */

} ssd1306_t;


bool ssd1306_init(ssd1306_t *p, uint16_t width, uint16_t height, uint8_t address, i2c_inst_t *i2c_instance);
void ssd1306_poweroff(ssd1306_t *p);
void ssd1306_poweron(ssd1306_t *p);
void ssd1306_contrast(ssd1306_t *p, uint8_t val);
void ssd1306_invert(ssd1306_t *p, uint8_t inv);
void ssd1306_show(ssd1306_t *p);
void ssd1306_clear(ssd1306_t *p);

void ssd1306_PSET(ssd1306_t *p, uint32_t x, uint32_t y);
void ssd1306_print_char(ssd1306_t* p, uint8_t x, uint8_t y, uint8_t s, bool invert);
void ssd1306_print_string(ssd1306_t* p, uint8_t x, uint8_t y, char* s, bool invert, bool vertical);
void ssd1306_log(ssd1306_t* p, char* s, uint16_t ms, bool clr);
void ssd1306_line(ssd1306_t* oled, uint8_t x, uint8_t y, uint8_t length, bool vertical);
void ssd1306_progress_bar(ssd1306_t* oled, uint16_t value, uint16_t x, uint16_t y, uint16_t max, uint8_t length, uint8_t width, bool vertical);
void ssd1306_glyph(ssd1306_t* oled, bool* data, uint8_t w, uint8_t h, uint8_t x, uint8_t y);

