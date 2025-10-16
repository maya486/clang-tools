[Debug] Traversing Function Body for union accesses
[Debug] Visiting MemberExpr: x
[Debug] Parent is union: [Debug] Checking Union
[Debug] Union a
[Debug] Union passed 2 fields, same size check
[Debug] Owner variable: u
[Debug] Access type: write
[Debug] Visiting MemberExpr: b
[Debug] Parent is union: [Debug] Checking Union
[Debug] Union a
[Debug] Union passed 2 fields, same size check
[Debug] Owner variable: u
[Debug] Access type: read
[Debug] Visiting MemberExpr: b
[Debug] Parent is union: [Debug] Checking Union
[Debug] Union a
[Debug] Union passed 2 fields, same size check
[Debug] Owner variable: u
[Debug] Access type: read
[Debug] Visiting MemberExpr: b
[Debug] Parent is union: [Debug] Checking Union
[Debug] Union a
[Debug] Union passed 2 fields, same size check
[Debug] Owner variable: u
[Debug] Access type: read
[Debug] Done traversing Function Body for union accesses
  Variable: u (4 accesses)
    Field: x | WRITE | at /home/mrebholz/clang-tools/tests/conversion/in/char_arr_simple1.cpp:13
    Field: b | READ | at /home/mrebholz/clang-tools/tests/conversion/in/char_arr_simple1.cpp:14
    Field: b | READ | at /home/mrebholz/clang-tools/tests/conversion/in/char_arr_simple1.cpp:14
    Field: b | READ | at /home/mrebholz/clang-tools/tests/conversion/in/char_arr_simple1.cpp:14
[Debug] counts: writes_a=1 reads_a=0 writes_b=0 reads_b=3
[Debug] Found enclosing CompoundStmt for: u.x
[Debug] CompoundStmt: {
    union (unnamed union at /home/mrebholz/clang-tools/tests/conversion/in/char_arr_simple1.cpp:9:9) u;
    u.x = x;
    return u.b[0] + u.b[1] + u.b[2];
}

=== Parent Chain ===
[Debug] Found enclosing CompoundStmt for: u.b
[Debug] CompoundStmt: {
    union (unnamed union at /home/mrebholz/clang-tools/tests/conversion/in/char_arr_simple1.cpp:9:9) u;
    u.x = x;
    return u.b[0] + u.b[1] + u.b[2];
}

=== Parent Chain ===
[Debug] Found enclosing CompoundStmt for: u.b
[Debug] CompoundStmt: {
    union (unnamed union at /home/mrebholz/clang-tools/tests/conversion/in/char_arr_simple1.cpp:9:9) u;
    u.x = x;
    return u.b[0] + u.b[1] + u.b[2];
}

=== Parent Chain ===
[Debug] Found enclosing CompoundStmt for: u.b
[Debug] CompoundStmt: {
    union (unnamed union at /home/mrebholz/clang-tools/tests/conversion/in/char_arr_simple1.cpp:9:9) u;
    u.x = x;
    return u.b[0] + u.b[1] + u.b[2];
}

=== Parent Chain ===
Rewrote union pun for variable 'u' using tenjin_T_to_char_8_ with tmps __tenjin_tmp_in_u __tenjin_tmp_out_u
[Debug] Traversing Function Body for union accesses
[Debug] Done traversing Function Body for union accesses
[Debug] Traversing Function Body for union accesses
[Debug] Done traversing Function Body for union accesses
=== Rewritten File: /home/mrebholz/clang-tools/tests/conversion/in/char_arr_simple1.cpp ===
#include <cstring>
#include <iostream>
#include <stdint.h>

typedef long T;

void tenjin_T_to_char_8_(T x, char *out) {
    memcpy(out, &x, 8);
}

T test(int c) {
    T x = 300000*c;
    if (sizeof(T) > 1) {
        
        int64_t __tenjin_tmp_in_u = x;
        char __tenjin_tmp_out_u[8];
        tenjin_T_to_char_8_(__tenjin_tmp_in_u, __tenjin_tmp_out_u);
        return __tenjin_tmp_out_u[0]+__tenjin_tmp_out_u[1]+__tenjin_tmp_out_u[2];
    }
    return -1;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <float>" << std::endl;
        return 1;
    }
    float input = std::stof(argv[1]);
    int c = (int)input;
    std::cout << test(c) << std::endl;
    return 0;
}
