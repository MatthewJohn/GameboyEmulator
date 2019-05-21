#pragma once

#include <memory>
#include "ram_subset.h"

// 0x10000
#define MAX_MEM_SIZE 65535

class RAM {
public:
    RAM();
    uint8_t get_val(uint16_t address);
    uint8_t get_val(int address);
    uint8_t* get_ref(uint16_t a);
    void set(uint16_t address, uint8_t val);
    void set(int address, uint8_t val);
    void v_set(uint16_t address, uint8_t val);
    void v_set(int address, uint8_t val);
    uint8_t dec(uint16_t address);
    uint8_t dec(int address);
    uint8_t inc(uint16_t address);
    uint8_t inc(int address);
    uint8_t v_inc(uint16_t address);
    uint8_t v_inc(int address);
    void stack_push(uint16_t &sp_val, uint8_t pc_val);
    void stack_push(uint16_t &sp_val, uint16_t pc_val);
    uint8_t stack_pop8(uint16_t &sp_val);
    uint16_t stack_pop(uint16_t &sp_val);

    RamSubset get_character_ram();
    RamSubset get_bg_map_1();
    RamSubset get_bg_map_2();
    RamSubset get_internal_work_ram();
    RamSubset get_echo_ram();
    RamSubset get_oam();
    RamSubset get_io_registers();
    RamSubset get_high_ram();
    void load_bios(char *bios_path);
    void load_rom(char *rom_path);
    
    unsigned int get_ram_bit(uint16_t address, unsigned int bit_shift);
    uint8_t set_ram_bit(uint16_t address, uint8_t bit_shift, unsigned int val);
    
    // Video addresses
    const uint16_t LCDC_CONTROL_ADDR = (uint16_t)0xff40; // LCD Control
    const uint16_t LCDC_STATUS_ADDR = (uint16_t)0xff41; // LCD status
    const uint16_t LCDC_SCY = (uint16_t)0xff42; // Background Scroll Y
    const uint16_t LCDC_SCX = (uint16_t)0xff43; // Background Scroll X
    const uint16_t LCDC_LY_ADDR = (uint16_t)0xff44; // Current line draw iterator
    const uint16_t LCDC_LYC_ADDR = (uint16_t)0xff45; // Custom location for interupt if LY equals this
    const uint16_t LCDC_WY_ADDR = (uint16_t)0xff4a; // Window y position
    const uint16_t LCDC_WX_ADDR = (uint16_t)0xff4b; // Window x position
    
private:
    // @TODO Reduce this to exclude mappings to other
    // periferals.
    uint8_t memory[MAX_MEM_SIZE] = {};
    uint16_t memory_max = MAX_MEM_SIZE;
};
