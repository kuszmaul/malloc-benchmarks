#ifndef WRITEIO_H
#define WRITEIO_H
#include <stddef.h>

void writec(int fd, char c);

void writes(int fd, char *str);
// Effect: Write `str` to `fd`.

void writeul(int fd, unsigned long v);
// Effect: Write `v` in decimal to `fd`, as for printf("%lu", v);

void writeux(int fd, unsigned long v);
// Effect: Write `v` in hex to `fd`, as for printf("%lx", v);

void writep(int fd, void*p);
// Effect: Write `p` in hex to `fd`, as for printf("%p", p);

void writespaces(int fd, size_t n);
// Effect: Write `n` spaces to `fd`.
#endif
