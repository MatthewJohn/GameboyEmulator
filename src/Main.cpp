// Copyright (C) Dock Studios Ltd, Inc - All Rights Reserved
// Unauthorized copying of this file, via any medium is strictly prohibited
// Proprietary and confidential
// Written by Matt Comben <matthew@dockstudios.co.uk>, May 2019

#include <iostream>
#include <string>
#include <signal.h>
#include <getopt.h>

#include "./helper.h"
#include "./ram.h"
#include "./vpu.h"
#include "./cpu.h"
#include "./test_runner.h"

#define APP_NAME "GameBoy Emulator"

#define RUN_TESTS 0
#define DISABLE_VPU 0

#define PATH_SIZE 4096


int main(int argc, char *args[])
{
    arguments_t arguments = {nullptr, nullptr, nullptr, 0};


    for(;;)
    {
        // Arguments
        // -h - help/usage
        // -f - rom file path
        // -b - bios file path
        // -s - screenshot filepath
        // -t - screenshot after X CPU ticks
        switch(getopt(argc, args, "hf:b:s:t:"))
        {
            case 'f':
                arguments.rom_path = (char*)malloc(sizeof(optarg));
                strcpy(arguments.rom_path, optarg);
                continue;
            case 'b':
                arguments.bios_path = (char*)malloc(sizeof(optarg));
                strcpy(arguments.bios_path, optarg);
                continue;
            case 's':
                arguments.screenshot_path = (char*)malloc(sizeof(optarg));
                strcpy(arguments.screenshot_path, optarg);
                continue;
            case 't':
                arguments.screenshot_ticks = atoi(optarg);
                continue;

            case '?':
            case 'h':
            default :
                std::cout << "Usage: ./GameboyEmulator -b <BIOS path> -f <ROM path> [-s <Screenshot filepath> -t <Screenshot After X CPU ticks>]" << std::endl;
                exit(1);
                break;

            case -1:
                break;
        }

        break;
    }

    
    Helper::init();
    RAM *ram_inst = new RAM();
    VPU *vpu_inst = new VPU(ram_inst);
    CPU *cpu_inst = new CPU(ram_inst, vpu_inst);

#if RUN_TESTS
    // Run tests
    TestRunner *rt = new TestRunner(vpu_inst, cpu_inst, ram_inst);
    rt->run_tests();
#endif
    cpu_inst->reset_state();

    // Load bios/RAM
    if (! arguments.bios_path)
    {
        char bios_path[] = "./copyright/DMG_ROM.bin";
        arguments.bios_path = (char*)malloc(sizeof(bios_path));
        strcpy(arguments.bios_path, bios_path);
    }
    if (! arguments.rom_path)
    {
        char rom_path[] = "./resources/test_roms/cpu_instrs/individual/08-misc instrs.gb";
        arguments.rom_path = (char*)malloc(sizeof(rom_path));
        strcpy(arguments.rom_path, rom_path);
    }

    //char bios_path[] = "./matt-test-daa.rom";
    //char rom_path[] = "./copyright/Tetris (JUE) (V1.1) [!].gb";
    //char rom_path[] = "./resources/test_roms/cpu_instrs/cpu_instrs.gb";
    
    // Tests
    // 01-special.gb
    // 02-interrupts.gb
    // 03-op sp,hl.gb - hangs
    // 04-op r,imm.gb - passed
    // 05-op rp.gb - passed
    // 06-ld r,r.gb - passed
    // 07-jr,jp,call,ret,rst.gb - passed!
    // 08-misc instrs.gb - passed!
    // 09-op r,r.gb - passed
    // 10-bit ops.gb - passed
    // 11-op a,(hl).gb - passed

    //arguments.rom_path = "./resources/test_roms/cpu_instrs/individual/08-misc instrs.gb";
    //char rom_path[] = "./debug/testrom.gb";
    //char rom_path[] = "./copyright/dmg_test_prog_ver1.gb";
    ram_inst->load_bios(&arguments);
    ram_inst->load_rom(&arguments);

    // run the program as long as the window is open
    while (cpu_inst->is_running())
    {
        cpu_inst->tick();

#if ! DISABLE_VPU
        vpu_inst->tick();
#endif

        // Check for screenshot
        if (arguments.screenshot_ticks && arguments.screenshot_ticks == cpu_inst->get_tick_counter())
        {
            vpu_inst->capture_screenshot(arguments.screenshot_path);
        }

    }

    std::cin.get();
    vpu_inst->tear_down();

    return 0;
}
