#include <bsp.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// TODO
// make printing function.
// more functions.

static unsigned int P = 4;
unsigned long MAX;

void bsp_main(void);
unsigned long closest_odd(unsigned long n);

/* closest_odd: returns the smallest odd integer closest to input. */
/* make sure that m >= 1, or else we've got an issue ;) */
unsigned long closest_odd(unsigned long n) {
    if ((n & 1) == 0) {
        return n - 1;
    } else {
        return n;
    }
}

/* sieve the given array a, which starts at start and ends at end,
 * up to the specified bound */
void sieve(char *a, unsigned long start, unsigned long end,
           unsigned long bound) {
    unsigned long sqrt_bound = closest_odd(sqrt(bound));
    for (unsigned long i = start; i <= sqrt_bound; i += 2) {
        if (a[i >> 1] == 0) {
            for (unsigned long j = i * i; j <= end; j += i << 1) {
                a[j >> 1] = 1;
            }
        }
    }
}

void bsp_main(void) {
    bsp_begin(P);
    long s = bsp_pid();
    long p = bsp_nprocs();
    unsigned long sqrtMAX = closest_odd(sqrt(MAX));

    unsigned long primes_len = (sqrtMAX >> 1) + 1;
    char *primes = calloc(primes_len, sizeof(char));
    if (primes == NULL) {
        bsp_abort("Error assigning memory");
    }
    bsp_push_reg(primes, sizeof(char) * primes_len);

    unsigned long full_array_len = (MAX >> 1) - primes_len + 1;
    unsigned long composite_array_len = 0;
    unsigned long blocksize = (full_array_len + p - 1) / p;

    if (s == (p - 1)) {
        composite_array_len = full_array_len - (p - 1) * blocksize;
    } else {
        composite_array_len = blocksize;
    }

    char *composite_array = calloc(composite_array_len, sizeof(char));
    if (composite_array == NULL) {
        bsp_abort(" WAA");
    }
    bsp_sync();
    if (s == 0) {
        primes[0] = 1;
        sieve(primes, 1, sqrtMAX, sqrtMAX);
        for (long t = 1; t < p; t++) {
            bsp_put(t, primes, primes, 0, sizeof(char) * primes_len);
        }
    }
    bsp_sync();
    bsp_pop_reg(primes);

    unsigned long start = sqrtMAX + 2 + (s * blocksize * 2) | 1;
    unsigned long end = start + (composite_array_len << 1) - 2;
    printf("s = %ld is responsible for %lu-%lu\n", s, start, end);
    for (unsigned long k = 1; k < primes_len; k++) {
        if (primes[k] == 0) {
            unsigned long q = (k << 1) | 1;
            unsigned long j = q * q;
            if (j < start) {
                j = (start + q - 1) / q * q;
                if ((j & 1) == 0) {
                    j += q;
                }
            }
            for (; j <= end; j += (q << 1)) {
                composite_array[(j - start) >> 1] = 1;
            }
        }
    }
    bsp_sync();
    unsigned long local_count = 0;
    unsigned long *sum = calloc(p, sizeof(unsigned long));
    bsp_push_reg(sum, p * sizeof(unsigned long));
    bsp_sync();
    for (unsigned long i = 0; i < composite_array_len; i++) {
        if (composite_array[i] == 0) {
            local_count += 1;
        }
    }
    for (long t = 0; t < p; t++) {
        bsp_put(t, &local_count, sum, s * sizeof(unsigned long), sizeof(unsigned long));
    }
    bsp_sync();
    unsigned long total = 1;
    for (unsigned long i = 0; i < primes_len; i++) {
        if (primes[i] == 0) {
            total += 1;
        }
    }
    for (long t = 0; t < p; t++) {
        total += sum[t];
    }
    bsp_pop_reg(sum);
    free(sum);
    free(primes);
    free(composite_array);
    printf("I, s=%ld, have found %lu primes\n", s, total);
    bsp_end();
}

int main(int argc, char **argv) {
    bsp_init(bsp_main, argc, argv);
    MAX = strtol(argv[1], NULL, 10);
    MAX = closest_odd(MAX);
    bsp_main();
    return EXIT_SUCCESS;
}
