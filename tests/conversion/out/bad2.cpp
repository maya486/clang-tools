[Debug] Traversing Function Body for union accesses
[Debug] Visiting MemberExpr: i
[Debug] Parent is union: U
[Debug] Checking Union
[Debug] Union a
[Debug] Union b
[Debug] Union c
[Debug] Union passed int/float sized check
[Debug] Owner variable: u1
[Debug] Access type: write
[Debug] Visiting MemberExpr: f
[Debug] Parent is union: U
[Debug] Checking Union
[Debug] Union a
[Debug] Union b
[Debug] Union c
[Debug] Union passed int/float sized check
[Debug] Owner variable: u2
[Debug] Access type: read
[Debug] Done traversing Function Body for union accesses
[Debug] Function: foo
[Debug] Collected MemberExpr accesses:
  Variable: u1 (1 accesses)
    Field: i | WRITE | at /home/mrebholz/clang-tools/tests/conversion/in/bad2.cpp:6
[Debug] counts: writes_f=0 reads_f=0 writes_i=1 reads_i=0
[Debug] Not a supported access pattern; skipping
  Variable: u2 (1 accesses)
    Field: f | READ | at /home/mrebholz/clang-tools/tests/conversion/in/bad2.cpp:7
[Debug] counts: writes_f=0 reads_f=1 writes_i=0 reads_i=0
[Debug] Not a supported access pattern; skipping
[Debug] Traversing Function Body for union accesses
[Debug] Done traversing Function Body for union accesses
[Debug] Function: U
[Debug] Collected MemberExpr accesses:
=== Rewritten File: /home/mrebholz/clang-tools/tests/conversion/in/bad2.cpp ===
//union U { int i; float f; };

void foo() {
    union U { int i; float f; };
    union U u1, u2;
    u1.i = 42;
    float x = u2.f;   // separate variable!
}
