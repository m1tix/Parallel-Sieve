# Idea

The usual sieve of eratosthenes is simple: start at 2 and clear all multiples of
2 up to n. Then clear all multiples of the next uncleared number, in this case
3, and repeat until you reach sqrt(n). Some very basic optimizations can be
made: only store odd numbers, start clearing at the square, increment by 2 times
the prime etc.

```c
n = (n % 2 == 0) ? n - 1 : n;
int sizeprimelist = n / 2;
char *primelist = calloc(sizeprimelist, sizeof(char));
primelist[0] = 1;
for (int i = 3; i < sizeprimelist; i+=2) {
    int iindex = i / 2;
    if (primelist[iindex] == 0) {
        for (int j = i*i; j < n; j+= 2*i) {
            primelist[j / 2] = 1;
        }
    }
}
// now 2*i+1 is prime iff i-th element of primelist is 0.
```

## Parallel

The first for-loop cannot be parallelized: whether i enters the second loop
depends on ALL previous values of i. The second for-loop can however. What we do
is as follows: let processor 0 take care of the outer loop. The total array is
then divided amongst all other array. How are we going to divide them? Well, let
the first p - 1 processors take `B = floor(L/p)` elements where `L = (n >> 1) + 1`.
So that means that processor s for s between 0 and p - 2 takes the following
numbers

```c
{2*s*B + 1, ..., 2*((s+1)*B - 1) + 1}
```

The last processor will take the remainder. Note that we want the first
processor to take care of the outer loop, hence `sqrt(n)` should be smaller than
the last element of the array which `s = 0` possess. A rough bound would be
`p <= sqrt((n + 1)^2 / n)` or about `sqrt(n + 2)`, i.e. around `sqrt(n)`.

This implementation will yield a mere 0.3x speedup, i.e. it actually slows
down. So something is wrong....
