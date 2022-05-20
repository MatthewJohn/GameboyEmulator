// Copyright (C) Dock Studios Ltd, Inc - All Rights Reserved
// Unauthorized copying of this file, via any medium is strictly prohibited
// Proprietary and confidential
// Written by Matt Comben <matthew@dockstudios.co.uk>, May 2019

#pragma once

#include <memory>
#include "./ram.h"
#include "./vpu.h"

// Stub-class for friend
class TestRunner;

// 8-bit register
class reg8 {
private:
   uint8_t value;
public:
    uint8_t get_value();
    uint8_t& get_pointer();
    void set_value(uint8_t new_value);
};

// 16-bit register
class reg16 {
private:
   uint16_t value;
public:
    uint16_t get_value();
    uint16_t& get_pointer();
    void set_value(uint16_t new_value);
};

// Accumulator
class accumulator : public reg8 { };
// General Register
class gen_reg : public reg8 { };
// Stack pointer
class stack_pointer : public reg16 { };
// Program counter
class program_counter : public reg16 { };

class combined_reg {
    // The z80 is little endian because if you were to store HL to memory,
    // L would be written a byte before H.
    // If you were to read, L would be read from the address before H.
public:
    reg8 *lower;
    reg8 *upper;
    uint16_t value() { return ((uint16_t)upper->get_value() << 8) | ((uint16_t)lower->get_value());};
    void set_value(uint16_t data) {
        this->upper->set_value((data & 0xff00) >> 8);
        this->lower->set_value(data & 0x00ff);
    };
};

class CPU {
    friend TestRunner;

public:
    explicit CPU(RAM *ram, VPU *vpu_inst);
    virtual ~CPU() {};

    void tick();
    bool is_running();
    void stop();
    void reset_state();
    int get_tick_counter();
    
    void execute_terminal_command(char* cmd);
    //void print_state();
protected:
    enum INTERRUPT_STATE {
        DISABLED,
        PENDING_DISABLE,
        PENDING_ENABLE,
        ENABLED
    };
    INTERRUPT_STATE interrupt_state;
    bool halt_state;
    bool cb_state;
    bool stepped_in;
    unsigned int op_val;

    union {
        uint8_t bit8[2];
        uint16_t bit16[1];
    } data_conv;

    union {
        uint8_t bit8[4];
        uint16_t bit16[2];
        uint32_t bit32[1];
    } data_conv32;

    int tick_counter = 0;

    // Bits for flag register
    // C flag
    const int CARRY_FLAG_BIT = 4;
    // H flag
    const int HALF_CARRY_FLAG_BIT = 5;
    // N Flag
    const int SUBTRACT_FLAG_BIT = 6;
    // Z flag
    const int ZERO_FLAG_BIT = 7;

    const int CPU_FREQ = 4194304;

    // Timer config
    const unsigned int TIMER_FREQ[4] = {4096, 262144, 65536, 16384};


    // Registers
    //  Accumulator
    accumulator r_a;
    //
    gen_reg r_f;
    // General purpose
    gen_reg r_b;
    gen_reg r_c;
    gen_reg r_d;
    gen_reg r_e;
    gen_reg r_h;
    gen_reg r_l;

    combined_reg r_bc;
    combined_reg r_de;
    combined_reg r_hl;
    combined_reg r_af;

    // Stack pointer
    stack_pointer r_sp;
    // Program counter
    program_counter r_pc;

    uint8_t get_inc_pc_val8();
    int8_t get_inc_pc_val8s();
    uint16_t get_inc_pc_val16();
    void set_register_bit(reg8 *source, uint8_t bit_shift, unsigned int val);
    uint8_t get_register_bit(reg8 *source, unsigned int bit_shift);

    void print_state_m();

    uint8_t execute_op_code(unsigned int op_val);
    uint8_t execute_cb_code(unsigned int op_val);

    // Current ticks to execute operation
    uint8_t current_op_ticks;

    void check_interrupts();
    int checked_op_codes[1000];
    int checked_op_codes_itx;
    int checked_cb_codes[1000];
    int checked_cb_codes_itx;
    
    // Timer
    bool get_timer_state();
    void increment_timer();
    bool timer_overflow;
    
    bool h_blank_executed;
    bool v_blank_executed;

    void set_zero_flag(const uint8_t val);
    uint8_t get_zero_flag();
    void set_half_carry(uint16_t original_val, uint16_t input);
    void set_half_carry16(uint16_t original_val, uint16_t input);
    void set_half_carry_sub(uint8_t original_val, uint8_t input);
    void set_half_carry_sub16(uint16_t original_val, uint16_t input);
    void set_half_carry_sub2(uint8_t original_val, uint8_t input);
    uint8_t get_carry_flag();
    uint8_t get_half_carry_flag();
    uint8_t get_subtract_flag();
    void flip_half_carry_flag();
    void flip_carry_flag();

    void op_Load(reg8 *dest);
    void op_Load(reg16 *dest);
    void opm_Load(uint16_t dest);
    void opm_Load(uint16_t dest_addr, reg16 *source);
    void op_Load(reg8 *dest, reg8 *source);
    void op_Load(combined_reg *dest);
    void op_Load(combined_reg *dest, uint16_t val);
    void opm_Load(uint16_t dest_addr, reg8 *source);
    void opm_Load(uint16_t dest_addr, uint8_t val);
    void op_Load(reg16 *dest, combined_reg *src);
    void opm_Load(reg8 *dest, uint16_t source_addr);
    void op_LoadSpN(combined_reg *dest);

    void op_Add(reg8 *dest);
    void op_Add(reg16 *dest);
    void op_Add(reg8 *dest, reg8 *src);
    void opm_Add(reg8 *dest, uint16_t mem_addr);
    void op_Add(combined_reg *dest, combined_reg *src);
    void op_Add(combined_reg *dest, reg16 *src);
    void op_Add(reg8 *dest, uint16_t src);
    void op_Add(combined_reg *dest, uint32_t src);
    void op_Add(reg16 *dest, int8_t val);
    void op_Sub();
    void op_Sub(reg8 *src);
    void opm_Sub(uint16_t mem_addr);
    void op_Sub(uint8_t src);
    void op_SBC(reg8 *src);
    void op_SBC();
    void op_SBC_common(uint8_t value);
    void opm_SBC(uint16_t mem_addr);
    void op_DAA();
    void op_CPL();
    void op_CCF();

    void op_Inc(reg8 *dest);
    void opm_Inc(uint16_t mem_addr);
    uint8_t op_Inc(uint8_t val);
    void op_Inc(combined_reg *dest);
    void op_Inc(reg16 *dest);
    void op_Dec(reg8 *dest);
    void opm_Dec(uint16_t mem_addr);
    uint8_t op_Dec(uint8_t val);
    void op_Dec(reg16 *dest);
    void op_Dec(combined_reg *dest);

    void op_CP();
    void op_CP(reg8 *in);
    void opm_CP(uint16_t mem_addr);
    void op_CP(uint8_t in);

    void op_SCF();
    void op_Swap(reg8 *dest);
    void opm_Swap(uint16_t mem_addr);
    void op_SRL(reg8 *src);
    void opm_SRL(uint16_t mem_addr);
    void op_RL(reg8 *src);
    void opm_RL(uint16_t mem_addr);
    void op_RR(reg8 *src);
    void opm_RR(uint16_t mem_addr);
    void op_RLC(reg8* src);
    void opm_RLC(uint16_t mem_addr);
    void op_RRC(reg8* src);
    void opm_RRC(uint16_t mem_addr);
    void op_SLA(reg8 *src);
    void opm_SLA(uint16_t mem_addr);
    uint8_t op_SLA(uint8_t val);
    void op_SRA(reg8 *src);
    void opm_SRA(uint16_t mem_addr);
    uint8_t op_SRA(uint8_t val);

    void op_Call();
    void op_Return();
    void op_JR();
    void op_JP();
    void op_JP(combined_reg *jmp_reg);
    void op_JP(uint16_t jump_to);
    void op_RST(uint16_t memory_addr);
    void op_Push(reg16 *src);
    void op_Push(combined_reg *src);
    void op_Push(uint16_t src);
    void op_Pop(reg16 *dest);
    void op_Pop(combined_reg *dest);
    uint16_t op_Pop();

    void op_XOR(reg8 *comp);
    void opm_XOR(uint16_t mem_addr);
    void op_XOR(uint8_t val);

    void op_AND();
    void opm_AND(uint16_t mem_addr);
    void op_AND(reg8 *comp);
    void op_AND(uint8_t comp);

    void op_OR();
    void op_OR(reg8 *comp);
    void opm_OR(uint16_t mem_addr);
    void op_OR(uint8_t comp);

    void op_Load_Dec(combined_reg *dest, reg8 *source);
    void op_Load_Dec(reg8 *dest, combined_reg *source);
    void op_Load_Inc(combined_reg *dest, reg8 *source);
    void op_Load_Inc(reg8 *dest, combined_reg *source);

    void op_Bit(unsigned int bit, reg8 *comp);
    void opm_Bit(unsigned int bit, uint16_t mem_addr);
    void op_Res(uint8_t bit, reg8 *dest);
    void opm_Res(uint8_t bit, uint16_t mem_addr);
    void op_Set(uint8_t bit, reg8 *dest);
    void opm_Set(uint8_t bit, uint16_t mem_addr);

    void op_Adc(reg8 *dest);
    void op_Adc(reg8 *dest, reg8 *source);
    void opm_Adc(reg8 *dest, uint16_t mem_addr);
    void op_Adc(reg8 *dest, uint8_t source);

    void op_EI();
    void op_DI();
    void op_Halt();

    RAM *ram;
    VPU *vpu_inst;
    bool running;
    
    void debug_op_codes(unsigned int op_val);
    void debug_post_tick();
    bool debug_opcode;
};
