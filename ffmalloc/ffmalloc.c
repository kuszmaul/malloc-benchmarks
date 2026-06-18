// First-fit implementation of malloc.

// Definitions:
//
//   Small blocks: b locks that are managed with first fit.
//
//   Large blocks: are memory-mapped rather than managed with first fit.
//    "large" maens >= `mmap_lower_bound`, which is tentatively, `1<<18`.
//
//   Word: 8 bytes.
//
//   Page: 4096 bytes (we have `page_size == 4096`).
//
//   Boundary tag: [the idea is due to Knuth TAOCP vol 1].  The malloc functions
//    return a pointer to the user.  Just before that pointer is a word
//    containing a header, called the boundary tag. The boundary tag is 1 word containing
//      ```
//      struct boundary_tag {
//          size_t is_memaligned:1;
//          size_t size:63;
//      }}
//      ```
//
//    The meaning is this:
//
//      If `is_memaligned` is `false` then `size` is the number of bytes in the
//        block.  if `size` is >= `mmap_lower_bound` then the block is mmapped.
//        The size must be a multiple of `page_size`.
//
//      If `is_memaligned` is `true` then we this block is the result of a call
//        to `posix_memalign` or `aligned_alloc` or `memalign`.  To allocate a
//        block of size `s` aligned to `a` (which must be a power of two) we
//        need to allocate extra bytes and then waste part of the allocation.
//        The idea is that if we allocate `s + a - 1` bytes, then somewhere in
//        the resulting block there is a block of size `s` that is aligned to
//        `a`.  For a memaligned block we need a 2-word header: The first is
//        a pointer to the actual beginning of the block, and the second is our
//        header with `is_memaligned==true`.  The `size` is the size of the
//        originally allocated block.  (And if `size >= mmap_lower_bound` then
//        the originally allocated block is allocated via mmap and the size is a
//        multiple of `page_size`.)
//
//        When allocating a memaligned block, one could imagine peeling off the
//        unused memory at the beginning of the allocation and making it
//        available for different allocation.  Doing so could be problematic,
//        however, sine the resulting block would be in the wrong place for
//        first fit.  You could end up with a very small block at a very high
//        address, which would violate the invariant proved in Robson77 (Section
//        3) that small blocks appear only at low addresses. For memaligned
//        blocks, the extra block wouldn't even be in the right part of the
//        address space.  So instead, we simply treat the unused memory as part
//        of the allocation.
//
//        We also special-case alignment==8, since that adds no alignment
//        constraints: it just directly calls malloc, which always returns
//        8-byte aligned values.

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>

#include "headers.h"
#include "ffmalloc.h"
#include "max.h"
#include "tree.h"
#include "tree-test-helpers.h"

FFTREE *arena = NULL;
static size_t last_sbrk_size = 1;
static void *sbrk_start = NULL;
static void *sbrk_end = NULL;


bool ffmalloc_owns_address(void *p) {
  return (sbrk_start <= p ) && (p < sbrk_end);
}

BOUNDARY_TAG* get_boundary_tag_pointer(void *p) {
  return ((BOUNDARY_TAG*)(p)) - 1;
}

static void** get_memaligned_original_stored_at_pointer(void *p) {
  return ((void**)(get_boundary_tag_pointer(p))) - 1;
}

BOUNDARY_TAG get_boundary_tag(void *p) {
  return *(get_boundary_tag_pointer(p));
}

void* get_memaligned_original(void *p) {
  return *get_memaligned_original_stored_at_pointer(p);
}

static bool ispow2(size_t n) {
  return n > 0 && (n & (n-1)) == 0;
}

static size_t alignup(size_t n, size_t alignment) {
  ASSERT(ispow2(alignment));
  return (n + alignment - 1) & ~(alignment -1);
}

static void* alignup_pointer(void* p, size_t alignment) {
  return (void*)(alignup((uintptr_t)(p), alignment));
}

static size_t compute_next_sbrk_size(size_t size) {
  size_t prev = last_sbrk_size;
  if (prev < size) {
    size = alignup(size, page_size);
  } else {
    size *= 2;
  }
  last_sbrk_size = size;
  return size;
}

// Handle the case for mallocing a large (mmapped) block.
// Returns 0 on success (and sets *result) or an error code.
static int ff_malloc_mmap_e(void** result, size_t size) {
  size = alignup(size + sizeof(BOUNDARY_TAG), page_size);
  ASSERT(size >= mmap_lower_bound);
  void* p = mmap(0, size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  if (p == (void*)-1) {
    ASSERT(errno == ENOMEM);
    return ENOMEM;
  }
  BOUNDARY_TAG *bt = p;
  bt->is_memaligned = false;
  bt->size = size;
  *result = (void*)((char*)p + sizeof(BOUNDARY_TAG));
  return 0;
}

static void fftree_insert_and_merge(FFTREE **tree_p, void* node, size_t node_size) {
  ASSERT(node_size >= sizeof(FFTREE));
  FFTREE *here = (FFTREE*)node;
  *here = (FFTREE){NULL, NULL, 0, node_size, node_size};
  {
    FFTREE *prev = fftree_find_and_remove_prev_adjacent(tree_p, here);
    if (prev != NULL) {
      prev->size += node_size;
      here = prev;
    }
  }
  {
    FFTREE *next = fftree_find_and_remove_next_adjacent(tree_p, here);
    if (next != NULL) {
      here->size += next->size;
    }
  }
  fftree_insert(tree_p, here);
}

static int ff_malloc_firstfit_e(void **result, size_t size) {
  maxf(&size, sizeof(FFTREE) - sizeof(BOUNDARY_TAG));
  ASSERT(size < mmap_lower_bound);
  FFTREE *node = fftree_find_and_remove_first_fit(&arena, size);
  if (node == NULL) {
    const size_t overhead_at_beginning = 8;
    // So we'll need a few extra bytes at the end to have a free block at the end.
    const size_t overhead_at_end = sizeof(FFTREE);
    // We'll also need a size_t in the overhead for the boundary tag.
    const size_t overhead = overhead_at_beginning + overhead_at_end;
    const size_t n_to_sbrk = compute_next_sbrk_size(size + overhead);
    // TODO: Watch out that we don't sbrk more than a gigabyte per call to sbrk.
    void *p = sbrk(n_to_sbrk);;
    if (p == (void*)-1) {
      ASSERT(errno == ENOMEM);
      return errno;
    }
    if (sbrk_start == NULL) {
      sbrk_start = p;
    }
    sbrk_end = (char*)p+n_to_sbrk;
    fftree_insert_and_merge(&arena, p, n_to_sbrk);
    ASSERT(fftree_validate(arena));
    node = fftree_find_and_remove_first_fit(&arena, size);
  }
  ASSERT(fftree_validate(arena));

  size_t nsize = node->size;

  if (nsize >= size + sizeof(FFTREE) + sizeof(BOUNDARY_TAG)) {
    // We can break the node in two.  There's enough space for the boundary tag
    // and for the cut off piece to hold an FFTREE.
    FFTREE *here = (FFTREE*)((char*)node + size + sizeof(BOUNDARY_TAG));
    here->size = nsize - size - sizeof(BOUNDARY_TAG);
    // Don't need to merge here, since there won't be any adjacent nodes.
    fftree_insert(&arena, here);
    ASSERT(fftree_validate(arena));
  }
  BOUNDARY_TAG* tag = (BOUNDARY_TAG*)(node);
  tag->is_memaligned = false;
  tag->size = size + sizeof(BOUNDARY_TAG);
  *result = (void*)((char*)node + sizeof(BOUNDARY_TAG));
  return 0;
}

// `ff_malloc_e` is like malloc except that it uses the calling interface that
// posix_memalign uses.  It returns 0 on success (and stores the result in
// *result) or returns an error code.
int ff_malloc_e(void **result, size_t size) {
  if (size == 0) {
    *result = NULL;
    return 0;
  }
  size = alignup(size, 8);
  if (size >= mmap_lower_bound) {
    return ff_malloc_mmap_e(result, size);
  } else {
    return ff_malloc_firstfit_e(result, size);
  }
}

///////////////////////////////// tests ///////////////////////////////////////

int ff_posix_memalign(void **result, size_t alignment, size_t size) {
  if (!ispow2(alignment)) return EINVAL;
  ASSERT(alignment % sizeof(void*) == 0);
  if (size == 0) {
    *result = NULL;
    return 0;
  }
  if (alignment <= sizeof(BOUNDARY_TAG)) {
    // small alignments don't need anything
    int r = ff_malloc_e(result, size);
    return r;
  }
  size_t amount_to_malloc = sizeof(BOUNDARY_TAG) + size + alignment - 1;
  void *p;
  int r = ff_malloc_e(&p, amount_to_malloc);
  if (r != 0) {
    return r;
  }
  // We now have
  //   p[-8] the original boundary tag (which we won't use)
  //   p[0] space for the new boundary tag
  //   p[8 .. 8 + size + alignment -1]    space for an aligned block.
  BOUNDARY_TAG original_boundary_tag = get_boundary_tag(p);
  void *p_to_return = alignup_pointer((char*)p + sizeof(BOUNDARY_TAG), alignment);
  BOUNDARY_TAG *new_tag_p = get_boundary_tag_pointer(p_to_return);
  *new_tag_p = (BOUNDARY_TAG){1, original_boundary_tag.size};
  void ** store_start_at = get_memaligned_original_stored_at_pointer(p_to_return);
  *store_start_at = get_boundary_tag_pointer(p);
  *result = p_to_return;
  return 0;
}

void ff_free(void *p) {
  BOUNDARY_TAG bt = get_boundary_tag(p);
  if (!bt.is_memaligned) {
    if (bt.size >= mmap_lower_bound) {
      BOUNDARY_TAG* btp = get_boundary_tag_pointer(p);
      ASSERT(((uintptr_t)(btp)) % page_size == 0);
      int r = munmap(btp, bt.size);
      ASSERT(r == 0);
    } else {
      size_t size = bt.size;
      fftree_insert_and_merge(&arena, get_boundary_tag_pointer(p), size);
    }
  } else {
    // Reconstruct the boundary tag as it was originally before we did the
    // memalignment hacking.  (We could have overwritten the boundary tag with
    // the original_stored_at information.)
    BOUNDARY_TAG *original_tag_p = (BOUNDARY_TAG*)(*(get_memaligned_original_stored_at_pointer(p)));
    *original_tag_p = (BOUNDARY_TAG){0, bt.size};
    ff_free(original_tag_p + 1);
  }
}

size_t ff_malloc_usable_size(void *p) {
  BOUNDARY_TAG bt = get_boundary_tag(p);
  if (!bt.is_memaligned) {
    return bt.size - sizeof(BOUNDARY_TAG);
  } else {
    return bt.size + ((char*)p - (char*)(get_memaligned_original_stored_at_pointer(p)));
  }
}
