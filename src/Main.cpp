// Copyright (C) Dock Studios Ltd, Inc - All Rights Reserved
// Unauthorized copying of this file, via any medium is strictly prohibited
// Proprietary and confidential
// Written by Matt Comben <matthew@dockstudios.co.uk>, May 2019

#include <iostream>
#include <string>
#include <signal.h>
#include "./helper.h"
#include "./ram.h"
#include "./vpu.h"
#include "./cpu.h"
#include "./test_runner.h"

#define APP_NAME "GameBoy Emulator"

#define RUN_TESTS 0
#define DISABLE_VPU 0

struct arguments_t
{
    char *bios_path;
    char *rom_path;
    bool valid;
};

arguments_t get_arguments(int argc, char* args[])
{
    arguments_t arguments;
    if (argc != 3) {
        std::cout << "Usage: GameboyEmulator <BIOS path> <ROM path>" << std::endl;
        arguments.valid = false;
    } else {
        arguments.valid = true;
        arguments.bios_path = args[1];
        arguments.rom_path = args[2];
        std::cout << "Using BIOS path: " << arguments.bios_path << std::endl;
        std::cout << "Using ROM path: " << arguments.rom_path << std::endl;
    }
    return arguments;
}


int main(int argc, char* args[])
{
    Helper::init();
    RAM *ram_inst = new RAM();
    VPU *vpu_inst = new VPU(ram_inst);
    CPU *cpu_inst = new CPU(ram_inst, vpu_inst);
    //CPU cpu_inst = *cpu_inst_ptr;
    //StaticState::cpu_inst = cpu_inst;

#if RUN_TESTS
    // Run tests
    TestRunner *rt = new TestRunner(vpu_inst, cpu_inst, ram_inst);
    rt->run_tests();
#endif
    cpu_inst->reset_state();

    // Load bios/RAM
    char bios_path[] = "./copyright/DMG_ROM.bin";
    //char bios_path[] = "./matt-test-daa.rom";
    //char rom_path[] = "./copyright/Tetris (JUE) (V1.1) [!].gb";
    //char rom_path[] = "./resources/test_roms/cpu_instrs/cpu_instrs.gb";
    
    // Tests
    // 01-special.gb
    // 02-interrupts.gb
    // 03-op sp,hl.gb
    // 04-op r,imm.gb - fail (de)
    // 05-op rp.gb -passed
    // 06-ld r,r.gb - pased
    // 07-jr,jp,call,ret,rst.gb
    // 08-misc instrs.gb
    // 09-op r,r.gb
    // 10-bit ops.gb -- passed
    // 11-op a,(hl).gb - (9e 27 failed)

    char rom_path[] = "./resources/test_roms/cpu_instrs/individual/04-op r,imm.gb";
    //char rom_path[] = "./debug/testrom.gb";
    //char rom_path[] = "./copyright/dmg_test_prog_ver1.gb";
    ram_inst->load_bios(bios_path);
    ram_inst->load_rom(rom_path);

#if ! DISABLE_VPU
    bool to_vpu_tick = true;
#endif

    // run the program as long as the window is open
    while (cpu_inst->is_running())
    {
        cpu_inst->tick();

#if ! DISABLE_VPU
        if (to_vpu_tick)
        {
            vpu_inst->tick();
        }
        to_vpu_tick = ! to_vpu_tick;
#endif

    }

    std::cin.get();
    vpu_inst->tear_down();

    return 0;
}
