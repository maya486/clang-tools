#include <stdint.h>

uint32_t simple(float flt) {
    union {
        float flt;
        uint32_t num;
    } in;
    uint32_t n, j;
    in.flt = flt;
    in.flt = in.flt + 3;
    return in.num;
}

