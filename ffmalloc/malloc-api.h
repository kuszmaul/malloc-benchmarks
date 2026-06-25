#ifndef MALLOC_API_H
#define MALLOC_API_H
#include <stddef.h>

__attribute__((visibility("default")))
void *my_malloc(size_t size);

__attribute__((visibility("default")))
void my_free(void *p);

__attribute__((visibility("default")))
void *my_calloc(size_t nmemb, size_t size);

__attribute__((visibility("default")))
void *my_realloc(void *p, size_t size);

__attribute__((visibility("default")))
void *my_memalign(size_t alignment, size_t size);

__attribute__((visibility("default")))
void *my_reallocarray(void *p, size_t nmemb, size_t size);

__attribute__((visibility("default")))
void *my_valloc(size_t size);

__attribute__((visibility("default")))
void *my_pvalloc(size_t size);

__attribute__((visibility("default")))
int my_posix_memalign(void** memptr, size_t alignment, size_t size);

__attribute__((visibility("default")))
size_t my_malloc_usable_size (void *m);
#endif
