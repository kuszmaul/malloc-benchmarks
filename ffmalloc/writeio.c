#include <stdint.h>
#include <unistd.h>

#include "writeio.h"

const int STDERR = 2;

void ewritec(char c) {
  // If this write fails, there's really not much to do, since we typicaly use
  // this function while printing an assertion failure.
  int r __attribute__((unused)) = write(STDERR, &c, 1);
}

void ewritenl(void) {
  ewritec('\n');
}

void ewrites(const char *str) {
  char c;
  while ((c=*str++)) {
    ewritec(c);
  }
}

void ewriteux(unsigned long v) {
  ewrites("0x");
  for (size_t i = 0; i < 16; i++) {
    size_t nibble = (v >> (4*(15-i))) & 0xf;
    ewritec((nibble < 10) ? nibble + '0' : nibble + 'a' - 10);
  }
}

static void ewriteul0(unsigned long v) {
  if (v == 0) return;
  ewriteul0(v / 10);
  ewritec((v % 10) + '0');
}

void ewriteul(unsigned long v) {
  if (v == 0) {
    ewritec('0');
  } else {
    ewriteul0(v);
  }
}

void ewritep(void*p) {
  if (p == NULL) {
    ewrites("(nil)");
  } else {
    uintptr_t v = (uintptr_t)p;
    ewriteux(v);
  }
}

void ewritespaces(size_t n) {
  for (size_t i = 0; i < n; i++) {
    ewritec(' ');
  }
}
