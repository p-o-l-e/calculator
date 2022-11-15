#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "pico-ss-oled/include/ss_oled.h"
#include "pico/multicore.h"
#include "CD74HC4067.h"
#include "ShiftRegister74HC595-Pico/sr_common.h"
#include "interface.h"

#define SPI_PORT    spi0 

#define SDA_PIN     4
#define SCL_PIN     5

#define CLK_595     18
#define DATA_595    19
#define LATCH_595   16

#define OLED_WIDTH  128
#define OLED_HEIGHT 64

ShiftRegister74HC595 sr;

////////////////////////////////////////////////////////////////////////////////////
// Core 1 interrupt Handler ////////////////////////////////////////////////////////
void core1_interrupt_handler() 
{
    while (multicore_fifo_rvalid())
    {
        uint16_t raw = multicore_fifo_pop_blocking();   
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
	multicore_launch_core1(core1_entry);
    bool clk = set_sys_clock_khz(133000, true);

    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    shift_register_74HC595_init(&sr, SPI_PORT, DATA_595, CLK_595, LATCH_595);

    gpio_set_outover(DATA_595, GPIO_OVERRIDE_INVERT); 

    int note = 0x10;
    for(int i = 0; i < _tracks_; i++)
    {
        fired[i] = true;
    }

    while (true) 
	{
        for(int i = 0; i < 8; i++)
        {
            if(fired[i])
            {
                add_alarm_in_ms(o[i].timestamp[o->current][0], timer_callback[i], NULL, false);
                fired[i] = false;
            }
        }


        for (size_t i = 0; i < 8; i++)
        {
            set_bits(&sr, point[i]);
            _74HC595_set_all_low(&sr);
        }
        

    }
    return 0;
}