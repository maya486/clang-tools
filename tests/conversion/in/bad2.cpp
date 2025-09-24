//union U { int i; float f; };

void foo() {
    union U { int i; float f; };
    union U u1, u2;
    u1.i = 42;
    float x = u2.f;   // separate variable!
}
