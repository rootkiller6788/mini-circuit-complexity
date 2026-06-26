/**
 * src/parity_lower.c — PARITY Lower Bound Implementation
 * =====================================================
 *
 * Implements L4-L6: PARITY complexity analysis and the full
 * Hastad lower bound proof simulation.
 *
 * KEY THEOREM (Furst-Saxe-Sipser 1981, Ajtai 1983, Hastad 1986):
 *   PARITY ∉ AC0. Any constant-depth, polynomial-size circuit
 *   cannot compute the PARITY function on n variables.
 *
 *   Hastad's bound: size ≥ exp(Ω(n^{1/(d-1)})) for depth d.
 *   This is SUPER-POLYNOMIAL for constant d.
 *
 * COURSE MAPPING:
 *   - MIT 6.841: AC0 lower bounds
 *   - Stanford CS358: PARITY lower bound via switching lemma
 *   - Berkeley CS278: Circuit lower bounds
 *   - CMU 15-855: Hastad's switching lemma applications
 *
 * REFERENCES:
 *   - Hastad (1986), "Almost optimal lower bounds..."
 *   - Arora & Barak (2009), Chapter 14
 *   - Furst, Saxe, Sipser (1981), "Parity, circuits, and the polynomial hierarchy"
 */
#include "switching.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/**
 * parity_dnf_size — DNF size for PARITY on n variables.
 *
 * PARITY DNF needs 2^{n-1} terms (all odd-weight assignments).
 * Each term has width n (all n literals).
 *
 * @param n  Number of variables
 * @return   2^{n-1}, or -1 if n ≥ 63 (overflow)
 */
long long parity_dnf_size(int n) {
    if (n <= 0) return 1;
    if (n >= 63) return -1;
    return 1LL << (n - 1);
}

/**
 * parity_dt_depth — Decision tree depth for PARITY on n variables.
 *
 * PARITY requires querying ALL n variables because changing any
 * single bit flips the output. Thus decision tree depth = n.
 *
 * @param n  Number of variables
 * @return   n (always)
 */
int parity_dt_depth(int n) { return n; }

/**
 * parity_complexity_summary — Print complexity summary for PARITY.
 */
void parity_complexity_summary(int n) {
    printf("\n=== PARITY Complexity (n=%d) ===\n\n", n);
    printf("DNF size = 2^{%d} = %lld terms\n", n-1, parity_dnf_size(n));
    printf("DT depth = %d (must query ALL variables)\n", n);
    printf("AC0[2] (with XOR): O(n) size, O(1) depth\n");
}

/**
 * hastad_bound_is_super_polynomial — Check if Hastad's bound is super-polynomial.
 *
 * The bound is super-polynomial if size_lower > n^{10}.
 * For constant depth d, this typically holds for all n ≥ 2.
 *
 * @param n  Number of variables
 * @param d  Circuit depth
 * @return   1 if super-polynomial, 0 otherwise
 */
int hastad_bound_is_super_polynomial(int n, int d) {
    double bound = hastad_parity_lower_bound(n, d);
    double poly_threshold = pow((double)n, 10.0);
    return (bound > poly_threshold) ? 1 : 0;
}

/**
 * parity_ac0_size_vs_depth — Print lower bound table.
 */
void parity_ac0_size_vs_depth(int nmax) {
    printf("\n=== PARITY Lower Bound: Size vs Depth ===\n\n");
    printf("Hastad (1986): size >= 2^{n^{1/(d-1)}/10}\n\n");
    printf("%6s  %14s  %14s  %14s  %14s\n", "n", "d=2", "d=3", "d=4", "d=5");
    int ns[] = {16, 32, 64, 128, 256, 512, 1024};
    for (int i = 0; i < 7; i++) {
        int n = ns[i]; if (n > nmax) continue;
        printf("%6d", n);
        for (int d = 2; d <= 5; d++) {
            double lb = hastad_parity_lower_bound(n, d);
            if (lb > 1e60) printf("  %14s", ">1e60");
            else printf("  %14.1e", lb);
        }
        printf("\n");
    }
}

/**
 * verify_parity_dnf_size — Verify DNF size for PARITY on m variables.
 *
 * Constructs the DNF from truth table and checks that it has
 * exactly 2^{m-1} terms. Only feasible for m ≤ 10.
 *
 * @param m  Number of variables
 * @return   1 if verification passes, 0 otherwise
 */
int verify_parity_dnf_size(int m) {
    if (m > 10) { printf("  m=%d too large for brute force\n", m); return 1; }
    int total = 1 << m;
    int* table = (int*)malloc((size_t)total * sizeof(int));
    if (!table) return 0;
    for (int x = 0; x < total; x++) {
        int parity = 0;
        for (int i = 0; i < m; i++) parity ^= ((x >> i) & 1);
        table[x] = parity;
    }
    BoolFunc bf; bf.n_vars = m; bf.table = table; bf.table_size = total;
    DNF* d = bf_to_dnf(&bf);
    if (!d) { free(table); return 0; }
    int actual = dnf_term_count(d);
    long long expected = parity_dnf_size(m);
    int ok = (actual == (int)expected);
    printf("  PARITY(%d): DNF has %d terms (expected %lld) %s\n",
           m, actual, expected, ok ? "OK" : "FAIL");
    dnf_free(d); free(table);
    return ok;
}

/**
 * parity_lower_bound_full_simulation — Full Hastad proof simulation.
 *
 * Simulates the complete proof:
 *   1. Choose p = n^{-1/(d-1)}
 *   2. Apply d-1 switching rounds
 *   3. Check PARITY DNF size on surviving variables
 *   4. Derive lower bound
 *
 * @param n      Number of variables
 * @param depth  Circuit depth
 */
void parity_lower_bound_full_simulation(int n, int depth) {
    printf("\n=== HASTAD PROOF SIMULATION (n=%d, d=%d) ===\n\n", n, depth);
    double p = pow((double)n, -1.0 / (double)(depth - 1));
    printf("Step 1: p = n^{-1/(d-1)} = %.6f\n", p);
    printf("  After %d rounds: ~%.1f variables survive\n\n",
           depth - 1, (double)n * pow(p, (double)(depth - 1)));
    DepthReduction* dr = depth_reduction_simulate(n, depth);
    if (!dr) return;
    depth_reduction_print(dr);
    int m = dr->final_vars;
    long long dnf_sz = parity_dnf_size(m);
    printf("\nStep 3: PARITY on %d vars needs DNF size 2^{%d} = %lld\n",
           m, m - 1, dnf_sz);
    double lb = hastad_parity_lower_bound(n, depth);
    printf("\nStep 4: Lower Bound = %.1e\n", lb);
    printf("  This is %s.\n",
           hastad_bound_is_super_polynomial(n, depth)
           ? "SUPER-POLYNOMIAL (PARITY not in AC0)"
           : "polynomial for these parameters");
    depth_reduction_free(dr);
}

/**
 * parity_lower_bound_demo — Full PARITY lower bound demonstration.
 *
 * Shows:
 *   - PARITY complexity summary
 *   - DNF size verification for small n
 *   - Lower bound table
 *   - Full proof simulation for sample parameters
 */
void parity_lower_bound_demo(void) {
    printf("\n================================================================\n");
    printf("  PARITY LOWER BOUND via HASTAD SWITCHING LEMMA\n");
    printf("  Furst-Saxe-Sipser (1981) + Ajtai (1983) + Hastad (1986)\n");
    printf("  Godel Prize 1994\n");
    printf("================================================================\n");

    parity_complexity_summary(8);

    printf("\n--- DNF Size Verification ---\n");
    for (int m = 2; m <= 8; m++) verify_parity_dnf_size(m);

    parity_ac0_size_vs_depth(1024);

    parity_lower_bound_full_simulation(64, 3);
    parity_lower_bound_full_simulation(256, 4);

    printf("\nCONCLUSION: PARITY is NOT in AC0. First circuit lower bound.\n");
}
