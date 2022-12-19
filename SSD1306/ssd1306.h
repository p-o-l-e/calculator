#ifndef __SSD1306_H__
#define __SSD1306_H__

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pico/stdlib.h>
#include <hardware/i2c.h>
#include "font.h"


#define SSD1306_CONTRAST 0x20
#define SSD1306_DISPLAYPAUSEOFF 0xA4
#define SSD1306_DISPLAYPAUSEON 0xA5
#define SSD1306_NORMALDISPLAY 0xA6
#define SSD1306_INVERTDISPLAY 0xA7
#define SSD1306_DISPLAYOFF 0xAE
#define SSD1306_DISPLAYON 0xAF
#define SSD1306_DISPLAYOFFSET 0xD3
#define SSD1306_COMPINS 0xDA
#define SSD1306_VCOMDESELECT 0xDB
#define SSD1306_DISPLAYCLOCKDIV 0xD5
#define SSD1306_PRECHARGE 0xD9
#define SSD1306_MULTIPLEX 0xA8
#define SSD1306_LOWCOLUMN 0x00
#define SSD1306_HIGHCOLUMN 0x10
#define SSD1306_STARTLINE 0x40
#define SSD1306_MEMORYMODE 0x20
#define SSD1306_COLUMNADDR 0x21
#define SSD1306_PAGEADDR   0x22
#define SSD1306_COMSCANNORMAL 0xC0
#define SSD1306_COMSCANREMAP 0xC8
#define SSD1306_SEGNORMAL 0xA0
#define SSD1306_SEGREMAP 0xA0
#define SSD1306_CHARGEPUMP 0x8D
#define SSD1306_EXTERNALVCC 0x1
#define SSD1306_SWITCHCAPVCC 0x2

#define SSD1306_CLOCKDIVRESET 0x80
#define SSD1306_DISPLAYOFFSETRESET 0x00
#define SSD1306_ENABLECHARGEPUMP 0x8D
#define SSD1306_COMPINSRESET 0x12
#define SSD1306_CONTRASTRESET 0x7f
#define SSD1306_PRECHARGERESET 0x22
#define SSD1306_VCOMDESELECTRESET 0x20
#define SSD1306_PAGEADDRSTART 0x00
#define SSD1306_PAGEADDREND 0x07
#define SSD1306_COLUMNADDRSTART 0x00

#define SSD1306_CONTROL_COMMAND 0x00
#define SSD1306_CONTROL_DATA 0x40


typedef enum 
{
	BLACK = 0,
    WHITE = 1,

} ssd1306_color_t;

typedef enum 
{
    HORIZONTAL_ADDR = 0x00,
    VERTICAL_ADDR   = 0x01,
    PAGE_ADDR       = 0x02

} ssd1306_memory_mode_t;

typedef struct 
{
    uint16_t addr;
    i2c_inst_t* restrict i2c;
    uint8_t width;
    uint8_t height;
    uint8_t* restrict buffer;

} ssd1306_t;

uint32_t ssd1306_init(ssd1306_t* restrict ssd, uint16_t addr, i2c_inst_t* restrict i2c, ssd1306_color_t color);

static inline void ssd1306_free(ssd1306_t* restrict ssd) 
{
    free(ssd->buffer);
}

static inline void ssd1306_send_command(ssd1306_t* restrict ssd, uint8_t command) 
{
    const uint8_t message[] = {SSD1306_CONTROL_COMMAND, command};
    i2c_write_blocking(ssd->i2c, ssd->addr, message, 2, false);
}

void ssd1306_send_command_list(ssd1306_t* restrict ssd, const uint8_t* restrict commands, size_t command_size);
void ssd1306_send_data(ssd1306_t* restrict ssd, const uint8_t* restrict data, size_t data_size);
void ssd1306_set_pixels(ssd1306_t* restrict ssd);

static inline void ssd1306_set_display_power(ssd1306_t* restrict ssd, bool power) 
{
    ssd1306_send_command(ssd, SSD1306_DISPLAYOFF | (uint8_t)power);
}

static inline void ssd1306_set_pause_display(ssd1306_t* restrict ssd, bool pause) 
{
    ssd1306_send_command(ssd, SSD1306_DISPLAYPAUSEOFF | (uint8_t)pause);
}

static inline void ssd1306_set_invert_colors(ssd1306_t* restrict ssd, bool invert) 
{
    ssd1306_send_command(ssd, SSD1306_NORMALDISPLAY | (uint8_t)invert);
}

static inline void ssd1306_set_memory_mode(ssd1306_t* restrict ssd, ssd1306_memory_mode_t mode) 
{
    const uint8_t message[] = {SSD1306_MEMORYMODE, (uint8_t)mode};
    ssd1306_send_command_list(ssd, message, 2);
}

static inline void ssd1306_set_vertical_flip(ssd1306_t* restrict ssd, bool flipped) 
{
    ssd1306_send_command(ssd, SSD1306_COMSCANNORMAL | ((uint8_t)flipped) * 0x8);
}

static inline void ssd1306_set_horizontal_flip(ssd1306_t* restrict ssd, bool flipped) 
{
    ssd1306_send_command(ssd, SSD1306_SEGNORMAL | (uint8_t)flipped);
}

static inline void ssd1306_set_full_rotation(ssd1306_t* restrict ssd, bool rotated) 
{
    ssd1306_set_vertical_flip(ssd, rotated);
    ssd1306_set_horizontal_flip(ssd, rotated);
}

static inline void ssd1306_buffer_set_pixels_direct(ssd1306_t* restrict ssd, const uint8_t *pixels) 
{
    memcpy(ssd->buffer, pixels, ssd->width * ssd->height / 8);
}

static inline void ssd1306_buffer_fill_pixels(ssd1306_t* restrict ssd, ssd1306_color_t color) 
{
    memset(ssd->buffer, (color == WHITE) ? 0xff : 0x00, ssd->width * ssd->height / 8);
}

static inline void ssd1306_pset(ssd1306_t* restrict p, uint32_t x, uint32_t y)
{
    if(x>=p->width || y>=p->height) return;
    p->buffer[x + p->width * (y >> 3)] |= 0x1 << (y & 0x07);
}

void ssd1306_print_char(ssd1306_t* restrict p, uint8_t x, uint8_t y, const uint8_t s, bool invert);
void ssd1306_print_string(ssd1306_t* restrict p, uint8_t x, uint8_t y, const char* restrict s, bool invert, bool vertical);
void ssd1306_log(ssd1306_t* restrict p, const char* restrict s, uint16_t ms, bool clr);
void ssd1306_line(ssd1306_t* restrict oled, uint8_t x, uint8_t y, uint8_t length, bool vertical);
void ssd1306_square(ssd1306_t* restrict oled, uint8_t x, uint8_t y, uint8_t width);
void ssd1306_corners(ssd1306_t* restrict oled, uint8_t x, uint8_t y, uint8_t width, uint8_t height);
void ssd1306_progress_bar(ssd1306_t* restrict oled, uint16_t value, uint16_t x, uint16_t y, uint16_t max, uint8_t length, uint8_t width, bool vertical);
void ssd1306_glyph(ssd1306_t* restrict oled, const bool* restrict data, uint8_t w, uint8_t h, uint8_t x, uint8_t y);
void ssd1306_xbm(ssd1306_t* restrict oled, const unsigned char* restrict data, uint8_t w, uint8_t h, uint8_t x, uint8_t y);
void ssd1306_progress_cv_bar(ssd1306_t* restrict oled, int8_t value, uint8_t x, uint8_t y, uint8_t max, uint8_t length, uint8_t width);

#endif
