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
[Debug] Visiting MemberExpr: flt
[Debug] Parent is union: (anonymous)
[Debug] Checking Union
[Debug] Union a
[Debug] Union b
[Debug] Union c
[Debug] Union passed int/float sized check
[Debug] Owner variable: in
[Debug] Access type: write
[Debug] Visiting MemberExpr: flt
[Debug] Parent is union: (anonymous)
[Debug] Checking Union
[Debug] Union a
[Debug] Union b
[Debug] Union c
[Debug] Union passed int/float sized check
[Debug] Owner variable: in
[Debug] Access type: read
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
  Variable: in (4 accesses)
    Field: flt | WRITE | at /home/mrebholz/clang-tools/tests/conversion/in/simple3.cpp:9
    Field: flt | WRITE | at /home/mrebholz/clang-tools/tests/conversion/in/simple3.cpp:10
    Field: flt | READ | at /home/mrebholz/clang-tools/tests/conversion/in/simple3.cpp:10
    Field: num | READ | at /home/mrebholz/clang-tools/tests/conversion/in/simple3.cpp:11
[Debug] counts: writes_f=2 reads_f=1 writes_i=0 reads_i=1
Rewrote union pun for variable 'in' using tenjin_f32_to_u32 with tmp __tenjin_tmp_in
[Debug] Traversing Function Body for union accesses
[Debug] Done traversing Function Body for union accesses
[Debug] Function: (unnamed union at /home/mrebholz/clang-tools/tests/conversion/in/simple3.cpp:4:5)
[Debug] Collected MemberExpr accesses:
=== Rewritten File: /home/mrebholz/clang-tools/tests/conversion/in/simple3.cpp ===
#include <stdint.h>

uint32_t tenjin_f32_to_u32(float x) {
    uint32_t y;
    memcpy(&y, &x, sizeof y);
    return y;
}

uint32_t simple(float flt) {
    
    uint32_t n, j;
    float __tenjin_tmp_in = flt;
    __tenjin_tmp_in = __tenjin_tmp_in + 3;
    return tenjin_f32_to_u32(__tenjin_tmp_in);
}

