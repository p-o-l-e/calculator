cmake_minimum_required(VERSION 3.12)

add_library(74HC595 INTERFACE)

target_sources(74HC595 INTERFACE

    ${CMAKE_CURRENT_LIST_DIR}/74HC595.c

)

pico_generate_pio_header(74HC595 ${CMAKE_CURRENT_LIST_DIR}/shift_reg.pio)
