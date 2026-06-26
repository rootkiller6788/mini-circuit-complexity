/* acc0_polynomial.h — GF(p) Polynomial Representation of ACC0 Circuits
 * =====================================================================
 * L3: Mathematical Structures — Polynomials over finite fields.
 * L5: Algorithms — Circuit-to-polynomial conversion.
 *
 * Key insight: An ACC0 circuit of depth d can be simulated by a
 * polynomial over GF(p) of degree O(p^{d}) (for MOD_p gates).
 *
 * For ACC0 (union over all m), each MOD_m gate becomes a polynomial
 * of degree m-1 over GF(p) for appropriate p.
 *
 * The polynomial simulation is the core of Williams' (2014) algorithmic
 * method: convert ACC0 circuit → GF(p) polynomial → fast SAT algorithm.
 *
 * References:
 * - Williams (2014): "Non-uniform ACC Circuit Lower Bounds", JACM 61(1)
 * - Beigel & Tarui (1994): "On ACC", Computational Complexity 4
 * - Yao (1990): "On ACC and threshold circuits", FOCS 1990
 */

#ifndef ACC0_POLYNOMIAL_H
#define ACC0_POLYNOMIAL_H

#include "acc0.h"

/* Maximum degree for polynomial representation. */
#define ACC0_POLY_MAX_DEGREE 1024
#define ACC0_POLY_MAX_TERMS  4096

/* A single term c · ∏ xᵢ^{eᵢ} over GF(p).
 * coefficient ∈ [0, p-1].
 * exponents[eᵢ] for each variable. */
typedef struct {
    int coefficient;                     /* coefficient in GF(p) */
    int exponents[ACC0_MAX_FANIN];       /* exponent per variable */
    int degree;                          /* total degree ∑ eᵢ */
} ACC0PolyTerm;

/* A polynomial over GF(p): sum of terms. */
typedef struct {
    ACC0PolyTerm *terms;                 /* array of terms */
    int           nterms;                /* number of terms */
    int           max_terms;             /* allocated capacity */
    int           modulus;               /* prime p for GF(p) */
    int           nvars;                 /* number of variables */
    int           total_degree;          /* max degree among terms */
} ACC0Polynomial;

/* ================================================================
 * L3: Polynomial Construction
 * ================================================================ */

/* Create an empty polynomial over GF(p) with n variables. */
ACC0Polynomial* acc0_poly_create(int modulus, int nvars, int max_terms);

/* Free a polynomial. */
void acc0_poly_free(ACC0Polynomial *poly);

/* Add a term to the polynomial.
 * Returns 0 on success, -1 if capacity exceeded. */
int acc0_poly_add_term(ACC0Polynomial *poly, int coeff, const int *exponents);

/* ================================================================
 * L5: Polynomial Arithmetic over GF(p)
 * ================================================================ */

/* Evaluate polynomial on boolean inputs xᵢ ∈ {0,1}.
 * Result is in GF(p) (i.e., in [0, p-1]).
 * Complexity: O(nterms · nvars). */
int acc0_poly_eval(ACC0Polynomial *poly, const int *x);

/* Add two polynomials: poly = poly + other (mod p). */
int acc0_poly_add(ACC0Polynomial *poly, ACC0Polynomial *other);

/* Multiply two polynomials: poly = poly * other (mod p). */
int acc0_poly_multiply(ACC0Polynomial *poly, ACC0Polynomial *other);

/* Compute polynomial^e mod (xᵢ² - xᵢ) — i.e., reduce using xᵢ² = xᵢ.
 * This is the "boolean reduction" since xᵢ ∈ {0,1}. */
int acc0_poly_boolean_reduce(ACC0Polynomial *poly);

/* Reduce polynomial modulo prime p (all coefficients mod p). */
void acc0_poly_mod_reduce(ACC0Polynomial *poly);

/* ================================================================
 * L5: Circuit-to-Polynomial Conversion
 * ================================================================ */

/* Convert an ACC0 circuit to a polynomial over GF(p).
 * The polynomial computes the same function as the circuit
 * when restricted to boolean inputs.
 *
 * For MOD_m gates: represented as 1 - (∑ xᵢ)^{m-1} mod p
 * (using Fermat's little theorem for prime m).
 *
 * Complexity: exponential in circuit depth. */
ACC0Polynomial* acc0_circuit_to_polynomial(ACC0Circuit *c, int prime_modulus);

/* Convert a single gate to a polynomial over GF(p).
 * Recursively converts the gate's fan-in gates first. */
ACC0Polynomial* acc0_gate_to_polynomial(ACC0Circuit *c, int gate_id,
                                         int prime_modulus, ACC0Polynomial **cache);

/* ================================================================
 * L3: Polynomial Properties
 * ================================================================ */

/* Compute the total degree of the polynomial. */
int acc0_poly_degree(ACC0Polynomial *poly);

/* Count the number of non-zero terms. */
int acc0_poly_term_count(ACC0Polynomial *poly);

/* Print polynomial in human-readable form. */
void acc0_poly_print(ACC0Polynomial *poly);

/* ================================================================
 * L6: Polynomial Functions for Canonical Boolean Functions
 * ================================================================ */

/* Construct the polynomial for PARITY(x₁,...,xₙ) over GF(2).
 * PARITY = ∑ xᵢ (mod 2) as a polynomial. */
ACC0Polynomial* acc0_poly_parity(int n);

/* Construct the polynomial for MOD₃(x₁,...,xₙ) over GF(3).
 * MOD₃ = 1 iff ∑ xᵢ ≡ 0 (mod 3). */
ACC0Polynomial* acc0_poly_mod3(int n);

/* Construct the polynomial for AND(x₁,...,xₙ): ∏ xᵢ.
 * Degree = n. */
ACC0Polynomial* acc0_poly_and(int n);

/* Construct the polynomial for OR(x₁,...,xₙ):
 * 1 - ∏(1 - xᵢ). Degree = n. */
ACC0Polynomial* acc0_poly_or(int n);

/* ================================================================
 * L7: Application — Polynomial Identity Testing
 * ================================================================ */

/* Check if two polynomials are equivalent on all boolean inputs.
 * Uses Schwartz-Zippel-inspired random evaluation.
 * Returns 1 if equivalent with high probability, 0 if definitely not. */
int acc0_poly_equivalent(ACC0Polynomial *p1, ACC0Polynomial *p2, int trials);

/* ================================================================
 * L8: Advanced — Degree lower bounds
 * ================================================================ */

/* Estimate the minimum polynomial degree over GF(p) needed to
 * represent a given boolean function.
 * Based on the Razborov-Smolensky degree lower bound:
 * any polynomial computing MOD_q (q ≠ p) over GF(p) has degree Ω(n^{1/(2d)}). */
double acc0_degree_lower_bound(int n, int depth, int p, int q);

/* Demo functions */
void acc0_poly_demo(void);
void acc0_circuit_poly_demo(void);

#endif /* ACC0_POLYNOMIAL_H */
