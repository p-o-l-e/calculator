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


inline static void swith_led(const int* track);
inline static void scale_led(const int* track);
inline static void arm(uint lag, absolute_time_t* t);
inline static void send(uint8_t id, const uint8_t status);
static CD74HC595 sr;
static ssd1306_t oled;
static sequencer esq;
////////////////////////////////////////////////////////////////////////////////////
// Core 1 interrupt Handler ////////////////////////////////////////////////////////
void core1_interrupt_handler() 
{
    quadrature_decoder ncoder;
    int ncoder_index;
    ncoder_index = quad_encoder_init(&ncoder);
    
    bool hold[16];
    bool hold_matrix[16];
    bool refresh = false;
    bool repaint = true;
    bool tap_armed = false;
    bool save = true;
    int tap = 0;
    int prior = 0;    // Last encoder value
    int last = 0;     // Last action
    int sreg = 4;     // Shift Register iterator
    int line = 0;     // Selected display line
    int selected = 0; // Displayed track
    int page = 0;     // Current page

    int cap = 0;
    const char* lsel[] = {" ", "\x80"};
    const char* mode[] = {"FWD", "BWD", "PNG", "RND"};
    const int xs[12] = { 56, 60, 66, 70, 76, 86, 90, 96, 100, 106, 110, 116};
    const int ys[12] = { 10,  0, 10,  0, 10, 10,  0, 10,   0,  10,   0,  10};
    const char* chromatic[]    = {" C","#C"," D","#D"," E"," F","#F"," G","#G"," A","#A"," B"};
    const char* chromatic_lr[] = {"C ","C#","D ","D#","E ","F ","F#","G ","G#","A ","A#","B "};


    while (multicore_fifo_rvalid())
    {
        uint16_t raw = multicore_fifo_pop_blocking();  
        if(++sreg >= N4067) sreg = 0;
        _4067_switch(sreg, 0);
        int ccol = keypad_switch();
        // ENCODER /////////////////////////////////////////////////////////////////////////////////////////
        int current = get_count(&ncoder, ncoder_index);
        if(current!=prior)
        {
            int f = -1;
            cap += ((prior - current)*300);
            for(int i = 0; i < 16; ++i)
            {
                if(hold_matrix[i])
                {
                    f = i;
                    break;
                }
            }
            if(f >= 0) 
            {
                switch (page)
                {
                    case PAGE_DRTN: 
                        if(hold[ENCDR]) esq.o[selected].data[f].value += ((prior > current) ? -3 : 3);
                        else esq.o[selected].data[f].value += ((prior > current) ? -1 : 1);
                        if(esq.o[selected].data[f].value > 0xFF) esq.o[selected].data[f].value = 0xFF;
                        else if(esq.o[selected].data[f].value < 1) esq.o[selected].data[f].value = 1;
                        break;

                    case PAGE_VELO: 
                        if(hold[ENCDR]) esq.o[selected].data[f].velocity  += ((prior > current) ? -3 : 3);
                        else esq.o[selected].data[f].velocity  += ((prior > current) ? -1 : 1);
                        if(esq.o[selected].data[f].velocity > 0x7F) esq.o[selected].data[f].velocity = 0x7F;
                        else if(esq.o[selected].data[f].velocity < 1) esq.o[selected].data[f].velocity = 1;
                        break;

                    case PAGE_FFST: 
                        if(hold[ENCDR]) esq.o[selected].data[f].offset  += ((prior > current) ? -3 : 3);
                        else esq.o[selected].data[f].offset  += ((prior > current) ? -1 : 1);
                        if(esq.o[selected].data[f].offset > 0x7F) esq.o[selected].data[f].offset = 0x7F;
                        else if(esq.o[selected].data[f].offset < -0x7F) esq.o[selected].data[f].offset = -0x7F;
                        break;

                    case PAGE_NOTE: 
                        if(abs(cap)>1000)
                        {
                            if(hold[ENCDR]) 
                            {
                                esq.o[selected].data[f].octave -= ((cap > 0)? 1:-1);
                                if(esq.o[selected].data[f].octave > 9) esq.o[selected].data[f].octave = 9;
                                else if(esq.o[selected].data[f].octave < 0) esq.o[selected].data[f].octave = 0;
                            }
                            else 
                            {
                                esq.o[selected].data[f].degree -= ((cap > 0)? 1:-1);
                                if(esq.o[selected].data[f].degree > (esq.o[selected].scale.width - 1)) 
                                esq.o[selected].data[f].degree = esq.o[selected].scale.width - 1;
                                else if(esq.o[selected].data[f].degree <  0) esq.o[selected].data[f].degree = 0;
                            }
                            cap = 0;
                            esq.o[selected].data[f].recount = true;
                        }
                        break;
                    default: 
                        break;
                }
                note_from_degree(&esq.o[selected].scale, &esq.o[selected].data[f]);
                repaint = true;
            }

            else if(page == PAGE_MAIN)
            {
                switch(line)
                {
                    case 0: 
                        if(abs(cap)>1000)
                        {
                            int bpm = esq.o[selected].bpm - ((cap > 0)? 1:-1);
                            if(bpm > 800) bpm = 800;
                            else if(bpm < 1) bpm = 1;
                            reset_timestamp(&esq, selected, bpm);
                            cap = 0;
                        }
                        break;

                    case 1: 
                        if(abs(cap)>1000)
                        {
                            esq.o[selected].steps  -= ((cap > 0)? 1:-1);
                            if(esq.o[selected].steps > 16) esq.o[selected].steps = 16;
                            else if(esq.o[selected].steps < 2) esq.o[selected].steps = 2;
                            cap = 0;
                        }
                        break;

                    case 2:
                        if(abs(cap)>1000)
                        {
                            esq.o[selected].mode -= ((cap > 0)? 1:-1);
                            if(esq.o[selected].mode > 3) esq.o[selected].mode = 3;
                            else if(esq.o[selected].mode < 0) esq.o[selected].mode = 0;
                            cap = 0;
                        }       
                        break;         
                    default:
                        break;
                }
                repaint = true;
            }

            else if(page == PAGE_DRFT)
            {
                switch(line)
                {
                    case 0: 
                        if(abs(cap)>1000)
                        {
                            esq.o[selected].drift[0] -= ((cap > 0)? 1:-1);
                            if(esq.o[selected].drift[0] > 0x40) esq.o[selected].drift[0] = 0x40;
                            else if(esq.o[selected].drift[0] < 0) esq.o[selected].drift[0] = 0;
                            cap = 0;
                        }
                        break;
                    case 1:
                        if(abs(cap)>1000)
                        {
                            esq.o[selected].drift[1] -= ((cap > 0)? 1:-1);
                            if(esq.o[selected].drift[1] > 0x40) esq.o[selected].drift[1] = 0x40;
                            else if(esq.o[selected].drift[1] < 0) esq.o[selected].drift[1] = 0;
                            cap = 0;
                        }
                        break;
                    case 2:
                        if(abs(cap)>1000)
                        {
                            esq.o[selected].drift[2] -= ((cap > 0)? 1:-1);
                            if(esq.o[selected].drift[2] > 12) esq.o[selected].drift[2] = 12;
                            else if(esq.o[selected].drift[2] < 0) esq.o[selected].drift[2] = 0;
                            cap = 0;
                        }
                    case 3:
                        if(abs(cap)>1000)
                        {
                            esq.o[selected].drift[3] -= ((cap > 0)? 1:-1);
                            if(esq.o[selected].drift[3] > 8) esq.o[selected].drift[3] = 8;
                            else if(esq.o[selected].drift[3] < 0) esq.o[selected].drift[3] = 0;
                            cap = 0;
                        }
                        break;
       
                    default:
                        break;
                }
                repaint = true;
            }
            prior = current;
        }

        if(refresh)
        {
            ssd1306_set_pixels(&oled);
            refresh = false;
        }
        // REPAINT ////////////////////////////////////////////////////////////////////////////////
        else if(repaint)
        {
            char str[16];
            switch (page)
            {
            case PAGE_NCDR:
                sprintf(str, "%d", current);
                ssd1306_log(&oled, str, 200, 0);
                refresh = true;
                break;

            case PAGE_MAIN:
                ssd1306_buffer_fill_pixels(&oled, BLACK);
                sprintf(str, "%s BPM     : %d",(line==0)?lsel[1]:lsel[0], esq.o[selected].bpm);
                ssd1306_print_string(&oled, 4,  0, str, 0, 0);
                sprintf(str, "%s STEPS   : %d",(line==1)?lsel[1]:lsel[0], esq.o[selected].steps);
                ssd1306_print_string(&oled, 4, 10, str, 0, 0);
                sprintf(str, "%s MODE    : %s",(line==2)?lsel[1]:lsel[0], mode[esq.o[selected].mode]);
                ssd1306_print_string(&oled, 4, 20, str, 0, 0);
                sprintf(str, "%s FREERUN : %s",(line==3)?lsel[1]:lsel[0], esq.o[selected].freerun ? "ON" : "OFF");
                ssd1306_print_string(&oled, 4, 30, str, 0, 0);
                sprintf(str, "\x81\x84\x81\x84\x81\x84\x81\x84\x81\x84\x81\x84\x81\x84\x81 ");
                str[selected*2] = '\x82';
                ssd1306_print_string(&oled, 4, 56, str, 0, 0);
                repaint = false;
                refresh = true;
                break;
            
            case PAGE_DRTN:
                ssd1306_buffer_fill_pixels(&oled, BLACK);

                ssd1306_print_string(&oled, 4, 0, "DURATION", 0, 0);
                for(int i = 0; i < 16; ++i)
                {
                    ssd1306_progress_bar(&oled, esq.o[selected].data[i].value, i*8 + 1, 10, 0xFF, 40, 6, true);
                }
                sprintf(str, "\x81\x84\x81\x84\x81\x84\x81\x84\x81\x84\x81\x84\x81\x84\x81 ");
                str[selected*2] = '\x82';
                ssd1306_print_string(&oled, 4, 56, str, 0, 0);
                repaint = false;
                refresh = true;
                break;

            case PAGE_AUTO:
                ssd1306_buffer_fill_pixels(&oled, BLACK);
                sprintf(str, "AUTOMATA : %s", esq.o[selected].regenerate[0]? "ON":"OFF");
                ssd1306_print_string(&oled, 4, 0, str, 0, 0);
                for(int i = 0; i < 8; ++i)
                {
                    switch (esq.automata[selected].rule[i])
                    {
                        case 0: ssd1306_print_char(&oled, 4 + 16 * i, 20, 0x8C, 0); break;
                        case 1: ssd1306_print_char(&oled, 4 + 16 * i, 20, 0x8D, 0); break;
                        case 2: ssd1306_print_char(&oled, 4 + 16 * i, 20, 0x8E, 0); break;
                        case 3: ssd1306_print_char(&oled, 4 + 16 * i, 20, 0x8F, 0); break;
                        default: break;
                    }
                }
                for(int i = 0; i < 4; ++i)
                {
                    if(((esq.automata[selected].rule[8 + i])>>1)&1) ssd1306_print_string(&oled, 4 + 32*i, 30, "Y", 0, 0);
                    else ssd1306_print_string(&oled, 4 + 32*i, 30, "X", 0, 0);
                    if(((esq.automata[selected].rule[8 + i]))&1) ssd1306_print_string(&oled, 20 + 32*i, 30, "Y", 0, 0);
                    else ssd1306_print_string(&oled, 20 + 32*i, 30, "X", 0, 0);
                }
                for(int i = 0; i < 8; ++i)
                {
                    if(esq.automata[selected].step[i] > 0)
                    ssd1306_print_char(&oled, 4 + 16*i, 40, 0x88 + esq.automata[selected].step[i], 0);
                    else
                    ssd1306_print_char(&oled, 4 + 16*i, 40, 0x88 + esq.automata[selected].step[i], 0);
                }
                sprintf(str, "\x81\x84\x81\x84\x81\x84\x81\x84\x81\x84\x81\x84\x81\x84\x81 ");
                str[selected*2] = '\x82';
                ssd1306_print_string(&oled, 4, 56, str, 0, 0);
                repaint = false;
                refresh = true;
                break;


            case PAGE_VELO:
                ssd1306_buffer_fill_pixels(&oled, BLACK);

                ssd1306_print_string(&oled, 4, 0, "VELOCITY", 0, 0);
                for(int i = 0; i < 16; ++i)
                {
                    ssd1306_progress_bar(&oled, esq.o[selected].data[i].velocity, i*8 + 1, 10, 0x7F, 40, 6, true);
                }
                sprintf(str, "\x81\x84\x81\x84\x81\x84\x81\x84\x81\x84\x81\x84\x81\x84\x81 ");
                str[selected*2] = '\x82';
                ssd1306_print_string(&oled, 4, 56, str, 0, 0);
                repaint = false;
                refresh = true;
                break;

            case PAGE_DRFT:
                ssd1306_buffer_fill_pixels(&oled, BLACK);
                ssd1306_print_string(&oled, 4, 0, "DRIFT", 0, 0);
                sprintf(str, "%s VELOCITY : %d",(line==0)?lsel[1]:lsel[0], esq.o[selected].drift[0]);
                ssd1306_print_string(&oled, 4, 10, str, 0, 0);
                sprintf(str, "%s OFFSET   : %d",(line==1)?lsel[1]:lsel[0], esq.o[selected].drift[1]);
                ssd1306_print_string(&oled, 4, 20, str, 0, 0);
                sprintf(str, "%s DEGREE   : %d",(line==2)?lsel[1]:lsel[0], esq.o[selected].drift[2]);
                ssd1306_print_string(&oled, 4, 30, str, 0, 0);
                sprintf(str, "%s OCTAVE   : %d",(line==3)?lsel[1]:lsel[0], esq.o[selected].drift[3]);
                ssd1306_print_string(&oled, 4, 40, str, 0, 0);
                sprintf(str, "\x81\x84\x81\x84\x81\x84\x81\x84\x81\x84\x81\x84\x81\x84\x81 ");
                str[selected*2] = '\x82';
                ssd1306_print_string(&oled, 4, 56, str, 0, 0);
                repaint = false;
                refresh = true;
                break;

            case PAGE_FFST:
                ssd1306_buffer_fill_pixels(&oled, BLACK);
                ssd1306_print_string(&oled, 4, 0, "OFFSET", 0, 0);
                for(int i = 0; i < 16; ++i)
                {
                    ssd1306_progress_cv_bar(&oled, esq.o[selected].data[i].offset, i*8 + 1, 10, 0xFF, 40, 6);
                }
                sprintf(str, "\x81\x84\x81\x84\x81\x84\x81\x84\x81\x84\x81\x84\x81\x84\x81 ");
                str[selected*2] = '\x82';
                ssd1306_print_string(&oled, 4, 56, str, 0, 0);
                repaint = false;
                refresh = true;
                break;

            case PAGE_NOTE:
                char s[1];
                ssd1306_buffer_fill_pixels(&oled, BLACK);

                ssd1306_print_string(&oled,  4,  0, "ROOT", 0, 0);
                ssd1306_print_string(&oled, 40,  0, chromatic_lr[esq.o[selected].scale.root], 0, 0);

                for(int i = 0; i < 12; ++i)
                {
                    if(esq.o[selected].scale.data & (0x800 >> i))
                    ssd1306_print_char(&oled, xs[i], ys[i], 0x82, 0);
                    else
                    ssd1306_print_char(&oled, xs[i], ys[i], 0x81, 0);
                    ssd1306_print_string(&oled,  6 + 10*i,  24, chromatic[esq.o[selected].data[i].chroma%12], 0, true);
                    sprintf(s, "%d", esq.o[selected].data[i].octave);
                    ssd1306_print_string(&oled,  6 + 10*i,  41, s, 0, true);

                }
                ssd1306_print_char(&oled, xs[esq.o[selected].scale.root], ys[esq.o[selected].scale.root], 0x83, 0);
                sprintf(str, "\x81\x84\x81\x84\x81\x84\x81\x84\x81\x84\x81\x84\x81\x84\x81 ");
                str[selected*2] = '\x82';
                ssd1306_print_string(&oled, 4, 56, str, 0, 0);
                repaint = false;
                refresh = true;
                break;
            
            case PAGE_SAVE:
                ssd1306_buffer_fill_pixels(&oled, BLACK);
                sprintf(str, "%s", save? "SAVE":"LOAD");
                for(int i = 0; i < 16; ++i)
                {
                    
                }
                ssd1306_print_string(&oled,  0,  0, str, 0, 0);
                repaint = false;
                refresh = true;
                break;

            default:
                break;
            }
        }       
        if(hold[BTNUP])
        {
            if(page == PAGE_NOTE) scale_led(&selected);
            else swith_led(&selected);
        }
        else swith_led(&selected);

        if(_4067_get())
        {
            switch (sreg)
            {
                case SHIFT:
                    if(!hold[SHIFT]) 
                    {
                        if(tap_armed)
                        {   
                            if(last != SHIFT) tap_armed = false;
                            int bpm = (esq.o[selected].bpm + 60000000/(time_us_32() - tap))/2;
                            reset_timestamp(&esq, selected, bpm);
                            if(!esq.o[selected].freerun)
                            {
                                for(int i = 0; i < TRACKS; ++i)
                                {
                                    if(!esq.o[i].freerun)
                                    {
                                        esq.o[i].bpm  = esq.o[selected].bpm;
                                        esq.o[i].beat = esq.o[selected].beat;
                                        esq.o[i].step = esq.o[selected].step;
                                        esq.o[i].atom = esq.o[selected].atom;
                                    }
                                }
                            }                            
                            repaint = true;
                        }
                        if(hold[ALTGR])
                        if(page == PAGE_AUTO) { automata_rand(&esq.automata[selected]); repaint = true; }
                        hold[SHIFT] = true;
                    }
                    last = SHIFT;
                    tap = time_us_32();
                    break;

                case ENCDR:
                    if(!hold[ENCDR])
                    {
                        hold[ENCDR] = true;
                    }
                    last = ENCDR;
                    break;

                case ALTGR:
                    if(!hold[ALTGR]) 
                    { 
                        hold[ALTGR] = true;
                    }
                    last = ALTGR;
                    break;

                case BTNUP:
                    if(!hold[BTNUP]) 
                    { 
                        if(hold[ALTGR])
                        {
                            if(page==PAGE_AUTO) 
                            {
                                esq.o[selected].regenerate[0] ^= 1;
                                repaint = true;
                            }
                        }
                        else if((page == PAGE_MAIN) || (page == PAGE_DRFT))
                        {
                            if(--line < 0) line = 3;
                            repaint = true;
                        }
                        hold[BTNUP] = true;
                    }
                    last = BTNUP;
                    break;

                case BTNCT:
                    if(!hold[BTNCT])
                    {
                        switch (esq.state)
                        {
                        case PLAY:
                            if(hold[SHIFT]) esq.state = STOP;
                            else esq.state = PAUSE;
                            break;
                        case PAUSE: 
                            esq.state = PLAY;
                            break;
                        case STOP:
                            esq.state = PLAY;
                            break;
                        default:
                            break;
                        }
                        hold[BTNCT] = true;
                    }
                    last = BTNCT;
                    break;

                case BTNDW:
                    if(!hold[BTNDW])
                    {
                        if(hold[BTNUP]) page = PAGE_SAVE;
                        else if((page == PAGE_MAIN) || (page == PAGE_DRFT))
                        {
                            line++;
                            if(line > 3) line = 0;
                            repaint = true;
                        }
                        hold[BTNDW] = true;
                    }
                    last = BTNDW;
                    break;

                case PGLFT: 
                    if(!hold[PGLFT]) 
                    { 
                        if(hold[ALTGR])
                        {
                            --selected;
                            if(selected < 0) selected = TRACKS - 1;
                        }
                        else
                        {
                            --page; 
                        }
                        repaint = true; 
                        hold[PGLFT] = true;
                    } 
                    last = PGLFT;
                    break;

                case PGRGT: 
                    if(!hold[PGRGT]) 
                    { 
                        if(hold[ALTGR])
                        {
                            ++selected;
                            if(selected >= TRACKS) selected = 0;
                        }
                        else
                        {
                            ++page;
                        }
                        repaint = true; 
                        hold[PGRGT] = true;
                    } 
                    last = PGRGT;
                    break;

                case MROW0: 
                    if(!hold_matrix[ccol]) 
                    { 
                        if(hold[BTNUP]) 
                        {   
                            if(page == PAGE_NOTE)
                            {
                                if(hold[ALTGR]) 
                                {
                                    esq.o[selected].scale.root = ccol&3;
                                    set_scale(&esq.o[selected].scale);
                                    recount_all(&esq, selected);
                                    repaint = true;
                                }
                                else
                                {
                                    esq.o[selected].scale.data ^= (0x800 >> (ccol&3));
                                    set_scale(&esq.o[selected].scale);
                                    recount_all(&esq, selected);
                                    repaint = true;
                                }
                            }
                        }
                        else if(hold[SHIFT]) esq.o[selected].trigger[ccol] = !esq.o[selected].trigger[ccol]; 
                        hold_matrix[ccol] = true; 
                    } 
                    last = MROW0;
                    break;

                case MROW1: 
                    if(!hold_matrix[ccol + 4]) 
                    { 
                        if(hold[BTNUP]) 
                        {   
                            if(page == PAGE_NOTE)
                            {
                                if(hold[ALTGR]) 
                                {
                                    esq.o[selected].scale.root = ((ccol&3) + 4);
                                    set_scale(&esq.o[selected].scale);
                                    recount_all(&esq, selected);
                                    repaint = true;
                                }
                                else
                                {
                                    esq.o[selected].scale.data ^= (0x80 >> (ccol&3));
                                    set_scale(&esq.o[selected].scale);
                                    recount_all(&esq, selected);
                                    repaint = true;
                                }
                            }
                        }
                        if(hold[SHIFT]) esq.o[selected].trigger[ccol + 4] = !esq.o[selected].trigger[ccol + 4]; 
                        hold_matrix[ccol + 4] = true; 
                    } 
                    last = MROW1;
                    break;

                case MROW2: 
                    if(!hold_matrix[ccol + 8]) 
                    { 
                        if(hold[BTNUP]) 
                        {   
                            if(page == PAGE_NOTE)
                            {
                                if(hold[ALTGR]) 
                                {
                                    esq.o[selected].scale.root = ((ccol&3) + 8);
                                    set_scale(&esq.o[selected].scale);
                                    recount_all(&esq, selected);
                                    repaint = true;
                                }
                                else
                                {
                                    esq.o[selected].scale.data ^= (0x8 >> (ccol&3));
                                    set_scale(&esq.o[selected].scale);
                                    recount_all(&esq, selected);
                                    repaint = true;
                                }
                            }
                        }
                        if(hold[SHIFT]) esq.o[selected].trigger[ccol + 8] = !esq.o[selected].trigger[ccol + 8]; 
                        hold_matrix[ccol + 8] = true; 
                    } 
                    last = MROW2;
                    break;

                case MROW3: 
                    if(!hold_matrix[ccol + 12]) 
                    { 
                        if(hold[SHIFT]) esq.o[selected].trigger[ccol + 12] = !esq.o[selected].trigger[ccol + 12]; 
                        hold_matrix[ccol + 12] = true; 
                    } 
                    last = MROW3;
                    break;

                default: 
                    break;
            }
            if(page > PAGES) page = 0;
            else if(page < 0) page = PAGES - 1;
        }
        else 
        {
            if(sreg < 4) hold_matrix[sreg * 4 + ccol] = false;
            else if(sreg == SHIFT) 
            { 
                if(last == SHIFT) tap_armed = true;// tap = time_us_32(); 
                hold[SHIFT] = false;
            }
            else hold[sreg] = false;
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
    ///////////////////////////////////////////////////////////////////////////////
    // OLED Init //////////////////////////////////////////////////////////////////
    i2c_init(i2c1, 3200000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);
    ssd1306_init(&oled, 0x3C, i2c1, BLACK);
    ssd1306_set_full_rotation(&oled, 0);
    // MIDI DIN Init //////////////////////////////////////////////////////////////
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    // 595 Init ///////////////////////////////////////////////////////////////////
    shift_register_74HC595_init(&sr, SPI_PORT, DATA_595, CLOCK_595, LATCH_595);
    gpio_set_outover(DATA_595, GPIO_OVERRIDE_INVERT); 
    _74HC595_set_all_low(&sr);
    // 4067 Init //////////////////////////////////////////////////////////////////
    _4067_init();
    keypad_init();
    ///////////////////////////////////////////////////////////////////////////////
    // Sequencer Init /////////////////////////////////////////////////////////////
    absolute_time_t tts[TRACKS]; // Trigger ON 
    absolute_time_t gts[TRACKS]; // Gate OFF
    bool gate[TRACKS];           // OFF is pending
    int  rv[TRACKS];             // Revolution counters
    srand(time_us_32());         // Random seed

    sequencer_init(&esq, 120);
    for(int i = 0; i < TRACKS; ++i) sequencer_rand(&esq, i);
    multicore_fifo_push_blocking(0);
    ////////////////////////////////////////////////////////////////////////////////
    // CORE 0 Loop /////////////////////////////////////////////////////////////////
    ARM:
    if(esq.state == PLAY) arm(100, tts);
    RUN:
    while (esq.state == PLAY) 
	{
        tud_task();
        for(int i = 0; i < TRACKS; ++i)
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
            //////////////////////////////////////////////////////////////////////////////////////
            // AUTOMATA //////////////////////////////////////////////////////////////////////////
            if(esq.o[i].regenerate[0])
            {
                if(rv[i] != esq.o[i].revolutions)
                {
                    for(int j = 0; j < 16; j++) esq.automata[i].field[j] = esq.o[i].trigger[j];
                    automata_evolve(&esq.automata[i]);
                    for(int j = 0; j < 16; j++) esq.o[i].trigger[j] = esq.automata[i].field[j];
                    rv[i] = esq.o[i].revolutions;
                }
            }
        }
        multicore_fifo_push_blocking(0);
    }
    //////////////////////////////////////////////////////////////////////////////////////
    // Paused ////////////////////////////////////////////////////////////////////////////
    while(esq.state == PAUSE) 
	{
        for(int i = 0; i < TRACKS; ++i)
        {
            if(time_reached(gts[i]))
            {
                if(gate[i])
                {
                    send(i, 0x80);
                    gate[i] = false;
                }
            }
        }
        multicore_fifo_push_blocking(0);
        if(esq.state == PLAY) goto RUN;
    }
    //////////////////////////////////////////////////////////////////////////////////////
    // Stopped ///////////////////////////////////////////////////////////////////////////
    while(esq.state == STOP) 
	{
        for(int i = 0; i < TRACKS; ++i)
        {
            if(time_reached(gts[i]))
            {
                if(gate[i])
                {
                    send(i, 0x80);
                    gate[i] = false;
                }
            }
            esq.o[i].current = 0;
        }
        multicore_fifo_push_blocking(0);
        if(esq.state == PLAY) goto ARM;
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////
// Send MIDI message //////////////////////////////////////////////////////////////////
inline static void send(uint8_t id, const uint8_t status) 
{
    switch (status)
    {
    case 0x80: // Note ON
        uint8_t off[3] = { status|id, esq.o[id].data[esq.o[id].current].chroma, 0 };
        _send_note(off);
        tud_midi_stream_write(cable_num, off, 3);
        break;

    case 0x90: // Note OFF
        int velocity = esq.o[id].data[esq.o[id].current].velocity;
        if(esq.o[id].drift[0]) 
        {
            velocity = esq.o[id].data[esq.o[id].current].velocity + rand_in_range(-esq.o[id].drift[0], esq.o[id].drift[0]);
            if(velocity > 0x7F) velocity = 0x7F;
            else if(velocity < 1) velocity = 1;
        }
        uint8_t on[3] = { status|id, esq.o[id].data[esq.o[id].current].chroma, velocity };
        _send_note(on);
        tud_midi_stream_write(cable_num, on, 3);
        break;

    default:
        break;
    }
    // if(page == PAGE_LOG_)
    // {
    //     char str[16];
    //     if(status == 0x80)
    //     sprintf(str, "ON :%2X:%2X:%2X", status|id, esq.o[id].data[esq.o[id].current].chroma, esq.o[id].data[esq.o[id].current].velocity );
    //     if(status == 0x90)
    //     sprintf(str, "OFF:%2X:%2X:%2X", status|id, esq.o[id].data[esq.o[id].current].chroma, esq.o[id].data[esq.o[id].current].velocity );
    //     ssd1306_log(&oled, str, 0, 0);
    // }
}

//////////////////////////////////////////////////////////////////////////////////////////
// Prepare to play routine ///////////////////////////////////////////////////////////////
inline static void arm(uint lag, absolute_time_t* t) 
{
    int f = esq.o[0].data[esq.o[0].current].offset;
    for(int i = 1; i < TRACKS; ++i)
    {
        if(f > esq.o[i].data[esq.o[i].current].offset)
        {
            f = esq.o[i].data[esq.o[i].current].offset;
        }
    }
    for(int i = 0; i < TRACKS; ++i)
    {
        t[i] = make_timeout_time_ms(esq.o[i].data[esq.o[i].current].offset - f + lag);
    }
}

//////////////////////////////////////////////////////////////////////////////////////
// Loop led run //////////////////////////////////////////////////////////////////////
inline static void swith_led(const int* track)
{
    static int led;
    _74HC595_set_all_low(&sr);
    if(esq.o[*track].trigger[led]) pset(&sr, led&3, led>>2, 4);
    if(++led > STEPS) 
    {
        led = 0;
        _74HC595_set_all_low(&sr);
        pset(&sr, esq.o[*track].current&3, esq.o[*track].current>>2, 2);
    }
}

//////////////////////////////////////////////////////////////////////////////////////
// Scale representation //////////////////////////////////////////////////////////////
inline static void scale_led(const int* track)
{
    static int led;
    _74HC595_set_all_low(&sr);
    if((esq.o[*track].scale.data&(0x800>>led))) pset(&sr, led&3, led>>2, 2);
    if(++led > 11) 
    {
        led = 0;
        _74HC595_set_all_low(&sr);
        pset(&sr, esq.o[*track].scale.root &3, esq.o[*track].scale.root>>2, 4);
    }
}