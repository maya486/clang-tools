
#include <stdint.h>

float simple(uint32_t num) {
    union {
        float flt;
        uint32_t num;
    } in;
    uint32_t n, j;
    in.num = num;
    return in.flt;
}
