#include <stdint.h>
#include <iostream>

float test(float flt) {
    union {
        float flt;
        uint32_t num;
    } in;
    uint32_t n, j;
    in.flt = flt;
    in.num = 2;
    float a = in.flt;
    return a + 10;
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

