#include <iostream>
#include <stdint.h>

typedef long T;

T test(char c) {
    T x = 300000;
    if (sizeof(T) > 1) {
        union {
            T x;
            char b[sizeof(T)];
        } u;
        u.b[0] = c;
        u.b[1] = c+1;
        u.b[3] = c-3;
        x = u.x;
        return x+3;
    }
    return -1;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <float>" << std::endl;
        return 1;
    }
    float input = std::stof(argv[1]);
    char c = (char)input;
    std::cout << test(c) << std::endl;
    return 0;
}
