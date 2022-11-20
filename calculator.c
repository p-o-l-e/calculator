#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/binary_info.h"
#include "bsp/board.h"
#include "tusb.h"

// #include "pico-ss-oled/include/ss_oled.h"
#include "pico-ssd1306/ssd1306.h"
#include "pico/multicore.h"
#include "CD74HC4067.h"
#include "ShiftRegister74HC595-Pico/sr_common.h"
#include "interface.h"
#include "suspend.h"

const uint LED_PIN = PICO_DEFAULT_LED_PIN;



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

////////////////////////////////////////////////////////////////////////////////////
// Core 1 interrupt Handler ////////////////////////////////////////////////////////
void core1_interrupt_handler() 
{
    
    while (multicore_fifo_rvalid())
    {
        uint16_t raw = multicore_fifo_pop_blocking();  
        if(refresh_display)
        {
            ssd1306_clear(&oled);
            // ssd1306_print_string(&oled, 0, 0, "ABCDEFGHIJKLMNO");
            // ssd1306_print_string(&oled, 0, 8, "[0123456789]:[]");
            // ssd1306_show(&oled);
            refresh_display = false;
        }
        if(raw == 1)
        {
            // _74HC595_set_all_low(&sr);
            pset(&sr, esq.o[0].current%4, esq.o[0].current/4, 2);
            // f++; if(f>15) f = 0;
            // set_bits(&sr, point[0]);
            // set_bits(&sr, rand_in_range(0, 0b1111111111111111));
            // sleep_ms(1000);

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
    i2c_init(i2c1, 400000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);
    oled.external_vcc=false;
    ssd1306_init(&oled, 128, 64, 0x3C, i2c1);
    ssd1306_clear(&oled);
    // MIDI DIN Init //////////////////////////////////////////////////////////
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    // 595 Init ///////////////////////////////////////////////////////////////
    shift_register_74HC595_init(&sr, SPI_PORT, DATA_595, CLOCK_595, LATCH_595);
    gpio_set_outover(DATA_595, GPIO_OVERRIDE_INVERT); 
    _74HC595_set_all_low(&sr);
    // Sequencer Init /////////////////////////////////////////////////////////
    // track_init(&esq.o[0], 300);
    fired[0] = true;

    refresh_display = true;
    absolute_time_t ts = make_timeout_time_ms(300);
    for(int i = 0; i < steps; i++)
    esq.o[0].timestamp[i] = 10 * i;
    
    while (true) 
	{
        

        if(time_reached(ts))
        {
            ts = make_timeout_time_ms(esq.o[0].timestamp[esq.o[0].current]);
            loop_sequence[esq.o[0].mode](&esq.o[0]);
        }

        uint16_t raw = 1;
        multicore_fifo_push_blocking(raw);

    }
    return 0;
}




// uint8_t note_off[3] = { 0x80 | channel, note_sequence[previous], 0};
// tud_midi_stream_write(cable_num, note_off, 3);