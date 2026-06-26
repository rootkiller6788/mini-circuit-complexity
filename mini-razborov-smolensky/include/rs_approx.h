#ifndef RS_APPROX_H
#define RS_APPROX_H
/*============================================================================
 * rs_approx.h — Polynomial Approximation for Circuit Gates
 *
 * Implements the core Razborov-Smolensky technique: replacing each gate
 * of an AC0 circuit by a low-degree probabilistic polynomial over GF(p).
 *
 * Key insight (Smolensky 1987):
 *   AND(x_1,...,x_k) can be ε-approximated by a degree-(p-1) polynomial
 *   over GF(p) in the probabilistic sense: for any fixed input x,
 *   Pr_r[poly_r(x) = AND(x)] ≥ 1 - ε, where r is the random seed.
 *
 *   Construction: pick random coefficients r_i ∈ GF(p) \ {0},
 *   form L = Σ r_i x_i, output L^{p-1} mod p.
 *   By Fermat's little theorem, L^{p-1} = 1 iff L ≠ 0.
 *   When all x_i = 1, L = Σ r_i and with probability > 1 - 1/p, L ≠ 0.
 *   When any x_i = 0, its term vanishes but the polynomial still
 *   mostly agrees (probabilistic guarantee).
 *
 * OR approximation follows by De Morgan: OR = NOT(AND(NOT ...)).
 *
 * Knowledge: L2 (probabilistic approximation), L3 (Fermat theorem),
 * L4 (approximation lemma), L5 (randomized construction algorithm).
 *
 * References:
 *   Smolensky, "Algebraic methods in the theory of lower bounds
 *     for Boolean circuit complexity" (STOC 1987)
 *   Arora & Barak, "Computational Complexity", §14.4.2
 *   Jukna, "Boolean Function Complexity", §12
 *==========================================================================*/

#include "gf_poly.h"

/*----------------------------------------------------------------------------
 * L2 — ε-approximation definition
 *
 * A polynomial P over GF(p) ε-approximates a Boolean function f if
 *   Pr_{x∈{0,1}^n}[P(x) ≠ f(x)] ≤ ε
 * for a fixed probabilistic construction (or over random seeds).
 *----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
 * L4 — Approximation parameters
 *
 * For AND of fanin k over GF(p):
 *   probability of correct output ≥ 1 - k/p
 *   degree of approximating polynomial = p-1 (constant for fixed p)
 *
 * For a depth-d AC0 circuit of size S:
 *   each gate replaced with error ≤ ε/S
 *   total error ≤ ε (by union bound)
 *   total degree = (polynomial degree of one level)^d = (p-1)^d
 *   For fixed p and small d, total degree is constant!
 *----------------------------------------------------------------------------*/

/*============================================================================
 * L5 — Probabilistic Gate Approximations
 *==========================================================================*/

/* Probabilistic AND polynomial:
 *   Choose r_1,...,r_k ∈ GF(p)^* uniformly at random.
 *   L(x) = Σ r_i x_i (mod p)
 *   P_and(x) = L(x)^{p-1} (mod p)
 *
 * For x_i∈{0,1}: when all x_i=1, P=1 with high prob; otherwise ≈0.
 * seed controls the random choice of r_i.                            */
GFPoly* rs_approx_and_probabilistic(int p, const int* var_indices, int k,
                                     unsigned int seed);

/* Probabilistic OR polynomial:
 *   OR(x) = NOT(AND(NOT x_1, ..., NOT x_k))
 *   P_or(x) = 1 - P_and(1-x_1, ..., 1-x_k)                          */
GFPoly* rs_approx_or_probabilistic(int p, const int* var_indices, int k,
                                    unsigned int seed);

/* Deterministic approximation (fallback for small k):
 *   Simply multiply the first d variables (degree-d truncation).     */
GFPoly* rs_approx_and_truncated(int p, const int* var_indices, int k, int d);
GFPoly* rs_approx_or_truncated(int p, const int* var_indices, int k, int d);

/* MOD_p gate as polynomial:
 *   MOD_p(x_1,...,x_k) = 1 - (Σ x_i)^{p-1} (mod p)
 *   Exact (not approximate), degree = p-1 = O(1) for fixed p.        */
GFPoly* rs_poly_modp(int p, const int* var_indices, int k);

/* NOT gate as polynomial: NOT(x) = 1 - x (exact, degree 1)           */
GFPoly* rs_poly_not(int p, int var_idx, int n_vars);

/*============================================================================
 * L5 — Circuit-to-Polynomial Translation (Gate-by-Gate)
 *==========================================================================*/

/* Result of translating a circuit to a polynomial */
typedef struct {
    GFPoly* poly;       /* the resulting polynomial                   */
    int     max_degree; /* maximum degree encountered                  */
    int     n_gates;    /* gates translated                           */
    int     n_errors;   /* number of probabilistic choices made       */
    double  err_bound;  /* union-bound error probability upper bound  */
} RSPolyResult;

/* Translate an entire AC0[p] circuit to a single GF(p) polynomial.
 * Circuit must be over GF(p) (modp == p for all MOD gates).
 * Returns NULL if circuit uses incompatible MOD_q with q≠p.          */
RSPolyResult* rs_circuit_to_poly(const struct RSCircuit* C, int p,
                                  unsigned int seed);

void rs_poly_result_free(RSPolyResult* res);

/* Experiment: test approximation quality on random inputs */
double rs_measure_approximation_error(const GFPoly* poly,
                                       int (*fn)(const int*, int),
                                       int n_vars, int n_trials);

/*============================================================================
 * Print / analysis
 *============================================================================*/
void rs_poly_result_print(const RSPolyResult* res);

#endif /* RS_APPROX_H */
