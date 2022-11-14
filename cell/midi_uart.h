#pragma once
#include "midi.h"
#include "pico/stdlib.h"
#include "hardware/uart.h"

#define UART_ID     uart0
#define BAUD_RATE   31250
#define UART_TX_PIN 0
#define UART_RX_PIN 1

void _send_mmsg(mmsg* msg, UMPstatus c)
{
    uart_putc_raw(UART_ID, (uint8_t)c + msg->channel);
    uart_putc_raw(UART_ID, msg->msb);
    uart_putc_raw(UART_ID, msg->lsb);
}