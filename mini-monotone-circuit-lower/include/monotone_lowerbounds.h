/* monotone_lowerbounds.h -- Monotone Circuit Lower Bounds
 *
 * The central topic: proving that certain monotone Boolean functions
 * require large monotone circuits. The landmark result is:
 *
 * Razborov's Theorem (1985):
 *   The CLIQUE_{n,k} function requires monotone circuits of size
 *   exp(Omega(k^{1/2} * log n)) for k <= n^{1/4}.
 *   In particular, for k = n^{1/4}, the size is super-polynomial:
 *   exp(Omega(n^{1/8})).
 *
 * Other major results:
 *   Alon-Boppana (1987): CLIQUE requires exp(Omega(k^{1/3} * log n))
 *     monotone size, improving Razborov's bound.
 *   Andreev (1985): Another super-polynomial monotone lower bound
 *     via the method of approximations for a different function.
 *   Tardos (1988): Exponential gap between monotone and non-monotone
 *     circuit complexity of a monotone function.
 *   Razborov (1985b): PERFECT MATCHING requires super-polynomial
 *     monotone size.
 *
 * The method of approximations (Razborov 1985) is the main technique.
 * No super-polynomial lower bounds are known for general circuits!
 *
 * References:
 *   Razborov (1985), Doklady AN SSSR
 *   Alon & Boppana (1987), Combinatorica 7(1), 1-22
 *   Andreev (1985), DAN SSSR 282(5), 1033-1037
 *   Tardos (1988), Combinatorica 8(1), 141-142
 *   Jukna (2012), "Boolean Function Complexity"
 *   Arora & Barak (2009), "Computational Complexity", Ch. 14
 */
#ifndef MONOTONE_LOWERBOUNDS_H
#define MONOTONE_LOWERBOUNDS_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "monotone_circuit.h"
#include "monotone_graph.h"

/* ============================================================
 * L1: Lower Bound Results Structure
 * ============================================================ */

/* A lower bound result: for a specific monotone function f,
 * any monotone circuit computing f must have size >= bound. */
typedef struct {
    char    function_name[64];
    int     n;              /* Input size parameter */
    int     k;              /* Secondary parameter (e.g., clique size) */
    double  size_bound;     /* Lower bound on circuit size */
    int     reference_year; /* Year of publication */
    char    author[64];
    char    method[128];    /* Proof method used */
} MonotoneLowerBound;

/* ============================================================
 * L4: Razborov's Lower Bound (1985)
 * ============================================================ */

/* Razborov's original bound:
 * CLIQUE_{n,k} requires monotone circuits of size at least
 * exp(Omega(sqrt(k) * log n)) for k <= n^{1/4}.
 *
 * More precisely: for k = floor(n^{1/4}), the lower bound is
 *   size >= exp(Omega(n^{1/8}))
 * which is SUPER-POLYNOMIAL in n.
 */
double razborov_clique_lower_bound(int n, int k);

/* The constant factor version returning the asymptotic bound */
double razborov_clique_lower_bound_exact(int n, int k, double constant);

/* ============================================================
 * L4: Alon-Boppana Improved Bound (1987)
 * ============================================================ */

/* Improved the exponent from sqrt(k) to k^{1/3}.
 * CLIQUE_{n,k} requires monotone circuits of size at least
 * exp(Omega(k^{1/3} * log n)).
 *
 * Key improvement: better analysis of the approximator blowup
 * at AND gates using the sunflower lemma more carefully. */
double alon_boppana_clique_lower_bound(int n, int k);

/* ============================================================
 * L4: Andreev's Lower Bound (1985)
 * ============================================================ */

/* Andreev independently proved a super-polynomial lower bound
 * for a DIFFERENT monotone function (not CLIQUE) using a similar
 * method of approximations.
 *
 * His function: a carefully constructed Boolean matrix product.
 *
 * The bound: exp(Omega(n^{1/3} / log n)) */
double andreev_matrix_lower_bound(int n);

/* ============================================================
 * L4: Tardos Gap Result (1988)
 * ============================================================ */

/* Tardos showed that there exists a monotone Boolean function
 * that has polynomial-size GENERAL circuits but requires
 * EXPONENTIAL-size MONOTONE circuits.
 *
 * This shows the gap between monotone and general circuits
 * is exponential! */
double tardos_gap_lower_bound(int n);

/* ============================================================
 * L4: Perfect Matching Bound (Razborov 1985)
 * ============================================================ */

/* PERFECT MATCHING requires super-polynomial monotone circuits.
 * This is interesting because PERFECT MATCHING is in P
 * (Edmonds' blossom algorithm) but its monotone circuit
 * complexity is provably super-polynomial! */
double razborov_pm_lower_bound(int n);

/* ============================================================
 * L5: Lower Bound Verification
 * ============================================================ */

/* Compare the computed lower bound against brute-force enumeration
 * of all circuits up to a small size (for very small n).
 * This provides empirical validation of the analytic bounds. */
int verify_lower_bound_empirically(int n, int k, 
                                    double bound, int max_check_size);

/* ============================================================
 * L5: Method of Approximations - Analysis
 * ============================================================ */

/* Analyze the approximator size growth through circuit levels.
 *
 * At level 0 (inputs): approximator size = O(1) per input.
 * At AND gate: size <= sunflower_bound(p, k) * |A1| * |A2|
 *              but sunflower compression reduces this to O(|A1| + |A2|)
 * At OR gate:  size <= |A1| + |A2| (additive, no blowup)
 *
 * After d levels with reasonable AND/OR mix:
 *   size <= O((2*poly(k))^d)
 * For circuit of size S and depth d:
 *   if S < lower_bound, then final approximator is small
 *   but CLIQUE requires large approximator -> contradiction
 */

/* Compute approximator size after one AND level (with sunflower) */
double approx_size_after_and(double a1_size, double a2_size, int p, int k);

/* Compute approximator size after one OR level */
double approx_size_after_or(double a1_size, double a2_size);

/* Simulate approximator size through l levels */
double approx_size_after_levels(double initial_size, int num_levels,
                                 int p, int k, double and_fraction);

/* ============================================================
 * L6: Main Theorem Statement
 * ============================================================ */

/* Main theorem: monotone circuit lower bound for CLIQUE.
 *
 * Theorem (Razborov 1985, Alon-Boppana 1987):
 *   Any monotone Boolean circuit computing CLIQUE_{n,k} must have
 *   size at least exp(Omega(k^{1/3} * log n)).
 *
 * In particular:
 *   - For k = n^{1/4}: size >= exp(Omega(n^{1/12}))
 *   - For k = Theta(n): size >= exp(Omega(n))
 *   - For k = Theta(log n): size >= n^{Omega(log log n)}
 *
 * The proof uses the method of approximations with
 * the Erdos-Rado sunflower lemma.
 */
void lower_bound_theorem_statement(void);

/* ============================================================
 * L6: Specific Bound Computations
 * ============================================================ */

/* Compute bound for specific parameter settings */
double monotone_size_estimate(int n, int k, int depth);

/* Bound for constant-depth monotone circuits */
double constant_depth_monotone_bound(int n, int k, int depth);

/* ============================================================
 * L7: Applications in Cryptography
 * ============================================================ */

/* Monotone circuits are relevant to cryptography because:
 * 1. Some cryptographic primitives rely on the conjectured hardness
 *    of monotone functions (e.g., Goldreich's PRG proposal)
 * 2. Lower bounds guide the design of secure hardware
 * 3. Monotone complexity provides a framework for average-case
 *    hardness analysis
 */

/* Check if a given circuit is suitable for crypto (sufficiently large) */
int circuit_crypto_grade(const MonotoneCircuit *c, double security_param);

/* ============================================================
 * L8 Advanced: Natural Proofs Barrier
 * ============================================================ */

/* Razborov-Rudich (1997): Natural Proofs barrier.
 * Any "natural" proof of a super-polynomial circuit lower bound
 * (for general circuits) would break certain cryptographic assumptions.
 *
 * Why Razborov's proof is NOT "natural" (and thus avoids the barrier):
 * - It works ONLY for monotone circuits
 * - The approximator property (being small on average) is not
 *   constructible in polynomial time for general circuits
 * - The "largeness" condition of natural proofs is not satisfied
 *
 * This explains why extending to general circuits is so hard. */

/* Demonstrate that the method of approximations is NOT a natural proof.
 * Prints explanation. */
void natural_proofs_barrier_explanation(void);

/* ============================================================
 * L8: Karchmer-Wigderson Connection (1988)
 * ============================================================ */

/* KW Theorem: The monotone circuit depth complexity of f
 * equals the communication complexity of the KW game for f.
 *
 * For monotone f: monotone depth = monotone KW game complexity.
 *
 * This connects circuit complexity to communication complexity,
 * providing another tool for lower bounds. */

/* Compute KW game complexity lower bound (simplified) for CLIQUE */
double kw_monotone_depth_lower_bound(int n, int k);

/* ============================================================
 * L9: Research Frontiers
 * ============================================================ */

/* Open problems documented for research context */
void open_problems_monotone_lower_bounds(void);

/* Meta-complexity approach to monotone lower bounds */
void meta_complexity_monotone_approach(void);

#endif /* MONOTONE_LOWERBOUNDS_H */
