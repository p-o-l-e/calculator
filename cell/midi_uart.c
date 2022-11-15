#include "midi_uart.h"

void _send_note_on(sequencer* o)
{
    uart_putc_raw(UART_ID, 0x90 + o->channel);
    uart_putc_raw(UART_ID, o->data[o->current].pitch);
    uart_putc_raw(UART_ID, o->data[o->current].velocity);
}


void _send_note_off(sequencer* o)
{
    uart_putc_raw(UART_ID, 0x80 + o->channel);
    uart_putc_raw(UART_ID, o->data[o->current].pitch);
    uart_putc_raw(UART_ID, o->data[o->current].velocity);
}