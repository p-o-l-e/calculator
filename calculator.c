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
#include "pico/util/queue.h"
#include "CD74HC4067.h"
#include "ShiftRegister74HC595-Pico/sr_common.h"
#include "interface.h"
#include "suspend.h"
#include "cell/automata.h"

// #define _MULTI_THREADED
// #include <pthread.h>
// pthread_t _thread;
queue_t queue;


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
void arm(uint32_t lag);
void send(uint8_t id, uint8_t status);
ant la;
element el;
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
            char str[16];
            switch (page)
            {
            case 0:
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
                break;
            
            case 1:
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
                break;

            case 2:
                char s[1];
                ssd1306_clear(&oled);

                ssd1306_print_string(&oled,  4,  0, "ROOT", 0, 0);
                ssd1306_print_string(&oled, 40,  0, chromatic_lr[esq.o[selected_track].scale.root], 0, 0);
                const uint8_t xs[12] = { 56, 60, 66, 70, 76, 86, 90, 96,100,106,110,116};
                const uint8_t ys[12] = { 10,  0, 10,  0, 10, 10,  0, 10,  0, 10,  0, 10};
                for(int i = 0; i < 12; i++)
                {
                    if(esq.o[selected_track].scale.data & (0x800 >> i))
                    ssd1306_glyph(&oled, circle_glyph_8x8f, 8, 8, xs[i], ys[i]);
                    else
                    ssd1306_glyph(&oled, circle_glyph_8x8h, 8, 8, xs[i], ys[i]);

                    ssd1306_print_string(&oled,  6 + 10*i,  24, chromatic[esq.o[selected_track].data[i].chroma%12], 0, true);
                    sprintf(s, "%d", esq.o[selected_track].data[i].octave);
                    ssd1306_print_string(&oled,  6 + 10*i,  41, s, 0, true);

                }
                ssd1306_glyph(&oled, circle_glyph_8x8r, 8, 8, xs[esq.o[selected_track].scale.root], ys[esq.o[selected_track].scale.root]);

                sprintf(str, "\x9A\x9C\x9A\x9C\x9A\x9C\x9A\x9C\x9A\x9C\x9A\x9C\x9A\x9C\x9A ");
                str[selected_track*2] = '\x9B';
                ssd1306_print_string(&oled, 4, 56, str, 0, 0);
                repaint_display = false;
                refresh_display = true;
                break;

            default:
                break;
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
    set_sys_clock_khz(133000, true);
    ////////////////////////////////////////////////////////////////////////////////
    // OLED Init ///////////////////////////////////////////////////////////////////
    i2c_init(i2c1, 3200000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);
    oled.external_vcc = false;
    ssd1306_init(&oled, 128, 64, 0x3C, i2c1);
    ssd1306_contrast(&oled, 20);
    // MIDI DIN Init //////////////////////////////////////////////////////////
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    // 595 Init ///////////////////////////////////////////////////////////////
    shift_register_74HC595_init(&sr, SPI_PORT, DATA_595, CLOCK_595, LATCH_595);
    gpio_set_outover(DATA_595, GPIO_OVERRIDE_INVERT); 
    _74HC595_set_all_low(&sr);
    ///////////////////////////////////////////////////////////////////////////
    // Sequencer Init /////////////////////////////////////////////////////////
    srand(time_us_32());
    sequencer_init(&esq, 40);
    sequencer_randomize(&esq, 0);
    ant_init(&la);
    element_init(&el);
    queue_init(&queue, sizeof(uint32_t), 32);
    repaint_display = true;
    page = -1;
    uint32_t rv[_tracks]; // Revolution counters
    ////////////////////////////////////////////////////////////////////////////////
    // CORE 0 Loop /////////////////////////////////////////////////////////////////
    RUN:
    arm(100);
    while (esq.state == PLAY) 
	{
        tud_task();
        for(int i = 0; i < _tracks; i++)
        {
            if(esq.o[i].data[esq.o[i].current].recount) 
            {
                note_from_degree(&esq.o[i].scale, &esq.o[i].data[esq.o->current]);
                esq.o[i].data[esq.o[i].current].recount = false;
            }
            ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // NOTE ON Timings ////////////////////////////////////////////////////////////////////////////////////////////////////////////
            if(time_reached(tts[i]))
            {
                loop_sequence[esq.o[i].mode](&esq.o[i]); // Loop to next step
                tts[i] = make_timeout_time_ms(esq.o[i].step + esq.o[i].data[esq.o[i].current].offset); // Next step timer
                if(esq.o[i].trigger[esq.o[i].current])  // If current step is trigger
                {
                    if(gate[i]) // Trigger OFF previous note to prevent overlap
                    {
                        send(i, 0x80);
                        gate[i] = false;
                    }
                    send(i, 0x90);
                    gts[i] = make_timeout_time_ms(esq.o[i].atom * esq.o[i].data[esq.o[i].current].value); // Create a NOTE OFF timer
                    gate[i] = true;
                }
            }
            if(time_reached(gts[i]))
            {
                if(gate[i])
                {
                    send(i, 0x80);
                    gate[i] = false;
                }
            }
        }

        if(rv[0] != esq.o[0].revolutions)
        {
            ant_evolve(&la);
            // evolve(&el);
            for(int i = 0; i < 16; i++) esq.o[0].trigger[i] = la.field[i];
            // el.rule = 86;
            // for(int i = 0; i < 4; i++) 
            // {
            //     for(int j = 0; j < 4; j++)
            //     esq.o[0].trigger[i + (4*j)] = el.field[i][j];
            // }
            esq.o[0].regenerate[0] = false;
            rv[0] = esq.o[0].revolutions;
        }
        multicore_fifo_push_blocking(0);        
    }
    return 0;
}


void send(uint8_t id, uint8_t status) // Send MIDI message
{
    switch (status)
    {
    case 0x80:
        _send_note_off(&esq.o[id]);
        uint8_t off[3] = { status|id, esq.o[id].data[esq.o[id].current].chroma, 0 };
        tud_midi_stream_write(cable_num, off, 3);
        break;
    case 0x90:
        _send_note_on(&esq.o[id]);
        uint8_t mm[3] = { status|id, esq.o[id].data[esq.o[id].current].chroma, esq.o[id].data[esq.o[id].current].velocity };
        tud_midi_stream_write(cable_num, mm, 3);
        break;
    default:
        break;
    }
    if(page == -1)
    {
        char str[16];
        if(status == 0x80)
        sprintf(str, "ON :%2X:%2X:%2X", status|id, esq.o[id].data[esq.o[id].current].chroma, esq.o[id].data[esq.o[id].current].velocity );
        if(status == 0x90)
        sprintf(str, "OFF:%2X:%2X:%2X", status|id, esq.o[id].data[esq.o[id].current].chroma, esq.o[id].data[esq.o[id].current].velocity );
        ssd1306_log(&oled, str, 0, 0);
    }
}

void arm(uint32_t lag) // Prepare to play routine
{
    int8_t f = esq.o[0].data[esq.o[0].current].offset;
    for(int i = 1; i < _tracks; i++)
    {
        if(f > esq.o[i].data[esq.o[i].current].offset)
        {
            f = esq.o[i].data[esq.o[i].current].offset;
        }
    }
    for(int i = 0; i < _tracks; i++)
    {
        tts[i] = make_timeout_time_ms(esq.o[i].data[esq.o[i].current].offset - f + lag);
    }
}

void swith_led()
{
    static uint8_t led;
    _74HC595_set_all_low(&sr);
    if(esq.o[selected_track].trigger[led]) pset(&sr, led&3, led>>2, 4);
    led++;
    if(led > _steps) 
    {
        led = 0;
        _74HC595_set_all_low(&sr);
        pset(&sr, esq.o[selected_track].current&3, esq.o[selected_track].current>>2, 2);
    }
}