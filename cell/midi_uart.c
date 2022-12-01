#include "midi_uart.h"



void _send_note(uint8_t* data)
{
    uart_putc_raw(UART_ID, data[0]);
    uart_putc_raw(UART_ID, data[1]);
    uart_putc_raw(UART_ID, data[2]);
}