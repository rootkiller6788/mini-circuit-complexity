#ifndef RS_BOUNDS_H
#define RS_BOUNDS_H
/*============================================================================
 * rs_bounds.h — Razborov-Smolensky Lower Bound Computation
 *
 * Computes the famous lower bound: MAJORITY ∉ AC0[p] for any prime p≠2.
 *
 * The proof structure (Arora & Barak §14.4):
 * 1. Any depth-d AC0[p] circuit of size S can be approximated by a
 *    GF(p) polynomial of degree D = O((log S)^d).
 * 2. Any GF(p) polynomial that ε-approximates MAJORITY requires
 *    degree Ω(√n).
 * 3. Therefore S = exp(Ω(n^{1/(2d)})).  For constant d, this is
 *    super-polynomial in n.  So MAJORITY ∉ AC0[p] (p≠2).
 *
 * Knowledge: L4 (lower bound theorem), L5 (degree-to-size translation),
 * L6 (MAJORITY vs AC0[p] separation), L7 (implications for circuit classes),
 * L8 (comparison with natural proofs barrier).
 *
 * References:
 *   Razborov (1987), Smolensky (1987)
 *   Arora & Barak, "Computational Complexity", Theorem 14.4
 *   Vollmer, "Introduction to Circuit Complexity", §4.6
 *==========================================================================*/

#include <math.h>

/*----------------------------------------------------------------------------
 * L4 — Degree lower bound for MAJORITY over GF(p), p ≠ 2
 *
 * Theorem (Razborov 1987):
 *   Any polynomial P over GF(p) (p prime, p≠2) that computes MAJORITY
 *   on n variables must have degree deg(P) ≥ √(n·ln(1/ε)) / 3.
 *
 * For ε = 1/3, this gives deg(P) ≥ (√n) / 3.
 *
 * This is the key lemma: MAJORITY needs HIGH degree over GF(p) for p≠2.
 * Contrast: over GF(2), MAJORITY has degree 1 (just addition).
 *----------------------------------------------------------------------------*/

/* Razborov's degree lower bound for MAJORITY:
 *   deg ≥ √(n · ln(1/ε)) / 3
 * epsilon: allowed error probability (typically 0.1 or 0.01)         */
double rs_deg_lower_bound(int n, double epsilon);

/* Smolensky's refined bound:
 *   deg ≥ (n/2)^{1/2} · (1/2)^{1/2} · (p-1)^{-1/2}
 * Incorporates the field size explicitly.                              */
double rs_deg_lower_bound_smolensky(int n, int p, double epsilon);

/*----------------------------------------------------------------------------
 * L4 — Degree of a depth-d AC0[p] circuit
 *
 * Theorem: A depth-d AC0[p] circuit of size S can be ε-approximated by
 * a GF(p) polynomial of degree D where
 *   D ≤ (c · log(S/ε))^d      (for some constant c depending on p).
 *
 * Practical bound used in the construction:                             */
double rs_circuit_degree_upper(int depth, int64_t size, double epsilon, int p);

/*----------------------------------------------------------------------------
 * L4 — Size lower bound (combining the two inequalities)
 *
 * From deg_lower ≤ deg_upper we get:
 *   S ≥ exp(Ω(n^{1/(2d)}))
 *
 * This is super-polynomial for constant d → MAJORITY ∉ AC0[p].         */

/* Razborov size lower bound:
 *   size ≥ exp(n^{1/(2d)} / 5)  (asymptotic form)                      */
double rs_size_lower_bound(int n, int depth, int p);

/* Smolensky size lower bound (with explicit constants):
 *   S ≥ 2^{c_p · n^{1/(2d)}} where c_p depends on p                    */
double rs_size_lower_bound_smolensky(int n, int depth, int p);

/*----------------------------------------------------------------------------
 * L5 — Checking the lower bound parameters
 *----------------------------------------------------------------------------*/

/* Verify that for given n,d,p, the polynomial method gives a non-trivial
 * lower bound (size > poly(n)).  Returns 1 if bound is super-polynomial. */
int  rs_bound_is_nontrivial(int n, int depth, int p);

/* Find smallest n where the lower bound exceeds poly(n) = n^k.
 * Returns n, or -1 if no such n within search range.                   */
int  rs_find_crossover(int min_n, int max_n, int depth, int p, int k);

/*----------------------------------------------------------------------------
 * L6 — Specific separation results
 *----------------------------------------------------------------------------*/

/* Check: Does MOD_q have low-degree representation over GF(p)?
 * (Yes if q = p, no otherwise.)  Returns degree needed, or -1.         */
int  rs_modq_degree(int p, int q, int n, double epsilon);

/* Compare AC0[p] vs AC0[q]:
 * For distinct primes p, q: AC0[p] ≠ AC0[q].
 * Witness: MOD_p ∈ AC0[p] but MOD_p ∉ AC0[q].                         */
int  rs_separates_ac0_mod(int p, int q, int n);

/* Check if a given Boolean function (truth table) has a low-degree
 * GF(p) approximation.  Returns 1 if low-degree poly exists.           */
int  rs_has_low_degree_approx(const int* truth_table, int n, int p, int max_deg);

/*----------------------------------------------------------------------------
 * L7 — Circuit class separation table
 *----------------------------------------------------------------------------*/

/* Print known separations table:
 *   AC0 ⊊ AC0[2] ⊊ AC0[3] ⊊ ... ⊊ TC0 ⊆ NC1
 *   MAJORITY ∈ AC0[2] but ∉ AC0[p] for p≠2.                            */
void rs_print_separation_table(void);

/* Print the state of the ACC0 vs NC1 problem (open):
 *   ACC0 = ∪_m AC0[MOD_m].  Is ACC0 = NC1?  Major open problem.
 *   Williams (2014) proved NEXP ⊄ ACC0 via different methods.
 *   The natural proofs barrier (Razborov-Rudich 1997) explains
 *   why RS-like methods cannot resolve ACC0 vs NC1.                    */
void rs_print_acc0_frontier(void);

/*----------------------------------------------------------------------------
 * L8 — Advanced: polynomial threshold degree
 *----------------------------------------------------------------------------*/

/* The polynomial threshold degree (PTF degree) of a Boolean function:
 * minimum degree of a real polynomial P such that sign(P(x)) = f(x).
 * Used in upper bounds for TC0 and lower bounds for AC0.               */
int  rs_ptf_degree(const int* truth_table, int n);

#endif /* RS_BOUNDS_H */
