#include <iostream>
#include <stdint.h>

float test(uint32_t num) {
    union {
        float flt;
        uint32_t num;
    } in;
    uint32_t n, j;
    in.num = num;
    return in.flt;
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
