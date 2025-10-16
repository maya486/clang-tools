[Debug] Traversing Function Body for union accesses
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
[Debug] Visiting MemberExpr: x
[Debug] Parent is union: [Debug] Checking Union
[Debug] Union a
[Debug] Union passed 2 fields, same size check
[Debug] Owner variable: u
[Debug] Access type: read
[Debug] Done traversing Function Body for union accesses
  Variable: u (4 accesses)
    Field: b | READ | at /home/mrebholz/clang-tools/tests/conversion/in/char_arr_simple2.cpp:13
    Field: b | READ | at /home/mrebholz/clang-tools/tests/conversion/in/char_arr_simple2.cpp:14
    Field: b | READ | at /home/mrebholz/clang-tools/tests/conversion/in/char_arr_simple2.cpp:15
    Field: x | READ | at /home/mrebholz/clang-tools/tests/conversion/in/char_arr_simple2.cpp:16
[Debug] counts: writes_a=0 reads_a=1 writes_b=0 reads_b=3
[Debug] Found enclosing CompoundStmt for: u.b
[Debug] CompoundStmt: {
    union (unnamed union at /home/mrebholz/clang-tools/tests/conversion/in/char_arr_simple2.cpp:9:9) u;
    u.b[0] = c;
    u.b[1] = c + 1;
    u.b[3] = c - 3;
    x = u.x;
    return x + 3;
}

=== Parent Chain ===
[Debug] Found enclosing CompoundStmt for: u.b
[Debug] CompoundStmt: {
    union (unnamed union at /home/mrebholz/clang-tools/tests/conversion/in/char_arr_simple2.cpp:9:9) u;
    u.b[0] = c;
    u.b[1] = c + 1;
    u.b[3] = c - 3;
    x = u.x;
    return x + 3;
}

=== Parent Chain ===
[Debug] Found enclosing CompoundStmt for: u.b
[Debug] CompoundStmt: {
    union (unnamed union at /home/mrebholz/clang-tools/tests/conversion/in/char_arr_simple2.cpp:9:9) u;
    u.b[0] = c;
    u.b[1] = c + 1;
    u.b[3] = c - 3;
    x = u.x;
    return x + 3;
}

=== Parent Chain ===
[Debug] Found enclosing CompoundStmt for: u.x
[Debug] CompoundStmt: {
    union (unnamed union at /home/mrebholz/clang-tools/tests/conversion/in/char_arr_simple2.cpp:9:9) u;
    u.b[0] = c;
    u.b[1] = c + 1;
    u.b[3] = c - 3;
    x = u.x;
    return x + 3;
}

=== Parent Chain ===
[Debug] No source write before destination read; skipping
[Debug] Traversing Function Body for union accesses
[Debug] Done traversing Function Body for union accesses
[Debug] Traversing Function Body for union accesses
[Debug] Done traversing Function Body for union accesses
=== Rewritten File: /home/mrebholz/clang-tools/tests/conversion/in/char_arr_simple2.cpp ===
#include <iostream>
#include <stdint.h>

typedef long T;

T test(char c) {
    T x = 300000;
    if (sizeof(T) > 1) {
        union {
            T x;
            char b[sizeof(T)];
        } u;
        u.b[0] = c;
        u.b[1] = c+1;
        u.b[3] = c-3;
        x = u.x;
        return x+3;
    }
    return -1;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <float>" << std::endl;
        return 1;
    }
    float input = std::stof(argv[1]);
    char c = (char)input;
    std::cout << test(c) << std::endl;
    return 0;
}
