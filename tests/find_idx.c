int __attribute__((noinline)) find_idx(char *buf, char *c) {
  for (int i = 0; buf[i] != '\0'; i++) {
    if (buf[i] == *c) {
      return i;
    }
  }

  return -1;
}

