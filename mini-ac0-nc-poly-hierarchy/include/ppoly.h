/* ppoly.h — P/poly Definitions
 * =====================================================================
 * P/poly: languages decidable by polynomial-size circuit families.
 * A circuit family {C_n} has size poly(n) but need not be uniform
 * (no algorithm required to construct C_n from n).
 *
 * Key facts:
 *   P ⊆ P/poly (any P-algorithm can be unrolled to a circuit family)
 *   BPP ⊆ P/poly (Adleman 1978: derandomization with advice)
 *   P/poly is NOT recursively presentable (no enumeration)
 *   P/poly has NO known complete problem
 *   Karp-Lipton: NP ⊆ P/poly ⇒ PH = Σ₂^p (collapse)
 *   If NEXP ⊆ P/poly then NEXP = MA (Impagliazzo-Kabanets-Wigderson 2002)
 *
 * Non-uniform computation allows different circuits for different
 * input sizes, with no requirement that the circuits be efficiently
 * constructible. The "advice string" α(n) captures non-uniformity.
 *
 * References:
 *   AB §6.1, §6.8, §14.1
 *   Karp & Lipton (1982) — NP in P/poly ⇒ PH collapses
 *   Adleman (1978) — BPP ⊆ P/poly
 */
#ifndef PPOLY_H
#define PPOLY_H

#include "ac0nc.h"

/* ─── P/poly Circuit Family ──────────────────────────────────────── */
/* A non-uniform circuit family: for each input size n, a circuit C_n.
 * The family is represented compactly via a constructor function. */
typedef struct AC0Circuit AC0Circuit;

typedef AC0Circuit* (*CircuitConstructor)(int n);

typedef struct {
    CircuitConstructor  constructor;  /* builds C_n from n */
    int                 max_size;     /* max |C_n| across n ≤ max_size */
    const char         *language_name;
} PPolyFamily;

PPolyFamily* ppoly_family_create(CircuitConstructor f,
                                   const char *name);
void         ppoly_family_free(PPolyFamily *fam);
AC0Circuit*  ppoly_family_get_circuit(PPolyFamily *fam, int n);

/* ─── Advice Strings ─────────────────────────────────────────────── */
/* An advice string α(n) of length a(n) is prepended to the input.
 * Language L ∈ P/poly iff ∃ poly p, advice α, and language A ∈ P
 * such that x ∈ L ⇔ (x, α(|x|)) ∈ A for all x. */
typedef struct {
    int          *bits;
    int           length;
    double         generation_time_ms;
    const char    *description;
} Advice;

Advice* advice_create(int length);
void    advice_free(Advice *a);
int     advice_get(Advice *a, int index);
void    advice_set(Advice *a, int index, int value);
void    advice_randomize(Advice *a);

/* ─── Non-Uniform Complexity Classes ───────────────────────────── */
/* SIZE(s(n)): languages decidable by circuits of size O(s(n)).
 * SIZE(poly(n)) = P/poly.
 * SIZE(exp(n)) = all languages (every language has exp-size circuits). */
int    language_in_size(const char *lang, double size_bound);
double size_lower_bound_for_function(int n, int (*f)(const int *));

/* ─── Karp-Lipton Theorem ──────────────────────────────────────── */
/* If SAT has polynomial-size circuits, then PH = Σ₂^p.
 * This is a fundamental connection between circuit complexity
 * and the polynomial hierarchy. */
int karp_lipton_check(int sat_circuit_size);

/* ─── Impagliazzo-Kabanets-Wigderson (IKW) ──────────────────────── */
/* If NEXP ⊆ P/poly, then NEXP = MA.
 * If NEXP ⊆ P/poly, then derandomization of MA is possible. */
int ikw_check(int nexp_circuit_size);

/* ─── SIZE Hierarchy ───────────────────────────────────────────── */
/* SIZE(s(n)) ⊂ SIZE(2^{O(s(n))}) (non-uniform hierarchy theorem).
 * Unlike time/space hierarchies, non-uniform hierarchy is
 * much coarser since we cannot diagonalize against advice. */
double size_hierarchy_bound(double s_n);

#endif /* PPOLY_H */
