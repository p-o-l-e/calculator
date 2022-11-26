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

#include "pico/multicore.h"
#include "interface.h"
#include "suspend.h"
#include "io.h"

bool refresh_display = false;
bool repaint_display = true;
int8_t selected_track = 0; // Displayed track
int8_t page = 0; // Current page
uint16_t cable_num = 0;

bool hold[16];
bool hold_matrix[16];

volatile absolute_time_t tts[_tracks]; // Trigger ON 
volatile absolute_time_t gts[_tracks]; // Gate OFF
volatile bool gate[_tracks]; // OFF is pending

void swith_led();
void arm(uint32_t lag);
void send(uint8_t id, uint8_t status);

int32_t prior = 0;
float cap = 0.0f;
////////////////////////////////////////////////////////////////////////////////////
// Core 1 interrupt Handler ////////////////////////////////////////////////////////
void core1_interrupt_handler() 
{
    while (multicore_fifo_rvalid())
    {
        uint16_t raw = multicore_fifo_pop_blocking();  
        _4067_iterator++;
        if(_4067_iterator >= N4067) _4067_iterator = 0;
        _4067_switch(_4067_iterator, 0);
        uint8_t ccol = keypad_switch();
        int32_t current = get_count(&ncoder, ncoder_index);
        if(current!=prior)
        {
            int f = -1;
            for(int i = 0; i < 16; i++)
            {
                if(hold_matrix[i])
                {
                    f = i;
                    break;
                }
            }
            if(f >= 0) 
            {
                cap += ((float)(prior - current)*0.33f);
                switch (page)
                {
                    case PAGE_DRTN: 
                        esq.o[selected_track].data[f].value  += ((prior > current) ? -1 : 1); 
                        if(esq.o[selected_track].data[f].value > 0xFF) esq.o[selected_track].data[f].value = 0xFF;
                        else if(esq.o[selected_track].data[f].value < 1) esq.o[selected_track].data[f].value = 1;
                        break;

                    case PAGE_VELO: 
                        esq.o[selected_track].data[f].velocity  += ((prior > current) ? -1 : 1); 
                        if(esq.o[selected_track].data[f].velocity > 0x7F) esq.o[selected_track].data[f].velocity = 0x7F;
                        else if(esq.o[selected_track].data[f].velocity < 1) esq.o[selected_track].data[f].velocity = 1;
                        break;

                    case PAGE_FFST: 
                        esq.o[selected_track].data[f].offset  += ((prior > current) ? -1 : 1); 
                        if(esq.o[selected_track].data[f].offset > 0x7F) esq.o[selected_track].data[f].offset = 0x7F;
                        else if(esq.o[selected_track].data[f].offset < -0x7F) esq.o[selected_track].data[f].offset = -0x7F;
                        break;

                    case PAGE_NOTE: 
                        if(fabsf(cap)>1.0f)
                        {
                            esq.o[selected_track].data[f].degree -= cap; 
                            if(esq.o[selected_track].data[f].degree > (esq.o[selected_track].scale.width - 1)) 
                            esq.o[selected_track].data[f].degree = esq.o[selected_track].scale.width - 1;
                            else if(esq.o[selected_track].data[f].degree <  0) esq.o[selected_track].data[f].degree = 0;
                            cap = 0.0f;
                        }
                        break;
                    default: 
                        break;
                }
                note_from_degree(&esq.o[selected_track].scale, &esq.o[selected_track].data[f]);
                repaint_display = true;
            }

            prior = current;
        }

        if(refresh_display)
        {
            ssd1306_set_pixels(&oled);
            refresh_display = false;
        }        
        else if(repaint_display)
        {
            char str[16];
            switch (page)
            {
            case PAGE_MAIN:
                ssd1306_buffer_fill_pixels(&oled, BLACK);

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
            
            case PAGE_DRTN:
                ssd1306_buffer_fill_pixels(&oled, BLACK);

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

            case PAGE_AUTO:
                ssd1306_buffer_fill_pixels(&oled, BLACK);

                ssd1306_print_string(&oled, 4, 0, "AUTOMATA", 0, 0);
                for(int i = 0; i < 8; i++)
                {
                    switch (esq.ant[selected_track].rule[i])
                    {
                        case 0: ssd1306_glyph(&oled, circle_u_8x8, 8, 8, 4 + 16*i, 10); break;
                        case 1: ssd1306_glyph(&oled, circle_r_8x8, 8, 8, 4 + 16*i, 10); break;
                        case 2: ssd1306_glyph(&oled, circle_d_8x8, 8, 8, 4 + 16*i, 10); break;
                        case 3: ssd1306_glyph(&oled, circle_l_8x8, 8, 8, 4 + 16*i, 10); break;
                        default: break;
                    }
                }
                for(int i = 0; i < 4; i++)
                {
                    if(((esq.ant[selected_track].rule[8 + i])>>1)&1) ssd1306_print_string(&oled, 4 + 32*i, 20, "Y", 0, 0);
                    else ssd1306_print_string(&oled, 4 + 32*i, 20, "X", 0, 0);
                    if(((esq.ant[selected_track].rule[8 + i]))&1) ssd1306_print_string(&oled, 20 + 32*i, 20, "Y", 0, 0);
                    else ssd1306_print_string(&oled, 20 + 32*i, 20, "X", 0, 0);
                }
                for(int i = 0; i < 8; i++)
                {
                    if(esq.ant[selected_track].step[i] > 0)
                    ssd1306_print_char(&oled, 4 + 16*i, 30, 0xA3 + esq.ant[selected_track].step[i], 0);
                    else
                    ssd1306_print_char(&oled, 4 + 16*i, 30, 0xA3 + esq.ant[selected_track].step[i], 0);

                }
                // ssd1306_print_string(&oled, 56, 8, str, 0, 0);
                sprintf(str, "\x9A\x9C\x9A\x9C\x9A\x9C\x9A\x9C\x9A\x9C\x9A\x9C\x9A\x9C\x9A ");
                str[selected_track*2] = '\x9B';
                ssd1306_print_string(&oled, 4, 56, str, 0, 0);
                repaint_display = false;
                refresh_display = true;
                break;


            case PAGE_VELO:
                ssd1306_buffer_fill_pixels(&oled, BLACK);

                ssd1306_print_string(&oled, 4, 0, "VELOCITY", 0, 0);
                for(int i = 0; i < 16; i++)
                {
                    ssd1306_progress_bar(&oled, esq.o[selected_track].data[i].velocity, i*8 + 1, 10, 0x7F, 40, 6, true);
                }
                sprintf(str, "\x9A\x9C\x9A\x9C\x9A\x9C\x9A\x9C\x9A\x9C\x9A\x9C\x9A\x9C\x9A ");
                str[selected_track*2] = '\x9B';
                ssd1306_print_string(&oled, 4, 56, str, 0, 0);
                repaint_display = false;
                refresh_display = true;
                break;

            case PAGE_FFST:
                ssd1306_buffer_fill_pixels(&oled, BLACK);

                ssd1306_print_string(&oled, 4, 0, "OFFSET", 0, 0);
                for(int i = 0; i < 16; i++)
                {
                    ssd1306_progress_cv_bar(&oled, esq.o[selected_track].data[i].offset, i*8 + 1, 10, 0xFF, 40, 6);
                }
                sprintf(str, "\x9A\x9C\x9A\x9C\x9A\x9C\x9A\x9C\x9A\x9C\x9A\x9C\x9A\x9C\x9A ");
                str[selected_track*2] = '\x9B';
                ssd1306_print_string(&oled, 4, 56, str, 0, 0);
                repaint_display = false;
                refresh_display = true;
                break;

            case PAGE_NOTE:
                char s[1];
                ssd1306_buffer_fill_pixels(&oled, BLACK);

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

        if(_4067_get())
        {
            switch (_4067_iterator)
            {
                case ENCDR:
                    hold[ENCDR] = true;
                    break;
                case ALTGR:
                    hold[ALTGR] = true;
                    break;
                case PGLFT: 
                    if(!hold[PGLFT]) 
                    { 
                        if(hold[ALTGR])
                        {
                            selected_track--;
                            if(selected_track < 0) selected_track = _tracks - 1;
                        }
                        else
                        {
                            page--; 
                        }
                        repaint_display = true; 
                        hold[PGLFT] = true;
                    } 
                    break;
                case PGRGT: 
                    if(!hold[PGRGT]) 
                    { 
                        if(hold[ALTGR])
                        {
                            selected_track++;
                            if(selected_track >= _tracks) selected_track = 0;
                        }
                        else
                        {
                            page++;
                        }
                        repaint_display = true; 
                        hold[PGRGT] = true;
                    } 
                    break;
                case MROW0: 
                    if(!hold_matrix[ccol]) 
                    { 
                        esq.o[selected_track].trigger[ccol] = !esq.o[selected_track].trigger[ccol]; 
                        hold_matrix[ccol] = true; 
                    } 
                    break;
                case MROW1: 
                    if(!hold_matrix[ccol + 4]) 
                    { 
                        esq.o[selected_track].trigger[ccol + 4] = !esq.o[selected_track].trigger[ccol + 4]; 
                        hold_matrix[ccol + 4] = true; 
                    } 
                    break;
                case MROW2: 
                    if(!hold_matrix[ccol + 8]) 
                    { 
                        esq.o[selected_track].trigger[ccol + 8] = !esq.o[selected_track].trigger[ccol + 8]; 
                        hold_matrix[ccol + 8] = true; 
                    } 
                    break;
                case MROW3: 
                    if(!hold_matrix[ccol + 12]) 
                    { 
                        esq.o[selected_track].trigger[ccol + 12] = !esq.o[selected_track].trigger[ccol + 12]; 
                        hold_matrix[ccol + 12] = true; 
                    } 
                    break;
                default: break;
            }
            if(page > PAGES) page = 0;
            else if(page < 0) page = PAGES - 1;
        }
        else 
        {
            hold[_4067_iterator] = false;
            if(_4067_iterator < 4) hold_matrix[_4067_iterator * 4 + ccol] = false;
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
    set_sys_clock_khz(133000, true);
    ////////////////////////////////////////////////////////////////////////////////
    // OLED Init ///////////////////////////////////////////////////////////////////
    i2c_init(i2c1, 3200000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);
    ssd1306_init(&oled, 0x3C, i2c1, BLACK);
    ssd1306_set_full_rotation(&oled, 0);
    // MIDI DIN Init //////////////////////////////////////////////////////////
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    // 595 Init ///////////////////////////////////////////////////////////////
    shift_register_74HC595_init(&sr, SPI_PORT, DATA_595, CLOCK_595, LATCH_595);
    gpio_set_outover(DATA_595, GPIO_OVERRIDE_INVERT); 
    _74HC595_set_all_low(&sr);
    // 4067 Init //////////////////////////////////////////////////////////////
    _4067_init();
    keypad_init();
    ncoder_index = quad_encoder_init();
    ///////////////////////////////////////////////////////////////////////////
    // Sequencer Init /////////////////////////////////////////////////////////
    srand(time_us_32());
    sequencer_init(&esq, 40);
    for(int i = 0; i < _tracks; i++) sequencer_randomize(&esq, i);
    repaint_display = true;
    page = 0;
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
            // AUTOMATA //////////////////////////////////////////////////////////////
            if(rv[i] != esq.o[i].revolutions)
            {
                automata_evolve(&esq.ant[i]);
                for(int j = 0; j < 16; j++) esq.o[i].trigger[j] = esq.ant[i].field[j];
                esq.o[i].regenerate[i] = false;
                rv[i] = esq.o[i].revolutions;
            }
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
    if(page == PAGE_LOG_)
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