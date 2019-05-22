#include "cpu.h"

// Used for printing hex
#include "helper.h"
#include  <iomanip>

#include <iostream>
#include <string.h>

#define DEBUG 1
#define INTERUPT_DEBUG 1
#define STEPIN 0xc
#define STOP_ON_BAD_OPCODE 1
#define STOP_BEFORE_ROM 1

CPU::CPU(RAM *ram, VPU *vpu_inst) {
    
    this->r_a = accumulator();
    this->r_a.value = 0;
    this->r_f = gen_reg();
    this->r_f.value = 0;
    this->r_b = gen_reg();
    this->r_b.value = 0;
    this->r_c = gen_reg();
    this->r_c.value = 0;
    this->r_d = gen_reg();
    this->r_d.value = 0;
    this->r_e = gen_reg();
    this->r_e.value = 0;
    this->r_h = gen_reg();
    this->r_h.value = 0;
    this->r_l = gen_reg();
    this->r_l.value = 0;
    
    this->r_bc.upper = &this->r_b;
    this->r_bc.lower = &this->r_c;
    this->r_de.upper = &this->r_d;
    this->r_de.lower = &this->r_e;
    this->r_hl.upper = &this->r_h;
    this->r_hl.lower = &this->r_l;
    this->r_af.upper = &this->r_a;
    this->r_af.lower = &this->r_f;
    
    this->cb_state = false;
    
    this->interupt_state = this->INTERUPT_STATE::DISABLED;
    this->halt_state = false;
    
    // Stack pointer
    this->r_sp = stack_pointer();
    this->r_sp.value = 0xfffe;
    // Program counter
    this->r_pc = program_counter();
    this->r_pc.value = 0;
    
    this->running = true;
    this->stepped_in = false;
    
    this->ram = ram;
    this->vpu_inst = vpu_inst;
}

bool CPU::is_running() {
    return this->running;
}

void CPU::tick() {
    this->temp_counter ++;
    //if (this->temp_counter % 2401 == 0)
    //    std::cout << "1000 Tick: " << std::hex << this->r_pc.value << ", SP: " << this->r_sp.value << std::endl;
        //this->running = false;

    if (DEBUG || this->stepped_in) {
        if (! this->stepped_in) {
            std::cout << std::hex <<
            "a : " << std::setfill('0') << std::setw(2) << (unsigned int)this->r_a.value << std::endl <<
            " f: " << std::setfill('0') << std::setw(2) << (unsigned int)this->r_f.value << std::endl <<
            "af: " << std::setfill('0') << std::setw(4) << this->r_af.value() << std::endl <<
            "b : " << std::setfill('0') << std::setw(2) << (unsigned int)this->r_b.value << std::endl <<
            " c: " << std::setfill('0') << std::setw(2) << (unsigned int)this->r_c.value << std::endl <<
            "bc: " << std::setfill('0') << std::setw(4) << this->r_bc.value() << std::endl <<
            "d : " << std::setfill('0') << std::setw(2) << (unsigned int)this->r_d.value << std::endl <<
            " e: " << std::setfill('0') << std::setw(2) << (unsigned int)this->r_e.value << std::endl <<
            "de: " << std::setfill('0') << std::setw(4) << this->r_de.value() << std::endl <<
            "h : " << std::setfill('0') << std::setw(2) << (unsigned int)this->r_h.value << std::endl <<
            " l: " << std::setfill('0') << std::setw(2) << (unsigned int)this->r_l.value << std::endl <<
            "hl: " << std::setfill('0') << std::setw(4) << this->r_hl.value() << std::endl <<
            "sp: " << std::setfill('0') << std::setw(4) << this->r_sp.value << std::endl <<
            "pc: " << std::setfill('0') << std::setw(4) << this->r_pc.value << std::endl;
        }
        std::cout << std::endl << std::endl << "New Tick: " << std::hex << this->r_pc.value << ", SP: " << this->r_sp.value << std::endl;    
    }
    // If interupt state is either enabled or pending disable,
    // check interupts
    if (this->interupt_state == this->INTERUPT_STATE::ENABLED ||
        this->interupt_state == this->INTERUPT_STATE::PENDING_DISABLE)
        this->check_interupts();
    
    // Check if interupt state is in a pending state
    // and move to actual state, since a clock cycle has been waited
    if (this->interupt_state == this->INTERUPT_STATE::PENDING_DISABLE)
        this->interupt_state = this->INTERUPT_STATE::DISABLED;
    if (this->interupt_state == this->INTERUPT_STATE::PENDING_ENABLE)
        this->interupt_state = this->INTERUPT_STATE::ENABLED;

    if (this->halt_state)
        return;
        
    // Determine stepped-in before PC is incremented
    if ((STEPIN == 1 || (STEPIN + 1) == this->r_pc.value || STEPIN == this->r_pc.value) && STEPIN != 0)
    {
        std::cout << (unsigned int)this->ram->get_val((uint16_t)0x8010) << (unsigned int)this->ram->get_val(0x8011) << (unsigned int)this->ram->get_val(0x8012) << (unsigned int)this->ram->get_val(0x8013) << std::endl;
        this->stepped_in = true;
    }

    // Read value from memory
    int op_val = (int)this->get_inc_pc_val8();
    
    if (DEBUG || this->stepped_in)
        std::cout << "CB: " << (int)this->cb_state << " Op Code: " << std::hex << op_val << std::endl;

    if (this->cb_state) {
        this->execute_cb_code(op_val);
        this->cb_state = false;
    } else {
        this->execute_op_code(op_val);
    }
    
    // Stop runnign when we hit the start of the ROM
    int current_pc = (int)this->r_pc.value;
    if ((current_pc == 0x0100) && STOP_BEFORE_ROM) {
        this->running = false;
        std::cout << "HIT the start of the ROM!" << std::endl;
    }
    
    if (this->stepped_in) {
        std::cout << std::hex <<
            "a : " << std::setfill('0') << std::setw(2) << (unsigned int)this->r_a.value << std::endl <<
            " f: " << std::setfill('0') << std::setw(2) << (unsigned int)this->r_f.value << std::endl <<
            "af: " << std::setfill('0') << std::setw(4) << this->r_af.value() << std::endl <<
            "b : " << std::setfill('0') << std::setw(2) << (unsigned int)this->r_b.value << std::endl <<
            " c: " << std::setfill('0') << std::setw(2) << (unsigned int)this->r_c.value << std::endl <<
            "bc: " << std::setfill('0') << std::setw(4) << this->r_bc.value() << std::endl <<
            "d : " << std::setfill('0') << std::setw(2) << (unsigned int)this->r_d.value << std::endl <<
            " e: " << std::setfill('0') << std::setw(2) << (unsigned int)this->r_e.value << std::endl <<
            "de: " << std::setfill('0') << std::setw(4) << this->r_de.value() << std::endl <<
            "h : " << std::setfill('0') << std::setw(2) << (unsigned int)this->r_h.value << std::endl <<
            " l: " << std::setfill('0') << std::setw(2) << (unsigned int)this->r_l.value << std::endl <<
            "hl: " << std::setfill('0') << std::setw(4) << this->r_hl.value() << std::endl <<
            "sp: " << std::setfill('0') << std::setw(4) << this->r_sp.value << std::endl <<
            "pc: " << std::setfill('0') << std::setw(4) << this->r_pc.value << std::endl;
        std::cin.get();
    }
}

void CPU::check_interupts() {
    // Check VPU V-Blank nterupt
    if ((this->ram->get_val(this->ram->LCDC_STATUS_ADDR) & (uint8_t)0x11) == (uint8_t)0x11)
    {    
        if (INTERUPT_DEBUG || DEBUG || this->stepped_in)
            std::cout << std::hex << "Got v-blank interupt at: " << (unsigned int)this->r_pc.value << std::endl;

        // Store PC on the stack
        this->ram->stack_push(this->r_sp.value, this->r_pc.value);
        // Jump to 0x0040
        this->r_pc.value = (uint8_t)0x0040;
    } else if ((this->ram->get_val(this->ram->LCDC_STATUS_ADDR) & (uint8_t)0x08) == (uint8_t)0x08)
    {
        if (INTERUPT_DEBUG || DEBUG || this->stepped_in)
            std::cout << std::hex << "Got h-blank interupt at: " << (unsigned int)this->r_pc.value << std::endl;

        // Do a straight jump to 0x0048
        this->r_pc.value = (uint8_t)0x0048;
    }
}

void CPU::execute_op_code(int op_val) {
    switch(op_val) {
        case 0x0:
            // STOP
            //this->op_Noop();
            if (DEBUG || this->stepped_in)
                std::cout << "Noop: " << std::hex << this->r_pc.value << std::endl;
            break;
        case 0x01:
            this->op_Load(&this->r_bc);
            break;
        case 0x02:
            this->op_Load(this->r_bc.value(), &this->r_a);
            break;
        case 0x03:
            this->op_Inc(&this->r_bc);
            break;
        case 0x04:
            this->op_Inc(&this->r_b);
            break;
        case 0x05:
            this->op_Dec(&this->r_b);
            break;
        case 0x06:
            // Load byte into C
            this->op_Load(&this->r_b);
            break;
        case 0x08:
            // Load byte into C
            this->op_Load(this->get_inc_pc_val16(), &this->r_sp);
            break;
        case 0x0a:
            this->op_Load(&this->r_a, this->get_register_value16(&this->r_bc));
            break;
        case 0x0b:
            this->op_Dec(&this->r_bc);
            break;
        case 0x0c:
            this->op_Inc(&this->r_c);
            break;
        case 0x0d:
            this->op_Dec(&this->r_c);
            break;
        case 0x0e:
            // Load byte into C
            this->op_Load(&this->r_c);
            break;
        case 0x10:
            //this->op_Stop();
            // @TODO: This is TEMPORARY
            //this->running = false;
            break;
        case 0x11:
            this->op_Load(&this->r_de);
            break;
        case 0x13:
            this->op_Inc(&this->r_de);
            break;
        case 0x15:
            this->op_Dec(&this->r_d);
            break;
        case 0x16:
            // Load byte into C
            this->op_Load(&this->r_d);
            break;
        case 0x17:
            this->op_RL(&this->r_a);
            break;
        case 0x18:
            this->op_JR();
            break;
        case 0x1a:
            this->op_Load(&this->r_a, this->get_register_value16(&this->r_de));
            break;
        case 0x1b:
            this->op_Dec(&this->r_de);
            break;
        case 0x1c:
            this->op_Inc(&this->r_e);
            break;
        case 0x1d:
            this->op_Dec(&this->r_e);
            break;
        case 0x1e:
            this->op_Load(&this->r_e);
            break;
        case 0x20:
            if (! this->get_zero_flag())
                this->op_JR();
            else
                // If we don't perform the OP, pull
                // data from ram to inc PC
                this->get_inc_pc_val8();
            break;
        case 0x21:
            this->op_Load(&this->r_hl);
            break;
        case 0x22:
            // Get HL, dec and set
            this->op_Load_Inc(&this->r_hl, &this->r_a);
            break;
        case 0x23:
            this->op_Inc(&this->r_hl);
            break;
        case 0x24:
            this->op_Inc(&this->r_h);
            break;
        case 0x25:
            this->op_Dec(&this->r_h);
            break;
        case 0x26:
            this->op_Load(&this->r_h);
            break;
        case 0x28:
            if (this->get_zero_flag())
                this->op_JR();
            else
                // If we don't perform the OP, pull
                // data from ram to inc PC
                this->get_inc_pc_val8();
            break;
        case 0x2a:
            this->op_Load_Inc(&this->r_a, &this->r_hl);
            break;
        case 0x2b:
            this->op_Dec(&this->r_hl);
            break;
        case 0x2c:
            this->op_Inc(&this->r_l);
            break;
        case 0x2d:
            this->op_Dec(&this->r_l);
            break;
        case 0x2e:
            this->op_Load(&this->r_l);
            break;
        case 0x31:
            // Load 2-bytes into SP
            this->op_Load(&this->r_sp);
            break;
        case 0x32:
            // Get HL, dec and set
            this->op_Load_Dec(&this->r_hl, &this->r_a);
            break;
        case 0x37:
            this->op_SCF();
            break;
        case 0x39:
            this->op_Add(&this->r_hl, &this->r_sp);
            break;
        case 0x3a:
            // Get HL, dec and set
            this->op_Load_Dec(&this->r_a, &this->r_hl);
            break;
        case 0x3b:
            this->op_Dec(&this->r_sp);
            break;
        case 0x3c:
            this->op_Inc(&this->r_a);
            break;
        case 0x3d:
            this->op_Dec(&this->r_a);
            break;
        case 0x3e:
            this->op_Load(&this->r_a);
            break;
        case 0x47:
            this->op_Load(&this->r_b, &this->r_a);
            break;
        case 0x4f:
            this->op_Load(&this->r_c, &this->r_a);
            break;
        case 0x50:
            this->op_Load(&this->r_d, &this->r_b);
            break;
        case 0x51:
            this->op_Load(&this->r_d, &this->r_c);
            break;
        case 0x52:
            this->op_Load(&this->r_d, &this->r_d);
            break;
        case 0x53:
            this->op_Load(&this->r_d, &this->r_e);
            break;
        case 0x54:
            this->op_Load(&this->r_d, &this->r_h);
            break;
        case 0x55:
            this->op_Load(&this->r_d, &this->r_l);
            break;
        case 0x56:
            this->op_Load(&this->r_d, this->get_register_value16(&this->r_hl));
            break;
        case 0x57:
            this->op_Load(&this->r_d, &this->r_a);
            break;
        case 0x60:
            this->op_Load(&this->r_h, &this->r_b);
            break;
        case 0x61:
            this->op_Load(&this->r_h, &this->r_c);
            break;
        case 0x62:
            this->op_Load(&this->r_h, &this->r_d);
            break;
        case 0x63:
            this->op_Load(&this->r_h, &this->r_e);
            break;
        case 0x64:
            this->op_Load(&this->r_h, &this->r_h);
            break;
        case 0x65:
            this->op_Load(&this->r_h, &this->r_l);
            break;
        case 0x66:
            this->op_Load(&this->r_h, this->get_register_value16(&this->r_hl));
            break;
        case 0x67:
            this->op_Load(&this->r_h, &this->r_a);
            break;
        case 0x70:
            this->op_Load(this->get_register_value16(&this->r_hl), &this->r_b);
            break;
        case 0x71:
            this->op_Load(this->get_register_value16(&this->r_hl), &this->r_c);
            break;
        case 0x72:
            this->op_Load(this->get_register_value16(&this->r_hl), &this->r_d);
            break;
        case 0x73:
            this->op_Load(this->get_register_value16(&this->r_hl), &this->r_e);
            break;
        case 0x76:
            this->op_Halt();
            break;
        case 0x77:
            this->op_Load(this->get_register_value16(&this->r_hl), &this->r_a);
            break;
        case 0x78:
            this->op_Load(&this->r_a, &this->r_b);
            break;
        case 0x79:
            this->op_Load(&this->r_a, &this->r_c);
            break;
        case 0x7a:
            this->op_Load(&this->r_a, &this->r_d);
            break;
        case 0x7b:
            this->op_Load(&this->r_a, &this->r_e);
            break;
        case 0x7c:
            this->op_Load(&this->r_a, &this->r_h);
            break;
        case 0x7d:
            this->op_Load(&this->r_a, &this->r_l);
            break;
        case 0x7e:
            this->op_Load(&this->r_a, this->r_hl.value());
            break;
        case 0x7f:
            this->op_Load(&this->r_a, &this->r_a);
            break;
        case 0x80:
            this->op_Add(&this->r_a, &this->r_b);
            break;
        case 0x81:
            this->op_Add(&this->r_a, &this->r_c);
            break;
        case 0x82:
            this->op_Add(&this->r_a, &this->r_d);
            break;
        case 0x83:
            this->op_Add(&this->r_a, &this->r_e);
            break;
        case 0x84:
            this->op_Add(&this->r_a, &this->r_h);
            break;
        case 0x85:
            this->op_Add(&this->r_a, &this->r_l);
            break;
        case 0x86:
            this->op_Add(&this->r_a, (uint16_t)this->ram->get_val(this->get_register_value16(&this->r_hl)));
            break;
        case 0x87:
            this->op_Add(&this->r_a, &this->r_a);
            break;
        case 0x88:
            this->op_Adc(&this->r_a, &this->r_b);
            break;
        case 0x89:
            this->op_Adc(&this->r_a, &this->r_c);
            break;
        case 0x8a:
            this->op_Adc(&this->r_a, &this->r_d);
            break;
        case 0x8b:
            this->op_Adc(&this->r_a, &this->r_e);
            break;
        case 0x8c:
            this->op_Adc(&this->r_a, &this->r_h);
            break;
        case 0x8d:
            this->op_Adc(&this->r_a, &this->r_l);
            break;
        case 0x8e:
            this->op_Adc(&this->r_a, this->get_register_value16(&this->r_hl));
            break;
        case 0x8f:
            this->op_Adc(&this->r_a, &this->r_a);
            break;
        case 0x90:
            this->op_Sub(&this->r_a, &this->r_b);
            break;
        case 0xa0:
            this->op_AND(&this->r_b);
            break;
        case 0xa1:
            this->op_AND(&this->r_c);
            break;
        case 0xa2:
            this->op_AND(&this->r_d);
            break;
        case 0xa3:
            this->op_AND(&this->r_e);
            break;
        case 0xa4:
            this->op_AND(&this->r_h);
            break;
        case 0xa5:
            this->op_AND(&this->r_l);
            break;
        case 0xa6:
            this->op_AND(this->ram->get_val(this->get_register_value16(&this->r_hl)));
            break;
        case 0xa7:
            this->op_AND(&this->r_a);
            break;
        case 0xaf:
            // X-OR A with A into A
            this->op_XOR(&this->r_a);
            break;
        case 0xb0:
            this->op_OR(&this->r_b);
            break;
        case 0xb1:
            this->op_OR(&this->r_c);
            break;
        case 0xb2:
            this->op_OR(&this->r_d);
            break;
        case 0xb3:
            this->op_OR(&this->r_e);
            break;
        case 0xb4:
            this->op_OR(&this->r_h);
            break;
        case 0xb5:
            this->op_OR(&this->r_l);
            break;
        case 0xb6:
            this->op_OR(this->ram->get_val(this->get_register_value16(&this->r_hl)));
            break;
        case 0xb7:
            this->op_OR(&this->r_a);
            break;
        case 0xb9:
            this->op_CP(&this->r_c);
            break;
        case 0xbe:
            this->op_CP(this->ram->get_val(this->r_hl.value()));
            break;
        case 0xc1:
            this->op_Pop(&this->r_bc);
            break;
        case 0xc2:
            if (! this->get_zero_flag())
                this->op_JP();
            else
                // If we don't perform the OP, pull
                // data from ram to inc PC
                this->get_inc_pc_val16();
            break;
        case 0xc3:
            this->op_JP();
            break;
        case 0xc5:
            this->op_Push(&this->r_bc);
            break;
        case 0xc9:
            this->op_Return();
            break;
        case 0xca:
            if (this->get_zero_flag())
                this->op_JP();
            else
                // If we don't perform the OP, pull
                // data from ram to inc PC
                this->get_inc_pc_val16();
            break;
        case 0xcb:
            // Set flag for CB
            this->cb_state = true;
            break;
        case 0xcc:
            if (this->get_zero_flag())
                this->op_Call();
            else
                // If we don't perform the OP, pull
                // data from ram to inc PC
                this->get_inc_pc_val16();
            break;
        case 0xcd:
            this->op_Call();
            break;
        case 0xce:
            this->op_Adc(&this->r_a);
            break;
        case 0xd1:
            this->op_Pop(&this->r_de);
            break;
        case 0xd2:
            if (! this->get_carry_flag())
                this->op_JP();
            else
                // If we don't perform the OP, pull
                // data from ram to inc PC
                this->get_inc_pc_val16();
            break;
        case 0xd4:
            if (! this->get_carry_flag())
                this->op_Call();
            break;
        case 0xd5:
            this->op_Push(&this->r_de);
            break;
        case 0xda:
            if (this->get_carry_flag())
                this->op_JP();
            else
                // If we don't perform the OP, pull
                // data from ram to inc PC
                this->get_inc_pc_val16();
            break;
        case 0xdf:
            this->op_RST(0x0018);
            break;
        case 0xe0:
            this->op_Load(0xff00 + this->get_inc_pc_val8(), &this->r_a);
            break;
        case 0xe1:
            this->op_Pop(&this->r_hl);
            break;
        case 0xe2:
            this->op_Load(0xff00 + this->r_c.value, &this->r_a);
            break;
        case 0xe5:
            this->op_Push(&this->r_hl);
            break;
        case 0xe6:
            this->op_AND();
            break;
        case 0xea:
            this->op_Load(this->get_inc_pc_val16(), &this->r_a);
            break;
        case 0xf0:
            this->op_Load(&this->r_a, 0xff00 + this->get_inc_pc_val8());
            break;
        case 0xf1:
            this->op_Pop(&this->r_af);
            break;
        case 0xf3:
            // Disable interupts
            this->op_DI();
            break;
        case 0xf5:
            this->op_Push(&this->r_af);
            break;
        case 0xfb:
            // Enable interupts
            this->op_EI();
            break;
        case 0xfe:
            this->op_CP();
            break;

        default:
            std::cout << std::hex << ((int)this->r_pc.value - 1) << "Unknown op code: 0x";
            std::cout << std::setfill('0') << std::setw(2) << std::hex << op_val;
            std::cout << std::endl;
            if (STOP_ON_BAD_OPCODE) {
                this->running = false;
                this->stepped_in = true;
            }
            break;
    }
}

void CPU::execute_cb_code(int op_val) {
    switch(op_val) {
        case 0x11:
            this->op_RL(&this->r_c);
            break;
        case 0x37:
            this->op_Swap(&this->r_a);
            break;
        case 0x7c:
            this->op_Bit(&this->r_h, 7);
            break;
        case 0xcd:
            this->op_Set(1, &this->r_l);
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
}

uint16_t CPU::get_register_value16(combined_reg *dest) {
    union {
        uint8_t bit8[2];
        uint16_t bit16[1];
    } data_conv;
    data_conv.bit8[0] = dest->lower->value;
    data_conv.bit8[1] = dest->upper->value;

    return data_conv.bit16[0];
}

// Perform XOR of registry against A and then store
// result in A
void CPU::op_XOR(reg8 *comp) {
    uint8_t res = this->r_a.value ^ comp->value;
    this->r_a.value = res;
    this->set_register_bit(&this->r_f, this->SUBTRACT_FLAG_BIT, 0U);
    this->set_register_bit(&this->r_f, this->HALF_CARRY_FLAG_BIT, 0U);
    this->set_register_bit(&this->r_f, this->CARRY_FLAG_BIT, 0U);
    this->set_zero_flag(res);
}

// AND operators - And with the A register value, set result to A
void CPU::op_AND() {
    uint8_t comp = this->get_inc_pc_val8();
    this->op_AND(comp);
}
void CPU::op_AND(reg8 *comp) {
    this->op_AND(comp->value);
}
void CPU::op_AND(uint8_t comp) {
    uint8_t res = this->r_a.value & comp;
    this->r_a.value = res;
    this->set_zero_flag(res);
    this->set_register_bit(&this->r_f, this->SUBTRACT_FLAG_BIT, 0U);
    this->set_register_bit(&this->r_f, this->HALF_CARRY_FLAG_BIT, 1U);
    this->set_register_bit(&this->r_f, this->CARRY_FLAG_BIT, 0U);
}

// OR operators - OR with the A register value, set result to A
void CPU::op_OR() {
    uint8_t comp = this->get_inc_pc_val8();
    this->op_OR(comp);
}
void CPU::op_OR(reg8 *comp) {
    this->op_OR(comp->value);
}
void CPU::op_OR(uint8_t comp) {
    uint8_t res = this->r_a.value | comp;
    this->r_a.value = res;
    this->set_zero_flag(res);
    this->set_register_bit(&this->r_f, this->SUBTRACT_FLAG_BIT, 0U);
    this->set_register_bit(&this->r_f, this->HALF_CARRY_FLAG_BIT, 0U);
    this->set_register_bit(&this->r_f, this->CARRY_FLAG_BIT, 0U);
}

// Set single bit of a given register to a given value
void CPU::set_register_bit(reg8 *source, uint8_t bit_shift, unsigned int val) {
    if (val == 1)
        source->value |= (1 << bit_shift);
    else
        source->value &= (0xff ^ (1 << bit_shift));
}

// Obtain the value of a given bit of a given register
unsigned int CPU::get_register_bit(reg8 *source, unsigned int bit_shift) {
    return ((source->value & (1U  << bit_shift)) >> bit_shift);
}

// Set zero flag, based on the value of a given register
void CPU::set_zero_flag(const uint8_t val) {
    this->set_register_bit(&this->r_f, this->ZERO_FLAG_BIT, ((val == (uint8_t)0x0) ? 1U : 0U));
}

uint8_t CPU::get_zero_flag() {
    return this->get_register_bit(&this->r_f, this->ZERO_FLAG_BIT);
}

uint8_t CPU::get_carry_flag() {
    return this->get_register_bit(&this->r_f, this->CARRY_FLAG_BIT);
}

void CPU::set_half_carry(uint8_t original_val, uint8_t input) {
    // @TODO Check this implimentation
    this->set_register_bit(
        &this->r_f,
        this->HALF_CARRY_FLAG_BIT,
        // XOR the original and new values' 5th BIT.
        // This means it will result in half carry if the value has changed.
        //((0x10 & original_val) >> 4) ^ ((0x10 & input) >> 4));
        (((original_val & 0xf) + (input & 0xf)) & 0x10) ? 1U : 0U);
}
void CPU::set_half_carry(uint16_t original_val, uint16_t input) {
    // @TODO Check this implimentation
    this->set_register_bit(
        &this->r_f,
        this->HALF_CARRY_FLAG_BIT,
        // XOR the original and new values' 5th BIT.
        // This means it will result in half carry if the value has changed.
        (((original_val & 0xff) + (input & 0xff)) & 0x100) ? 1U : 0U);
}
void CPU::set_half_carry_sub(uint8_t original_val, uint8_t input) {
    // @TODO Check this implimentation
    this->set_register_bit(
        &this->r_f,
        this->HALF_CARRY_FLAG_BIT,
        // XOR the original and new values' 5th BIT.
        // This means it will result in half carry if the value has changed.
        //((0x10 & original_val) >> 4) ^ ((0x10 & input) >> 4));
        (((original_val & 0xf) - (input & 0xf)) & 0x10) ? 1U : 0U);
}
void CPU::set_half_carry_sub(uint16_t original_val, uint16_t input) {
    // @TODO Check this implimentation
    this->set_register_bit(
        &this->r_f,
        this->HALF_CARRY_FLAG_BIT,
        // XOR the original and new values' 5th BIT.
        // This means it will result in half carry if the value has changed.
        (((original_val & 0xff) - (input & 0xff)) & 0x100) ? 1U : 0U);
}

// Get value from memory at PC and increment PC
uint8_t CPU::get_inc_pc_val8() {
    uint8_t ori_val = this->ram->get_val(this->r_pc.value);
    if (DEBUG || this->stepped_in)
        std::cout << "Got PC value from RAM: " << std::hex << (int)ori_val << " at " << (int)this->r_pc.value << std::endl;
    uint8_t val;
    memcpy(&val, &ori_val, 1);
    this->r_pc.value ++;
    return val;
}

// Get 2-byte value from memory address at PC,
// incrementing the PC past this value
uint16_t CPU::get_inc_pc_val16() {
    union {
        uint8_t bit8[2];
        uint16_t bit16[1];
    } data_conv;
    uint16_t tmp;

    // @TODO: Why is this Not little endian?!
    tmp = this->get_inc_pc_val8();
    memcpy(&data_conv.bit8[0], &tmp, 1);
    tmp = this->get_inc_pc_val8();    
    memcpy(&data_conv.bit8[1], &tmp, 1);
    return data_conv.bit16[0];
}

// Get value from specified register, decrement and store
// in memory (using address of two registers)
void CPU::op_Load_Dec(combined_reg *dest, reg8 *source) {
    this->op_Load(dest->value(), source);
    this->op_Dec(dest);
}
void CPU::op_Load_Dec(reg8 *dest, combined_reg *source) {
    this->op_Load(dest, source->value());
    this->op_Dec(source);
}

// Get value from specified register, increment and store
// in memory (using address of two registers)
void CPU::op_Load_Inc(combined_reg *dest, reg8 *source) {
    this->op_Load(dest->value(), source);
    this->op_Inc(dest);
}
void CPU::op_Load_Inc(reg8 *dest, combined_reg *source) {
    this->op_Load(dest, source->value());
    this->op_Inc(source);
}

void CPU::op_Bit(reg8 *comp, unsigned int bit) {
    // Set flags accordinly before operation
    this->set_zero_flag(this->get_register_bit(comp, bit));
}

// Set single bit in a given register
void CPU::op_Set(uint8_t bit, reg8 *dest) {
    this->set_register_bit(dest, bit, 1);
}

// Add 8bit PC value and carry flag to A.
void CPU::op_Adc(reg8 *dest) {
    uint8_t source = this->get_inc_pc_val8();
    this->op_Adc(dest, source);
}
void CPU::op_Adc(reg8 *dest, reg8 *source) {
    this->op_Adc(dest, source->value);
}
void CPU::op_Adc(reg8 *dest, uint8_t source) {
    union {
        uint8_t bit8[2];
        uint16_t bit16[1];
    } data_conv;

    // Always work with r_a
    uint8_t original_val;
    memcpy(&original_val, &dest->value, 1);
    memcpy(&data_conv.bit8[0], &source, 1);
    data_conv.bit8[1] = 0;
    data_conv.bit16[0] += dest->value;
    dest->value = data_conv.bit8[0];

    // Set carry flag, based on 1st bit of second byte
    uint8_t carry_flag = (0x01 & data_conv.bit8[1]) >> 0;
    this->set_register_bit(&this->r_f, this->CARRY_FLAG_BIT, carry_flag);

    // Set zero flag
    this->set_zero_flag(dest->value);

    // Set subtract flag to 0
    this->set_register_bit(&this->r_f, this->SUBTRACT_FLAG_BIT, 0U);

    // Determine half carry flag based on 5th bit of first byte
    this->set_half_carry(original_val, source);
}


// @TODO: Move these

////////////////////////// Load OPs //////////////////////////
void CPU::op_Load(reg8 *dest) {
    dest->value = this->get_inc_pc_val8();
}
void CPU::op_Load(combined_reg *dest) {
    dest->lower->value = this->get_inc_pc_val8();
    dest->upper->value = this->get_inc_pc_val8();
}
void CPU::op_Load(reg16 *dest) {
    dest->value = this->get_inc_pc_val16();
}
void CPU::op_Load(uint16_t dest_addr, reg16 *source) {
    this->ram->set((int)dest_addr, source->value);
}
// Copy 1 byte between registers
void CPU::op_Load(reg8 *dest, reg8 *source) {
    mempcpy(&dest->value, &source->value, 1);
}
// Copy register value into destination address of memory
void CPU::op_Load(int dest_addr, reg8 *source) {
    this->ram->set(dest_addr, source->value);
}
// Copy data from source memory address to destination
void CPU::op_Load(reg8 *dest, int source_addr) {
    dest->value = this->ram->get_val(source_addr);
}


////////////////////////// General arithmatic OPs //////////////////////////
void CPU::op_Add(reg8 *dest) {
    uint16_t source = this->get_inc_pc_val8();
    this->op_Add(dest, source);
}
void CPU::op_Add(reg8 *dest, reg8 *src) {
    this->op_Add(dest, (uint16_t)src->value);
}
void CPU::op_Add(reg8 *dest, uint16_t src) {
    uint8_t original_val = dest->value;

    this->data_conv.bit8[0] = dest->value;
    this->data_conv.bit8[1] = 0;

    this->data_conv.bit16[0] += src;

    dest->value = this->data_conv.bit8[0];
    
    this->set_zero_flag(dest->value);
    this->set_half_carry(original_val, (uint8_t)src);

    // Set subtract flag to 0, since this is add
    this->set_register_bit(&this->r_f, this->SUBTRACT_FLAG_BIT, 0L);
    this->set_register_bit(
        &this->r_f, this->CARRY_FLAG_BIT,
        (0x01 & this->data_conv.bit8[1]) >> 0);

}

void CPU::op_Add(combined_reg *dest, combined_reg *src) {
    this->op_Add(dest, (uint32_t)src->value());
}
void CPU::op_Add(combined_reg *dest, reg16 *src) {
    this->op_Add(dest, (uint32_t)src->value);
}
void CPU::op_Add(combined_reg *dest, uint32_t src) {
    uint16_t original_val = dest->value();

    this->data_conv32.bit16[0] = dest->value();
    this->data_conv32.bit16[1] = 0;

    this->data_conv32.bit32[0] += src;

    dest->lower->value = this->data_conv32.bit8[0];
    dest->upper->value = this->data_conv32.bit8[1];
    
    this->set_half_carry(original_val, src);

    // Set subtract flag to 0, since this is add
    this->set_register_bit(&this->r_f, this->SUBTRACT_FLAG_BIT, 0L);
    this->set_register_bit(
        &this->r_f, this->CARRY_FLAG_BIT,
        (0x01 & this->data_conv32.bit16[1]) >> 0);

}


void CPU::op_Sub(reg8 *dest) {
    uint16_t source = this->get_inc_pc_val8();
    this->op_Sub(dest, source);
}
void CPU::op_Sub(reg8 *dest, reg8 *src) {
    this->op_Sub(dest, (uint16_t)src->value);
}
void CPU::op_Sub(reg8 *dest, uint16_t src) {
    uint8_t original_val = dest->value;

    this->data_conv.bit8[0] = dest->value;
    this->data_conv.bit8[1] = 0;

    this->data_conv.bit16[0] -= src;

    dest->value = this->data_conv.bit8[0];
    
    this->set_zero_flag(dest->value);
    this->set_half_carry_sub(original_val, (uint8_t)src);

    // Set subtract flag to 0, since this is add
    this->set_register_bit(&this->r_f, this->SUBTRACT_FLAG_BIT, 1L);
    this->set_register_bit(
        &this->r_f, this->CARRY_FLAG_BIT,
        (0x01 & this->data_conv.bit8[1]) >> 0);

}

void CPU::op_Sub(combined_reg *dest, combined_reg *src) {
    this->op_Sub(dest, (uint32_t)src->value());
}
void CPU::op_Sub(combined_reg *dest, reg16 *src) {
    this->op_Sub(dest, (uint32_t)src->value);
}
void CPU::op_Sub(combined_reg *dest, uint32_t src) {
    uint16_t original_val = dest->value();

    this->data_conv32.bit16[0] = dest->value();
    this->data_conv32.bit16[1] = 0;

    this->data_conv32.bit32[0] -= src;

    dest->lower->value = this->data_conv32.bit8[0];
    dest->upper->value = this->data_conv32.bit8[1];
    
    this->set_half_carry_sub(original_val, src);

    // Set subtract flag to 0, since this is add
    this->set_register_bit(&this->r_f, this->SUBTRACT_FLAG_BIT, 1L);
    this->set_register_bit(
        &this->r_f, this->CARRY_FLAG_BIT,
        (0x01 & this->data_conv32.bit16[1]) >> 0);

}


void CPU::op_Inc(reg8 *dest) {
    union {
        uint8_t bit8[2];
        uint16_t bit16[1];
    } data_conv;

    uint8_t original_val = dest->value;
    data_conv.bit8[0] = dest->value;
    data_conv.bit16[0] = (uint16_t)((int)(data_conv.bit16[0]) + 1);
    dest->value = data_conv.bit8[0];

    // Set zero flag
    this->set_zero_flag(dest->value);

    // Set subtract flag to 0
    this->set_register_bit(&this->r_f, this->SUBTRACT_FLAG_BIT, 0U);

    // Determine half carry flag based on 5th bit of first byte
    this->set_half_carry(original_val, 0x1);
}
void CPU::op_Inc(combined_reg *dest) {
    union {
        uint8_t bit8[4];
        uint16_t bit16[2];
        uint32_t bit32[1];
    } data_conv;

    data_conv.bit8[0] = dest->lower->value;
    data_conv.bit8[1] = dest->upper->value;
    data_conv.bit32[0] = (uint32_t)((int)(data_conv.bit32[0]) + 1);
    dest->lower->value = data_conv.bit8[0];
    dest->upper->value = data_conv.bit8[1];
}
void CPU::op_Dec(reg16 *dest) {
    this->data_conv32.bit16[0] = dest->value;
    this->data_conv32.bit16[1] = 0;
    this->data_conv32.bit32[0] = (uint32_t)((int)(this->data_conv32.bit32[0]) - 1);
    dest->value = this->data_conv32.bit16[0];
}
void CPU::op_Dec(combined_reg *dest) {
    this->data_conv32.bit16[0] = dest->value();
    this->data_conv32.bit16[1] = 0;
    this->data_conv32.bit32[0] = (uint32_t)((int)(this->data_conv32.bit32[0]) - 1);
    dest->lower->value = this->data_conv32.bit8[0];
    dest->upper->value = this->data_conv32.bit8[1];
}
void CPU::op_Dec(reg8 *dest) {
    uint8_t original_val = dest->value;
    this->data_conv.bit8[0] = dest->value;
    this->data_conv.bit8[1] = 0;
    this->data_conv.bit16[0] = (uint16_t)((int)(this->data_conv.bit16[0]) - 1);
    dest->value = this->data_conv.bit8[0];

    // Set zero flag
    this->set_zero_flag(dest->value);

    // Set subtract flag to 1
    this->set_register_bit(&this->r_f, this->SUBTRACT_FLAG_BIT, 1U);

    // Determine half carry flag based on 5th bit of first byte
    this->set_half_carry_sub(original_val, 0x1);
}

////////////////////////// bit-related OPs //////////////////////////
void CPU::op_CP() {
    this->op_CP(this->get_inc_pc_val8());
}
void CPU::op_CP(reg8 *in) {
    this->op_CP(in->value);
}
void CPU::op_CP(uint8_t in) {
    unsigned int res = (unsigned int)this->r_a.value - (unsigned int)in;

    // Set zero flag based on the result
    this->set_zero_flag((uint8_t)res);
    // Always sert subtract flag (since that is what we're doing!)
    this->set_register_bit(&this->r_f, this->SUBTRACT_FLAG_BIT, 1U);
    this->set_half_carry(this->r_a.value, in);
    this->set_register_bit(&this->r_f, this->CARRY_FLAG_BIT, ((unsigned int)this->r_a.value < (unsigned int)in) ? 1U : 0U);
}

void CPU::op_Swap(reg8 *dest) {
    dest->value = ((dest->value & 0x0F) << 4) | ((dest->value & 0xF0) >> 4);
    this->set_register_bit(&this->r_f, this->SUBTRACT_FLAG_BIT, 0U);
    this->set_register_bit(&this->r_f, this->HALF_CARRY_FLAG_BIT, 0U);
    this->set_register_bit(&this->r_f, this->CARRY_FLAG_BIT, 0U);
    this->set_zero_flag(dest->value);
}

void CPU::op_RL(reg8 *src) {
    // Shift old value left 1 bit into a 16-bit register
    this->data_conv.bit16[0] = src->value << 1 | this->get_carry_flag();
    src->value = this->data_conv.bit8[1];
    this->set_register_bit(&this->r_f, this->CARRY_FLAG_BIT, (unsigned int)(this->data_conv.bit8[1] & 0x01));
    this->set_zero_flag(src->value);
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
        std::cout << "Jumping from: " << std::hex << this->r_pc.value << " to " << (int)jmp_dest_addr << std::endl;

    // Push PC (which has already been incremented) to stack
    this->ram->stack_push(this->r_sp.value, this->r_pc.value);
    
    // Set PC to jump destination address
    this->r_pc.value = jmp_dest_addr;
}

void CPU::op_Return() {
    this->r_pc.value = this->ram->stack_pop(this->r_sp.value);
}

void CPU::op_Push(reg16 *src) {
    this->op_Push(src->value);
}
void CPU::op_Push(combined_reg *src) {
    this->op_Push(src->value());
}
void CPU::op_Push(uint16_t src) {
    this->ram->stack_push(this->r_sp.value, src);
}

void CPU::op_Pop(reg16 *dest) {
    dest->value = this->op_Pop();
}
void CPU::op_Pop(combined_reg *dest) {
    dest->set_value(this->op_Pop());
}
uint16_t CPU::op_Pop() {
    return this->ram->stack_pop(this->r_sp.value);
}

// Jump forward N number instructions
void CPU::op_JR() {
    // Default to obtaining value from next byte
    union {
        uint8_t uint8[2];
        int8_t int8[2];
        uint16_t uint16[1];
        int16_t int16[1];
    } data;
    data.int8[0] = (int8_t)this->get_inc_pc_val8();
    data.uint16[0] += (uint16_t)this->r_pc.value;
    int jump_by = (int)data.uint8[0] - (int)this->r_pc.value;
    if (DEBUG || this->stepped_in)
        std::cout << "Jump from " << std::hex << (int)this->r_pc.value << " by " << (int)jump_by;
    // @TODO Verify that this jumps by the value AFTER the pc increment
    // for this instruction.
    this->r_pc.value = data.uint8[0];
    //this->r_pc.value --;
    if (DEBUG || this->stepped_in)
        std::cout << " to " << std::hex << (int)this->r_pc.value << std::endl;
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
        std::cout << std::hex << "Jump from " << (int)this->r_pc.value << " to " << (int)jump_to << std::endl;
    // @TODO Verify that this jumps by the value AFTER the pc increment
    // for this instruction.
    this->r_pc.value = jump_to;
}

void CPU::op_RST(uint16_t memory_addr) {
    // Get jump address
    if (DEBUG || this->stepped_in)
        std::cout << "Jumping from: " << std::hex << this->r_pc.value << " to " << (int)memory_addr << std::endl;

    // Push PC (which has already been incremented) to stack
    this->ram->stack_push(this->r_sp.value, this->r_pc.value);
    
    // Set PC to jump destination address
    this->r_pc.value = memory_addr;
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
    this->interupt_state = this->INTERUPT_STATE::PENDING_ENABLE;
}
void CPU::op_DI() {
    // Set DI state to pending, e.g.
    // perform one more command before halting
    this->interupt_state = this->INTERUPT_STATE::PENDING_DISABLE;
}