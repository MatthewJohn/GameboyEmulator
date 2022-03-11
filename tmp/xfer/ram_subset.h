#pragma once

#include <memory>

class RamSubset {

public:
    // Memory getter and setters
    uint8_t get_val(uint16_t a);
    uint8_t* get_ref(uint16_t a);
    bool set(uint16_t a, uint8_t value);

    // Constructor
    RamSubset(uint8_t *memory, uint16_t size);

private:
    // Store pointer to memory
    uint8_t *memory;
    // Store size of memory
    uint16_t size;
    // Validate if address is within ram subset
    bool validate_address(uint16_t address);
};