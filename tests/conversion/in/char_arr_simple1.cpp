#include <iostream>
#include <stdint.h>

typedef long T;

T test(int c) {
    T x = 300000*c;
    if (sizeof(T) > 1) {
        union {
            T x;
            char b[sizeof(T)];
        } u;
        u.x = x;
        return u.b[0]+u.b[1]+u.b[2];
    }
    return -1;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <float>" << std::endl;
        return 1;
    }
    float input = std::stof(argv[1]);
    int c = (int)input;
    std::cout << test(c) << std::endl;
    return 0;
}
