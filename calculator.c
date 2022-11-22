// MIT License

// Copyright (c) 2022 unmanned

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

#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/binary_info.h"
#include "bsp/board.h"
#include "tusb.h"

#include "pico-ssd1306/ssd1306.h"
#include "pico/multicore.h"
#include "CD74HC4067.h"
#include "ShiftRegister74HC595-Pico/sr_common.h"
#include "interface.h"
#include "suspend.h"
#include "cell/automata.h"

#define SPI_PORT    spi0 
#define CLOCK_595   18
#define DATA_595    19
#define LATCH_595   16
ShiftRegister74HC595 sr;

#define SDA_PIN     2
#define SCL_PIN     3
#define RESET_PIN  -1
#define OLED_WIDTH  128
#define OLED_HEIGHT 64
ssd1306_t oled;

bool refresh_display = false;
bool repaint_display = false;
uint8_t selected_track = 0; // Displayed track
int8_t page = 0; // Current page
uint16_t cable_num = 0;

volatile absolute_time_t tts[_tracks]; // Trigger ON 
volatile absolute_time_t gts[_tracks]; // Gate OFF
volatile bool gate[_tracks]; // OFF is pending

void swith_led();
langtons_ant la;
////////////////////////////////////////////////////////////////////////////////////
// Core 1 interrupt Handler ////////////////////////////////////////////////////////
void core1_interrupt_handler() 
{
    while (multicore_fifo_rvalid())
    {
        uint16_t raw = multicore_fifo_pop_blocking();  
        if(refresh_display)
        {
            ssd1306_show(&oled);
            refresh_display = false;
        }
        else if(repaint_display)
        {
            if(page == 0)
            {
                char str[16];
                ssd1306_clear(&oled);

                sprintf(str, "BPM     : %d", esq.o[selected_track].bpm);
                ssd1306_print_string(&oled, 4,  0, str, 0, 0);
                sprintf(str, "STEPS   : %d", esq.o[selected_track].steps);
                ssd1306_print_string(&oled, 4, 10, str, 0, 0);
                sprintf(str, "CHANNEL : %d", esq.o[selected_track].channel);
                ssd1306_print_string(&oled, 4, 20, str, 0, 0);
                sprintf(str, "MODE    : %d", esq.o[selected_track].mode);
                ssd1306_print_string(&oled, 4, 30, str, 0, 0);
                sprintf(str, "FREERUN : %d", esq.o[selected_track].freerun);
                ssd1306_print_string(&oled, 4, 40, str, 0, 0);
                sprintf(str, "\x9A\x9C\x9A\x9C\x9A\x9C\x9A\x9C\x9A\x9C\x9A\x9C\x9A\x9C\x9A ");
                str[selected_track*2] = '\x9B';
                ssd1306_print_string(&oled, 4, 56, str, 0, 0);
                repaint_display = false;
                refresh_display = true;
            }
            else if(page == 1)
            {
                char str[16];
                ssd1306_clear(&oled);

                ssd1306_print_string(&oled, 4, 0, "DURATION", 0, 0);
                for(int i = 0; i < 16; i++)
                {
                    ssd1306_progress_bar(&oled, esq.o[selected_track].data[i].value, i*8 + 1, 10, 0xFF, 40, 6, true);
                }
                sprintf(str, "\x9A\x9C\x9A\x9C\x9A\x9C\x9A\x9C\x9A\x9C\x9A\x9C\x9A\x9C\x9A ");
                str[selected_track*2] = '\x9B';
                ssd1306_print_string(&oled, 4, 56, str, 0, 0);
                repaint_display = false;
                refresh_display = true;
            }
            else if(page == 2)
            {
                char str[16];
                char s[1];
                ssd1306_clear(&oled);

                ssd1306_print_string(&oled,  4,  0, "ROOT", 0, 0);
                ssd1306_print_string(&oled, 40,  0, chromatic[esq.o[selected_track].scale.root], 0, 0);
                const uint8_t xs[12] = {  4,  8, 14, 18, 24, 34, 38, 44, 48, 54, 58, 64};
                const uint8_t ys[12] = { 10,  0, 10,  0, 10, 10,  0, 10,  0, 10,  0, 10};
                for(int i = 0; i < 12; i++)
                {
                    if(esq.o[selected_track].scale.data & (0x800 >> i))
                    ssd1306_glyph(&oled, circle_glyph_f_8x8, 8, 8, xs[i] + 52, ys[i]);
                    else
                    ssd1306_glyph(&oled, circle_glyph_8x8, 8, 8, xs[i] + 52, ys[i]);

                    ssd1306_print_string(&oled,  6 + 10*i,  24, chromatic[esq.o[selected_track].data[i].chroma%12], 0, true);
                    sprintf(s, "%d", esq.o[selected_track].data[i].octave);
                    ssd1306_print_string(&oled,  6 + 10*i,  41, s, 0, true);

                }
                ssd1306_glyph(&oled, circle_glyph_r_8x8, 8, 8, xs[esq.o[selected_track].scale.root] + 52, ys[esq.o[selected_track].scale.root]);

                sprintf(str, "\x9A\x9C\x9A\x9C\x9A\x9C\x9A\x9C\x9A\x9C\x9A\x9C\x9A\x9C\x9A ");
                str[selected_track*2] = '\x9B';
                ssd1306_print_string(&oled, 4, 56, str, 0, 0);
                repaint_display = false;
                refresh_display = true;;
            }
        }
        if((raw & 0x80) == 0x80) // NOTE ON
        {
            uint16_t id = raw ^ 0x80;
            if(esq.o[id].data[esq.o[id].current].recount) 
            {
                note_from_degree(&esq.o[id].scale, &esq.o[id].data[esq.o->current]);
                esq.o[id].data[esq.o[id].current].recount = false;
            }
            _send_note_on(&esq.o[id]);
            uint8_t note_on[3] = { 0x80 | esq.o[id].channel, esq.o[id].data[esq.o->current].chroma, esq.o[id].data[esq.o->current].velocity };
            tud_midi_stream_write(cable_num, note_on, 3);
            // Log ///////////////////////////////////////////////////////////////////////////////////////////////
            if(page == -1)
            {
                char str[16];
                sprintf(str, "ON :%02X:%02X:%02X:%02X", esq.o[selected_track].current, note_on[0], note_on[1], note_on[2]);
                ssd1306_log(&oled, str, 0, 0);
            }

        }
        if((raw & 0x90) == 0x90) // NOTE OFF
        {
            uint16_t id = raw ^ 0x90;
            _send_note_off(&esq.o[id]);
            uint8_t note_off[3] = { 0x90 | esq.o[id].channel, esq.o[id].data[esq.o->current].chroma, 0 };
            tud_midi_stream_write(cable_num, note_off, 3);
            // Log ///////////////////////////////////////////////////////////////////////////////////////////////
            if(page == -1)
            {
                char str[16];
                sprintf(str, "OFF:%02X:%02X:%02X:%02X", esq.o[selected_track].current, note_off[0], note_off[1], note_off[2]);
                ssd1306_log(&oled, str, 0, 0);
            }
        }
        swith_led();
    }
    multicore_fifo_clear_irq(); // Clear interrupt
}

////////////////////////////////////////////////////////////////////////////////////
// Core 1 Main Code ////////////////////////////////////////////////////////////////
void core1_entry() 
{
    // Configure Core 1 Interrupt
    multicore_fifo_clear_irq();
    irq_set_exclusive_handler(SIO_IRQ_PROC1, core1_interrupt_handler);
    irq_set_enabled(SIO_IRQ_PROC1, true);
    // Infinte While Loop to wait for interrupt
    while (true)
    {
        tight_loop_contents();
    }
}



int main()
{
	stdio_init_all();
    board_init();
    tusb_init();
	multicore_launch_core1(core1_entry);
    int clk = set_sys_clock_khz(133000, true);

    ////////////////////////////////////////////////////////////////////////////////
    // OLED Init ///////////////////////////////////////////////////////////////////
    i2c_init(i2c1, 3200000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);
    oled.external_vcc=false;
    ssd1306_init(&oled, 128, 64, 0x3C, i2c1);
    ssd1306_contrast(&oled, 20);
    ssd1306_log(&oled, "CLOCK SET....", 0, true);
    // MIDI DIN Init //////////////////////////////////////////////////////////
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    ssd1306_log(&oled, "UART SET.....", 0, 0);
    // 595 Init ///////////////////////////////////////////////////////////////
    shift_register_74HC595_init(&sr, SPI_PORT, DATA_595, CLOCK_595, LATCH_595);
    gpio_set_outover(DATA_595, GPIO_OVERRIDE_INVERT); 
    _74HC595_set_all_low(&sr);
    // Sequencer Init /////////////////////////////////////////////////////////
    srand(time_us_32());
    sequencer_init(&esq, 120);
    sequencer_randomize(&esq, 0);
    // refresh_display = true;
    repaint_display = true;
    tts[0] = make_timeout_time_ms(300);
    
    la_init(&la, 4, 4);
    ssd1306_log(&oled, "ESQ SET......", 0, 0);
    ssd1306_log(&oled, "", 0, 0);
   
    ////////////////////////////////////////////////////////////////////////////////
    // CORE 0 Loop /////////////////////////////////////////////////////////////////
    RUN:
    repaint_display = true;
    page = 2;
    while (esq.state == PLAY) 
	{
        tud_task();
        // uint16_t raw = 0;
        for(int i = 0; i < _tracks; i++)
        {
            // NOTE ON Timings /////////////////////////////////////////////////////
            if(time_reached(tts[i]))
            {
                tts[i] = make_timeout_time_ms(esq.o[i].step + esq.o[i].data[esq.o[i].current].offset);
                loop_sequence[esq.o[i].mode](&esq.o[i]);
                if(esq.o[i].trigger[esq.o[i].current]) 
                {
                    multicore_fifo_push_blocking(i + 0x80);
                    gts[i] = make_timeout_time_ms(esq.o[i].atom * esq.o[i].data[esq.o[i].current].value);
                    gate[i] = true;
                }
                if(esq.o[0].current == 0) esq.o[0].regenerate[0] = true;
            }
            if(time_reached(gts[i]))
            {
                if(gate[i])
                {
                    multicore_fifo_push_blocking(i + 0x90);
                    gate[i] = false;
                }
            }
        }

        if(esq.o[0].regenerate[0])
        {
            la_move(&la);
            for(int i = 0; i < 16; i++) esq.o[0].trigger[i] = la.field[i];
            esq.o[0].regenerate[0] = false;
        }
        multicore_fifo_push_blocking(0);        
    }

    return 0;
}


void swith_led()
{
    static uint8_t led;
    _74HC595_set_all_low(&sr);
    if(esq.o[selected_track].trigger[led]) pset(&sr, led%4, led/4, 4);
    led++;
    if(led > _steps) 
    {
        led = 0;
        _74HC595_set_all_low(&sr);
        pset(&sr, esq.o[selected_track].current%4, esq.o[selected_track].current/4, 2);
    }
}