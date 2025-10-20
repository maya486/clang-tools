#include <iostream>

float test(float a) {
    union U { int i; float f; };
    union U u1, u2;
    u1.i = 0;
    u1.f = 0;
    u2.i = 0;
    u2.f = 0;
    u1.i = int(a);
    float x = u2.f;   // separate variable!
    return x;
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

