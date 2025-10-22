#include <cstring>
#include <iostream>
#include <stdint.h>

void tenjin_uint32_t_to_float(uint32_t x, float *out) {
    memcpy(out, &x, 4);
}

float test(uint32_t num) {
    uint32_t __tenjin_tmp_in_in;
float __tenjin_tmp_out_in;

    uint32_t n, j;
    __tenjin_tmp_in_in = num;
    tenjin_uint32_t_to_float(__tenjin_tmp_in_in, &__tenjin_tmp_out_in);
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
