// Copyright (C) Dock Studios Ltd, Inc - All Rights Reserved
// Unauthorized copying of this file, via any medium is strictly prohibited
// Proprietary and confidential
// Written by Matt Comben <matthew@dockstudios.co.uk>, May 2019

#include "cpu.h"

// Used for printing hex
#include "helper.h"
#include <iomanip>

#include <iostream>
#include <string.h>



// TEMP
#include <algorithm> // for std::find
#include <iterator> // for std::begin, std::end
//DONE

// TODO DEBUG -> CPU count: 2ccdee a == 0080 (should be 00c0) before add hl hl ?

#define DEBUG 0
#define INTERRUPT_DEBUG 1
//#define STEPIN 0x0101
//#define STEPIN 0x07f2
//#define STEPIN 0x00//x0217//x075b
// Step-in when SP points to this adress
#define STEPIN 0//0xc66f//0xc781//0x00//0x100

//#define STEPIN 0 //0x06ef //0x0271 //0x029d
// Debug all calls when PC is over this value
#define DEBUG_POINT 0//xc4a0//x02b7//x086f//x086f//x0870
//0x086e //0x086f//0x02bd//0x0291 //0x26c
// 0x9c9d19
//#define STEPIN_AFTER 0x9c9bca

// AF should be 10a0 PC: 086e
// Stepin after X CPU ticks
#define STEPIN_AFTER 0//0x1c06a7d//x2f1000//x308ac0//x2e8a00(write to 2000)//x2C3D70//2c2c85//0x2cf51d//0x39a378//0x9c9d68//0x2ca378
// Print debug every X cpu ticks
//#define STEPIN_AFTER 0x2ca380
//#define STEPIN_AFTER 0x2ca370
#define DEBUG_EVERY 1//1//x200

// Debug all new op codes
#define DEBUG_OP_CODES false
// Debug single op code
#define DEBUG_SINGLE_OP_CODE 0x0

// Look for FFC3 set to 7f
//x98
#define STOP_ON_BAD_OPCODE 1
#define STOP_BEFORE_ROM 0

signed int convert_signed_uint8_to_signed_int(uint8_t orig)
{
    if (orig & 0x80)
        return (int)(0 - ((orig - 1) ^ 0xff));
    else
        return (int)orig;
}

int8_t convert_signed_uint8_to_int8(uint8_t orig)
{
    if (orig & 0x80)
        return (int8_t)(0 - ((orig - 1) ^ 0xff));
    else
        return (int8_t)orig;
}

uint8_t reg8::get_value() {
    return this->value;
}
uint8_t& reg8::get_pointer() {
    return this->value;
}
void reg8::set_value(uint8_t new_value) {
    this->value = new_value;
}

uint16_t reg16::get_value() {
    return this->value;
}
uint16_t& reg16::get_pointer() {
    return this->value;
}
void reg16::set_value(uint16_t new_value) {
    this->value = new_value;
}


CPU::CPU(RAM *ram, VPU *vpu_inst)
{
    this->r_a = accumulator();
    this->r_f = gen_reg();
    this->r_b = gen_reg();
    this->r_c = gen_reg();
    this->r_d = gen_reg();
    this->r_e = gen_reg();
    this->r_h = gen_reg();
    this->r_l = gen_reg();

    this->r_bc.upper = &this->r_b;
    this->r_bc.lower = &this->r_c;
    this->r_de.upper = &this->r_d;
    this->r_de.lower = &this->r_e;
    this->r_hl.upper = &this->r_h;
    this->r_hl.lower = &this->r_l;
    this->r_af.upper = &this->r_a;
    this->r_af.lower = &this->r_f;

    // Stack pointer
    this->r_sp = stack_pointer();
    // Program counter
    this->r_pc = program_counter();

    this->ram = ram;
    this->vpu_inst = vpu_inst;
    this->reset_state();
}

void CPU::reset_state()
{
    this->r_a.set_value(0);
    this->r_f.set_value(0);
    this->r_b.set_value(0);
    this->r_c.set_value(0);
    this->r_d.set_value(0);
    this->r_e.set_value(0);
    this->r_h.set_value(0);
    this->r_l.set_value(0);

    this->h_blank_executed = false;

    this->cb_state = false;
    this->timer_itx = 0;

    this->interrupt_state = this->INTERRUPT_STATE::DISABLED;
    this->halt_state = false;

    this->r_sp.set_value(0xfffe);
    this->r_pc.set_value(0x00);

    this->checked_op_codes_itx = 0;
    this->checked_cb_codes_itx = 0;

    this->running = true;
    this->stepped_in = false;
    this->timer_overflow = false;
    
    const uint8_t initial_ff00_memory_values[48] = {
        0xcf, 0x00, 0x7e, 0xff, 0x00, 0x00, 0x00, 0xf8,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe1,
        0x80, 0x3f, 0x00, 0xff, 0xbf, 0xff, 0x3f, 0x00
    };
    for (uint16_t mem_itx = 0; mem_itx < 16; mem_itx ++)
        this->ram->set(0xff00 + mem_itx, initial_ff00_memory_values[mem_itx]);
}

void CPU::execute_terminal_command(char* cmd) {
    // Handle step in
    if (strcmp(cmd, "c si") == 0)
    {
        this->stepped_in = true;
    }
    else if (strcmp(cmd, "c so") == 0)
    {
        this->stepped_in = false;
    }
}

void CPU::stop() {
    this->running = false;
}

bool CPU::is_running() {
    return this->running;
}

int CPU::get_tick_counter()
{
    return this->tick_counter;
}

void CPU::tick() {
    this->tick_counter ++;

    if (this->current_op_ticks == 0) {
        if (DEBUG_EVERY != 1 && this->tick_counter % DEBUG_EVERY == 0)
            std::cout << this->tick_counter << " Tick: " << std::hex << this->r_pc.get_value() << ", SP: " << this->r_sp.get_value() << std::endl;
            //this->running = false;
        if (STEPIN_AFTER && (! this->stepped_in) && this->tick_counter >= STEPIN_AFTER)
            this->stepped_in = true;

        if (DEBUG || this->stepped_in) {
            if (! this->stepped_in) {
                this->print_state_m();
            }
            std::cout << std::endl << std::endl << "New Tick: " << std::hex << this->r_pc.get_value() << ", SP: " << this->r_sp.get_value() << std::endl;
        } else if (DEBUG_POINT && this->r_pc.get_value() == DEBUG_POINT)
        {
            this->print_state_m();
            std::cin.get();
        }
    }

    if (this->get_timer_state())
        this->increment_timer();
    
    // Check for interrupts if internal state is true
    if (this->interrupt_state == this->INTERRUPT_STATE::ENABLED ||
            this->interrupt_state == this->INTERRUPT_STATE::PENDING_DISABLE)
        this->check_interrupts();

    // Check if interrupt state is in a pending state
    // and move to actual state, since a clock cycle has been waited
    if (this->interrupt_state == this->INTERRUPT_STATE::PENDING_DISABLE)
        this->interrupt_state = this->INTERRUPT_STATE::DISABLED;
    if (this->interrupt_state == this->INTERRUPT_STATE::PENDING_ENABLE)
        this->interrupt_state = this->INTERRUPT_STATE::ENABLED;

    if (this->halt_state)
        return;

    // Determine stepped-in before PC is incremented
    if (this->current_op_ticks == 0 && (STEPIN == 1 || (STEPIN + 1) == this->r_pc.get_value() ||
        STEPIN == this->r_pc.get_value()) && STEPIN != 0)
    {
        std::cout << std::hex << (unsigned int)this->ram->get_val((uint16_t)0x8010) <<
                                 (unsigned int)this->ram->get_val(0x8011) <<
                                 (unsigned int)this->ram->get_val(0x8012) <<
                                 (unsigned int)this->ram->get_val(0x8013) << std::endl;
        this->stepped_in = true;
    }

    // If currently in an operation,
    // decrement the ticks and move on
    if (this->current_op_ticks > 0x00) {
        this->current_op_ticks --;
        return;
    }

    // Read value from memory, incrementing PC
    this->op_val = (unsigned int)this->get_inc_pc_val8();

    if (DEBUG_OP_CODES || (DEBUG_SINGLE_OP_CODE != 0x00 && this->op_val == DEBUG_SINGLE_OP_CODE) || this->stepped_in || DEBUG)
        this->debug_op_codes(this->op_val);

    if (this->cb_state) {
        this->current_op_ticks = this->execute_cb_code(this->op_val);
        this->cb_state = false;
    } else {
        this->current_op_ticks = this->execute_op_code(this->op_val);
    }

    // Stop runnign when we hit the start of the ROM
    unsigned int current_pc = (unsigned int)this->r_pc.get_value();
    if ((current_pc == 0x0100) && STOP_BEFORE_ROM) {
        this->running = false;
        std::cout << "HIT the start of the ROM!" << std::endl;
    }

    this->debug_post_tick();
}

void CPU::debug_op_codes(unsigned int op_val)
{
    // TEMP CHECK ALL OPCODES
    //this->debug_opcode = false;
    if ((!this->cb_state) && (! (std::find(std::begin(this->checked_op_codes), std::end(this->checked_op_codes), op_val) != std::end(this->checked_op_codes))))
    {
        this->checked_op_codes[this->checked_op_codes_itx] = op_val;
        this->checked_op_codes_itx ++;
        if (this->ram->boot_rom_swapped || this->r_pc.get_value() > 0x100)
            this->debug_opcode = true;
    } else if (this->cb_state && (! (std::find(std::begin(this->checked_cb_codes), std::end(this->checked_cb_codes), op_val) != std::end(this->checked_cb_codes))))
    {
        this->checked_cb_codes[this->checked_cb_codes_itx] = op_val;
        this->checked_cb_codes_itx ++;
        if (this->ram->boot_rom_swapped || this->r_pc.get_value() > 0x100)
            this->debug_opcode = true;
    }
    //this->debug_opcode = false;
    if (this->debug_opcode)
    {
        std::cout << "CB: " << (int)this->cb_state << " Op Code: " << std::hex << op_val << std::endl;
        this->print_state_m();

    }
    if (DEBUG || this->stepped_in)
        std::cout << "CB: " << (int)this->cb_state << " Op Code: " << std::hex << op_val << std::endl;
    //DONE
}

void CPU::debug_post_tick()
{
    if (this->debug_opcode) {
        this->print_state_m();
        this->debug_opcode = false;
        std::cin.get();
    }

    if (this->stepped_in) {
        this->print_state_m();
        std::cin.get();
    }
}

bool CPU::get_timer_state()
{
    return (this->ram->get_ram_bit(this->TAC_TIMER_CONTROL_MEM_ADDRESS, 0x02) == 0x1);
}

void CPU::increment_timer()
{
    unsigned int freq = this->TIMER_FREQ[this->ram->get_val(this->TAC_TIMER_CONTROL_MEM_ADDRESS) & 0x03];

    this->timer_itx ++;

    // Set internal divider value to top byte of internal timer
    this->ram->set(this->DIV_TIMER_DIVIDER_ADDRESS, (this->timer_itx >> 4) & 0xff);

    // If CPU count since last tick is greater/equal to CPU frequency/timer frequency
    // increment timer in mem
    if (this->timer_itx >= (this->CPU_FREQ / freq))
    {
        this->timer_itx = 0;
        this->ram->inc(this->TIMA_TIMER_COUNTER_ADDRESS);
        
        // Check if TIMA overflowed
        if (this->ram->get_val(this->TIMA_TIMER_COUNTER_ADDRESS) == 0)
        {
            // Set timer interrupt
            this->ram->set_ram_bit(this->ram->INTERRUPT_IF_REGISTER_ADDRESS, 3, 1);
            // Reset counter value back to moduli
            this->ram->set(
                this->TIMA_TIMER_COUNTER_ADDRESS,
                // Set current timer value to modulo
                this->ram->get_val(this->TMA_TIMER_INTERRUPT_MODULO_ADDRESS)
            );
        }
    }
}

void CPU::print_state_m() {
    std::cout << std::hex <<
        "CPU Count: " << this->tick_counter << std::endl <<
        //"a : " << std::setfill('0') << std::setw(2) << (unsigned int)this->r_a.value << std::endl <<
        //" f: " << std::setfill('0') << std::setw(2) << (unsigned int)this->r_f.value << std::endl <<
        "af: " << std::setfill('0') << std::setw(4) << this->r_af.value() << std::endl <<
        //"b : " << std::setfill('0') << std::setw(2) << (unsigned int)this->r_b.value << std::endl <<
        //" c: " << std::setfill('0') << std::setw(2) << (unsigned int)this->r_c.value << std::endl <<
        "bc: " << std::setfill('0') << std::setw(4) << this->r_bc.value() << std::endl <<
        //"d : " << std::setfill('0') << std::setw(2) << (unsigned int)this->r_d.value << std::endl <<
        //" e: " << std::setfill('0') << std::setw(2) << (unsigned int)this->r_e.value << std::endl <<
        "de: " << std::setfill('0') << std::setw(4) << this->r_de.value() << std::endl <<
        //"h : " << std::setfill('0') << std::setw(2) << (unsigned int)this->r_h.value << std::endl <<
        //" l: " << std::setfill('0') << std::setw(2) << (unsigned int)this->r_l.value << std::endl <<
        "hl: " << std::setfill('0') << std::setw(4) << this->r_hl.value() << std::endl <<
        "sp: " << std::setfill('0') << std::setw(4) << this->r_sp.get_value() << std::endl <<
        "pc: " << std::setfill('0') << std::setw(4) << this->r_pc.get_value() << std::endl;
}

void CPU::check_interrupts() {

    // Check if VLBANK has been triggered and interrupt is enabled
    if (this->ram->get_ram_bit(this->ram->INTERRUPT_IF_REGISTER_ADDRESS, 0) &&
        this->ram->get_ram_bit(this->ram->INTERRUPT_IE_REGISTER_ADDRESS, 0))
    {

        // Reset interrupt user interrupt bit
        this->ram->set_ram_bit(this->ram->INTERRUPT_IF_REGISTER_ADDRESS, 0, 0);

        if (INTERRUPT_DEBUG || DEBUG || this->stepped_in)
            std::cout << "Got VLBANK INTERRUPT!" << std::endl;

        // Push current pointer to stack and update PC to
        // interrupt address
        this->ram->stack_push(this->r_sp.get_pointer(), this->r_pc.get_value());
        this->r_pc.set_value(this->VBLANK_INTERRUPT_PTR_ADDR);

        // Do not process any more interrupts
        return;
    }


    // Check if LCD Status has been triggered and interrupt is enabled
    if (this->ram->get_ram_bit(this->ram->INTERRUPT_IF_REGISTER_ADDRESS, 1) &&
        this->ram->get_ram_bit(this->ram->INTERRUPT_IE_REGISTER_ADDRESS, 1))
    {
        // Reset interrupt user interrupt bit
        this->ram->set_ram_bit(this->ram->INTERRUPT_IF_REGISTER_ADDRESS, 1, 0);

        if (INTERRUPT_DEBUG || DEBUG || this->stepped_in)
            std::cout << "Got STAT INTERRUPT!" << std::endl;

        // Push current pointer to stack and update PC to
        // interrupt address
        this->ram->stack_push(this->r_sp.get_pointer(), this->r_pc.get_value());
        this->r_pc.set_value(this->LCDC_STATUS_INTERRUPT_PTR_ADDR);

        // Do not process any more interrupts
        return;
    }


    // Check if timer has been triggered and interrupt is enabled
    if (this->ram->get_ram_bit(this->ram->INTERRUPT_IF_REGISTER_ADDRESS, 2) &&
        this->ram->get_ram_bit(this->ram->INTERRUPT_IE_REGISTER_ADDRESS, 2))
    {
        // Reset interrupt user interrupt bit
        this->ram->set_ram_bit(this->ram->INTERRUPT_IF_REGISTER_ADDRESS, 2, 0);

        if (INTERRUPT_DEBUG || DEBUG || this->stepped_in)
            std::cout << "Got TIMER INTERRUPT!" << std::endl;

        // Push current pointer to stack and update PC to
        // interrupt address
        this->ram->stack_push(this->r_sp.get_pointer(), this->r_pc.get_value());
        this->r_pc.set_value(this->TIMER_INTERRUPT_PTR_ADDR);

        return;
    }
}

uint8_t CPU::execute_op_code(unsigned int op_val) {
    // number of ticks
    uint8_t t = 0;
    switch(op_val) {
        case 0x0:
            // STOP
            //this->op_Noop();
            if (DEBUG || this->stepped_in)
                std::cout << "Noop: " << std::hex << this->r_pc.get_value() << std::endl;
            t = 4;
            break;
        case 0x01:
            this->op_Load(&this->r_bc);
            t = 12;
            break;
        case 0x02:
            this->opm_Load(this->r_bc.value(), &this->r_a);
            t = 8;
            break;
        case 0x03:
            this->op_Inc(&this->r_bc);
            t = 8;
            break;
        case 0x04:
            this->op_Inc(&this->r_b);
            t = 4;
            break;
        case 0x05:
            this->op_Dec(&this->r_b);
            t = 4;
            break;
        case 0x06:
            // Load byte into C
            this->op_Load(&this->r_b);
            t = 8;
            break;
        case 0x07:
            this->op_RLC(&this->r_a);
            t = 4;
            break;
        case 0x08:
            // Load byte into C
            this->opm_Load(this->get_inc_pc_val16(), &this->r_sp);
            t = 20;
            break;
        case 0x09:
            this->op_Add(&this->r_hl, &this->r_bc);
            t = 8;
            break;
        case 0x0a:
            this->opm_Load(&this->r_a, this->r_bc.value());
            t = 8;
            break;
        case 0x0b:
            this->op_Dec(&this->r_bc);
            t = 8;
            break;
        case 0x0c:
            this->op_Inc(&this->r_c);
            t = 4;
            break;
        case 0x0d:
            this->op_Dec(&this->r_c);
            t = 4;
            break;
        case 0x0e:
            // Load byte into C
            this->op_Load(&this->r_c);
            t = 8;
            break;
        case 0x0f:
            this->op_RRC(&this->r_a);
            t = 4;
            break;
        case 0x10:
            //this->op_Stop();
            // @TODO: This is TEMPORARY
            //this->running = false;
            t = 4;
            break;
        case 0x11:
            this->op_Load(&this->r_de);
            t = 12;
            break;
        case 0x12:
            this->opm_Load(this->r_de.value(), &this->r_a);
            t = 8;
            break;
        case 0x13:
            this->op_Inc(&this->r_de);
            t = 8;
            break;
        case 0x14:
            this->op_Inc(&this->r_d);
            t = 4;
            break;
        case 0x15:
            this->op_Dec(&this->r_d);
            t = 4;
            break;
        case 0x16:
            // Load byte into C
            this->op_Load(&this->r_d);
            t = 8;
            break;
        case 0x17:
            this->op_RL(&this->r_a);
            t = 4;
            break;
        case 0x18:
            this->op_JR();
            t = 12;
            break;
        case 0x19:
            this->op_Add(&this->r_hl, &this->r_de);
            t = 8;
            break;
        case 0x1a:
            this->opm_Load(&this->r_a, this->r_de.value());
            t = 8;
            break;
        case 0x1b:
            this->op_Dec(&this->r_de);
            t = 8;
            break;
        case 0x1c:
            this->op_Inc(&this->r_e);
            t = 4;
            break;
        case 0x1d:
            this->op_Dec(&this->r_e);
            t = 4;
            break;
        case 0x1e:
            this->op_Load(&this->r_e);
            t = 8;
            break;
        case 0x1f:
            this->op_RR(&this->r_a);
            t = 4;
            break;
        case 0x20:
            if (this->get_zero_flag() == (uint8_t)0x00) {
                this->op_JR();
                t = 12;  //?
            }
            else {
                // If we don't perform the OP, pull
                // data from ram to inc PC
                this->get_inc_pc_val8();
                t = 8;  //?
            }
            break;
        case 0x21:
            this->op_Load(&this->r_hl);
            t = 12;
            break;
        case 0x22:
            // Get HL, dec and set
            this->op_Load_Inc(&this->r_hl, &this->r_a);
            t = 8;
            break;
        case 0x23:
            this->op_Inc(&this->r_hl);
            t = 8;
            break;
        case 0x24:
            this->op_Inc(&this->r_h);
            t = 4;
            break;
        case 0x25:
            this->op_Dec(&this->r_h);
            t = 4;
            break;
        case 0x26:
            this->op_Load(&this->r_h);
            t = 8;
            break;
        case 0x27:
            this->op_DAA();
            t = 4;
            break;
        case 0x28:
            if (this->get_zero_flag() == (uint8_t)0x01) {
                this->op_JR();
                t = 12;  //?
            }
            else {
                // If we don't perform the OP, pull
                // data from ram to inc PC
                this->get_inc_pc_val8();
                t = 4;  //?
            }
            break;
        case 0x29:
            this->op_Add(&this->r_hl, &this->r_hl);
            t = 8;
            break;
        case 0x2a:
            this->op_Load_Inc(&this->r_a, &this->r_hl);
            t = 8;
            break;
        case 0x2b:
            this->op_Dec(&this->r_hl);
            t = 8;
            break;
        case 0x2c:
            this->op_Inc(&this->r_l);
            t = 4;
            break;
        case 0x2d:
            this->op_Dec(&this->r_l);
            t = 4;
            break;
        case 0x2e:
            this->op_Load(&this->r_l);
            t = 8;
            break;
        case 0x2f:
            this->op_CPL();
            t = 4;
            break;
        case 0x30:
            if (this->get_carry_flag() == (uint8_t)0x00) {
                this->op_JR();
                t = 12;
            }
            else {
                // If we don't perform the OP, pull
                // data from ram to inc PC
                this->get_inc_pc_val8();
                t = 8;
            }
            break;
        case 0x31:
            // Load 2-bytes into SP
            this->op_Load(&this->r_sp);
            t = 12;
            break;
        case 0x32:
            // Get HL, dec and set
            this->op_Load_Dec(&this->r_hl, &this->r_a);
            t = 8;
            break;
        case 0x33:
            this->op_Inc(&this->r_sp);
            t = 8;
            break;
        case 0x34:
            this->opm_Inc(this->r_hl.value());
            t = 12;
            break;
        case 0x35:
            this->opm_Dec(this->r_hl.value());
            t = 12;
            break;
        case 0x36:
            this->opm_Load(this->r_hl.value());
            t = 12;
            break;
        case 0x37:
            this->op_SCF();
            t = 4;
            break;
        case 0x38:
            if (this->get_carry_flag() == (uint8_t)0x01) {
                this->op_JR();
                t = 12;
            }
            else {
                // If we don't perform the OP, pull
                // data from ram to inc PC
                this->get_inc_pc_val8();
                t = 8;
            }
            break;
        case 0x39:
            this->op_Add(&this->r_hl, &this->r_sp);
            t = 8;
            break;
        case 0x3a:
            // Get HL, dec and set
            this->op_Load_Dec(&this->r_a, &this->r_hl);
            t = 8;
            break;
        case 0x3b:
            this->op_Dec(&this->r_sp);
            t = 8;
            break;
        case 0x3c:
            this->op_Inc(&this->r_a);
            t = 4;
            break;
        case 0x3d:
            this->op_Dec(&this->r_a);
            t = 4;
            break;
        case 0x3e:
            this->op_Load(&this->r_a);
            t = 8;
            break;
        case 0x3f:
            this->op_CCF();
            t = 4;
            break;
        case 0x40:
            this->op_Load(&this->r_b, &this->r_b);
            t = 4;
            break;
        case 0x41:
            this->op_Load(&this->r_b, &this->r_c);
            t = 4;
            break;
        case 0x42:
            this->op_Load(&this->r_b, &this->r_d);
            t = 4;
            break;
        case 0x43:
            this->op_Load(&this->r_b, &this->r_e);
            t = 4;
            break;
        case 0x44:
            this->op_Load(&this->r_b, &this->r_h);
            t = 4;
            break;
        case 0x45:
            this->op_Load(&this->r_b, &this->r_l);
            t = 4;
            break;
        case 0x46:
            this->opm_Load(&this->r_b, this->r_hl.value());
            t = 8;
            break;
        case 0x47:
            this->op_Load(&this->r_b, &this->r_a);
            t = 4;
            break;
        case 0x48:
            this->op_Load(&this->r_c, &this->r_b);
            t = 4;
            break;
        case 0x49:
            this->op_Load(&this->r_c, &this->r_c);
            t = 4;
            break;
        case 0x4a:
            this->op_Load(&this->r_c, &this->r_d);
            t = 4;
            break;
        case 0x4b:
            this->op_Load(&this->r_c, &this->r_e);
            t = 4;
            break;
        case 0x4c:
            this->op_Load(&this->r_c, &this->r_h);
            t = 4;
            break;
        case 0x4d:
            this->op_Load(&this->r_c, &this->r_l);
            t = 4;
            break;
        case 0x4e:
            this->opm_Load(&this->r_c, this->r_hl.value());
            t = 8;
            break;
        case 0x4f:
            this->op_Load(&this->r_c, &this->r_a);
            t = 4;
            break;
        case 0x50:
            this->op_Load(&this->r_d, &this->r_b);
            t = 4;
            break;
        case 0x51:
            this->op_Load(&this->r_d, &this->r_c);
            t = 4;
            break;
        case 0x52:
            this->op_Load(&this->r_d, &this->r_d);
            t = 4;
            break;
        case 0x53:
            this->op_Load(&this->r_d, &this->r_e);
            t = 4;
            break;
        case 0x54:
            this->op_Load(&this->r_d, &this->r_h);
            t = 4;
            break;
        case 0x55:
            this->op_Load(&this->r_d, &this->r_l);
            t = 4;
            break;
        case 0x56:
            this->opm_Load(&this->r_d, this->r_hl.value());
            t = 8;
            break;
        case 0x57:
            this->op_Load(&this->r_d, &this->r_a);
            t = 4;
            break;
        case 0x58:
            this->op_Load(&this->r_e, &this->r_b);
            t = 4;
            break;
        case 0x59:
            this->op_Load(&this->r_e, &this->r_c);
            t = 4;
            break;
        case 0x5a:
            this->op_Load(&this->r_e, &this->r_d);
            break;
        case 0x5b:
            this->op_Load(&this->r_e, &this->r_e);
            t = 4;
            break;
        case 0x5c:
            this->op_Load(&this->r_e, &this->r_h);
            t = 4;
            break;
        case 0x5d:
            this->op_Load(&this->r_e, &this->r_l);
            t = 4;
            break;
        case 0x5e:
            this->opm_Load(&this->r_e, this->r_hl.value());
            t = 8;
            break;
        case 0x5f:
            this->op_Load(&this->r_e, &this->r_a);
            t = 4;
            break;
        case 0x60:
            this->op_Load(&this->r_h, &this->r_b);
            t = 4;
            break;
        case 0x61:
            this->op_Load(&this->r_h, &this->r_c);
            t = 4;
            break;
        case 0x62:
            this->op_Load(&this->r_h, &this->r_d);
            t = 4;
            break;
        case 0x63:
            this->op_Load(&this->r_h, &this->r_e);
            t = 4;
            break;
        case 0x64:
            this->op_Load(&this->r_h, &this->r_h);
            t = 4;
            break;
        case 0x65:
            this->op_Load(&this->r_h, &this->r_l);
            t = 4;
            break;
        case 0x66:
            this->opm_Load(&this->r_h, this->r_hl.value());
            t = 8;
            break;
        case 0x67:
            this->op_Load(&this->r_h, &this->r_a);
            t = 4;
            break;
        case 0x68:
            this->op_Load(&this->r_l, &this->r_b);
            t = 4;
            break;
        case 0x69:
            this->op_Load(&this->r_l, &this->r_c);
            t = 4;
            break;
        case 0x6a:
            this->op_Load(&this->r_l, &this->r_d);
            t = 4;
            break;
        case 0x6b:
            this->op_Load(&this->r_l, &this->r_e);
            t = 4;
            break;
        case 0x6c:
            this->op_Load(&this->r_l, &this->r_h);
            t = 4;
            break;
        case 0x6d:
            this->op_Load(&this->r_l, &this->r_l);
            t = 4;
            break;
        case 0x6e:
            this->opm_Load(&this->r_l, this->r_hl.value());
            t = 8;
            break;
        case 0x6f:
            this->op_Load(&this->r_l, &this->r_a);
            t = 4;
            break;
        case 0x70:
            this->opm_Load(this->r_hl.value(), &this->r_b);
            t = 8;
            break;
        case 0x71:
            this->opm_Load(this->r_hl.value(), &this->r_c);
            t = 8;
            break;
        case 0x72:
            this->opm_Load(this->r_hl.value(), &this->r_d);
            t = 8;
            break;
        case 0x73:
            this->opm_Load(this->r_hl.value(), &this->r_e);
            t = 8;
            break;
        case 0x74:
            this->opm_Load(this->r_hl.value(), &this->r_h);
            t = 8;
            break;
        case 0x75:
            this->opm_Load(this->r_hl.value(), &this->r_l);
            t = 8;
            break;
        case 0x76:
            this->op_Halt();
            t = 4;
            break;
        case 0x77:
            this->opm_Load(this->r_hl.value(), &this->r_a);
            t = 8;
            break;
        case 0x78:
            this->op_Load(&this->r_a, &this->r_b);
            t = 4;
            break;
        case 0x79:
            this->op_Load(&this->r_a, &this->r_c);
            t = 4;
            break;
        case 0x7a:
            this->op_Load(&this->r_a, &this->r_d);
            t = 4;
            break;
        case 0x7b:
            this->op_Load(&this->r_a, &this->r_e);
            t = 4;
            break;
        case 0x7c:
            this->op_Load(&this->r_a, &this->r_h);
            t = 4;
            break;
        case 0x7d:
            this->op_Load(&this->r_a, &this->r_l);
            t = 4;
            break;
        case 0x7e:
            this->opm_Load(&this->r_a, this->r_hl.value());
            t = 8;
            break;
        case 0x7f:
            this->op_Load(&this->r_a, &this->r_a);
            t = 4;
            break;
        case 0x80:
            this->op_Add(&this->r_a, &this->r_b);
            t = 4;
            break;
        case 0x81:
            this->op_Add(&this->r_a, &this->r_c);
            t = 4;
            break;
        case 0x82:
            this->op_Add(&this->r_a, &this->r_d);
            t = 4;
            break;
        case 0x83:
            this->op_Add(&this->r_a, &this->r_e);
            t = 4;
            break;
        case 0x84:
            this->op_Add(&this->r_a, &this->r_h);
            t = 4;
            break;
        case 0x85:
            this->op_Add(&this->r_a, &this->r_l);
            t = 4;
            break;
        case 0x86:
            this->opm_Add(&this->r_a, this->r_hl.value());
            t = 8;
            break;
        case 0x87:
            this->op_Add(&this->r_a, &this->r_a);
            t = 4;
            break;
        case 0x88:
            this->op_Adc(&this->r_a, &this->r_b);
            t = 4;
            break;
        case 0x89:
            this->op_Adc(&this->r_a, &this->r_c);
            t = 4;
            break;
        case 0x8a:
            this->op_Adc(&this->r_a, &this->r_d);
            t = 4;
            break;
        case 0x8b:
            this->op_Adc(&this->r_a, &this->r_e);
            t = 4;
            break;
        case 0x8c:
            this->op_Adc(&this->r_a, &this->r_h);
            t = 4;
            break;
        case 0x8d:
            this->op_Adc(&this->r_a, &this->r_l);
            t = 4;
            break;
        case 0x8e:
            this->opm_Adc(&this->r_a, this->r_hl.value());
            t = 8;
            break;
        case 0x8f:
            this->op_Adc(&this->r_a, &this->r_a);
            t = 4;
            break;
        case 0x90:
            this->op_Sub(&this->r_b);
            t = 4;
            break;
        case 0x91:
            this->op_Sub(&this->r_c);
            t = 4;
            break;
        case 0x92:
            this->op_Sub(&this->r_d);
            t = 4;
            break;
        case 0x93:
            this->op_Sub(&this->r_e);
            t = 4;
            break;
        case 0x94:
            this->op_Sub(&this->r_h);
            t = 4;
            break;
        case 0x95:
            this->op_Sub(&this->r_l);
            t = 4;
            break;
        case 0x96:
            this->opm_Sub(this->r_hl.value());
            t = 8;
            break;
        case 0x97:
            this->op_Sub(&this->r_a);
            t = 4;
            break;
        case 0x98:
            this->op_SBC(&this->r_b);
            t = 4;
            break;
        case 0x99:
            this->op_SBC(&this->r_c);
            t = 4;
            break;
        case 0x9a:
            this->op_SBC(&this->r_d);
            t = 4;
            break;
        case 0x9b:
            this->op_SBC(&this->r_e);
            t = 4;
            break;
        case 0x9c:
            this->op_SBC(&this->r_h);
            t = 4;
            break;
        case 0x9d:
            this->op_SBC(&this->r_l);
            t = 4;
            break;
        case 0x9e:
            this->opm_SBC(this->r_hl.value());
            t = 8;
            break;
        case 0x9f:
            this->op_SBC(&this->r_a);
            t = 4;
            break;
        case 0xa0:
            this->op_AND(&this->r_b);
            t = 4;
            break;
        case 0xa1:
            this->op_AND(&this->r_c);
            t = 4;
            break;
        case 0xa2:
            this->op_AND(&this->r_d);
            t = 4;
            break;
        case 0xa3:
            this->op_AND(&this->r_e);
            t = 4;
            break;
        case 0xa4:
            this->op_AND(&this->r_h);
            t = 4;
            break;
        case 0xa5:
            this->op_AND(&this->r_l);
            t = 4;
            break;
        case 0xa6:
            this->opm_AND(this->r_hl.value());
            t = 8;
            break;
        case 0xa7:
            this->op_AND(&this->r_a);
            t = 4;
            break;
        case 0xa8:
            this->op_XOR(&this->r_b);
            t = 4;
            break;
        case 0xa9:
            this->op_XOR(&this->r_c);
            t = 4;
            break;
        case 0xaa:
            this->op_XOR(&this->r_d);
            t = 4;
            break;
        case 0xab:
            this->op_XOR(&this->r_e);
            t = 4;
            break;
        case 0xac:
            this->op_XOR(&this->r_h);
            t = 4;
            break;
        case 0xad:
            this->op_XOR(&this->r_l);
            t = 4;
            break;
        case 0xae:
            this->opm_XOR(this->r_hl.value());
            t = 8;
            break;
        case 0xaf:
            // X-OR A with A into A
            this->op_XOR(&this->r_a);
            t = 4;
            break;
        case 0xb0:
            this->op_OR(&this->r_b);
            t = 4;
            break;
        case 0xb1:
            this->op_OR(&this->r_c);
            t = 4;
            break;
        case 0xb2:
            this->op_OR(&this->r_d);
            t = 4;
            break;
        case 0xb3:
            this->op_OR(&this->r_e);
            t = 4;
            break;
        case 0xb4:
            this->op_OR(&this->r_h);
            t = 4;
            break;
        case 0xb5:
            this->op_OR(&this->r_l);
            t = 4;
            break;
        case 0xb6:
            this->opm_OR(this->r_hl.value());
            t = 8;
            break;
        case 0xb7:
            this->op_OR(&this->r_a);
            t = 4;
            break;
        case 0xb8:
            this->op_CP(&this->r_b);
            t = 4;
            break;
        case 0xb9:
            this->op_CP(&this->r_c);
            t = 4;
            break;
        case 0xba:
            this->op_CP(&this->r_d);
            t = 4;
            break;
        case 0xbb:
            this->op_CP(&this->r_e);
            t = 4;
            break;
        case 0xbc:
            this->op_CP(&this->r_h);
            t = 4;
            break;
        case 0xbd:
            this->op_CP(&this->r_l);
            t = 4;
            break;
        case 0xbe:
            this->opm_CP(this->r_hl.value());
            t = 8;
            break;
        case 0xbf:
            this->op_CP(&this->r_a);
            t = 4;
            break;
        case 0xc0:
            if (this->get_zero_flag() == 0x00) {
                this->op_Return();
                t = 20;
            } else {
                t = 8;
            }
            break;
        case 0xc1:
            this->op_Pop(&this->r_bc);
            t = 12;
            break;
        case 0xc2:
            if (this->get_zero_flag() == 0x00) {
                this->op_JP();
                t = 16;
            }
            else {
                // If we don't perform the OP, pull
                // data from ram to inc PC
                this->get_inc_pc_val16();
                t = 12;
            }
            break;
        case 0xc3:
            this->op_JP();
            t = 16;
            break;
        case 0xc4:
            if (this->get_zero_flag() == 0x00) {
                this->op_Call();
                t = 16;
            }
            else {
                // If we don't perform the OP, pull
                // data from ram to inc PC
                this->get_inc_pc_val16();
                t = 12;
            }
            break;
        case 0xc5:
            this->op_Push(&this->r_bc);
            t = 16;
            break;
        case 0xc6:
            this->op_Add(&this->r_a);
            t = 8;
            break;
        case 0xc7:
            this->op_RST(0x0000);
            t = 16;
            break;
        case 0xc8:
            if (this->get_zero_flag() == 0x01) {
                this->op_Return();
                t = 20;
            } else {
                t = 8;
            }
            break;
        case 0xc9:
            this->op_Return();
            t = 16;
            break;
        case 0xca:
            if (this->get_zero_flag() == 0x01) {
                this->op_JP();
                t = 16;
            }
            else {
                // If we don't perform the OP, pull
                // data from ram to inc PC
                this->get_inc_pc_val16();
                t = 12;
            }
            break;
        case 0xcb:
            // Set flag for CB
            this->cb_state = true;
            t = 4;
            break;
        case 0xcc:
            if (this->get_zero_flag() == 0x01) {
                this->op_Call();
                t = 24;
            }
            else {
                // If we don't perform the OP, pull
                // data from ram to inc PC
                this->get_inc_pc_val16();
                t = 12;
            }
            break;
        case 0xcd:
            this->op_Call();
            t = 24;
            break;
        case 0xce:
            this->op_Adc(&this->r_a);
            t = 8;
            break;
        case 0xcf:
            this->op_RST(0x0008);
            t = 16;
            break;
        case 0xd0:
            if (this->get_carry_flag() == 0x00) {
                this->op_Return();
                t = 20;
            } else {
                t = 8;
            }
            break;
        case 0xd1:
            this->op_Pop(&this->r_de);
            t = 12;
            break;
        case 0xd2:
            if (this->get_carry_flag() == 0x00) {
                this->op_JP();
                t = 16;
            }
            else {
                // If we don't perform the OP, pull
                // data from ram to inc PC
                this->get_inc_pc_val16();
                t = 12;
            }
            break;
        case 0xd4:
            if (this->get_carry_flag() == 0x00) {
                this->op_Call();
                t = 24;
            }
            else {
                this->get_inc_pc_val16();
                t = 12;
            }
            break;
        case 0xd5:
            this->op_Push(&this->r_de);
            t = 16;
            break;
        case 0xd6:
            this->op_Sub();
            t = 8;
            break;
        case 0xd7:
            this->op_RST(0x0010);
            t = 16;
            break;
        case 0xd8:
            if (this->get_carry_flag() == 0x01) {
                this->op_Return();
                t = 20;
            } else {
                t = 8;
            }
            break;
        case 0xd9:
            this->op_Return();
            // Re-enable interrupts
            this->op_EI();
            t = 16;
            break;
        case 0xda:
            if (this->get_carry_flag() == 0x01) {
                this->op_JP();
                t = 16;
            }
            else {
                // If we don't perform the OP, pull
                // data from ram to inc PC
                this->get_inc_pc_val16();
                t = 12;
            }
            break;
        case 0xdc:
            if (this->get_carry_flag() == 0x01) {
                this->op_Call();
                t = 24;
            }
            else {
                this->get_inc_pc_val16();
                t = 12;
            }
            break;
        case 0xde:
            this->op_SBC();
            t = 8;
            break;
        case 0xdf:
            this->op_RST(0x0018);
            t = 16;
            break;
        case 0xe0:
            this->opm_Load((uint16_t)((uint16_t)0xff00 + this->get_inc_pc_val8()), &this->r_a);
            t = 12;
            break;
        case 0xe1:
            this->op_Pop(&this->r_hl);
            t = 12;
            break;
        case 0xe2:
            this->opm_Load((uint16_t)((uint16_t)0xff00 + this->r_c.get_value()), &this->r_a);
            t = 8;
            break;
        case 0xe5:
            this->op_Push(&this->r_hl);
            t = 16;
            break;
        case 0xe6:
            this->op_AND();
            t = 8;
            break;
        case 0xe7:
            this->op_RST(0x0020);
            t = 16;
            break;
        case 0xe8:
            this->op_Add(&this->r_sp, this->get_inc_pc_val8s());
            t = 16;
            break;
        case 0xe9:
            this->op_JP(this->r_hl.value());
            t = 4;
            break;
        case 0xea:
            this->opm_Load(this->get_inc_pc_val16(), &this->r_a);
            t = 16;
            break;
        // No available OPs here
        case 0xee:
            this->op_XOR(this->get_inc_pc_val8());
            t = 8;
            break;
        case 0xef:
            this->op_RST(0x0028);
            t = 16;
            break;
        case 0xf0:
            this->opm_Load(&this->r_a, (uint16_t)(0xff00 + this->get_inc_pc_val8()));
            t = 12;
            break;
        case 0xf1:
            this->op_Pop(&this->r_af);
            // Purge anything in the LSB nibble of the flag
            this->r_f.set_value(this->r_f.get_value() & 0xf0);
            t = 12;
            break;
        case 0xf2:
            this->opm_Load(&this->r_a, (uint16_t)(0xff00 + this->r_c.get_value()));
            t = 8;
            break;
        case 0xf3:
            // Disable interrupts
            this->op_DI();
            t = 4;
            break;
        case 0xf5:
            this->op_Push(&this->r_af);
            t = 16;
            break;
        case 0xf6:
            this->op_OR();
            t = 8;
            break;
        case 0xf7:
            this->op_RST(0x0030);
            t = 16;
            break;
        case 0xf8:
            this->op_LoadSpN(&this->r_hl);
            t = 12;
            break;
        case 0xf9:
            this->op_Load(&this->r_sp, &this->r_hl);
            t = 8;
            break;
        case 0xfa:
            this->opm_Load(&this->r_a, this->get_inc_pc_val16());
            t = 16;
            break;
        case 0xfb:
            // Enable interrupts
            this->op_EI();
            t = 4;
            break;
        case 0xfe:
            this->op_CP();
            t = 8;
            break;
        case 0xff:
            this->op_RST(0x0038);
            t = 16;
            break;

        default:
            std::cout << std::hex << ((unsigned int)this->r_pc.get_value() - 1) << "Unknown op code: 0x";
            std::cout << std::setfill('0') << std::setw(2) << std::hex << op_val;
            std::cout << std::endl;
            if (STOP_ON_BAD_OPCODE) {
                this->running = false;
                this->stepped_in = true;
            }
            break;
    }
    if (t == 0) {
        std::cout << "WARNING - No ticks defined for Opcode!" << std::endl;
    }
    return t;
}

uint8_t CPU::execute_cb_code(unsigned int op_val) {
    // ticks
    uint8_t t = 8;
    switch(op_val) {
        case 0x00:
            this->op_RLC(&this->r_b);
            break;
        case 0x01:
            this->op_RLC(&this->r_c);
            break;
        case 0x02:
            this->op_RLC(&this->r_d);
            break;
        case 0x03:
            this->op_RLC(&this->r_e);
            break;
        case 0x04:
            this->op_RLC(&this->r_h);
            break;
        case 0x05:
            this->op_RLC(&this->r_l);
            break;
        case 0x06:
            this->opm_RLC(this->r_hl.value());
            t = 16;
            break;
        case 0x07:
            this->op_RLC(&this->r_a);
            t = 4;
            break;
        case 0x08:
            this->op_RRC(&this->r_b);
            break;
        case 0x09:
            this->op_RRC(&this->r_c);
            break;
        case 0x0a:
            this->op_RRC(&this->r_d);
            break;
        case 0x0b:
            this->op_RRC(&this->r_e);
            break;
        case 0x0c:
            this->op_RRC(&this->r_h);
            break;
        case 0x0d:
            this->op_RRC(&this->r_l);
            break;
        case 0x0e:
            this->opm_RRC(this->r_hl.value());
            t = 16;
            break;
        case 0x0f:
            this->op_RRC(&this->r_a);
            break;
        case 0x10:
            this->op_RL(&this->r_b);
            break;
        case 0x11:
            this->op_RL(&this->r_c);
            break;
        case 0x12:
            this->op_RL(&this->r_d);
            break;
        case 0x13:
            this->op_RL(&this->r_e);
            break;
        case 0x14:
            this->op_RL(&this->r_h);
            break;
        case 0x15:
            this->op_RL(&this->r_l);
            break;
        case 0x16:
            this->opm_RL(this->r_hl.value());
            t = 16;
            break;
        case 0x17:
            this->op_RL(&this->r_a);
            break;
        case 0x18:
            this->op_RR(&this->r_b);
            break;
        case 0x19:
            this->op_RR(&this->r_c);
            break;
        case 0x1a:
            this->op_RR(&this->r_d);
            break;
        case 0x1b:
            this->op_RR(&this->r_e);
            break;
        case 0x1c:
            this->op_RR(&this->r_h);
            break;
        case 0x1d:
            this->op_RR(&this->r_l);
            break;
        case 0x1e:
            this->opm_RR(this->r_hl.value());
            t = 16;
            break;
        case 0x1f:
            this->op_RR(&this->r_a);
            break;
        case 0x20:
            this->op_SLA(&this->r_b);
            break;
        case 0x21:
            this->op_SLA(&this->r_c);
            break;
        case 0x22:
            this->op_SLA(&this->r_d);
            break;
        case 0x23:
            this->op_SLA(&this->r_e);
            break;
        case 0x24:
            this->op_SLA(&this->r_h);
            break;
        case 0x25:
            this->op_SLA(&this->r_l);
            break;
        case 0x26:
            this->opm_SLA(this->r_hl.value());
            t = 16;
            break;
        case 0x27:
            this->op_SLA(&this->r_a);
            break;
        case 0x28:
            this->op_SRA(&this->r_b);
            break;
        case 0x29:
            this->op_SRA(&this->r_c);
            break;
        case 0x2a:
            this->op_SRA(&this->r_d);
            break;
        case 0x2b:
            this->op_SRA(&this->r_e);
            break;
        case 0x2c:
            this->op_SRA(&this->r_h);
            break;
        case 0x2d:
            this->op_SRA(&this->r_l);
            break;
        case 0x2e:
            this->opm_SRA(this->r_hl.value());
            t = 16;
            break;
        case 0x2f:
            this->op_SRA(&this->r_a);
            break;

        case 0x30:
            this->op_Swap(&this->r_b);
            break;
        case 0x31:
            this->op_Swap(&this->r_c);
            break;
        case 0x32:
            this->op_Swap(&this->r_d);
            break;
        case 0x33:
            this->op_Swap(&this->r_e);
            break;
        case 0x34:
            this->op_Swap(&this->r_h);
            break;
        case 0x35:
            this->op_Swap(&this->r_l);
            break;
        case 0x36:
            this->opm_Swap(this->r_hl.value());
            t = 16;
            break;
        case 0x37:
            this->op_Swap(&this->r_a);
            break;
        case 0x38:
            this->op_SRL(&this->r_b);
            break;
        case 0x39:
            this->op_SRL(&this->r_c);
            break;
        case 0x3a:
            this->op_SRL(&this->r_d);
            break;
        case 0x3b:
            this->op_SRL(&this->r_e);
            break;
        case 0x3c:
            this->op_SRL(&this->r_h);
            break;
        case 0x3d:
            this->op_SRL(&this->r_l);
            break;
        case 0x3e:
            this->opm_SRL(this->r_hl.value());
            t = 16;
            break;
        case 0x3f:
            this->op_SRL(&this->r_a);
            break;
        case 0x40:
            this->op_Bit(0, &this->r_b);
            break;
        case 0x41:
            this->op_Bit(0, &this->r_c);
            break;
        case 0x42:
            this->op_Bit(0, &this->r_d);
            break;
        case 0x43:
            this->op_Bit(0, &this->r_e);
            break;
        case 0x44:
            this->op_Bit(0, &this->r_h);
            break;
        case 0x45:
            this->op_Bit(0, &this->r_l);
            break;
        case 0x46:
            this->opm_Bit(0, this->r_hl.value());
            t = 16;
            break;
        case 0x47:
            this->op_Bit(0, &this->r_a);
            break;
        case 0x48:
            this->op_Bit(1, &this->r_b);
            break;
        case 0x49:
            this->op_Bit(1, &this->r_c);
            break;
        case 0x4a:
            this->op_Bit(1, &this->r_d);
            break;
        case 0x4b:
            this->op_Bit(1, &this->r_e);
            break;
        case 0x4c:
            this->op_Bit(1, &this->r_h);
            break;
        case 0x4d:
            this->op_Bit(1, &this->r_l);
            break;
        case 0x4e:
            this->opm_Bit(1, this->r_hl.value());
            t = 16;
            break;
        case 0x4f:
            this->op_Bit(1, &this->r_a);
            break;
        case 0x50:
            this->op_Bit(2, &this->r_b);
            break;
        case 0x51:
            this->op_Bit(2, &this->r_c);
            break;
        case 0x52:
            this->op_Bit(2, &this->r_d);
            break;
        case 0x53:
            this->op_Bit(2, &this->r_e);
            break;
        case 0x54:
            this->op_Bit(2, &this->r_h);
            break;
        case 0x55:
            this->op_Bit(2, &this->r_l);
            break;
        case 0x56:
            this->opm_Bit(2, this->r_hl.value());
            t = 16;
            break;
        case 0x57:
            this->op_Bit(2, &this->r_a);
            break;
        case 0x58:
            this->op_Bit(3, &this->r_b);
            break;
        case 0x59:
            this->op_Bit(3, &this->r_c);
            break;
        case 0x5a:
            this->op_Bit(3, &this->r_d);
            break;
        case 0x5b:
            this->op_Bit(3, &this->r_e);
            break;
        case 0x5c:
            this->op_Bit(3, &this->r_h);
            break;
        case 0x5d:
            this->op_Bit(3, &this->r_l);
            break;
        case 0x5e:
            this->opm_Bit(3, this->r_hl.value());
            t = 16;
            break;
        case 0x5f:
            this->op_Bit(3, &this->r_a);
            break;
        case 0x60:
            this->op_Bit(4, &this->r_b);
            break;
        case 0x61:
            this->op_Bit(4, &this->r_c);
            break;
        case 0x62:
            this->op_Bit(4, &this->r_d);
            break;
        case 0x63:
            this->op_Bit(4, &this->r_e);
            break;
        case 0x64:
            this->op_Bit(4, &this->r_h);
            break;
        case 0x65:
            this->op_Bit(4, &this->r_l);
            break;
        case 0x66:
            this->opm_Bit(4, this->r_hl.value());
            t = 16;
            break;
        case 0x67:
            this->op_Bit(4, &this->r_a);
            break;
        case 0x68:
            this->op_Bit(5, &this->r_b);
            break;
        case 0x69:
            this->op_Bit(5, &this->r_c);
            break;
        case 0x6a:
            this->op_Bit(5, &this->r_d);
            break;
        case 0x6b:
            this->op_Bit(5, &this->r_e);
            break;
        case 0x6c:
            this->op_Bit(5, &this->r_h);
            break;
        case 0x6d:
            this->op_Bit(5, &this->r_l);
            break;
        case 0x6e:
            this->opm_Bit(5, this->r_hl.value());
            t = 16;
            break;
        case 0x6f:
            this->op_Bit(5, &this->r_a);
            break;
        case 0x70:
            this->op_Bit(6, &this->r_b);
            break;
        case 0x71:
            this->op_Bit(6, &this->r_c);
            break;
        case 0x72:
            this->op_Bit(6, &this->r_d);
            break;
        case 0x73:
            this->op_Bit(6, &this->r_e);
            break;
        case 0x74:
            this->op_Bit(6, &this->r_h);
            break;
        case 0x75:
            this->op_Bit(6, &this->r_l);
            break;
        case 0x76:
            this->opm_Bit(6, this->r_hl.value());
            t = 16;
            break;
        case 0x77:
            this->op_Bit(6, &this->r_a);
            break;
        case 0x78:
            this->op_Bit(7, &this->r_b);
            break;
        case 0x79:
            this->op_Bit(7, &this->r_c);
            break;
        case 0x7a:
            this->op_Bit(7, &this->r_d);
            break;
        case 0x7b:
            this->op_Bit(7, &this->r_e);
            break;
        case 0x7c:
            this->op_Bit(7, &this->r_h);
            break;
        case 0x7d:
            this->op_Bit(7, &this->r_l);
            break;
        case 0x7e:
            this->opm_Bit(7, this->r_hl.value());
            t = 16;
            break;
        case 0x7f:
            this->op_Bit(7, &this->r_a);
            break;
        case 0x80:
            this->op_Res(0, &this->r_b);
            break;
        case 0x81:
            this->op_Res(0, &this->r_c);
            break;
        case 0x82:
            this->op_Res(0, &this->r_d);
            break;
        case 0x83:
            this->op_Res(0, &this->r_e);
            break;
        case 0x84:
            this->op_Res(0, &this->r_h);
            break;
        case 0x85:
            this->op_Res(0, &this->r_l);
            break;
        case 0x86:
            this->opm_Res(0, this->r_hl.value());
            t = 16;
            break;
        case 0x87:
            this->op_Res(0, &this->r_a);
            break;
        case 0x88:
            this->op_Res(1, &this->r_b);
            break;
        case 0x89:
            this->op_Res(1, &this->r_c);
            break;
        case 0x8a:
            this->op_Res(1, &this->r_d);
            break;
        case 0x8b:
            this->op_Res(1, &this->r_e);
            break;
        case 0x8c:
            this->op_Res(1, &this->r_h);
            break;
        case 0x8d:
            this->op_Res(1, &this->r_l);
            break;
        case 0x8e:
            this->opm_Res(1, this->r_hl.value());
            t = 16;
            break;
        case 0x8f:
            this->op_Res(1, &this->r_a);
            break;
        case 0x90:
            this->op_Res(2, &this->r_b);
            break;
        case 0x91:
            this->op_Res(2, &this->r_c);
            break;
        case 0x92:
            this->op_Res(2, &this->r_d);
            break;
        case 0x93:
            this->op_Res(2, &this->r_e);
            break;
        case 0x94:
            this->op_Res(2, &this->r_h);
            break;
        case 0x95:
            this->op_Res(2, &this->r_l);
            break;
        case 0x96:
            this->opm_Res(2, this->r_hl.value());
            t = 16;
            break;
        case 0x97:
            this->op_Res(2, &this->r_a);
            break;
        case 0x98:
            this->op_Res(3, &this->r_b);
            break;
        case 0x99:
            this->op_Res(3, &this->r_c);
            break;
        case 0x9a:
            this->op_Res(3, &this->r_d);
            break;
        case 0x9b:
            this->op_Res(3, &this->r_e);
            break;
        case 0x9c:
            this->op_Res(3, &this->r_h);
            break;
        case 0x9d:
            this->op_Res(3, &this->r_l);
            break;
        case 0x9e:
            this->opm_Res(3, this->r_hl.value());
            t = 16;
            break;
        case 0x9f:
            this->op_Res(3, &this->r_a);
            break;
        case 0xa0:
            this->op_Res(4, &this->r_b);
            break;
        case 0xa1:
            this->op_Res(4, &this->r_c);
            break;
        case 0xa2:
            this->op_Res(4, &this->r_d);
            break;
        case 0xa3:
            this->op_Res(4, &this->r_e);
            break;
        case 0xa4:
            this->op_Res(4, &this->r_h);
            break;
        case 0xa5:
            this->op_Res(4, &this->r_l);
            break;
        case 0xa6:
            this->opm_Res(4, this->r_hl.value());
            t = 16;
            break;
        case 0xa7:
            this->op_Res(4, &this->r_a);
            break;
        case 0xa8:
            this->op_Res(5, &this->r_b);
            break;
        case 0xa9:
            this->op_Res(5, &this->r_c);
            break;
        case 0xaa:
            this->op_Res(5, &this->r_d);
            break;
        case 0xab:
            this->op_Res(5, &this->r_e);
            break;
        case 0xac:
            this->op_Res(5, &this->r_h);
            break;
        case 0xad:
            this->op_Res(5, &this->r_l);
            break;
        case 0xae:
            this->opm_Res(5, this->r_hl.value());
            t = 16;
            break;
        case 0xaf:
            this->op_Res(5, &this->r_a);
            break;
        case 0xb0:
            this->op_Res(6, &this->r_b);
            break;
        case 0xb1:
            this->op_Res(6, &this->r_c);
            break;
        case 0xb2:
            this->op_Res(6, &this->r_d);
            break;
        case 0xb3:
            this->op_Res(6, &this->r_e);
            break;
        case 0xb4:
            this->op_Res(6, &this->r_h);
            break;
        case 0xb5:
            this->op_Res(6, &this->r_l);
            break;
        case 0xb6:
            this->opm_Res(6, this->r_hl.value());
            t = 16;
            break;
        case 0xb7:
            this->op_Res(6, &this->r_a);
            break;
        case 0xb8:
            this->op_Res(7, &this->r_b);
            break;
        case 0xb9:
            this->op_Res(7, &this->r_c);
            break;
        case 0xba:
            this->op_Res(7, &this->r_d);
            break;
        case 0xbb:
            this->op_Res(7, &this->r_e);
            break;
        case 0xbc:
            this->op_Res(7, &this->r_h);
            break;
        case 0xbd:
            this->op_Res(7, &this->r_l);
            break;
        case 0xbe:
            this->opm_Res(7, this->r_hl.value());
            t = 16;
            break;
        case 0xbf:
            this->op_Res(7, &this->r_a);
            break;
        case 0xc0:
            this->op_Set(0, &this->r_b);
            break;
        case 0xc1:
            this->op_Set(0, &this->r_c);
            break;
        case 0xc2:
            this->op_Set(0, &this->r_d);
            break;
        case 0xc3:
            this->op_Set(0, &this->r_e);
            break;
        case 0xc4:
            this->op_Set(0, &this->r_h);
            break;
        case 0xc5:
            this->op_Set(0, &this->r_l);
            break;
        case 0xc6:
            this->opm_Set(0, this->r_hl.value());
            t = 16;
            break;
        case 0xc7:
            this->op_Set(0, &this->r_a);
            break;
        case 0xc8:
            this->op_Set(1, &this->r_b);
            break;
        case 0xc9:
            this->op_Set(1, &this->r_c);
            break;
        case 0xca:
            this->op_Set(1, &this->r_d);
            break;
        case 0xcb:
            this->op_Set(1, &this->r_e);
            break;
        case 0xcc:
            this->op_Set(1, &this->r_h);
            break;
        case 0xcd:
            this->op_Set(1, &this->r_l);
            break;
        case 0xce:
            this->opm_Set(1, this->r_hl.value());
            t = 16;
            break;
        case 0xcf:
            this->op_Set(1, &this->r_a);
            break;
        case 0xd0:
            this->op_Set(2, &this->r_b);
            break;
        case 0xd1:
            this->op_Set(2, &this->r_c);
            break;
        case 0xd2:
            this->op_Set(2, &this->r_d);
            break;
        case 0xd3:
            this->op_Set(2, &this->r_e);
            break;
        case 0xd4:
            this->op_Set(2, &this->r_h);
            break;
        case 0xd5:
            this->op_Set(2, &this->r_l);
            break;
        case 0xd6:
            this->opm_Set(2, this->r_hl.value());
            t = 16;
            break;
        case 0xd7:
            this->op_Set(2, &this->r_a);
            break;
        case 0xd8:
            this->op_Set(3, &this->r_b);
            break;
        case 0xd9:
            this->op_Set(3, &this->r_c);
            break;
        case 0xda:
            this->op_Set(3, &this->r_d);
            break;
        case 0xdb:
            this->op_Set(3, &this->r_e);
            break;
        case 0xdc:
            this->op_Set(3, &this->r_h);
            break;
        case 0xdd:
            this->op_Set(3, &this->r_l);
            break;
        case 0xde:
            this->opm_Set(3, this->r_hl.value());
            t = 16;
            break;
        case 0xdf:
            this->op_Set(3, &this->r_a);
            break;
        case 0xe0:
            this->op_Set(4, &this->r_b);
            break;
        case 0xe1:
            this->op_Set(4, &this->r_c);
            break;
        case 0xe2:
            this->op_Set(4, &this->r_d);
            break;
        case 0xe3:
            this->op_Set(4, &this->r_e);
            break;
        case 0xe4:
            this->op_Set(4, &this->r_h);
            break;
        case 0xe5:
            this->op_Set(4, &this->r_l);
            break;
        case 0xe6:
            this->opm_Set(4, this->r_hl.value());
            t = 16;
            break;
        case 0xe7:
            this->op_Set(4, &this->r_a);
            break;
        case 0xe8:
            this->op_Set(5, &this->r_b);
            break;
        case 0xe9:
            this->op_Set(5, &this->r_c);
            break;
        case 0xea:
            this->op_Set(5, &this->r_d);
            break;
        case 0xeb:
            this->op_Set(5, &this->r_e);
            break;
        case 0xec:
            this->op_Set(5, &this->r_h);
            break;
        case 0xed:
            this->op_Set(5, &this->r_l);
            break;
        case 0xee:
            this->opm_Set(5, this->r_hl.value());
            t = 16;
            break;
        case 0xef:
            this->op_Set(5, &this->r_a);
            break;
        case 0xf0:
            this->op_Set(6, &this->r_b);
            break;
        case 0xf1:
            this->op_Set(6, &this->r_c);
            break;
        case 0xf2:
            this->op_Set(6, &this->r_d);
            break;
        case 0xf3:
            this->op_Set(6, &this->r_e);
            break;
        case 0xf4:
            this->op_Set(6, &this->r_h);
            break;
        case 0xf5:
            this->op_Set(6, &this->r_l);
            break;
        case 0xf6:
            this->opm_Set(6, this->r_hl.value());
            t = 16;
            break;
        case 0xf7:
            this->op_Set(6, &this->r_a);
            break;
        case 0xf8:
            this->op_Set(7, &this->r_b);
            break;
        case 0xf9:
            this->op_Set(7, &this->r_c);
            break;
        case 0xfa:
            this->op_Set(7, &this->r_d);
            break;
        case 0xfb:
            this->op_Set(7, &this->r_e);
            break;
        case 0xfc:
            this->op_Set(7, &this->r_h);
            break;
        case 0xfd:
            this->op_Set(7, &this->r_l);
            break;
        case 0xfe:
            this->opm_Set(7, this->r_hl.value());
            t = 16;
            break;
        case 0xff:
            this->op_Set(7, &this->r_a);
            break;
        default:
            std::cout << "Unknown CB op code: ";
            std::cout << std::hex << op_val;
            std::cout << std::endl;
            if (STOP_ON_BAD_OPCODE) {
                this->running = false;
                this->stepped_in = true;
            }
            break;
    }
    return t;
}


// Perform XOR of registry against A and then store
// result in A
void CPU::opm_XOR(uint16_t mem_addr) {
    this->op_XOR(this->ram->get_val(mem_addr));
}
void CPU::op_XOR(reg8 *comp) {
    this->op_XOR(comp->get_value());

}
void CPU::op_XOR(uint8_t val) {
    this->r_a.set_value(this->r_a.get_value() ^ val);
    this->set_register_bit(&this->r_f, this->SUBTRACT_FLAG_BIT, 0U);
    this->set_register_bit(&this->r_f, this->HALF_CARRY_FLAG_BIT, 0U);
    this->set_register_bit(&this->r_f, this->CARRY_FLAG_BIT, 0U);
    this->set_zero_flag(this->r_a.get_value());
}

// AND operators - And with the A register value, set result to A
void CPU::op_AND() {
    uint8_t comp = this->get_inc_pc_val8();
    this->op_AND(comp);
}
void CPU::op_AND(reg8 *comp) {
    this->op_AND(comp->get_value());
}
void CPU::opm_AND(uint16_t mem_addr)
{
    this->op_AND(this->ram->get_val(mem_addr));
}
void CPU::op_AND(uint8_t comp) {
    this->r_a.set_value(this->r_a.get_value() & comp);
    this->set_zero_flag(this->r_a.get_value());
    this->set_register_bit(&this->r_f, this->SUBTRACT_FLAG_BIT, 0U);
    this->set_register_bit(&this->r_f, this->HALF_CARRY_FLAG_BIT, 1U);
    this->set_register_bit(&this->r_f, this->CARRY_FLAG_BIT, 0U);
}

// OR operators - OR with the A register value, set result to A
void CPU::op_OR() {
    this->op_OR(this->get_inc_pc_val8());
}
void CPU::op_OR(reg8 *comp) {
    this->op_OR(comp->get_value());
}
void CPU::opm_OR(uint16_t mem_addr)
{
    this->op_OR(this->ram->get_val(mem_addr));
}
void CPU::op_OR(uint8_t comp) {
    uint8_t res = this->r_a.get_value() | comp;
    this->r_a.set_value(res);
    this->set_zero_flag(res);
    this->set_register_bit(&this->r_f, this->SUBTRACT_FLAG_BIT, 0U);
    this->set_register_bit(&this->r_f, this->HALF_CARRY_FLAG_BIT, 0U);
    this->set_register_bit(&this->r_f, this->CARRY_FLAG_BIT, 0U);
}

// Set single bit of a given register to a given value
void CPU::set_register_bit(reg8 *source, uint8_t bit_shift, unsigned int val) {
    if (val == 1)
        // OR the register with bit shifted 1
        source->set_value(source->get_value() | (1 << bit_shift));
    else
        // XOR 0xff with bit to be unset.
        // AND register with this value, meaning only
        // specified bit is masked.
        source->set_value(source->get_value() & (0xff ^ (1 << bit_shift)));
}

// Obtain the value of a given bit of a given register
uint8_t CPU::get_register_bit(reg8 *source, unsigned int bit_shift) {
    // Bit shift 1 by bit to retrieve and AND with register value.
    return ((source->get_value() & (1  << bit_shift)) >> bit_shift);
}

// Set zero flag, based on the value of a given register
void CPU::set_zero_flag(const uint8_t val) {
    this->set_register_bit(&this->r_f, this->ZERO_FLAG_BIT, ((val == 0) ? 1U : 0U));
}

uint8_t CPU::get_zero_flag() {
    return this->get_register_bit(&this->r_f, this->ZERO_FLAG_BIT);
}

uint8_t CPU::get_carry_flag() {
    return this->get_register_bit(&this->r_f, this->CARRY_FLAG_BIT);
}

uint8_t CPU::get_half_carry_flag() {
    return this->get_register_bit(&this->r_f, this->HALF_CARRY_FLAG_BIT);
}

uint8_t CPU::get_subtract_flag() {
    return this->get_register_bit(&this->r_f, this->SUBTRACT_FLAG_BIT);
}

void CPU::flip_carry_flag() {
    this->set_register_bit(&this->r_f, this->CARRY_FLAG_BIT, this->get_carry_flag() ? 0U : 1U);
}

void CPU::flip_half_carry_flag() {
    this->set_register_bit(&this->r_f, this->HALF_CARRY_FLAG_BIT, this->get_half_carry_flag() ? 0U : 1U);
}

void CPU::set_half_carry(uint16_t original_val, uint16_t input) {
    // @TODO Check this implimentation
    this->set_register_bit(
        &this->r_f,
        this->HALF_CARRY_FLAG_BIT,
        // XOR the original and new values' 5th BIT.
        // This means it will result in half carry if the value has changed.
        //((0x10 & original_val) >> 4) ^ ((0x10 & input) >> 4));
        (((original_val & 0x000f) + (input & 0x000f)) & 0x0010) == 0x0010 ? 1U : 0U);
}
void CPU::set_half_carry16(uint16_t original_val, uint16_t input) {
    // @TODO Check this implimentation
    this->set_register_bit(
        &this->r_f,
        this->HALF_CARRY_FLAG_BIT,
        // XOR the original and new values' 5th BIT.
        // This means it will result in half carry if the value has changed.
        (((original_val & 0x0fff) + (input & 0x0fff)) & 0x1000) ? 1U : 0U);
}
void CPU::set_half_carry_sub(uint8_t original_val, uint8_t input) {
    // @TODO Check this implimentation
    // Create Test-bed, which sets up half-byte (lower half-byte) of data.
    // Remove original value and determine if the uppper nibble of data is affected.
    uint8_t test = 0xf0 | original_val;
    test = test - (input & 0x0f);
    this->set_register_bit(
        &this->r_f,
        this->HALF_CARRY_FLAG_BIT,
        // XOR the original and new values' 5th BIT.
        // This means it will result in half carry if the value has changed.
        //((0x10 & original_val) >> 4) ^ ((0x10 & input) >> 4));
        //(test < 0xf0) ? 1U : 0U);
        ((((int)original_val & 0xF) - ((int)input & 0xF)) < 0) ? 1U : 0U);
}
void CPU::set_half_carry_sub2(uint8_t original_val, uint8_t input) {
    this->set_register_bit(
        &this->r_f,
        this->HALF_CARRY_FLAG_BIT,
        ((input & 0x0f) > (original_val & 0x0f)) ? 1 : 0
    );
}
void CPU::set_half_carry_sub16(uint16_t original_val, uint16_t input) {
    // @TODO Check this implimentation
    this->set_register_bit(
        &this->r_f,
        this->HALF_CARRY_FLAG_BIT,
        // XOR the original and new values' 5th BIT.
        // This means it will result in half carry if the value has changed.
        ((input & 0xff) > (original_val & 0xff)) ? 1U : 0U);
}

// Get value from memory at PC and increment PC
uint8_t CPU::get_inc_pc_val8()
{
    uint8_t ori_val = this->ram->get_val(this->r_pc.get_value());
    if (DEBUG || this->stepped_in)
        std::cout << "Got PC value from RAM: " << std::hex << (unsigned int)ori_val << " at " << (unsigned int)this->r_pc.get_value() << std::endl;
    uint8_t val;
    memcpy(&val, &ori_val, 1);
    this->r_pc.set_value(this->r_pc.get_value() + 1);
    return val;
}

// Get value from memory at PC, treat as signed and increment PC
int8_t CPU::get_inc_pc_val8s()
{
    return convert_signed_uint8_to_int8(this->get_inc_pc_val8());
}

// Get 2-byte value from memory address at PC,
// incrementing the PC past this value
uint16_t CPU::get_inc_pc_val16()
{
    union {
        uint8_t bit8[2];
        uint16_t bit16[1];
    } data_conv;

    data_conv.bit8[0] = this->get_inc_pc_val8();
    data_conv.bit8[1] = this->get_inc_pc_val8();
    return data_conv.bit16[0];
}

// Get value from specified register, decrement and store
// in memory (using address of two registers)
void CPU::op_Load_Dec(combined_reg *dest, reg8 *source) {
    this->opm_Load(dest->value(), source);
    this->op_Dec(dest);
}
void CPU::op_Load_Dec(reg8 *dest, combined_reg *source) {
    this->opm_Load(dest, source->value());
    this->op_Dec(source);
}

// Get value from specified register, increment and store
// in memory (using address of two registers)
void CPU::op_Load_Inc(combined_reg *dest, reg8 *source) {
    this->opm_Load(dest->value(), source);
    this->op_Inc(dest);
}
void CPU::op_Load_Inc(reg8 *dest, combined_reg *source) {
    this->opm_Load(dest, source->value());
    this->op_Inc(source);
}

void CPU::op_CPL()
{
    this->r_a.set_value(this->r_a.get_value() ^ 0xff);
    this->set_register_bit(&this->r_f, this->SUBTRACT_FLAG_BIT, 1U);
    this->set_register_bit(&this->r_f, this->HALF_CARRY_FLAG_BIT, 1U);
}

void CPU::op_CCF()
{
    this->set_register_bit(&this->r_f, this->SUBTRACT_FLAG_BIT, 0U);
    this->set_register_bit(&this->r_f, this->HALF_CARRY_FLAG_BIT, 0U);
    this->set_register_bit(&this->r_f, this->CARRY_FLAG_BIT, this->get_carry_flag() ? 0U : 1U);
}

void CPU::op_Bit(unsigned int bit, reg8 *comp) {
    // Set flags accordinly before operation
    this->set_zero_flag(this->get_register_bit(comp, bit));
    this->set_register_bit(&this->r_f, this->SUBTRACT_FLAG_BIT, 0U);
    this->set_register_bit(&this->r_f, this->HALF_CARRY_FLAG_BIT, 1U);
}

void CPU::opm_Bit(unsigned int bit, uint16_t mem_addr) {
    // Set flags accordinly before operation
    this->set_zero_flag(this->ram->get_ram_bit(mem_addr, bit));
    this->set_register_bit(&this->r_f, this->SUBTRACT_FLAG_BIT, 0U);
    this->set_register_bit(&this->r_f, this->HALF_CARRY_FLAG_BIT, 1U);
}

// Set single bit in a given register
void CPU::op_Res(uint8_t bit, reg8 *dest) {
    this->set_register_bit(dest, bit, 0U);
}
void CPU::opm_Res(uint8_t bit, uint16_t mem_addr) {
    this->ram->set_ram_bit(mem_addr, bit, 0U);
}
void CPU::op_Set(uint8_t bit, reg8 *dest) {
    this->set_register_bit(dest, bit, 1U);
}
void CPU::opm_Set(uint8_t bit, uint16_t mem_addr) {
    this->ram->set_ram_bit(mem_addr, bit, 1U);
}


void CPU::op_DAA()
{
    if (this->get_subtract_flag() == 0x00) {
        if (this->get_carry_flag() || this->r_a.get_value() > 0x99) {
            this->set_register_bit(&this->r_f, this->CARRY_FLAG_BIT, 1U);
            this->r_a.set_value(this->r_a.get_value() + 0x60);
        }
        if (this->get_half_carry_flag() || (this->r_a.get_value() & 0x0f) > 0x09) {
            this->r_a.set_value(this->r_a.get_value() + 0x06);
        }
    } else {
        if (this->get_carry_flag()) {
            this->r_a.set_value(this->r_a.get_value() - 0x60);
        }
        if (this->get_half_carry_flag()) {
            this->r_a.set_value(this->r_a.get_value() - 0x06);
        }
    }

    this->set_zero_flag(this->r_a.get_value());
    this->set_register_bit(&this->r_f, this->HALF_CARRY_FLAG_BIT, 0U);
}

// Add 8bit PC value and carry flag to A.
void CPU::op_Adc(reg8 *dest) {
    uint8_t source = this->get_inc_pc_val8();
    this->op_Adc(dest, source);
}
void CPU::op_Adc(reg8 *dest, reg8 *source) {
    this->op_Adc(dest, source->get_value());
}
void CPU::opm_Adc(reg8 *dest, uint16_t mem_addr) {
    this->op_Adc(dest, this->ram->get_val(mem_addr));
}
void CPU::op_Adc(reg8 *dest, uint8_t source) {

    // Always work with r_a
    uint8_t original_val = dest->get_value();
    this->data_conv.bit16[0] = 0;
    this->data_conv.bit8[0] = dest->get_value();
    this->data_conv.bit16[0] += source;
    this->data_conv.bit16[0] += this->get_carry_flag();
    dest->set_value(this->data_conv.bit8[0]);

    // Set zero flag
    this->set_zero_flag(dest->get_value());

    // Set subtract flag to 0
    this->set_register_bit(&this->r_f, this->SUBTRACT_FLAG_BIT, 0U);

    // Handle edge-case where source is ffff and carry flag is set,
    // since adding these in the parameters will overflow into 0x00
    if ((source & 0x0f) == 0x0f && this->get_carry_flag() == 0x01)
        this->set_register_bit(&this->r_f, this->HALF_CARRY_FLAG_BIT, 0x01);
    else
        // Determine half carry flag based on 5th bit of first byte
        this->set_half_carry(original_val, source + this->get_carry_flag());

    // Set carry flag, based on 1st bit of second byte
    this->set_register_bit(&this->r_f, this->CARRY_FLAG_BIT, (this->data_conv.bit8[1] & 0x01));

}


// @TODO: Move these

////////////////////////// Load OPs //////////////////////////
void CPU::op_Load(reg8 *dest) {
    dest->set_value(this->get_inc_pc_val8());
}
// Load next byte into provided address
void CPU::opm_Load(uint16_t dest) {
    uint8_t val = this->get_inc_pc_val8();
    this->opm_Load(dest, val);
}
void CPU::op_Load(combined_reg *dest) {
    dest->lower->set_value(this->get_inc_pc_val8());
    dest->upper->set_value(this->get_inc_pc_val8());
}
void CPU::op_Load(reg16 *dest) {
    dest->set_value(this->get_inc_pc_val16());

}
void CPU::opm_Load(uint16_t dest_addr, reg16 *source) {
    // Obtain 2-byte source value
    this->data_conv.bit16[0] = source->get_value();

    // Set MS-byte into first address
    this->ram->set(dest_addr, this->data_conv.bit8[0]);
    // Set LS-byte into following address
    this->ram->set(dest_addr + 1, this->data_conv.bit8[1]);
}
// Copy 1 byte between registers
void CPU::op_Load(reg8 *dest, reg8 *source) {
    dest->set_value(source->get_value());
}
// Copy register value into destination address of memory
void CPU::opm_Load(uint16_t dest_addr, reg8 *source) {
    this->opm_Load(dest_addr, source->get_value());
}
void CPU::opm_Load(uint16_t dest_addr, uint8_t val) {
    this->ram->set(dest_addr, val);
}
// Copy data from source memory address to destination
void CPU::opm_Load(reg8 *dest, uint16_t source_addr) {
    dest->set_value(this->ram->get_val(source_addr));
}
void CPU::op_Load(combined_reg *dest, uint16_t val) {
    dest->set_value(val);
}
void CPU::op_Load(reg16 *dest, combined_reg *src) {
    dest->set_value(src->value());
}

void CPU::op_LoadSpN(combined_reg* dest)
{
    // Obtain value from memory at SP, add signed 8 bit value and put into combined reg
    uint16_t original_val = this->r_sp.get_value();

    int8_t val = this->get_inc_pc_val8s();

    uint16_t src_uint16 = (uint16_t)val;

    uint16_t result = original_val + val;
    dest->lower->set_value(result & 0xff);
    dest->upper->set_value((result & 0xff00) >> 8);

    this->set_register_bit(
        &this->r_f,
        this->HALF_CARRY_FLAG_BIT,
        ((original_val & 0x000f) + (src_uint16 & 0x000f) > 0x000f) ? 1U : 0U
    );

    this->set_register_bit(
        &this->r_f,
        this->CARRY_FLAG_BIT,
        ((original_val & 0x00ff) + (src_uint16 & 0x00ff) > 0x00ff) ? 1U : 0U);

    this->set_register_bit(&this->r_f, this->SUBTRACT_FLAG_BIT, 0U);
    this->set_register_bit(&this->r_f, this->ZERO_FLAG_BIT, 0U);
}


////////////////////////// General arithmatic OPs //////////////////////////
void CPU::op_Add(reg8 *dest) {
    uint16_t source = (0x0000 | this->get_inc_pc_val8());
    this->op_Add(dest, source);
}
void CPU::op_Add(reg8 *dest, reg8 *src) {
    this->op_Add(dest, (uint16_t)(src->get_value() | 0x0000));
}
void CPU::opm_Add(reg8 *dest, uint16_t mem_addr) {
    this->op_Add(dest, (uint16_t)(this->ram->get_val(mem_addr) | 0x0000));
}
inline void CPU::op_Add(reg8 *dest, uint16_t src) {
    uint8_t original_val = dest->get_value();

    this->data_conv.bit8[0] = dest->get_value();
    this->data_conv.bit8[1] = 0;

    this->data_conv.bit16[0] += src;


    this->set_zero_flag(this->data_conv.bit8[0]);
    this->set_half_carry(original_val, (uint8_t)src);
    dest->set_value(this->data_conv.bit8[0]);

    // Set subtract flag to 0, since this is add
    this->set_register_bit(&this->r_f, this->SUBTRACT_FLAG_BIT, 0U);
    this->set_register_bit(
        &this->r_f, this->CARRY_FLAG_BIT,
        (0x01 & this->data_conv.bit8[1]));

}

void CPU::op_Add(combined_reg *dest, combined_reg *src) {
    this->op_Add(dest, (uint32_t)src->value());
}
void CPU::op_Add(combined_reg *dest, reg16 *src) {
    this->op_Add(dest, (uint32_t)src->get_value());
}
inline void CPU::op_Add(combined_reg *dest, uint32_t src) {
    uint16_t original_val = dest->value();

    this->data_conv32.bit16[0] = dest->value();
    this->data_conv32.bit16[1] = 0;

    this->data_conv32.bit32[0] += src;

    dest->lower->set_value(this->data_conv32.bit8[0]);
    dest->upper->set_value(this->data_conv32.bit8[1]);

    this->set_half_carry16(original_val, (uint16_t)src);

    // Set subtract flag to 0, since this is add
    this->set_register_bit(&this->r_f, this->SUBTRACT_FLAG_BIT, 0U);
    this->set_register_bit(
        &this->r_f, this->CARRY_FLAG_BIT,
        (0x01 & this->data_conv32.bit16[1]) >> 0);
}

void CPU::op_Add(reg16 *dest, int8_t val) {
    // Add signed byte to reg16
    uint16_t original_val = dest->get_value();
    uint16_t src_uint16 = (uint16_t)val;

    dest->set_value(dest->get_value() + val);

    this->set_register_bit(
        &this->r_f,
        this->HALF_CARRY_FLAG_BIT,
        ((original_val & 0x000f) + (src_uint16 & 0x000f) > 0x000f) ? 1U : 0U
    );

    this->set_register_bit(
        &this->r_f,
        this->CARRY_FLAG_BIT,
        ((original_val & 0x00ff) + (src_uint16 & 0x00ff) > 0x00ff) ? 1U : 0U);

    this->set_register_bit(&this->r_f, this->SUBTRACT_FLAG_BIT, 0U);
    this->set_register_bit(&this->r_f, this->ZERO_FLAG_BIT, 0U);
}
void CPU::op_Add(reg16 *dest) {
    // Get byte from next byte, treat as signed 8-bit value
    int8_t source = this->get_inc_pc_val8();
    // Add to value of dest
    dest->set_value(dest->get_value() + source);
}

void CPU::op_Sub() {
    uint16_t source = 0x00ff & this->get_inc_pc_val8();
    this->op_Sub(source);
}
void CPU::op_Sub(reg8 *src) {
    this->op_Sub((uint16_t)src->get_value() & 0x00ff);
}
void CPU::opm_Sub(uint16_t mem_addr) {
    this->op_Sub(this->ram->get_val(mem_addr));
}
void CPU::op_Sub(uint8_t src) {
    uint8_t original_val = this->r_a.get_value();
    //std::cout << std::hex << "subtracting " << (int)src << " from " << (int)this->r_a.get_value() << std::endl;

    this->data_conv.bit8[0] = this->r_a.get_value();
    this->data_conv.bit8[1] = 0;

    this->data_conv.bit16[0] -= src;

    this->r_a.set_value(this->data_conv.bit8[0]);

    this->set_zero_flag(this->r_a.get_value());

    this->set_register_bit(&this->r_f, this->SUBTRACT_FLAG_BIT, 1U);

    // @RULE AS per GCPUMan P83:
    // H - Set if no borrow from bit 4.
    // C - Set if no borrow.
    // Use sub half_carry method and
    // reverse logic in setting carry flag
    // TESTED USING 09-op r,r.gb
    this->set_half_carry_sub2(original_val, src);
    this->set_register_bit(
        &this->r_f, this->CARRY_FLAG_BIT,
        ((original_val < src) ? 1U : 0U));

//    // If 0x0100 is removed, set half carry flag
//    if (src == 0x0100) {
//        this->set_register_bit(&this->r_f, this->HALF_CARRY_FLAG_BIT, 0U);
//        this->set_register_bit(&this->r_f, this->CARRY_FLAG_BIT, 0U);
//    }

    if (this->op_val == 0xde) {
    std::cout << std::endl;
    std::cout << std::hex << "original value: " << (unsigned int)original_val << std::endl;

    std::cout << std::hex << "value to remove: " << (unsigned int)src << std::endl;

    std::cout << std::hex << "output value: " << (unsigned int)this->r_a.get_value() << std::endl;
    std::cout << std::hex << "carry: " << (unsigned int)this->get_carry_flag() << std::endl;
    std::cout << std::hex << "hary-carry: " << (unsigned int)this->get_half_carry_flag() << std::endl;
    std::cout << std::hex << "zero: " << (unsigned int)this->get_zero_flag() << std::endl;
    std::cout << std::hex << "subbtract: " << (unsigned int)this->get_subtract_flag() << std::endl;
    }
}

void CPU::op_SBC(reg8 *src)
{
    this->op_SBC_common(src->get_value());
}

void CPU::op_SBC()
{
    this->op_SBC_common(this->get_inc_pc_val8());
}

void CPU::opm_SBC(uint16_t mem_addr)
{
    // Subtract src plus carry flag
    this->op_SBC_common(this->ram->get_val(mem_addr));
}

void CPU::op_SBC_common(uint8_t value)
{
    uint16_t combined_value = value + this->get_carry_flag();

    // Calculate half-carry flag using 0x0f of initial value without carry flag, plus carry flag afterwards.
    uint8_t half_carry_flag = ((value & 0x0f) + this->get_carry_flag()) > (this->r_a.get_value() & 0x0f) ? 1U : 0U;

    // Subtract src plus carry flag
    this->op_Sub((uint8_t)combined_value);
    
    // Handle case where 
    if (combined_value == 0x0100U)
        this->set_register_bit(&this->r_f, this->CARRY_FLAG_BIT, 1U);
        
    // Handle SBC-specific carry flag
    this->set_register_bit(&this->r_f, this->HALF_CARRY_FLAG_BIT, half_carry_flag);
}

void CPU::op_Inc(reg8 *src)
{
    // Calls op_Inc(uint8_t)
    src->set_value(this->op_Inc(src->get_value()));
}
void CPU::opm_Inc(uint16_t mem_addr)
{
    // Calls op_Inc(uint8_t)
    this->ram->set(mem_addr, this->op_Inc(this->ram->get_val(mem_addr)));
}
uint8_t CPU::op_Inc(uint8_t val) {
    union {
        uint8_t bit8[2];
        uint16_t bit16[1];
    } data_conv;

    uint8_t original_val = val;
    data_conv.bit16[0] = 0;
    data_conv.bit8[0] = val;
    data_conv.bit16[0] ++;
    val = data_conv.bit8[0];

    // Set zero flag
    this->set_zero_flag(val);

    // Set subtract flag to 0
    this->set_register_bit(&this->r_f, this->SUBTRACT_FLAG_BIT, 0U);

    // Determine half carry flag based on 5th bit of first byte
    this->set_half_carry(original_val, 0x01);
    return val;
}


void CPU::op_Inc(combined_reg *dest) {
    union {
        uint8_t bit8[4];
        uint16_t bit16[2];
        uint32_t bit32[1];
    } data_conv;

    data_conv.bit8[0] = dest->lower->get_value();
    data_conv.bit8[1] = dest->upper->get_value();
    data_conv.bit32[0] = (uint32_t)((unsigned int)(data_conv.bit32[0]) + 1);
    dest->lower->set_value(data_conv.bit8[0]);
    dest->upper->set_value(data_conv.bit8[1]);
    //std::cout << "BUNNNNN" << std::endl;
}
void CPU::op_Inc(reg16 *dest) {
    //std::cout << "RUNNNNN" << std::endl;
    this->data_conv32.bit16[0] = dest->get_value();
    this->data_conv32.bit16[1] = 0;
    this->data_conv32.bit32[0] = (uint32_t)((unsigned int)(this->data_conv32.bit32[0]) + 1);
    dest->set_value(this->data_conv32.bit16[0]);
}
void CPU::op_Dec(reg16 *dest) {
    this->data_conv32.bit16[0] = dest->get_value();
    this->data_conv32.bit16[1] = 0;
    this->data_conv32.bit32[0] = (uint32_t)((unsigned int)(this->data_conv32.bit32[0]) - 1);
    dest->set_value(this->data_conv32.bit16[0]);
}
void CPU::op_Dec(combined_reg *dest) {
    this->data_conv32.bit16[0] = dest->value();
    this->data_conv32.bit16[1] = 0;
    this->data_conv32.bit32[0] = (uint32_t)((int)(this->data_conv32.bit32[0]) - 1);
    dest->lower->set_value(this->data_conv32.bit8[0]);
    dest->upper->set_value(this->data_conv32.bit8[1]);
}
void CPU::op_Dec(reg8 *src)
{
    src->set_value(this->op_Dec(src->get_value()));
}
void CPU::opm_Dec(uint16_t mem_addr)
{
    this->ram->set(mem_addr, this->op_Dec(this->ram->get_val(mem_addr)));
}
uint8_t CPU::op_Dec(uint8_t val) {
    uint8_t original_val = val;
    this->data_conv.bit8[0] = val;
    this->data_conv.bit8[1] = 0;
    this->data_conv.bit16[0] = (uint16_t)((int)(this->data_conv.bit16[0]) - 1);
    val = this->data_conv.bit8[0];

    // Set zero flag
    this->set_zero_flag(val);

    // Set subtract flag to 1
    this->set_register_bit(&this->r_f, this->SUBTRACT_FLAG_BIT, 1U);

    // Determine half carry flag based on 5th bit of first byte
    this->set_half_carry_sub(original_val, 0x01);
    return val;
}

////////////////////////// bit-related OPs //////////////////////////
void CPU::op_CP() {
    this->op_CP(this->get_inc_pc_val8());
}
void CPU::op_CP(reg8 *in) {
    this->op_CP(in->get_value());
}
void CPU::opm_CP(uint16_t mem_addr) {
    this->op_CP(this->ram->get_val(mem_addr));
}
// Compare 8 bit value against value in register a
void CPU::op_CP(uint8_t in) {

    // Set zero flag based on the result of comparison
    this->set_register_bit(&this->r_f, this->ZERO_FLAG_BIT, (this->r_a.get_value() == in) ? 1U : 0U);

    // Always set subtract flag (since that is what we're doing!)
    this->set_register_bit(&this->r_f, this->SUBTRACT_FLAG_BIT, 1U);
    this->set_half_carry_sub(this->r_a.get_value(), in);
    this->set_register_bit(&this->r_f, this->CARRY_FLAG_BIT, ((unsigned int)this->r_a.get_value() < (unsigned int)in) ? 1U : 0U);
}

void CPU::opm_Swap(uint16_t mem_addr)
{
    uint8_t val = this->ram->get_val(mem_addr);
    val = (((val & 0x0F) << 4) | ((val & 0xF0) >> 4));
    this->ram->set(mem_addr, val);
    this->set_register_bit(&this->r_f, this->SUBTRACT_FLAG_BIT, 0U);
    this->set_register_bit(&this->r_f, this->HALF_CARRY_FLAG_BIT, 0U);
    this->set_register_bit(&this->r_f, this->CARRY_FLAG_BIT, 0U);
    this->set_zero_flag(val);
}
void CPU::op_Swap(reg8 *dest)
{
    dest->set_value(((dest->get_value() & 0x0F) << 4) | ((dest->get_value() & 0xF0) >> 4));
    this->set_register_bit(&this->r_f, this->SUBTRACT_FLAG_BIT, 0U);
    this->set_register_bit(&this->r_f, this->HALF_CARRY_FLAG_BIT, 0U);
    this->set_register_bit(&this->r_f, this->CARRY_FLAG_BIT, 0U);
    this->set_zero_flag(dest->get_value());
}

void CPU::opm_SRL(uint16_t mem_addr)
{
    // Get LSB from value, to set as carry flag
    uint8_t val = this->ram->get_val(mem_addr);
    uint8_t carry_bit = val & (0x01);
    // Shift bits right and force MSB to 0
    val = ((val >> 1) & (uint8_t)0x7F);

    // Push new value back into memory
    this->ram->set(mem_addr, val);

    // Put pushed bit into carry flag
    this->set_register_bit(&this->r_f, this->CARRY_FLAG_BIT, carry_bit);
    // Update zero flag
    this->set_zero_flag(val);
    // Reset subtract and half-carry bits
    this->set_register_bit(&this->r_f, this->HALF_CARRY_FLAG_BIT, 0U);
    this->set_register_bit(&this->r_f, this->SUBTRACT_FLAG_BIT, 0U);
}

void CPU::op_SRL(reg8 *src)
{
    // Get LSB from value, to set as carry flag
    uint8_t carry_bit = src->get_value() & (0x01);

    // Shift bits right and force MSB to 0
    src->set_value(((src->get_value() >> 1) & (uint8_t)0x7F));

    // Put pushed bit into carry flag
    this->set_register_bit(&this->r_f, this->CARRY_FLAG_BIT, carry_bit);
    // Update zero flag
    this->set_zero_flag(src->get_value());
    // Reset subtract and half-carry bits
    this->set_register_bit(&this->r_f, this->HALF_CARRY_FLAG_BIT, 0U);
    this->set_register_bit(&this->r_f, this->SUBTRACT_FLAG_BIT, 0U);
}

void CPU::op_SLA(reg8 *src) {
    src->set_value(this->op_SLA(src->get_value()));
}
void CPU::opm_SLA(uint16_t mem_addr) {
    this->ram->set(mem_addr, this->op_SLA(this->ram->get_val(mem_addr)));
}
uint8_t CPU::op_SLA(uint8_t val) {
    this->set_register_bit(&this->r_f, this->CARRY_FLAG_BIT, ((val & 0x80) >> 7));
    val = (uint8_t)((val << 1) & 0xfe);
    this->set_zero_flag(val);
    this->set_register_bit(&this->r_f, this->HALF_CARRY_FLAG_BIT, 0U);
    this->set_register_bit(&this->r_f, this->SUBTRACT_FLAG_BIT, 0U);
    return val;
}

void CPU::op_SRA(reg8 *src) {
    src->set_value(this->op_SRA(src->get_value()));
}
void CPU::opm_SRA(uint16_t mem_addr) {
    this->ram->set(mem_addr, this->op_SRA(this->ram->get_val(mem_addr)));
}
uint8_t CPU::op_SRA(uint8_t val) {
    this->set_register_bit(&this->r_f, this->CARRY_FLAG_BIT, (val & 0x01));
    val = (uint8_t)(((val >> 1) & 0x7f) | (val & 0x80));
    this->set_zero_flag(val);
    this->set_register_bit(&this->r_f, this->HALF_CARRY_FLAG_BIT, 0U);
    this->set_register_bit(&this->r_f, this->SUBTRACT_FLAG_BIT, 0U);
    return val;
}

void CPU::op_RL(reg8 *src) {
    // Shift old value left 1 bit into a 16-bit register
    this->data_conv.bit16[0] = 0x0000;
    this->data_conv.bit16[0] = ((uint16_t)src->get_value() << 1) | (this->get_carry_flag() & 0x01);
    src->set_value(this->data_conv.bit8[0]);
    this->set_register_bit(&this->r_f, this->CARRY_FLAG_BIT, (this->data_conv.bit8[1] & 0x01));
    // If not RLA, set zero flag
    if (this->cb_state)
        this->set_zero_flag(src->get_value());
    else
        this->set_register_bit(&this->r_f, this->ZERO_FLAG_BIT, 0U);
    this->set_register_bit(&this->r_f, this->SUBTRACT_FLAG_BIT, 0U);
    this->set_register_bit(&this->r_f, this->HALF_CARRY_FLAG_BIT, 0U);
}
void CPU::opm_RL(uint16_t mem_addr) {
    // Shift old value left 1 bit into a 16-bit register
    uint8_t val = this->ram->get_val(mem_addr);
    this->data_conv.bit16[0] = ((uint16_t)val << 1) | (this->get_carry_flag() & 0x01);
    val = this->data_conv.bit8[0];
    this->ram->set(mem_addr, val);
    this->set_register_bit(&this->r_f, this->CARRY_FLAG_BIT, (unsigned int)(this->data_conv.bit8[1] & 0x01));
    this->set_zero_flag(val);
    this->set_register_bit(&this->r_f, this->SUBTRACT_FLAG_BIT, 0U);
    this->set_register_bit(&this->r_f, this->HALF_CARRY_FLAG_BIT, 0U);
}

void CPU::op_RR(reg8 *src) {
    // Capture carry bit from LSB
    uint8_t carry_bit = src->get_value() & (0x01);
    // Shift old value right 1 bit, setting MSB to original carry flag
    src->set_value((((src->get_value() >> 1) & 0x7f) | ((this->get_carry_flag() << 7) & 0x80)));
    this->set_register_bit(&this->r_f, this->CARRY_FLAG_BIT, carry_bit);

    // If not RRA, set zero flag
    if (this->cb_state)
        this->set_zero_flag(src->get_value());
    else
        this->set_register_bit(&this->r_f, this->ZERO_FLAG_BIT, 0U);

    this->set_register_bit(&this->r_f, this->SUBTRACT_FLAG_BIT, 0U);
    this->set_register_bit(&this->r_f, this->HALF_CARRY_FLAG_BIT, 0U);
}
void CPU::opm_RR(uint16_t mem_addr) {
    // Capture carry bit from LSB
    uint8_t val = this->ram->get_val(mem_addr);
    uint8_t carry_bit = val & (0x01);
    // Shift old value right 1 bit, setting MSB to original carry flag
    val = (((val >> 1)  & 0x7f) | ((this->get_carry_flag() << 7) & 0x80));
    this->ram->set(mem_addr, val);
    this->set_register_bit(&this->r_f, this->CARRY_FLAG_BIT, carry_bit);
    this->set_zero_flag(val);
    this->set_register_bit(&this->r_f, this->SUBTRACT_FLAG_BIT, 0U);
    this->set_register_bit(&this->r_f, this->HALF_CARRY_FLAG_BIT, 0U);
}

void CPU::op_RLC(reg8* src)
{
    // Shit to left 1 bit, storing original bit 7 into bit 0
    src->set_value(((((src->get_value() & 0x80) >> 7) & 0x01) | ((src->get_value() << 1) & 0xfe)));
    // Store bit 0 (what was bit 7, into carry flag
    this->set_register_bit(&this->r_f, this->CARRY_FLAG_BIT, (src->get_value() & 0x01));
    // If not RLCA, set zero flag
    if (this->cb_state)
        this->set_zero_flag(src->get_value());
    else
        this->set_register_bit(&this->r_f, this->ZERO_FLAG_BIT, 0U);
    this->set_register_bit(&this->r_f, this->SUBTRACT_FLAG_BIT, 0U);
    this->set_register_bit(&this->r_f, this->HALF_CARRY_FLAG_BIT, 0U);
}
void CPU::opm_RLC(uint16_t mem_addr)
{
    uint8_t val = this->ram->get_val(mem_addr);

    // Shit to left 1 bit, storing original bit 7 into bit 0
    val = (((val & 0x80) >> 7) & 0x01) | ((val << 1) & 0xfe);
    // Store bit 0 (what was bit 7, into carry flag
    this->set_register_bit(&this->r_f, this->CARRY_FLAG_BIT, (val & 0x01));

    // Store value into memory
    this->ram->set(mem_addr, val);
    this->set_zero_flag(val);
    this->set_register_bit(&this->r_f, this->SUBTRACT_FLAG_BIT, 0U);
    this->set_register_bit(&this->r_f, this->HALF_CARRY_FLAG_BIT, 0U);
}

void CPU::op_RRC(reg8* src)
{
    // Store bit 0 in carry flag
    this->set_register_bit(&this->r_f, this->CARRY_FLAG_BIT, (src->get_value() & 0x01));
    // Shit to right 1 bit, storing original bit 0 into bit 7
    src->set_value(((src->get_value() & 0x01) << 7) | ((src->get_value() >> 1) & 0x7f));
    // If not RRCA, set zero flag
    if (this->cb_state)
        this->set_zero_flag(src->get_value());
    else
        this->set_register_bit(&this->r_f, this->ZERO_FLAG_BIT, 0U);
    this->set_register_bit(&this->r_f, this->SUBTRACT_FLAG_BIT, 0U);
    this->set_register_bit(&this->r_f, this->HALF_CARRY_FLAG_BIT, 0U);
}

void CPU::opm_RRC(uint16_t mem_addr)
{
    uint8_t val = this->ram->get_val(mem_addr);

    // Store bit 0 in carry flag
    this->set_register_bit(&this->r_f, this->CARRY_FLAG_BIT, (val & 0x01));

    // Shit to left 1 bit, storing original bit 7 into bit 0
    val = (((val & 0x01) << 7) | ((val >> 1) & 0x7f));

    // Store value into memory
    this->ram->set(mem_addr, val);
    this->set_zero_flag(val);
    this->set_register_bit(&this->r_f, this->SUBTRACT_FLAG_BIT, 0U);
    this->set_register_bit(&this->r_f, this->HALF_CARRY_FLAG_BIT, 0U);
}


////////////////////////// Misc OPs //////////////////////////

void CPU::op_SCF() {
    this->set_register_bit(&this->r_f, this->SUBTRACT_FLAG_BIT, 0U);
    this->set_register_bit(&this->r_f, this->HALF_CARRY_FLAG_BIT, 0U);
    this->set_register_bit(&this->r_f, this->CARRY_FLAG_BIT, 1U);
}

////////////////////////// STACK-related OPs //////////////////////////

void CPU::op_Call() {
    // Get jump address
    uint16_t jmp_dest_addr = this->get_inc_pc_val16();
    if (DEBUG || this->stepped_in)
        std::cout << "Jumping from: " << std::hex << this->r_pc.get_value() << " to " << (int)jmp_dest_addr << std::endl;

    // Push PC (which has already been incremented) to stack
    this->ram->stack_push(this->r_sp.get_pointer(), this->r_pc.get_value());

    // Set PC to jump destination address
    this->r_pc.set_value(jmp_dest_addr);
}

void CPU::op_Return() {
    this->r_pc.set_value(this->ram->stack_pop(this->r_sp.get_pointer()));
}

void CPU::op_Push(reg16 *src) {
    this->op_Push(src->get_value());
}
void CPU::op_Push(combined_reg *src) {
    this->op_Push(src->value());
}
void CPU::op_Push(uint16_t src) {
    this->ram->stack_push(this->r_sp.get_pointer(), src);
}

void CPU::op_Pop(reg16 *dest) {
    dest->set_value(this->op_Pop());
}
void CPU::op_Pop(combined_reg *dest) {
    dest->set_value(this->op_Pop());
}
uint16_t CPU::op_Pop() {
    return this->ram->stack_pop(this->r_sp.get_pointer());
}

// Jump forward N number instructions
void CPU::op_JR() {
    // Default to obtaining value from next byte
    int8_t jp = this->get_inc_pc_val8s();

    if (DEBUG || this->stepped_in)
        std::cout << "Jump from " << std::hex << (unsigned int)this->r_pc.get_value() << " by " << signed(jp);
    // Setting this to the 'current OP' PC appears to break the BIOS,
    // so basically confirmed this functionality.
    this->r_pc.set_value(this->r_pc.get_value() + jp);
    if (DEBUG || this->stepped_in)
        std::cout << " to " << std::hex << this->r_pc.get_value() << std::endl;
}

// Jump to address
void CPU::op_JP() {
    // Default to obtaining value from next byte
    int16_t jump_to = this->get_inc_pc_val16();
    this->op_JP(jump_to);
}
void CPU::op_JP(combined_reg *jmp_reg) {
    this->op_JP(jmp_reg->value());
}
void CPU::op_JP(uint16_t jump_to) {
    if (DEBUG || this->stepped_in)
        std::cout << std::hex << "Jump from " << (int)this->r_pc.get_value() << " to " << (int)jump_to << std::endl;
    this->r_pc.set_value(jump_to);
}

void CPU::op_RST(uint16_t memory_addr) {
    // Get jump address
    if (DEBUG || this->stepped_in)
        std::cout << "Jumping from: " << std::hex << this->r_pc.get_value() << " to " << (int)memory_addr << std::endl;

    // Push PC to of current op to stack
    this->ram->stack_push(this->r_sp.get_pointer(), this->r_pc.get_value());

    // Set PC to jump destination address
    this->r_pc.set_value(memory_addr);
}

////////////////////////// halty-waity-related OPs //////////////////////////

void CPU::op_Halt() {
    // Set DI state to pending, e.g.
    // perform one more command before halting
    this->halt_state = true;
}

void CPU::op_EI() {
    // Set DI state to pending, e.g.
    // perform one more command before halting
    this->interrupt_state = this->INTERRUPT_STATE::PENDING_ENABLE;
}
void CPU::op_DI() {
    // Set DI state to pending, e.g.
    // perform one more command before halting
    this->interrupt_state = this->INTERRUPT_STATE::PENDING_DISABLE;
}
