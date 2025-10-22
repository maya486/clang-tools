#include <cstring>
#include <iostream>
#include <stdint.h>

typedef struct {
    uint32_t __l;
    uint32_t __h;
} tenjin_struct_94825943349392;
void tenjin_uint64_t_to_tenjin_struct_94825943349392(uint64_t x, tenjin_struct_94825943349392 *out) {
    memcpy(out, &x, 8);
}

uint32_t test(uint64_t n) {
    n <<= 25;
    uint64_t __tenjin_tmp_in___x;
tenjin_struct_94825943349392 __tenjin_tmp_out___x;

    __tenjin_tmp_in___x = n;
    tenjin_uint64_t_to_tenjin_struct_94825943349392(__tenjin_tmp_in___x, &__tenjin_tmp_out___x);
    return __tenjin_tmp_out___x.__h  * __tenjin_tmp_out___x.__l;
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
