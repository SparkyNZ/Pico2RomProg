#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include <stdio.h>

// Control Lines
static const uint PIN_PWE  = 4;   // /PWE_ROM
static const uint PIN_POE  = 18;  // /POE_ROM
static const uint PIN_PCS  = 22;  // /PCS_ROM
static const uint PIN_PROG = 34;  // PROG (Bus isolation)

// Flash Address Pins (PA0..PA18)
static const uint ADDR_PINS[19] = {
    32, 25, 27, 29, 31, 33, 35, 37, 12, 14, 20, 16, 39, 10, 8, 41, 43, 6, 45
};

// Flash Data Pins (PD0..PD7)
static const uint DATA_PINS[8] = {
    30, 28, 26, 23, 21, 19, 17, 24
};

void set_address(uint32_t addr) {
    for (int i = 0; i < 19; i++) {
        gpio_put(ADDR_PINS[i], (addr >> i) & 1);
    }
}

void set_data_dir(bool output) {
    for (int i = 0; i < 8; i++) {
        gpio_set_dir(DATA_PINS[i], output ? GPIO_OUT : GPIO_IN);
    }
}

void write_data_bus(uint8_t data) {
    for (int i = 0; i < 8; i++) {
        gpio_put(DATA_PINS[i], (data >> i) & 1);
    }
}

uint8_t read_data_bus() {
    uint8_t data = 0;
    for (int i = 0; i < 8; i++) {
        if (gpio_get(DATA_PINS[i])) {
            data |= (1 << i);
        }
    }
    return data;
}

void flash_write_cycle(uint32_t addr, uint8_t data) {
    set_address(addr);
    write_data_bus(data);
    set_data_dir(true);

    gpio_put(PIN_PCS, 0);
    gpio_put(PIN_PWE, 0);
    sleep_us(1);

    gpio_put(PIN_PWE, 1);
    gpio_put(PIN_PCS, 1);
    set_data_dir(false);
}

void sst39_write_cmd(uint32_t addr, uint8_t data) {
    flash_write_cycle(addr, data);
}

void sst39_chip_erase() {
    sst39_write_cmd(0x5555, 0xAA);
    sst39_write_cmd(0x2AAA, 0x55);
    sst39_write_cmd(0x5555, 0x80);
    sst39_write_cmd(0x5555, 0xAA);
    sst39_write_cmd(0x2AAA, 0x55);
    sst39_write_cmd(0x5555, 0x10);
    sleep_ms(100);
}

void sst39_program_byte(uint32_t addr, uint8_t byte) {
    sst39_write_cmd(0x5555, 0xAA);
    sst39_write_cmd(0x2AAA, 0x55);
    sst39_write_cmd(0x5555, 0xA0);
    flash_write_cycle(addr, byte);
    sleep_us(20);
}

void enter_programming_mode() {
    gpio_init(PIN_PCS); gpio_set_dir(PIN_PCS, GPIO_OUT); gpio_put(PIN_PCS, 1);
    gpio_init(PIN_POE); gpio_set_dir(PIN_POE, GPIO_OUT); gpio_put(PIN_POE, 1);
    gpio_init(PIN_PWE); gpio_set_dir(PIN_PWE, GPIO_OUT); gpio_put(PIN_PWE, 1);

    for (int i = 0; i < 19; i++) {
        gpio_init(ADDR_PINS[i]);
        gpio_set_dir(ADDR_PINS[i], GPIO_OUT);
        gpio_put(ADDR_PINS[i], 0);
    }

    for (int i = 0; i < 8; i++) {
        gpio_init(DATA_PINS[i]);
        gpio_set_dir(DATA_PINS[i], GPIO_IN);
    }

    gpio_init(PIN_PROG);
    gpio_set_dir(PIN_PROG, GPIO_OUT);
    gpio_put(PIN_PROG, 1);
    sleep_ms(1);
}

void release_bus() {
    gpio_set_dir(PIN_PCS, GPIO_IN);
    gpio_set_dir(PIN_POE, GPIO_IN);
    gpio_set_dir(PIN_PWE, GPIO_IN);

    for (int i = 0; i < 19; i++) gpio_set_dir(ADDR_PINS[i], GPIO_IN);
    for (int i = 0; i < 8; i++)  gpio_set_dir(DATA_PINS[i], GPIO_IN);

    gpio_put(PIN_PROG, 0);
}

int main() {
    stdio_init_all();

    // 2-second delay so you can open/see PuTTY after a reset
    sleep_ms(2000); 
    printf("\n--- SST39VF040 Programmer Starting ---\n");

    const uint8_t *payload = (const uint8_t *)0x10080000;

    enter_programming_mode();
    
    printf("Erasing SST39VF040...\n");
    sst39_chip_erase();

    printf("Programming 32KB payload...\n");
    for (uint32_t i = 0; i < 32768; i++) {
        sst39_program_byte(i, payload[i]);

        if ((i % 4096) == 0) {
            printf("Programmed %u / 32768 bytes...\n", i);
        }
    }

    release_bus();
    printf("Programming complete! SBUS2 bus released.\n");

    // Heartbeat to confirm the MCU hasn't crashed
    while (1) {
        printf("Waiting...\n");
        sleep_ms(2000);
    }
}
