/**
 * boolean_funcs.h — Boolean Functions for Hastad Lower Bounds
 *
 * Defines PARITY, MAJORITY, MOD-q, THRESHOLD functions and
 * their properties: sensitivity, influence, Fourier coefficients.
 *
 * L1: Definition of Boolean functions f:{0,1}^n → {0,1}
 * L2: PARITY/MOD restrict to subfunctions = restriction-stable
 * L3: Fourier expansion over GF(2)^n
 * L6: PARITY, MAJORITY as canonical hard functions for AC0
 */
#ifndef BOOLEAN_FUNCS_H
#define BOOLEAN_FUNCS_H

#include <stdint.h>

/* ── Basic Boolean Functions ─────────────────────────────────── */

/** PARITY(x) = x_1 ⊕ x_2 ⊕ ... ⊕ x_n */
int parity(const int* x, int n);

/** PARITY via XOR reduction (branchless) */
int parity_xor(const int* x, int n);

/** MAJORITY(x) = 1 iff Σ x_i > n/2 */
int majority(const int* x, int n);

/** MAJORITY via sorting median */
int majority_median(const int* x, int n);

/** MOD_m(x) = 1 iff Σ x_i ≡ 0 (mod m) */
int mod_m(const int* x, int n, int m);

/** THRESHOLD_t(x) = 1 iff Σ x_i ≥ t */
int threshold(const int* x, int n, int t);

/** AND of n bits */
int and_n(const int* x, int n);

/** OR of n bits */
int or_n(const int* x, int n);

/** Exact COUNT-k: 1 iff Σ x_i = k */
int count_k(const int* x, int n, int k);

/* ── Composite Functions ─────────────────────────────────────── */

/** PARITY-of-ANDs: depth-2 AC0 circuit */
int parity_of_ands(const int** x, int m, int k);

/** AND-of-ORs (DNF structure): standard AC0 template */
int and_of_ors(const int** x, int m, int k);

/** OR-of-ANDs (CNF structure) */
int or_of_ands(const int** x, int m, int k);

/* ── Sipser Functions (AC0 depth hierarchy) ─────────────────── */

/** Sipser's S_d function: depth-d tree-of-gates separator */
int sipser_depth(const int* x, int n, int d);

/** S_d ∈ AC0[d] but S_d ∉ AC0[d-1] (Sipser 1983) */
int sipser_hierarchy_fn(const int* x, int n, int depth);

/* ── Function Properties ─────────────────────────────────────── */

/** Sensitivity of f at input x: number of neighbors flipping output */
int sensitivity(const int* x, int n, int (*f)(const int*, int));

/** Block sensitivity */
int block_sensitivity(const int* x, int n, int (*f)(const int*, int));

/** Degree of f as polynomial over GF(2): max degree with non-zero Fourier coeff */
int gf2_degree_estimate(const int* x, int n, int (*f)(const int*, int));

/** Check if function is constant */
int is_constant_function(const int* x, int n, int (*f)(const int*, int));

/** Check if function depends on variable i */
int depends_on_var(const int* x, int n, int i, int (*f)(const int*, int));

/* ── Fourier Analysis ────────────────────────────────────────── */

/** Transform binary {0,1} to {+1,-1} basis */
double to_fourier_basis(int bit);

/** Compute one Fourier coefficient via sampling (LMN 1993) */
double fourier_coefficient_sample(const int* x, int n, int (*f)(const int*, int),
                                   const int* subset, int subset_size);

/** Check if f has all low-degree Fourier coefficients small */
int is_fourier_concentrated(int (*f)(const int*, int), int n, int degree, double threshold);

/* ── Subcube Enumeration ─────────────────────────────────────── */

/** Count minterms (true points) in subcube defined by restriction */
int64_t count_minterms_subcube(const int* restriction, int n,
                                int (*f)(const int*, int));

/** List all assignments of free variables in a restriction */
void enumerate_free_assignments(const int* restr, int n,
                                 void (*callback)(const int*, int, void*), void* ctx);

#endif /* BOOLEAN_FUNCS_H */
