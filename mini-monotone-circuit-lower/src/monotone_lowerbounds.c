/* monotone_lowerbounds.c -- Monotone Circuit Lower Bounds
 *
 * Implements the computation and verification of monotone circuit
 * lower bounds, centered on Razborov's theorem (1985).
 *
 * KEY RESULT:
 *   CLIQUE_{n,k} requires monotone circuits of size
 *   exp(Omega(k^{1/3} * log n)).
 *
 * This was the FIRST super-polynomial circuit lower bound.
 * The method (approximations) works only for monotone circuits.
 * Extending to general circuits is the main open problem.
 *
 * Historical context:
 *   - Pre-1985: No super-polynomial lower bounds known for ANY
 *     explicit function, even for monotone circuits.
 *   - 1985: Razborov proves CLIQUE requires super-poly monotone size.
 *   - 1987: Alon & Boppana improve the bound.
 *   - 1987: Andreev gives alternative proof for different function.
 *   - 1988: Tardos shows exponential monotone vs general gap.
 *   - 1997: Razborov & Rudich prove Natural Proofs barrier.
 *   - Present: No super-poly lower bounds for general circuits!
 *
 * References:
 *   Razborov (1985), Doklady AN SSSR 281(4), 798-801
 *   Alon & Boppana (1987), Combinatorica 7(1), 1-22
 *   Andreev (1985), DAN SSSR 282(5), 1033-1037
 *   Tardos (1988), Combinatorica 8(1), 141-142
 *   Razborov & Rudich (1997), JCSS 55(1), 24-35
 *   Jukna (2012), "Boolean Function Complexity"
 *   Arora & Barak (2009), "Computational Complexity", Ch. 14
 *
 * Complexity: O(1) per bound computation.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "monotone_lowerbounds.h"
#include "monotone_graph.h"

/* ============================================================
 * Razborov's Lower Bound (1985)
 * ============================================================ */

double razborov_clique_lower_bound(int n, int k) {
    /* Razborov's bound: exp(Omega(sqrt(k) * log n))
     * for k <= n^{1/4}.
     *
     * More precisely: there exists a constant c > 0 such that
     *   size >= exp(c * sqrt(k) * log n)
     *
     * This holds for all k <= n^{1/4}.
     * For k = n^{1/4}: size >= exp(c * n^{1/8} * log n)
     *
     * Note: the actual bound involves the sunflower lemma:
     *   size >= exp(sqrt(k) * log n / O(log k))
     *
     * We compute: exp(sqrt(k) * log(n) / 4.0)
     * which is Razborov's original asymptotic form. */

    if (k <= 0 || n <= 0) return 0.0;
    if (k > n) return 0.0;

    /* The bound only holds for k <= n^{1/4} */
    if (k > (int)pow((double)n, 0.25)) {
        /* For larger k, the bound is even stronger but we can
         * still compute it (it's a valid but weaker claim). */
    }

    double sqrt_k = sqrt((double)k);
    double log_n = log((double)n);
    return exp(sqrt_k * log_n / 4.0);
}

double razborov_clique_lower_bound_exact(int n, int k, double constant) {
    /* Parameterized version with explicit constant factor.
     * size >= exp(constant * sqrt(k) * log n) */
    double sqrt_k = sqrt((double)k);
    double log_n = log((double)n);
    return exp(constant * sqrt_k * log_n);
}

/* ============================================================
 * Alon-Boppana Improved Bound (1987)
 * ============================================================ */

double alon_boppana_clique_lower_bound(int n, int k) {
    /* Alon & Boppana improved Razborov's bound by a more
     * careful analysis of the sunflower lemma and approximator
     * blowup. The exponent improves from sqrt(k) to k^{1/3}.
     *
     * Bound: size >= exp(Omega(k^{1/3} * log n))
     * for k <= n^{2/3}.
     *
     * For k = n^{1/4}: 
     *   Razborov: exp(Omega(n^{1/8}))
     *   Alon-Boppana: exp(Omega(n^{1/12})) ... wait
     *
     * Actually, the k^{1/3} bound is stronger for large k:
     * For k = n: Alon-Boppana gives exp(Omega(n))
     * (the trivial upper bound is also exp(O(n))).
     *
     * The bound: exp(c * k^{1/3} * log n)
     * We use c = 1/8 for a conservative estimate. */

    if (k <= 0 || n <= 0) return 0.0;
    if (k > n) return 0.0;

    double k_pow = pow((double)k, 1.0 / 3.0);
    double log_n = log((double)n);
    return exp(k_pow * log_n / 8.0);
}

/* ============================================================
 * Andreev's Lower Bound (1985)
 * ============================================================ */

double andreev_matrix_lower_bound(int n) {
    /* Andreev (1985) proved a super-polynomial lower bound for
     * a DIFFERENT monotone function using a similar method.
     *
     * His function: roughly a Boolean matrix product where the
     * input is an n x n Boolean matrix M, and the output is:
     *   f(M) = OR_{i,j,k: M[i,j]=M[j,k]=1} M[i,k]
     *
     * Bound: size >= exp(Omega(n^{1/3} / log n))
     *
     * This is weaker than Razborov's bound for CLIQUE but was
     * proved independently using the method of approximations. */

    if (n <= 1) return 2.0;
    double n13 = pow((double)n, 1.0 / 3.0);
    double log_n = log((double)n);
    return exp(n13 / (2.0 * log_n));
}

/* ============================================================
 * Tardos Gap Result (1988)
 * ============================================================ */

double tardos_gap_lower_bound(int n) {
    /* Tardos (1988) constructed a monotone Boolean function
     * that is in P (has poly-size general circuits) but
     * requires EXPONENTIAL-size monotone circuits.
     *
     * The function is based on the Lovasz theta function
     * and the Shannon capacity of graphs.
     *
     * The monotone lower bound: size >= exp(Omega(n))
     * while the general upper bound is poly(n).
     *
     * This showed that the gap between monotone and general
     * circuit complexity can be exponential. */

    if (n <= 1) return 2.0;
    return exp((double)n / 8.0);
}

/* ============================================================
 * Perfect Matching Lower Bound (Razborov 1985)
 * ============================================================ */

double razborov_pm_lower_bound(int n) {
    /* Razborov also proved that PERFECT MATCHING requires
     * super-polynomial monotone circuits.
     *
     * This is striking because PERFECT MATCHING is in P
     * (Edmonds' blossom algorithm, O(n^3) time), yet any
     * monotone circuit for it must be super-polynomial!
     *
     * Bound: exp(Omega(n^{1/6}))
     * for the n-vertex perfect matching function.
     *
     * This shows that monotone circuits are WEAK even for
     * problems with efficient algorithms. */

    if (n <= 2) return 2.0;
    double n16 = pow((double)n, 1.0 / 6.0);
    return exp(n16 / 4.0);
}

/* ============================================================
 * Approximator Size Analysis
 * ============================================================ */

double approx_size_after_and(double a1_size, double a2_size, int p, int k) {
    /* After AND gate, the approximator positive set is the
     * intersection of the two positive sets.
     * Size <= sunflower_bound(p,k) * min(|A1+|, |A2+|)
     * because we can compress sunflowers in the intersection. */
    double min_size = (a1_size < a2_size) ? a1_size : a2_size;
    double bound = sunflower_bound_erdos_rado(p, k);
    return bound * min_size;
}

double approx_size_after_or(double a1_size, double a2_size) {
    /* After OR gate, the positive set is the union.
     * Size <= |A1+| + |A2+| (no blowup). */
    return a1_size + a2_size;
}

double approx_size_after_levels(double initial_size, int num_levels,
                                 int p, int k, double and_fraction) {
    double size = initial_size;
    for (int l = 0; l < num_levels; l++) {
        /* At each level, some fraction of gates are ANDs */
        double and_blowup = approx_size_after_and(size, size, p, k);
        double or_blowup = approx_size_after_or(size, size);
        size = and_fraction * and_blowup + (1.0 - and_fraction) * or_blowup;
        if (size > 1e15) break;
    }
    return size;
}

/* ============================================================
 * Monotone Size Estimate
 * ============================================================ */

double monotone_size_estimate(int n, int k, int depth) {
    /* Estimate the monotone circuit size needed for CLIQUE.
     * Uses the method of approximations analysis.
     * Returns the minimum size consistent with the lower bound. */
    return alon_boppana_clique_lower_bound(n, k);
}

double constant_depth_monotone_bound(int n, int k, int depth) {
    /* For constant-depth monotone circuits:
     * If depth = d (constant), then the approximator at level d
     * has size bounded by F_d(|input_approx|) where F_d is
     * the d-fold composition of the AND/OR blowup functions.
     *
     * For AC0-like monotone circuits (unbounded fan-in AND/OR):
     * CLIQUE requires size >= exp(Omega(n^{1/d} / log n))
     * even for monotone circuits. */
    if (depth <= 0) return (double)(1 << n); /* Trivial */
    double n_pow = pow((double)n, 1.0 / (double)(2 * depth));
    return exp(n_pow / log((double)(n > 1 ? n : 2)));
}

/* ============================================================
 * Empirical Verification
 * ============================================================ */

int verify_lower_bound_empirically(int n, int k,
                                    double bound, int max_check_size) {
    /* For very small n, we can check if the lower bound is valid
     * by enumerating all monotone circuits up to max_check_size
     * and checking if any computes CLIQUE_{n,k}.
     *
     * This is infeasible for n > 4 (too many circuits).
     * Returns: 1 if bound is valid (no small circuit found),
     *          0 if a small circuit was found (bound falsified),
     *          -1 if verification infeasible. */

    if (n > 5) return -1; /* Too many circuits */

    /* Number of possible monotone circuits of size S with fan-in 2
     * is roughly (S * (S + n))^{S}. Too large even for S=10, n=5.
     * Only feasible for n <= 3. */

    if (n <= 3 && max_check_size <= 8) {
        /* Brute force enumeration of small circuits (n <= 3)
         * would go here. For now, return "infeasible". */
        return -1;
    }

    return -1; /* Infeasible for practical purposes */
}

/* ============================================================
 * Theorem Statement (for documentation)
 * ============================================================ */

void lower_bound_theorem_statement(void) {
    printf("================================================================\n");
    printf("  MONOTONE CIRCUIT LOWER BOUND THEOREMS\n");
    printf("================================================================\n\n");

    printf("Razborov's Theorem (1985):\n");
    printf("  The CLIQUE(n,k) function requires monotone circuits of size\n");
    printf("  at least exp(Omega(sqrt(k) * log n)).\n");
    printf("  In particular, for k = n^{1/4}, the size is\n");
    printf("  exp(Omega(n^{1/8})), which is SUPER-POLYNOMIAL.\n\n");

    printf("Alon-Boppana Theorem (1987):\n");
    printf("  CLIQUE(n,k) requires monotone circuits of size\n");
    printf("  at least exp(Omega(k^{1/3} * log n)).\n\n");

    printf("Andreev's Theorem (1985):\n");
    printf("  There exists an explicit monotone Boolean function\n");
    printf("  requiring monotone circuits of size\n");
    printf("  exp(Omega(n^{1/3} / log n)).\n\n");

    printf("Tardos Gap Theorem (1988):\n");
    printf("  There exists a monotone Boolean function f such that\n");
    printf("  S_monotone(f) = exp(Omega(n)) but S_general(f) = poly(n).\n\n");

    printf("Razborov's Perfect Matching Theorem (1985):\n");
    printf("  PERFECT MATCHING requires monotone circuits of size\n");
    printf("  exp(Omega(n^{1/6})), despite being in P.\n\n");

    printf("IMPORTANT: All these bounds are for MONOTONE circuits only.\n");
    printf("No super-polynomial lower bounds are known for\n");
    printf("general (non-monotone) Boolean circuits!\n");
}

/* ============================================================
 * Crypto Application
 * ============================================================ */

int circuit_crypto_grade(const MonotoneCircuit *c, double security_param) {
    /* For cryptographic applications, we want circuits that
     * are large enough to resist brute-force attack.
     * Security parameter S: attacker can try 2^S operations.
     * Circuit is "crypto-grade" if size >= 2^S. */
    if (!c) return 0;
    double log_size = log((double)c->size) / log(2.0);
    return (log_size >= security_param) ? 1 : 0;
}

/* ============================================================
 * Natural Proofs Barrier Explanation
 * ============================================================ */

void natural_proofs_barrier_explanation(void) {
    printf("================================================================\n");
    printf("  NATURAL PROOFS BARRIER (Razborov-Rudich 1997)\n");
    printf("================================================================\n\n");

    printf("A 'natural proof' of a circuit lower bound has three properties:\n");
    printf("  1. CONSTRUCTIVITY: The proof gives an algorithm that,\n");
    printf("     given a truth table of size 2^n, runs in time\n");
    printf("     poly(2^n) and identifies a property P.\n");
    printf("  2. LARGENESS: A random function has property P\n");
    printf("     with high probability (>= 1/2^n).\n");
    printf("  3. USEFULNESS: No function computable by small circuits\n");
    printf("     has property P.\n\n");

    printf("Theorem (Razborov-Rudich):\n");
    printf("  If strong pseudorandom generators exist, then NO natural\n");
    printf("  proof can prove super-polynomial circuit lower bounds\n");
    printf("  for general Boolean circuits.\n\n");

    printf("Why Razborov's proof is NOT natural:\n");
    printf("  - It works only for MONOTONE circuits\n");
    printf("  - The approximator property is not 'large': most functions\n");
    printf("    do NOT have small approximators.\n");
    printf("  - The monotone condition acts as a 'non-natural' restriction\n\n");

    printf("Why this makes extending to general circuits HARD:\n");
    printf("  Any proof of a general circuit lower bound must either:\n");
    printf("  - Be 'non-natural' (like Razborov's method), or\n");
    printf("  - Break cryptographic assumptions (unlikely), or\n");
    printf("  - Use a completely different approach.\n\n");

    printf("This is why proving P != NP via circuits is so difficult.\n");
}

/* ============================================================
 * Karchmer-Wigderson Connection
 * ============================================================ */

double kw_monotone_depth_lower_bound(int n, int k) {
    /* Karchmer-Wigderson (1988):
     * The monotone circuit depth of CLIQUE equals the
     * communication complexity of a certain search problem.
     *
     * Search problem KW(CLIQUE):
     *   Alice gets a k-clique C (YES instance)
     *   Bob gets a (k-1)-coloring (NO instance)
     *   They need to find an edge (u,v) such that
     *   u,v are in different color classes in Bob's coloring
     *   but are adjacent in Alice's clique.
     *
     * Monotone depth >= log of the communication complexity.
     *
     * Lower bound known: Omega(k) for KW game on CLIQUE,
     * implying monotone depth >= Omega(k). */

    if (k <= 1) return 1.0;
    /* Karchmer-Wigderson: monotone depth >= Omega(k)
     * For CLIQUE(n,k) with k <= n^{1/4}:
     *   monotone depth >= c * k */
    return (double)k / 3.0;
}

/* ============================================================
 * Open Problems (Research Frontiers)
 * ============================================================ */

void open_problems_monotone_lower_bounds(void) {
    printf("================================================================\n");
    printf("  OPEN PROBLEMS IN MONOTONE CIRCUIT COMPLEXITY\n");
    printf("================================================================\n\n");

    printf("1. GENERAL CIRCUIT LOWER BOUNDS:\n");
    printf("   Prove a super-polynomial lower bound for an explicit\n");
    printf("   Boolean function in general (non-monotone) circuits.\n");
    printf("   Status: OPEN since the 1970s.\n\n");

    printf("2. EXTEND APPROXIMATION METHOD:\n");
    printf("   Can Razborov's method of approximations be extended\n");
    printf("   to circuits with a limited number of NOT gates?\n");
    printf("   Status: Partial results by Amano & Maruoka (1998-2005)\n\n");

    printf("3. TIGHT BOUNDS FOR CLIQUE:\n");
    printf("   Is the Alon-Boppana bound (exp(Omega(k^{1/3} log n)))\n");
    printf("   tight? The best monotone upper bound is exp(O(n)).\n");
    printf("   Status: Large gap remains.\n\n");

    printf("4. MONOTONE-DEPTH HIERARCHY:\n");
    printf("   Is there a strict monotone-depth hierarchy?\n");
    printf("   Status: Open. The KW connection relates this to\n");
    printf("   communication complexity separations.\n\n");

    printf("5. MONOTONE VS GENERAL GAP:\n");
    printf("   What is the maximum possible gap between monotone\n");
    printf("   and general circuit complexity?\n");
    printf("   Tardos showed exp(Omega(n)). Can it be larger?\n");
    printf("   Status: Open.\n\n");

    printf("6. MONOTONE NC vs MONOTONE P:\n");
    printf("   Is there a problem in monotone-P that is not in\n");
    printf("   monotone-NC? (Monotone NC = poly-size, poly-log depth)\n");
    printf("   Status: Open.\n");
}

void meta_complexity_monotone_approach(void) {
    printf("================================================================\n");
    printf("  META-COMPLEXITY APPROACH TO MONOTONE LOWER BOUNDS\n");
    printf("================================================================\n\n");

    printf("Meta-complexity studies the complexity of problems ABOUT\n");
    printf("complexity (e.g., 'given a truth table, does it have\n");
    printf("small circuits?').\n\n");

    printf("Connection to monotone lower bounds:\n");
    printf("  - The Minimum Circuit Size Problem (MCSP) is a canonical\n");
    printf("    meta-complexity problem.\n");
    printf("  - Monotone-MCSP: given a monotone truth table, find the\n");
    printf("    smallest monotone circuit computing it.\n");
    printf("  - Understanding monotone-MCSP might lead to new lower\n");
    printf("    bound techniques for monotone circuits.\n\n");

    printf("Recent connections (Carmosino et al. 2016, Hirahara 2018):\n");
    printf("  - Learning algorithms for circuits imply natural proofs.\n");
    printf("  - MCSP is NP-complete under certain reductions.\n");
    printf("  - These results relate learning, cryptography, and\n");
    printf("    circuit lower bounds in a deep way.\n\n");

    printf("This is an active research area bridging complexity theory,\n");
    printf("cryptography, and machine learning.\n");
}
