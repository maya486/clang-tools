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
[Debug] Variable in has writes=2, reads=2
[Debug] Assignment stmt: in.flt = in.flt + 3
[Debug] Traversing Function Body for union accesses
[Debug] Done traversing Function Body for union accesses
[Debug] Function: (unnamed union at /home/mrebholz/clang-tools/tests/conversion/in/simple3.cpp:4:5)
[Debug] Collected MemberExpr accesses:
=== Rewritten File: /home/mrebholz/clang-tools/tests/conversion/in/simple3.cpp ===
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

