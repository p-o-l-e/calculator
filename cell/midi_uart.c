#include "midi_uart.h"

void _send_note_on(track* o)
{
    uart_putc_raw(UART_ID, 0x90 | o->channel);
    uart_putc_raw(UART_ID, o->data[o->current].chroma);
    uart_putc_raw(UART_ID, o->data[o->current].velocity);
}


void _send_note_off(track* o)
{
    uart_putc_raw(UART_ID, 0x80 | o->channel);
    uart_putc_raw(UART_ID, o->data[o->current].chroma);
    uart_putc_raw(UART_ID, o->data[o->current].velocity);
}

void _send_note(uint32_t data)
{
    uart_putc_raw(UART_ID, ((data>>16)&0xFF));
    uart_putc_raw(UART_ID, ((data>> 8)&0xFF));
    uart_putc_raw(UART_ID, ((data    )&0xFF));
}