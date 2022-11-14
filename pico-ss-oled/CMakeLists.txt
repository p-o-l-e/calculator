cmake_minimum_required(VERSION 3.12)

# include(pico_sdk_import.cmake)
# include(pico_extras_import.cmake)


add_library(pico_ss_oled INTERFACE)

target_sources(pico_ss_oled INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/BitBang_I2C.c
  ${CMAKE_CURRENT_LIST_DIR}/ss_oled.c
)

target_include_directories(pico_ss_oled INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

# Pull in pico libraries that we need
# target_link_libraries(pico_ss_oled INTERFACE pico_stdlib hardware_i2c pico_audio_i2s)

