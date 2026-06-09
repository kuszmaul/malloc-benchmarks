/* Worst case workload for best fit, from Robson 77 fig 2. */

#include <assert.h>
#include <malloc.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>  // Don't use stdio, since it allocates memory.
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "rss.h"

static size_t max(size_t a, size_t b) {
  return a < b ? b : a;
}
static void maxf(size_t *a, size_t b) {
  *a = max(*a, b);
}

// To run Robson's worst-case workload for best fit, we need to account for the
// overheads that the actually allocator uses.  Since the libc allocator
// (apparently a version of dlmalloc) uses 8-byte overheads and makes blocks
// 32-byte aligned, when Robson calls for allocating `n` words, we need to
// allocate n*32-8 bytes.

static size_t words_to_bytes(size_t w) {
    return (w * 32) - 8;
}
//static size_t bytes_to_words(size_t b) {
//  assert(b + 8 % 32 == 0);
//  return (b + 8) / 32;
//}

static void mywrite(int fd, const void *p, size_t n) {
  int r = write(fd, p, n);
  assert(r > 0 && (size_t)r == n);
}

static void writes(int fd, const char *str) {
  mywrite(fd, str, strlen(str));
}

enum { strbuflen = 10000 };
struct strbuf {
  size_t next;
  char str[strbuflen];
};

static struct strbuf* init_buf(struct strbuf *buf) {
  buf->next = 0;
  return buf;
}

static void writec_buf(struct strbuf *buf, char c) {
  assert(buf->next+1 < strbuflen);
  buf->str[buf->next++] = c;
  buf->str[buf->next] = 0;
}

static void writes_buf(struct strbuf *buf, const char *str) {
  char c;
  while ((c = *str++)) {
    writec_buf(buf, c);
  }
}

static void printpd_internal(struct strbuf *buf, ptrdiff_t n) {
  assert(n >= 0);
  if (n < 10) {
    char v = '0' + n;
    writec_buf(buf, v);
  } else {
    printpd_internal(buf, n / 10);
    char v = '0' + (n % 10);
    writec_buf(buf, v);
  }
}

static void printpd_buf(struct strbuf *buf, ptrdiff_t n) {
  if (n < 0) {
    writec_buf(buf, '-');
    if (n == -1) {
      writec_buf(buf, '1');
    } else {
      n = -n;
    }
  }
  if (n == 0) {
    writec_buf(buf, '0');
    return;
  }
  printpd_internal(buf, n);
}

//static void printpd(int fd, ptrdiff_t n) {
//  struct strbuf buf = {0};
//  printpd_buf(&buf, n);
//  mywrite(fd, buf.str, buf.next);
//}


static void *firstmalloc = NULL;
//static void printm(int fd, void *p) {
//  ptrdiff_t d = p - firstmalloc;
//  mywrite(fd, "o", 1);
//  printpd(fd, d);
//}
//static void printmnl(int fd, void *p) {
//  printm(fd, p);
//  mywrite(fd, "\n", 1);
//}
//static void printp(int fd, void *p, void *prevp) {
//  printm(fd, p);
//  mywrite(fd, " d", 2);
//  printpd(fd, ((char*)p) - (char*)prevp);
//  mywrite(fd, "\n", 1);
//}
//static void printp0(int fd, void *p, void *prevp) {
//  printm(fd, p);
//  mywrite(fd, "=d", 2);
//  printpd(fd, ((char*)p) - (char*)prevp);
//  mywrite(fd, "=b", 2);
//  printpd(fd, (((char*)p) - (char*)prevp)/16);
//  mywrite(fd, " ", 1);
//}

static size_t ptrcount = 0 ;
enum {PTRSIZE = 16000};
struct item {
  bool free;
  void *p;
  size_t wordcount;
};
static struct item ptrs[PTRSIZE];

static size_t high_water_mark = 0;
static size_t current_water_mark = 0;

static void* pmalloc(size_t words) {
  current_water_mark += words_to_bytes(words);
  maxf(&high_water_mark, current_water_mark);
  assert(ptrcount < PTRSIZE);
  void *p = malloc(words_to_bytes(words));
  memset(p, 1, words_to_bytes(words));
  struct item result = {false, p, words};
  if (firstmalloc == NULL) {
    firstmalloc = result.p;
  }
  for (size_t i = 0; i < ptrcount; i++) {
    struct item tmp = ptrs[i];
    if (result.p < tmp.p) {
      ptrs[i] = result;
      result = tmp;
    } else if (result.p == tmp.p) {
      ptrs[i] = result;
      assert(tmp.free);
      tmp.p = (char*)(tmp.p) + result.wordcount * 32;
      assert(tmp.wordcount >= result.wordcount);
      tmp.wordcount -= result.wordcount;
      if (tmp.wordcount == 0) return p;
      result = tmp;
    }
  }
  ptrs[ptrcount++] = result;
  return p;
}

static void pmfree(size_t off, size_t wordcount) {
  void *p = ptrs[off].p;
  assert(ptrs[off].wordcount == wordcount);
  free(p);
  assert(current_water_mark >= words_to_bytes(wordcount));
  current_water_mark -= words_to_bytes(wordcount);
  //writes(1, "Freeing something\n");
  size_t i = off;
  struct strbuf buf;
  init_buf(&buf);
  //writes_buf(&buf, "Freeing item ");
  //printpd_buf(&buf, (ptrdiff_t)(i));
  //writec_buf(&buf, '\n');
  //mywrite(2, buf.str, buf.next);
  assert(!ptrs[i].free);
  ptrs[i].free = true;
  if (i + 1 < ptrcount && ptrs[i+1].free) {
    assert(ptrs[i].p + ptrs[i].wordcount * 32 == ptrs[i+1].p);
    ptrs[i].wordcount += ptrs[i+1].wordcount;
    memmove(&ptrs[i+1], &ptrs[i+2], sizeof(ptrs[i])*(ptrcount-i-2));
    ptrcount--;
    //writes(2, "did coalesce next\n");
  } else {
    //writes(2, "Don't coalesce next\n");
  }
  if (i > 0 && ptrs[i-1].free) {
    ptrs[i-1].wordcount += ptrs[i].wordcount;
    memmove(&ptrs[i], &ptrs[i+1], sizeof(ptrs[i])*(ptrcount-i-1));
    ptrcount--;
    //writes(2, "Did coalesce prev\n");
  } else {
    //writes(2, "Didn't coalesce prev\n");
  }
  if (ptrcount > 0 && ptrs[ptrcount-1].free) {
    ptrcount--; // Don't leave a trailing free block.
  }
}

static const size_t n = 4097; // The largest number that this program seems to work for.

static char* printptrs_buf(struct strbuf *buf) {
  init_buf(buf);
  for (size_t i = 0; i < ptrcount; i++) {
    if (i > 0) writes_buf(buf, " ");
    if (ptrs[i].free) {
      writes_buf(buf, "F");
    } else {
      writes_buf(buf, "A");
    }
    size_t wc = ptrs[i].wordcount;
    if (wc <= 4) {
      printpd_buf(buf, wc);
    } else if (wc < n) {
      writes_buf(buf, "(n-");
      printpd_buf(buf, n-wc);
      writec_buf(buf, ')');
    } else if (wc == n) {
      writec_buf(buf, 'n');
    } else if (wc <= n+4) {
      writes_buf(buf, "(n+");
      printpd_buf(buf, wc - n);
      writec_buf(buf, ')');
    } else if (wc <= 2*n) {
      writes_buf(buf, "(2n-");
      printpd_buf(buf, 2*n - wc);
      writec_buf(buf, ')');
    } else if (wc == 2*n) {
      writes_buf(buf, "2n");
    } else {
      writes_buf(buf, "(2n+");
      printpd_buf(buf, wc - 2*n);
      writec_buf(buf, ')');
    }
    if (i > 0) {
      if (ptrs[i-1].p >= ptrs[i].p) {
        fprintf(stderr, "Not strictly ascending ptrs[%lu].p=%p ptrs[%lu].p=%p\n",
                i-1, ptrs[i-1].p, i, ptrs[i].p);
        writes_buf(buf, " errored\n");
        return buf->str; // don't error out, so we can print what we got...
      }
      assert(ptrs[i-1].p < ptrs[i].p);
      size_t diff = ptrs[i].p - ptrs[i-1].p;
      assert((size_t)(diff) == ptrs[i-1].wordcount * 32);
    }
  }
  writes_buf(buf, "\n");
  mywrite(2, buf->str, buf->next);
  return buf->str;
}

static const int fd = 1;

//static void test_printptrs(const char *str) {
//  struct strbuf buf;
//  int r = strcmp(printptrs_buf(init_buf(&buf)), str) == 0;
//  if (!r) {
//    fprintf(stderr, "Expect %s\n", str);
//    fprintf(stderr, "Got    %s\n", buf.str);
//  }
//  assert(r);
//}

static void assert_n_repeats(size_t n_repeats) {
  // Check that we have an `A(n-2)` allocation followed by `n_repeats` of `F(n-3); A1`.
  assert(ptrcount == 2 * n_repeats + 1);
  assert(!ptrs[0].free);
  assert(ptrs[0].wordcount == n - 2);
  for (size_t i = 0; i < n_repeats; i++) {
    assert(ptrs[1+2*i].free);
    assert(ptrs[1+2*i].wordcount == n - 3);
    assert(!ptrs[2+2*i].free);
    assert(ptrs[2+2*i].wordcount == 1);
  }

}

int main(int argc __attribute__((unused)), char **argv __attribute__((unused))) {
  mallopt(M_MXFAST, 0); // Do get glibc to actually do best-first we have to turn off the fast bins.  This version of the code wants to do that. (When fastbins is the default, we should be able to get a bigger blowup by allocating things from every fastbin size, since libc never consolodates them.
  // We also need to have done "export GLIBC_TUNABLES=glibc.malloc.tcache_count=0"
  assert(strcmp(getenv("GLIBC_TUNABLES"),"glibc.malloc.tcache_count=0") == 0);

  if (false) {
    size_t s1 = 24;
    size_t s2 = 56;
    size_t s3 = 56+32;
    void *p1 = malloc(s1);
    void *p2 = malloc(s1);
    void *p3 = malloc(s1);
    void *p4 = malloc(s1);
    void *p5 = malloc(s1);
    void *p2_1 = malloc(s2);
    void *p2_2 = malloc(s2);
    void *p2_3 = malloc(s2);
    void *p3_1 = malloc(s3);
    void *p3_2 = malloc(s3);
    void *p3_3 = malloc(s3);
    // Remember that printf mallocs, so do all the prints after all the mallocs.
    printf("p2   interval for %lu %lu\n", s1, p2-p1);
    printf("p3   interval for %lu %lu\n", s1, p3-p2);
    printf("p4   interval for %lu %lu\n", s1, p4-p3);
    printf("p5   interval for %lu %lu\n", s1, p5-p4);
    printf("p2_1 interval for %lu %lu\n", s2, p2_1-p5);
    printf("p2_2 interval for %lu %lu\n", s2, p2_2-p2_1);
    printf("p2_3 interval for %lu %lu\n", s2, p2_3-p2_2);
    printf("p3_1 interval for %lu %lu\n", s3, p3_1-p2_3);
    printf("p3_2 interval for %lu %lu\n", s3, p3_2-p3_1);
    printf("p3_3 interval for %lu %lu\n", s3, p3_3-p3_2);
//    const size_t overhead = 16;
//    const size_t one = 8;
//    const size_t oneO = one + overhead;
//    const size_t n = 512;
//    const size_t nm2 = n - 2*oneO;
//    void *pa = malloc(8);
//    firstmalloc = pa;
//    printmnl(fd, pa);
//    void *pb = malloc(8);
//    printp(fd, pb, pb);
//
//    void *p1 = malloc(nm2);
//    void *p2 = malloc(one);
//    //printf("%p\n%p\n", p1, p2);
//    printp(fd, p1, pa);
//    printp(fd, p2, p1);
//
//    printf("pa=%p\npb=%p\n", pa, pb);
//    printf("p1=%p\np2=%p\n", p1, p2);

    return 0;
  }

  init_rss();
  size_t brss = get_base_rss();

  assert(n>5);

  // line 1
  pmalloc(n-2);
  pmalloc(n-3);
  pmalloc(1);
  pmalloc(n-3);
  pmalloc(1);
  pmalloc(n-3);
  pmalloc(1);
  pmfree(1, n-3);
  pmfree(3, n-3);
  pmfree(5, n-3);

  const size_t target_n_repeats = 4000;
  for (size_t n_repeats = 3; n_repeats < target_n_repeats; n_repeats++) {
    // We start with an `A(n-2)` allocation followed by `n_repeats` of `F(n-3); A1`.
    assert_n_repeats(n_repeats);

    // Robson:
    // > First two n-2 word blocks are allocated (of which the second merely acts a buffer)

    // line 2
    pmalloc(n-2);
    pmalloc(n-2);
    // Quoting Robson:
    // > then a sequence of freeings and allocations transforms the first of them
    // > into an n-3 word gap followed by a single word block
    assert(!ptrs[6].free && ptrs[6].wordcount == 1);
    // line 3
    pmfree(2*n_repeats +1, n-2);
    pmfree(2*n_repeats, 1);
    // line 4
    pmalloc(n);
    // line 5
    pmalloc(n-5);
    // line 6
    pmalloc(1);
    // line 7a
    pmfree(2*n_repeats + 2, n-2);

    for (size_t it = 0; it<n_repeats - 1; it++) {
      // lines 7 (d5); 14 (d3)
      pmfree(2*n_repeats -1 - 2*it, n);
      // lines 8; 15
      pmalloc(n-2);
      // lines 9; 16
      pmalloc(2);
      // lines 10; 17
      pmfree(2*n_repeats -1 - 2*it, n-2);
      pmfree(2*n_repeats -2 - 2*it, 1);
      // lines (11, 12, 13); (18, 19, 20)
      pmalloc(n);
      pmalloc(n-5);
      pmalloc(1);
      pmfree(2 * n_repeats + 1 - 2*it, n-5);
      pmfree(2 * n_repeats - 2*it, 2);
    }
    //test_printptrs("A(n-2) A(n-2) A1 A2 A(n-5) A1\n");
    // The final iteration of the loop changes on the size of the free at line 24
    {
      // line 21
      pmfree(1, n);
      // line 22
      pmalloc(n-2);
      // line 23
      pmalloc(2);
      // line 24
      pmfree(1, n-2);
      pmfree(0, n-2);
      pmalloc(n);
      pmalloc(n-5);
      // line 27
      pmalloc(1);
    }
    pmfree(4, n-5);
    pmfree(3, 2);
    pmfree(1, n-5);
    pmfree(0, n);
    pmalloc(n-2);
  }
  assert_n_repeats(target_n_repeats);
  if (target_n_repeats < 100) {
    struct strbuf buf;
    printptrs_buf(init_buf(&buf));
    writes(fd, "Final state\n");
    mywrite(fd, buf.str, buf.next);
  }
  printf("baserss=%lu\nrss    =%lu\n", get_base_rss(), get_adjusted_rss());
  printf("blowup=%f\n", get_adjusted_rss()/(double)high_water_mark);
  printf("brss=%lu\n", brss);
  printf("rss=%lu\n", get_rss());
  return 0;
}
