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
uint8_t selected_track = 0; // Blinking track
uint8_t page = 0;
uint16_t cable_num = 0;
////////////////////////////////////////////////////////////////////////////////////
// Core 1 interrupt Handler ////////////////////////////////////////////////////////
void core1_interrupt_handler() 
{
    int led = 0;
    uint32_t tt = 0;
    while (multicore_fifo_rvalid())
    {
        uint16_t raw = multicore_fifo_pop_blocking();  
        if(refresh_display)
        {
            if(page == 0)
            {
                char str[16];
                ssd1306_clear(&oled);

                sprintf(str, "BPM     : %d", esq.o[selected_track].bpm);
                ssd1306_print_string(&oled, 4,  0, str, 0);
                sprintf(str, "STEPS   : %d", esq.o[selected_track].steps);
                ssd1306_print_string(&oled, 4, 10, str, 0);
                sprintf(str, "CHANNEL : %d", esq.o[selected_track].channel);
                ssd1306_print_string(&oled, 4, 20, str, 0);
                sprintf(str, "MODE    : %d", esq.o[selected_track].mode);
                ssd1306_print_string(&oled, 4, 30, str, 0);
                sprintf(str, "FREERUN : %d", esq.o[selected_track].freerun);
                ssd1306_print_string(&oled, 4, 40, str, 0);
                sprintf(str, "\x9A\x9C\x9A\x9C\x9A\x9C\x9A\x9C\x9A\x9C\x9A\x9C\x9A\x9C\x9A ");
                str[selected_track*2] = '\x9B';
                ssd1306_print_string(&oled, 4, 56, str, 0);

                ssd1306_show(&oled);
            }
            refresh_display = false;
        }
        if(raw == 0)
        {
            // SEND MIDI
            // _send_note_on(&esq.o[raw]);
            // uint8_t note_on[3] = { 0x80 | esq.o[raw].channel, esq.o[raw].data->pitch, esq.o[raw].data->velocity };
            // tud_midi_stream_write(cable_num, note_on, 3);
            // Log ///////////////////////////////////////////////////////////////////////////////////////////////
            // char str[16];
            // sprintf(str, "Step: %d", esq.o[selected_track].current);
            // ssd1306_log(&oled, str, 0, 0);

        }


        _74HC595_set_all_low(&sr);
        if(esq.o[selected_track].trigger[led]) pset(&sr, led%4, led/4, 2);
        led++;
        if(led > _steps) 
        {
            led = 0;
            _74HC595_set_all_low(&sr);
            pset(&sr, esq.o[selected_track].current%4, esq.o[selected_track].current/4, 4);
        }
    
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
    i2c_init(i2c1, 400000);
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
    sequencer_init(&esq, 30);
    refresh_display = true;
    absolute_time_t tts[_tracks]; // Trigger ON 
    absolute_time_t gts[_tracks]; // Gate OFF
    tts[0] = make_timeout_time_ms(300);

    esq.o[0].trigger[0] = 1;
    esq.o[0].trigger[2] = 1;
    esq.o[0].trigger[4] = 1;
    esq.o[0].trigger[6] = 1;
    esq.o[0].trigger[8] = 1;
    esq.o[0].trigger[10] = 1;
    esq.o[0].trigger[12] = 1;
    esq.o[0].trigger[14] = 1;

    ssd1306_log(&oled, "ESQ SET......", 0, 0);
    ssd1306_log(&oled, "", 0, 0);
   
    ////////////////////////////////////////////////////////////////////////////////
    // CORE 0 Loop /////////////////////////////////////////////////////////////////
    RUN:
    refresh_display = true;
    while (esq.state == PLAY) 
	{
        uint16_t raw = -1;
        for(int i = 0; i < _tracks; i++)
        {
            // NOTE ON Timings /////////////////////////////////////////////////////
            if(time_reached(tts[i]))
            {
                tts[i] = make_timeout_time_ms(esq.o[i].step + esq.o[i].data[esq.o[i].current].offset);
                loop_sequence[esq.o[i].mode](&esq.o[i]);
                raw = esq.o[i].trigger[esq.o[i].current] ? i : -1;
            }
        }
        multicore_fifo_push_blocking(raw);
    }

    return 0;
}