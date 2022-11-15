#include "hardware/clocks.h"
#include "hardware/structs/clocks.h"
#include "cell/midi_uart.h"

sequencer o[_tracks_];
volatile bool fired[_tracks_];
uint16_t point[_tracks_];

int64_t (*timer_callback[])(alarm_id_t, void*);
 
int64_t callback_on_a(alarm_id_t id, void *user_data) 
{
    fired[0] = true;
    _send_note_on(&o[0]);
    add_alarm_in_ms(o[0].timestamp[o->current][0], timer_callback[0], NULL, true);
    loop_sequence[o[0].direction](&o[0]);
    point[0] = get_point(o[0].current%4, o[0].current/4, 0);
    return 0;
}

int64_t callback_on_b(alarm_id_t id, void *user_data) 
{
    fired[1] = true;
    _send_note_on(&o[1]);
    add_alarm_in_ms(o[1].timestamp[o->current][0], timer_callback[1], NULL, true);
    loop_sequence[o[1].direction](&o[1]);
    point[1] = get_point(o[1].current%4, o[1].current/4, 0);
    return 0;
}

int64_t callback_on_c(alarm_id_t id, void *user_data) 
{
    fired[2] = true;
    _send_note_on(&o[2]);
    add_alarm_in_ms(o[2].timestamp[o->current][0], timer_callback[2], NULL, true);
    loop_sequence[o[2].direction](&o[2]);
    point[2] = get_point(o[2].current%4, o[2].current/4, 0);
    return 0;
}

int64_t callback_on_d(alarm_id_t id, void *user_data) 
{
    fired[3] = true;
    _send_note_on(&o[3]);
    add_alarm_in_ms(o[3].timestamp[o->current][0], timer_callback[3], NULL, true);
    loop_sequence[o[3].direction](&o[3]);
    point[3] = get_point(o[3].current%4, o[3].current/4, 0);
    return 0;
}

int64_t callback_on_e(alarm_id_t id, void *user_data) 
{
    fired[4] = true;
    _send_note_on(&o[4]);
    add_alarm_in_ms(o[4].timestamp[o->current][0], timer_callback[4], NULL, true);
    loop_sequence[o[4].direction](&o[4]);
    point[4] = get_point(o[4].current%4, o[4].current/4, 0);
    return 0;
}

int64_t callback_on_f(alarm_id_t id, void *user_data) 
{
    fired[5] = true;
    _send_note_on(&o[5]);
    add_alarm_in_ms(o[5].timestamp[o->current][0], timer_callback[5], NULL, true);
    loop_sequence[o[5].direction](&o[5]);
    point[5] = get_point(o[5].current%4, o[5].current/4, 0);
    return 0;
}

int64_t callback_on_g(alarm_id_t id, void *user_data) 
{
    fired[6] = true;
    _send_note_on(&o[6]);
    add_alarm_in_ms(o[6].timestamp[o->current][0], timer_callback[6], NULL, true);
    loop_sequence[o[6].direction](&o[6]);
    point[6] = get_point(o[6].current%4, o[6].current/4, 0);
    return 0;
}

int64_t callback_on_h(alarm_id_t id, void *user_data) 
{
    fired[7] = true;
    _send_note_on(&o[7]);
    add_alarm_in_ms(o[7].timestamp[o->current][0], timer_callback[7], NULL, true);
    loop_sequence[o[7].direction](&o[7]);
    point[7] = get_point(o[7].current%4, o[7].current/4, 0);
    return 0;
}

int64_t (*timer_callback[])(alarm_id_t, void*) =
{
    callback_on_a,
    callback_on_b,
    callback_on_c,
    callback_on_d,
    callback_on_e,
    callback_on_f,
    callback_on_g,
    callback_on_h

};