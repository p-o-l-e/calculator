#include "ssd1306.h"
#include <assert.h>

uint32_t ssd1306_init(ssd1306_t* restrict ssd, uint16_t addr, i2c_inst_t* restrict i2c, ssd1306_color_t color) {
    ssd->addr = addr;
    ssd->i2c = i2c;
    ssd->width = 128;
    ssd->height = 64;
    ssd->buffer = malloc(ssd->width * ssd->height);

    uint8_t cmds[] = 
    {
        SSD1306_DISPLAYOFF,
        SSD1306_DISPLAYCLOCKDIV,
        SSD1306_CLOCKDIVRESET,
        SSD1306_MULTIPLEX,        ssd->height - 1,
        SSD1306_DISPLAYOFFSET,    0x00,
        SSD1306_STARTLINE,
        SSD1306_CHARGEPUMP,       0x14,
        SSD1306_SEGREMAP        | 0x01,
        SSD1306_COMSCANNORMAL   | 0x08,
        SSD1306_COMPINS,          ssd->width > 2 * ssd->height ? 0x02 : 0x12,
        SSD1306_CONTRAST,         0x20,
        SSD1306_PRECHARGE,        0xF1,
        SSD1306_VCOMDESELECT,     0x30,
        SSD1306_DISPLAYPAUSEOFF,
        SSD1306_NORMALDISPLAY,                  
        SSD1306_DISPLAYOFF      | 0x01,
        SSD1306_MEMORYMODE,       0x00,
    };

    ssd1306_send_command_list(ssd, cmds, sizeof(cmds));
    ssd1306_buffer_fill_pixels(ssd, color);
    ssd1306_set_pixels(ssd);
}


void ssd1306_set_pixels(ssd1306_t* restrict ssd) 
{
    assert(ssd != NULL);
    const size_t BUFFER_SIZE = ssd->width * ssd->height / 8;
    const uint8_t init[] = {
        SSD1306_PAGEADDR,
        SSD1306_PAGEADDRSTART,
        SSD1306_PAGEADDREND,
        SSD1306_COLUMNADDR,
        SSD1306_COLUMNADDRSTART,
        ssd->width - 1,
    };
    ssd1306_send_command_list(ssd, init, sizeof(init));
    ssd1306_send_data(ssd, ssd->buffer, BUFFER_SIZE);
}

void ssd1306_send_command_list(ssd1306_t* restrict ssd, const uint8_t* restrict commands, size_t command_size) 
{
    assert(ssd != NULL && commands != NULL && command_size > 0);
    uint8_t command_buffer[command_size + 1];
    command_buffer[0] = SSD1306_CONTROL_COMMAND;
    memcpy(command_buffer + 1, commands, command_size);
    i2c_write_blocking(ssd->i2c, ssd->addr, command_buffer, command_size + 1, false);
}

void ssd1306_send_data(ssd1306_t* restrict ssd, const uint8_t* restrict data, size_t data_size) 
{
    uint8_t pixel_buffer[data_size + 1];
    memcpy(pixel_buffer + 1, ssd->buffer, data_size);
    pixel_buffer[0] = SSD1306_CONTROL_DATA;
    i2c_write_blocking(ssd->i2c, ssd->addr, pixel_buffer, data_size + 1, false);
}



void ssd1306_print_char(ssd1306_t* restrict p, uint8_t x, uint8_t y, const uint8_t s, bool invert)
{
    for(int i = 0; i < 8; ++i) // X
    {
        uint8_t l = gtFont[(s-32)*8 + i];
        for(int j = 0; j < 8; ++j) // Y
        {
            if((l&0b1)!=invert) ssd1306_pset(p, x + i, y + j);
            l>>=1;
        }
    }
}

void ssd1306_print_string(ssd1306_t* restrict p, uint8_t x, uint8_t y, const char* restrict s, bool invert, bool vertical)
{
    if(vertical) for(int i = 0; i < strlen(s); ++i) ssd1306_print_char(p, x , y + 8*i, s[i], invert);
    else for(int i = 0; i < strlen(s); ++i) ssd1306_print_char(p, x + 8*i, y, s[i], invert);
}

void ssd1306_log(ssd1306_t* restrict p, const char* restrict s, uint16_t ms, bool clr)
{
    static char _log_[8][16];
    if(clr) for(int i = 0; i < 8; ++i) strcpy(_log_[i], "               ");
    ssd1306_buffer_fill_pixels(p, BLACK);
    for(int i = 0; i < 7; ++i)    
    {
        strcpy(_log_[i], _log_[i+1]);
        ssd1306_print_string(p, 0, i*8, _log_[i], 0, 0);
    }
    strcpy(_log_[7], "               ");
    strcpy(_log_[7], s);
    ssd1306_print_string(p, 0, 7*8, s, 0, 0);
    ssd1306_set_pixels(p);
    sleep_ms(ms);
}

void ssd1306_line(ssd1306_t* restrict oled, uint8_t x, uint8_t y, uint8_t length, bool vertical)
{
    if(vertical) for(int i = y; i < length + y; ++i) ssd1306_pset(oled, x, i);
    else for(int i = x; i < length + x; i++) ssd1306_pset(oled, i, y);
}

void ssd1306_progress_bar(ssd1306_t* restrict oled, uint16_t value, uint16_t x, uint16_t y, uint16_t max, uint8_t length, uint8_t width, bool vertical)
{
    if(vertical)
    {
        uint16_t v = roundf((float)(value * length) / (float)max);
        for(int i = (y + length); i > (y + length - v); i-=2 ) ssd1306_line(oled, x, i, width, false);
    }
}

void ssd1306_progress_cv_bar(ssd1306_t* restrict oled, int8_t value, uint8_t x, uint8_t y, uint8_t max, uint8_t length, uint8_t width)
{
    int8_t v = roundf((float)(value * length) / (float)max);
    if(v < 0) for(int i = (y + length/2); i < (y + length/2 - v); i+=2 ) ssd1306_line(oled, x, i, width, false);
    else for(int i = (y + length/2); i > (y + length/2 - v); i-=2 ) ssd1306_line(oled, x, i, width, false);
}

void ssd1306_glyph(ssd1306_t* restrict oled, const bool* restrict data, uint8_t w, uint8_t h, uint8_t x, uint8_t y)
{
    for(int i = 0; i < h; ++i)
        for(int j = 0; j < w; ++j)
            if(data[j + w * i]) ssd1306_pset(oled, x + j, y + i);
}