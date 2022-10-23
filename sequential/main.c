#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

void sieve_bitpack(unsigned long max);
void sieve_normal(unsigned long max);
void sieve_multsix(unsigned long max);
unsigned long closest_odd(unsigned long n);

// BUGS:
//      NONE;
// TODO:
//      sieve_fast should loop in multiples of 6 as every prime except
//                 2 and 3 are \pm 1 a multiple of 6!
//      maybe use uint_64t?

/* closest_odd: returns the smallest odd integer closest to input. */
/* make sure that m >= 1, or else we've got an issue ;) */
unsigned long closest_odd(unsigned long n) {
    if ((n & 1) == 0) {
        return n - 1;
    } else {
        return n;
    }
}

/* sieve_multsix: prints prime up to given maximum max
 * uses a char array to represent multiples of 6 up to difference of one
 * input should be bigger than 25 */
void sieve_multsix(unsigned long max) {
    /* an integer N that is +-1 mod 6 corresponds to the
     * N/3-th entry of char. We start from 1. */
    max = closest_odd(max);
    if (max % 6 == 3) {
        max = max - 2;
    }
    unsigned long len = (max / 3) + 1;
    char *arr = calloc(len, sizeof(char));
    if (arr == NULL) {
        puts("error assigning memory");
        abort();
    }
    unsigned long sqrtmax = closest_odd(sqrt(max));
    if (sqrtmax % 6 == 3) {
        sqrtmax = sqrtmax - 2;
    }
    arr[0] = 1;
    for (unsigned long i = 5; i <= sqrtmax; i += 6) {
        if (arr[i / 3] == 0) {
            // note that j = 1 mod 6 at start
            for (unsigned long j = i * i; j <= max; j += 6 * i) {
                arr[j / 3] = 1;
            }
            for (unsigned long j = i * i + 2 * i; j <= max; j += 6 * i) {
                arr[j / 3] = 1;
            }
        }
    }
    for (unsigned long i = 7; i <= sqrtmax; i += 6) {
        if (arr[i / 3] == 0) {
            // note that j = 1 mod 6 at start
            for (unsigned long j = i * i; j <= max; j += 6 * i) {
                arr[j / 3] = 1;
            }
            for (unsigned long j = i * i + 4 * i; j <= max; j += 6 * i) {
                arr[j / 3] = 1;
            }
        }
    }
    unsigned long count = 2;
    for (unsigned long i = 1; i < len; i++) {
        if (arr[i] == 0) {
            count++;
        }
    }
    free(arr);
    printf("primes found: %lu\n", count);
}
/* sieve_normal: prints prime up to given maximum max
 * uses a normal char array to represent odd numbers */
void sieve_normal(unsigned long max) {
    max = closest_odd(max);
    unsigned long len = (max >> 1) + 1;
    char *arr = calloc(len, sizeof(char));
    unsigned long sqrtmax = closest_odd(sqrt(max));
    if (arr == NULL) {
        puts("error assigning memory");
        abort();
    }
    arr[0] = 1;
    for (unsigned long i = 3; i <= sqrtmax; i += 2) {
        if (arr[i >> 1] == 0) {
            for (unsigned long j = i * i; j <= max; j += 2 * i) {
                arr[j >> 1] = 1;
            }
        }
    }

    // count them
    unsigned long count = 1;
    for (unsigned long i = 3; i <= max; i += 2) {
        if (arr[i >> 1] == 0)
            count++;
    }
    free(arr);
    printf("primes found: %lu\n", count);
}
/* sieve_bitpack: prints primes up to given maximum max
 * uses an array of uint32, where each bit is an odd number */
void sieve_bitpack(unsigned long max) {
    /* we first initialize an array of uint_32s
     * an odd number x>=3 then corresponds to the s-th bit in the t-th element
     * where
     *      s = ((x - 3) & 63) >> 1
     *      t = (x - 3) // 65
     *  This actually shows that its easier to include 1: this gets rid of -3 in
     * both eq! so we have (possibly) 32-bit extra storage for a big performance
     * boost! new values of s and t are thus
     *      s = (x & 63) >> 1
     *      t = x >> 6
     *  Hence the t-th element (t >= 0) stores all the odd integers in the
     * interval [2^6*t + 1, 2^6*(t+1) - 1]. It is then easy to see that we thus
     * need
     *      (max >> 6) + 1
     * elements in our array!
     */

    /* ensure max is odd */
    max = closest_odd(max);
    unsigned long composite_list_len = (max >> 7) + 1;
    uint64_t *composite_list = calloc(composite_list_len, sizeof(uint64_t));
    unsigned long sqrtmax = closest_odd(sqrt(max));

    /* make sure bits after max are set to 1 as they are not counted */
    /* this may not be bug free, might depend on size of unsigned int */
    composite_list[composite_list_len - 1] = (~1ULL << ((max & 127) >> 1));
    /* 1 is not prime ;) */
    composite_list[0] |= 1;

    for (unsigned long i = 3; i <= sqrtmax; i += 2) {
        if ((composite_list[i >> 7] & (1ULL << ((i & 127) >> 1))) == 0) {
            for (unsigned long j = i * i; j <= max; j += i << 1) {
                composite_list[j >> 7] |= 1ULL << ((j & 127) >> 1);
            }
        }
    }
    /* its printing time, 2 is hardcoded. */
    unsigned long count = 1;
    // puts("2");
    for (unsigned long k = 0; k < composite_list_len; k++) {
        unsigned long r = 0;
        while (r < 64) {
            if ((composite_list[k] & (1ULL << r)) == 0) {
                // this can be made faster, 2*r + 1 will always be smaller than
                // 1 << 6 unless k = 0. printf("%lu\n", 2 * r + 1 + k * (1 <<
                // 6));
                count++;
            }
            r++;
        }
    }
    free(composite_list);
    printf("primes found: %lu\n", count);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Please specify an integer smaller than %lu\n", LONG_MAX);
        return 1;
    }
    unsigned long bound = strtol(argv[1], NULL, 10);
    unsigned int flagopt = 0;
    if (argc >= 3) {
        flagopt = strtol(argv[2], NULL, 10);
    }
    /* some hand checking, very ugly :(*/
    if (bound < 2) {
        return 0;
    } else if (bound == 2) {
        printf("2\n");
        return 0;
    } else if (bound == 3) {
        printf("2\n3\n");
        return 0;
    }

    if (flagopt == 0) {
        puts("Using the bitpack version");
        sieve_bitpack(bound);
    } else if (flagopt == 1) {
        puts("Using the char array version");
        sieve_normal(bound);
    } else if (flagopt == 2) {
        puts("Using the 6-multiples array version");
        sieve_multsix(bound);
    } else {
        puts("Unrecognized flag option");
        abort();
    }
    return 0;
}
