cmake_minimum_required(VERSION 3.13)

set(FAMILY rp2040)
set(BOARD pico_sdk)
set(PICO_SDK_PATH /home/alayalava/git/pico-sdk)
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)
include(${PICO_SDK_PATH}/lib/tinyusb/hw/bsp/${FAMILY}/family.cmake)

project(calculator C CXX ASM)


set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(calculator
	calculator.c
	usb_descriptors.c
)

pico_enable_stdio_usb(calculator 0)
pico_enable_stdio_uart(calculator 1)

target_include_directories(calculator PRIVATE ${CMAKE_CURRENT_LIST_DIR})

#add_subdirectory(pico-ss-oled build/ssoled)
add_subdirectory(pico-ssd1306 build/ssd1306)
add_subdirectory(littlefs build/littlefs)
add_subdirectory(cell build/cell)
add_subdirectory(pico-keypad build/keypad)
add_subdirectory(ShiftRegister74HC595-Pico build/ShiftRegister74HC595-Pico)



target_link_libraries(calculator PRIVATE
	ShiftRegister74HC595
	pico_stdlib
	tinyusb_device 
	tinyusb_board
	cell
	keypad
	pico_multicore
	hardware_i2c
	hardware_spi
	hardware_flash
	SSD1306
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