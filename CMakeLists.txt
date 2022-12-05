cmake_minimum_required(VERSION 3.13)

set(FAMILY rp2040)
set(BOARD pico_sdk)
set(PICO_SDK_PATH /home/alayalava/git/pico/pico-sdk)
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)
include(tinyusb/hw/bsp/family_support.cmake)
include(tinyusb/hw/bsp/${FAMILY}/family.cmake)

project(calculator C CXX ASM)
family_get_project_name(calculator ${CMAKE_CURRENT_LIST_DIR})

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

#pico_sdk_init()
family_initialize_project(calculator ${CMAKE_CURRENT_LIST_DIR}/family)

add_executable(calculator
	calculator.c
	usb_descriptors.c
)

pico_enable_stdio_usb(calculator 0)
pico_enable_stdio_uart(calculator 1)

target_include_directories(calculator PRIVATE ${CMAKE_CURRENT_LIST_DIR})

family_configure_device_example(calculator)

add_subdirectory(SSD1306 build/SSD1306)
add_subdirectory(littlefs build/littlefs)
add_subdirectory(cell build/cell)
add_subdirectory(quadrature-decoder build/quadrature-decoder)
add_subdirectory(74HC595 build/74HC595)



target_link_libraries(calculator PRIVATE
	74HC595
	pico_stdlib
	tinyusb_device 
	tinyusb_board
	tinyusb_additions
	pico_fix_rp2040_usb_device_enumeration
	cell
	quadrature_decoder
	pico_multicore
	hardware_i2c
	hardware_spi
	hardware_pio
	hardware_dma
	hardware_flash
	pico_unique_id
	SSD1306
	littlefs
)

target_compile_definitions(calculator PRIVATE

    PICO_NO_FPGA_CHECK=1
	PICO_TIME_DEFAULT_ALARM_POOL_DISABLED=1
	PICO_RP2040_USB_DEVICE_ENUMERATION_FIX=1
	TUD_OPT_RP2040_USB_DEVICE_ENUMERATION_FIX=1
)

set(PICO_DEFAULT_FLOAT_IMPL pico_float_pico)

target_compile_options(calculator PRIVATE

    -Ofast
	-march=armv6-m
	-mcpu=cortex-m0plus
	-mtune=cortex-m0plus
	-mfloat-abi=softfp
	-DBOARD=raspberry_pi_pico 
	-DFAMILY=rp2040

)

pico_set_double_implementation(calculator none)
pico_set_binary_type(calculator copy_to_ram)
pico_add_extra_outputs(calculator)