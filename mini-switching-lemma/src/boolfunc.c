/**
 * src/boolfunc.c — Boolean Function Representations
 * =================================================
 *
 * Implements L1-L6: Core Boolean functions (PARITY, MAJORITY, THRESHOLD,
 * MOD) and utilities for truth table manipulation. These functions are
 * the canonical examples in circuit complexity theory.
 *
 * THEORETICAL BACKGROUND:
 *
 *   PARITY(x_0,...,x_{n-1}) = x_0 ⊕ x_1 ⊕ ... ⊕ x_{n-1}
 *     - PARITY ∉ AC0 (Furst-Saxe-Sipser 1981, Ajtai 1983, Hastad 1986)
 *     - Any depth-d AC0 circuit for PARITY needs size exp(Ω(n^{1/(d-1)}))
 *     - PARITY has full decision tree depth n (must query all variables)
 *     - DNF for PARITY: 2^{n-1} terms, each of width n
 *
 *   MAJORITY(x) = 1 iff Σ x_i > n/2
 *     - MAJORITY ∉ AC0 (Hastad, via switching lemma)
 *     - But MAJORITY ∈ TC0 (threshold circuits)
 *     - Related to MOD_p for prime p (Razborov-Smolensky)
 *
 *   THRESHOLD_k(x) = 1 iff Σ x_i ≥ k
 *     - Generalization of MAJORITY (k = ⌈n/2⌉)
 *     - THRESHOLD_k ∈ TC0 for all k
 *
 *   MOD_m(x) = 1 iff (Σ x_i) ≡ 0 (mod m)
 *     - MOD_2 = PARITY
 *     - MOD_p ∉ AC0 for odd prime p (Razborov 1987, Smolensky 1987)
 *     - AC0 can compute MOD_6 easily (Chinese remainder theorem)
 *
 * COURSE MAPPING:
 *   - MIT 6.841: Boolean functions as complexity classes
 *   - Stanford CS358: PARITY as canonical hard function for AC0
 *   - Berkeley CS278: AC0[p] and modular gates
 *   - CMU 15-855: Circuit lower bounds for explicit functions
 *   - ETH 263-4650: Boolean function analysis
 *
 * REFERENCES:
 *   - O'Donnell (2014), "Analysis of Boolean Functions"
 *   - Jukna (2012), "Boolean Function Complexity"
 *   - Furst, Saxe, Sipser (1981), "Parity, circuits, and the polynomial hierarchy"
 */

#include "switching.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/* ================================================================
 * INTERNAL: Truth table helpers
 * ================================================================ */

/* Allocate a BoolFunc with given parameters */
static BoolFunc* bf_alloc(int n_vars, const char* name) {
    if (n_vars < 1 || n_vars > 24) return NULL; /* practical limit: 16M entries */

    BoolFunc* bf = (BoolFunc*)calloc(1, sizeof(BoolFunc));
    if (!bf) return NULL;

    bf->n_vars = n_vars;
    bf->table_size = 1 << n_vars;
    bf->table = (int*)calloc((size_t)bf->table_size, sizeof(int));
    if (!bf->table) {
        free(bf);
        return NULL;
    }
    if (name) {
        bf->name = (char*)malloc(strlen(name) + 1);
        if (bf->name) strcpy(bf->name, name);
    }
    return bf;
}

/* Build truth table from a function (ptr to function computing one bit) */
typedef int (*bit_compute_fn)(const int* assign, int n_vars, int param);

static void bf_fill_from_fn(BoolFunc* bf, bit_compute_fn fn, int param) {
    if (!bf || !fn) return;
    int* assign = (int*)calloc((size_t)bf->n_vars, sizeof(int));
    if (!assign) return;
    for (int mask = 0; mask < bf->table_size; mask++) {
        for (int v = 0; v < bf->n_vars; v++) {
            assign[v] = (mask >> v) & 1;
        }
        bf->table[mask] = fn(assign, bf->n_vars, param);
    }
    free(assign);
}

/* ================================================================
 * L6: CANONICAL BOOLEAN FUNCTIONS
 * ================================================================ */

/**
 * bf_parity — Create truth table for PARITY on n variables.
 *
 * PARITY(x) = x_0 ⊕ x_1 ⊕ ... ⊕ x_{n-1}
 *           = (Σ x_i) mod 2
 *
 * Complexity: O(2^n)
 */
BoolFunc* bf_parity(int n) {
    BoolFunc* bf = bf_alloc(n, "PARITY");
    if (!bf) return NULL;

    for (int mask = 0; mask < bf->table_size; mask++) {
        int parity = 0;
        for (int i = 0; i < n; i++) {
            parity ^= ((mask >> i) & 1);
        }
        bf->table[mask] = parity;
    }
    return bf;
}

/**
 * bf_majority — Create truth table for MAJORITY on n variables.
 *
 * MAJORITY(x) = 1 iff Σ x_i > n/2
 *
 * For even n, ties are decided as 0 (strict majority).
 *
 * Complexity: O(2^n * n)
 */
BoolFunc* bf_majority(int n) {
    BoolFunc* bf = bf_alloc(n, "MAJORITY");
    if (!bf) return NULL;

    for (int mask = 0; mask < bf->table_size; mask++) {
        int sum = 0;
        for (int i = 0; i < n; i++) {
            sum += ((mask >> i) & 1);
        }
        bf->table[mask] = (sum > n / 2) ? 1 : 0;
    }
    return bf;
}

/**
 * bf_threshold — Create truth table for THRESHOLD_k on n variables.
 *
 * THRESHOLD_k(x) = 1 iff Σ x_i ≥ k
 *
 * @param k  Threshold value (0 ≤ k ≤ n+1)
 *           k = 0: always true
 *           k = n+1: always false
 *
 * Complexity: O(2^n * n)
 */
BoolFunc* bf_threshold(int n, int k) {
    char name[64];
    snprintf(name, sizeof(name), "THRESHOLD_%d", k);
    BoolFunc* bf = bf_alloc(n, name);
    if (!bf) return NULL;

    for (int mask = 0; mask < bf->table_size; mask++) {
        int sum = 0;
        for (int i = 0; i < n; i++) {
            sum += ((mask >> i) & 1);
        }
        bf->table[mask] = (sum >= k) ? 1 : 0;
    }
    return bf;
}

/**
 * bf_mod — Create truth table for MOD_m on n variables.
 *
 * MOD_m(x) = 1 iff (Σ x_i) mod m == 0
 *
 * MOD_2 = PARITY (the hardest function for AC0)
 * MOD_p for odd prime p is also hard for AC0 (Razborov-Smolensky)
 * MOD_6 ∈ AC0 (since MOD_6 = MOD_2 ∧ MOD_3 on sums)
 *
 * Value: for m ≤ 0, returns NULL.
 *
 * Complexity: O(2^n * n)
 */
BoolFunc* bf_mod(int n, int m) {
    if (m <= 0) return NULL;
    char name[64];
    snprintf(name, sizeof(name), "MOD_%d", m);
    BoolFunc* bf = bf_alloc(n, name);
    if (!bf) return NULL;

    for (int mask = 0; mask < bf->table_size; mask++) {
        int sum = 0;
        for (int i = 0; i < n; i++) {
            sum += ((mask >> i) & 1);
        }
        bf->table[mask] = ((sum % m) == 0) ? 1 : 0;
    }
    return bf;
}

/**
 * bf_and — Create truth table for AND on n variables.
 *
 * AND(x) = 1 iff all x_i = 1
 *
 * AND ∈ AC0 (trivially: one AND gate with fan-in n)
 * DNF: one term with n literals (x_0 ∧ x_1 ∧ ... ∧ x_{n-1})
 * CNF: n clauses, each with one literal
 *
 * Complexity: O(2^n)
 */
BoolFunc* bf_and(int n) {
    BoolFunc* bf = bf_alloc(n, "AND");
    if (!bf) return NULL;

    int all_ones_mask = (1 << n) - 1;
    for (int mask = 0; mask < bf->table_size; mask++) {
        bf->table[mask] = (mask == all_ones_mask) ? 1 : 0;
    }
    return bf;
}

/**
 * bf_or — Create truth table for OR on n variables.
 *
 * OR(x) = 1 iff at least one x_i = 1
 *
 * OR ∈ AC0 (one OR gate with fan-in n)
 * DNF: n terms, each with one literal (x_0 ∨ x_1 ∨ ... ∨ x_{n-1})
 * CNF: one clause with n literals
 *
 * Complexity: O(2^n)
 */
BoolFunc* bf_or(int n) {
    BoolFunc* bf = bf_alloc(n, "OR");
    if (!bf) return NULL;

    for (int mask = 0; mask < bf->table_size; mask++) {
        bf->table[mask] = (mask != 0) ? 1 : 0;
    }
    return bf;
}

/* ================================================================
 * BOOLEAN FUNCTION OPERATIONS
 * ================================================================ */

/**
 * bf_eval — Evaluate a Boolean function on an assignment.
 *
 * @param bf     Boolean function
 * @param assign Assignment array, assign[v] ∈ {0, 1}
 * @return       f(assign)
 *
 * Complexity: O(1) (table lookup)
 */
int bf_eval(const BoolFunc* bf, const int* assign) {
    if (!bf || !assign) return 0;

    /* Build index: bit v = assign[v] */
    int idx = 0;
    for (int v = 0; v < bf->n_vars; v++) {
        if (assign[v]) idx |= (1 << v);
    }
    if (idx < 0 || idx >= bf->table_size) return 0;
    return bf->table[idx];
}

/**
 * bf_satisfying_count — Count satisfying assignments.
 *
 * Complexity: O(2^n)
 */
long long bf_satisfying_count(const BoolFunc* bf) {
    if (!bf) return 0;
    long long count = 0;
    for (int i = 0; i < bf->table_size; i++) {
        if (bf->table[i]) count++;
    }
    return count;
}

/**
 * bf_weight — Compute the weight (number of 1s in truth table).
 * Same as satisfying count.
 *
 * Complexity: O(2^n)
 */
long long bf_weight(const BoolFunc* bf) {
    return bf_satisfying_count(bf);
}

/**
 * bf_equal — Check if two Boolean functions are equal.
 *
 * Complexity: O(min(2^n_a, 2^n_b))
 */
int bf_equal(const BoolFunc* a, const BoolFunc* b) {
    if (!a || !b) return (a == b);
    if (a->n_vars != b->n_vars) return 0;
    if (a->table_size != b->table_size) return 0;

    for (int i = 0; i < a->table_size; i++) {
        if (a->table[i] != b->table[i]) return 0;
    }
    return 1;
}

/* ================================================================
 * FORMULA CONVERSION FROM TRUTH TABLE
 * ================================================================ */

/**
 * bf_to_dnf — Compute DNF for a Boolean function from its truth table.
 *
 * Each satisfying assignment becomes a minterm (AND of all variables
 * in their respective polarities).
 *
 * Complexity: O(2^n * n)
 * Returns NULL if there are too many satisfying assignments.
 */
DNF* bf_to_dnf(const BoolFunc* bf) {
    if (!bf) return NULL;

    int n = bf->n_vars;
    long long sat_count = bf_satisfying_count(bf);

    if (sat_count == 0) {
        return dnf_create(n, 1, 1);  /* always false */
    }
    if (sat_count > 10000) {
        return NULL;  /* too many terms */
    }

    DNF* result = dnf_create(n, (int)sat_count, n);
    if (!result) return NULL;

    int term_idx = 0;
    for (int mask = 0; mask < bf->table_size && term_idx < (int)sat_count; mask++) {
        if (bf->table[mask]) {
            for (int v = 0; v < n; v++) {
                int val = (mask >> v) & 1;
                dnf_set_literal(result, term_idx, v, v,
                    val ? LITERAL_POS : LITERAL_NEG);
            }
            term_idx++;
        }
    }
    return result;
}

/**
 * bf_to_cnf — Compute CNF for a Boolean function from its truth table.
 *
 * Each falsifying assignment becomes a maxterm clause (OR of all
 * variables with polarities flipped).
 *
 * Complexity: O(2^n * n)
 */
CNF* bf_to_cnf(const BoolFunc* bf) {
    if (!bf) return NULL;

    int n = bf->n_vars;
    long long unsat_count = (1LL << n) - bf_satisfying_count(bf);

    if (unsat_count == 0) {
        /* Tautology: always true */
        CNF* result = cnf_create(n, 1, 1);
        return result;
    }
    if (unsat_count > 10000) {
        return NULL;
    }

    CNF* result = cnf_create(n, (int)unsat_count, n);
    if (!result) return NULL;

    int clause_idx = 0;
    for (int mask = 0; mask < bf->table_size && clause_idx < (int)unsat_count; mask++) {
        if (!bf->table[mask]) {
            for (int v = 0; v < n; v++) {
                int val = (mask >> v) & 1;
                /* For CNF, flip the literal: if val=1, use negated; if val=0, use positive */
                cnf_set_literal(result, clause_idx, v, v,
                    val ? LITERAL_NEG : LITERAL_POS);
            }
            clause_idx++;
        }
    }
    return result;
}

/* ================================================================
 * DISPLAY AND DESTRUCTION
 * ================================================================ */

/**
 * bf_print — Print truth table of a Boolean function.
 *
 * For n ≤ 5, prints the full table.
 * For larger n, prints summary statistics.
 */
void bf_print(const BoolFunc* bf) {
    if (!bf) { printf("BoolFunc(NULL)\n"); return; }

    printf("Boolean Function: %s\n", bf->name ? bf->name : "(unnamed)");
    printf("  n_vars = %d, table_size = %d\n", bf->n_vars, bf->table_size);
    long long sat = bf_satisfying_count(bf);
    printf("  satisfying = %lld / %d (%.2f%%)\n",
           sat, bf->table_size,
           100.0 * (double)sat / (double)bf->table_size);

    if (bf->n_vars <= 5) {
        printf("  Truth table:\n");
        for (int mask = 0; mask < bf->table_size; mask++) {
            printf("    ");
            for (int v = bf->n_vars - 1; v >= 0; v--) {
                printf("%d", (mask >> v) & 1);
            }
            printf(" -> %d\n", bf->table[mask]);
        }
    } else {
        printf("  (table too large to print, n_vars=%d)\n", bf->n_vars);
    }
}

/**
 * bf_free — Free a Boolean function.
 */
void bf_free(BoolFunc* bf) {
    if (!bf) return;
    if (bf->table) free(bf->table);
    if (bf->name)  free(bf->name);
    free(bf);
}

/* ================================================================
 * L8: FOURIER ANALYSIS (for switching lemma connection)
 * ================================================================ */

/**
 * bf_fourier_coefficient — Compute the Fourier coefficient at subset S.
 *
 * For a Boolean function f: {0,1}^n → {-1,1} (standard Fourier basis),
 * the Fourier coefficient at S ⊆ [n] is:
 *   f̂(S) = E_x[f(x) · χ_S(x)]
 * where χ_S(x) = (-1)^{Σ_{i∈S} x_i} is the parity character.
 *
 * For f: {0,1}^n → {0,1}, we convert: f'(x) = (-1)^{f(x)}.
 *
 * The switching lemma + LMN theorem implies:
 *   Σ_{|S| > L} f̂(S)² ≤ S · 2^{-Ω(L^{1/d})}
 * for AC0 circuits of size S and depth d.
 *
 * @param bf    Boolean function
 * @param S     Bitmask representing subset S
 * @return      Fourier coefficient f̂(S)
 *
 * Complexity: O(2^n)
 */
double bf_fourier_coefficient(const BoolFunc* bf, int S) {
    if (!bf) return 0.0;

    int n = bf->n_vars;
    double sum = 0.0;

    for (int x = 0; x < bf->table_size; x++) {
        /* f'(x) = (-1)^{f(x)}: maps 0→1, 1→-1 */
        double f_val = bf->table[x] ? -1.0 : 1.0;

        /* χ_S(x) = (-1)^{Σ_{i∈S} x_i} = (-1)^{popcount(x ∧ S)} */
        int dot = 0;
        int temp = x & S;
        while (temp) {
            dot ^= (temp & 1);
            temp >>= 1;
        }
        double chi = dot ? -1.0 : 1.0;

        sum += f_val * chi;
    }

    return sum / (double)bf->table_size;
}

/**
 * bf_fourier_level_weight — Compute total Fourier weight at level l.
 *
 * W_l(f) = Σ_{|S|=l} f̂(S)²
 *
 * This is used to verify the LMN bound: for AC0 functions,
 * high-level Fourier weight drops exponentially.
 *
 * Complexity: O(2^n * C(n,l)) for exhaustive enumeration,
 *   or O(2^n) for all levels simultaneously.
 *
 * For n ≤ 16, exhaustive. For larger n, returns -1.
 */
double bf_fourier_level_weight(const BoolFunc* bf, int level) {
    if (!bf || level < 0 || level > bf->n_vars) return 0.0;
    if (bf->n_vars > 16) return -1.0;

    double total = 0.0;
    for (int S = 0; S < bf->table_size; S++) {
        /* Count bits in S */
        int bits = 0;
        int temp = S;
        while (temp) { bits += (temp & 1); temp >>= 1; }
        if (bits == level) {
            double coeff = bf_fourier_coefficient(bf, S);
            total += coeff * coeff;
        }
    }
    return total;
}

/**
 * bf_fourier_spectrum — Print the Fourier spectrum by level.
 *
 * Shows how weight is distributed across levels.
 * For PARITY: all weight at level n (f̂({1,...,n}) = 1).
 * For AND/OR: weight concentrated at low levels.
 * For AC0 functions: weight decays rapidly after level ≈ log S.
 */
void bf_fourier_spectrum(const BoolFunc* bf) {
    if (!bf || bf->n_vars > 16) {
        printf("Fourier spectrum: n_vars too large for exhaustive computation.\n");
        return;
    }
    printf("Fourier Spectrum for %s:\n", bf->name ? bf->name : "f");
    printf("  Level  Weight\n");
    printf("  -----  ------\n");
    for (int l = 0; l <= bf->n_vars; l++) {
        double w = bf_fourier_level_weight(bf, l);
        if (w > 1e-12) {
            printf("  %5d  %.6f\n", l, w);
        }
    }
}
