# pthreads-timing

A short program demonstrating the benefits of pthreads on a parallelizable algorithm, a (step-wise) array summation. Each thread is given a block of memory to sum.

*    -b #     set the number blocks in which to divide the input array (one block per pthread)
*    -m #     set init mode; 0 for arr[i] = i, 1 for rng
*    -p #     the step size for the array summation (default 1, with a single pass over each block)
*    -r #     the seed (the random number generator is unseeded if this value is not provided)
