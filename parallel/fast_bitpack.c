#include <bsp.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// TODO
//  NEW SOLUTION JUST DROPPED
//      let s=0 do the prime finding prior,
//      i.e. sieve [1, sqrt(N)] on s = 0
//      send this array to all processors
//      let the processors sieve locally
//      and combine.
//      Assume for simplicity that sqrt(N) is actually in s = 0.
//  superstep 1 is completely WRONG

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
void sieve(uint32_t *a, unsigned long start, unsigned long end, unsigned long bound) {

}

void bsp_main(void) {
    /* bulk of program */
    bsp_begin(P);
    long s = bsp_pid();
    long p = bsp_nprocs();
    unsigned long full_array_len = (MAX >> 6) + 1;
    unsigned long composite_array_len = 0;
    unsigned long sqrtMAX = closest_odd(sqrt(MAX));
    unsigned long blocksize = (full_array_len + p - 1) / p;

    if (s == (p - 1)) {
        composite_array_len = full_array_len - (p - 1) * blocksize;
    } else {
        composite_array_len = blocksize;
    }
    uint32_t *composite_array = calloc(composite_array_len, sizeof(uint32_t));

    if (composite_array == NULL) {
        bsp_abort("WAAAAAAAA, something wrong while assigning mem\n");
    }
    // superstep 0: proc s=0 calculates all primes below sqrt(MAX)
    unsigned long index_sqrtMAX = sqrtMAX >> 1;
    if (s == 0) {
        // 1 is not prime ;)
        composite_array[0] |= 1;
        for (unsigned long i = 1; i <= index_sqrtMAX; i++) {
            if ((composite_array[i >> 5] & (1 << (i & 31))) == 0) {
                unsigned long i_val = (i << 1) + 1;
                for (unsigned long j = i_val * i_val; j <= sqrtMAX;
                     j += i_val << 1) {
                    composite_array[j >> 6] |= 1 << ((j & 63) >> 1);
                }
            }
        }
    }

    // array containing the found primes in first superstep
    unsigned long primes_len = (sqrtMAX >> 6) + 1;
    uint32_t *primes = malloc(primes_len * sizeof(uint32_t));
    if (primes == NULL) {
        bsp_abort("WAAAAAAA, error while assigning memory\n");
    }
    bsp_push_reg(primes, sizeof(uint32_t) * primes_len);
    bsp_sync(); // sync is necessary
    if (s == 0) {
        // copy the found primes into the array.
        memcpy(primes, composite_array, sizeof(uint32_t) * primes_len);
        // make sure everything after sqrt(MAX) is not counted as prime,
        // this is not necessary but still nice to have.
        primes[primes_len - 1] |= (~1u << (index_sqrtMAX & 31));
        for (long t = 1; t < p; t++) {
            bsp_put(t, primes, primes, 0, sizeof(uint32_t) * primes_len);
        }
    }
    bsp_sync();
    bsp_pop_reg(primes);

    unsigned long start = (s * blocksize * (1 << 6)) | 1;
    unsigned long end = start + (composite_array_len << 6) - 2;
    // printf("s = %ld is responsible for %lu-%lu\n", s, start, end);
    bsp_sync();
    float start_time = bsp_time();
    // superstep 1: let all processors sieve their part of the array.
    for (unsigned long k = 0; k < primes_len; k++) {
        for (char r = 0; r < 32; r++) {
            if ((primes[k] & (1 << r)) == 0) {
                unsigned long q = r << 1 | 1 | (k << 6);
                unsigned long j = q * q;
                if (j < start) {
                    j = (start + q - 1) / q * q;
                    if ((j & 1) == 0) {
                        j += q;
                    }
                }
                for (; j <= end; j += (q << 1)) {
                    composite_array[(j - start) >> 6] |= 1 << ((j & 63) >> 1);
                }
            }
        }
    }
    bsp_sync();
    free(primes);

    // superstep 2: count all the primes;
    unsigned long local_count = 0;
    unsigned long *sum = calloc(p, sizeof(unsigned long));
    bsp_push_reg(sum, p * sizeof(unsigned long));
    bsp_sync();

    for (unsigned long j = 0; j < composite_array_len; j++) {
        for (char r = 0; r < 32; r++) {
            if ((composite_array[j] & (1 << r)) == 0) {
                local_count += 1;
            }
        }
    }
    for (long t = 0; t < p; t++)
        bsp_put(t, &local_count, sum, s * sizeof(unsigned long),
                sizeof(unsigned long));
    bsp_sync();
    unsigned long total = 1;
    for (long t = 0; t < p; t++) {
        total += sum[t];
    }
    bsp_pop_reg(sum);
    free(sum);

    printf("I, s=%ld, have found %lu primes\n", s, total);
    fflush(stdout);
    if (s == 0) {
        printf("Took %f\n", bsp_time() - start_time);
    }
    free(composite_array);
    bsp_end();
}

int main(int argc, char **argv) {
    bsp_init(bsp_main, argc, argv);
    MAX = strtol(argv[1], NULL, 10);
    MAX = closest_odd(MAX);
    bsp_main();
    return EXIT_SUCCESS;
}
