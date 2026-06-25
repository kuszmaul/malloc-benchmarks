# ffmalloc

A malloc library that that employs first fit.

For small blocks (less than mmap_lower_bound, 256K as of this writing) we use
first fit.

For large blocks (>=256K) we just use the operating system's mmap to allocate
space.

The first-fit part of the code uses sbrk to grow the memory being managed, and
keeps all the free blocks in a binary search tree, sorted by address.  The tree
is augmented so that each node remembers the size of the largest block in its
subtree, which makes it possible to implement first-fit in log time.

For memalign we allocate a block that's too big, and then return the first
aligned pointer in the allocated block.  We have to take care to store enough
information that free can figure out the true size of the block.  We keep the
overallocated block intact (for example, we don't take the unused part at the
end and return it to the free list.  We could try to do that sometime, but the
benefit seems marginal since first fit will tend not to use a small block that's
deep into the address.

For realloc:
 * If the realloc doesn't change the size, we just use the same block.

 * For small blocks, if the realloc changes the size, we just allocate a new
   block and copy the data.  (Rationale: If the block shrinks we would have
   unused space, and it's probably not a good idea to stick that small unused
   space into the free block tree, because we'd have a small block far to the
   right of where we expect to find one.)
   
   Maybe that's a bad idea and we should shave it.

DONE: build the library with only the exported API

DONE: Get the linking with libc done write.  (see for example how hoard does the linking at wrapper.cpp)
   1) define __libc_malloc which calls our function.
      (glibc uses a strong alias to make malloc be __libc_malloc)
   2) define vers.map
   3) add to the link flags
     -Wl,--version-script=/home/bradley/github/Hoard/build/vers.map

TODO: Make sure that we don't sbrk too much (there's some bug in sbrk that doesn't let you allocate 8GB at a time, but if you do 1GB at a time it seems ok).

TODO: madvise-dontneed free the interior of freed blocks
TODO: test calloc overflow
TODO: the other memalign functions
TODO:
   4) maybe also add -Wl,-soname,libhoard.so -dl
      (change "libhoard" to "libffmalloc" and Do we need -ldl?)

TODO: Add align functions to scripts.

There's a bug in argtable2: If malloc(0) returns NULL, the code fails.  So a fix
is to return a pointer that can be freed.
