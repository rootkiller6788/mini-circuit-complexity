#ifndef SHANNON_H
#define SHANNON_H
#include <stdint.h>

/*
 * shannon.h -- Shannon Counting for Circuit Lower Bounds
 *
 * L1: Definitions
 *   - shannon_circuit_count: upper bound on number of circuits of given size
 *   - shannon_feasible_fraction: fraction of functions computable by small circuits
 *   - shannon_hardness_threshold: minimum size for "most" functions
 *
 * L2: Core Concepts
 *   - Counting argument: there are 2^{2^n} functions but only ~2^{O(S log S)} circuits
 *   - Existence of hard functions: most functions require exponential-size circuits
 *   - Largeness of "hardness" property: Pr[random f requires size > S] ~ 1
 *
 * L3: Mathematical Structures
 *   - Combinatorial counting over circuit DAGs
 *   - Asymptotic bounds and Stirling approximations
 *   - Logarithmic scaling for numerical stability
 *
 * L4: Fundamental Laws
 *   - Shannon's Theorem (1949): Almost all Boolean functions require
 *     circuits of size Omega(2^n / n)
 *   - Lupanov's Theorem (1958): Every function has circuits of size
 *     O(2^n / n) -- asymptotically tight
 *   - Counting lemma: Number of circuits of size S with n inputs is
 *     at most (S * (n + S)^2)^S = 2^{O(S log(n+S))}
 *
 * L5: Algorithms/Methods
 *   - Numerical computation of circuit counts (with overflow protection)
 *   - Logarithmic computation for large n
 *   - Comparison of function space vs circuit space
 *
 * References:
 *   Shannon (1949): BSTJ 28(1):59-98
 *   Lupanov (1958): Doklady AN SSSR
 *   Arora-Barak (2009), Section 6.1
 *
 * Key insight (Shannon 1949):
 *   The number of circuits of size S is at most:
 *     (S * (n + S)^2)^S  <  2^{O(S log S)}
 *   The number of Boolean functions on n inputs is:
 *     2^{2^n}
 *   When S = 2^n / (c*n) for suitable c:
 *     2^{O(S log S)} = 2^{O(2^n / n * n)} = 2^{O(2^n)}
 *     << 2^{2^n} for sufficiently large n.
 *
 *   => Almost all functions require size > 2^n / (c*n).
 */

/* ========================================================================
 * Core Shannon Counting Functions
 * ======================================================================== */

/*
 * shannon_circuit_upper_bound: Upper bound on number of circuits of size <= S
 *   on n input variables.
 *
 * Derivation: Each gate in a circuit of size S:
 *   - Has type: AND, OR, or NOT (3 choices for fan-in 1, 2 each for fan-in 2)
 *   - Has fan-in wires from previous gates or inputs: at most (n+S)^2 choices
 *   - Conservative upper bound: 4*(n+S)^2 choices per gate
 *   - Total over S gates: (4*(n+S)^2)^S = 2^{S * log2(4*(n+S)^2)}
 *
 * Parameters:
 *   n: number of input variables
 *   S: circuit size bound (number of non-input gates)
 * Returns: upper bound on number of circuits (may be capped at 1e308 for overflow)
 */
double shannon_circuit_upper_bound(int n, int S);

/*
 * shannon_circuit_upper_bound_log: Same as above but returns log2 of the count.
 *   This avoids numerical overflow for large n, S.
 *   Returns: S * (2 + 2*log2(n+S))
 */
double shannon_circuit_upper_bound_log(int n, int S);

/*
 * shannon_function_count: Total number of Boolean functions on n inputs.
 *   Returns: 2^{2^n}
 *   Overflows quickly (n > 5 requires double; n > 10 returns Inf).
 */
double shannon_function_count(int n);

/*
 * shannon_function_count_log: log2 of total functions = 2^n.
 *   This is exact for n up to 1023 (2^n fits in double).
 */
double shannon_function_count_log(int n);

/*
 * shannon_feasible_fraction: Fraction of all Boolean functions that can be
 *   computed by circuits of size <= S.
 *
 *   fraction = min(1.0, circuits / total_functions)
 *
 * Interpretation:
 *   - fraction ~ 0 => most functions are "hard" (require size > S)
 *   - fraction ~ 1 => all functions are easy at size S
 *
 * For S = poly(n) and large n: fraction ~ 2^{O(n log n) - 2^n} ~ 0
 *   => "Hardness" is large.
 *
 * For S = 2^n / n and large n: fraction could be close to 1
 *   => Lupanov bound shows all functions computable at O(2^n/n).
 */
double shannon_feasible_fraction(int n, int S);

/*
 * shannon_feasible_fraction_log: Log2 of the feasible fraction.
 *   Returns: min(0, log_circuits - log_functions)
 *   Negative values indicate tiny fraction.
 */
double shannon_feasible_fraction_log(int n, int S);

/* ========================================================================
 * Largeness Analysis
 * ======================================================================== */

/*
 * shannon_largeness_fraction: Fraction of functions with circuit complexity > S.
 *   This is 1 - shannon_feasible_fraction(n, S).
 *
 * For the "hardness" property P(f) = "f requires size > S":
 *   P is LARGE iff this fraction >= 2^{-c*n} for some constant c.
 *
 * When S = poly(n): fraction ~ 1 (very large), so P is trivially large.
 * When S = 2^{n/2}: fraction may be ~ 0.5 (still large enough for natural).
 */
double shannon_largeness_fraction(int n, int S);

/*
 * shannon_hardness_threshold: The size S such that 50% of functions require
 *   circuits larger than S. This is the "median" circuit complexity.
 *
 * Shannon (1949): median ~ c * 2^n / n for some constant c.
 * This is computed by binary search on S to find the 50% point.
 */
int shannon_hardness_threshold(int n);

/* ========================================================================
 * Circuit Size Estimation
 * ======================================================================== */

/*
 * shannon_min_size_for_fraction: The minimum size S such that at most
 *   fraction 'frac' of functions can be computed with size <= S.
 *
 * Used to determine: what size bound S makes "hardness" property
 * satisfy the largeness criterion with parameter c.
 */
int shannon_min_size_for_fraction(int n, double frac);

/*
 * shannon_constructiveness_cost: Estimate the time to check whether a
 *   function has circuit complexity > S by exhaustive enumeration.
 *
 * Algorithm: enumerate all circuits of size <= S, check if any computes f.
 * Cost: O(circuit_count * 2^n) = O(2^{O(S log S)} * 2^n)
 *
 * For S = poly(n): cost = 2^{O(n log n)} -- constructive!
 * For S = exp(n): cost = 2^{exp(n)} -- NOT constructive.
 */
double shannon_constructiveness_cost(int n, int S);

/*
 * shannon_is_constructive: Check whether the naive enumeration algorithm
 *   for complexity > S runs in time 2^{O(n)}.
 * Returns 1 if constructive, 0 otherwise.
 */
int shannon_is_constructive(int n, int S);

/* ========================================================================
 * Barrier Analysis Helpers
 * ======================================================================== */

/*
 * shannon_natural_criteria_check: Check all three criteria for the property
 *   P(f) = "f has circuit complexity > S(n)".
 *
 * Returns a NaturalProperty struct (defined in natural_core.h) with all
 * three boolean flags and quantitative measures.
 *
 * This is the core function connecting Shannon counting to the
 * natural proofs barrier.
 */
#include "natural_core.h"
NaturalProperty shannon_natural_criteria_check(int n, int S);

/*
 * shannon_print_analysis: Print a detailed Shannon analysis for given n, S.
 *   Shows circuit counts, function counts, fractions, and criterion status.
 */
void shannon_print_analysis(int n, int S, FILE *fp);

/*
 * shannon_print_table: Print a table of Shannon data for various n, S pairs.
 *   Useful for demonstrating the asymptotic behavior.
 */
void shannon_print_table(int n_min, int n_max, int n_step,
                         int S_style, FILE *fp);

#endif /* SHANNON_H */
