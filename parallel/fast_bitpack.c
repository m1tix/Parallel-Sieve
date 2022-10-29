#include <bsp.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// TODO
// sieving part is a mess, please see main.c

unsigned int P = 1;
unsigned long MAX;

/* closest_odd: returns the smallest odd integer closest to input. */
/* make sure that m >= 1, or else we've got an issue ;) */
unsigned long closest_odd(unsigned long n) {
    if ((n & 1) == 0) {
        return n - 1;
    } else {
        return n;
    }
}

/* ceildiv: calculates ceil(n/d) */
unsigned long ceildiv(unsigned long n, unsigned long d) {
    return (n + d - 1) / d;
}

/* returns a zero initialized array of length len of uint32_t */
uint32_t *vecalloc32(unsigned long len) {
    uint32_t *vec = calloc(len, sizeof(uint32_t));
    if (vec == NULL) {
        bsp_abort("error while assigning memory\n");
    }
    return vec;
}

/* sieve sieves all integers up to bound and returns
 * a bitpacked array whose t-th element and r-th bit is 1 iff
 * 2*r + 1 + 64t is composite. */
uint32_t *sieve(unsigned long bound) {
    bound = closest_odd(bound);
    unsigned long composite_list_len = (bound >> 6) + 1;
    uint32_t *composite_list = vecalloc32(composite_list_len);
    unsigned long sqrtbound = closest_odd(sqrt(bound));

    composite_list[composite_list_len - 1] = (~1ULL << ((bound & 63) >> 1));
    /* 1 is not prime ;) */
    composite_list[0] |= 1;
    for (unsigned long i = 3; i <= sqrtbound; i += 2) {
        if ((composite_list[i >> 6] & (1ULL << ((i & 63) >> 1))) == 0) {
            for (unsigned long j = i * i; j <= bound; j += i << 1) {
                composite_list[j >> 6] |= 1ULL << ((j & 63) >> 1);
            }
        }
    }

    return composite_list;
}

/* Unmark all elements of arr, which starts at the number arr_start, which are
 * a multiple of some prime in *primes */
void unmark_multiples(uint32_t *primes, unsigned long primes_len, uint32_t *arr,
                      unsigned long arr_start, unsigned long arr_end) {
    for (unsigned long k = 0; k < primes_len; k++) {
        for (char r = 0; r < 32; r++) {
            if ((primes[k] & (1 << r)) == 0) {
                unsigned long q = r << 1 | 1 | (k << 6);
                unsigned long j = q * q;
                if (j < arr_start) {
                    // calculate first multiple of q in array
                    j = ceildiv(arr_start, q) * q;
                    if ((j & 1) == 0) {
                        j += q;
                    }
                }
                j -= arr_start;
                for (; j <= arr_end - arr_start; j += (q << 1)) {
                    arr[j >> 6] |= 1 << ((j & 63) >> 1);
                }
            }
        }
    }
}

/* bulk of program */
void bsp_main(void) {
    bsp_begin(P);
    // basic bsp variables
    long s = bsp_pid();
    long p = bsp_nprocs();

    unsigned long full_array_len = (MAX >> 6) + 1;
    unsigned long composite_array_len = 0;
    unsigned long sqrtMAX = closest_odd(sqrt(MAX));
    unsigned long blocksize = ceildiv(full_array_len, p);

    // divide the full array equally among all processor
    if (s == (p - 1)) {
        composite_array_len = full_array_len - (p - 1) * blocksize;
    } else {
        composite_array_len = blocksize;
    }
    uint32_t *composite_array = vecalloc32(composite_array_len);
    // make sure all numbers after MAX are unaccounted for
    if (s == p - 1) {
        composite_array[composite_array_len - 1] |=
            (~1ULL << ((MAX & 63) >> 1));
    }
    unsigned long start = (s * blocksize * (1 << 6)) | 1;
    unsigned long end = start + (composite_array_len << 6) - 2;

    // primes below sqrtMAX
    unsigned long primes_len = (sqrtMAX >> 6) + 1;
    uint32_t *primes = vecalloc32(primes_len);
    bsp_push_reg(primes, sizeof(uint32_t) * primes_len);
    bsp_sync(); // sync is necessary

    double start_time = bsp_time();

    // superstep 0: proc s=0 calculates all primes below sqrt(MAX)
    if (s == 0) {
        // 1 is not prime ;)
        composite_array[0] |= 1;
        uint32_t *test = sieve(sqrtMAX);
        for (long t = 0; t < p; t++) {
            bsp_put(t, test, primes, 0, sizeof(uint32_t) * primes_len);
        }
        free(test);
    }
    bsp_sync();
    bsp_pop_reg(primes);

    // superstep 1: let all processors sieve their part of the array.
    unmark_multiples(primes, primes_len, composite_array, start, end);
    free(primes);
    bsp_sync();

    // print them primes baby
    // for (unsigned long i = 0; i < composite_array_len; i++) {
    //     for (char r = 0; r < 32; r++) {
    //         if ((composite_array[i] & (1 << (r & 31))) == 0) {
    //             printf("%lu\n", (r << 1) | 1 | (i << 6) + start);
    //             fflush(stdout);
    //         }
    //     }
    // }
    if (s == 0) {
        printf("This took %f\n", bsp_time() - start_time);
    }
    free(composite_array);
    bsp_end();
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Please specify an integer smaller than %lu\n", LONG_MAX);
        return 1;
    }

    bsp_init(bsp_main, argc, argv);
    MAX = strtol(argv[1], NULL, 10);
    MAX = closest_odd(MAX);

    if (argc >= 3) {
        P = strtol(argv[2], NULL, 10);
        if (P > bsp_nprocs()) {
            printf("Maximum number of processors is %u", bsp_nprocs());
            return 1;
        }
    }
    bsp_main();
    return 0;
}
