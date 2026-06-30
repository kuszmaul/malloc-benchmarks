#ifndef WRITEIO_H
#define WRITEIO_H
#include <stddef.h>

void ewritec(char c);
// Effect: Write `c` to standard error.

void ewritenl(void);
// Effect: Write `\n` to standard error.

void ewrites(const char *str);
// Effect: Write `str` to standard error.

void ewriteul(unsigned long v);
// Effect: Write `v` in decimal to standard error, as if by `printf(stderr,
// "%lu", v)` (but to stderr and directly with `write`).

void ewriteux(unsigned long v);
// Effect: Write `v` in hex to standard error, as if by `printf("%lx", v)` (but
// to stderr and directly with `write`).

void ewritep(void*p);
// Effect: Write `p` in hex to standard error, as if by `printf("%p", p)` (but
// to stderr and directly with `write`).

void ewritespaces(size_t n);
// Effect: Write `n` spaces to stderr.

#define VASSERT(a) ({         \
    if (!(a)) {               \
      ewrites("Failure at "); \
      ewrites(__FILE__);      \
      ewritec(':');           \
      ewriteul(__LINE__);     \
      ewritenl();             \
      return false;           \
    }                         \
})

void my_abort(void);
// Effect: call abort().
//
// Rationale: Since the abort is called in a macro, iwyu gets confused.  So
// instead we'll call my_abort and define it in writeio.c.

#define IGNORE(x) ((x)=(x))

#ifdef NOASSERT
#define ASSERT(a) ((void)(0))
#else
#define ASSERT(a) ({          \
    if (!(a)) {               \
      ewrites("Failure at "); \
      ewrites(__FILE__);      \
      ewritec(':');           \
      ewriteul(__LINE__);     \
      ewritenl();             \
      my_abort();             \
    }                         \
})
#endif

#endif
