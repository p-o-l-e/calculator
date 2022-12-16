#pragma once
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "sequencer.h"

#define UART_ID     uart0
#define BAUD_RATE   31250
#define UART_TX_PIN 0
#define UART_RX_PIN 1

void _send_note(uint8_t* restrict data);