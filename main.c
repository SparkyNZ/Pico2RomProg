//-------------------------------------------------------------------------------------------------------------
// RP2350B SST39VF040 Programmer
// Clean 32KB write with robust write timing and polling
//-------------------------------------------------------------------------------------------------------------

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

// Memory location in RP2350 flash
static const uint8_t *payload = (const uint8_t *)0x10080000;

//-------------------------------------------------------------------------------------------------------------
// Low-Level Bus Helpers
//-------------------------------------------------------------------------------------------------------------

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

//-------------------------------------------------------------------------------------------------------------
// Flash Cycle Primitives
//-------------------------------------------------------------------------------------------------------------

void flash_write_cycle(uint32_t addr, uint8_t data) {
    set_address(addr);
    write_data_bus(data);
    set_data_dir(true);
    sleep_us(1);          // Address/Data setup time

    gpio_put(PIN_PCS, 0); // Assert /CE
    gpio_put(PIN_PWE, 0); // Assert /WE
    sleep_us(2);          // tWP min is 100ns (2us gives clean margin)

    gpio_put(PIN_PWE, 1); // De-assert /WE (latches data on rising edge)
    gpio_put(PIN_PCS, 1); // De-assert /CE
    sleep_us(1);          // Hold time

    set_data_dir(false);
}

uint8_t flash_read_cycle(uint32_t addr) {
    set_address(addr);
    set_data_dir(false);
    sleep_us(1);

    gpio_put(PIN_PCS, 0); // Assert /CE
    gpio_put(PIN_POE, 0); // Assert /OE
    sleep_us(2);          // tAA max is 70ns

    uint8_t val = read_data_bus();

    gpio_put(PIN_POE, 1);
    gpio_put(PIN_PCS, 1);
    sleep_us(1);
    return val;
}

//-------------------------------------------------------------------------------------------------------------
// SST39VF040 Commands & Hardware Polling
//-------------------------------------------------------------------------------------------------------------

void sst39_write_cmd(uint32_t addr, uint8_t data) {
    flash_write_cycle(addr, data);
}

void sst39_poll_dq7(uint32_t addr, uint8_t byte) {
    uint8_t expected_dq7 = byte & 0x80;
    uint32_t timeout = 10000;

    while (timeout--) {
        uint8_t read_val = flash_read_cycle(addr);
        if ((read_val & 0x80) == expected_dq7) {
            return; // Programming/erase pass completed
        }
        sleep_us(5);
    }
    printf("Timeout polling DQ7 at address 0x%05X!\n", addr);
}

void check_sst39_id() {
    sst39_write_cmd(0x5555, 0xAA);
    sst39_write_cmd(0x2AAA, 0x55);
    sst39_write_cmd(0x5555, 0x90);
    sleep_us(10);

    uint8_t mfg_id = flash_read_cycle(0x00000);
    uint8_t dev_id = flash_read_cycle(0x00001);

    sst39_write_cmd(0x5555, 0xAA);
    sst39_write_cmd(0x2AAA, 0x55);
    sst39_write_cmd(0x5555, 0xF0);
    sleep_us(10);

    printf("SST39VF040 ID Read: Manufacturer = 0x%02X (Expected 0xBF), Device = 0x%02X (Expected 0xD7)\n", mfg_id, dev_id);
}

void sst39_chip_erase() {
    sst39_write_cmd(0x5555, 0xAA);
    sst39_write_cmd(0x2AAA, 0x55);
    sst39_write_cmd(0x5555, 0x80);
    sst39_write_cmd(0x5555, 0xAA);
    sst39_write_cmd(0x2AAA, 0x55);
    sst39_write_cmd(0x5555, 0x10);

    sst39_poll_dq7(0x0000, 0xFF);
    sleep_ms(50);
}

void sst39_program_byte(uint32_t addr, uint8_t byte) {
    // If byte is 0xFF, skip writing since the chip is already erased to 0xFF
    if (byte == 0xFF) return;

    sst39_write_cmd(0x5555, 0xAA);
    sst39_write_cmd(0x2AAA, 0x55);
    sst39_write_cmd(0x5555, 0xA0);
    flash_write_cycle(addr, byte);

    sst39_poll_dq7(addr, byte);
}

//-------------------------------------------------------------------------------------------------------------
// Bus State Management
//-------------------------------------------------------------------------------------------------------------

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

//-------------------------------------------------------------------------------------------------------------
// Main Execution
//-------------------------------------------------------------------------------------------------------------

int main() {
    stdio_init_all();

    sleep_ms(2000); 
    printf("\n--- SST39VF040 Programmer Starting ---\n");

    enter_programming_mode();
    
    check_sst39_id();

    printf("Erasing SST39VF040...\n");
    sst39_chip_erase();

    printf("Programming 32KB payload...\n");
    for (uint32_t i = 0; i < 32768; i++) {
        sst39_program_byte(i, payload[i]);

        if ((i % 4096) == 0) {
            printf("Programmed %u / 32768 bytes...\n", i);
        }
    }

    printf("\nReading back SST39VF040 for verification...\n");
    uint32_t errors = 0;

    for (uint32_t i = 0; i < 32768; i++) {
        uint8_t read_val = flash_read_cycle(i);
        if (read_val != payload[i]) {
            if (errors < 10) {
                printf("Mismatch at 0x%04X: expected 0x%02X, got 0x%02X\n", i, payload[i], read_val);
            }
            errors++;
        }
    }

    if (errors == 0) {
        printf("Verification SUCCESSFUL! All 32KB matched.\n");
    } else {
        printf("Verification FAILED! Total errors: %u\n", errors);
    }

    release_bus();
    printf("Programming complete! SBUS2 bus released.\n");

    while (1) {
        printf("Waiting.../\r");
        sleep_ms(2000);
        printf("Waiting...\\\r");
        sleep_ms(2000);        
    }
}
