// Copyright (C) Dock Studios Ltd, Inc - All Rights Reserved
// Unauthorized copying of this file, via any medium is strictly prohibited
// Proprietary and confidential
// Written by Matt Comben <matthew@dockstudios.co.uk>, May 2019

#ifndef HELPER_DEF
#define HELPER_DEF

#include <iostream>
#include <ios>

class Helper {
public:
    static void init() {};
    static void reset_cout() {};
private:
    static std::ios_base::fmtflags RESET_FLAGS;
};

#define PATH_SIZE 4096

struct arguments_t
{
    char bios_path[PATH_SIZE];
    char rom_path[PATH_SIZE];
    char screenshot_path[PATH_SIZE];
    int screenshot_ticks;
};

#endif