#include <iostream>
#include <string>
#include <signal.h>
#include "helper.h"
#include "ram.h"
#include "./vpu.h"
#include "cpu.h"

#define SCREEN_WIDTH 144
#define SCREEN_HEIGHT 160
#define APP_NAME "GameBoy Emulator"

#define DISABLE_VPU 0
#define DEBUG 0

struct arguments_t {
    char *bios_path;
    char *rom_path;
    bool valid;
};

arguments_t get_arguments(int argc, char* args[]) {
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

//void handle_error() {
//    CPU::print_state();
//};

//class StaticState {
//public:
//    StaticState(CPU cpu_inst_);
//    static CPU get_inst();
//};
//StaticState::StaticState(CPU cpu_inst_) {
//    static CPU StaticState::cpu_inst = cpu_inst_;
//}
//CPU StaticState::get_inst() {
//    return StaticState::cpu_inst;
//}

int main(int argc, char* args[])
{
    //arguments_t arguments = get_arguments(argc, args);
    //if (arguments.valid == false) {
    //    return 1;
    //}
    
    // create the window
//    Display *di = XOpenDisplay(getenv("DISPLAY"));
//    if (di == NULL) {
//		printf("Couldn't open display.\n");
//		return -1;
//	}
    Helper::init();
    RAM *ram_inst = new RAM();
    char bios_path[] = "./copyright/DMG_ROM.bin";
    //char rom_path[] = "./copyright/Tetris (JUE) (V1.1) [!].gb";
    char rom_path[] = "./resources/test_roms/cpu_instrs/cpu_instrs.gb";
    ram_inst->load_bios(bios_path);
    ram_inst->load_rom(rom_path);

    VPU *vpu_inst = new VPU(ram_inst);
    CPU *cpu_inst = new CPU(ram_inst, vpu_inst);
    //CPU cpu_inst = *cpu_inst_ptr;
    //StaticState::cpu_inst = cpu_inst;

    // Setup handler for exception
    //void (*prev_handler)(int);
    //prev_handler =
    //signal(SIGSEGV, CPU::print_state);

    // run the program as long as the window is open
    while (cpu_inst->is_running())
    {
        cpu_inst->tick();
        if (! DISABLE_VPU) {
            vpu_inst->tick();
            //vpu_inst->tick();
            //vpu_inst->tick();
            //vpu_inst->tick();
//            vpu_inst->tick();
//            vpu_inst->tick();
//            vpu_inst->tick();
//            vpu_inst->tick();
//            vpu_inst->tick();
//            vpu_inst->tick();
//            vpu_inst->tick();
//            vpu_inst->tick();
//            vpu_inst->tick();
        }

        //vpu_inst->process_events();
    }

    std::cin.get();
    vpu_inst->tear_down();

    return 0;
}