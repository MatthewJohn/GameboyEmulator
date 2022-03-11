
#include <iostream>
#include <ios>

class Helper {
public:
    static void init() {};
    static void reset_cout() {};
private:
    static std::ios_base::fmtflags RESET_FLAGS;
};
