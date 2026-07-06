# ffmalloc

A malloc library that that employs first fit.

For small blocks (less than mmap_lower_bound, 256K as of this writing) we use
first fit.

For large blocks (>=256KiB) we just use the operating system's mmap to allocate
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

## Data Layout

An allocated block at address `p` has an 8-byte header, called a `BOUNDARY_TAG`
at `p-8`.  `p` is always 8-byte aligned.  A block can either be a tree-allocated
block (used for small blocks < 256KiB), or a map-allocated block (for blocks >=
256KiB).  Orthogonally to that, a block can be unaligned or aligned.  There are
thus four cases

1. A tree-allocated unaligned block.  This is the kind of block returned by
   `malloc(16)`, for example.  The boundary tag contains
     - low-order-bit:  is_free:1 = 0
	 - next-bit:       is_memaligned:1 = 0
	 - next_bits       size:62 = n, where `n` is the size of the block including the boundary tag and `n < 1<<18`.
2. A map-allocated unaligned block.  This is the kind of of block returned by
   `malloc(1<<20)`, for example.  The boundary tag contains
     - low-order-bit:  is_free:1 = 0
	 - next-bit:       is_memaligned:1 = 0
	 - next_bits:      size:62 = n, where `n` is the size of the block including the boundary tag and `n >= 1<<18`.

	Note that if you allocate `1<<20`, which with 4096-byte pages is 256 pages, the allocator will actually memory map 257 pages because we need the first 8 byte for the boundary tag.
	
	Thus, the only difference in the data layout between a map-allocated unaligned block and a tree-aallocated unaligned block is the size.

3. A tree-allocated aligned block.  This is the kind of returned by
   `posix_memalign(&p, 32, 64)`, for example.  Whereas for aligned blocks, `p`
   always points at the second 8-byte word in the underlying allocation, for
   aligned blocks, `p` may point into to the middle of the block.  We need a
   representation that can allow us to figure out how bit the underlyuing
   allocation is, and where it starts.

	The boundary tag contains
     - low-order-bit: is_free:1 = 0
	 - next-bit:      is_memaligned:1 = 1
	 - next-bits:     size:62 = n, where `n` is the size of the underlying allocation (including headers).
     - The word before the boundary tag: contains a pointer to the the start of the underlying allocation, except that we steal the low-order two bits of the pointer to store `is_free=0` and `is_memaligned=1` again.
	 - Also if `p` doesn't point at the second word in the allocation (it might point at the third word or the fourth word or something), then we repeat the boundary tag again in the second word.
	 
	So given `p` we can figure out the size by looking at the `size` field at
    `p-8`, and if the memaligned bit is true, we can find the start of the
    underlying block in the pointer stored at `p-16` (zero out the two low order
    bits of the pointer before using it.)

    Given a pointer `q` to the beginning of the underlying block, we can
    determine what's going on by looking at the `is_free` and `is_memaligned` in
    the low-order bits of `*(uint64_t*)q`. Those bits line up the same way for
    all the cases, so if we see `!is_free` and `is_memaligned` then the rest of
    the pointer will turn out to be equal to `q` itself, and if we look at `q+8`
    we'll find a boundary tag that tells us the size of block.

4. A map-allocated aligned block is the same as a tree-allocated aligned block, except the size is larger.

## TODO

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

Done: Fixed a bug in which when a block of size n is found to satisfy a block of
size m where m + 24 > n, so we don't break the block, but the usable size was
set wrong.

DONE: having abstracted the headers, we had a slowdown.  Inline the haaders
 inline everything in headers.c: (ff7aeb2d590e909b62b1eac4d375bf5273a15944
  LD_PRELOAD=ffmalloc/lib/libffmalloc.so time ./boom --malloclib=DEFAULT > /dev/null
  6.18user 0.91system 0:07.10elapsed 99%CPU (0avgtext+0avgdata 716544maxresident)k
  6.37user 0.87system 0:07.25elapsed 99%CPU (0avgtext+0avgdata 716544maxresident)k
  6.97user 0.93system 0:07.90elapsed 99%CPU (0avgtext+0avgdata 716544maxresident)k
 main: 75821f9530474c1e1c0d166ec66bfa309334c5a4
  8.24user 0.90system 0:09.15elapsed 99%CPU (0avgtext+0avgdata 716544maxresident)k
  8.30user 0.88system 0:09.19elapsed 99%CPU (0avgtext+0avgdata 716544maxresident)k
  8.39user 0.90system 0:09.31elapsed 99%CPU (0avgtext+0avgdata 716544maxresident)k

DONE: Don't waste a word on 16-byte alignments.  Previously we stored a pointer
to the original boundary tag just before the aligned boundary tag.  Now we store
the offset to the original tag in the aligned tag.  That means that
malloc_usable_size is a little more complicated in the aligned case since we
need to go to original tag and get the size and then subtract the offset.

DONE: optimize the consolidation with the next node.  Previously we did a find
and a delete.  Now just a delete.  Probabl 5% faster
  optimized: ab8454ab8a9369288ffad808ea358543c66315da
   LD_PRELOAD=ffmalloc/lib/libffmalloc.so time ./boom --malloclib=DEFAULT > /dev/null
   5.85user 0.89system 0:06.75elapsed 99%CPU (0avgtext+0avgdata 716544maxresident)k
   5.74user 0.92system 0:06.66elapsed 99%CPU (0avgtext+0avgdata 716544maxresident)k
   6.69user 0.92system 0:07.61elapsed 99%CPU (0avgtext+0avgdata 716544maxresident)k
  main c7037ba379a9c8561bf07f784b4174e4be396594
   6.36user 0.90system 0:07.26elapsed 99%CPU (0avgtext+0avgdata 716544maxresident)k
   6.17user 0.90system 0:07.08elapsed 99%CPU (0avgtext+0avgdata 716544maxresident)k
   6.57user 0.90system 0:07.49elapsed 99%CPU (0avgtext+0avgdata 716544maxresident)k
   
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

 If we turn off block consolidation (on branch no-consolidate-blocks) we get to
 3.2x slower than libc on boom.
  4.58user 0.88system 0:05.47elapsed 99%CPU (0avgtext+0avgdata 1505504maxresident)k
 That suggests that we should try to optimize block consolidation.

TODO: Do worst-case for TCMALLOC (various web pages claim tcmalloc is best fit)

