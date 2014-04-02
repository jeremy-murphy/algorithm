radix sort
==========

This is an implementation (and speed test) of radix sort, based on the algorithm in CLRS[1].

Sorting is one of the fundamental tasks that a computer carries out, often implicitly.
Knuth made some quip about a large percentage of all computation being sorting, but I can't find it.
Common comparison sort algorithms have a worst-case complexity of O(n lg(n))[2].
Counting sort algorithms have a worst-case complexity of Θ(n + k) if k = O(n).

This implementation of radix sort has pretty good performance characteristics on x86 for large n.
x std::sort means "times faster than std::sort" on average.

    type    x std::sort
    --------------------
    char    13.5
    short    3.6
    int      2.9
    long     1.4


I would really like to test on non-x86 architectures!
MIPS, Sparc, Itanium, Power, anything will help -- please get in touch.


[1] Cormen, T. H., et al. "Introduction to Algorithms MIT Press." Cambridge, MA, (2009).

[2] https://en.wikipedia.org/wiki/Introsort
