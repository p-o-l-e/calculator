cmake_minimum_required(VERSION 3.12)

add_library(keypad INTERFACE)

target_sources(keypad INTERFACE

    ${CMAKE_CURRENT_LIST_DIR}/keypad.c
)