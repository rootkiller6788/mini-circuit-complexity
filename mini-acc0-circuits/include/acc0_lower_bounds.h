/* acc0_lower_bounds.h — ACC0 Circuit Lower Bounds
 * ==================================================
 * L4: Fundamental Laws — Williams (2014), Razborov-Smolensky (1987).
 * L8: Advanced Topics — Algorithmic method for lower bounds.
 *
 * The central theme: proving that certain functions CANNOT be computed
 * by ACC0 circuits of a given size/depth.
 *
 * Known lower bounds:
 * 1. PARITY ∉ AC0 (Furst-Saxe-Sipser 1984, Håstad 1986).
 * 2. MOD_q ∉ AC0[p] for distinct primes p,q (Razborov 1987, Smolensky 1987).
 * 3. MAJORITY ∉ AC0[p] for any prime p (Razborov-Smolensky).
 * 4. NEXP ⊄ ACC0 (Williams 2014 — breakthrough).
 *
 * The Williams Method:
 *    If ACC0-SAT ∈ O(2ⁿ/n^k) for all k, then NEXP ⊄ ACC0.
 *    Williams proved ACC0-SAT ∈ O(2^{n - n^{ε}}) for some ε > 0.
 *    This suffices to separate NEXP from ACC0.
 *
 * References:
 * - Williams (2014): JACM 61(1), Article 2
 * - Razborov (1987): Mat. Zametki 41(4), pp. 598-607
 * - Smolensky (1987): STOC 1987, pp. 77-82
 */

#ifndef ACC0_LOWER_BOUNDS_H
#define ACC0_LOWER_BOUNDS_H

#include "acc0.h"

/* ================================================================
 * L1: Complexity class definitions relevant to lower bounds
 * ================================================================ */

/* Circuit complexity classes */
typedef enum {
    CLASS_AC0,          /* constant-depth, poly-size, unbounded fan-in AND/OR/NOT */
    CLASS_AC0_p,        /* AC0 + MOD_p gates (p prime) */
    CLASS_ACC0_m,       /* AC0 + MOD_m gates (specific m) */
    CLASS_ACC0,         /* union over all m of AC0[m] */
    CLASS_TC0,          /* constant-depth, poly-size, MAJORITY gates */
    CLASS_NC1,          /* O(log n) depth, bounded fan-in AND/OR/NOT */
    CLASS_P_POLY,       /* polynomial-size circuits (non-uniform P) */
    CLASS_NEXP          /* nondeterministic 2^{n^{O(1)}} time */
} CircuitClass;

/* Structure for a known separation result. */
typedef struct {
    CircuitClass lower_class;    /* the smaller class */
    CircuitClass upper_class;    /* the larger class (not contained in smaller) */
    const char  *function;       /* the separating function */
    const char  *result_source;  /* paper / authors / year */
    int          is_conditional; /* 1 if conditional on unproven assumptions */
} ClassSeparation;

/* ================================================================
 * L4: Known separations (theorems)
 * ================================================================ */

/* Get all known class separation results as an array.
 * Sets *count to the number of results. */
ClassSeparation* acc0_known_separations(int *count);

/* Print all known separations relevant to ACC0. */
void acc0_print_separations(void);

/* ================================================================
 * L4: Williams (2014) Bound
 * ================================================================ */

/* Compute the Williams bound: f(n) = exp(O(log^c n)) for various c.
 * c=3 is the original 2014 bound.
 * c=1 would be quasi-polynomial; c=0 would be polynomial. */
double acc0_williams_bound_general(int n, double c);

/* Improved Williams bound (2011): f(n) = exp(O(log^{0.5} n)).
 * This gave the NEXP ⊄ ACC0 result. */
double acc0_williams_bound_2011(int n);

/* If ACC0-SAT is solvable in time T(n) = 2^{n^{o(1)}},
 * then the Williams bound implies NEXP requires ACC0 circuits
 * of size at least S(n) = ... */
double acc0_williams_size_lower_bound(int n);

/* ================================================================
 * L5: ACC0 Circuit-SAT — The Algorithmic Engine
 * ================================================================ */

/* Brute-force ACC0-SAT: check all 2ⁿ assignments.
 * Returns 1 if satisfiable, 0 if unsatisfiable.
 * Also stores a satisfying assignment in *assignment if found.
 * Complexity: O(2ⁿ · |C|).
 * Set assignment = NULL to just check satisfiability. */
int acc0_sat_bruteforce(ACC0Circuit *c, int *assignment);

/* Improved ACC0-SAT using the polynomial method.
 * For ACC0[m] circuits: convert to GF(p) polynomials, reduce degree,
 * then search over a subspace.
 * This achieves the improved running time needed for Williams' result.
 *
 * Returns 1 if satisfiable, 0 if unsatisfiable.
 * Complexity: O(2^{n - n^ε}) for some ε > 0, under technical conditions. */
int acc0_sat_polynomial(ACC0Circuit *c, int *assignment);

/* ================================================================
 * L5: Lower Bound Proof Framework
 * ================================================================ */

/* Check if a given boolean function has a property that ACC0 cannot compute.
 * This is a heuristic based on known lower bound techniques:
 * - Degree over GF(p) too high → not in ACC0[m] for certain m.
 * - Sensitivity too high for shallow circuits.
 * - Communication complexity too high for ACC0.
 *
 * Returns a "likelihood" score: higher = more likely hard for ACC0. */
double acc0_hardness_score(int (*func)(const int*, int), int n);

/* Compute the approximate degree of a boolean function over GF(p).
 * The approximate degree is the minimum degree of a polynomial
 * that agrees with the function on ≥ 3/4 of inputs. */
int acc0_approximate_degree(int (*func)(const int*, int), int n, int p);

/* ================================================================
 * L8: Advanced — Non-uniform ACC0 and advice
 * ================================================================ */

/* Generate a random ACC0 circuit family with given size parameters.
 * This is useful for studying typical-case complexity. */
ACC0CircuitFamily* acc0_random_family(int max_n, int max_size, int max_depth);

/* Free a circuit family. */
void acc0_family_free(ACC0CircuitFamily *family);

/* ================================================================
 * L8: Advanced — Circuit lower bound conjectures
 * ================================================================ */

/* Print the status of major open problems related to ACC0. */
void acc0_print_open_problems(void);

/* Check if a function is "likely" to be outside ACC0 based on
 * known necessary conditions. Returns 1 if evidence suggests
 * the function is not in ACC0. */
int acc0_evidence_not_in_acc0(int (*func)(const int*, int), int n);

/* ================================================================
 * L9: Research Frontiers — Extending Williams
 * ================================================================ */

/* Estimate the circuit size needed by ACC0 to compute a function,
 * based on current best lower bound techniques.
 * Returns a lower bound estimate (may be very weak). */
double acc0_estimated_circuit_size_lower(int (*func)(const int*, int), int n);

/* Print an overview of the current research landscape. */
void acc0_research_frontier_demo(void);

/* Demo functions */
void acc0_lower_bound_demo(void);
void acc0_sat_demo(void);
void acc0_williams_demo(void);

#endif /* ACC0_LOWER_BOUNDS_H */
