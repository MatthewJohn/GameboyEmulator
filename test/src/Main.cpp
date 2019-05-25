#define GB_UNITTEST 1
#include <iostream>
#include <string>
#include <signal.h>
#include "helper.h"
#include "./ram.h"
#include "./vpu.h"
#include "./cpu.h"
#include "./unittest.h"

#define SCREEN_WIDTH 144
#define SCREEN_HEIGHT 160
#define APP_NAME "GameBoy Emulator"

#define DISABLE_VPU 0
#define DEBUG 0

int main(int argc, char* args[])
{
    Helper::init();
    RAM *ram_inst = new RAM();

    VPU *vpu_inst = new VPU(ram_inst);
    CPU *cpu_inst = new CPU(ram_inst, vpu_inst);
    TestCPU *test_inst = TestCPU(cpu_inst);
    test_inst->run_tests();

    std::cin.get();
    vpu_inst->tear_down();

    return 0;
}