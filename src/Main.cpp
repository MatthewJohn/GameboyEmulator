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


void handle_terminal_entry(CPU* cpu_inst, VPU* vpu_inst, RAM* ram_inst)
{
    char cmd_in[50];
    while (strcmp(cmd_in, "exit"))
    {
        std::cout << "Enter command: ";
        std::cin.getline(cmd_in, 49);
        std::cout << std::endl;

        char first_char[2] = {cmd_in[0], '\0'};
        // Check CPU command
        if (strcmp(first_char, "c") == 0)
            cpu_inst->execute_terminal_command(cmd_in);
        else
            std::cout << "Unrecognised target: " << cmd_in[0] << std::endl;

    }
    
}

int main(int argc, char *args[])
{
    arguments_t arguments = {0, 0, 0, 0};


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
                strncpy(arguments.rom_path, optarg, sizeof(arguments.rom_path) - 1);
                continue;
            case 'b':
                strncpy(arguments.bios_path, optarg, sizeof(arguments.bios_path) - 1);
                continue;
            case 's':
                strncpy(arguments.screenshot_path, optarg, sizeof(arguments.screenshot_path) - 1);
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
    if (strlen(arguments.bios_path) == 0)
    {
        //char bios_path[] = "./matt-test-daa.rom";
        char bios_path[] = "./copyright/DMG_ROM.bin";
        strncpy(arguments.bios_path, bios_path, sizeof(arguments.bios_path) - 1);
    }
    if (strlen(arguments.rom_path) == 0)
    {
        //char rom_path[] = "./resources/test_roms/cpu_instrs/individual/03-op sp,hl.gb";
        //char rom_path[] = "./debug/testrom.gb";
        //char rom_path[] = "./copyright/dmg_test_prog_ver1.gb";
        //char rom_path[] = "./copyright/Tetris (JUE) (V1.1) [!].gb";
        //char rom_path[] = "./resources/test_roms/cpu_instrs/cpu_instrs.gb";
        char rom_path[] = "./resources/test_roms/cpu_instrs/individual/02-interrupts.gb";
        strncpy(arguments.rom_path, rom_path, sizeof(arguments.rom_path) - 1);
    }
    
    // Tests
    // 01-special.gb - passed!
    // 02-interrupts.gb - fails
    // 03-op sp,hl.gb - passed
    // 04-op r,imm.gb - passed
    // 05-op rp.gb - passed
    // 06-ld r,r.gb - passed
    // 07-jr,jp,call,ret,rst.gb - passed!
    // 08-misc instrs.gb - passed!
    // 09-op r,r.gb - passed
    // 10-bit ops.gb - passed
    // 11-op a,(hl).gb - passed

    ram_inst->load_bios(&arguments);
    ram_inst->load_rom(&arguments);

    // run the program as long as the window is open
    while (cpu_inst->is_running())
    {
        cpu_inst->tick();

#if ! DISABLE_VPU
        switch (vpu_inst->tick())
        {
            case VpuEventType::EXIT:
                cpu_inst->stop();
                break;
            case VpuEventType::TERMINAL:
                handle_terminal_entry(cpu_inst, vpu_inst, ram_inst);
                break;
            default:
                break;
        }
#endif

        // Check for screenshot
        if (arguments.screenshot_ticks && arguments.screenshot_ticks == cpu_inst->get_tick_counter())
        {
            std::cout << "Capturing Screenshot" << std::endl;
            vpu_inst->capture_screenshot(arguments.screenshot_path);
            cpu_inst->stop();
        }

    }

    vpu_inst->tear_down();

    return 0;
}
