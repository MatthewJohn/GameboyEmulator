
#include <memory>
#include <iostream>

// Any register
struct reg { };

// 8-bit register
struct reg8 : reg {
    uint8_t value;
};
// 16-bit register
struct reg16 : reg {
    uint16_t value;
};

// Accumulator
struct accumulator : reg8 { };
// General Register
struct gen_reg : reg8 { };
// Stack pointer
struct stack_pointer : reg16 { };
// Program counter
struct program_counter : reg16 { };

union combined_reg {
    uintptr_t lower[2];
    uint16_t value;
};


int main() {
  reg8 r_a;
  reg8 r_b;
  r_a.value = 2;
  r_b.value = 1;
  combined_reg combin;
  combin.lower[0] = r_a.value;
  combin.lower[1] = r_b.value;
  int output = combin.value;
  std::cout << output << std::endl;

  r_a.value = 3;
  output = combin.value;
  std::cout << output << std::endl;

  return 0;
}
