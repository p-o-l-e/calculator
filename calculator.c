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
uint8_t selected_track = 0; // Blinking track
uint16_t cable_num = 0;
////////////////////////////////////////////////////////////////////////////////////
// Core 1 interrupt Handler ////////////////////////////////////////////////////////
void core1_interrupt_handler() 
{
    int led = 0;
    while (multicore_fifo_rvalid())
    {
        uint16_t raw = multicore_fifo_pop_blocking();  
        if(refresh_display)
        {
            ssd1306_clear(&oled);
            // ssd1306_print_string(&oled, 0, 0, "ABCDEFGHIJKLMNO");
            // ssd1306_print_string(&oled, 0, 8, "[0123456789]:[]");
            ssd1306_show(&oled);
            refresh_display = false;
        }
        if(raw >= 0)
        {
            // SEND MIDI
            _send_note_on(&esq.o[raw]);
            uint8_t note_on[3] = { 0x80 | esq.o[raw].channel, esq.o[raw].data->pitch, esq.o[raw].data->velocity };
            tud_midi_stream_write(cable_num, note_on, 3);
        }


        _74HC595_set_all_low(&sr);
        if(esq.o[selected_track].triggers & (0x8000>>led)) pset(&sr, led%4, led/4, 2);
        led++;
        if(led > steps) 
        {
            led = 0;
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
    sequencer_init(&esq, 120);
    reset_timestamp(&esq, 1, 40);
    refresh_display = true;
    absolute_time_t ts[tracks];
    ts[0] = make_timeout_time_ms(300);
    ts[1] = make_timeout_time_ms(350);
    esq.o[0].triggers = 0b1111000011110100;
    // for(int i = 0; i < steps; i++)
    // esq.o[0].timestamp[i] = 10 * i;
    ////////////////////////////////////////////////////////////////////////////////
    // CORE 0 Loop /////////////////////////////////////////////////////////////////
    while (true) 
	{
        uint16_t raw = -1;
        for(int i = 0; i < tracks; i++)
        {
            if(time_reached(ts[i]))
            {
                ts[i] = make_timeout_time_ms(esq.o[i].step + esq.o[i].data->offset);
                loop_sequence[esq.o[i].mode](&esq.o[i]);
                raw = i;
            }
        }
        multicore_fifo_push_blocking(raw);
    }
    return 0;
}