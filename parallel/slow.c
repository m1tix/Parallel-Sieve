#include <bsp.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
// TODO
// If bit packing, primes are quite equally distributed mod 64.
// mfw this version is 3 times as slow as the sequential.
//      im gonna become the joker....
//  Might want to broadcast i in batches?
//  NEW SOLUTION JUST DROPPED
//      let s=0 do the prime finding prior,
//      i.e. sieve [1, sqrt(N)] on s = 0
//      send this array to all processors
//      let the processors sieve locally
//      and combine.
//      Assume for simplicity that sqrt(N) is actually in s = 0.

static unsigned int P = 4;
unsigned long MAX;

void bsp_main(void);
void bsp_sieve(unsigned long prime, char *sieve_array, unsigned long start,
               unsigned long size_sieve_array);

/* bsp_sieve does the actual sieving part,
 * i.e. change all elements divisible by q to be 1.
 * start signifies the starting element of the array while size_a is the length
 */
void bsp_sieve(unsigned long q, char *a, unsigned long start,
               unsigned long size_a) {
    unsigned long start_mod_q = start % q;
    unsigned long first_index = 0;
    // determine the first index of a such that the corresponding element is div
    // by q
    if (start_mod_q != 0) {
        // this works for now but ugly af.
        if (start_mod_q % 2 == 0) {
            first_index = (2 * q - start_mod_q) >> 1;
        } else {
            first_index = (q - start_mod_q) >> 1;
        }
        if (2 * first_index + start == q) {
            first_index += q;
        }
    }
    // printf("s=%u is currently sieving q=%lu at i=%lu\n", bsp_pid(), q,
    //        first_index);
    // fflush(stdout);
    for (unsigned long j = first_index; j < size_a; j += q) {
        a[j] = 1;
    }
}

void bsp_main(void) {
    /* bulk of program */
    bsp_begin(P);
    // bsp variables
    long s = bsp_pid();
    long p = bsp_nprocs();
    // other variables
    unsigned long full_array_len = (MAX >> 1) + 1;
    unsigned long len_local_array;
    unsigned long blocksize = (full_array_len + p - 1) / p;
    unsigned long sqrtmax = sqrt(MAX);
    unsigned long start_number = 2 * s * blocksize + 1;

    if (s == (p - 1)) {
        len_local_array = full_array_len - (p - 1) * blocksize;
    } else {
        len_local_array = blocksize;
    }
    char *local_array = calloc(len_local_array, sizeof(char));
    if (local_array == NULL) {
        bsp_abort("error has occured while assigning memory");
    }

    unsigned long i = 3;
    bsp_push_reg(&i, sizeof(unsigned long));
    double time_start = bsp_time();
    while (i <= sqrtmax) {
        bsp_sieve(i, local_array, start_number, len_local_array);
        if (s == 0) {
            i += 2; // kinda inefficient but necessary...
            while (local_array[i >> 1] == 1 && i <= sqrtmax) {
                i += 2;
            }
        }
        bsp_sync();
        if (s == 0) {
            for (long t = 1; t < p; t++) {
                bsp_put(t, &i, &i, 0, sizeof(unsigned long));
            }
        }
        bsp_sync();
    }
    bsp_pop_reg(&i);

    for (unsigned long t = 0; t < len_local_array; t++) {
        if (local_array[t] == 0) {
            printf("%lu\n", start_number + 2 * t);
            fflush(stdout);
        }
    }
    unsigned long local_count = 0;
    unsigned long *sum = calloc(p, sizeof(unsigned long));
    bsp_push_reg(sum, p * sizeof(unsigned long));
    bsp_sync();

    for (unsigned long j = 0; j < len_local_array; j++) {
        if (local_array[j] == 0) {
            local_count += 1;
        }
    }
    for (long t = 0; t < p; t++)
        bsp_put(t, &local_count, sum, s * sizeof(unsigned long),
                sizeof(unsigned long));
    free(local_array);
    printf("Took us %f\n", bsp_time() - time_start);
    fflush(stdout);
    bsp_sync();

    unsigned long total = 0;
    for (long t = 0; t < p; t++) {
        total += sum[t];
    }
    bsp_pop_reg(sum);
    free(sum);

    printf("I, s=%ld, have found %lu primes\n", s, total);
    fflush(stdout);
    bsp_end();
}

int main(int argc, char **argv) {
    bsp_init(bsp_main, argc, argv);
    MAX = strtol(argv[1], NULL, 10);
    MAX = (MAX % 2 == 0) ? MAX - 1 : MAX;
    bsp_main();
    return EXIT_SUCCESS;
}
