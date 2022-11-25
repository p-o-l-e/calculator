cmake_minimum_required(VERSION 3.12)

add_library(ShiftRegister74HC595 INTERFACE)

target_sources(ShiftRegister74HC595 INTERFACE

    ${CMAKE_CURRENT_LIST_DIR}/ShiftRegister74HC595.c

)

pico_generate_pio_header(ShiftRegister74HC595 ${CMAKE_CURRENT_LIST_DIR}/shift_reg.pio)
