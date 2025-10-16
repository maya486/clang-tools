[Debug] Traversing Function Body for union accesses
[Debug] Visiting MemberExpr: num
[Debug] Parent is union: [Debug] Checking Union
[Debug] Union a
[Debug] Union passed 2 fields, same size check
[Debug] Owner variable: in
[Debug] Access type: write
[Debug] Visiting MemberExpr: flt
[Debug] Parent is union: [Debug] Checking Union
[Debug] Union a
[Debug] Union passed 2 fields, same size check
[Debug] Owner variable: in
[Debug] Access type: read
[Debug] Done traversing Function Body for union accesses
  Variable: in (2 accesses)
    Field: num | WRITE | at /home/mrebholz/clang-tools/tests/conversion/in/simple2.cpp:10
    Field: flt | READ | at /home/mrebholz/clang-tools/tests/conversion/in/simple2.cpp:11
[Debug] counts: writes_a=0 reads_a=1 writes_b=1 reads_b=0
[Debug] Found enclosing CompoundStmt for: in.num
[Debug] CompoundStmt: {
    union (unnamed union at /home/mrebholz/clang-tools/tests/conversion/in/simple2.cpp:5:5) in;
    uint32_t n, j;
    in.num = num;
    return in.flt;
}

=== Parent Chain ===
[Debug] Found enclosing CompoundStmt for: in.flt
[Debug] CompoundStmt: {
    union (unnamed union at /home/mrebholz/clang-tools/tests/conversion/in/simple2.cpp:5:5) in;
    uint32_t n, j;
    in.num = num;
    return in.flt;
}

=== Parent Chain ===
Rewrote union pun for variable 'in' using tenjin_uint32_t_to_float with tmps __tenjin_tmp_in_in __tenjin_tmp_out_in
[Debug] Traversing Function Body for union accesses
[Debug] Done traversing Function Body for union accesses
[Debug] Traversing Function Body for union accesses
[Debug] Done traversing Function Body for union accesses
=== Rewritten File: /home/mrebholz/clang-tools/tests/conversion/in/simple2.cpp ===
#include <cstring>
#include <iostream>
#include <stdint.h>

void tenjin_uint32_t_to_float(uint32_t x, float *out) {
    memcpy(out, &x, 4);
}

float test(uint32_t num) {
    
    uint32_t n, j;
    uint32_t __tenjin_tmp_in_in = num;
    float __tenjin_tmp_out_in;
    tenjin_uint32_t_to_float(__tenjin_tmp_in_in, &__tenjin_tmp_out_in);
    return __tenjin_tmp_out_in;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <float>" << std::endl;
        return 1;
    }
    float input = std::stof(argv[1]);
    std::cout << test((uint32_t)input) << std::endl;
    return 0;
}
