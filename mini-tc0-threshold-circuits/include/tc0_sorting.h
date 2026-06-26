/* tc0_sorting.h -- Sorting Networks in TC0 (L5-L6)
 *
 * Sorting networks are a key construction proving SORTING in TC0.
 * Each comparator = threshold gate (min = AND, max = OR).
 */

#ifndef TC0_SORTING_H
#define TC0_SORTING_H

#include "tc0.h"

/* ================================================================
 * SORTING ALGORITHMS
 * ================================================================ */

/* Bitonic sort (Batcher 1968): O(log^2 n) depth, O(n log^2 n) size.
 * Sorts array a[lo..lo+cnt-1] in direction dir (1=ascending, 0=descending).
 * Requires cnt to be a power of 2. */
void tc0_bitonic_sort(int *a, int lo, int cnt, int dir);
void tc0_bitonic_merge(int *a, int lo, int cnt, int dir);

/* Odd-even merge sort (Batcher 1968): O(log^2 n) depth.
 * Works for any n (not just powers of 2). */
void tc0_oddeven_sort(int *a, int lo, int n);
void tc0_oddeven_merge(int *a, int lo, int n, int step);

/* Pairwise sorting network: simpler O(log^2 n) construction. */
void tc0_pairwise_sort(int *a, int n);

/* ================================================================
 * COUNTING AND ITERATED ADDITION
 * ================================================================ */

/* Count ones in Boolean array using iterated 3-to-2 reduction.
 * In TC0: O(log n) depth. */
void tc0_count_ones_iterated(const int *bits, int n, int *result,
                              int max_bits);
int tc0_popcount(const int *bits, int n);

/* ================================================================
 * SORTING NETWORK AS TC0 CIRCUIT
 * ================================================================ */

/* Build a TC0 circuit implementing a sorting network for n elements,
 * each having bits_per_element bits. */
TC0Circuit *tc0_build_sorting_network(int n, int bits_per_element);

/* ================================================================
 * COMPLEXITY ANALYSIS
 * ================================================================ */

/* Number of comparators in a bitonic sort of n elements */
int bitonic_comparator_count(int n);

/* Depth (number of parallel comparator stages) in bitonic sort */
int bitonic_depth(int n);

/* Print complexity summary for sorting networks of size n */
void tc0_sorting_complexity_report(int n);

#endif /* TC0_SORTING_H */
