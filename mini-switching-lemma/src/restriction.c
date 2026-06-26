/* restriction.c -- Random Restriction Operations
 * ==============================================
 * Implements L3-L5: The random restriction, the key tool in
 * Hastad's proof of the switching lemma.
 *
 * RESTRICTION: A partial assignment rho: {x_1,...,x_n} -> {0, 1, *}
 *   - * (written as -1): variable remains free
 *   - 0: variable is fixed to 0
 *   - 1: variable is fixed to 1
 *
 * RANDOM RESTRICTION R_p: Each variable independently set:
 *   - * with probability p
 *   - 0 with probability (1-p)/2
 *   - 1 with probability (1-p)/2
 *
 * KEY PROPERTY: For a k-DNF, after random restriction with
 *   p ~ 1/(5k), the expected width drops substantially.
 *   With high probability, the restricted DNF is an s-CNF
 *   for s = O(log n).
 *
 * THEOREM (Hastad 1986): Pr[f|_rho is NOT an s-CNF] < (5pk)^s
 *
 * References:
 *   - Hastad, "Almost optimal lower bounds..." (1986)
 *   - Arora & Barak, Ch.14.2 (Random restrictions)
 *   - Beame, "A Switching Lemma Primer" (1994)
 */
#include "switching.h"

Restriction* restriction_create(int n_vars) {
    Restriction* r = (Restriction*)malloc(sizeof(Restriction));
    if (!r) return NULL;
    r->n_vars = n_vars;
    r->values = (int*)malloc((size_t)n_vars * sizeof(int));
    if (!r->values) { free(r); return NULL; }
    r->n_free = n_vars;
    for (int i = 0; i < n_vars; i++) r->values[i] = BOOL_UNDEF;
    return r;
}

void restriction_random(Restriction* r, double p_star) {
    if (!r) return;
    r->n_free = 0;
    for (int i = 0; i < r->n_vars; i++) {
        double d = (double)rand() / (double)RAND_MAX;
        if (d < p_star) {
            r->values[i] = BOOL_UNDEF;
            r->n_free++;
        } else if (d < p_star + (1.0 - p_star) / 2.0) {
            r->values[i] = BOOL_FALSE;
        } else {
            r->values[i] = BOOL_TRUE;
        }
    }
}

void restriction_random_seeded(Restriction* r, double p_star, unsigned int seed) {
    if (!r) return;
    srand(seed);
    restriction_random(r, p_star);
}

void restriction_set(Restriction* r, int var, int value) {
    if (!r || var < 0 || var >= r->n_vars) return;
    if (r->values[var] == BOOL_UNDEF && value != BOOL_UNDEF) r->n_free--;
    else if (r->values[var] != BOOL_UNDEF && value == BOOL_UNDEF) r->n_free++;
    r->values[var] = value;
}

int restriction_get(const Restriction* r, int var) {
    if (!r || var < 0 || var >= r->n_vars) return BOOL_UNDEF;
    return r->values[var];
}

int restriction_free_count(const Restriction* r) {
    return r ? r->n_free : 0;
}

void restriction_counts(const Restriction* r, int* n_zero, int* n_one, int* n_star) {
    if (!r) { if (n_zero) *n_zero = 0; if (n_one) *n_one = 0; if (n_star) *n_star = 0; return; }
    int nz = 0, no = 0, ns = 0;
    for (int i = 0; i < r->n_vars; i++) {
        if (r->values[i] == BOOL_UNDEF) ns++;
        else if (r->values[i] == BOOL_FALSE) nz++;
        else no++;
    }
    if (n_zero) *n_zero = nz;
    if (n_one)  *n_one  = no;
    if (n_star) *n_star = ns;
}

/* Apply restriction to a DNF formula.
 * For each term:
 *   - If any literal is fixed to 1 by the restriction: term is SATISFIED -> term becomes vacuous (T)
 *   - If all literals are fixed to 0: term is FALSIFIED -> remove term
 *   - Otherwise: keep surviving literal positions, remove fixed ones
 *
 * The returned DNF may have fewer terms and reduced width.
 * Terms that are satisfied by the restriction become empty (always true).
 */
DNF* dnf_restrict(const DNF* d, const Restriction* r) {
    if (!d || !r) return NULL;

    /* Count surviving terms after restriction */
    int ns = 0;
    int* surv = (int*)malloc((size_t)d->n_terms * (size_t)d->k_alloc * sizeof(int));
    int* new_sizes = (int*)malloc((size_t)d->n_terms * sizeof(int));
    if (!surv || !new_sizes) { free(surv); free(new_sizes); return NULL; }

    for (int t = 0; t < d->n_terms; t++) {
        int is_satisfied = 0;   /* term becomes T (always true) */
        int term_dead    = 1;   /* all remaining literals gone? */
        int literal_buf[64];
        int nl = 0;

        for (int l = 0; l < d->k_alloc; l++) {
            int lit = d->terms[t * d->k_alloc + l];
            if (lit == LITERAL_NULL) continue;

            int v   = LITERAL_VAR(lit);
            int sig = LITERAL_SIGN(lit);
            int rv  = restriction_get(r, v);

            if (rv == BOOL_UNDEF) {
                /* Variable is free: keep literal */
                if (nl < 64) literal_buf[nl++] = lit;
                term_dead = 0;
            } else if (sig == rv) {
                /* Literal evaluates to 1: term satisfied */
                is_satisfied = 1;
                break;
            }
            /* else: literal evaluates to 0, drop it */
        }

        if (is_satisfied) {
            /* Term is vacuously true -> encode as empty term */
            new_sizes[ns] = 0;
            ns++;
        } else if (!term_dead && nl > 0) {
            /* Term survives with reduced width */
            new_sizes[ns] = nl;
            for (int j = 0; j < nl; j++)
                surv[ns * d->k_alloc + j] = literal_buf[j];
            for (int j = nl; j < d->k_alloc; j++)
                surv[ns * d->k_alloc + j] = LITERAL_NULL;
            ns++;
        }
        /* term_dead && nl==0: term is falsified, drop it */
    }

    /* Build result DNF */
    DNF* result = dnf_create(d->n_vars, ns, d->k_alloc);
    if (!result) { free(surv); free(new_sizes); return NULL; }

    for (int t = 0; t < ns; t++) {
        int sz = new_sizes[t];
        for (int l = 0; l < sz; l++) {
            result->terms[t * d->k_alloc + l] = surv[t * d->k_alloc + l];
        }
    }

    free(surv);
    free(new_sizes);
    return result;
}

/* Apply restriction to a CNF formula.
 * For each clause:
 *   - If any literal is fixed to 1 by restriction: clause is SATISFIED -> remove clause
 *   - If all literals fixed to 0: clause is FALSIFIED -> clause becomes empty (F)
 *   - Otherwise: keep surviving literals
 */
CNF* cnf_restrict(const CNF* c, const Restriction* r) {
    if (!c || !r) return NULL;

    int ns = 0;
    int* surv = (int*)malloc((size_t)c->n_terms * (size_t)c->k_alloc * sizeof(int));
    int* new_sizes = (int*)malloc((size_t)c->n_terms * sizeof(int));
    if (!surv || !new_sizes) { free(surv); free(new_sizes); return NULL; }

    for (int cl = 0; cl < c->n_terms; cl++) {
        int clause_sat = 0;  /* clause satisfied by restriction */
        int lit_buf[64];
        int nl = 0;

        for (int l = 0; l < c->k_alloc; l++) {
            int lit = c->terms[cl * c->k_alloc + l];
            if (lit == LITERAL_NULL) continue;

            int v   = LITERAL_VAR(lit);
            int sig = LITERAL_SIGN(lit);
            int rv  = restriction_get(r, v);

            if (rv == BOOL_UNDEF) {
                if (nl < 64) lit_buf[nl++] = lit;
            } else if (sig == rv) {
                /* Literal true -> clause satisfied */
                clause_sat = 1;
                break;
            }
            /* Literal false -> drop it */
        }

        if (!clause_sat) {
            /* Clause survives */
            new_sizes[ns] = nl;
            for (int j = 0; j < nl; j++)
                surv[ns * c->k_alloc + j] = lit_buf[j];
            for (int j = nl; j < c->k_alloc; j++)
                surv[ns * c->k_alloc + j] = LITERAL_NULL;
            ns++;
        }
        /* clause_sat: satisfied clause removed (AND of T = skip) */
    }

    CNF* result = cnf_create(c->n_vars, ns, c->k_alloc);
    if (!result) { free(surv); free(new_sizes); return NULL; }

    for (int cl = 0; cl < ns; cl++) {
        int sz = new_sizes[cl];
        for (int l = 0; l < sz; l++) {
            result->terms[cl * c->k_alloc + l] = surv[cl * c->k_alloc + l];
        }
    }

    free(surv);
    free(new_sizes);
    return result;
}

Restriction* restriction_from_array(int n_vars, const int* settings) {
    if (!settings) return NULL;
    Restriction* r = restriction_create(n_vars);
    if (!r) return NULL;
    for (int i = 0; i < n_vars; i++) restriction_set(r, i, settings[i]);
    return r;
}

Restriction* restriction_clone(const Restriction* r) {
    if (!r) return NULL;
    Restriction* cp = restriction_create(r->n_vars);
    if (!cp) return NULL;
    memcpy(cp->values, r->values, (size_t)r->n_vars * sizeof(int));
    cp->n_free = r->n_free;
    return cp;
}

void restriction_free(Restriction* r) {
    if (r) { free(r->values); free(r); }
}

void restriction_print(const Restriction* r) {
    if (!r) { printf("Restriction(NULL)\n"); return; }
    printf("Restriction(n_vars=%d, n_free=%d): ", r->n_vars, r->n_free);
    for (int i = 0; i < r->n_vars && i < 32; i++) {
        if (r->values[i] == BOOL_UNDEF) printf("x%d=* ", i);
        else printf("x%d=%d ", i, r->values[i]);
    }
    if (r->n_vars > 32) printf("...");
    printf("\n");
}

/* ===== Restriction Analysis ===== */

/* Compute expected number of surviving literals in a k-DNF term
 * after a random restriction with probability p.
 * Each literal survives with probability p, dies with prob (1-p)/2,
 * and makes term true with prob (1-p)/2.
 * E[surviving literals] = p * k */
double restriction_expected_surviving_literals(int k, double p) {
    return p * (double)k;
}

/* Probability that a specific k-DNF term is satisfied by the restriction
 * (at least one literal matches the fixed value). */
double restriction_term_satisfied_prob(int k, double p) {
    /* Pr[not satisfied] = (1 - (1-p)/2)^k
     * Each literal: prob matches fixed = (1-p)/2, doesn't match = 1-(1-p)/2 */
    return 1.0 - pow(1.0 - (1.0 - p) / 2.0, (double)k);
}

/* Probability that a k-DNF term survives (neither satisfied nor falsified).
 * Survives iff: no literal matches fixed, and at least one literal is free. */
double restriction_term_survival_prob(int k, double p) {
    /* All literals must NOT match fixed value */
    double no_match = pow(1.0 - (1.0 - p) / 2.0, (double)k);
    /* All literals must NOT be free (would mean all fixed to wrong value -> dead) */
    double all_dead = pow(1.0 - p - (1.0 - p) / 2.0, (double)k);
    return no_match - all_dead;
}
