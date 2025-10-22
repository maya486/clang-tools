#include <cstring>
#include <iostream>
#include <stdint.h>

typedef long T;

void tenjin_T_to_char(T x, char *out) {
    memcpy(out, &x, 8);
}

T test(int c) {
    T x = 300000*c;
    if (sizeof(T) > 1) {
        T __tenjin_tmp_in_u;
char __tenjin_tmp_out_u[8];

        __tenjin_tmp_in_u = x;
        tenjin_T_to_char(__tenjin_tmp_in_u, __tenjin_tmp_out_u);
        return __tenjin_tmp_out_u[0]+__tenjin_tmp_out_u[1]+__tenjin_tmp_out_u[2];
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
