cmake_minimum_required(VERSION 3.12)

add_library(595 INTERFACE)

target_sources(595 INTERFACE

    ${CMAKE_CURRENT_LIST_DIR}/rp2040_shift_register.c

)