/**
 * src/formula.c — DNF/CNF Formula Implementation
 * ==============================================
 *
 * Implements the core formula data structure and operations for
 * Disjunctive Normal Form (OR of ANDs) and Conjunctive Normal Form
 * (AND of ORs). This is the foundation upon which the switching lemma
 * operates.
 *
 * THEORETICAL BACKGROUND (L1-L3):
 *
 *   DNF: A formula is in Disjunctive Normal Form if it is an OR
 *   (disjunction) of one or more terms, where each term is an AND
 *   (conjunction) of one or more literals. A literal is either a
 *   variable x_i or its negation ~x_i.
 *
 *     DNF = T_1 ∨ T_2 ∨ ... ∨ T_n
 *     where each T_j = l_{j,1} ∧ l_{j,2} ∧ ... ∧ l_{j,k_j}
 *
 *   k-DNF: A DNF where each term has at most k literals.
 *
 *   CNF: A formula is in Conjunctive Normal Form if it is an AND
 *   (conjunction) of one or more clauses, where each clause is an OR
 *   (disjunction) of one or more literals.
 *
 *     CNF = C_1 ∧ C_2 ∧ ... ∧ C_n
 *     where each C_j = l_{j,1} ∨ l_{j,2} ∨ ... ∨ l_{j,s_j}
 *
 *   s-CNF: A CNF where each clause has at most s literals.
 *
 *   SATISFIABILITY: A formula is satisfiable if there exists an
 *   assignment to its variables that makes the formula evaluate to 1.
 *
 * LITERAL ENCODING (from switching.h):
 *   - Literal encoded as (variable_index << 1) | sign_bit
 *   - sign_bit = 0 for negated (~x_i), 1 for positive (x_i)
 *   - LITERAL_NULL = -1 marks empty/deleted slots
 *
 * COURSE MAPPING:
 *   - MIT 6.841: Boolean formulas, DNF/CNF representation
 *   - Stanford CS254: Propositional logic for complexity
 *   - Berkeley CS278: SAT and circuit representations
 *   - CMU 15-855: Normal forms and decision procedures
 *
 * COMPLEXITY NOTES:
 *   - DNF/CNF evaluation: O(terms * width), linear in formula size
 *   - Exact DNF↔CNF conversion: exponential in worst case
 *   - Satisfying assignment count (#DNF-SAT): #P-complete
 *   - k-DNF SAT: P (since k-DNF → small decision tree)
 *
 * REFERENCES:
 *   - Arora & Barak (2009), Section 1.2: Boolean formulas
 *   - Sipser (2013), Chapter 7: Time complexity
 *   - Garey & Johnson (1979): Appendix on NP-complete problems
 */

#include "switching.h"

#include <stdint.h>
#include <time.h>

/* ================================================================
 * Internal helpers
 * ================================================================ */

/* PRNG state (simple xorshift128+ for reproducibility with seeds) */
static uint64_t prng_state[2] = {0x123456789ABCDEF1ULL, 0xDEADBEEFCAFEBABEULL};

void formula_set_seed(unsigned int seed) {
    prng_state[0] = ((uint64_t)seed << 32) | (seed ^ 0x9E3779B9);
    prng_state[1] = ((uint64_t)(seed * 2654435761U) << 16) | 0xDEAD;
}

static uint64_t xorshift128plus(void) {
    uint64_t s1 = prng_state[0];
    const uint64_t s0 = prng_state[1];
    prng_state[0] = s0;
    s1 ^= s1 << 23;
    prng_state[1] = s1 ^ s0 ^ (s1 >> 18) ^ (s0 >> 5);
    return prng_state[1] + s0;
}

/* Return random integer in [0, n-1] */
static int rand_int(int n) {
    if (n <= 0) return 0;
    return (int)(xorshift128plus() % (uint64_t)n);
}

/* Return random sign: 0 or 1 */
static int rand_sign(void) {
    return (int)(xorshift128plus() & 1);
}

/* Check if a literal value is the null sentinel */
static int is_literal_null(int lit) {
    return lit == LITERAL_NULL;
}

/* Copy literals from src to dst, both arrays of length width */
static void copy_literals(int* dst, const int* src, int width) {
    for (int i = 0; i < width; i++) {
        dst[i] = src[i];
    }
}

/* ================================================================
 * L1-L3: FORMULA CREATION AND DESTRUCTION
 * ================================================================ */

/**
 * dnf_create — Allocate a new DNF formula.
 *
 * @param n_vars   Number of Boolean variables (x_0, ..., x_{n_vars-1})
 * @param n_terms  Number of terms in the OR
 * @param k_alloc  Allocated width per term (allows growth beyond actual k)
 * @return         Newly allocated DNF, or NULL on allocation failure
 *
 * Complexity: O(n_terms * k_alloc)
 * The internal terms array is initialized with LITERAL_NULL in all positions.
 */
DNF* dnf_create(int n_vars, int n_terms, int k_alloc) {
    if (n_vars < 1 || n_terms < 1 || k_alloc < 1) return NULL;

    DNF* d = (DNF*)calloc(1, sizeof(DNF));
    if (!d) return NULL;

    d->n_vars  = n_vars;
    d->n_terms = n_terms;
    d->k_alloc = k_alloc;
    d->is_cnf  = 0;

    int total = n_terms * k_alloc;
    d->terms = (int*)malloc((size_t)total * sizeof(int));
    if (!d->terms) {
        free(d);
        return NULL;
    }
    for (int i = 0; i < total; i++) {
        d->terms[i] = LITERAL_NULL;
    }
    return d;
}

/**
 * cnf_create — Allocate a new CNF formula.
 *
 * @param n_vars     Number of Boolean variables
 * @param n_clauses  Number of clauses in the AND
 * @param s_alloc    Allocated width per clause
 * @return           Newly allocated CNF, or NULL on allocation failure
 *
 * Internally uses the same Formula struct with is_cnf = 1.
 * The terms array holds clauses (for CNF, "term" = clause).
 */
CNF* cnf_create(int n_vars, int n_clauses, int s_alloc) {
    if (n_vars < 1 || n_clauses < 1 || s_alloc < 1) return NULL;

    CNF* c = (CNF*)calloc(1, sizeof(CNF));
    if (!c) return NULL;

    c->n_vars  = n_vars;
    c->n_terms = n_clauses;
    c->k_alloc = s_alloc;
    c->is_cnf  = 1;

    int total = n_clauses * s_alloc;
    c->terms = (int*)malloc((size_t)total * sizeof(int));
    if (!c->terms) {
        free(c);
        return NULL;
    }
    for (int i = 0; i < total; i++) {
        c->terms[i] = LITERAL_NULL;
    }
    return c;
}

/* ================================================================
 * LITERAL MANIPULATION
 * ================================================================ */

/**
 * dnf_set_literal — Set a literal at a specific position in a DNF term.
 *
 * @param d        DNF formula
 * @param term     Term index (0-based)
 * @param lit_pos  Position within term (0-based, < k_alloc)
 * @param var      Variable index (0-based, < n_vars)
 * @param sign     LITERAL_POS (x_var) or LITERAL_NEG (~x_var)
 *
 * Complexity: O(1)
 * Precondition: term < n_terms, lit_pos < k_alloc, var < n_vars
 */
void dnf_set_literal(DNF* d, int term, int lit_pos, int var, int sign) {
    if (!d || term < 0 || term >= d->n_terms) return;
    if (lit_pos < 0 || lit_pos >= d->k_alloc) return;
    if (var < 0 || var >= d->n_vars) return;

    d->terms[term * d->k_alloc + lit_pos] = MAKE_LITERAL(var, sign);
}

/**
 * cnf_set_literal — Set a literal at a specific position in a CNF clause.
 *
 * For CNF, a "term" in the Formula struct corresponds to a clause.
 */
void cnf_set_literal(CNF* c, int clause, int lit_pos, int var, int sign) {
    if (!c || clause < 0 || clause >= c->n_terms) return;
    if (lit_pos < 0 || lit_pos >= c->k_alloc) return;
    if (var < 0 || var >= c->n_vars) return;

    c->terms[clause * c->k_alloc + lit_pos] = MAKE_LITERAL(var, sign);
}

/* ================================================================
 * FORMULA EVALUATION
 * ================================================================ */

/**
 * formula_evaluate — Evaluate a formula (DNF or CNF) on a given assignment.
 *
 * DNF semantics:
 *   f(x) = ∨_{t} (∧_{l∈term_t} eval_lit(l, x))
 *   where eval_lit((v, sign), x) = x_v if sign=1, else ~x_v
 *
 * CNF semantics:
 *   f(x) = ∧_{c} (∨_{l∈clause_c} eval_lit(l, x))
 *
 * For DNF: a term is true iff ALL its non-null literals are true.
 *          The formula is true iff ANY term is true.
 *
 * For CNF: a clause is true iff ANY of its non-null literals is true.
 *          The formula is true iff ALL clauses are true.
 *
 * @param f      Formula (DNF or CNF)
 * @param assign Assignment array of length n_vars, assign[v] ∈ {0, 1}
 * @return       1 if formula evaluates to true, 0 otherwise
 *
 * Complexity: O(n_terms * k_alloc), linear in formula size
 */
int formula_evaluate(const Formula* f, const int* assign) {
    if (!f || !assign) return 0;

    if (f->is_cnf) {
        /* CNF: AND of ORs. All clauses must be satisfied. */
        for (int c = 0; c < f->n_terms; c++) {
            int clause_true = 0;
            for (int l = 0; l < f->k_alloc; l++) {
                int lit = f->terms[c * f->k_alloc + l];
                if (is_literal_null(lit)) continue;

                int var  = LITERAL_VAR(lit);
                int sign = LITERAL_SIGN(lit);
                int val  = (var >= 0 && var < f->n_vars) ? assign[var] : 0;

                /* A clause in CNF is satisfied if ANY literal is true */
                if ((sign == LITERAL_POS && val == 1) ||
                    (sign == LITERAL_NEG && val == 0)) {
                    clause_true = 1;
                    break;  /* no need to check remaining literals in clause */
                }
            }
            if (!clause_true) return 0;  /* this clause failed */
        }
        return 1;  /* all clauses passed */
    } else {
        /* DNF: OR of ANDs. At least one term must be satisfied. */
        for (int t = 0; t < f->n_terms; t++) {
            int term_true = 1;
            for (int l = 0; l < f->k_alloc; l++) {
                int lit = f->terms[t * f->k_alloc + l];
                if (is_literal_null(lit)) continue;

                int var  = LITERAL_VAR(lit);
                int sign = LITERAL_SIGN(lit);
                int val  = (var >= 0 && var < f->n_vars) ? assign[var] : 0;

                /* A term in DNF is satisfied only if ALL literals are true */
                if ((sign == LITERAL_POS && val == 0) ||
                    (sign == LITERAL_NEG && val == 1)) {
                    term_true = 0;
                    break;  /* this literal fails, term fails */
                }
            }
            if (term_true) return 1;  /* one satisfied term suffices */
        }
        return 0;  /* no term satisfied */
    }
}

/**
 * dnf_evaluate — Evaluate DNF on an assignment (type-safe wrapper).
 */
int dnf_evaluate(const DNF* d, const int* assign) {
    if (!d || d->is_cnf) return 0;
    return formula_evaluate((const Formula*)d, assign);
}

/**
 * cnf_evaluate — Evaluate CNF on an assignment (type-safe wrapper).
 */
int cnf_evaluate(const CNF* c, const int* assign) {
    if (!c || !c->is_cnf) return 0;
    return formula_evaluate((const Formula*)c, assign);
}

/* ================================================================
 * RANDOM FORMULA GENERATION
 * ================================================================ */

/**
 * dnf_random — Generate a random k-DNF formula.
 *
 * Each term is given exactly k distinct literals with random signs.
 * Variables are chosen uniformly without replacement within each termb
 * (no variable appears twice in the same term, though this is not
 * enforced for simplicity — actual generation picks randomly with
 * possible duplicates).
 *
 * @param n_vars   Number of Boolean variables
 * @param n_terms  Number of terms
 * @param k        Exact width of each term
 * @return         Newly allocated random k-DNF
 *
 * Complexity: O(n_terms * k)
 *
 * The generated DNF is not guaranteed to be satisfiable; it depends
 * on the random assignment of literals and signs.
 */
DNF* dnf_random(int n_vars, int n_terms, int k) {
    if (n_vars < 1 || n_terms < 1 || k < 1 || k > n_vars) return NULL;

    DNF* d = dnf_create(n_vars, n_terms, k);
    if (!d) return NULL;

    for (int t = 0; t < n_terms; t++) {
        /* Simple: pick k random variables (may repeat, but unlikely with large n_vars) */
        for (int l = 0; l < k; l++) {
            int var  = rand_int(n_vars);
            int sign = rand_sign();
            dnf_set_literal(d, t, l, var, sign);
        }
    }
    return d;
}

/**
 * cnf_random — Generate a random s-CNF formula.
 *
 * Each clause has exactly s distinct literals with random signs.
 */
CNF* cnf_random(int n_vars, int n_clauses, int s) {
    if (n_vars < 1 || n_clauses < 1 || s < 1 || s > n_vars) return NULL;

    CNF* c = cnf_create(n_vars, n_clauses, s);
    if (!c) return NULL;

    for (int cl = 0; cl < n_clauses; cl++) {
        for (int l = 0; l < s; l++) {
            int var  = rand_int(n_vars);
            int sign = rand_sign();
            cnf_set_literal(c, cl, l, var, sign);
        }
    }
    return c;
}

/* ================================================================
 * FORMULA WIDTH AND SIZE QUERIES
 * ================================================================ */

/**
 * formula_term_width — Count the number of actual (non-null) literals
 * in a given term (for DNF) or clause (for CNF).
 *
 * Complexity: O(k_alloc)
 */
int formula_term_width(const Formula* f, int term_idx) {
    if (!f || term_idx < 0 || term_idx >= f->n_terms) return 0;

    int count = 0;
    int base  = term_idx * f->k_alloc;
    for (int l = 0; l < f->k_alloc; l++) {
        if (!is_literal_null(f->terms[base + l])) count++;
    }
    return count;
}

/**
 * dnf_width — Maximum term width in the DNF.
 * This gives the "k" value for the switching lemma bound.
 *
 * Complexity: O(n_terms * k_alloc)
 */
int dnf_width(const DNF* d) {
    if (!d) return 0;
    int max_width = 0;
    for (int t = 0; t < d->n_terms; t++) {
        int w = formula_term_width((const Formula*)d, t);
        if (w > max_width) max_width = w;
    }
    return max_width;
}

/**
 * cnf_width — Maximum clause width in the CNF.
 */
int cnf_width(const CNF* c) {
    return dnf_width((const DNF*)c);  /* same computation */
}

/**
 * dnf_term_count — Count non-null terms in the DNF.
 * A "non-null term" has at least one literal.
 *
 * Complexity: O(n_terms * k_alloc)
 */
int dnf_term_count(const DNF* d) {
    if (!d) return 0;
    int count = 0;
    for (int t = 0; t < d->n_terms; t++) {
        if (formula_term_width((const Formula*)d, t) > 0) count++;
    }
    return count;
}

/* ================================================================
 * FORMULA CLONING
 * ================================================================ */

/**
 * dnf_clone — Create a deep copy of a DNF formula.
 *
 * Complexity: O(n_terms * k_alloc)
 */
DNF* dnf_clone(const DNF* d) {
    if (!d) return NULL;

    DNF* copy = dnf_create(d->n_vars, d->n_terms, d->k_alloc);
    if (!copy) return NULL;

    int total = d->n_terms * d->k_alloc;
    for (int i = 0; i < total; i++) {
        copy->terms[i] = d->terms[i];
    }
    return copy;
}

/**
 * cnf_clone — Create a deep copy of a CNF formula.
 */
CNF* cnf_clone(const CNF* c) {
    if (!c) return NULL;

    CNF* copy = cnf_create(c->n_vars, c->n_terms, c->k_alloc);
    if (!copy) return NULL;

    int total = c->n_terms * c->k_alloc;
    for (int i = 0; i < total; i++) {
        copy->terms[i] = c->terms[i];
    }
    return copy;
}

/* ================================================================
 * FORMULA DESTRUCTION
 * ================================================================ */

/**
 * formula_free — Free a generic formula (DNF or CNF).
 */
void formula_free(Formula* f) {
    if (!f) return;
    if (f->terms) {
        free(f->terms);
        f->terms = NULL;
    }
    free(f);
}

/**
 * dnf_free — Free a DNF formula.
 */
void dnf_free(DNF* d) {
    formula_free((Formula*)d);
}

/**
 * cnf_free — Free a CNF formula.
 */
void cnf_free(CNF* c) {
    formula_free((Formula*)c);
}

/* ================================================================
 * FORMULA DISPLAY
 * ================================================================ */

/**
 * dnf_print — Print a DNF formula in human-readable form.
 *
 * Format: (x0 ∧ ~x2) ∨ (x1 ∧ x3 ∧ ~x5) ∨ ...
 * Each term enclosed in parentheses, literals joined by ∧,
 * terms joined by ∨.
 *
 * Uses ? for null-tagged terms (should not happen in normal use).
 */
void dnf_print(const DNF* d) {
    if (!d) { printf("NULL\n"); return; }

    if (d->n_terms == 0) {
        printf("(EMPTY DNF)\n");
        return;
    }

    int printed_terms = 0;
    for (int t = 0; t < d->n_terms; t++) {
        int first_lit = 1;
        int has_lit = 0;

        /* Check if term has any literals */
        for (int l = 0; l < d->k_alloc; l++) {
            int lit = d->terms[t * d->k_alloc + l];
            if (!is_literal_null(lit)) {
                has_lit = 1;
                break;
            }
        }
        if (!has_lit) continue; /* skip empty terms */

        if (printed_terms > 0) printf(" V ");
        printf("(");

        for (int l = 0; l < d->k_alloc; l++) {
            int lit = d->terms[t * d->k_alloc + l];
            if (is_literal_null(lit)) continue;

            if (!first_lit) printf(" ^ ");
            int var  = LITERAL_VAR(lit);
            int sign = LITERAL_SIGN(lit);
            if (sign == LITERAL_NEG) printf("~");
            printf("x%d", var);
            first_lit = 0;
        }
        printf(")");
        printed_terms++;
    }
    if (printed_terms == 0) printf("(EMPTY DNF)");
    printf("\n");
}

/**
 * cnf_print — Print a CNF formula in human-readable form.
 *
 * Format: (x0 ∨ ~x2) ∧ (x1 ∨ x3 ∨ ~x5) ∧ ...
 */
void cnf_print(const CNF* c) {
    if (!c) { printf("NULL\n"); return; }

    if (c->n_terms == 0) {
        printf("(EMPTY CNF)\n");
        return;
    }

    int printed_clauses = 0;
    for (int cl = 0; cl < c->n_terms; cl++) {
        int first_lit = 1;
        int has_lit = 0;

        for (int l = 0; l < c->k_alloc; l++) {
            int lit = c->terms[cl * c->k_alloc + l];
            if (!is_literal_null(lit)) { has_lit = 1; break; }
        }
        if (!has_lit) continue;

        if (printed_clauses > 0) printf(" ^ ");
        printf("(");

        for (int l = 0; l < c->k_alloc; l++) {
            int lit = c->terms[cl * c->k_alloc + l];
            if (is_literal_null(lit)) continue;

            if (!first_lit) printf(" v ");
            int var  = LITERAL_VAR(lit);
            int sign = LITERAL_SIGN(lit);
            if (sign == LITERAL_NEG) printf("~");
            printf("x%d", var);
            first_lit = 0;
        }
        printf(")");
        printed_clauses++;
    }
    if (printed_clauses == 0) printf("(EMPTY CNF)");
    printf("\n");
}

/* ================================================================
 * FORMULA EQUIVALENCE CHECKING
 * ================================================================ */

/**
 * dnf_equivalent — Check if two DNF formulas compute the same Boolean function.
 *
 * Uses brute-force truth table comparison. Only feasible for small n_vars
 * (n <= 20, since 2^20 = ~1M assignments takes ~1 second).
 *
 * @param a  First DNF
 * @param b  Second DNF
 * @return   1 if a and b are logically equivalent, 0 otherwise
 *
 * Complexity: O(2^n_vars * (|a| + |b|))
 *
 * Note: DNF equivalence is coNP-complete in general.
 * This brute-force approach is exact but exponential.
 */
int dnf_equivalent(const DNF* a, const DNF* b) {
    if (!a || !b) return (a == b);
    if (a->n_vars != b->n_vars) return 0;

    int n = a->n_vars;
    if (n > 20) {
        fprintf(stderr, "dnf_equivalent: n_vars=%d too large for brute force\n", n);
        return 0;
    }

    int n_assignments = 1 << n;
    int* assign = (int*)calloc((size_t)n, sizeof(int));
    if (!assign) return 0;

    int eq = 1;
    for (int mask = 0; mask < n_assignments && eq; mask++) {
        /* Build assignment from bitmask */
        for (int v = 0; v < n; v++) {
            assign[v] = (mask >> v) & 1;
        }
        int va = dnf_evaluate(a, assign);
        int vb = dnf_evaluate(b, assign);
        if (va != vb) eq = 0;
    }
    free(assign);
    return eq;
}

/* ================================================================
 * DNF ↔ CNF CONVERSION (EXACT)
 * ================================================================ */

/**
 * dnf_to_cnf_exact — Convert a DNF to an equivalent CNF.
 *
 * Algorithm: Apply distributivity.
 *   DNF:  T1 ∨ T2 ∨ ... ∨ Tm   where Ti = li1 ∧ li2 ∧ ... ∧ lik_i
 *   CNF:  ∧_{(j1,...,jm) : 1 ≤ ji ≤ |Ti|} (l1j1 ∨ l2j2 ∨ ... ∨ lmjm)
 *
 * The number of clauses = product of term widths, which is exponential
 * in the worst case.
 *
 * @param d  DNF formula
 * @return   Equivalent CNF (may be huge), or NULL if too large
 *
 * Complexity: O(Π |Ti| * ...), worst case O(k^m) clauses
 *
 * WARNING: This is a theoretical conversion for small formulas only.
 * The switching lemma provides a *probabilistic* alternative:
 * with a random restriction, a k-DNF becomes an s-CNF with high prob.
 *
 * Implementation uses recursive enumeration of all clause combinations.
 * For formulas with n_terms * k > 20, we bail out with NULL.
 */
CNF* dnf_to_cnf_exact(const DNF* d) {
    if (!d) return NULL;

    int n = d->n_vars;
    int m = d->n_terms;

    if (m == 0) {
        /* Empty DNF = always false */
        CNF* result = cnf_create(n, 1, 1);
        if (result) {
            /* (x0 ∨ ~x0) as contradictory clause → always false */
            cnf_set_literal(result, 0, 0, 0, LITERAL_POS);
        }
        return result;
    }

    /* Compute term widths and total clauses */
    int* widths = (int*)calloc((size_t)m, sizeof(int));
    if (!widths) return NULL;

    long long total_clauses = 1;
    for (int t = 0; t < m; t++) {
        widths[t] = formula_term_width((const Formula*)d, t);
        if (widths[t] == 0) {
            /* Term with 0 literals = always true → entire DNF is always true */
            free(widths);
            CNF* result = cnf_create(n, 1, 1);
            if (result) {
                /* Empty clause = always true */
            }
            return result;
        }
        total_clauses *= widths[t];
        if (total_clauses > 1000000LL) {
            /* Too many clauses, bail out */
            free(widths);
            return NULL;
        }
    }

    if (total_clauses > 100000) {
        free(widths);
        return NULL;  /* too large */
    }

    int nc = (int)((total_clauses > 100000) ? 100000 : total_clauses);
    int max_clause_width = m;  /* one literal per original term */

    CNF* result = cnf_create(n, nc, max_clause_width);
    if (!result) { free(widths); return NULL; }

    /* Enumerate all clause combinations */
    int* indices = (int*)calloc((size_t)nc * (size_t)m, sizeof(int));
    if (!indices) { free(widths); cnf_free(result); return NULL; }

    /* Generate index tuples: indices[clause_idx * m + t] = selected literal pos in term t */
    int clause_idx = 0;
    int* idx_combo = (int*)calloc((size_t)m, sizeof(int));
    if (!idx_combo) { free(widths); free(indices); cnf_free(result); return NULL; }

    while (clause_idx < nc) {
        /* Record current combination */
        for (int t = 0; t < m; t++) {
            indices[clause_idx * m + t] = idx_combo[t];
        }

        /* Build clause from this combination */
        for (int t = 0; t < m; t++) {
            int base = t * d->k_alloc;
            int lit_pos = idx_combo[t];
            /* Find the lit_pos-th non-null literal in term t */
            int nonnull_count = 0;
            for (int l = 0; l < d->k_alloc; l++) {
                int lit = d->terms[base + l];
                if (!is_literal_null(lit)) {
                    if (nonnull_count == lit_pos) {
                        int var  = LITERAL_VAR(lit);
                        int sign = LITERAL_SIGN(lit);
                        cnf_set_literal(result, clause_idx, t, var, sign);
                        break;
                    }
                    nonnull_count++;
                }
            }
        }
        clause_idx++;

        /* Increment combination (mixed-radix counting) */
        int carry = 1;
        for (int t = m - 1; t >= 0 && carry; t--) {
            idx_combo[t]++;
            if (idx_combo[t] >= widths[t]) {
                idx_combo[t] = 0;
                carry = 1;
            } else {
                carry = 0;
            }
        }
        if (carry) break;  /* all combinations enumerated */
    }

    free(idx_combo);
    free(indices);
    free(widths);
    return result;
}

/**
 * cnf_to_dnf_exact — Convert a CNF to an equivalent DNF.
 *
 * Dual of dnf_to_cnf_exact. The number of terms = product of clause widths.
 *
 * Complexity: O(Π |Ci|), exponential in the worst case.
 */
DNF* cnf_to_dnf_exact(const CNF* c) {
    if (!c) return NULL;

    /* For small formulas: use the truth table method */
    int n = c->n_vars;
    int m = c->n_terms;

    if (n > 15 || m > 100) return NULL;  /* too large for brute force */

    /* Compute truth table */
    int n_assignments = 1 << n;
    int* assign = (int*)calloc((size_t)n, sizeof(int));
    if (!assign) return NULL;

    /* Count satisfying assignments */
    int sat_count = 0;
    for (int mask = 0; mask < n_assignments; mask++) {
        for (int v = 0; v < n; v++) assign[v] = (mask >> v) & 1;
        if (cnf_evaluate(c, assign)) sat_count++;
    }

    if (sat_count == 0) {
        /* Unsatisfiable: return empty DNF (always false) */
        free(assign);
        return dnf_create(n, 1, 1);  /* trivial DNF with no satisfiable terms */
    }

    if (sat_count > 10000) {
        free(assign);
        return NULL;  /* too many terms */
    }

    /* Create DNF with one term per satisfying assignment */
    DNF* result = dnf_create(n, sat_count, n);
    if (!result) { free(assign); return NULL; }

    int term_idx = 0;
    for (int mask = 0; mask < n_assignments && term_idx < sat_count; mask++) {
        for (int v = 0; v < n; v++) assign[v] = (mask >> v) & 1;
        if (cnf_evaluate(c, assign)) {
            for (int v = 0; v < n; v++) {
                dnf_set_literal(result, term_idx, v, v,
                    (assign[v] == 1) ? LITERAL_POS : LITERAL_NEG);
            }
            term_idx++;
        }
    }

    free(assign);
    return result;
}

/* ================================================================
 * FORMULA ANALYSIS
 * ================================================================ */

/**
 * dnf_is_s_cnf — Check if a DNF can be represented as an s-CNF.
 *
 * After applying a restriction, the resulting DNF may actually be
 * equivalent to an s-CNF. This checks whether the DNF's structure
 * already conforms to CNF requirements.
 *
 * For a DNF to be an s-CNF:
 *   - Each term must have at most s literals
 *   - AND: the DNF as a whole must be an AND of ORs
 *
 * But since switching transforms DNF → CNF, we simply check:
 *   is the DNF narrow enough to be considered an s-CNF?
 *
 * A simpler interpretation: the DNF has max width ≤ s.
 *
 * @param d  DNF formula
 * @param s  Target CNF width
 * @return   1 if dnf_width(d) ≤ s, 0 otherwise
 *
 * Complexity: O(n_terms * k_alloc)
 */
int dnf_is_s_cnf(const DNF* d, int s) {
    if (!d) return 0;
    return dnf_width(d) <= s ? 1 : 0;
}

/**
 * dnf_first_satisfying_term — Find the first term (lowest index) that
 * evaluates to true under the given assignment.
 *
 * @param d      DNF formula
 * @param assign Assignment array
 * @return       Index of first satisfying term (0-based), or -1 if none
 *
 * Complexity: O(n_terms * k_alloc)
 */
int dnf_first_satisfying_term(const DNF* d, const int* assign) {
    if (!d || !assign) return -1;

    for (int t = 0; t < d->n_terms; t++) {
        int term_true = 1;
        for (int l = 0; l < d->k_alloc; l++) {
            int lit = d->terms[t * d->k_alloc + l];
            if (is_literal_null(lit)) continue;

            int var  = LITERAL_VAR(lit);
            int sign = LITERAL_SIGN(lit);
            int val  = (var >= 0 && var < d->n_vars) ? assign[var] : 0;

            if ((sign == LITERAL_POS && val == 0) ||
                (sign == LITERAL_NEG && val == 1)) {
                term_true = 0;
                break;
            }
        }
        if (term_true) return t;
    }
    return -1;
}

/**
 * dnf_satisfying_count — Count the number of assignments that satisfy the DNF.
 *
 * Uses brute-force enumeration. Only feasible for small n_vars (n ≤ 20).
 *
 * @param d  DNF formula
 * @return   Number of satisfying assignments, or -1 if too large
 *
 * Complexity: O(2^n_vars * n_terms * k_alloc)
 * This is #P-complete in general (Valiant 1979).
 */
long long dnf_satisfying_count(const DNF* d) {
    if (!d || d->n_vars > 20) return -1;

    int n = d->n_vars;
    int n_assignments = 1 << n;
    long long count = 0;

    int* assign = (int*)calloc((size_t)n, sizeof(int));
    if (!assign) return -1;

    for (int mask = 0; mask < n_assignments; mask++) {
        for (int v = 0; v < n; v++) {
            assign[v] = (mask >> v) & 1;
        }
        if (dnf_evaluate(d, assign)) count++;
    }

    free(assign);
    return count;
}

/**
 * formula_set_seed — Set PRNG seed for reproducible random formulas.
 * (Declared locally in this file, called before any random generation.
 */
#undef  formula_set_seed
#define formula_set_seed formula_set_seed_wrapper

/* Redefine for public visibility via header (we use the internal one above) */
void formula_set_seed_wrapper(unsigned int seed) {
    formula_set_seed(seed);
}

/* ================================================================
 * CONVENIENCE: DNF from boolean function (truth table)
 * ================================================================ */

/**
 * dnf_from_truth_table — Build a DNF from a truth table.
 * Each true assignment becomes a term (minterm).
 *
 * @param n_vars  Number of variables
 * @param table   Truth table of length 2^n_vars, table[mask] = f(assignment)
 * @return        DNF with one term per satisfying assignment, or NULL
 *
 * Complexity: O(2^n_vars * n_vars)
 */
DNF* dnf_from_truth_table(int n_vars, const int* table) {
    if (!table || n_vars < 1 || n_vars > 20) return NULL;

    int n_assignments = 1 << n_vars;

    /* Count satisfying assignments */
    int sat_count = 0;
    for (int mask = 0; mask < n_assignments; mask++) {
        if (table[mask]) sat_count++;
    }

    if (sat_count == 0) {
        /* Always false → create trivial DNF */
        return dnf_create(n_vars, 1, 1);
    }

    DNF* result = dnf_create(n_vars, sat_count, n_vars);
    if (!result) return NULL;

    int term_idx = 0;
    for (int mask = 0; mask < n_assignments && term_idx < sat_count; mask++) {
        if (table[mask]) {
            for (int v = 0; v < n_vars; v++) {
                int val = (mask >> v) & 1;
                dnf_set_literal(result, term_idx, v, v,
                    val ? LITERAL_POS : LITERAL_NEG);
            }
            term_idx++;
        }
    }
    return result;
}
