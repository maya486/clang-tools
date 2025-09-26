[Debug] Traversing Function Body for union accesses
[Debug] Visiting MemberExpr: flt
[Debug] Parent is union: (anonymous)
[Debug] Checking Union
[Debug] Union a
[Debug] Union b
[Debug] Union c
[Debug] Union passed int/float sized check
[Debug] Owner variable: in
[Debug] Access type: write
[Debug] Visiting MemberExpr: num
[Debug] Parent is union: (anonymous)
[Debug] Checking Union
[Debug] Union a
[Debug] Union b
[Debug] Union c
[Debug] Union passed int/float sized check
[Debug] Owner variable: in
[Debug] Access type: read
[Debug] Done traversing Function Body for union accesses
[Debug] Function: simple
[Debug] Collected MemberExpr accesses:
  Variable: in (2 accesses)
    Field: flt | WRITE | at /home/mrebholz/clang-tools/tests/conversion/in/simple1.cpp:9
    Field: num | READ | at /home/mrebholz/clang-tools/tests/conversion/in/simple1.cpp:10
[Debug] Variable in has writes=1, reads=1
[Debug] Assignment stmt: in.flt = flt
Rewrote union pun in function 'simple' using tenjin_f32_to_u32
[Debug] Traversing Function Body for union accesses
[Debug] Done traversing Function Body for union accesses
[Debug] Function: (unnamed union at /home/mrebholz/clang-tools/tests/conversion/in/simple1.cpp:4:5)
[Debug] Collected MemberExpr accesses:
=== Rewritten File: /home/mrebholz/clang-tools/tests/conversion/in/simple1.cpp ===
#include <stdint.h>

uint32_t tenjin_f32_to_u32(float x) {
    uint32_t y;
    memcpy(&y, &x, sizeof y);
    return y;
}

uint32_t simple(float flt) {
    
    uint32_t n, j;
    
    return tenjin_f32_to_u32(flt);
}
