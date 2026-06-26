/**
 * restriction.h — Random Restriction Process
 *
 * A restriction ρ: {x_1,...,x_n} → {0,1,*} assigns each variable
 * to 0, 1, or leaves it free (*). The probability distribution R_p:
 *   Pr[ρ(x_i)=*] = p
 *   Pr[ρ(x_i)=0] = Pr[ρ(x_i)=1] = (1-p)/2
 *
 * Key property: under R_p with p = 1/(10k), a k-DNF becomes
 * a small-depth decision tree with high probability.
 *
 * L2: Restriction concept, random restriction distribution
 * L3: Probability space R_p, restriction as partial assignment
 * L5: Iterative restriction for depth reduction
 */
#ifndef RESTRICTION_H
#define RESTRICTION_H

#include <stdint.h>

/* ── Restriction Data Structure ──────────────────────────────── */

typedef struct {
    int n_vars;          /* total variables in original domain */
    int* map;            /* map[i] ∈ {-1,0,1}: -1=free, 0→0, 1→1 */
    int n_free;          /* number of free (starred) variables */
    int* free_indices;   /* indices of free variables */
    double p;            /* probability p (keep probability) */
} Restriction;

/* ── Restriction Creation ────────────────────────────────────── */

/** Create a restriction with given keep probability p.
 *  Each var: * with prob p, 0 with prob (1-p)/2, 1 with prob (1-p)/2. */
Restriction* restriction_create(int n, double p);

/** Create a restriction from explicit map */
Restriction* restriction_from_map(int n, const int* map);

/** Create the trivial restriction (all vars free, p=1) */
Restriction* restriction_all_free(int n);

/** Clone a restriction */
Restriction* restriction_clone(const Restriction* r);

/** Free restriction memory */
void restriction_free(Restriction* r);

/* ── Restriction Operations ──────────────────────────────────── */

/** Apply restriction to a specific variable */
void restriction_set(Restriction* r, int var, int value);

/** Re-randomize: draw new assignment from R_p */
void restriction_rerandomize(Restriction* r);

/** Count surviving (free) variables */
int restriction_n_free(const Restriction* r);

/** Get the value assigned to variable i */
int restriction_get(const Restriction* r, int var);

/** Check if variable i is free */
int restriction_is_free(const Restriction* r, int var);

/** Get the index of the j-th free variable in the original domain */
int restriction_free_index(const Restriction* r, int j);

/** Print restriction as string (e.g., "01*0*11") */
void restriction_print(const Restriction* r);

/* ── Restricted Function Evaluation ──────────────────────────── */

/** Evaluate f on restricted input: fill free vars from x, fixed vars from r.
 *  x should have length r->n_free (only values for free variables). */
int restriction_evaluate(const Restriction* r,
                          int (*f)(const int*, int),
                          const int* x);

/** Evaluate f on all 2^{n_free} assignments, count those with f(x)=1 */
int64_t restriction_count_ones(const Restriction* r,
                                int (*f)(const int*, int));

/* ── Restriction Composition ─────────────────────────────────── */

/** Compose two restrictions: r2 ∘ r1.
 *  r1 maps original vars, r2 maps r1's free vars. */
Restriction* restriction_compose(const Restriction* r1,
                                  const Restriction* r2);

/** Check if restriction r1 is a refinement of r2
 *  (every var fixed in r2 is fixed to same value in r1) */
int restriction_is_refinement(const Restriction* r1,
                               const Restriction* r2);

/* ── CNF/DNF Under Restriction ───────────────────────────────── */

/** How many terms survive a restriction? (rough estimate for DNF) */
int dnf_surviving_terms_estimate(int n_terms, int term_width, const Restriction* r);

/** Check if DNF term is killed by restriction (evaluates to 0) */
int dnf_term_killed(const int* term, int n_vars, const Restriction* r);

/** Check if DNF term becomes constant 1 under restriction */
int dnf_term_forced_true(const int* term, int n_vars, const Restriction* r);

/* ── Decision Tree Depth Under Restriction ───────────────────── */

/** Estimate decision tree depth of restricted k-DNF (based on p, k) */
double decision_tree_depth_estimate(int n, int k, double p);

/** Bound probability that a k-DNF under R_p has DT depth > s */
double switching_lemma_bound(int k, double p, int s);

/** Compute optimal p for depth reduction: p = n^{-1/(d-1)} */
double optimal_restriction_prob(int n, int depth);

/** Compute expected surviving variables after restriction */
double expected_surviving_vars(int n, double p);

/** Compute variance of surviving variable count */
double surviving_vars_variance(int n, double p);

/* ── Restriction Distribution Analysis ───────────────────────── */

/** Monte Carlo estimate of surviving variables */
double monte_carlo_surviving_vars(int n, double p, int trials);

/** Probability that at least k variables survive */
double prob_at_least_k_surviving(int n, double p, int k);

/** Probability that at most k variables survive */
double prob_at_most_k_surviving(int n, double p, int k);

#endif /* RESTRICTION_H */
