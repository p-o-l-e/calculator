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

#include <pico/stdlib.h>
#include <hardware/i2c.h>
#include <pico/binary_info.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "ssd1306.h"
#include "font.h"


inline static void ssd1306_write(ssd1306_t *p, uint8_t val) 
{
    uint8_t d[2]= { 0x00, val };
    i2c_write_blocking(p->i2c_i, p->address, d, 2, true);
}

bool ssd1306_init(ssd1306_t *p, uint16_t width, uint16_t height, uint8_t address, i2c_inst_t *i2c_instance) 
{
    p->width    = width;
    p->height   = height;
    p->pages    = height/8;
    p->address  = address;
    p->i2c_i    = i2c_instance;
    p->bufsize  = p->pages * p->width;

    if((p->buffer = malloc(p->bufsize+1)) == NULL) 
    {
        p->bufsize=0;
        return false;
    }
    ++p->buffer;

    // from https://github.com/makerportal/rpi-pico-ssd1306
    uint8_t cmds[] = {
        SET_DISP,
        // timing and driving scheme
        SET_DISP_CLK_DIV,
        0x80,
        SET_MUX_RATIO,
        height - 1,
        SET_DISP_OFFSET,
        0x00,
        // resolution and layout
        SET_DISP_START_LINE,
        // charge pump
        SET_CHARGE_PUMP,
        p->external_vcc ? 0x10 : 0x14,
        SET_SEG_REMAP | 0x01,           // column addr 127 mapped to SEG0
        SET_COM_OUT_DIR | 0x08,         // scan from COM[N] to COM0
        SET_COM_PIN_CFG,
        width > 2 * height ? 0x02 : 0x12,
        // display
        SET_CONTRAST,
        0xff,
        SET_PRECHARGE,
        p->external_vcc ? 0x22 : 0xF1,
        SET_VCOM_DESEL,
        0x30,                           // or 0x40?
        SET_ENTIRE_ON,                  // output follows RAM contents
        SET_NORM_INV,                   // not inverted
        SET_DISP | 0x01,
        // address setting
        SET_MEM_ADDR,
        0x00,  // horizontal
    };

    for(size_t i=0; i<sizeof(cmds); ++i) ssd1306_write(p, cmds[i]);
    return true;
}

inline void ssd1306_deinit(ssd1306_t *p) 
{
    free(p->buffer-1);
}

inline void ssd1306_poweroff(ssd1306_t *p) 
{
    ssd1306_write(p, SET_DISP|0x00);
}

inline void ssd1306_poweron(ssd1306_t *p) 
{
    ssd1306_write(p, SET_DISP|0x01);
}

inline void ssd1306_contrast(ssd1306_t *p, uint8_t val) 
{
    ssd1306_write(p, SET_CONTRAST);
    ssd1306_write(p, val);
}

inline void ssd1306_invert(ssd1306_t *p, uint8_t inv) 
{
    ssd1306_write(p, SET_NORM_INV | (inv & 1));
}

inline void ssd1306_clear(ssd1306_t *p) 
{
    memset(p->buffer, 0, p->bufsize);
}


void ssd1306_show(ssd1306_t *p) 
{
    uint8_t payload[]= { SET_COL_ADDR, 0, p->width - 1, SET_PAGE_ADDR, 0, p->pages - 1 };
    if(p->width==64) 
    {
        payload[1]+=32;
        payload[2]+=32;
    }

    for(size_t i = 0; i < sizeof(payload); ++i) ssd1306_write(p, payload[i]);
    *(p->buffer-1)=0x40;
    i2c_write_blocking(p->i2c_i, p->address, p->buffer-1, p->bufsize+1, true);
}

void ssd1306_PSET(ssd1306_t *p, uint32_t x, uint32_t y) 
{
    if(x>=p->width || y>=p->height) return;
    auto xi = 127 - x;  // INVERTED POSITION !!!
    auto yi = 63  - y;  // INVERTED POSITION !!!
    p->buffer[xi+p->width*(yi>>3)]|=0x1<<(yi&0x07); // y>>3==y/8 && y&0x7==y%8
}

void ssd1306_print_char(ssd1306_t* p, uint8_t x, uint8_t y, uint8_t s, bool invert)
{
    for(int i = 0; i < 8; i++) // X
    {
        uint8_t l = gtFont[(s-32)*8 + i];
        for(int j = 0; j < 8; j++) // Y
        {
            if((l&0b1)!=invert) ssd1306_PSET(p, x + i, y + j);
            l>>=1;
        }
    }
}

void ssd1306_print_string(ssd1306_t* p, uint8_t x, uint8_t y, char* s, bool invert, bool vertical)
{
    if(vertical) for(int i = 0; i < strlen(s); i++) ssd1306_print_char(p, x , y + 8*i, s[i], invert);
    else for(int i = 0; i < strlen(s); i++) ssd1306_print_char(p, x + 8*i, y, s[i], invert);
}

void ssd1306_log(ssd1306_t* p, char* s, uint16_t ms, bool clr)
{
    static char _log_[8][16];
    if(clr) for(int i = 0; i < 8; i++) strcpy(_log_[i], "                ");
    ssd1306_clear(p);
    for(int i = 0; i < 7; i++)    
    {
        strcpy(_log_[i], _log_[i+1]);
        ssd1306_print_string(p, 0, i*8, _log_[i], 0, 0);
    }
    strcpy(_log_[7], "                ");
    strcpy(_log_[7], s);
    ssd1306_print_string(p, 0, 7*8, s, 0, 0);
    ssd1306_show(p);
    sleep_ms(ms);
}
