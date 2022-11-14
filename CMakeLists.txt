cmake_minimum_required(VERSION 3.12)

include(pico_sdk_import.cmake)
include(pico_extras_import.cmake)

project(calculator)
pico_sdk_init()

add_executable(calculator
	calculator.c
)

add_subdirectory(pico-ss-oled build/ssoled)
add_subdirectory(littlefs build/littlefs)
add_subdirectory(cell build/cell)
add_subdirectory(ShiftRegister74HC595-Pico build/ShiftRegister74HC595-Pico)

# enable usb output, disable uart output
    pico_enable_stdio_usb (calculator 1)
    pico_enable_stdio_uart(calculator 0)

target_link_libraries(calculator PRIVATE
	ShiftRegister74HC595
	pico_stdlib
	cell
	pico_multicore
	hardware_i2c
	hardware_spi
	hardware_flash
	pico_ss_oled
	littlefs
)


target_compile_definitions(calculator PRIVATE

    PICO_NO_FPGA_CHECK=1

)

set(PICO_DEFAULT_FLOAT_IMPL pico_float_pico)

target_compile_options(calculator PRIVATE

    -O3
	-march=armv6-m
	-mcpu=cortex-m0plus
	-mtune=cortex-m0plus
	-mfloat-abi=softfp

)

pico_set_binary_type(calculator copy_to_ram)

pico_add_extra_outputs(calculator)