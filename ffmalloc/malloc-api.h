#ifndef MALLOC_API_H
#define MALLOC_API_H
#include <stddef.h>

__attribute__((visibility("default")))
void *__libc_malloc(size_t size);

__attribute__((visibility("default")))
void __libc_free(void *p);

__attribute__((visibility("default")))
void *__libc_calloc(size_t nmemb, size_t size);

__attribute__((visibility("default")))
void *__libc_realloc(void *p, size_t size);

__attribute__((visibility("default")))
void *__libc_memalign(size_t alignment, size_t size);

__attribute__((visibility("default")))
void *__libc_reallocarray(void *p, size_t nmemb, size_t size);

__attribute__((visibility("default")))
void *__libc_valloc(size_t size);

__attribute__((visibility("default")))
void *__libc_pvalloc(size_t size);

__attribute__((visibility("default")))
int __posix_memalign(void** memptr, size_t alignment, size_t size);

__attribute__((visibility("default")))
size_t __malloc_usable_size (void *m);
#endif
