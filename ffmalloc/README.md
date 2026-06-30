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

DONE: Get the linking with libc done right.  (see for example how hoard does the linking at wrapper.cpp, which isn't how we did it.)  My solution:
   1) define __libc_malloc which calls our function.
      (glibc uses a strong alias to make malloc be __libc_malloc)
   2) define vers.map
   3) add to the link flags
     -Wl,--version-script=/home/bradley/github/Hoard/build/vers.map

DONE: madvise-dontneed free the interior of freed blocks.  The resulting curve for worst-case first fit beats everything, including libc.

DONE: malloc(0) cannot return null.   It breaks libraries such argtable2-13.
 (There's a bug in argtable2: If malloc(0) returns NULL, the code fails.  So a fix
  is to return a pointer that can be freed.)

Fixed: My rand is 255 too often

Done: Add -Wl,-soname,libffmalloc.so

Done: How does the kernel allocate virtual addresses for mmap?
 mmap() uses a first-fit scheme, goint top-down in the address space, starting at a randomized initial position.
 It appears to use a linear-time search, which would be bad if you had lots of mappings.
 But the kernel only accounts memory for O(100,000) mappings, and searches are always fast in that range.
 It turns out that the if you mmap a bunch of 1-page objects and then start calling free of every other object, the kernel runs out of memory on the free call.  So it must be merging mmaps as it goes.
 Moral: Don't mmap in a way that requires the kernel to maintain too many mappings.

TODO: Make sure that we don't sbrk too much (there's some bug in sbrk that doesn't let you allocate 8GB at a time, but if you do 1GB at a time it seems ok).

TODO: test calloc overflow
TODO: the other memalign functions
TODO: maybe -ldl

TODO: Add align functions to scripts.

TODO: ffmalloc boom is very slow (26x slower than libc)

 time ./boom --malloclib=DEFAULT > boom-ffworst-glibc-2to27.data
 1.36user 0.36system 0:01.72elapsed 99%CPU (0avgtext+0avgdata 1126912maxresident)k

 LD_PRELOAD=ffmalloc/lib/libffmalloc.so time ./boom --malloclib=DEFAULT > boom-ffworst-ffmalloc-2to27.data
 35.64user 0.94system 0:36.60elapsed 99%CPU (0avgtext+0avgdata 716928maxresident)k

 got rid of an gratuitous validate: (now 18x slower)
 30.67user 0.84system 0:31.58elapsed 99%CPU (0avgtext+0avgdata 716928maxresident)k

 Removing the madvise makes no difference.

 Better hash (so the tree is probably better balanced) (now 4.1x slower than libc)
 6.24user 0.83system 0:07.08elapsed 99%CPU (0avgtext+0avgdata 717056maxresident)k

TODO: Do worst-case for TCMALLOC (various web pages claim tcmalloc is best fit)



