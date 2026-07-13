# Malloc Benchmarks

## Definition of memory blowup

The space ratio at a given point of time $t$ while a program is running is
$N(t)/M(t)$ where $M(t)$ is the amount of memory that the user actually needs at
time $t$, and $N(t)$ is the is the amount of memory that the allocator uses to
satisfy that need.  We also define $n$ as the ratio in size of the largest block
allocated to the smallest.

## The Benchmarks

### Boom

Measures space blowup using the worst-known workload for first fit.  

The  worst known workload for first-fit is:
1. Allocate $m$ blocks of size $1$.
2. Free every other block from (1).
3. Allocate $m/2$ blocks of size $2$.
4. Free every other remaining block from (1), and every other block from (3).
5. Allocate $m/4$ blocks of size $4$.
6. Free every other remaining block from each of (1), (3), and (5).
....
(Repeat up until you get to blocks of size $m$, having performed $\log m$ rounds of allocation/freeing overall.)

The most memory ever allocated is $(1 + 1/2 + 1/4 + ....) m = 2m$.

On the other hand, in each round where we allocate new stuff, we are forced to
use totally new memory, meaning that the total memory in use by the allocator
ends up being $m \log m$.

So the overall blowup in memory compared to OPT is $(\log m)/2$.

From the upper-bound side, it's also been proven that first-fit has competitive
ratio at most $\log m + O(1)$, where $m$ is the maximum amount ever allocated
(or the maximum block size). So there's a factor of two gap between the upper
and lower bound.

As far as we know, nobody has ever resolved this factor-of-two gap. In fact, we
think it's even open what the optimal constant is for an arbitrary
memory-allocation algorithm (between $0.5 \log m$ and $0.84 \log m$ I think,
based on the second to last paragraph of Page 52 of
[WilsonJoNe95](bib/WilsonJoNe95.pdf)). Maybe this is worth thinking about...

### HoardWorst

Measures the space blowup using the worst-known workload for Hoard.  Hoard
allocates memory using *superblocks*.  A superblock is a block of memory
(perhaps of size 256KiB) that contains blocks all the same size.  The sizes are
limited to certain values.  In hoard, the sizes classes start with 16 and then
the next size is gotten by multiplying by 1.2 and rounding up to the next
multiple of 16.

The worst-known workload for hoard is to allocate $m$ bytes in the first size
class, and then free all but one block in every superblock.  Then allocate $m$
bytes in the second size class, and then free all but one block in every
superblock, and so forth.  The memory blowup is approximately equal to the
number of size clases, which is 68.  Asymptotically this would be something like
$M \log_{1.2} n$.

For hoard we see a blowup of about 19.5 with this workload.
