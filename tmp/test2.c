#include <iostream>
#include <memory>
#include <iomanip>
#include <string.h>


void convert(int &val) {
  val ++;
}

uint8_t convert_int8_uint8(uint8_t in_val) {
    union {
        uint8_t uint8[2];
        int8_t int8[2];
    } data;
    data.int8[0] = (int8_t)in_val;
    return data.uint8[0];
}

signed int test()
{
    uint8_t orig = (uint8_t)0xfb;
    //uint8_t* orig_ptr = orig;
    //int8_t* new_ptr = &orig_ptr;
    //int8_t new_ = static_cast<uint8_t>(orig);
    //memcpy(&new_, &orig, 1);
    //uint8_t res = &new_ptr;
    //if (orig & 0x70)
    //{
    //return (int)(0 - (unsigned int)orig);
    //}
    int ret;
    if (orig & 0x80)
	ret = 0 - ((orig - 1) ^ 0xff);
    else
        ret = orig;
    return ret;
    //int8_t new_ = (int8_t)orig;
    //return (int)new_;
}

int main() {
  //uint8_t bin = 0x2;
  //uint8_t test_me = 0x4;
  //uint16_t test2 = test_me;
  //convert(test_me);
  //convert(test_me);
  //uint8_t test_carry = bin;
  //test_carry &= test_me;
  //test_carry = test_carry >> (bin - 0x1);
  //
  //union {
  //  uint8_t in[1];
  //  int8_t out[1];
    //uint16_t out[1];
  //} test_carry;
  //test_carry.in[0] = (uint8_t)0xfb;
  //uint8_t test = 0x1c;
  //uint8_t test2 = (int8_t)test;
  //uint8_t test3 = test2;
  //std::cout << std::hex << (int)test << std::endl;
  //std::cout << std::hex << (int)test3 << std::endl;
  //int8_t output = (uint8_t)test_carry.in[0];
  //test test_carry;
  //test_carry.in[0] = (uint8_t)0x00;
  //test_carry.in[1] = (uint8_t)0x11;
  //std::cout << std::hex << (int)output << std::endl;
  //unsigned int test_bit = 7;
  //uint8_t val = 0x7f;
  //unsigned int res = ((val & (1U << test_bit)) >> test_bit);
  //std::cout << std::hex << int(res) << std::endl;
  //unsigned int test1 = 5;
  //unsigned int test2 = 2;
  //int8_t test1 = (int8_t)0x81;
  //uint16_t test2 = 0x0000;
  //std::cout << 512 + (int8_t)0xff << std::endl;
  union {
        uint8_t bit8[2];
        uint16_t bit16[1];
    } data_conv;
  data_conv.bit8[0] = 0xff;
  data_conv.bit8[1] = 0;
  std::cout << std::hex << data_conv.bit16[0];
}

