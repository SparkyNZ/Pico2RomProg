# RP2350 / 6502 ROM Emulator Cheat Sheet

## Environment & Prerequisites

### Visual Studio 2019 Developer Command Prompt
Launch the environment using the x86 or x64 Native Tools Command Prompt:
```cmd
"C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat"
```
*(Adjust the directory above if using `Professional` or `Enterprise` editions).*

---

## Hardware Mappings

### 1. 6502 Address Bus to RP2350 GPIO
| 6502 Address Line | RP2350 GPIO Pin | Decoding Logic (`main.c`) |
| :--- | :--- | :--- |
| **A0 – A3** | GP13 – GP16 | `((gpio_raw >> 13) & 0x0F) << 0` |
| **A4** | GP23 | `((gpio_raw >> 23) & 0x01) << 4` |
| **A5 – A9** | GP18 – GP22 | `((gpio_raw >> 18) & 0x1F) << 5` |
| **A10** | GP17 | `((gpio_raw >> 17) & 0x01) << 10` |
| **A11** | GP24 | `((gpio_raw >> 24) & 0x01) << 11` |
| **A12** | GP25 | `((gpio_raw >> 25) & 0x01) << 12` |
| **A13** | GP26 | `((gpio_raw >> 26) & 0x01) << 13` |
| **A14** | GP27 | `((gpio_raw >> 27) & 0x01) << 14` |
| **A15** | GP28 | `((gpio_raw >> 28) & 0x01) << 15` |

### 2. Control Lines & Data Bus
* **Data Bus (D0 - D7)**: GP0 – GP7 (GP2 – GP7 when `DEBUG_OUTPUT` is active on UART)
* **Bus Timing (`PHI2`)**: GP8 (Monitored by PIO via `wait 1 gpio 8`)
* **Output Enable (`/OE`)**: GP10 (Monitored by PIO via `jmp pin inactive`)
* **Target System Reset (`/RES`)**: GP30 (Monitored in `main.c` to reload RAM array from Flash)

---

## Memory Map

* **Base Flash Execution (XIP)**: `0x10000000`
* **6502 ROM Binary Flash Offset**: `0x10080000` (512KB offset into Flash)
* **Target System Memory Space**: 64KB ($0000–$FFFF)

---

## Essential Commands & Terminal Operations

### 1. Build & Deploy RP2350 Firmware (NMake)
Run inside the VS2019 Developer Command Prompt:
```cmd
cd C:\pico\pico-projects\Pico2Rom
mkdir build
cd build
cmake -G "NMake Makefiles" ..
nmake
openocd -f interface/cmsis-dap.cfg -f target/rp2350.cfg -c "adapter speed 5000" -c "program Pico2Rom.elf verify reset exit"
```

### 2. Assemble & Flash 6502 Binary over SWD (5 MHz)
Use the build_test.bat for reference. build_test8000.bat builds the same source but to a different location.
```cmd
c:\cc65\bin\ca65 test.asm -l test.lst
c:\cc65\bin\ld65 -C rom64k.cfg test.o -o test.bin
openocd -f interface/cmsis-dap.cfg -f target/rp2350.cfg -c "adapter speed 5000" -c "program test.bin 0x10080000 verify reset exit"
```

---

## Minimal Project Reference Code

### `CMakeLists.txt` (Zero-Overhead Setup)
```cmake
cmake_minimum_required(VERSION 3.13)
set(PICO_SDK_VERSION_REQUIRED "2.0.0")
set(PICO_BOARD pico2 CACHE STRING "Board type")

include(pico_sdk_import.cmake)
project(Pico2Rom C CXX ASM)
pico_sdk_init()

option(DEBUG_OUTPUT "Enable UART output on GP0/GP1" OFF)
add_executable(Pico2Rom main.c)

if(DEBUG_OUTPUT)
    add_compile_definitions(DEBUG_OUTPUT)
    pico_enable_stdio_usb(Pico2Rom 0)
    pico_enable_stdio_uart(Pico2Rom 1)
endif()

pico_generate_pio_header(Pico2Rom ${CMAKE_CURRENT_LIST_DIR}/rom.pio)
target_include_directories(Pico2Rom PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(Pico2Rom pico_stdlib hardware_pio)
pico_add_extra_outputs(Pico2Rom)
```

### `rom64k.cfg` (cc65 Linker Config)
```text
memory {
    ROM: start = $0000, size = $10000, fill = no;
}
segments {
    CODE: load = ROM, type = ro;
}
```