#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "max.h"
#include "writeio.h"

__attribute__((visibility("default")))
size_t my_malloc_usable_size(void *p);

__attribute__((visibility("default")))
void *my_malloc(size_t n);

__attribute__((visibility("default")))
void my_free(void*p __attribute__((__unused__)));

__attribute__((visibility("default")))
void *my_calloc(size_t nmemb, size_t size);

__attribute__((visibility("default")))
void *my_realloc(void *p, size_t size);

__attribute__((visibility("default")))
int my_posix_memalign(void **memptr, size_t alignment, size_t size);

__attribute__((visibility("default")))
void *my_aligned_alloc(size_t alignment, size_t size);

void *data = NULL;
size_t free_index = 0;
size_t data_size = 0;
size_t last_sbrk_size = 0;

static void do_sbrk(size_t n) {
  // We generally can sbrk a huge amount of memory, but we cannot do it more
  // than a few gigabytes at a time.
  const size_t max_single_sbrk_size = 1<<24;
  if (0) { ewrites("do_sbrk("); ewriteul(n); ewrites(")\n"); }
  while (n > 0) {
    size_t m = min(max_single_sbrk_size, n);
    void *r = sbrk(m);
    if (0) { ewrites(" sbrk("); ewriteul(m); ewrites(")="); ewritep(r); ewritenl(); }
    if (r == (void*)-1) {
      if (0) ewrites("sbrk failed\n");
      abort();
    }
    data_size += m;
    if (0) { ewrites(" data_size="); ewriteul(data_size); ewritenl(); }
    n -= m;
  }
}

static void ensure_space(size_t n) {
  // Make sure there are n bytes free.
  if (n <= data_size - free_index) return;

  if (data == NULL) {
    data = sbrk(0);
    if (data == (void*)-1) {
      ewrites("sbrk(0) failed\n");
      abort();
    }
    free_index = 0;
  }
  last_sbrk_size = max(n, last_sbrk_size) + last_sbrk_size;
  last_sbrk_size += 4095;
  last_sbrk_size &= ~4095;
  do_sbrk(last_sbrk_size);

  if (n > data_size - free_index) {
    ewrites("data size not good\n");
    abort();
  }
}

__asm__(".symver my_malloc,__libc_malloc@GLIBC_2.2.5");
__asm__(".symver my_malloc,malloc@GLIBC_2.2.5");

static char empty[1];

__attribute__((visibility("default")))
void *my_malloc(size_t n) {
  if (0) { ewrites(__FUNCTION__); ewrites("("); ewriteul(n); ewrites(")\n"); }
  if (n == 0) {
    return empty;
  }
  // Round up to 8-alignment.
  n = n + 7;
  n &= ~7;
  ensure_space(n + 8);
  void *header = ((char*)data)+free_index;
  void *return_result = ((char*)header) + sizeof(size_t);
  free_index += n + 8;
  if (0) {
    ewrites(" free_index="); ewriteul(free_index); ewritenl();
    ewrites(" data_size ="); ewriteul(data_size); ewritenl();
    ewrites(" data      ="); ewritep(data); ewritenl();
    ewrites(__FUNCTION__); ewrites(" returns "); ewritep(return_result); ewritenl();
  }
  size_t header_content = n + 8;
  memcpy(header, &header_content, sizeof(size_t));
  if (0) { ewrites(__FUNCTION__); ewrites(" returns "); ewritep(return_result); ewritenl(); }
  return return_result;
}

__asm__(".symver my_free,__libc_free@GLIBC_2.2.5");
__asm__(".symver my_free,free@GLIBC_2.2.5");

__attribute__((visibility("default")))
void my_free(void*p __attribute__((__unused__))) {
  if (0) ewrites("my_free\n");
  // Do nothing
}

__asm__(".symver my_calloc,__libc_calloc@GLIBC_2.2.5");
__asm__(".symver my_calloc,calloc@GLIBC_2.2.5");

__attribute__((visibility("default")))
void *my_calloc(size_t nmemb, size_t size) {
  if (0) ewrites("calloc\n");
  void *p = my_malloc(nmemb * size);
  memset(p, 0, nmemb * size);
  return p;
}

__asm__(".symver my_realloc,__libc_realloc@GLIBC_2.2.5");
__asm__(".symver my_realloc,realloc@GLIBC_2.2.5");

__attribute__((visibility("default")))
void *my_realloc(void *p __attribute__((unused)), size_t size __attribute__((unused))) {
  if (0) ewrites("my_realloc\n");
  if (size <= my_malloc_usable_size(p)) {
    if (0) ewrites("my_realloc done1\n");
    return p;
  }
  void *q = my_malloc(size);
  if (my_malloc_usable_size(q) < size) {
    if (0) ewrites("my_realloc failed\n");
    abort();
  }
  memcpy(q, p, my_malloc_usable_size(p));
  if (0) ewrites("my_realloc done2\n");
  return q;
}

__attribute__((visibility("default")))
void *reallocarray(void *p __attribute__((unused)), size_t nmemb __attribute__((unused)), size_t size __attribute__((unused))) {
  ewrites("reallocarray\n");
  abort();
}

__asm__(".symver my_posix_memalign,posix_memalign@GLIBC_2.2.5");

__attribute__((visibility("default")))
int my_posix_memalign(void **memptr __attribute__((unused)), size_t alignment __attribute__((unused)), size_t size __attribute__((unused))) {
  ewrites("posix_memalign\n");
  abort();
}

__asm__(".symver my_aligned_alloc,aligned_alloc@GLIBC_2.2.5");

__attribute__((visibility("default")))
void *my_aligned_alloc(size_t alignment __attribute__((unused)), size_t size __attribute__((unused))) {
  ewrites("aligned_alloc\n");
  abort();
}

__attribute__((visibility("default")))
void *valloc(size_t size __attribute__((unused))) {
  ewrites("valloc\n");
  abort();
}

__asm__(".symver my_malloc_usable_size,__malloc_usable_size@GLIBC_2.2.5");
__asm__(".symver my_malloc_usable_size,malloc_usable_size@GLIBC_2.2.5");

__attribute__((visibility("default")))
size_t my_malloc_usable_size(void *p) {
  if (0) ewrites("malloc_usable_size\n");
  if (p == NULL) return 0;
  size_t r = *((size_t*)(((char*)p)-8)) - 8;
  if (0) ewrites("malloc_usable_size done\n");
  return r;
}
