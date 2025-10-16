#include <stdint.h>
#include <iostream>

uint32_t test(float flt) {
    union {
        float flt;
        uint32_t num;
    } in;
    union {
        float flt;
        uint32_t num;
    } in2;
    in2.num = 0;
    uint32_t n, j;
    in.flt = flt;
    return in2.num;
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
