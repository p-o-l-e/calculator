cmake_minimum_required(VERSION 3.12)

add_library(encoder INTERFACE)

target_sources(encoder INTERFACE

    ${CMAKE_CURRENT_LIST_DIR}/encoder.c
)