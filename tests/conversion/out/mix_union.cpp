[Debug] Traversing Function Body for union accesses
[Debug] Visiting MemberExpr: num
[Debug] Parent is union: [Debug] Checking Union
[Debug] Union a
[Debug] Union passed 2 fields, same size check
[Debug] Owner variable: in2
[Debug] Access type: write
[Debug] Visiting MemberExpr: flt
[Debug] Parent is union: [Debug] Checking Union
[Debug] Union a
[Debug] Union passed 2 fields, same size check
[Debug] Owner variable: in
[Debug] Access type: write
[Debug] Visiting MemberExpr: num
[Debug] Parent is union: [Debug] Checking Union
[Debug] Union a
[Debug] Union passed 2 fields, same size check
[Debug] Owner variable: in2
[Debug] Access type: read
[Debug] Done traversing Function Body for union accesses
  Variable: in (1 accesses)
    Field: flt | WRITE | at /home/mrebholz/clang-tools/tests/conversion/in/mix_union.cpp:15
[Debug] counts: writes_a=1 reads_a=0 writes_b=0 reads_b=0
[Debug] Not a supported access pattern; skipping
  Variable: in2 (2 accesses)
    Field: num | WRITE | at /home/mrebholz/clang-tools/tests/conversion/in/mix_union.cpp:13
    Field: num | READ | at /home/mrebholz/clang-tools/tests/conversion/in/mix_union.cpp:16
[Debug] counts: writes_a=0 reads_a=0 writes_b=1 reads_b=1
[Debug] Not a supported access pattern; skipping
[Debug] Traversing Function Body for union accesses
[Debug] Done traversing Function Body for union accesses
[Debug] Traversing Function Body for union accesses
[Debug] Done traversing Function Body for union accesses
[Debug] Traversing Function Body for union accesses
[Debug] Done traversing Function Body for union accesses
=== Rewritten File: /home/mrebholz/clang-tools/tests/conversion/in/mix_union.cpp ===
#include <stdint.h>
#include <iostream>

uint32_t test(float flt) {
    union {
        float flt;
        uint32_t num;
    } in;
    union {
        float flt;
        uint32_t num;
    } in2;
    in2.num = 0;
    uint32_t n, j;
    in.flt = flt;
    return in2.num;
}


int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <float>" << std::endl;
        return 1;
    }
    float input = std::stof(argv[1]);
    std::cout << test(input) << std::endl;
    return 0;
}
