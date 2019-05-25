#ifdef HELPER_INCLUDED
#define HELPER_INCLUDED
#include <iostream>
#include "helper.h"


void Helper::reset_cout() {
     std::cout.flags(Helper::RESET_FLAGS);
}
void Helper::init() {
    Helper::RESET_FLAGS = std::cout.flags();
}
#endif