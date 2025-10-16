// OLD
static uint32_t get_bits(bs_t *bs, int n) {
    uint32_t next, cache = 0, s = bs->pos & 7;
    int shl = n + s;
    const uint8_t *p = bs->buf + (bs->pos >> 3);
    if ((bs->pos += n) > bs->limit)
        return 0;
    next = *p++ & (255 >> s);
    while ((shl -= 8) > 0) {
        cache |= next << shl;
        next = *p++;
    }
    return cache | (next >> -shl);
}

// NEW
// bs does not get changed because it is a constant pointer
// p does get changed because it is implicitly an array
static uint32_t get_bits(bs_t *bs, int n) {
    uint32_t next, cache = 0, s = bs->pos & 7;
    int shl = n + s;
    const uint8_t *p = bs->buf + (bs->pos >> 3);
    int p_idx = 0; // new
    if ((bs->pos += n) > bs->limit)
        return 0;
    next = p[p_idx++] & (255 >> s); // changed
    while ((shl -= 8) > 0) {
        cache |= next << shl;
        next = p[p_idx++]; // changed
    }
    return cache | (next >> -shl);
}

// OLD
void example_2(foo *p) {
    for (int i = 0; i < 2; ++i) {                 // loop to use p multiple times
        println("foo %d field f: %d\n", i, p->f); // Looks like p is a singleton pointer to foo
        ++p;                                      // nope! p is an array, with implicit index
        bar(p); // In Rust, should this pass an object reference? a slice? A Vec plus index?
    }
}

// NEW 
// p is incremented so is an implicit vector of foo. Therefore, needs to be converted to ptr + idx + len when needed
void example_2(foo *p) {
    int p_idx = 0;
    for (int i = 0; i < 2; ++i) {                 // loop to use p multiple times
        println("foo %d field f: %d\n", i, p[p_idx].f); // Looks like p is a singleton pointer to foo
        ++p_idx;                                      // nope! p is an array, with implicit index
        bar(p[p_idx]); // In Rust, should this pass an object reference? a slice? A Vec plus index?
    }
}

// NEW + LEN
// p is incremented so is an implicit vector of foo. Therefore, needs to be converted to ptr + idx + len when needed
void example_2(foo *p, int p_len) {
    int p_idx = 0;
    for (int i = 0; i < 2; ++i) {                 // loop to use p multiple times
        println("foo %d field f: %d\n", i, p[p_idx].f); // Looks like p is a singleton pointer to foo
        ++p_idx;                                      // nope! p is an array, with implicit index
        bar(p[p_idx], p_len-p_idx); // In Rust, should this pass an object reference? a slice? A Vec plus index?
    }
}

// General Observations
// - if a pointer p is only used locally and is constant, no need to add explicity indexing
// - the only exception to a mutated pointer not having explicit indexing is if it is set arbitrarily 
//     to a new address
// - if a pointer is incremented or decremented relative to its current value it is inferred as an array and should have explicit indexing
//
//
// Questions For Ben
// - If there is a pointer that is an implicitly indexed array, it should be converted to a slice in Rust right? and if this is the case, should the c code reveal that length as well? What if it isn't available?
// - Question from Jeff: If in memory you have an array of pointers (like a pointer to a pointer sort of situation) does C2Rust allow for changing what is in memory?
// - If a pointer is passed to a callee would a first step be to assume that the the callee is non ownership assuming, non-pointer mutating method? Should these methods be investigated to see if they treat a pointer as a single element or a whole array and then maybe pass then length or something accordingly?
// - Anymore particular examples?
