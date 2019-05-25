
#pragma once

#include "ram.h"
#include "cpu.h"
#include "vpu.h"

class TestCPU {
public:
    //TestCPU(RAM *ram, VPU *vpu_inst);
    TestCPU(CPU *cpu_inst, RAM *ram_inst, VPU *vpu_inst);
    void run_tests();
    void tick();
    CPU* cpu_inst;
    RAM* ram_inst;
    VPU* vpu_inst;
    //TestCPU* next;

};

TestCPU::TestCPU(CPU *cpu_inst, RAM *ram_inst, VPU *vpu_inst) {
    this->cpu_inst = cpu_inst;
    this->ram_inst = ram_inst;
    this->vpu_inst = vpu_inst;
}

void TestCPU::run_tests() {
    std::cout << "Running tests!";
}
void TestCPU::tick() {
    std::cout << "i did a tick";
}