#pragma once
#include "hardware/clocks.h"
#include "hardware/structs/clocks.h"
#include "cell/midi_uart.h"

sequencer esq;
volatile bool fired[tracks];
uint16_t point[tracks];
volatile int f = 0;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Note ON Callbacks /////////////////////////////////////////////////////////////////////////////////////////
// int64_t callback_a(alarm_id_t id, void *user_data);
// int64_t callback_b(alarm_id_t id, void *user_data);

// int64_t (*timer_callback[])(alarm_id_t, void*) =
// {
//     callback_a,
//     callback_b

// };

int64_t callback_a(alarm_id_t id, void *user_data) 
{
    fired[0] = true;
    f++; if(f>15) f = 0;
    return id;
}

// int64_t callback_b(alarm_id_t id, void *user_data) 
// {
//     add_alarm_in_ms(1000, timer_callback[0], NULL, false); 
//     f++; if(f>15) f = 0;
//     return id;
// }




