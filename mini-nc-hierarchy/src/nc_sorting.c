/* nc_sorting.c -- Parallel Sorting Networks (NC)
 *
 * Sorting networks are a key NC primitive. They sort using only
 * compare-swap operations with no data-dependent control flow.
 *
 * Sorting networks belong to the comparator circuit model:
 *   - Each comparator takes two inputs and outputs them in sorted order
 *   - Network depth = number of parallel comparator layers
 *
 * Kinds of sorting networks:
 *   Bitonic sort (Batcher 1968): depth O(log^2 n), size O(n log^2 n)
 *   Odd-even merge (Batcher 1968): depth O(log n) merge, O(log^2 n) sort
 *   AKS (Ajtai-Komlós-Szemerédi 1983): depth O(log n) — optimal depth
 *     but huge constant (~2^1000) — impractical
 *   Goodrich (2014): zig-zag sort, O(log n) depth, practical constant
 *
 * Relationship to circuit complexity:
 *   Sorting ∈ TC^0 (via threshold gates for counting)
 *   Sorting ∈ NC^1 (via Batcher networks: O(log^2 n) depth for comparison
 *     model; but Boolean circuit depth for sorting is O(log n)?)
 *   The AKS network shows sorting has O(log n) comparator depth
 *
 * Reference:
 *   Batcher, K.E. (1968) "Sorting networks and their applications"
 *   Ajtai, Komlós, Szemerédi (1983) "An O(n log n) sorting network"
 *   Knuth, D.E. (1998) "The Art of Computer Programming, Vol. 3"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "nc.h"

/* ================================================================
 * COMPARE-AND-SWAP PRIMITIVES
 * ================================================================ */

void compare_swap(int *a, int *b) {
    if (*a > *b) { int t = *a; *a = *b; *b = t; }
}

void compare_swap_desc(int *a, int *b) {
    if (*a < *b) { int t = *a; *a = *b; *b = t; }
}

/* ================================================================
 * BITONIC SORT (Batcher 1968)
 *
 * A sequence is bitonic if it first increases then decreases
 * (or vice versa, under cyclic rotation).
 *
 * Bitonic sort:
 *   1. Arrange data into bitonic sequences
 *   2. Recursively merge bitonic sequences
 *
 * Depth:   O(log^2 n)  (log n stages, each O(log n) deep)
 * Size:    O(n log^2 n)
 * Class:   NC^2 (depth O(log^2 n) with bounded fan-in comparators)
 * ================================================================ */

/* Bitonic merge: merge a bitonic sequence of length n into sorted order.
 * Requires n to be a power of 2.
 * dir=1 for ascending, dir=0 for descending. */
void bitonic_merge(int *a, int n, int dir) {
    if (n <= 1) return;
    int half = n / 2;
    for (int i = 0; i < half; i++) {
        if (dir) compare_swap(&a[i], &a[i + half]);
        else     compare_swap_desc(&a[i], &a[i + half]);
    }
    bitonic_merge(a, half, dir);
    bitonic_merge(a + half, half, dir);
}

/* Bitonic sort: recursively build and merge bitonic sequences.
 * dir=1: ascending, dir=0: descending. */
void bitonic_sort_rec(int *a, int n, int dir) {
    if (n <= 1) return;
    int half = n / 2;
    bitonic_sort_rec(a, half, 1);           /* first half: ascending */
    bitonic_sort_rec(a + half, half, 0);    /* second half: descending */
    bitonic_merge(a, n, dir);               /* merge the bitonic sequence */
}

/* Top-level bitonic sort: sorts n elements ascending.
 * n must be a power of 2. For arbitrary n, pad to next power of 2. */
void bitonic_sort(int *a, int n) {
    /* Find next power of 2 */
    int m = 1;
    while (m < n) m <<= 1;

    if (m == n) {
        /* Exact power of 2 */
        bitonic_sort_rec(a, n, 1);
    } else {
        /* Pad with sentinel values */
        int *pad = (int *)malloc((size_t)m * sizeof(int));
        assert(pad);
        memcpy(pad, a, (size_t)n * sizeof(int));
        for (int i = n; i < m; i++) pad[i] = INT_MAX;
        bitonic_sort_rec(pad, m, 1);
        memcpy(a, pad, (size_t)n * sizeof(int));
        free(pad);
    }
}

/* ================================================================
 * ODD-EVEN MERGE (Batcher 1968)
 *
 * Odd-even merge sorts two sorted sequences of length n/2 each.
 * Used as the merge step in odd-even merge sort.
 *
 * Merge depth:   O(log n)
 * Sort depth:    O(log^2 n)
 * ================================================================ */

/* Merge two sorted halves of a[0..n-1] using odd-even transposition.
 * This is a parallel merge network: each step is a set of independent
 * compare-swap operations. */
void odd_even_merge(int *a, int n) {
    if (n <= 1) return;

    /* Separate into odd and even indexed elements */
    int *odd = (int *)malloc((size_t)((n + 1) / 2) * sizeof(int));
    int *even = (int *)malloc((size_t)(n / 2) * sizeof(int));
    assert(odd && even);

    for (int i = 0; i < n; i++) {
        if (i % 2 == 0) even[i / 2] = a[i];
        else            odd[i / 2] = a[i];
    }

    /* Recursively merge */
    odd_even_merge(odd, (n + 1) / 2);
    odd_even_merge(even, n / 2);

    /* Interleave back */
    for (int i = 0; i < n; i++) {
        if (i % 2 == 0) a[i] = even[i / 2];
        else            a[i] = odd[i / 2];
    }

    /* Compare-swap adjacent pairs */
    for (int i = 1; i < n - 1; i += 2)
        compare_swap(&a[i], &a[i + 1]);

    free(odd); free(even);
}

/* Odd-even merge sort: recursively sort halves, then merge */
void odd_even_merge_sort(int *a, int n) {
    if (n <= 1) return;
    int mid = n / 2;
    odd_even_merge_sort(a, mid);
    odd_even_merge_sort(a + mid, n - mid);
    odd_even_merge(a, n);
}

/* ================================================================
 * PAIRWISE SORTING NETWORK
 *
 * Pairwise sorting networks (Parberry 1992) offer a simpler alternative
 * to odd-even networks with similar depth O(log^2 n).
 * ================================================================ */

/* Pairwise comparison network for n=power of 2 */
static void pairwise_network(int *a, int n, int step) {
    if (n <= 1) return;
    int half = n / 2;
    /* Compare pairs separated by half */
    for (int i = 0; i < half; i++)
        compare_swap(&a[i], &a[i + half]);
    pairwise_network(a, half, step + 1);
    pairwise_network(a + half, half, step + 1);
}

/* Pairwise sort: uses divide-and-conquer with pairwise comparisons */
void pairwise_sort(int *a, int n) {
    /* First build pairwise sorted pairs at distance n/2, n/4, ..., 1 */
    for (int dist = n / 2; dist >= 1; dist /= 2) {
        for (int i = 0; i < n - dist; i++) {
            if ((i / dist) % 2 == 0)
                compare_swap(&a[i], &a[i + dist]);
        }
    }
}

/* ================================================================
 * AKS SORTING NETWORK (Demonstration)
 *
 * AKS (Ajtai-Komlós-Szemerédi 1983) proved that O(log n) depth
 * sorting networks exist. However, the constant factor is enormous
 * (estimated ~2^1000). This is a proof-of-concept demonstration
 * using a simplified O(log^2 n) network.
 *
 * The actual AKS construction uses:
 *   1. ε-halvers: networks that approximately split data
 *   2. Expander graphs for the comparator connections
 *   3. Recursive composition
 *
 * Here we demonstrate the structure without the expander construction.
 * ================================================================ */

void aks_sort_demonstration(int *a, int n) {
    /* For demonstration, use bitonic sort as a stand-in.
     * The depth is O(log^2 n) — close to but not matching
     * the optimal O(log n) of AKS. But it illustrates the concept:
     * sorting is in NC. */
    bitonic_sort(a, n);
}

/* ================================================================
 * SORT VERIFICATION
 * ================================================================ */

/* Verify array is sorted ascending */
static int is_sorted(const int *a, int n) {
    for (int i = 1; i < n; i++)
        if (a[i - 1] > a[i]) return 0;
    return 1;
}

/* Verify array equals sorted version of original (permutation check) */
static int is_permutation_of(const int *a, const int *orig, int n) {
    /* Count frequencies */
    int *cnt_a = (int *)calloc(10000, sizeof(int));
    int *cnt_o = (int *)calloc(10000, sizeof(int));
    assert(cnt_a && cnt_o);
    for (int i = 0; i < n; i++) {
        if (a[i] >= 0 && a[i] < 10000) cnt_a[a[i]]++;
        if (orig[i] >= 0 && orig[i] < 10000) cnt_o[orig[i]]++;
    }
    for (int i = 0; i < 10000; i++)
        if (cnt_a[i] != cnt_o[i]) { free(cnt_a); free(cnt_o); return 0; }
    free(cnt_a); free(cnt_o);
    return 1;
}

/* ================================================================
 * DEMONSTRATION
 * ================================================================ */

void bitonic_sort_demo(void) {
    printf("\n================================================================\n");
    printf("  PARALLEL SORTING NETWORKS (NC)\n");
    printf("================================================================\n\n");

    printf("Sorting networks: data-independent parallel comparison model.\n");
    printf("  - Bitonic Sort: O(log^2 n) depth, O(n log^2 n) size (NC^2)\n");
    printf("  - Odd-Even Merge: O(log^2 n) depth (NC^2)\n");
    printf("  - AKS Network: O(log n) depth (optimal, but huge constant)\n");
    printf("  - Sorting ∈ TC^0 (via threshold gates)\n\n");

    /* Test all sorting algorithms on small arrays */
    printf("--- Correctness Tests ---\n");

    for (int n = 4; n <= 32; n *= 2) {
        int *orig = (int *)malloc((size_t)n * sizeof(int));
        int *a = (int *)malloc((size_t)n * sizeof(int));
        assert(orig && a);

        /* Fill with pseudo-random values */
        for (int i = 0; i < n; i++) orig[i] = (i * 997 + 173) % 1000;
        memcpy(a, orig, (size_t)n * sizeof(int));
        bitonic_sort(a, n);
        printf("  n=%2d bitonic:    %s\n", n,
               (is_sorted(a, n) && is_permutation_of(a, orig, n)) ? "OK" : "FAIL");

        memcpy(a, orig, (size_t)n * sizeof(int));
        odd_even_merge_sort(a, n);
        printf("  n=%2d odd-even:   %s\n", n,
               (is_sorted(a, n) && is_permutation_of(a, orig, n)) ? "OK" : "FAIL");

        memcpy(a, orig, (size_t)n * sizeof(int));
        pairwise_sort(a, n);
        printf("  n=%2d pairwise:   %s\n", n,
               (is_sorted(a, n) && is_permutation_of(a, orig, n)) ? "OK" : "FAIL");

        free(orig); free(a);
    }

    /* Depth and size analysis */
    printf("\n--- Asymptotic Complexity ---\n");
    printf("  n        logn    log^2n   n*log^2n   (depth)   (size)\n");
    printf("  -------- ------- -------- ----------  --------  ---------\n");
    for (int n = 4; n <= 4096; n *= 2) {
        double ln = log2((double)n);
        printf("  %-8d %-7.1f %-8.1f %-10.0f\n", n, ln, ln*ln, n*ln*ln);
    }

    printf("\n--- Sorting Networks in the NC Hierarchy ---\n");
    printf("  Sorting networks (comparator model):\n");
    printf("    AKS depth = O(log n) → sorting network in NC^1?\n");
    printf("    But comparator fan-out is O(1), fan-in is 2.\n");
    printf("    Boolean circuit depth for sorting with AND/OR/NOT\n");
    printf("    gates is O(log n) via AKS? No — comparator model\n");
    printf("    has unbounded fan-out, not the same as Boolean circuit.\n");
    printf("\n");
    printf("  Known: Sorting as a Boolean function is in TC^0.\n");
    printf("  Using MAJORITY gates: count how many inputs are less\n");
    printf("  than each threshold → depth 2 threshold circuit.\n");
    printf("  This demonstrates TC^0 power over AC^0/NC^0.\n");
}
