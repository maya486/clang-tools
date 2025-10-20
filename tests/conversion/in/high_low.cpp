#include <iostream>
#include <stdint.h>

uint32_t test(uint64_t n) {
    n <<= 25;
    union {
        uint64_t __ll;
        struct {
            uint32_t __l, __h;
        } __i;
    } __x;
    __x.__ll = n;
    return __x.__i.__h  * __x.__i.__l;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <float>" << std::endl;
        return 1;
    }
    float input = std::stof(argv[1]);
    std::cout << test((uint64_t)input) << std::endl;
    return 0;
}
