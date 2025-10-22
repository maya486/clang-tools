#include <cstring>
#include <stdint.h>
#include <iostream>

void tenjin_float_to_uint32_t(float x, uint32_t *out) {
    memcpy(out, &x, 4);
}

uint32_t test(float flt) {
    float __tenjin_tmp_in_in;
uint32_t __tenjin_tmp_out_in;

    uint32_t n, j;
    __tenjin_tmp_in_in = flt;
    tenjin_float_to_uint32_t(__tenjin_tmp_in_in, &__tenjin_tmp_out_in);
    return __tenjin_tmp_out_in;
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
