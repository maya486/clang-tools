[Debug] Traversing Function Body for union accesses
[Debug] Visiting MemberExpr: num
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
[Debug] Done traversing Function Body for union accesses
[Debug] Function: simple
[Debug] Collected MemberExpr accesses:
  Variable: in (2 accesses)
    Field: num | WRITE | at /home/mrebholz/clang-tools/tests/conversion/in/simple2.cpp:10
    Field: flt | READ | at /home/mrebholz/clang-tools/tests/conversion/in/simple2.cpp:11
[Debug] counts: writes_f=0 reads_f=1 writes_i=1 reads_i=0
Rewrote union pun for variable 'in' using tenjin_u32_to_f32 with tmp __tenjin_tmp_in
[Debug] Traversing Function Body for union accesses
[Debug] Done traversing Function Body for union accesses
[Debug] Function: (unnamed union at /home/mrebholz/clang-tools/tests/conversion/in/simple2.cpp:5:5)
[Debug] Collected MemberExpr accesses:
=== Rewritten File: /home/mrebholz/clang-tools/tests/conversion/in/simple2.cpp ===

#include <stdint.h>

float tenjin_u32_to_f32(uint32_t x) {
    float y;
    memcpy(&y, &x, sizeof y);
    return y;
}

float simple(uint32_t num) {
    
    uint32_t n, j;
    uint32_t __tenjin_tmp_in = num;
    return tenjin_u32_to_f32(__tenjin_tmp_in);
}
