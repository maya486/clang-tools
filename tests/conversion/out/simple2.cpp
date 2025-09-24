[Debug] Traversing Function Body for union accesses
[Debug] Visiting MemberExpr: num
[Debug] Parent is union: (anonymous)
[Debug] Union passed int/float sized check
[Debug] Owner variable: in
[Debug] Access type: write
[Debug] Visiting MemberExpr: flt
[Debug] Parent is union: (anonymous)
[Debug] Union passed int/float sized check
[Debug] Owner variable: in
[Debug] Access type: read
[Debug] Done traversing Function Body for union accesses
[Debug] Function: simple
[Debug] Collected MemberExpr accesses:
  Variable: in (2 accesses)
    Field: num | WRITE | at /home/mrebholz/clang-tools/tests/conversion/in/simple2.cpp:10
    Field: flt | READ | at /home/mrebholz/clang-tools/tests/conversion/in/simple2.cpp:11
[Debug] Variable in has writes=1, reads=1
[Debug] Assignment stmt: in.num = num
[Debug] Return stmt: return in.flt
Rewrote union pun in function 'simple' using tenjin_u32_to_f32
[Debug] Traversing Function Body for union accesses
[Debug] Done traversing Function Body for union accesses
[Debug] Function: (unnamed union at /home/mrebholz/clang-tools/tests/conversion/in/simple2.cpp:5:5)
[Debug] Collected MemberExpr accesses:
=== Rewritten File: /home/mrebholz/clang-tools/tests/conversion/in/simple2.cpp ===
float tenjin_u32_to_f32(uint32_t x) {
    float y;
    memcpy(&y, &x, sizeof y);
    return y;
}


#include <stdint.h>

float simple(uint32_t num) {
    return tenjin_u32_to_f32(num);
}
