[Debug] Traversing Function Body for union accesses
[Debug] Visiting MemberExpr: i
[Debug] Parent is union: U
[Debug] Checking Union
[Debug] Union a
[Debug] Union passed 2 fields, same size check
[Debug] Owner variable: u1
[Debug] Access type: write
[Debug] Visiting MemberExpr: f
[Debug] Parent is union: U
[Debug] Checking Union
[Debug] Union a
[Debug] Union passed 2 fields, same size check
[Debug] Owner variable: u1
[Debug] Access type: write
[Debug] Visiting MemberExpr: i
[Debug] Parent is union: U
[Debug] Checking Union
[Debug] Union a
[Debug] Union passed 2 fields, same size check
[Debug] Owner variable: u2
[Debug] Access type: write
[Debug] Visiting MemberExpr: f
[Debug] Parent is union: U
[Debug] Checking Union
[Debug] Union a
[Debug] Union passed 2 fields, same size check
[Debug] Owner variable: u2
[Debug] Access type: write
[Debug] Visiting MemberExpr: i
[Debug] Parent is union: U
[Debug] Checking Union
[Debug] Union a
[Debug] Union passed 2 fields, same size check
[Debug] Owner variable: u1
[Debug] Access type: write
[Debug] Visiting MemberExpr: f
[Debug] Parent is union: U
[Debug] Checking Union
[Debug] Union a
[Debug] Union passed 2 fields, same size check
[Debug] Owner variable: u2
[Debug] Access type: read
[Debug] Done traversing Function Body for union accesses
  Variable: u1 (3 accesses)
    Field: i | WRITE | at /home/mrebholz/clang-tools/tests/conversion/in/bad2.cpp:6
    Field: f | WRITE | at /home/mrebholz/clang-tools/tests/conversion/in/bad2.cpp:7
    Field: i | WRITE | at /home/mrebholz/clang-tools/tests/conversion/in/bad2.cpp:10
[Debug] counts: writes_a=2 reads_a=0 writes_b=1 reads_b=0
[Debug] Not a supported access pattern; skipping
  Variable: u2 (3 accesses)
    Field: i | WRITE | at /home/mrebholz/clang-tools/tests/conversion/in/bad2.cpp:8
    Field: f | WRITE | at /home/mrebholz/clang-tools/tests/conversion/in/bad2.cpp:9
    Field: f | READ | at /home/mrebholz/clang-tools/tests/conversion/in/bad2.cpp:11
[Debug] counts: writes_a=1 reads_a=0 writes_b=1 reads_b=1
[Debug] Not a supported access pattern; skipping
[Debug] Traversing Function Body for union accesses
[Debug] Done traversing Function Body for union accesses
[Debug] Traversing Function Body for union accesses
[Debug] Done traversing Function Body for union accesses
=== Rewritten File: /home/mrebholz/clang-tools/tests/conversion/in/bad2.cpp ===
#include <iostream>

float test(float a) {
    union U { int i; float f; };
    union U u1, u2;
    u1.i = 0;
    u1.f = 0;
    u2.i = 0;
    u2.f = 0;
    u1.i = int(a);
    float x = u2.f;   // separate variable!
    return x;
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

