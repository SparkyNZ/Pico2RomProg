cmake_minimum_required(VERSION 3.13)
set(PICO_SDK_VERSION_REQUIRED "2.0.0")

set(ENV{PICO_TOOLCHAIN_PATH} "C:/Program Files (x86)/Arm GNU Toolchain arm-none-eabi/13.2 Rel1")
set(PICO_BOARD pico2 CACHE STRING "Board type")

include(pico_sdk_import.cmake)

project(Pico2RomProg C CXX ASM)
pico_sdk_init()

add_executable(Pico2RomProg main.c)

target_include_directories(Pico2RomProg PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
pico_set_program_name(Pico2RomProg "Pico2RomProg")

target_link_libraries(Pico2RomProg pico_stdlib)
pico_add_extra_outputs(Pico2RomProg)
