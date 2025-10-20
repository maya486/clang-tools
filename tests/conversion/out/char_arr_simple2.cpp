#include <cstring>
#include <iostream>
#include <stdint.h>

typedef long T;

void tenjin_char_to_T(char *x, T *out) {
    memcpy(out, x, 8);
}

T test(char c) {
    T x = 300000;
    if (sizeof(T) > 1) {
        
        int64_t __tenjin_tmp_in_u = c;
        __tenjin_tmp_in_u[1] = c+1;
        __tenjin_tmp_in_u[
        T __tenjin_tmp_out_u;
        tenjin_char_to_T(__tenjin_tmp_in_u, &__tenjin_tmp_out_u);3] = c-3;
        x = __tenjin_tmp_out_u;
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
