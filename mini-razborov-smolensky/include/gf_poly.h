#ifndef GF_POLY_H
#define GF_POLY_H
/*============================================================================
 * gf_poly.h — GF(p) Polynomial Library
 *
 * Multilinear polynomial representation over GF(p) for Boolean inputs.
 * Since x_i ∈ {0,1}, we have x_i^k = x_i for all k≥1, so every polynomial
 * can be written as a sum of products of distinct variables (multilinear).
 *
 * This is the central data structure for the Razborov-Smolensky method:
 * circuit gates are replaced by low-degree GF(p) polynomials, and the
 * degree explosion across depth yields the lower bound.
 *
 * Knowledge: L1 (GF(p) polynomial definition), L2 (multilinear representation),
 * L3 (algebraic structure of GF(p)), L4 (Fermat's Little Theorem for MOD_p),
 * L5 (polynomial arithmetic algorithms).
 *
 * References:
 *   Lidl & Niederreiter, "Finite Fields" (1997)
 *   Arora & Barak, "Computational Complexity", §14.4
 *==========================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

/*----------------------------------------------------------------------------
 * L1 — Core definition: GF(p) multilinear polynomial
 *
 * A polynomial over GF(p) in n Boolean variables is an expression
 *   f(x_1,...,x_n) = Σ_{S⊆[n]} c_S · Π_{i∈S} x_i   (mod p)
 * where c_S ∈ GF(p) and x_i ∈ {0,1}.
 *
 * The degree deg(f) = max{|S| : c_S ≠ 0}.
 *----------------------------------------------------------------------------*/

/*---- Monomial composition ----*/
typedef struct {
    int  coeff;        /* coefficient c ∈ {0, 1, ..., p-1}        */
    int* exps;         /* exps[i] ∈ {0,1} — whether x_i is present */
    int  deg;          /* degree = number of variables in monomial */
} RSMonomial;

/* Monomial list metadata */
typedef struct {
    RSMonomial* terms;
    int         n_terms;    /* current count                    */
    int         capacity;   /* allocated capacity               */
} RSMonoList;

/*---- Main polynomial type ----*/
typedef struct {
    int  p;               /* prime modulus                          */
    int  n_vars;          /* number of Boolean variables            */
    int  n_terms;         /* number of monomials with c_S ≠ 0      */
    int  capacity;        /* allocated term capacity               */
    int* coeffs;          /* coeffs[t] ∈ GF(p)                     */
    int** mons;           /* mons[t][i] ∈ {0,1} — variable presence */
    int* degs;            /* degs[t] = Σ_i mons[t][i]              */
    int  is_normalized;   /* whether like terms have been combined  */
} GFPoly;

/*============================================================================
 * L5 — Polynomial Arithmetic API
 *==========================================================================*/

/* Lifecycle */
GFPoly* gf_poly_create(int p, int n_vars);
GFPoly* gf_poly_copy(const GFPoly* src);
void    gf_poly_free(GFPoly* pl);
void    gf_poly_reset(GFPoly* pl);

/* Monomial insertion */
void    gf_poly_add_mono(GFPoly* pl, int coeff, const int* exps);
void    gf_poly_add_mono_list(GFPoly* pl, const RSMonoList* ml);

/* Arithmetic (return new polynomial) */
GFPoly* gf_poly_add(const GFPoly* a, const GFPoly* b);
GFPoly* gf_poly_sub(const GFPoly* a, const GFPoly* b);
GFPoly* gf_poly_mul(const GFPoly* a, const GFPoly* b);
GFPoly* gf_poly_scalar_mul(const GFPoly* a, int c);
GFPoly* gf_poly_pow(const GFPoly* a, int exp);     /* a^exp mod p  */

/* In-place arithmetic (modifies first argument) */
void    gf_poly_iadd(GFPoly* a, const GFPoly* b);
void    gf_poly_isub(GFPoly* a, const GFPoly* b);
void    gf_poly_imul(GFPoly* a, const GFPoly* b);
void    gf_poly_iscale(GFPoly* a, int c);

/* Evaluation */
int     gf_poly_eval(const GFPoly* pl, const int* x);
int*    gf_poly_truth_table(const GFPoly* pl);      /* 2^n entries      */
int     gf_poly_agrees(const GFPoly* pl, int (*fn)(const int*,int), int n, int* frac_ok);

/* Degree */
int     gf_poly_degree(const GFPoly* pl);
int     gf_poly_total_degree(const GFPoly* pl);    /* max over all terms */
double  gf_poly_avg_degree(const GFPoly* pl);

/* Query */
int     gf_poly_is_zero(const GFPoly* pl);
int     gf_poly_is_constant(const GFPoly* pl);
int     gf_poly_n_terms(const GFPoly* pl);
int64_t gf_poly_sparsity(const GFPoly* pl);        /* number of monomials */

/* Normalization (combine like terms, remove zero coeffs) */
void    gf_poly_normalize(GFPoly* pl);
void    gf_poly_sort_by_deg(GFPoly* pl);

/* Comparison */
int     gf_poly_equal(const GFPoly* a, const GFPoly* b);

/* Special polynomials */
GFPoly* gf_poly_constant(int p, int n_vars, int c);
GFPoly* gf_poly_variable(int p, int n_vars, int var_idx);
GFPoly* gf_poly_random(int p, int n_vars, int max_deg, int max_terms, unsigned int seed);
GFPoly* gf_poly_zero(int p, int n_vars);
GFPoly* gf_poly_one(int p, int n_vars);

/*----------------------------------------------------------------------------
 * L3 — Mathematical structure utilities
 *----------------------------------------------------------------------------*/

/* GF(p) arithmetic helpers (standalone) */
int gf_add(int a, int b, int p);
int gf_sub(int a, int b, int p);
int gf_mul(int a, int b, int p);
int gf_pow(int a, int e, int p);
int gf_inv(int a, int p);           /* modular inverse (Fermat: a^{p-2})  */
int gf_neg(int a, int p);

/* Field properties check */
int gf_is_field(int p);             /* primality check for small p        */

/*----------------------------------------------------------------------------
 * Print / debug
 *----------------------------------------------------------------------------*/
void    gf_poly_print(const GFPoly* pl);
void    gf_poly_fprint(FILE* fp, const GFPoly* pl);
char*   gf_poly_to_string(const GFPoly* pl);

/*----------------------------------------------------------------------------
 * L5 — Gate polynomial constructors (Razborov-Smolensky specific)
 *----------------------------------------------------------------------------*/

/* AND gate: f(x) = ∏ x_i            — exact, degree k               */
GFPoly* poly_and_exact(int p, const int* var_indices, int k);

/* OR gate: f(x) = 1 - ∏(1-x_i)     — exact, degree k                */
GFPoly* poly_or_exact(int p, const int* var_indices, int k);

/* NOT gate: f(x) = 1 - x           — exact, degree 1                */
GFPoly* poly_not_exact(int p, int var_idx, int n_vars);

/* MOD_p gate: f(x)=1 iff Σx_i≡0 mod p — exact via Fermat, deg p-1  */
GFPoly* poly_modp_exact(int p, const int* var_indices, int k);

/* Probabilistic AND: random linear form L=Σ r_i x_i, output L^{p-1}  */
GFPoly* poly_and_probabilistic(int p, const int* var_indices, int k, unsigned int seed);

/* Probabilistic OR: De Morgan dual of probabilistic AND               */
GFPoly* poly_or_probabilistic(int p, const int* var_indices, int k, unsigned int seed);

/* Simple AND approximation: multiply first d variables                 */
GFPoly* poly_and_approx(int p, const int* var_indices, int k, int degree_limit);

/* Simple OR approximation: expand De Morgan to specified degree limit  */
GFPoly* poly_or_approx(int p, const int* var_indices, int k, int degree_limit);

/* MOD_p gate (simplified): 1 - (Σx_i)^{p-1}                           */
GFPoly* poly_modp_easy(int p, int n);

#endif /* GF_POLY_H */
