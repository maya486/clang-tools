[Debug] Traversing Function Body for union accesses
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
[Debug] Owner variable: in
[Debug] Access type: read
[Debug] Done traversing Function Body for union accesses
  Variable: in (2 accesses)
    Field: flt | WRITE | at /home/mrebholz/clang-tools/tests/conversion/in/simple1.cpp:10
    Field: num | READ | at /home/mrebholz/clang-tools/tests/conversion/in/simple1.cpp:11
[Debug] counts: writes_a=1 reads_a=0 writes_b=0 reads_b=1
[Debug] Found enclosing CompoundStmt for: in.flt
[Debug] CompoundStmt: {
    union (unnamed union at /home/mrebholz/clang-tools/tests/conversion/in/simple1.cpp:5:5) in;
    uint32_t n, j;
    in.flt = flt;
    return in.num;
}

=== Parent Chain ===
[Debug] Found enclosing CompoundStmt for: in.num
[Debug] CompoundStmt: {
    union (unnamed union at /home/mrebholz/clang-tools/tests/conversion/in/simple1.cpp:5:5) in;
    uint32_t n, j;
    in.flt = flt;
    return in.num;
}

=== Parent Chain ===
Rewrote union pun for variable 'in' using tenjin_float_to_uint32_t with tmps __tenjin_tmp_in_in __tenjin_tmp_out_in
[Debug] Traversing Function Body for union accesses
[Debug] Done traversing Function Body for union accesses
[Debug] Traversing Function Body for union accesses
[Debug] Done traversing Function Body for union accesses
=== Rewritten File: /home/mrebholz/clang-tools/tests/conversion/in/simple1.cpp ===
#include <cstring>
#include <stdint.h>
#include <iostream>

void tenjin_float_to_uint32_t(float x, uint32_t *out) {
    memcpy(out, &x, 4);
}

uint32_t test(float flt) {
    
    uint32_t n, j;
    float __tenjin_tmp_in_in = flt;
    uint32_t __tenjin_tmp_out_in;
    tenjin_float_to_uint32_t(__tenjin_tmp_in_in, &__tenjin_tmp_out_in);
    return __tenjin_tmp_out_in;
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
