# Malloc Benchmarks

## Boom

Measures space blowup using the worst-known workload for first fit.  

The  worst known workload for first-fit is:
1. Allocate $n$ objects of size $1$.
2. Free every other object from (1).
3. Allocate $n/2$ objects of size $2$.
4. Free every other remaining object from (1), and every other object from (3).
5. Allocate `n/4` objects of size `4`.
6. Free every other remaining object from each of (1), (3), and (5).
....
(Repeat up until you get to objects of size n, having performed \log n rounds of allocation/freeing overall)

The most memory ever allocated is (1 + 1/2 + 1/4 + ....) n = 2n. 

On the other hand, in each round where we allocate new stuff, we are forced to use totally new memory, meaning that the total memory in use by the allocator ends up being n \log n. 

So the overall blowup in memory compared to OPT is (\log n)/2.

From the upper-bound side, it's also been proven that first-fit has competitive ratio at most log n + O(1), where n is the maximum amount ever allocated (or the maximum object size). So there's a factor of two gap between the upper and lower bound.

As far as I know, nobody has ever resolved this factor-of-two gap (maybe Michael knows better). In fact, I think it's even open what the optimal constant is for an arbitrary memory-allocation algorithm (between 0.5 \log n and 0.84 \log n I think, based on the second to last paragraph of Page 52 of this survey). Maybe this is worth thinking about...
