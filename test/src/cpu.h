#pragma once

#include <memory>
#include "ram.h"
#include "./vpu.h"

#ifdef GB_UNITTEST
#include "./unittest.h"
#endif

// Any register
struct reg { };

// 8-bit register
struct reg8 : reg {
    uint8_t value;
};
// 16-bit register
struct reg16 : reg {
    uint16_t value;
};

// Accumulator
struct accumulator : reg8 { };
// General Register
struct gen_reg : reg8 { };
// Stack pointer
struct stack_pointer : reg16 { };
// Program counter
struct program_counter : reg16 { };

class combined_reg {
    // The z80 is little endian because if you were to store HL to memory,
    // L would be written a byte before H.
    // If you were to read, L would be read from the address before H.
public:
    reg8 *lower;
    reg8 *upper;
    uint16_t value() { return ((uint16_t)upper->value << 8) | ((uint16_t)lower->value);};
    void set_value(uint16_t data) {
        union {
            uint8_t bit8[2];
            uint16_t bit16[1];
        } data_conv;
        data_conv.bit16[0] = data;
        this->upper->value = data_conv.bit8[1];
        this->lower->value = data_conv.bit8[0];
    };
};

class CPU {

    #ifdef GB_UNITTEST
    friend TestCPU;
    #endif
    
public:
    CPU(RAM *ram, VPU *vpu_inst);
    virtual ~CPU() {};

    void tick();
    bool is_running();

protected:
    enum INTERUPT_STATE {
        DISABLED,
        PENDING_DISABLE,
        PENDING_ENABLE,
        ENABLED
    };
    INTERUPT_STATE interupt_state;
    bool halt_state;
    bool cb_state;
    bool stepped_in;
    

    union {
        uint8_t bit8[2];
        uint16_t bit16[1];
    } data_conv;
    
    union {
        uint8_t bit8[4];
        uint16_t bit16[2];
        uint32_t bit32[1];
    } data_conv32;
    
    int temp_counter = 0;
    
    // Bits for flag register
    // C flag
    const int CARRY_FLAG_BIT = 4;
    // H flag
    const int HALF_CARRY_FLAG_BIT = 5;
    // N Flag
    const int SUBTRACT_FLAG_BIT = 6;
    // Z flag
    const int ZERO_FLAG_BIT = 7;

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
    signed int get_inc_pc_val8s();
    uint16_t get_inc_pc_val16();
    uint16_t get_register_value16(combined_reg *dest);
    void set_register_bit(reg8 *source, uint8_t bit_shift, unsigned int val);
    unsigned int get_register_bit(reg8 *source, unsigned int bit_shift);
    
    void print_state_m();
    
    void execute_op_code(int op_val);
    void execute_cb_code(int op_val);
    
    void check_interupts();
    
    void set_zero_flag(const uint8_t val);
    uint8_t get_zero_flag();
    void set_half_carry(uint8_t original_val, uint8_t input);
    void set_half_carry(uint16_t original_val, uint16_t input);
    void set_half_carry_sub(uint8_t original_val, uint8_t input);
    void set_half_carry_sub(uint16_t original_val, uint16_t input);
    uint8_t get_carry_flag();

    void op_Load(reg8 *dest);
    void op_Load(reg16 *dest);
    void op_Load(uint16_t dest);
    void op_Load(uint16_t dest_addr, reg16 *source);
    void op_Load(reg8 *dest, reg8 *source);
    void op_Load(combined_reg *dest);
    void op_Load(combined_reg *dest, uint16_t val);
    void op_Load(int dest_addr, reg8 *source);
    void op_Load(int dest_addr, uint8_t val);
    void op_Load(reg8 *dest, int source_addr);
    void op_Load(reg8 *dest, uint16_t source_addr);

    void op_Add(reg8 *dest);
    void op_Add(reg16 *dest);
    void op_Add(reg8 *dest, reg8 *src);
    void op_Add(reg8 *dest, uint16_t src);
    void op_Add(combined_reg *dest, signed int src);
    void op_Add(combined_reg *dest, combined_reg *src);
    void op_Add(combined_reg *dest, reg16 *src);
    void op_Add(combined_reg *dest, uint32_t src);
    void op_Add(reg16 *dest, signed int val);
    void op_Sub();
    void op_Sub(reg8 *src);
    void op_Sub(uint16_t src);
    void op_SBC(reg8 *src);
    void op_SBC(uint8_t src);
    
    void op_Inc(reg8 *dest);
    void op_Inc(uint16_t mem_addr);
    uint8_t op_Inc(uint8_t val);
    void op_Inc(combined_reg *dest);
    void op_Dec(reg8 *dest);
    void op_Dec(uint16_t mem_addr);
    uint8_t op_Dec(uint8_t val);
    void op_Dec(reg16 *dest);
    void op_Dec(combined_reg *dest);

    void op_CP();
    void op_CP(reg8 *in);
    void op_CP(uint8_t in);

    void op_SCF();
    void op_Swap(reg8 *dest);
    void op_Swap(uint16_t mem_addr);
    void op_SRL(reg8 *src);
    void op_SRL(uint16_t mem_addr);
    void op_RL(reg8 *src);
    void op_RL(uint16_t mem_addr);
    void op_RR(reg8 *src);
    void op_RR(uint16_t mem_addr);
    void op_RLC(reg8* src);
    void op_RLC(uint16_t mem_addr);
    void op_RRC(reg8* src);
    void op_RRC(uint16_t mem_addr);

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
    void op_XOR(uint16_t mem_addr);
    void op_XOR(uint8_t val);

    void op_AND();    
    void op_AND(reg8 *comp);
    void op_AND(uint8_t comp);

    void op_OR();
    void op_OR(reg8 *comp);
    void op_OR(uint8_t comp);

    void op_Load_Dec(combined_reg *dest, reg8 *source);
    void op_Load_Dec(reg8 *dest, combined_reg *source);
    void op_Load_Inc(combined_reg *dest, reg8 *source);
    void op_Load_Inc(reg8 *dest, combined_reg *source);

    void op_Bit(reg8 *comp, unsigned int bit);
    void op_Set(uint8_t bit, reg8 *dest);

    void op_Adc(reg8 *dest);
    void op_Adc(reg8 *dest, reg8 *source);
    void op_Adc(reg8 *dest, uint8_t source);

    void op_EI();
    void op_DI();
    void op_Halt();
    
    RAM *ram;
    VPU *vpu_inst;
    bool running;
};