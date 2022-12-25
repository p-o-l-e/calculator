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
#include "SSD1306/sieve.xbm"
#include "fs.h"

inline static void swith_led(const int* restrict track);
inline static void scale_led(const int* restrict track);
inline static void arm(uint lag, absolute_time_t* restrict t);
static void init();
static void __time_critical_func(send)(uint8_t id, const uint8_t status);
int __time_critical_func(main)();
static CD74HC595 sr;
static ssd1306_t oled;
static sequencer esq;
static lfs_t fs;
static lfs_file_t init_f;
static int files = 0;    // Saved presets count
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Core 1 interrupt Handler ///////////////////////////////////////////////////////////////////////////////////////////////////
void core1_interrupt_handler() 
{
    quadrature_decoder ncoder;
    int ncoder_index;
    ncoder_index = quad_encoder_init(&ncoder);   
    // load_file(&fs, &init, "INIT", &esq);
    
    bool hold[16]; 
    memset(hold, 0, sizeof(hold));
    bool hold_matrix[16]; 
    memset(hold_matrix, 0, sizeof(hold_matrix));
    bool refresh = false;                // ssd1306_xbm(&oled, sieve_bits, 128, 64, 0, 0);

    bool repaint = true;
    bool tap_armed = false;
    bool save = true;
    int tap = 0;
    int comb = 0; 	  // Ranged value accumulator
    int prior = 0;    // Last encoder value
    int last = 0;     // Last action
    int imux = 4;     // Multiplexer iterator
    int line = 0;     // Selected display line
    int selected = 0; // Displayed track
    int page = 0;     // Current page
    int crv[TRACKS];  // Revolution counters
    int step;   	  // Selected step
    int print_pos = 0;// Filename save underline
    int selected_file = 0;
    char filename[8] = "        ";
    memset(crv, 0, sizeof(crv));

    int cap = 0;
    const char* restrict lsel[] = { " ", "\x80" };
    const char* restrict mode[] = { "FWD", "BWD", "PNG", "RND" };
    const int xkeys[12] = { 56, 60, 66, 70, 76, 86, 90, 96, 100, 106, 110, 116};
    const int ykeys[12] = { 10,  0, 10,  0, 10, 10,  0, 10,   0,  10,   0,  10};
    const char* restrict chromatic[]    = { " c","#c"," d","#d"," e"," f","#f"," g","#g"," a","#a"," b" };
    const char* restrict chromatic_lr[] = { "C ","C#","D ","D#","E ","F ","F#","G ","G#","A ","A#","B " };


    while (multicore_fifo_rvalid())
    {
        uint16_t raw = multicore_fifo_pop_blocking();  
        if(++imux >= N4067) imux = 0;
        _4067_switch(imux);
        int ccol = keypad_switch();
        // ENCODER ////////////////////////////////////////////////////////////////////////////////////////////////////////////
        int current = get_count(&ncoder, ncoder_index);
        if(current!=prior)
        {
            step = -1;
            cap += ((prior - current)*300);
            for(int i = 0; i < 16; ++i)
            {
                if(hold_matrix[i])
                {
                    step = i;
                    break;
                }
            }
            ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // Matrix button pressed //////////////////////////////////////////////////////////////////////////////////////////
            if(step >= 0) 
            {
                switch (page)
                {
                    case PAGE_VELO:
                    	if(esq.brush > 1)
                    	{
	                    	int incr = ((prior > current) ? -1 - 4*hold[ENCDR] : 1 + 4*hold[ENCDR]);
	                    	if(comb > esq.brush) comb = 0;
	                    	set_section[esq.form](&esq, selected, step, esq.brush, comb, incr);
	                    	comb++;
                    	}
                    	else
                    	{
	                        esq.o[selected].data[step].velocity  += ((prior > current) ? -1 - 4*hold[ENCDR] : 1 + 4*hold[ENCDR]);
							fit_velocity(&esq, selected, step);
						}
                        repaint = true;
                    break;

					case PAGE_DRTN: 
						if(esq.brush > 1)
                    	{
	                    	int incr = ((prior > current) ? -1 - 4*hold[ENCDR] : 1 + 4*hold[ENCDR]);
	                    	if(comb > esq.brush) comb = 0;
	                    	set_section[esq.form + 3](&esq, selected, step, esq.brush, comb, incr);
	                    	comb++;
                    	}
                    	else
                    	{
	                        esq.o[selected].data[step].value += ((prior > current) ? -1 - 4*hold[ENCDR] : 1 + 4*hold[ENCDR]);
	                        fit_duration(&esq, selected, step);
                        }
                        repaint = true;
                    break;                    

                    case PAGE_FFST:
						if(esq.brush > 1)
                    	{
	                    	int incr = ((prior > current) ? -1 - 4*hold[ENCDR] : 1 + 4*hold[ENCDR]);
	                    	if(comb > esq.brush) comb = 0;
	                    	set_section[esq.form + 6](&esq, selected, step, esq.brush, comb, incr);
	                    	comb++;
                    	}                    	
                    	else
                    	{
	                        esq.o[selected].data[step].offset  += ((prior > current) ? -1 - 2*hold[ENCDR] : 1 + 2*hold[ENCDR]);
	                        fit_offset(&esq, selected, step);
                        }
                        repaint = true;
                    break;

                    case PAGE_NOTE: 
                        if(abs(cap)>1000)
                        {
                            if(hold[ENCDR]) 
                            {
                                esq.o[selected].data[step].octave -= ((cap > 0)? 1:-1);
                                if(esq.o[selected].data[step].octave > 9) esq.o[selected].data[step].octave = 9;
                                else if(esq.o[selected].data[step].octave < 0) esq.o[selected].data[step].octave = 0;
                            }
                            else 
                            {
                                esq.o[selected].data[step].degree -= ((cap > 0)? 1:-1);
                                if(esq.o[selected].data[step].degree > (esq.o[selected].scale.width - 1)) 
                                esq.o[selected].data[step].degree = esq.o[selected].scale.width - 1;
                                else if(esq.o[selected].data[step].degree <  0) esq.o[selected].data[step].degree = 0;
                            }
                            cap = 0;
                            repaint = true;
                            esq.o[selected].data[step].recount = true;
                            note_from_degree(&esq.o[selected].scale, &esq.o[selected].data[step]);
                        }
                    break;

                    case PAGE_SIEV:
						if(abs(cap)>1000)
						{
							int s = step&7;
							esq.o[selected].sieve[s] += ((cap > 0)? -1 : 1);
							if(esq.o[selected].sieve[s] > 16) esq.o[selected].sieve[s] = 16;
							else if(esq.o[selected].sieve[s] < 0) esq.o[selected].sieve[s] = 0;
							cap = 0;
                            repaint = true;
						}
                    break;

                    default: 
                    break;
                }
            }
            else // Matrix buttons unpressed
            {
            switch(page)
            {
            case PAGE_MAIN:
                switch(line)
                {
                    case 0: 
                        if(abs(cap)>1000)
                        {
                            int bpm = esq.o[selected].bpm - ((cap > 0)? 1 + 4*hold[ENCDR]: - 1 - 4*hold[ENCDR]);
                            if(bpm > 999) bpm = 999;
                            else if(bpm < 1) bpm = 1;
                            reset_timestamp(&esq, selected, bpm);
                            cap = 0;
                            repaint = true;
                        }
                    break;

                    case 1: 
                        if(abs(cap)>1000)
                        {
                            esq.o[selected].steps  -= ((cap > 0)? 1:-1);
                            if(esq.o[selected].steps > 16) esq.o[selected].steps = 16;
                            else if(esq.o[selected].steps < 2) esq.o[selected].steps = 2;
                            cap = 0;
                            repaint = true;
                        }
                    break;

                    case 2:
                        if(abs(cap)>1000)
                        {
                            esq.o[selected].mode -= ((cap > 0)? 1:-1);
                            if(esq.o[selected].mode > 3) esq.o[selected].mode = 3;
                            else if(esq.o[selected].mode < 0) esq.o[selected].mode = 0;
                            cap = 0;
                            repaint = true;
                        }       
                    break;

                    default:
                    break;
                }
            break;


            case PAGE_NOTE:
                if(hold[SHIFT])
                {
                    if(abs(cap)>1000)
                    {
                        for(int i = 0; i < STEPS; i++)
                        {
                            esq.o[selected].data[i].octave -= ((cap > 0)? 1:-1);
                            if(esq.o[selected].data[i].octave > 9) esq.o[selected].data[i].octave = 9;
                            else if(esq.o[selected].data[i].octave < 0) esq.o[selected].data[i].octave = 0;
                        }  
                        recount_all(&esq, selected);        
                        cap = 0;
                        repaint = true;
                    }                
                }
                else if(hold[ALTGR])
                {
                    if(abs(cap)>1000)
                    {
                        for(int i = 0; i < STEPS; i++)
                        {
                            esq.o[selected].data[i].degree -= ((cap > 0)? 1:-1);
                            if(esq.o[selected].data[i].degree > 11) esq.o[selected].data[i].degree -= 12;
                            else if(esq.o[selected].data[i].degree < 0) esq.o[selected].data[i].degree += 12;
                        }  
                        recount_all(&esq, selected);        
                        cap = 0;
                        repaint = true;
                    }                
                }
            break;

            case PAGE_VELO:
                if(hold[ALTGR])
                {
                    if(abs(cap)>1000)
                    {
                        for(int i = 0; i < STEPS; i++)
                        {
                            esq.o[selected].data[i].velocity -= ((cap > 0)? 1 + 4*hold[ENCDR]: - 1 - 4*hold[ENCDR]);
                            fit_velocity(&esq, selected, i);
                        }  
                        cap = 0;
                        repaint = true;
                    }                
                }
				else if(hold[SHIFT])
                {
                    if(abs(cap)>1000)
                    {
 						esq.form -= ((cap > 0)? 1 : -1);
                       	if(esq.form > 2) esq.form = 2;
                       	else if(esq.form < 0) esq.form = 0;
                        cap = 0;
                        repaint = true;
                    }                
                }                
                else
                {
      				if(abs(cap)>1000)
                    {
						esq.brush -= ((cap > 0)? 1 : -1);
                       	if(esq.brush > 0xF) esq.brush = 0xF;
                       	else if(esq.brush < 1) esq.brush = 1;
                        cap = 0;
                        repaint = true;
                    }                           	
                }
            break;

            case PAGE_DRTN:
                if(hold[ALTGR])
                {
                    if(abs(cap)>1000)
                    {
                        for(int i = 0; i < STEPS; i++)
                        {
                            esq.o[selected].data[i].value -= ((cap > 0)? 1 + 8*hold[ENCDR]: - 1 - 8*hold[ENCDR]);
                            fit_duration(&esq, selected, i);
                        }  
                        cap = 0;
                        repaint = true;
                    }                
                }
				else if(hold[SHIFT])
                {
                    if(abs(cap)>1000)
                    {
 						esq.form -= ((cap > 0)? 1 : -1);
                       	if(esq.form > 2) esq.form = 2;
                       	else if(esq.form < 0) esq.form = 0;
                        cap = 0;
                        repaint = true;
                    }                
                }                
                else
                {
      				if(abs(cap)>1000)
                    {
						esq.brush -= ((cap > 0)? 1 : -1);
                       	if(esq.brush > 0xF) esq.brush = 0xF;
                       	else if(esq.brush < 1) esq.brush = 1;
                        cap = 0;
                        repaint = true;
                    }                           	
                }                
            break;

            case PAGE_FFST:
				if(hold[ALTGR])
                {
                    if(abs(cap)>1000)
                    {
                        for(int i = 0; i < STEPS; i++)
                        {
                            esq.o[selected].data[i].offset -= ((cap > 0)? 1 + 4*hold[ENCDR]: - 1 - 4*hold[ENCDR]);
                            fit_offset(&esq, selected, i);
                        }  
                        cap = 0;
                        repaint = true;
                    }                
                }            
				else if(hold[SHIFT])
                {
                    if(abs(cap)>1000)
                    {
 						esq.form -= ((cap > 0)? 1 : -1);
                       	if(esq.form > 2) esq.form = 2;
                       	else if(esq.form < 0) esq.form = 0;
                        cap = 0;
                        repaint = true;
                    }                
                }                
                else
                {
      				if(abs(cap)>1000)
                    {
						esq.brush -= ((cap > 0)? 1 : -1);
                       	if(esq.brush > 0xF) esq.brush = 0xF;
                       	else if(esq.brush < 1) esq.brush = 1;
                        cap = 0;
                        repaint = true;
                    }                           	
                }
            break;

            case PAGE_SAVE:
				if(abs(cap)>1000)
                {
 					filename[print_pos] -= ((cap > 0)? 1 : -1);
                    if(filename[print_pos] > 90) filename[print_pos] = 32;
                    else if(filename[print_pos] < 32) filename[print_pos] = 90;
                    cap = 0;
                    repaint = true;
                }
            break;

			case PAGE_LOAD:
				if(abs(cap)>1000)
                {
 					selected_file += ((cap > 0)? -1 : 1);
                    if(selected_file < 0) selected_file = 0;
                    else if(selected_file >= files) selected_file = files - 1;
                    cap = 0;
                    repaint = true;
                }
            break;

            default:
            break;
            }}
            prior = current;
        }
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // LED RUN ////////////////////////////////////////////////////////////////////////////////////////////////////////////
        if(hold[ALTGR])
        {
            if(page == PAGE_NOTE) scale_led(&selected);
            else swith_led(&selected);
        }
        else swith_led(&selected);

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // READ MULTIPLEXER ///////////////////////////////////////////////////////////////////////////////////////////////////
        if(_4067_get())
        {
            switch (imux)
            {
                case SHIFT:
                    if(!hold[SHIFT])
                    {
                        if((hold[ALTGR])&&(page == PAGE_AUTO))
                        {
                            automata_rand(&esq.automata[selected]);
                            repaint = true;
                        }
                        else if(tap_armed)
                        {
                            if(last != SHIFT) tap_armed = false;
                            int bpm = (esq.o[selected].bpm + 60000000/(time_us_32() - tap))>>1;
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
                        if(hold[SHIFT]) esq.state = PLAY;
                        else if(hold[ALTGR])
                        {
                            if(page==PAGE_AUTO) 
                            {
                                esq.automata[selected].on ^= 1;
                                repaint = true;
                            }
							if(page==PAGE_SIEV) 
                            {
                                esq.o[selected].regenerate ^= 1;
                                repaint = true;
                            }                            
                        }
                        else
                        {
                            switch(page)
                            {
                            case PAGE_MAIN:
                                if(--line < 0) line = 4;
                                repaint = true;
                            break;

                            case PAGE_NOTE:
                                if(--line < 0) line = 3;
                                repaint = true;
                            break;
                            
							case PAGE_VELO:
                            	if(--line < 0) line = 1;
                            	repaint = true;
							break;

							case PAGE_DRTN:
								if(--line < 0) line = 1;
								repaint = true;
							break;

							case PAGE_FFST:
								if(--line < 0) line = 1;
								repaint = true;
							break;   

							case PAGE_SAVE:
								if(++print_pos > 7) print_pos = 0;
								repaint = true;
							break;                         

                            default:
                            break;
                            }
                        }
                        hold[BTNUP] = true;
                    }
                    last = BTNUP;
                break;

                case BTNCT:
                    if(!hold[BTNCT])
                    {
                        if(hold[SHIFT]) esq.state = PAUSE;
                        else
                        {
                            switch (page)
                            {
                            case PAGE_SAVE:                            	
                            	save_file(&fs, &init_f, filename, &esq );
                            	files = get_file_count(&fs, "");
                            break;
                            
                            case PAGE_LOAD:
                            break;

                            case PAGE_MAIN:
                                if(line == 3) esq.o[selected].freerun ^= 1;
                                else if(line == 4) esq.o[selected].euclidean ^= 1;
                                repaint = true;
                            break;

                            case PAGE_VELO:
								switch(line)
                                {
                                case 0:
                                    esq.o[selected].sift[2]^=1;
                                    repaint = true;
                                break;

                                case 1:
                                    esq.o[selected].permute[2]^=1;
                                    repaint = true;
                                break;
                                
                                default:
                                break;
                                }
                            break;

							case PAGE_DRTN:
								switch(line)
                                {
                                case 0:
                                    esq.o[selected].sift[3]^=1;
                                    repaint = true;
                                break;

                                case 1:
                                    esq.o[selected].permute[3]^=1;
                                    repaint = true;
                                break;
                                
                                default:
                                break;
                                }
                            break;                            

							case PAGE_FFST:
								switch(line)
                                {
                                case 0:
                                    esq.o[selected].sift[4]^=1;
                                    repaint = true;
                                break;

                                case 1:
                                    esq.o[selected].permute[4]^=1;
                                    repaint = true;
                                break;
                                
                                default:
                                break;
                                }
                            break;

							case PAGE_SIEV:
                              	esq.o[selected].permute[5]^=1;
                               	repaint = true;
                            break;                            
                            
                            case PAGE_NOTE:
                                switch(line)
                                {
                                case 0:
                                    esq.o[selected].sift[0]^=1;
                                    repaint = true;
                                break;

                                case 2:
                                    esq.o[selected].permute[0]^=1;
                                    repaint = true;
                                break;

                                case 1:
                                    esq.o[selected].sift[1]^=1;
                                    repaint = true;
                                break;

                                case 3:
                                    esq.o[selected].permute[1]^=1;
                                    repaint = true;
                                break;
                                
                                default:
                                break;
                                }
                            break;
                            
                            default:
                            break;
                            }
                        }
                        hold[BTNCT] = true;
                    }
                    last = BTNCT;
                break;

                case BTNDW:
                    if(!hold[BTNDW])
                    {
                        if(hold[SHIFT]) esq.state = STOP;
                        else if(hold[BTNUP]) 
                        {
                        	page = PAGE_SAVE;
                        	repaint = true;
                        }
                        else
                        {
                            switch(page)
                            {
                            case PAGE_MAIN:
                                if(++line > 4) line = 0;
                                repaint = true;
                            break;

                            case PAGE_NOTE:
                                if(++line > 3) line = 0;
                                repaint = true;
                            break;

                            case PAGE_VELO:
                            	if(++line > 1) line = 0;
                            	repaint = true;
							break;

							case PAGE_DRTN:
								if(++line > 1) line = 0;
								repaint = true;
							break;

							case PAGE_FFST:
								if(++line > 1) line = 0;
								repaint = true;
							break;

							case PAGE_SAVE:
								if(--print_pos < 0) print_pos = 7;
								repaint = true;
							break;
														
                            default:
                            break;
                            }
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
                            if(--selected < 0) selected = TRACKS - 1;
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
                            if(++selected >= TRACKS) selected = 0;
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
                        if(hold[ALTGR]) 
                        {   
                            if(page == PAGE_NOTE)
                            {
                                if(hold[SHIFT]) 
                                {
                                    esq.o[selected].scale.root = ccol&3;
                                    transpose_root(&esq.o[selected].scale);
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
                        else if(hold[SHIFT]) esq.o[selected].trigger ^= (1<<ccol); 
                        hold_matrix[ccol] = true; 
                    } 
                    last = MROW0;
                break;

                case MROW1: 
                    if(!hold_matrix[ccol + 4]) 
                    { 
                        if(hold[ALTGR]) 
                        {   
                            if(page == PAGE_NOTE)
                            {
                                if(hold[SHIFT]) 
                                {
                                    esq.o[selected].scale.root = ((ccol&3) + 4);
                                    transpose_root(&esq.o[selected].scale);
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
                        else if(hold[SHIFT]) esq.o[selected].trigger ^= (1<<(ccol + 4)); 
                        hold_matrix[ccol + 4] = true; 
                    } 
                    last = MROW1;
                break;

                case MROW2: 
                    if(!hold_matrix[ccol + 8]) 
                    { 
                        if(hold[ALTGR]) 
                        {   
                            if(page == PAGE_NOTE)
                            {
                                if(hold[SHIFT]) 
                                {
                                    esq.o[selected].scale.root = ((ccol&3) + 8);
                                    transpose_root(&esq.o[selected].scale);
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
                        else if(hold[SHIFT]) esq.o[selected].trigger ^= (1<<(ccol + 8)); 
                        hold_matrix[ccol + 8] = true;
                    }
                    last = MROW2;
                break;

                case MROW3:
                    if(!hold_matrix[ccol + 12])
                    {
                        if(hold[SHIFT]) esq.o[selected].trigger ^= (1<<(ccol + 12));
                        hold_matrix[ccol + 12] = true;
                    }
                    last = MROW3;
                break;

                default:
                break;
            }
            if(page >= PAGES) page = 0;
            else if(page < 0) page = PAGES - 1;
        }
        else
        {
            if(imux < 4) hold_matrix[imux * 4 + ccol] = false;
            if(imux == SHIFT)
            {
                if(last == SHIFT) tap_armed = true;
                hold[SHIFT] = false;
            }
            else hold[imux] = false;
        }
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // AUTOMATA ///////////////////////////////////////////////////////////////////////////////////////////////////////////
        for(int i = 0; i < TRACKS; ++i)
        {
        	if(crv[i] != esq.o[i].revolutions)
            {
                if(esq.automata[i].on)
                {
                    if(esq.o[i].euclidean)
                    {
                        automata_evolve(&esq.automata[i]);
                        int pulses = __builtin_popcount(esq.automata[i].field);
                        esq.o[i].trigger = rightrot16(bjorklund(STEPS, pulses), xor16to4(esq.automata[i].field));
                    }
                    else
                    {
                        esq.automata[i].field = esq.o[i].trigger;
                        automata_evolve(&esq.automata[i]);
                        esq.o[i].trigger = esq.automata[i].field;
                    }
                    if(esq.o[i].regenerate)
                    {
	                    regenerate_sieve(&esq, i, esq.automata[i].field);
	                    if(page == PAGE_SIEV) repaint = true;
                    }
				}
				
               	if(esq.o[i].permute[0])
                {
                  	mutate[0](&esq, i, esq.automata[i].field);
                    recount_all(&esq, i);
                  	repaint = true;
                }
                if(esq.o[i].permute[1])
                {
                    mutate[1](&esq, i, esq.automata[i].field);
                    recount_all(&esq, i);
                    repaint = true;
                }
                if(esq.o[i].permute[2])
                {
                    mutate[2](&esq, i, esq.automata[i].field);
                   	repaint = true;
                }
                if(esq.o[i].permute[3])
                {
                    mutate[3](&esq, i, esq.automata[i].field);
                    repaint = true;
                }
				if(esq.o[i].permute[4])
                {
                    mutate[4](&esq, i, esq.automata[i].field);
                   	repaint = true;
                }
				if(esq.o[i].permute[5])
                {
                    mutate[5](&esq, i, esq.automata[i].field);
                   	repaint = true;
                }
                
				if(esq.o[i].sift[0])
                {
                    sift[0](&esq, i);
                    recount_all(&esq, i);
                   	repaint = true;
                }  
				if(esq.o[i].sift[1])
                {
                    sift[1](&esq, i);
                    recount_all(&esq, i);
                   	repaint = true;
                }              
				if(esq.o[i].sift[3])
                {
                    sift[3](&esq, i);
                   	repaint = true;
                }   
				if(esq.o[i].sift[3])
                {
                    sift[3](&esq, i);
                   	repaint = true;
                } 
				if(esq.o[i].sift[4])
                {
                    sift[4](&esq, i);
                   	repaint = true;
                }
               
            	crv[i] = esq.o[i].revolutions;
           	}
        }

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // REPAINT ////////////////////////////////////////////////////////////////////////////////////////////////////////////
        if(repaint)
        {
            char str[16];
            char list[5][16];
            switch (page)
            {
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
                sprintf(str, "%s ALIGN   : %s",(line==4)?lsel[1]:lsel[0], esq.o[selected].euclidean ? "ON" : "OFF");
                ssd1306_print_string(&oled, 4, 40, str, 0, 0);
                sprintf(str, "\x81\x84\x81\x84\x81\x84\x81\x84\x81\x84\x81\x84\x81\x84\x81");
                str[selected*2] = '\x82';
                ssd1306_print_string(&oled, 4, 56, str, 0, 0);
                repaint = false;
                refresh = true;
            break;
            
  			case PAGE_NOTE:
                  ssd1306_buffer_fill_pixels(&oled, BLACK);
                  ssd1306_print_string(&oled,  4,  0, "ROOT", 0, 0);
                  ssd1306_print_string(&oled, 40,  0, chromatic_lr[esq.o[selected].scale.root], 0, 0);
                  for(int i = 0; i < 12; ++i)
                  {
                      if(esq.o[selected].scale.data & (0x800 >> i))
                      ssd1306_print_char(&oled, xkeys[i], ykeys[i], 0x82, 0);
                      else
                      ssd1306_print_char(&oled, xkeys[i], ykeys[i], 0x81, 0);
                  }
                  for(int i = 0; i < STEPS; ++i)
                  {
                      ssd1306_print_string(&oled, 29 + 6*i,  23, chromatic[esq.o[selected].data[i].chroma%12], 0, true);
                      ssd1306_print_char(&oled, 29 + 6*i, 41, 0x90 + esq.o[selected].data[i].octave, false);
                      if((esq.o[selected].trigger>>i)&1) 
                      {
                      	ssd1306_line(&oled, 29 + 6*i, 50, 5, false);
                      	ssd1306_line(&oled, 29 + 6*i, 51, 5, false);
                      }
                  }
                  ssd1306_print_char(&oled,  4, 31, esq.o[selected].sift[0] ? 0xA0 : 0xA1, false);
                  ssd1306_print_char(&oled, 16, 31, esq.o[selected].permute[0] ? 0x9E : 0x9F, false);
                  
                  ssd1306_print_char(&oled,  4, 41, esq.o[selected].sift[1] ? 0xA0 : 0xA1, false);
                  ssd1306_print_char(&oled, 16, 41, esq.o[selected].permute[1] ? 0x9E : 0x9F, false);
  				  if(line > 3) line = 0;
                  ssd1306_corners(&oled, 3 + 12*(line/2), 30 + 10*(line%2), 9, 8);
  
                  ssd1306_print_char(&oled, xkeys[esq.o[selected].scale.root], ykeys[esq.o[selected].scale.root], 0x83, 0);
                  sprintf(str, "\x81\x84\x81\x84\x81\x84\x81\x84\x81\x84\x81\x84\x81\x84\x81");
                  str[selected*2] = '\x82';
                  ssd1306_print_string(&oled, 4, 56, str, 0, 0);
                  repaint = false;
                  refresh = true;
          	break;

            case PAGE_AUTO:
                ssd1306_buffer_fill_pixels(&oled, BLACK);
                sprintf(str, "AUTOMATA : %s", esq.automata[selected].on? "ON":"OFF");
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
                    else ssd1306_print_string(&oled,  4 + 32*i, 30, "X", 0, 0);
                    if(((esq.automata[selected].rule[8 + i]))&1) ssd1306_print_string(&oled, 20 + 32*i, 30, "Y", 0, 0);
                    else ssd1306_print_string(&oled, 20 + 32*i, 30, "X", 0, 0);
                }
                for(int i = 0; i < 8; ++i)
                {
                    if(esq.automata[selected].rule[12 + i] > 0)
                    ssd1306_print_char(&oled, 4 + 16*i, 40, 0x88 + esq.automata[selected].rule[12 + i], 0);
                    else
                    ssd1306_print_char(&oled, 4 + 16*i, 40, 0x88 + esq.automata[selected].rule[12 + i], 0);
                }
                sprintf(str, "\x81\x84\x81\x84\x81\x84\x81\x84\x81\x84\x81\x84\x81\x84\x81");
                str[selected*2] = '\x82';
                ssd1306_print_string(&oled, 4, 56, str, 0, 0);
                repaint = false;
                refresh = true;
            break;

            case PAGE_VELO:
                ssd1306_buffer_fill_pixels(&oled, BLACK);
                ssd1306_print_string(&oled, 4, 0, "VELOCITY", 0, 0);
                ssd1306_print_char(&oled, 105, 1, esq.o[selected].sift[2] ? 0xA0 : 0xA1, false);
                ssd1306_print_char(&oled, 117, 1, esq.o[selected].permute[2] ? 0x9E : 0x9F, false);
                switch(esq.form)
                {
                	case 0:
     					ssd1306_glyph(&oled, rise_20x20_u, 20, 20, 105, 10);
                		break;
					case 1:
						ssd1306_glyph(&oled, fall_20x20_u, 20, 20, 105, 10);
						break;
					case 2:
				        ssd1306_glyph(&oled, wave_20x20_u, 20, 20, 105, 10);
				        break;
					default:
						break;
                }
                ssd1306_glyph(&oled, frame_20x20, 20, 20, 105, 31);
                sprintf(str, "%X", esq.brush);
                ssd1306_print_string(&oled, 111, 38, str, 0, 0);
				if(line > 1) line = 0;
                ssd1306_corners(&oled, 104 + 12*line, 0, 9, 8);
                
                for(int i = 0; i < 16; ++i)
                {
                    ssd1306_progress_bar(&oled, esq.o[selected].data[i].velocity, 5 + i*6, 10, 0x7F, 40, 5, true);
                }
                sprintf(str, "\x81\x84\x81\x84\x81\x84\x81\x84\x81\x84\x81\x84\x81\x84\x81");
                str[selected*2] = '\x82';
                ssd1306_print_string(&oled, 4, 56, str, 0, 0);
                repaint = false;
                refresh = true;
            break;
            
			case PAGE_DRTN:
                ssd1306_buffer_fill_pixels(&oled, BLACK);
                ssd1306_print_string(&oled, 4, 0, "DURATION", 0, 0);
                ssd1306_print_char(&oled, 105, 1, esq.o[selected].sift[3] ? 0xA0 : 0xA1, false);
                ssd1306_print_char(&oled, 117, 1, esq.o[selected].permute[3] ? 0x9E : 0x9F, false);
                switch(esq.form)
                {
                	case 0:
     					ssd1306_glyph(&oled, rise_20x20_u, 20, 20, 105, 10);
                		break;
					case 1:
						ssd1306_glyph(&oled, fall_20x20_u, 20, 20, 105, 10);
						break;
					case 2:
				        ssd1306_glyph(&oled, wave_20x20_u, 20, 20, 105, 10);
				        break;
					default:
						break;
                }
                ssd1306_glyph(&oled, frame_20x20, 20, 20, 105, 31);
				sprintf(str, "%X", esq.brush);
                ssd1306_print_string(&oled, 111, 38, str, 0, 0);                
                if(line > 1) line = 0;
                ssd1306_corners(&oled, 104 + 12*line, 0, 9, 8);
                
                for(int i = 0; i < 16; ++i)
                {
                    ssd1306_progress_bar(&oled, esq.o[selected].data[i].value, 5 + i*6, 10, 0xFF, 40, 5, true);
                }
                sprintf(str, "\x81\x84\x81\x84\x81\x84\x81\x84\x81\x84\x81\x84\x81\x84\x81");
                str[selected*2] = '\x82';
                ssd1306_print_string(&oled, 4, 56, str, 0, 0);
                repaint = false;
                refresh = true;
            break;
            
            case PAGE_FFST:
                ssd1306_buffer_fill_pixels(&oled, BLACK);
                ssd1306_print_string(&oled, 4, 0, "OFFSET", 0, 0);
                ssd1306_print_char(&oled, 105, 1, esq.o[selected].sift[4] ? 0xA0 : 0xA1, false);
                ssd1306_print_char(&oled, 117, 1, esq.o[selected].permute[4] ? 0x9E : 0x9F, false);
				switch(esq.form)
                {
                	case 0:
     					ssd1306_glyph(&oled, rise_20x20_u, 20, 20, 105, 10);
                		break;
					case 1:
						ssd1306_glyph(&oled, fall_20x20_u, 20, 20, 105, 10);
						break;
					case 2:
				        ssd1306_glyph(&oled, wave_20x20_u, 20, 20, 105, 10);
				        break;
					default:
						break;
                }
				ssd1306_glyph(&oled, frame_20x20, 20, 20, 105, 31);
				sprintf(str, "%X", esq.brush);
                ssd1306_print_string(&oled, 111, 38, str, 0, 0);
				if(line > 1) line = 0;
                ssd1306_corners(&oled, 104 + 12*line, 0, 9, 8);
                for(int i = 0; i < 16; ++i)
                {
                    ssd1306_progress_bar(&oled, esq.o[selected].data[i].offset, 5 + i*6, 10, 0x20, 40, 5, true);
                }
                sprintf(str, "\x81\x84\x81\x84\x81\x84\x81\x84\x81\x84\x81\x84\x81\x84\x81");
                str[selected*2] = '\x82';
                ssd1306_print_string(&oled, 4, 56, str, 0, 0);
                repaint = false;
                refresh = true;
            break;

            case PAGE_SIEV:
                ssd1306_buffer_fill_pixels(&oled, BLACK);
                ssd1306_print_string(&oled, 4,   0, "SIEVE", 0, 0);
                ssd1306_print_char(&oled, 105, 1, esq.o[selected].regenerate ? 0xA2 : 0xA3, false);
                ssd1306_print_char(&oled, 114, 1, esq.o[selected].permute[5] ? 0x9E : 0x9F, false);
                for(int i = 0; i < 8; ++i)
                {
                	ssd1306_glyph(&oled, frame_13x24, 13, 24, 5 + 15*i, 12);
                }
                for(int i = 0; i < 8/*esq.o[selected].gaps*/; ++i)
                {
                    int l = esq.o[selected].sieve[i];
                    sprintf(str, "%2d", l);
                    ssd1306_print_string(&oled, i*15 + 8, 16 - (l < 10 ? 4 : 0), str, false, true);
                }
                sprintf(str, "\x81\x84\x81\x84\x81\x84\x81\x84\x81\x84\x81\x84\x81\x84\x81");
                str[selected * 2] = '\x82';
                ssd1306_print_string(&oled, 4, 56, str, 0, 0);
                repaint = false;
                refresh = true;
            break;
            
            case PAGE_SAVE:
                ssd1306_buffer_fill_pixels(&oled, BLACK);
                ssd1306_print_string(&oled, 0, 0, "SAVE", 0, 0);

                sprintf(str, "[ %s ]", filename);
				ssd1306_print_string(&oled, 35, 0, str, 0, 0);
				
				ssd1306_line(&oled, 35 + 16 + 8*print_pos, 10, 8, false);
				ssd1306_line(&oled, 35 + 16 + 8*print_pos, 11, 8, false);

				lfs_ls(&fs, "", list, selected_file);

				sprintf(str, "[              ]");
				ssd1306_print_string(&oled, 0, 36, str, 0, 0);
				
                for(int i = 0; i < 5; ++i)
                {
                	sprintf(str, "%8s", list[i]);
                	ssd1306_print_string(&oled, 32, 18 + i*9, str, 0, 0);
                }				
                
                repaint = false;
                refresh = true;
            break;

            case PAGE_LOAD:
                ssd1306_buffer_fill_pixels(&oled, BLACK);
                ssd1306_print_string(&oled, 0, 0, "LOAD", 0, 0);

                lfs_ls(&fs, "", list, selected_file);
                
				sprintf(str, "[ %s ]", list[2]);
				ssd1306_print_string(&oled, 35, 0, str, 0, 0);
				
				sprintf(str, "[              ]");
				ssd1306_print_string(&oled, 0, 36, str, 0, 0);
				
                for(int i = 0; i < 5; ++i)
                {
                	sprintf(str, "%8s", list[i]);
                	ssd1306_print_string(&oled, 32, 18 + i*9, str, 0, 0);
                }
                repaint = false;
                refresh = true;
            break;

            default:
            break;
            }
        }
        else if(refresh)
        {
            ssd1306_set_pixels(&oled);
            refresh = false;
        }
        // ~REPAINT ///////////////////////////////////////////////////////////////////////////////////////////////////////////
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }
    multicore_fifo_clear_irq(); // Clear interrupt
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Core 1 Main Code ///////////////////////////////////////////////////////////////////////////////////////////////////////////
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
    init();
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    absolute_time_t tts[TRACKS]; // Trigger ON 
    absolute_time_t gts[TRACKS]; // Gate OFF
    bool gate[TRACKS];           // OFF is pending
    arm(100, tts);
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // CORE 0 Loop ////////////////////////////////////////////////////////////////////////////////////////////////////////////
    while (true) 
	{
        tud_task();
        switch(esq.state)
        {
        case PLAY:
        for(int i = 0; i < TRACKS; ++i)
        {
            if(esq.o[i].data[esq.o[i].current].recount) 
            {
                note_from_degree(&esq.o[i].scale, &esq.o[i].data[esq.o->current]);
                esq.o[i].data[esq.o[i].current].recount = false;
            }
            ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // NOTE ON Timings ////////////////////////////////////////////////////////////////////////////////////////////////
            if(time_reached(tts[i]))
            {
                loop_sequence[esq.o[i].mode](&esq.o[i]); // Loop to next step
                tts[i] = make_timeout_time_ms(esq.o[i].step + esq.o[i].data[esq.o[i].current].offset); // Next step timer
                if((esq.o[i].trigger>>esq.o[i].current)&1)  // If current step is trigger
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
        break;

        case PAUSE:
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
        break;

        case STOP:
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
        break;

        default:
        break;
        }
        multicore_fifo_push_blocking(0);
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Send MIDI message //////////////////////////////////////////////////////////////////////////////////////////////////////////
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
        uint8_t on[3] = { status|id, esq.o[id].data[esq.o[id].current].chroma, velocity };
        _send_note(on);
        tud_midi_stream_write(cable_num, on, 3);
    break;

    default:
    break;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Prepare to play routine ////////////////////////////////////////////////////////////////////////////////////////////////////
inline static void arm(uint lag, absolute_time_t* restrict t) 
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

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Loop led run ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline static void swith_led(const int* restrict track)
{
    static int led;
    _74HC595_set_all_low(&sr);
    if((esq.o[*track].trigger>>led)&1) pset(&sr, led&3, led>>2, 4);
    if(++led > STEPS) 
    {
        led = 0;
        _74HC595_set_all_low(&sr);
        pset(&sr, esq.o[*track].current&3, esq.o[*track].current>>2, 2);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Scale representation ///////////////////////////////////////////////////////////////////////////////////////////////////////
inline static void scale_led(const int* restrict track)
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


static void init()
{
	stdio_init_all();
    board_init();
    tusb_init();
	multicore_launch_core1(core1_entry);
    set_sys_clock_khz(150000, true);
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // OLED Init //////////////////////////////////////////////////////////////////////////////////////////////////////////////
    i2c_init(i2c1, 3200000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);
    ssd1306_init(&oled, 0x3C, i2c1, BLACK);
    ssd1306_set_full_rotation(&oled, 0);
    // MIDI DIN Init //////////////////////////////////////////////////////////////////////////////////////////////////////////
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    // 595 Init ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    shift_register_74HC595_init(&sr, SPI_PORT, DATA_595, CLOCK_595, LATCH_595);
    gpio_set_outover(DATA_595, GPIO_OVERRIDE_INVERT); 
    _74HC595_set_all_low(&sr);
    // 4067/keypad Init ///////////////////////////////////////////////////////////////////////////////////////////////////////
    _4067_init();
    keypad_init();
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Sequencer Init /////////////////////////////////////////////////////////////////////////////////////////////////////////
    srand(time_us_32());         // Random seed
    sequencer_init(&esq, 120);
    //for(int i = 0; i < TRACKS; ++i) sequencer_rand(&esq, i);
    format(&fs);
    load_file(&fs, &init_f, "INIT", &esq);
    files = get_file_count(&fs, "");
    // multicore_fifo_push_blocking(0);
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
