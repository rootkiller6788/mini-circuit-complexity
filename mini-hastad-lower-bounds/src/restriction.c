/**
 * restriction.c -- Random Restriction Process
 *
 * A restriction rho: {x_1,...,x_n} -> {0,1,*} assigns each variable
 * to 0, 1, or leaves it free (*). Distribution R_p:
 *   Pr[rho(x_i)=*] = p
 *   Pr[rho(x_i)=0] = Pr[rho(x_i)=1] = (1-p)/2
 *
 * Key property: under R_p with p = 1/(10k), a k-DNF becomes
 * a small-depth decision tree w.h.p. (Switching Lemma).
 *
 * L2: Restriction concept, random restriction distribution R_p
 * L3: Restriction data structure, composition, probability space
 * L5: Iterative restriction algorithm
 *
 * References:
 *   Hastad (1986), Furst-Saxe-Sipser (1981)
 *   Arora-Barak (2009) Ch.14.2
 */
#include "hastad.h"
#include <string.h>

/* =================================================================
 * L3: Restriction Creation and Management
 * ================================================================= */

/** Create random restriction from distribution R_p. */
Restriction* restriction_create(int n, double p) {
    Restriction* r = (Restriction*)malloc(sizeof(Restriction));
    if (!r) return NULL;
    r->n_vars = n;
    r->p = p;
    r->map = (int*)malloc(n * sizeof(int));
    r->free_indices = (int*)malloc(n * sizeof(int));
    if (!r->map || !r->free_indices) {
        free(r->map); free(r->free_indices); free(r); return NULL;
    }
    restriction_rerandomize(r);
    return r;
}

/** Create restriction from explicit assignment map. */
Restriction* restriction_from_map(int n, const int* map) {
    Restriction* r = (Restriction*)malloc(sizeof(Restriction));
    if (!r) return NULL;
    r->n_vars = n;
    r->p = 0.0;
    r->map = (int*)malloc(n * sizeof(int));
    r->free_indices = (int*)malloc(n * sizeof(int));
    if (!r->map || !r->free_indices) {
        free(r->map); free(r->free_indices); free(r); return NULL;
    }
    r->n_free = 0;
    for (int i = 0; i < n; i++) {
        r->map[i] = map[i];
        if (map[i] == -1) r->free_indices[r->n_free++] = i;
        /* Update p to match observed free fraction */
    }
    r->p = (double)r->n_free / n;
    return r;
}

/** Create trivial restriction (all vars free). */
Restriction* restriction_all_free(int n) {
    int* map = (int*)malloc(n * sizeof(int));
    if (!map) return NULL;
    for (int i = 0; i < n; i++) map[i] = -1;
    Restriction* r = restriction_from_map(n, map);
    free(map);
    return r;
}

/** Clone a restriction. */
Restriction* restriction_clone(const Restriction* r) {
    if (!r) return NULL;
    Restriction* c = (Restriction*)malloc(sizeof(Restriction));
    if (!c) return NULL;
    c->n_vars = r->n_vars;
    c->p = r->p;
    c->n_free = r->n_free;
    c->map = (int*)malloc(r->n_vars * sizeof(int));
    c->free_indices = (int*)malloc(r->n_vars * sizeof(int));
    if (!c->map || !c->free_indices) {
        free(c->map); free(c->free_indices); free(c); return NULL;
    }
    memcpy(c->map, r->map, r->n_vars * sizeof(int));
    memcpy(c->free_indices, r->free_indices, r->n_vars * sizeof(int));
    return c;
}

/** Free restriction. */
void restriction_free(Restriction* r) {
    if (!r) return;
    free(r->map);
    free(r->free_indices);
    free(r);
}

/* =================================================================
 * L3: Restriction Operations
 * ================================================================= */

/** Set a specific variable. Must update free_indices. */
void restriction_set(Restriction* r, int var, int value) {
    if (!r || var < 0 || var >= r->n_vars) return;
    int was_free = (r->map[var] == -1);
    r->map[var] = value;
    if (was_free && value != -1) {
        /* Variable was free, now fixed: remove from free_indices */
        int pos = -1;
        for (int i = 0; i < r->n_free; i++) {
            if (r->free_indices[i] == var) { pos = i; break; }
        }
        if (pos >= 0) {
            memmove(&r->free_indices[pos], &r->free_indices[pos + 1],
                    (r->n_free - pos - 1) * sizeof(int));
            r->n_free--;
        }
    } else if (!was_free && value == -1) {
        /* Variable was fixed, now free: add to free_indices */
        r->free_indices[r->n_free++] = var;
    }
    r->p = (double)r->n_free / r->n_vars;
}

/** Re-randomize from R_p distribution. */
void restriction_rerandomize(Restriction* r) {
    if (!r) return;
    r->n_free = 0;
    for (int i = 0; i < r->n_vars; i++) {
        double pr = (double)rand() / RAND_MAX;
        if (pr < r->p) {
            r->map[i] = -1;  /* free (star) */
            r->free_indices[r->n_free++] = i;
        } else if (pr < r->p + (1.0 - r->p) / 2.0) {
            r->map[i] = 0;
        } else {
            r->map[i] = 1;
        }
    }
}

int restriction_n_free(const Restriction* r) {
    return r ? r->n_free : 0;
}

int restriction_get(const Restriction* r, int var) {
    if (!r || var < 0 || var >= r->n_vars) return -1;
    return r->map[var];
}

int restriction_is_free(const Restriction* r, int var) {
    if (!r || var < 0 || var >= r->n_vars) return 0;
    return r->map[var] == -1;
}

int restriction_free_index(const Restriction* r, int j) {
    if (!r || j < 0 || j >= r->n_free) return -1;
    return r->free_indices[j];
}

void restriction_print(const Restriction* r) {
    if (!r) { printf("NULL\n"); return; }
    printf("R(n=%d,p=%.3f,free=%d): ", r->n_vars, r->p, r->n_free);
    for (int i = 0; i < r->n_vars; i++) {
        if (r->map[i] == -1) putchar('*');
        else putchar('0' + r->map[i]);
    }
    printf("\n");
}

/* =================================================================
 * L2: Restricted Function Evaluation
 * ================================================================= */

/** Evaluate f on x under restriction r.
 *  x should have length r->n_free (values only for free variables). */
int restriction_evaluate(const Restriction* r,
                          int (*f)(const int*, int),
                          const int* x) {
    if (!r || !f) return 0;
    int* full_x = (int*)malloc(r->n_vars * sizeof(int));
    if (!full_x) return 0;

    /* Fill fixed values from restriction */
    for (int i = 0; i < r->n_vars; i++) {
        if (r->map[i] == -1) full_x[i] = 0;  /* will overwrite */
        else full_x[i] = r->map[i];
    }
    /* Overlay free variable values */
    for (int j = 0; j < r->n_free; j++) {
        full_x[r->free_indices[j]] = x[j] & 1;
    }

    int result = f(full_x, r->n_vars);
    free(full_x);
    return result;
}

/** Count assignments to free vars where f=1. */
int64_t restriction_count_ones(const Restriction* r,
                                int (*f)(const int*, int)) {
    if (!r || !f) return 0;
    int64_t count = 0;
    int64_t max_assign = (int64_t)1 << r->n_free;
    if (max_assign > (1 << 20)) max_assign = (1 << 20);

    int* xx = (int*)malloc(r->n_vars * sizeof(int));
    if (!xx) return 0;

    for (int i = 0; i < r->n_vars; i++)
        xx[i] = (r->map[i] != -1) ? (r->map[i] & 1) : 0;

    for (int64_t a = 0; a < max_assign; a++) {
        for (int j = 0; j < r->n_free; j++)
            xx[r->free_indices[j]] = (a >> j) & 1;
        if (f(xx, r->n_vars) == 1) count++;
    }
    free(xx);
    return count;
}

/* =================================================================
 * L3: Restriction Composition
 * ================================================================= */

/** Compose two restrictions: rho2 o rho1.
 *  rho1: original vars -> {0,1,*}
 *  rho2: free vars of rho1 -> {0,1,*}
 *  Result: original vars -> {0,1,*} */
Restriction* restriction_compose(const Restriction* r1,
                                  const Restriction* r2) {
    if (!r1 || !r2) return NULL;
    /* r2 operates on r1's free variables.
     * For each original variable i:
     *   if r1 fixed i -> keep value
     *   if r1 left i free -> apply r2 to this free var */
    Restriction* out = (Restriction*)malloc(sizeof(Restriction));
    if (!out) return NULL;
    out->n_vars = r1->n_vars;
    out->n_free = 0;
    out->p = 0.0;
    out->map = (int*)malloc(r1->n_vars * sizeof(int));
    out->free_indices = (int*)malloc(r1->n_vars * sizeof(int));
    if (!out->map || !out->free_indices) {
        free(out->map); free(out->free_indices); free(out); return NULL;
    }

    for (int i = 0; i < r1->n_vars; i++) {
        if (r1->map[i] != -1) {
            /* Already fixed by r1: keep value */
            out->map[i] = r1->map[i];
        } else {
            /* Find position in r1's free list */
            int pos = -1;
            for (int j = 0; j < r1->n_free; j++) {
                if (r1->free_indices[j] == i) { pos = j; break; }
            }
            if (pos >= 0 && pos < r2->n_vars) {
                out->map[i] = r2->map[pos];
                if (r2->map[pos] == -1) {
                    out->free_indices[out->n_free++] = i;
                }
            } else {
                out->map[i] = -1;
                out->free_indices[out->n_free++] = i;
            }
        }
    }
    out->p = (double)out->n_free / out->n_vars;
    return out;
}

/** Check if r1 refines r2: every var fixed in r2 is fixed to the
 *  same value in r1, and r1 may fix additional variables. */
int restriction_is_refinement(const Restriction* r1,
                               const Restriction* r2) {
    if (!r1 || !r2) return 0;
    if (r1->n_vars != r2->n_vars) return 0;
    for (int i = 0; i < r1->n_vars; i++) {
        if (r2->map[i] != -1 && r1->map[i] != r2->map[i]) return 0;
    }
    return 1;
}

/* =================================================================
 * L5: DNF/CNF Under Restriction Analysis
 * ================================================================= */

/** Estimate surviving DNF terms after restriction.
 *  Each literal is set to * with prob p, and term survives
 *  if no literal is forced to 0. */
int dnf_surviving_terms_estimate(int n_terms, int term_width,
                                  const Restriction* r) {
    /* A term survives if no literal evaluates to 0.
     * Each literal: prob(not forced to 0) = 1 - (1-p)/2 = (1+p)/2
     * Term of width w survives with prob = ((1+p)/2)^w */
    double prob_survive = pow((1.0 + r->p) / 2.0, term_width);
    return (int)(n_terms * prob_survive);
}

/** Check if a DNF term is killed by restriction. */
int dnf_term_killed(const int* term, int n_vars, const Restriction* r) {
    for (int v = 0; v < n_vars; v++) {
        if (term[v] == -1) continue;  /* variable absent from term */
        if (r->map[v] == -1) continue;  /* variable free */
        /* term[v]=1 means x_v, term[v]=0 means NOT x_v */
        if (term[v] == 1 && r->map[v] == 0) return 1;  /* x_v forced 0 */
        if (term[v] == 0 && r->map[v] == 1) return 1;  /* NOT x_v forced 0 */
    }
    return 0;
}

/** Check if a DNF term becomes constant 1 under restriction. */
int dnf_term_forced_true(const int* term, int n_vars, const Restriction* r) {
    int all_fixed_correct = 1;
    int has_free_literal = 0;
    for (int v = 0; v < n_vars; v++) {
        if (term[v] == -1) continue;
        if (r->map[v] == -1) { has_free_literal = 1; continue; }
        if (term[v] == 1 && r->map[v] == 0) return 0;
        if (term[v] == 0 && r->map[v] == 1) return 0;
    }
    /* Term is forced true if all its literals that are fixed match,
     * and there are no remaining free variables that could break it. */
    return !has_free_literal;
}

/* =================================================================
 * L5: Decision Tree Depth Under Restriction
 * ================================================================= */

/** Estimate expected decision tree depth for k-DNF under R_p.
 *  Bound: E[depth] <= C * k * log(1/p) for some constant C. */
double decision_tree_depth_estimate(int n, int k, double p) {
    /* Simplified: each level reduces "active" variables */
    double expected = k * log(1.0 / p) / log(2.0);
    return expected;
}

/** Switching lemma probability bound: Pr[f|rho is not s-CNF].
 *  Hastad (1986): Bound <= (5pk)^s. */
double switching_lemma_bound(int k, double p, int s) {
    double x = 5.0 * p * k;
    if (x >= 1.0) return 1.0;
    return pow(x, (double)s);
}

/** Optimal p for Hastad depth reduction: p = n^{-1/(d-1)}.
 *  This choice balances surviving variables with switching probability. */
double optimal_restriction_prob(int n, int depth) {
    if (depth <= 1) return 1.0;
    return pow((double)n, -1.0 / (depth - 1));
}

/** Expected number of surviving variables after R_p restriction. */
double expected_surviving_vars(int n, double p) {
    return n * p;
}

/** Variance of surviving variable count under R_p. */
double surviving_vars_variance(int n, double p) {
    return n * p * (1.0 - p);
}

/* =================================================================
 * L5: Monte Carlo Analysis of Restrictions
 * ================================================================= */

/** Monte Carlo estimate of expected surviving variables. */
double monte_carlo_surviving_vars(int n, double p, int trials) {
    double total = 0.0;
    for (int t = 0; t < trials; t++) {
        int surv = 0;
        for (int i = 0; i < n; i++) {
            if ((double)rand() / RAND_MAX < p) surv++;
        }
        total += surv;
    }
    return total / trials;
}

/** Probability that at least k variables survive under R_p.
 *  Uses normal approximation to binomial distribution. */
double prob_at_least_k_surviving(int n, double p, int k) {
    if (k <= 0) return 1.0;
    if (k > n) return 0.0;

    double mean = n * p;
    double std = sqrt(n * p * (1.0 - p));

    if (std < 0.001) return (mean >= k) ? 1.0 : 0.0;

    /* Normal approximation with continuity correction */
    double z = (k - 0.5 - mean) / std;
    /* Q-function: P(Z > z) = 1 - Phi(z) */
    double q = 0.5 * erfc(z / sqrt(2.0));
    return q;
}

/** Probability that at most k variables survive. */
double prob_at_most_k_surviving(int n, double p, int k) {
    if (k >= n) return 1.0;
    if (k < 0) return 0.0;

    double mean = n * p;
    double std = sqrt(n * p * (1.0 - p));

    if (std < 0.001) return (mean <= k) ? 1.0 : 0.0;

    double z = (k + 0.5 - mean) / std;
    double Phi_z = 0.5 * (1.0 + erf(z / sqrt(2.0)));
    return Phi_z;
}

/**
 * Estimate total restriction process across d-1 rounds.
 *
 * Start: n vars, depth d.
 * Round 1: p_1 = n^{-1/(d-1)}, n_1 = n * p_1
 * Round 2: p_2 = n_1^{-1/(d-2)}, n_2 = n_1 * p_2
 * ...
 * Final: n_{d-1} vars at depth 2.
 *
 * Expected final vars: n * prod_{i=1}^{d-1} n_i^{-1/(d-i)}
 * = n * n^{-1/(d-1)} * (n * n^{-1/(d-1)})^{-1/(d-2)} * ...
 * This roughly gives Theta(1) variables remaining.
 */
void restriction_process_simulate(int n, int d, double* final_p,
                                   double* final_n) {
    int cur_n = n;
    double cur_p = 1.0;
    for (int r = 0; r < d - 1; r++) {
        cur_p = pow((double)cur_n, -1.0 / (d - r - 1));
        cur_n = (int)(cur_n * cur_p);
        if (cur_n < 1) cur_n = 1;
    }
    *final_p = cur_p;
    *final_n = cur_n;
}

/**
 * Generate n independent random restrictions from R_p,
 * count free variables in each, return statistics.
 */
void restriction_statistics(int n, double p, int trials,
                             double* mean, double* stddev,
                             int* min_free, int* max_free) {
    *min_free = n;
    *max_free = 0;
    double sum = 0.0, sum_sq = 0.0;

    for (int t = 0; t < trials; t++) {
        int free = 0;
        for (int i = 0; i < n; i++) {
            if ((double)rand() / RAND_MAX < p) free++;
        }
        sum += free;
        sum_sq += free * free;
        if (free < *min_free) *min_free = free;
        if (free > *max_free) *max_free = free;
    }

    *mean = sum / trials;
    *stddev = sqrt(sum_sq / trials - (*mean) * (*mean));
}