#include <cstring>
#include <stdint.h>
#include <iostream>

uint32_t tenjin_f32_to_u32(float x) {
    uint32_t y;
    memcpy(&y, &x, sizeof y);
    return y;
}

uint32_t test(float flt) {
    
    uint32_t n, j;
    float __tenjin_tmp_in_in = flt;
    __tenjin_tmp_in_in = __tenjin_tmp_in_in + 3;
    uint32_t __tenjin_tmp_out_in = tenjin_f32_to_u32(__tenjin_tmp_in_in);
    uint32_t a = __tenjin_tmp_out_in*3;
    uint32_t b = __tenjin_tmp_out_in+2;
    return a-b;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <float>" << std::endl;
        return 1;
    }
    float input = std::stof(argv[1]);
    std::cout << test(input) << std::endl;
    return 0;
}
