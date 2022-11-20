cmake_minimum_required(VERSION 3.12)

add_library(SSD1306 INTERFACE)

target_sources(SSD1306 INTERFACE

    ${CMAKE_CURRENT_LIST_DIR}/ssd1306.c
)