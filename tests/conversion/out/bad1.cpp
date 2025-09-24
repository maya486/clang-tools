[Debug] Traversing Function Body for union accesses
[Debug] Visiting MemberExpr: flt
[Debug] Parent is union: (anonymous)
[Debug] Union passed int/float sized check
[Debug] Owner variable: in
[Debug] Access type: write
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
[Debug] Function: bad1
[Debug] Collected MemberExpr accesses:
  Variable: in (3 accesses)
    Field: flt | WRITE | at /home/mrebholz/clang-tools/tests/conversion/in/bad1.cpp:9
    Field: num | WRITE | at /home/mrebholz/clang-tools/tests/conversion/in/bad1.cpp:10
    Field: flt | READ | at /home/mrebholz/clang-tools/tests/conversion/in/bad1.cpp:11
[Debug] Variable in has writes=2, reads=1
[Debug] Assignment stmt: in.num = 2
[Debug] Return stmt: <null>
[Debug] Traversing Function Body for union accesses
[Debug] Done traversing Function Body for union accesses
[Debug] Function: (unnamed union at /home/mrebholz/clang-tools/tests/conversion/in/bad1.cpp:4:5)
[Debug] Collected MemberExpr accesses:
=== Rewritten File: /home/mrebholz/clang-tools/tests/conversion/in/bad1.cpp ===
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
