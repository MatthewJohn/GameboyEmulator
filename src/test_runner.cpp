// Copyright (C) Dock Studios Ltd, Inc - All Rights Reserved
// Unauthorized copying of this file, via any medium is strictly prohibited
// Proprietary and confidential
// Written by Matt Comben <matthew@dockstudios.co.uk>, May 2019

#include "./test_runner.h"
#include <iostream>

TestRunner::TestRunner(VPU *vpu_inst, CPU *cpu_inst, RAM *ram_inst)
{
    this->cpu_inst = cpu_inst;
    this->vpu_inst = vpu_inst;
    this->ram_inst = ram_inst;
}


void TestRunner::run_tests()
{
    std::cout << "Starting tests" << std::endl;
    this->test_00();
    this->test_01();
    this->test_02();
    this->test_03();
    this->test_04();
    this->test_07();
    this->test_0f();
    
    this->test_18();
    
    this->test_20();
    this->test_28();
    this->test_2f();

    this->test_30();
    this->test_38();
    
    this->test_80();
    this->test_81();
    this->test_82();
    this->test_83();
    this->test_84();
    this->test_85();
    
    this->test_90();
    this->test_91();
    this->test_92();
    this->test_93();
    this->test_94();
    this->test_95();
    
    this->test_c0();
    this->test_c4();
    this->test_c8();
    this->test_cc();
    this->test_cd();
    
    this->test_d0();
    this->test_d4();
    this->test_d8();
    this->test_dc();

    this->test_cb_00();
    this->test_cb_01();
    this->test_cb_02();
    this->test_cb_03();
    this->test_cb_04();
    this->test_cb_05();

    this->test_cb_10();
    this->test_cb_11();
    this->test_cb_12();
    this->test_cb_13();
    this->test_cb_14();
    this->test_cb_15();

    this->test_cb_18();
    this->test_cb_19();
    this->test_cb_1a();
    this->test_cb_1b();
    this->test_cb_1c();
    this->test_cb_1d();

    this->test_cb_30();
    this->test_cb_31();
    this->test_cb_32();
    this->test_cb_33();
    this->test_cb_34();
    this->test_cb_35();

    std::cout << std::endl << "Completed tests" << std::endl;

}

void TestRunner::test_00()
{
    std::cout << "0x000";
    // Ensure that SP is 0
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x0000);

    this->ram_inst->memory[0x0000] = 0x00;
    this->cpu_inst->tick();

    // Ensure that SP has moved on
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x0001);
}

void TestRunner::test_01()
{
    std::cout << "0x001";

    this->cpu_inst->r_pc.set_value(0x0000);
    this->cpu_inst->r_f.set_value(0xff);

    // Setup b and c
    this->cpu_inst->r_b.set_value(0xff);
    this->cpu_inst->r_c.set_value(0xff);

    // Store load BC, d16 in memory and store 16 bit value
    this->ram_inst->memory[0x0000] = 0x01;
    // Store value in reverse byte
    this->ram_inst->memory[0x0001] = 0xcd;
    this->ram_inst->memory[0x0002] = 0xab;

    this->cpu_inst->tick();
    this->assert_equal(this->cpu_inst->r_b.get_value(), 0xab);
    this->assert_equal(this->cpu_inst->r_c.get_value(), 0xcd);
    this->assert_equal(this->cpu_inst->r_bc.value(), 0xabcd);

    // Ensure flags haven't changed
    this->assert_equal(this->cpu_inst->r_f.get_value(), 0xff);

    // Ensure that SP has moved on
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x0003);
}

void TestRunner::test_02()
{
    std::cout << "0x002";
    this->cpu_inst->r_pc.set_value(0x0000);
    this->cpu_inst->r_f.set_value(0xff);

    // Setup b and c, as destination memory address
    this->cpu_inst->r_a.set_value(0xd6);
    this->cpu_inst->r_b.set_value(0xfb);
    this->cpu_inst->r_c.set_value(0x23);
    this->ram_inst->memory[0xfb23] = 0x00;

    // Setup memory
    this->ram_inst->memory[0x0000] = 0x02;

    this->cpu_inst->tick();

    // Assert that registers were unchanged
    this->assert_equal(this->cpu_inst->r_a.get_value(), 0xd6);
    this->assert_equal(this->cpu_inst->r_b.get_value(), 0xfb);
    this->assert_equal(this->cpu_inst->r_c.get_value(), 0x23);
    this->assert_equal(this->cpu_inst->r_bc.value(), 0xfb23);

    // Ensure that byte has been put into memory
    this->assert_equal(this->ram_inst->memory[0xfb23], 0xd6);

    // Ensure flags haven't changed
    this->assert_equal(this->cpu_inst->r_f.get_value(), 0xff);

    // Ensure that SP has moved on
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x0001);
}

void TestRunner::test_03()
{
    std::cout << "0x003";

    // Test standard increment
    this->cpu_inst->r_pc.set_value(0x0000);
    this->cpu_inst->r_f.set_value(0xff);
    this->cpu_inst->r_b.set_value(0xaf);
    this->cpu_inst->r_c.set_value(0xfe);
    this->ram_inst->memory[0x0000] = 0x03;
    this->cpu_inst->tick();

    // Assert that registers were unchanged
    this->assert_equal(this->cpu_inst->r_b.get_value(), 0xaf);
    this->assert_equal(this->cpu_inst->r_c.get_value(), 0xff);
    this->assert_equal(this->cpu_inst->r_bc.value(), 0xafff);

    // Ensure flags haven't changed
    this->assert_equal(this->cpu_inst->r_f.get_value(), 0xff);

    // Ensure that SP has moved on
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x0001);

    // Test half-carry

    // Setup memory
    this->cpu_inst->r_f.set_value(0x00);
    this->ram_inst->memory[0x0001] = 0x03;
    this->cpu_inst->tick();

    // Assert that registers were unchanged
    this->assert_equal(this->cpu_inst->r_b.get_value(), 0xb0);
    this->assert_equal(this->cpu_inst->r_c.get_value(), 0x00);
    this->assert_equal(this->cpu_inst->r_bc.value(), 0xb000);

    // Ensure flags haven't changed
    this->assert_equal(this->cpu_inst->r_f.get_value(), 0x00);


    // Test Carry
    // Setup memory and run instruction
    this->cpu_inst->r_f.set_value(0x00);
    this->cpu_inst->r_bc.set_value(0xffff);
    this->ram_inst->memory[0x0002] = 0x03;
    this->cpu_inst->tick();

    // Assert that registers were unchanged
    this->assert_equal(this->cpu_inst->r_b.get_value(), 0x00);
    this->assert_equal(this->cpu_inst->r_c.get_value(), 0x00);
    this->assert_equal(this->cpu_inst->r_bc.value(), 0x0000);

    // Ensure flags haven't changed
    this->assert_equal(this->cpu_inst->r_f.get_value(), 0x00);
}

void TestRunner::test_04()
{
    std::cout << "0x004";
    // Test standard increment
    this->cpu_inst->r_pc.set_value(0x0000);
    this->cpu_inst->r_f.set_value(0xf0);
    this->cpu_inst->r_b.set_value(0x58);
    this->ram_inst->memory[0x0000] = 0x04;
    this->cpu_inst->tick();

    // Assert that registers were unchanged
    this->assert_equal(this->cpu_inst->r_b.get_value(), 0x59);

    // Ensure zero, subtract and half-carry flags have been unset
    // Carry has been left
    this->assert_equal(this->cpu_inst->r_f.get_value(), 0x10);

    // Ensure that SP has moved on
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x0001);

    // Test half-carry

    // Setup memory
    this->cpu_inst->r_f.set_value(0x00);
    this->cpu_inst->r_b.set_value(0x5f);
    this->ram_inst->memory[0x0001] = 0x04;
    this->cpu_inst->tick();

    // Assert that registers were unchanged
    this->assert_equal(this->cpu_inst->r_b.get_value(), 0x60);

    // Ensure half carry has been set
    this->assert_equal(this->cpu_inst->r_f.get_value(), 0x20);


    // Test Carry
    // Setup memory and run instruction
    this->cpu_inst->r_f.set_value(0x00);
    this->cpu_inst->r_b.set_value(0xff);
    this->ram_inst->memory[0x0002] = 0x04;
    this->cpu_inst->tick();

    // Assert that registers were unchanged
    this->assert_equal(this->cpu_inst->r_b.get_value(), 0x00);

    // Ensure flags haven't changed
    this->assert_equal(this->cpu_inst->r_f.get_value(), 0xa0);
}

void TestRunner::test_07()
{
    std::cout << "0x007";
    // Set A to 0110 1011
    this->cpu_inst->r_a.set_value(0x6b);
    
    // Set all flgs
    this->cpu_inst->r_f.set_value(0xf0);
    this->cpu_inst->r_pc.set_value(0x0);
    this->ram_inst->memory[0x0000] = 0x07;
    this->cpu_inst->tick();
    
    // Ensure that A has been rotated left
    // i.e. 1101 0110
    this->assert_equal(this->cpu_inst->r_a.get_value(), 0xd6);
    // Ensure that carry flag is set to 0, as well as all other flags
    this->assert_equal(this->cpu_inst->r_f.get_value(), 0x00);
    
    // Set A to 1000 0011
    this->cpu_inst->r_a.set_value(0x83);
    
    // Set reset flgs
    this->cpu_inst->r_f.set_value(0x00);
    this->cpu_inst->r_pc.set_value(0x00);
    this->ram_inst->memory[0x0000] = 0x07;
    this->cpu_inst->tick();
    
    // Ensure that A has been rotated right
    // i.e. 0000 0111
    this->assert_equal(this->cpu_inst->r_a.get_value(), 0x07);
    // Ensure that carry flag is set to 0, as well as all other flags
    this->assert_equal(this->cpu_inst->r_f.get_value(), 0x10);
}

void TestRunner::test_0f()
{
    std::cout << "0x00f";
    // Set A to 1011 0110
    this->cpu_inst->r_a.set_value(0xb6);
    
    // Set all flgs
    this->cpu_inst->r_f.set_value(0xf0);
    this->cpu_inst->r_pc.set_value(0x0);
    this->ram_inst->memory[0x0000] = 0x0f;
    this->cpu_inst->tick();
    
    // Ensure that A has been rotated right
    // i.e. 0101 1011
    this->assert_equal(this->cpu_inst->r_a.get_value(), 0x5b);
    // Ensure that carry flag is set to 0, as well as all other flags
    this->assert_equal(this->cpu_inst->r_f.get_value(), 0x00);
    
    // Set A to 1000 0011
    this->cpu_inst->r_a.set_value(0x83);
    
    // Set reset flgs
    this->cpu_inst->r_f.set_value(0x00);
    this->cpu_inst->r_pc.set_value(0x00);
    this->ram_inst->memory[0x0000] = 0x0f;
    this->cpu_inst->tick();
    
    // Ensure that A has been rotated right
    // i.e. 1100 0001
    this->assert_equal(this->cpu_inst->r_a.get_value(), 0xc1);
    // Ensure that carry flag is set to 0, as well as all other flags
    this->assert_equal(this->cpu_inst->r_f.get_value(), 0x10);
}


void TestRunner::test_18()
{
    std::cout << "0x018";

    // Test with flags as all set
    this->cpu_inst->r_f.set_value(0xf0);

    // Check jump of 0
    this->cpu_inst->r_pc.set_value(0);
    this->ram_inst->memory[0x0000] = 0x18;
    this->ram_inst->memory[0x0001] = 0x00;
    this->cpu_inst->tick();
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x02);
    
    // Check jump of 1
    this->cpu_inst->r_pc.set_value(0);
    this->ram_inst->memory[0x0000] = 0x18;
    this->ram_inst->memory[0x0001] = 0x01;
    this->cpu_inst->tick();
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x03);
    
    // Test with flags as all reset
    this->cpu_inst->r_f.set_value(0x00);
    
    // Check jump of 2
    this->cpu_inst->r_pc.set_value(0);
    this->ram_inst->memory[0x0000] = 0x18;
    this->ram_inst->memory[0x0001] = 0x02;
    this->cpu_inst->tick();
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x04);
    
    // Test jump of 127
    this->cpu_inst->r_pc.set_value(0);
    this->ram_inst->memory[0x0000] = 0x18;
    this->ram_inst->memory[0x0001] = 0x7f;
    this->cpu_inst->tick();
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x81);
    
    // Test jump of -1
    this->cpu_inst->r_pc.set_value(0x100);
    this->ram_inst->memory[0x0100] = 0x18;
    this->ram_inst->memory[0x0101] = 0xff;
    this->cpu_inst->tick();
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x101);
    
    // Test with flags as all set
    this->cpu_inst->r_f.set_value(0xf0);
    
    // Test jump of -2
    this->cpu_inst->r_pc.set_value(0x100);
    this->ram_inst->memory[0x0100] = 0x18;
    this->ram_inst->memory[0x0101] = 0xfe;
    this->cpu_inst->tick();
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x100);

    // Test jump of -128
    this->cpu_inst->r_pc.set_value(0x100);
    this->ram_inst->memory[0x0100] = 0x18;
    this->ram_inst->memory[0x0101] = 0x80;
    this->cpu_inst->tick();
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x82);
}

void TestRunner::test_20()
{
    std::cout << "0x020";

    // Check jump of 0
    // Test with zero set
    this->cpu_inst->r_f.set_value(0x80);
    this->cpu_inst->r_pc.set_value(0);
    this->ram_inst->memory[0x0000] = 0x20;
    this->ram_inst->memory[0x0001] = 0x00;
    this->cpu_inst->tick();
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x02);
    
    // Check jump of 2
    this->cpu_inst->r_pc.set_value(0);
    this->ram_inst->memory[0x0000] = 0x20;
    this->ram_inst->memory[0x0001] = 0x02;
    this->cpu_inst->tick();
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x02);
    
    // Test jump of -1
    this->cpu_inst->r_pc.set_value(0x100);
    this->ram_inst->memory[0x0100] = 0x20;
    this->ram_inst->memory[0x0101] = 0xff;
    this->cpu_inst->tick();
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x0102);
    
    // Check jump of 0
    // Test with zero reset
    this->cpu_inst->r_f.set_value(0x00);
    this->cpu_inst->r_pc.set_value(0);
    this->ram_inst->memory[0x0000] = 0x20;
    this->ram_inst->memory[0x0001] = 0x00;
    this->cpu_inst->tick();
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x02);
    
    // Check jump of 2
    this->cpu_inst->r_pc.set_value(0);
    this->ram_inst->memory[0x0000] = 0x20;
    this->ram_inst->memory[0x0001] = 0x02;
    this->cpu_inst->tick();
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x04);
    
    // Test jump of -1
    this->cpu_inst->r_pc.set_value(0x100);
    this->ram_inst->memory[0x0100] = 0x20;
    this->ram_inst->memory[0x0101] = 0xff;
    this->cpu_inst->tick();
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x101);
}

void TestRunner::test_28()
{
    std::cout << "0x028";

    // Check jump of 0
    // Test with zero reset
    this->cpu_inst->r_f.set_value(0x00);
    this->cpu_inst->r_pc.set_value(0);
    this->ram_inst->memory[0x0000] = 0x28;
    this->ram_inst->memory[0x0001] = 0x00;
    this->cpu_inst->tick();
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x02);
    
    // Check jump of 2
    this->cpu_inst->r_pc.set_value(0);
    this->ram_inst->memory[0x0000] = 0x28;
    this->ram_inst->memory[0x0001] = 0x02;
    this->cpu_inst->tick();
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x02);
    
    // Test jump of -1
    this->cpu_inst->r_pc.set_value(0x100);
    this->ram_inst->memory[0x0100] = 0x28;
    this->ram_inst->memory[0x0101] = 0xff;
    this->cpu_inst->tick();
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x0102);
    
    // Check jump of 0
    // Test with zero set
    this->cpu_inst->r_f.set_value(0x80);
    this->cpu_inst->r_pc.set_value(0);
    this->ram_inst->memory[0x0000] = 0x28;
    this->ram_inst->memory[0x0001] = 0x00;
    this->cpu_inst->tick();
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x02);
    
    // Check jump of 2
    this->cpu_inst->r_pc.set_value(0);
    this->ram_inst->memory[0x0000] = 0x28;
    this->ram_inst->memory[0x0001] = 0x02;
    this->cpu_inst->tick();
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x04);
    
    // Test jump of -1
    this->cpu_inst->r_pc.set_value(0x100);
    this->ram_inst->memory[0x0100] = 0x28;
    this->ram_inst->memory[0x0101] = 0xff;
    this->cpu_inst->tick();
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x101);
}


void TestRunner::test_2f()
{
    std::cout << "0x02f";
    this->cpu_inst->r_a.set_value(0x9a);
    this->cpu_inst->r_pc.set_value(0);
    this->ram_inst->memory[0x0000] = 0x2f;
    this->cpu_inst->tick();
    
    this->assert_equal(this->cpu_inst->r_a.get_value(), 0x65);
}

void TestRunner::test_30()
{
    std::cout << "0x030";

    // Check jump of 0
    // Test with zero set
    this->cpu_inst->r_f.set_value(0x10);
    this->cpu_inst->r_pc.set_value(0);
    this->ram_inst->memory[0x0000] = 0x30;
    this->ram_inst->memory[0x0001] = 0x00;
    this->cpu_inst->tick();
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x02);
    
    // Check jump of 2
    this->cpu_inst->r_pc.set_value(0);
    this->ram_inst->memory[0x0000] = 0x30;
    this->ram_inst->memory[0x0001] = 0x02;
    this->cpu_inst->tick();
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x02);
    
    // Test jump of -1
    this->cpu_inst->r_pc.set_value(0x100);
    this->ram_inst->memory[0x0100] = 0x30;
    this->ram_inst->memory[0x0101] = 0xff;
    this->cpu_inst->tick();
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x0102);
    
    // Check jump of 0
    // Test with zero reset
    this->cpu_inst->r_f.set_value(0x00);
    this->cpu_inst->r_pc.set_value(0);
    this->ram_inst->memory[0x0000] = 0x30;
    this->ram_inst->memory[0x0001] = 0x00;
    this->cpu_inst->tick();
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x02);
    
    // Check jump of 2
    this->cpu_inst->r_pc.set_value(0);
    this->ram_inst->memory[0x0000] = 0x30;
    this->ram_inst->memory[0x0001] = 0x02;
    this->cpu_inst->tick();
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x04);
    
    // Test jump of -1
    this->cpu_inst->r_pc.set_value(0x100);
    this->ram_inst->memory[0x0100] = 0x30;
    this->ram_inst->memory[0x0101] = 0xff;
    this->cpu_inst->tick();
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x101);
}

void TestRunner::test_38()
{
    std::cout << "0x038";

    // Check jump of 0
    // Test with zero reset
    this->cpu_inst->r_f.set_value(0x00);
    this->cpu_inst->r_pc.set_value(0);
    this->ram_inst->memory[0x0000] = 0x38;
    this->ram_inst->memory[0x0001] = 0x00;
    this->cpu_inst->tick();
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x02);
    
    // Check jump of 2
    this->cpu_inst->r_pc.set_value(0);
    this->ram_inst->memory[0x0000] = 0x38;
    this->ram_inst->memory[0x0001] = 0x02;
    this->cpu_inst->tick();
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x02);
    
    // Test jump of -1
    this->cpu_inst->r_pc.set_value(0x100);
    this->ram_inst->memory[0x0100] = 0x38;
    this->ram_inst->memory[0x0101] = 0xff;
    this->cpu_inst->tick();
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x0102);
    
    // Check jump of 0
    // Test with zero set
    this->cpu_inst->r_f.set_value(0x10);
    this->cpu_inst->r_pc.set_value(0);
    this->ram_inst->memory[0x0000] = 0x38;
    this->ram_inst->memory[0x0001] = 0x00;
    this->cpu_inst->tick();
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x02);
    
    // Check jump of 2
    this->cpu_inst->r_pc.set_value(0);
    this->ram_inst->memory[0x0000] = 0x38;
    this->ram_inst->memory[0x0001] = 0x02;
    this->cpu_inst->tick();
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x04);
    
    // Test jump of -1
    this->cpu_inst->r_pc.set_value(0x100);
    this->ram_inst->memory[0x0100] = 0x38;
    this->ram_inst->memory[0x0101] = 0xff;
    this->cpu_inst->tick();
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x101);
}



void TestRunner::test_80()
{
    std::cout << "0x080";

    this->test_Add(&this->cpu_inst->r_b, 0x80);
}
void TestRunner::test_81()
{
    std::cout << "0x081";

    this->test_Add(&this->cpu_inst->r_c, 0x81);
}
void TestRunner::test_82()
{
    std::cout << "0x082";

    this->test_Add(&this->cpu_inst->r_d, 0x82);
}
void TestRunner::test_83()
{
    std::cout << "0x083";

    this->test_Add(&this->cpu_inst->r_e, 0x83);
}
void TestRunner::test_84()
{
    std::cout << "0x084";

    this->test_Add(&this->cpu_inst->r_h, 0x84);
}
void TestRunner::test_85()
{
    std::cout << "0x085";

    this->test_Add(&this->cpu_inst->r_l, 0x85);
}


void TestRunner::test_90()
{
    std::cout << "0x090";

    this->test_Sub(&this->cpu_inst->r_b, 0x90);
}
void TestRunner::test_91()
{
    std::cout << "0x091";

    this->test_Sub(&this->cpu_inst->r_c, 0x91);
}
void TestRunner::test_92()
{
    std::cout << "0x092";

    this->test_Sub(&this->cpu_inst->r_d, 0x92);
}
void TestRunner::test_93()
{
    std::cout << "0x093";

    this->test_Sub(&this->cpu_inst->r_e, 0x93);
}
void TestRunner::test_94()
{
    std::cout << "0x094";

    this->test_Sub(&this->cpu_inst->r_h, 0x94);
}
void TestRunner::test_95()
{
    std::cout << "0x095";

    this->test_Sub(&this->cpu_inst->r_l, 0x95);
}


void TestRunner::test_c0()
{
    std::cout << "0x0c0";
    
    // Check with flag reset
    this->cpu_inst->r_pc.set_value(0x1234);
    this->cpu_inst->r_f.set_value(0x70);
    
    this->ram_inst->memory[0x1234] = 0xc0;
    this->ram_inst->memory[0x5003] = 0xff;
    this->ram_inst->memory[0x5002] = 0x56;
    this->ram_inst->memory[0x5001] = 0x79;
    this->cpu_inst->r_sp.set_value(0x5001);

    this->cpu_inst->tick();

    this->assert_equal(this->cpu_inst->r_sp.get_value(), 0x5003);
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x5679);
    this->assert_equal(this->ram_inst->memory[0x5003], 0xff);
    this->assert_equal(this->ram_inst->memory[0x5002], 0x56);
    this->assert_equal(this->ram_inst->memory[0x5001], 0x79);
    
    
    // Test with zero flag set
    this->cpu_inst->r_pc.set_value(0x1234);
    this->cpu_inst->r_f.set_value(0x80);
    
    this->ram_inst->memory[0x1234] = 0xc0;
    this->ram_inst->memory[0x5003] = 0xff;
    this->ram_inst->memory[0x5002] = 0x56;
    this->ram_inst->memory[0x5001] = 0x79;
    this->cpu_inst->r_sp.set_value(0x5001);

    this->cpu_inst->tick();

    this->assert_equal(this->cpu_inst->r_sp.get_value(), 0x5001);
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x1235);
    this->assert_equal(this->ram_inst->memory[0x5003], 0xff);
    this->assert_equal(this->ram_inst->memory[0x5002], 0x56);
    this->assert_equal(this->ram_inst->memory[0x5001], 0x79);
}

void TestRunner::test_c4()
{
    std::cout << "0x0c4";
    
    this->cpu_inst->r_pc.set_value(0x5678);
    // Call with zero reset
    this->cpu_inst->r_f.set_value(0x70);
    
    this->ram_inst->memory[0x5678] = 0xc4;
    // Read in the LSB of the call address first
    this->ram_inst->memory[0x5679] = 0x34;
    // Read in the MSB second
    this->ram_inst->memory[0x567a] = 0x12;
    this->ram_inst->memory[0x5003] = 0xff;
    this->ram_inst->memory[0x5002] = 0xff;
    this->ram_inst->memory[0x5001] = 0xff;
    this->cpu_inst->r_sp.set_value(0x5003);

    this->cpu_inst->tick();

    this->assert_equal(this->cpu_inst->r_sp.get_value(), 0x5001);
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x1234);
    this->assert_equal(this->ram_inst->memory[0x5003], 0xff);
    // Store MSB at top of stack
    this->assert_equal(this->ram_inst->memory[0x5002], 0x56);
    // Store LSB at bottom of stack
    this->assert_equal(this->ram_inst->memory[0x5001], 0x7b);
    
    
    // Call with zero set
    this->cpu_inst->r_pc.set_value(0x5678);
    this->cpu_inst->r_f.set_value(0x80);
    
    this->ram_inst->memory[0x5678] = 0xc4;
    // Read in the LSB of the call address first
    this->ram_inst->memory[0x5679] = 0x34;
    // Read in the MSB second
    this->ram_inst->memory[0x567a] = 0x12;
    this->ram_inst->memory[0x5003] = 0xff;
    this->ram_inst->memory[0x5002] = 0xff;
    this->ram_inst->memory[0x5001] = 0xff;
    this->cpu_inst->r_sp.set_value(0x5003);

    this->cpu_inst->tick();

    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x567b);
    // Ensure stack hasn't been modified
    this->assert_equal(this->cpu_inst->r_sp.get_value(), 0x5003);
    this->assert_equal(this->ram_inst->memory[0x5003], 0xff);
    this->assert_equal(this->ram_inst->memory[0x5002], 0xff);
    this->assert_equal(this->ram_inst->memory[0x5001], 0xff);
}

void TestRunner::test_c8()
{
    std::cout << "0x0c8";
    
    // Check with not zero
    this->cpu_inst->r_pc.set_value(0x1234);
    this->cpu_inst->r_f.set_value(0x80);
    
    this->ram_inst->memory[0x1234] = 0xc8;
    this->ram_inst->memory[0x5003] = 0xff;
    this->ram_inst->memory[0x5002] = 0x56;
    this->ram_inst->memory[0x5001] = 0x79;
    this->cpu_inst->r_sp.set_value(0x5001);

    this->cpu_inst->tick();

    this->assert_equal(this->cpu_inst->r_sp.get_value(), 0x5003);
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x5679);
    this->assert_equal(this->ram_inst->memory[0x5003], 0xff);
    this->assert_equal(this->ram_inst->memory[0x5002], 0x56);
    this->assert_equal(this->ram_inst->memory[0x5001], 0x79);
    
    
    // Test with zero flag set
    this->cpu_inst->r_pc.set_value(0x1234);
    this->cpu_inst->r_f.set_value(0x70);
    
    this->ram_inst->memory[0x1234] = 0xc8;
    this->ram_inst->memory[0x5003] = 0xff;
    this->ram_inst->memory[0x5002] = 0x56;
    this->ram_inst->memory[0x5001] = 0x79;
    this->cpu_inst->r_sp.set_value(0x5001);

    this->cpu_inst->tick();

    this->assert_equal(this->cpu_inst->r_sp.get_value(), 0x5001);
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x1235);
    this->assert_equal(this->ram_inst->memory[0x5003], 0xff);
    this->assert_equal(this->ram_inst->memory[0x5002], 0x56);
    this->assert_equal(this->ram_inst->memory[0x5001], 0x79);
}

void TestRunner::test_cc()
{
    std::cout << "0x0cc";
    
    this->cpu_inst->r_pc.set_value(0x5678);
    // Call with zero set
    this->cpu_inst->r_f.set_value(0x80);
    
    this->ram_inst->memory[0x5678] = 0xcc;
    // Read in the LSB of the call address first
    this->ram_inst->memory[0x5679] = 0x34;
    // Read in the MSB second
    this->ram_inst->memory[0x567a] = 0x12;
    this->ram_inst->memory[0x5003] = 0xff;
    this->ram_inst->memory[0x5002] = 0xff;
    this->ram_inst->memory[0x5001] = 0xff;
    this->cpu_inst->r_sp.set_value(0x5003);

    this->cpu_inst->tick();

    this->assert_equal(this->cpu_inst->r_sp.get_value(), 0x5001);
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x1234);
    this->assert_equal(this->ram_inst->memory[0x5003], 0xff);
    // Store MSB at top of stack
    this->assert_equal(this->ram_inst->memory[0x5002], 0x56);
    // Store LSB at bottom of stack
    this->assert_equal(this->ram_inst->memory[0x5001], 0x7b);
    
    
    // Call with zero reset
    this->cpu_inst->r_pc.set_value(0x5678);
    this->cpu_inst->r_f.set_value(0x70);
    
    this->ram_inst->memory[0x5678] = 0xcc;
    // Read in the LSB of the call address first
    this->ram_inst->memory[0x5679] = 0x34;
    // Read in the MSB second
    this->ram_inst->memory[0x567a] = 0x12;
    this->ram_inst->memory[0x5003] = 0xff;
    this->ram_inst->memory[0x5002] = 0xff;
    this->ram_inst->memory[0x5001] = 0xff;
    this->cpu_inst->r_sp.set_value(0x5003);

    this->cpu_inst->tick();

    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x567b);
    // Ensure stack hasn't been modified
    this->assert_equal(this->cpu_inst->r_sp.get_value(), 0x5003);
    this->assert_equal(this->ram_inst->memory[0x5003], 0xff);
    this->assert_equal(this->ram_inst->memory[0x5002], 0xff);
    this->assert_equal(this->ram_inst->memory[0x5001], 0xff);
}

void TestRunner::test_cd()
{
    std::cout << "0x0cd";
    
    this->cpu_inst->r_pc.set_value(0x5678);
    this->cpu_inst->r_f.set_value(0xf0);
    
    this->ram_inst->memory[0x5678] = 0xcd;
    // Read in the LSB of the call address first
    this->ram_inst->memory[0x5679] = 0x34;
    // Read in the MSB second
    this->ram_inst->memory[0x567a] = 0x12;
    this->ram_inst->memory[0x5003] = 0xff;
    this->ram_inst->memory[0x5002] = 0xff;
    this->ram_inst->memory[0x5001] = 0xff;
    this->cpu_inst->r_sp.set_value(0x5003);

    this->cpu_inst->tick();

    this->assert_equal(this->cpu_inst->r_sp.get_value(), 0x5001);
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x1234);
    this->assert_equal(this->ram_inst->memory[0x5003], 0xff);
    // Store MSB at top of stack
    this->assert_equal(this->ram_inst->memory[0x5002], 0x56);
    // Store LSB at bottom of stack
    this->assert_equal(this->ram_inst->memory[0x5001], 0x7b);
}

void TestRunner::test_d0()
{
    std::cout << "0x0d0";
    
    // Check with flag reset
    this->cpu_inst->r_pc.set_value(0x1234);
    this->cpu_inst->r_f.set_value(0xe0);
    
    this->ram_inst->memory[0x1234] = 0xd0;
    this->ram_inst->memory[0x5003] = 0xff;
    this->ram_inst->memory[0x5002] = 0x56;
    this->ram_inst->memory[0x5001] = 0x79;
    this->cpu_inst->r_sp.set_value(0x5001);

    this->cpu_inst->tick();

    this->assert_equal(this->cpu_inst->r_sp.get_value(), 0x5003);
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x5679);
    this->assert_equal(this->ram_inst->memory[0x5003], 0xff);
    this->assert_equal(this->ram_inst->memory[0x5002], 0x56);
    this->assert_equal(this->ram_inst->memory[0x5001], 0x79);
    
    
    // Test with zero flag set
    this->cpu_inst->r_pc.set_value(0x1234);
    this->cpu_inst->r_f.set_value(0x10);
    
    this->ram_inst->memory[0x1234] = 0xd0;
    this->ram_inst->memory[0x5003] = 0xff;
    this->ram_inst->memory[0x5002] = 0x56;
    this->ram_inst->memory[0x5001] = 0x79;
    this->cpu_inst->r_sp.set_value(0x5001);

    this->cpu_inst->tick();

    this->assert_equal(this->cpu_inst->r_sp.get_value(), 0x5001);
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x1235);
    this->assert_equal(this->ram_inst->memory[0x5003], 0xff);
    this->assert_equal(this->ram_inst->memory[0x5002], 0x56);
    this->assert_equal(this->ram_inst->memory[0x5001], 0x79);
}


void TestRunner::test_d4()
{
    std::cout << "0x0d4";
    
    this->cpu_inst->r_pc.set_value(0x5678);
    // Call with zero reset
    this->cpu_inst->r_f.set_value(0xe0);
    
    this->ram_inst->memory[0x5678] = 0xd4;
    // Read in the LSB of the call address first
    this->ram_inst->memory[0x5679] = 0x34;
    // Read in the MSB second
    this->ram_inst->memory[0x567a] = 0x12;
    this->ram_inst->memory[0x5003] = 0xff;
    this->ram_inst->memory[0x5002] = 0xff;
    this->ram_inst->memory[0x5001] = 0xff;
    this->cpu_inst->r_sp.set_value(0x5003);

    this->cpu_inst->tick();

    this->assert_equal(this->cpu_inst->r_sp.get_value(), 0x5001);
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x1234);
    this->assert_equal(this->ram_inst->memory[0x5003], 0xff);
    // Store MSB at top of stack
    this->assert_equal(this->ram_inst->memory[0x5002], 0x56);
    // Store LSB at bottom of stack
    this->assert_equal(this->ram_inst->memory[0x5001], 0x7b);
    
    
    // Call with zero set
    this->cpu_inst->r_pc.set_value(0x5678);
    this->cpu_inst->r_f.set_value(0x10);
    
    this->ram_inst->memory[0x5678] = 0xd4;
    // Read in the LSB of the call address first
    this->ram_inst->memory[0x5679] = 0x34;
    // Read in the MSB second
    this->ram_inst->memory[0x567a] = 0x12;
    this->ram_inst->memory[0x5003] = 0xff;
    this->ram_inst->memory[0x5002] = 0xff;
    this->ram_inst->memory[0x5001] = 0xff;
    this->cpu_inst->r_sp.set_value(0x5003);

    this->cpu_inst->tick();

    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x567b);
    // Ensure stack hasn't been modified
    this->assert_equal(this->cpu_inst->r_sp.get_value(), 0x5003);
    this->assert_equal(this->ram_inst->memory[0x5003], 0xff);
    this->assert_equal(this->ram_inst->memory[0x5002], 0xff);
    this->assert_equal(this->ram_inst->memory[0x5001], 0xff);
}

void TestRunner::test_d8()
{
    std::cout << "0x0d8";
    
    // Check with flag reset
    this->cpu_inst->r_pc.set_value(0x1234);
    this->cpu_inst->r_f.set_value(0x10);
    
    this->ram_inst->memory[0x1234] = 0xd8;
    this->ram_inst->memory[0x5003] = 0xff;
    this->ram_inst->memory[0x5002] = 0x56;
    this->ram_inst->memory[0x5001] = 0x79;
    this->cpu_inst->r_sp.set_value(0x5001);

    this->cpu_inst->tick();

    this->assert_equal(this->cpu_inst->r_sp.get_value(), 0x5003);
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x5679);
    this->assert_equal(this->ram_inst->memory[0x5003], 0xff);
    this->assert_equal(this->ram_inst->memory[0x5002], 0x56);
    this->assert_equal(this->ram_inst->memory[0x5001], 0x79);
    
    
    // Test with zero flag set
    this->cpu_inst->r_pc.set_value(0x1234);
    this->cpu_inst->r_f.set_value(0xe0);
    
    this->ram_inst->memory[0x1234] = 0xd8;
    this->ram_inst->memory[0x5003] = 0xff;
    this->ram_inst->memory[0x5002] = 0x56;
    this->ram_inst->memory[0x5001] = 0x79;
    this->cpu_inst->r_sp.set_value(0x5001);

    this->cpu_inst->tick();

    this->assert_equal(this->cpu_inst->r_sp.get_value(), 0x5001);
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x1235);
    this->assert_equal(this->ram_inst->memory[0x5003], 0xff);
    this->assert_equal(this->ram_inst->memory[0x5002], 0x56);
    this->assert_equal(this->ram_inst->memory[0x5001], 0x79);
}

void TestRunner::test_dc()
{
    std::cout << "0x0dc";
    
    this->cpu_inst->r_pc.set_value(0x5678);
    // Call with zero set
    this->cpu_inst->r_f.set_value(0x10);
    
    this->ram_inst->memory[0x5678] = 0xdc;
    // Read in the LSB of the call address first
    this->ram_inst->memory[0x5679] = 0x34;
    // Read in the MSB second
    this->ram_inst->memory[0x567a] = 0x12;
    this->ram_inst->memory[0x5003] = 0xff;
    this->ram_inst->memory[0x5002] = 0xff;
    this->ram_inst->memory[0x5001] = 0xff;
    this->cpu_inst->r_sp.set_value(0x5003);

    this->cpu_inst->tick();

    this->assert_equal(this->cpu_inst->r_sp.get_value(), 0x5001);
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x1234);
    this->assert_equal(this->ram_inst->memory[0x5003], 0xff);
    // Store MSB at top of stack
    this->assert_equal(this->ram_inst->memory[0x5002], 0x56);
    // Store LSB at bottom of stack
    this->assert_equal(this->ram_inst->memory[0x5001], 0x7b);
    
    
    // Call with zero reset
    this->cpu_inst->r_pc.set_value(0x5678);
    this->cpu_inst->r_f.set_value(0xe0);
    
    this->ram_inst->memory[0x5678] = 0xdc;
    // Read in the LSB of the call address first
    this->ram_inst->memory[0x5679] = 0x34;
    // Read in the MSB second
    this->ram_inst->memory[0x567a] = 0x12;
    this->ram_inst->memory[0x5003] = 0xff;
    this->ram_inst->memory[0x5002] = 0xff;
    this->ram_inst->memory[0x5001] = 0xff;
    this->cpu_inst->r_sp.set_value(0x5003);

    this->cpu_inst->tick();

    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x567b);
    // Ensure stack hasn't been modified
    this->assert_equal(this->cpu_inst->r_sp.get_value(), 0x5003);
    this->assert_equal(this->ram_inst->memory[0x5003], 0xff);
    this->assert_equal(this->ram_inst->memory[0x5002], 0xff);
    this->assert_equal(this->ram_inst->memory[0x5001], 0xff);
}

// CB TESTS
void TestRunner::test_cb_00()
{
    std::cout << "0x100";

    this->test_RLC(&this->cpu_inst->r_b, 0x00);
}
void TestRunner::test_cb_01()
{
    std::cout << "0x101";

    this->test_RLC(&this->cpu_inst->r_c, 0x01);
}
void TestRunner::test_cb_02()
{
    std::cout << "0x102";

    this->test_RLC(&this->cpu_inst->r_d, 0x02);
}
void TestRunner::test_cb_03()
{
    std::cout << "0x103";

    this->test_RLC(&this->cpu_inst->r_e, 0x03);
}
void TestRunner::test_cb_04()
{
    std::cout << "0x104";

    this->test_RLC(&this->cpu_inst->r_h, 0x04);
}
void TestRunner::test_cb_05()
{
    std::cout << "0x105";

    this->test_RLC(&this->cpu_inst->r_l, 0x05);
}

void TestRunner::test_cb_10()
{
    std::cout << "0x110";

    this->test_RL(&this->cpu_inst->r_b, 0x10);
}
void TestRunner::test_cb_11()
{
    std::cout << "0x111";

    this->test_RL(&this->cpu_inst->r_c, 0x11);
}
void TestRunner::test_cb_12()
{
    std::cout << "0x112";

    this->test_RL(&this->cpu_inst->r_d, 0x12);
}
void TestRunner::test_cb_13()
{
    std::cout << "0x113";

    this->test_RL(&this->cpu_inst->r_e, 0x13);
}
void TestRunner::test_cb_14()
{
    std::cout << "0x114";

    this->test_RL(&this->cpu_inst->r_h, 0x14);
}
void TestRunner::test_cb_15()
{
    std::cout << "0x115";

    this->test_RL(&this->cpu_inst->r_l, 0x15);
}

void TestRunner::test_cb_18()
{
    std::cout << "0x118";

    this->test_RR(&this->cpu_inst->r_b, 0x18);
}
void TestRunner::test_cb_19()
{
    std::cout << "0x119";

    this->test_RR(&this->cpu_inst->r_c, 0x19);
}
void TestRunner::test_cb_1a()
{
    std::cout << "0x11a";

    this->test_RR(&this->cpu_inst->r_d, 0x1a);
}
void TestRunner::test_cb_1b()
{
    std::cout << "0x11b";

    this->test_RR(&this->cpu_inst->r_e, 0x1b);
}
void TestRunner::test_cb_1c()
{
    std::cout << "0x11c";

    this->test_RR(&this->cpu_inst->r_h, 0x1c);
}
void TestRunner::test_cb_1d()
{
    std::cout << "0x11d";

    this->test_RR(&this->cpu_inst->r_l, 0x1d);
}


void TestRunner::test_cb_30()
{
    std::cout << "0x130";

    this->test_SWAP(&this->cpu_inst->r_b, 0x30);
}
void TestRunner::test_cb_31()
{
    std::cout << "0x131";

    this->test_SWAP(&this->cpu_inst->r_c, 0x31);
}
void TestRunner::test_cb_32()
{
    std::cout << "0x132";

    this->test_SWAP(&this->cpu_inst->r_d, 0x32);
}
void TestRunner::test_cb_33()
{
    std::cout << "0x133";

    this->test_SWAP(&this->cpu_inst->r_e, 0x33);
}
void TestRunner::test_cb_34()
{
    std::cout << "0x134";

    this->test_SWAP(&this->cpu_inst->r_h, 0x34);
}
void TestRunner::test_cb_35()
{
    std::cout << "0x135";

    this->test_SWAP(&this->cpu_inst->r_l, 0x35);
}



//void TestRunner::test_Inc(reg8 *reg, uint8_t)
//{
//
//}


void TestRunner::test_Add(reg8 *reg, uint8_t op_code)
{
    this->cpu_inst->r_pc.set_value(0x0000);
    this->cpu_inst->r_f.set_value(0xf0);
    
    // Test standard addition
    reg->set_value(0x04);
    this->cpu_inst->r_a.set_value(0x06);
    this->ram_inst->memory[0x0000] = op_code;
    this->cpu_inst->tick();
    
    this->assert_equal(reg->get_value(), 0x04);
    this->assert_equal(this->cpu_inst->r_a.get_value(), 0x0a);
    this->assert_equal(this->cpu_inst->r_f.get_value(), 0x00);

    // Test half-carry
    reg->set_value(0x0f);
    this->cpu_inst->r_a.set_value(0x02);
    this->ram_inst->memory[0x0001] = op_code;
    this->cpu_inst->tick();
    
    this->assert_equal(reg->get_value(), 0x0f);
    this->assert_equal(this->cpu_inst->r_a.get_value(), 0x11);
    this->assert_equal(this->cpu_inst->r_f.get_value(), 0x20);

    // Test full carry
    reg->set_value(0xa4);
    this->cpu_inst->r_a.set_value(0x87);
    this->ram_inst->memory[0x0002] = op_code;
    this->cpu_inst->tick();
    
    this->assert_equal(reg->get_value(), 0xa4);
    this->assert_equal(this->cpu_inst->r_a.get_value(), 0x2b);
    this->assert_equal(this->cpu_inst->r_f.get_value(), 0x10);

    // Test half-carry, full carry and zero
    reg->set_value(0xff);
    this->cpu_inst->r_a.set_value(0x01);
    this->ram_inst->memory[0x0003] = op_code;
    this->cpu_inst->tick();
    
    this->assert_equal(reg->get_value(), 0xff);
    this->assert_equal(this->cpu_inst->r_a.get_value(), 0x00);
    this->assert_equal(this->cpu_inst->r_f.get_value(), 0xb0);
}

void TestRunner::test_Sub(reg8 *reg, uint8_t op_code)
{
    this->cpu_inst->r_pc.set_value(0x0000);
    this->cpu_inst->r_f.set_value(0xf0);
    
    // Test standard addition
    this->cpu_inst->r_a.set_value(0x06);
    reg->set_value(0x02);
    this->ram_inst->memory[0x0000] = op_code;
    this->cpu_inst->tick();
    
    this->assert_equal(reg->get_value(), 0x02);
    this->assert_equal(this->cpu_inst->r_a.get_value(), 0x04);
    this->assert_equal(this->cpu_inst->r_f.get_value(), 0x40);

    // Test half-carry
    this->cpu_inst->r_a.set_value(0x52);
    reg->set_value(0x1b);
    this->ram_inst->memory[0x0001] = op_code;
    this->cpu_inst->tick();
    
    this->assert_equal(reg->get_value(), 0x1b);
    this->assert_equal(this->cpu_inst->r_a.get_value(), 0x37);
    this->assert_equal(this->cpu_inst->r_f.get_value(), 0x60);

    // Test full carry
    this->cpu_inst->r_a.set_value(0x3f);
    reg->set_value(0x8b);
    this->ram_inst->memory[0x0002] = op_code;
    this->cpu_inst->tick();
    
    this->assert_equal(reg->get_value(), 0x8b);
    this->assert_equal(this->cpu_inst->r_a.get_value(), 0xb4);
    this->assert_equal(this->cpu_inst->r_f.get_value(), 0x50);

    // Test remove to zero, with half carry
    reg->set_value(0x60);
    this->cpu_inst->r_a.set_value(0x60);
    this->ram_inst->memory[0x0003] = op_code;
    this->cpu_inst->tick();
    
    this->assert_equal(reg->get_value(), 0x60);
    this->assert_equal(this->cpu_inst->r_a.get_value(), 0x00);
    this->assert_equal(this->cpu_inst->r_f.get_value(), 0xc0);
}

void TestRunner::test_RLC(reg8 *reg, uint8_t op_code)
{
    // Test RLC
    this->cpu_inst->r_pc.set_value(0x0000);
    this->cpu_inst->r_f.set_value(0xf0);
    reg->set_value(0xaf);
    this->ram_inst->memory[0x0000] = 0xcb;
    this->ram_inst->memory[0x0001] = op_code;
    this->cpu_inst->tick(); // Enable cb-mode
    this->cpu_inst->tick(); // Perform RLC

    // Assert value at b
    this->assert_equal(reg->get_value(), 0x5f);

    // Ensure carry flag is set to 1 (the value that was moved
    // left
    this->assert_equal(this->cpu_inst->r_f.get_value(), 0x10);

    // Ensure that SP has moved on
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x0002);

    // Test Moving 0 into carry flag
    reg->set_value(0x02);
    this->ram_inst->memory[0x0002] = 0xcb;
    this->ram_inst->memory[0x0003] = op_code;
    this->cpu_inst->tick(); // Enable cb-mode
    this->cpu_inst->tick(); // Perform RLC

    this->assert_equal(reg->get_value(), 0x04);

    // Ensure carry flag is set to 0 (the value that was moved
    // left
    this->assert_equal(this->cpu_inst->r_f.get_value(), 0x00);

    // Ensure that SP has moved on
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x0004);

    // Test where outcome is 0
    reg->set_value(0x80);
    this->ram_inst->memory[0x0004] = 0xcb;
    this->ram_inst->memory[0x0005] = op_code;
    this->cpu_inst->tick(); // Enable cb-mode
    this->cpu_inst->tick(); // Perform RLC

    this->assert_equal(reg->get_value(), 0x01);

    // Ensure carry flag is set to 0 (the value that was moved
    // left
    this->assert_equal(this->cpu_inst->r_f.get_value(), 0x10);

    // Ensure that SP has moved on
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x0006);
}

void TestRunner::test_RL(reg8 *reg, uint8_t op_code)
{
    // Test RL
    this->cpu_inst->r_pc.set_value(0x0000);
    this->cpu_inst->r_f.set_value(0xf0);
    reg->set_value(0xad);
    this->ram_inst->memory[0x0000] = 0xcb;
    this->ram_inst->memory[0x0001] = op_code;
    this->cpu_inst->tick(); // Enable cb-mode
    this->cpu_inst->tick(); // Perform RLC

    // Assert value at b
    this->assert_equal(reg->get_value(), 0x5b);

    // Ensure carry flag is set to 1 (the value that was moved
    // left
    this->assert_equal(this->cpu_inst->r_f.get_value(), 0x10);

    // Ensure that SP has moved on
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x0002);

    // Test Moving 0 into carry flag
    this->ram_inst->memory[0x0002] = 0xcb;
    this->ram_inst->memory[0x0003] = op_code;
    this->cpu_inst->tick(); // Enable cb-mode
    this->cpu_inst->tick(); // Perform RL

    this->assert_equal(reg->get_value(), 0xb7);

    // Ensure carry flag is set to 0 (the value that was moved
    // left
    this->assert_equal(this->cpu_inst->r_f.get_value(), 0x00);

    // Ensure that SP has moved on
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x0004);

    // Test where outcome is 0
    reg->set_value(0x80);
    this->ram_inst->memory[0x0004] = 0xcb;
    this->ram_inst->memory[0x0005] = op_code;
    this->cpu_inst->tick(); // Enable cb-mode
    this->cpu_inst->tick(); // Perform RL

    this->assert_equal(reg->get_value(), 0x00);

    // Ensure carry flag is set to 0 (the value that was moved
    // left
    this->assert_equal(this->cpu_inst->r_f.get_value(), 0x90);

    // Ensure that SP has moved on
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x0006);
}


void TestRunner::test_RR(reg8 *reg, uint8_t op_code)
{
    // Test RL
    this->cpu_inst->r_pc.set_value(0x0000);
    this->cpu_inst->r_f.set_value(0xf0);
    reg->set_value(0xad);
    this->ram_inst->memory[0x0000] = 0xcb;
    this->ram_inst->memory[0x0001] = op_code;
    this->cpu_inst->tick(); // Enable cb-mode
    this->cpu_inst->tick(); // Perform RLC

    // Assert value at b
    this->assert_equal(reg->get_value(), 0xd6);

    // Ensure carry flag is set to 1 (the value that was moved
    // left
    this->assert_equal(this->cpu_inst->r_f.get_value(), 0x10);

    // Ensure that SP has moved on
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x0002);

    // Test Moving 0 into carry flag
    this->ram_inst->memory[0x0002] = 0xcb;
    this->ram_inst->memory[0x0003] = op_code;
    this->cpu_inst->tick(); // Enable cb-mode
    this->cpu_inst->tick(); // Perform RL

    this->assert_equal(reg->get_value(), 0xeb);

    // Ensure carry flag is set to 0 (the value that was moved
    // left
    this->assert_equal(this->cpu_inst->r_f.get_value(), 0x00);

    // Ensure that SP has moved on
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x0004);

    // Test where outcome is 0
    reg->set_value(0x01);
    this->ram_inst->memory[0x0004] = 0xcb;
    this->ram_inst->memory[0x0005] = op_code;
    this->cpu_inst->tick(); // Enable cb-mode
    this->cpu_inst->tick(); // Perform RL

    this->assert_equal(reg->get_value(), 0x00);

    // Ensure carry flag is set to 0 (the value that was moved
    // left
    this->assert_equal(this->cpu_inst->r_f.get_value(), 0x90);

    // Ensure that SP has moved on
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x0006);
}

void TestRunner::test_SWAP(reg8 *reg, uint8_t op_code)
{
    // Test RL
    this->cpu_inst->r_pc.set_value(0x0000);
    this->cpu_inst->r_f.set_value(0xf0);
    reg->set_value(0x85);
    this->ram_inst->memory[0x0000] = 0xcb;
    this->ram_inst->memory[0x0001] = op_code;
    this->cpu_inst->tick(); // Enable cb-mode
    this->cpu_inst->tick(); // Perform RLC

    // Assert value at b
    this->assert_equal(reg->get_value(), 0x58);

    // Ensure carry flag is set to 1 (the value that was moved
    // left
    this->assert_equal(this->cpu_inst->r_f.get_value(), 0x00);

    // Ensure that SP has moved on
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x0002);

    // Test Moving 0 into carry flag
    this->ram_inst->memory[0x0002] = 0xcb;
    this->ram_inst->memory[0x0003] = op_code;
    this->cpu_inst->tick(); // Enable cb-mode
    this->cpu_inst->tick(); // Perform RL

    this->assert_equal(reg->get_value(), 0x85);

    // Ensure carry flag is set to 0 (the value that was moved
    // left
    this->assert_equal(this->cpu_inst->r_f.get_value(), 0x00);

    // Ensure that SP has moved on
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x0004);

    // Test where outcome is 0
    reg->set_value(0x00);
    this->ram_inst->memory[0x0004] = 0xcb;
    this->ram_inst->memory[0x0005] = op_code;
    this->cpu_inst->tick(); // Enable cb-mode
    this->cpu_inst->tick(); // Perform RL

    this->assert_equal(reg->get_value(), 0x00);

    // Ensure carry flag is set to 0 (the value that was moved
    // left
    this->assert_equal(this->cpu_inst->r_f.get_value(), 0x80);

    // Ensure that SP has moved on
    this->assert_equal(this->cpu_inst->r_pc.get_value(), 0x0006);
}
