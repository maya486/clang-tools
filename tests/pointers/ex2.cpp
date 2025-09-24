#include <stdio.h>

#define println(...) printf(__VA_ARGS__)

typedef struct {
    int f;
} foo;

void bar(foo* p) {
    printf("Inside bar: foo field f = %d\n", p->f);
}

void example_2(foo* p) {
  for (int i = 0; i < 2; ++i) { // loop to use p multiple times
    println("foo %d field f: %d\n", i, p->f); // Looks like p is a singleton pointer to foo
    ++p; // nope! p is an array, with implicit index
    bar(p); // In Rust, should this pass an object reference? a slice? A Vec plus index?
  }
}
