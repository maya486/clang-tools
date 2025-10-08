#include <cstring>
#include <iostream>
#include <stdint.h>

float tenjin_u32_to_f32(uint32_t x) {
    float y;
    memcpy(&y, &x, sizeof y);
    return y;
}

float test(uint32_t num) {
    
    uint32_t n, j;
    uint32_t __tenjin_tmp_in_in = num;
    float __tenjin_tmp_out_in = tenjin_u32_to_f32(__tenjin_tmp_in_in);
    return __tenjin_tmp_out_in;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <float>" << std::endl;
        return 1;
    }
    float input = std::stof(argv[1]);
    std::cout << test((uint32_t)input) << std::endl;
    return 0;
}
