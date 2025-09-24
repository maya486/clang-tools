#include <stdint.h>

uint32_t bad1(float flt) {
    union {
        float flt;
        uint32_t num;
    } in;
    uint32_t n, j;
    in.flt = flt;
    in.num = 2;
    float a = in.flt;
    return uint32_t(a) + 10;
}
