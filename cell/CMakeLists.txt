cmake_minimum_required(VERSION 3.12)

add_library(cell INTERFACE)

target_sources(cell INTERFACE

    ${CMAKE_CURRENT_LIST_DIR}/sequencer.c
    ${CMAKE_CURRENT_LIST_DIR}/utility.c
    ${CMAKE_CURRENT_LIST_DIR}/midi.c
    ${CMAKE_CURRENT_LIST_DIR}/midi_uart.c


)