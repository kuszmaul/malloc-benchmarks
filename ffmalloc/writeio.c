#include <stdint.h>
#include <unistd.h>

#include "writeio.h"

void writec(int fd, char c) {
  // If this write fails, there's really not much to do, since we typicaly use
  // this function while printing an assertion failure.
  int r __attribute__((unused)) = write(fd, &c, 1);
}

void writes(int fd, char *str) {
  char c;
  while ((c=*str++)) {
    writec(fd, c);
  }
}

void writeux(int fd, unsigned long v) {
  writes(fd, "0x");
  for (size_t i = 0; i < 16; i++) {
    size_t nibble = (v >> (4*(15-i))) & 0xf;
    writec(fd, (nibble < 10) ? nibble + '0' : nibble + 'a' - 10);
  }
}

static void writeul0(int fd, unsigned long v) {
  if (v == 0) return;
  writeul0(fd, v / 10);
  writec(fd, (v % 10) + '0');
}

void writeul(int fd, unsigned long v) {
  if (v == 0) {
    writec(fd, '0');
  } else {
    writeul0(fd, v);
  }
}

void writep(int fd, void*p) {
  if (p == NULL) {
    writes(fd, "(nil)");
  } else {
    uintptr_t v = (uintptr_t)p;
    writeux(fd, v);
  }
}

void writespaces(int fd, size_t n) {
  for (size_t i = 0; i < n; i++) {
    writec(fd, ' ');
  }
}
