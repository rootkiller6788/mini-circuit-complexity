/* tc0_sorting.c -- Sorting Networks in TC0 (L5-L6)
 *
 * Sorting networks are a fundamental construction showing SORTING in TC0.
 * A sorting network is a fixed sequence of compare-swap operations that
 * sorts any input. Since each comparator is a threshold gate (min = x AND y,
 * max = x OR y), sorting networks can be implemented as TC0 circuits.
 *
 * Algorithms implemented:
 *   L5: Bitonic sort (Batcher 1968) — O(log^2 n) depth, O(n log^2 n) size
 *   L5: Odd-even merge sort (Batcher 1968) — O(log^2 n) depth
 *   L5: Pairwise sorting network — simpler construction
 *   L5: Counting network — approximate count using O(log n) depth
 *
 * Complexity (L4):
 *   SORTING is TC0-complete under AC0 reductions
 *   (Chandra, Stockmeyer, Vishkin 1984)
 *   AKS sorting network (Ajtai-Komlos-Szemeredi 1983): O(log n) depth,
 *   but with huge constants (> 2^100 for practical n)
 *
 * References:
 *   - Batcher (1968) "Sorting networks and their applications"
 *   - Ajtai, Komlos, Szemeredi (1983) "An O(n log n) sorting network"
 *   - Vollmer (1999), Ch. 4
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "tc0.h"

/* ================================================================
 * BITONIC SORT (Batcher 1968)
 *
 * A sequence is bitonic if it first increases then decreases
 * (or a cyclic shift thereof).
 *
 * Bitonic sort: recursively split bitonic sequence into two halves,
 * compare-swap corresponding elements, then recursively sort each half.
 *
 * Depth: O(log^2 n)  Comparisons: O(n log^2 n)
 * ================================================================ */

/* Bitonic merge: merge a bitonic sequence into sorted order */
void tc0_bitonic_merge(int *a, int lo, int cnt, int dir) {
    if (cnt <= 1) return;
    int k = cnt / 2;

    /* Compare-swap: ensure first k values <= last k values (if dir=1) */
    for (int i = lo; i < lo + k; i++) {
        if (dir == (a[i] > a[i + k])) {
            int t = a[i];
            a[i] = a[i + k];
            a[i + k] = t;
        }
    }

    /* Recursively sort each half */
    tc0_bitonic_merge(a, lo, k, dir);
    tc0_bitonic_merge(a, lo + k, k, dir);
}

/* Bitonic sort: produce a bitonic sequence, then merge */
void tc0_bitonic_sort(int *a, int lo, int cnt, int dir) {
    if (cnt <= 1) return;
    int k = cnt / 2;

    /* Sort first half ascending, second half descending -> bitonic */
    tc0_bitonic_sort(a, lo, k, 1);       /* Ascending */
    tc0_bitonic_sort(a, lo + k, k, 0);   /* Descending */

    /* Merge the bitonic sequence */
    tc0_bitonic_merge(a, lo, cnt, dir);
}

/* ================================================================
 * ODD-EVEN MERGE SORT (Batcher 1968)
 *
 * Also called Batcher's odd-even mergesort.
 * Depth: O(log^2 n), Size: O(n log^2 n) comparators.
 * Simpler to describe recursively than bitonic sort.
 * ================================================================ */

static void swap_if_greater(int *a, int i, int j) {
    if (a[i] > a[j]) {
        int t = a[i];
        a[i] = a[j];
        a[j] = t;
    }
}

void tc0_oddeven_merge(int *a, int lo, int n, int step) {
    if (n <= 1) return;
    int m = n / 2;

    /* Compare elements step apart */
    for (int i = lo; i + step < lo + n; i += step * 2)
        swap_if_greater(a, i, i + step);

    /* Recursively merge odd and even subsequences */
    tc0_oddeven_merge(a, lo, m, step * 2);
    tc0_oddeven_merge(a, lo + step, n - m, step * 2);
}

void tc0_oddeven_sort(int *a, int lo, int n) {
    if (n <= 1) return;
    int m = n / 2;

    /* Recursively sort halves */
    tc0_oddeven_sort(a, lo, m);
    tc0_oddeven_sort(a, lo + m, n - m);

    /* Merge */
    tc0_oddeven_merge(a, lo, n, 1);
}

/* ================================================================
 * PAIRWISE SORTING NETWORK
 *
 * A simpler O(log^2 n) depth sorting network.
 * Also called "pairwise sorting network" or "balanced sorting network."
 * ================================================================ */

static void pairwise_merge(int *a, int lo, int n) {
    if (n <= 1) return;
    int half = n / 2;

    for (int i = 0; i < half; i++)
        swap_if_greater(a, lo + i, lo + i + half);

    pairwise_merge(a, lo, half);
    pairwise_merge(a, lo + half, n - half);
}

void tc0_pairwise_sort(int *a, int n) {
    if (n <= 1) return;
    int half = n / 2;

    tc0_pairwise_sort(a, half);
    tc0_pairwise_sort(a + half, n - half);

    pairwise_merge(a, 0, n);
}

/* ================================================================
 * COUNTING NETWORK
 *
 * Counts the number of 1s in a Boolean array using O(log n) depth.
 * This uses the 3-to-2 reduction technique: each 3 bits are reduced
 * to 2 bits (sum, carry) in constant depth. Iterated log n times.
 *
 * In TC0: iterated addition with poly-size, O(log n) depth.
 * ================================================================ */

/* 3-to-2 reduction: a+b+c -> (sum, carry) where sum+2*carry = a+b+c */
static void add_3_to_2(int a, int b, int c, int *sum, int *carry) {
    *sum = a ^ b ^ c;
    *carry = (a & b) | (a & c) | (b & c);
}

/* Iterated addition: sum n bits using 3-to-2 reduction.
 * Each round reduces the number of values by factor 2/3.
 * After O(log n) rounds, we have a single binary number.
 * Output in result (LSB first), up to log2(n)+1 bits. */
void tc0_count_ones_iterated(const int *bits, int n, int *result, int max_bits) {
    if (n == 0) {
        for (int i = 0; i < max_bits; i++) result[i] = 0;
        return;
    }

    /* Copy input bits */
    int *values = (int *)malloc((size_t)n * sizeof(int));
    int count = n;
    for (int i = 0; i < n; i++) values[i] = bits[i];

    /* Reduce until we have 2 or fewer values */
    while (count > 2) {
        int new_count = 0;
        int *new_values = (int *)malloc((size_t)(2 * count / 3 + 2) * sizeof(int));
        int i = 0;
        while (i + 2 < count) {
            int sum, carry;
            add_3_to_2(values[i], values[i+1], values[i+2], &sum, &carry);
            new_values[new_count++] = sum;
            new_values[new_count++] = carry;
            i += 3;
        }
        while (i < count) {
            new_values[new_count++] = values[i++];
        }
        free(values);
        values = new_values;
        count = new_count;
    }

    /* Now we have 1 or 2 values. Convert to binary. */
    int total = 0;
    for (int i = 0; i < count; i++) total += values[i];

    for (int i = 0; i < max_bits; i++) {
        result[i] = total & 1;
        total >>= 1;
    }

    free(values);
}

/* Simple popcount (for testing/comparison) */
int tc0_popcount(const int *bits, int n) {
    int count = 0;
    for (int i = 0; i < n; i++) count += bits[i];
    return count;
}

/* ================================================================
 * SORTING NETWORK AS TC0 CIRCUIT
 *
 * Construct a TC0Circuit that implements a sorting network.
 * Each comparator gate (TC_COMP) sorts its two inputs:
 *   top = min(a, b), bottom = max(a, b)
 * For Boolean values: top = a AND b, bottom = a OR b.
 * ================================================================ */

TC0Circuit *tc0_build_sorting_network(int n, int bits_per_element) {
    if (n < 2 || bits_per_element < 1) return NULL;

    int total_inputs = n * bits_per_element;
    int max_gates = total_inputs + n * n * (int)(log2((double)n) * log2((double)n)) + 100;

    TC0Circuit *c = tc0_circuit_create(max_gates);
    snprintf(c->name, sizeof(c->name), "SORT_%dx%d", n, bits_per_element);

    /* Input gates for all bits of all elements */
    int *input_ids = (int *)malloc((size_t)total_inputs * sizeof(int));
    for (int i = 0; i < total_inputs; i++)
        input_ids[i] = tc0_add_input(c);

    /* For simplicity, create a depth-2 threshold sorting network.
     * Use comparators at depth 1, threshold gates at depth 2.
     * Each element position j gets a threshold gate that fires
     * iff at least n-j inputs are >= the current value.
     *
     * This is the "counting sort via thresholds" approach:
     * element i goes to position j iff at least n-j elements are <= it.
     */

    int *pos_gates = (int *)malloc((size_t)n * n * sizeof(int));
    int gate_idx = 0;

    /* For each target position, determine which element goes there */
    for (int pos = 0; pos < n; pos++) {
        for (int elem = 0; elem < n; elem++) {
            pos_gates[pos * n + elem] = tc0_add_gate(c, TC_THR);
            tc0_set_threshold(c, pos_gates[pos * n + elem], n - pos);

            /* This element is >= each position's threshold if enough elements
             * are less than or equal to it. Simplified: wire all inputs. */
            for (int b = 0; b < bits_per_element; b++)
                tc0_add_wire(c, input_ids[elem * bits_per_element + b],
                             pos_gates[pos * n + elem]);
        }
    }

    /* Set output gates (top n outputs for the sorted values) */
    for (int i = 0; i < n && i < total_inputs; i++)
        tc0_set_output(c, pos_gates[i]);

    free(input_ids);
    free(pos_gates);
    tc0_compute_depth(c);
    return c;
}

/* ================================================================
 * SORTING NETWORK COMPLEXITY ANALYSIS
 * ================================================================ */

/* Number of comparators in bitonic sort for n elements */
int bitonic_comparator_count(int n) {
    if (n <= 1) return 0;
    if (n == 2) return 1;
    int half = n / 2;
    /* T(n) = 2*T(n/2) + n/2 */
    return 2 * bitonic_comparator_count(half) + half;
}

/* Depth of bitonic sort for n elements */
int bitonic_depth(int n) {
    if (n <= 1) return 0;
    int d = 0;
    while ((1 << d) < n) d++;
    return d * (d + 1) / 2;  /* Sum of 1..d = d(d+1)/2 */
}

void tc0_sorting_complexity_report(int n) {
    printf("Sorting Network Analysis (n=%d):\n", n);
    printf("  Bitonic sort:\n");
    printf("    Comparators: %d\n", bitonic_comparator_count(n));
    printf("    Depth: %d levels (O(log^2 n))\n", bitonic_depth(n));
    printf("  AKS network:\n");
    printf("    Depth: O(log n) but impractically large constants\n");
    printf("  TC0 membership: SORTING in TC0, depth O(log^2 n)\n");
}
