
#include "ram_subset.h"
#include <memory>

RamSubset::RamSubset(uint8_t *memory, uint16_t size) {
    this->memory = memory;
    this->size = size;
}

bool RamSubset::validate_address(uint16_t address) {
    return (address <= this->size);
}

uint8_t RamSubset::get_val(uint16_t a) {
    if (! this->validate_address(a)) {
        return 0;
    }
    return this->memory[a];
}

bool RamSubset::set(uint16_t a, uint8_t value) {
    if (! this->validate_address(a)) {
        return false;
    }
    this->memory[a] = value;
    return true;
}
