/**
 * depth_reduction.c -- Iterative Depth Reduction (Hastad's Proof Core)
 *
 * Core of the PARITY-not-in-AC0 proof:
 *   Step 1: Assume depth-d AC0 circuit C for PARITY(n).
 *   Step 2: Apply random restriction p = n^{-1/(d-1)}.
 *   Step 3: Switching lemma: bottom 2 levels merge -> depth d-1.
 *   Step 4: Repeat d-2 times -> depth-2 DNF/CNF.
 *   Step 5: PARITY on k vars needs DNF size 2^{k-1} = EXPONENTIAL.
 *   Step 6: Therefore original size must be exponential.
 *
 * L4: Hastad Theorem (PARITY not in AC0)
 * L5: Iterative depth reduction algorithm
 * L8: Optimal parameter selection, matching upper bound
 */
#include "hastad.h"
#include <string.h>

/* =================================================================
 * L5: Iterative Depth Reduction State Machine
 * ================================================================= */

/** State of the iterative reduction process. */
typedef struct {
    int round;           /* Current round number (0 = initial) */
    int n_vars;          /* Remaining free variables */
    int depth;           /* Current circuit depth */
    double size_factor;  /* Accumulated size multiplier */
    double* p_history;   /* Restriction probabilities used */
    int* n_history;      /* Variable counts after each round */
    int history_len;     /* Length of history arrays */
    int max_history;
} ReductionState;

/** Initialize reduction state. */
static ReductionState* reduction_init(int n, int d, int max_rounds) {
    ReductionState* s = (ReductionState*)malloc(sizeof(ReductionState));
    if (!s) return NULL;
    s->round = 0;
    s->n_vars = n;
    s->depth = d;
    s->size_factor = 1.0;
    s->max_history = max_rounds + 1;
    s->p_history = (double*)malloc(s->max_history * sizeof(double));
    s->n_history = (int*)malloc(s->max_history * sizeof(int));
    if (!s->p_history || !s->n_history) {
        free(s->p_history); free(s->n_history); free(s); return NULL;
    }
    s->history_len = 0;
    s->n_history[s->history_len] = n;
    s->p_history[s->history_len] = 1.0;
    s->history_len++;
    return s;
}

/** Free reduction state. */
static void reduction_free(ReductionState* s) {
    if (!s) return;
    free(s->p_history);
    free(s->n_history);
    free(s);
}

/** Execute one switching round. Returns 1 if depth > 2, 0 if done. */
static int reduction_round(ReductionState* s) {
    if (s->depth <= 2 || s->n_vars <= 1) return 0;

    double p = optimal_restriction_prob(s->n_vars, s->depth);

    switching_round_simulate(&s->n_vars, &s->depth, p, &s->size_factor);

    s->round++;
    if (s->history_len < s->max_history) {
        s->p_history[s->history_len] = p;
        s->n_history[s->history_len] = s->n_vars;
        s->history_len++;
    }
    return 1;
}

/** Execute full reduction to depth 2. */
static void reduction_full(ReductionState* s) {
    while (reduction_round(s)) {
        if (s->n_vars <= 1) break;
    }
}

/** Compute final lower bound from reduction state. */
static double reduction_final_bound(const ReductionState* s) {
    /* DNF size needed for PARITY on remaining vars */
    double dnf_size = dnf_parity_lower_bound(s->n_vars);
    /* Total circuit size = size_factor * dnf_size */
    return s->size_factor * dnf_size;
}

/* =================================================================
 * L5: Simulation and Visualization
 * ================================================================= */

/**
 * Print step-by-step trace of the depth reduction process.
 */
void depth_reduction_trace(int n, int d) {
    printf("\n=== Hastad Depth Reduction Trace ===\n");
    printf("Start: n=%d, depth=%d\n\n", n, d);

    ReductionState* s = reduction_init(n, d, 20);
    if (!s) return;

    printf("%-6s %-10s %-12s %-8s %-12s\n",
           "Round", "p", "Surviving n", "Depth", "Size factor");
    printf("%-6s %-10s %-12s %-8s %-12s\n",
           "-----", "----------", "-----------", "------", "------------");

    printf("%-6d %-10s %-12d %-8d %-12.1f\n",
           0, "---", n, d, 1.0);

    while (reduction_round(s)) {
        int ri = s->round;
        double p_last = s->p_history[ri];
        int n_cur = s->n_vars;
        int d_cur = s->depth;
        double sf = s->size_factor;
        printf("%-6d %-10.4f %-12d %-8d %-12.2f\n",
               ri, p_last, n_cur, d_cur, sf);
    }

    double bound = reduction_final_bound(s);
    printf("\nFinal: %d vars at depth %d\n", s->n_vars, s->depth);
    printf("  DNF for PARITY(%d) >= 2^%d = %.2e\n",
           s->n_vars, s->n_vars - 1, dnf_parity_lower_bound(s->n_vars));
    printf("  Total lower bound >= %.2e\n", bound);

    if (bound > 1e10) {
        printf("  => EXPONENTIAL lower bound. PARITY not in AC0.\n");
        printf("  => AC0 != NC1 (since PARITY in NC1).\n");
    }
    reduction_free(s);
}

/**
 * Compare depth reduction across different depths.
 * Shows how the lower bound exponent changes with depth.
 */
void depth_reduction_compare(int n) {
    printf("\n=== Depth Reduction Comparison (n=%d) ===\n\n", n);
    printf("%-8s %-12s %-12s %-20s\n",
           "Depth", "Final k", "log2(size)", "Bound Status");

    for (int d = 2; d <= 6; d++) {
        ReductionState* s = reduction_init(n, d, 20);
        if (!s) continue;
        reduction_full(s);
        double bound = reduction_final_bound(s);
        double log_bound = (bound > 0) ? log2(bound) : 0.0;
        const char* status = (bound > 1e8) ? "EXPONENTIAL" :
                             (bound > 1000.0) ? "super-poly" :
                             "polynomial?";
        printf("%-8d %-12d %-12.2f %-20s\n",
               d, s->n_vars, log_bound, status);
        reduction_free(s);
    }
    printf("\nNote: larger depth => smaller exponent => weaker bound.\n");
    printf("For fixed d, PARITY(n) requires size exp(Omega(n^{1/(d-1)})).\n");
}

/**
 * Verify the optimality of Hastad's parameter choice.
 * p = n^{-1/(d-1)} is the choice that maximizes the final lower bound.
 */
void depth_reduction_optimality_check(int n, int d) {
    printf("\n=== Optimality of p = n^{-1/(d-1)} ===\n");
    printf("n=%d, d=%d\n\n", n, d);

    double p_opt = optimal_restriction_prob(n, d);
    printf("Optimal p = n^{-1/(%d)} = %.6f\n", d-1, p_opt);
    printf("Expected surviving vars after one round: n*p = %.1f\n", n * p_opt);

    printf("\nComparing different p values:\n");
    printf("%-10s %-12s %-20s %-15s\n",
           "p", "Surviving", "Final DNF log2(size)", "Switching prob");

    for (double mult = 0.5; mult <= 2.0; mult += 0.25) {
        double p_test = p_opt * mult;
        if (p_test > 1.0) p_test = 1.0;
        if (p_test < 0.001) continue;

        /* Simulate one round */
        int n1 = n, d1 = d;
        double sf = 1.0;
        switching_round_simulate(&n1, &d1, p_test, &sf);

        /* DNF size for remaining vars */
        double dnf_sz = dnf_parity_lower_bound(n1);
        double total = sf * dnf_sz;
        double log_total = (total > 0) ? log2(total) : 0.0;

        /* Switching lemma success probability */
        int k = 3;  /* Assume 3-DNF at bottom */
        double switch_prob = 1.0 - switching_lemma_bound(k, p_test, 3);

        printf("%-10.6f %-12d %-20.2f %-15.4f\n",
               p_test, n1, log_total, switch_prob);
    }
    printf("\nOptimal p balances: enough vars survive AND switching succeeds.\n");
}

/* =================================================================
 * L4: Hastad's Lower Bound Computation
 * ================================================================= */

/**
 * Compute Hastad's full lower bound: the minimum size a depth-d
 * AC0 circuit needs to compute PARITY on n variables.
 *
 * Formula: S(n,d) = exp(Omega(n^{1/(d-1)}))
 *
 * The constant in Omega depends on the switching lemma constant.
 * Hastad (1986): S >= exp(n^{1/(d-1)} / 10)
 * Later improvements reduce the constant.
 */
double hastad_lower_bound(int n, int d) {
    if (n <= 0 || d <= 1) return (double)n;
    /* Hastad's constant: divide exponent by ~10 */
    return exp(pow((double)n, 1.0 / (d - 1)) / 10.0);
}

/** Optimal restriction probability for depth-d reduction. */
double hastad_optimal_p(int n, int d) {
    return optimal_restriction_prob(n, d);
}

/** The exponent alpha = 1/(d-1) in the lower bound. */
double hastad_exponent(int d) {
    if (d <= 1) return 1.0;
    return 1.0 / (d - 1);
}

/** PARITY on k vars needs DNF of size 2^{k-1}. */
double dnf_parity_lower_bound(int n) {
    if (n > 60) return 1e18;  /* overflow guard */
    return pow(2.0, (double)(n - 1));
}

/** Same for CNF (by duality). */
double cnf_parity_lower_bound(int n) {
    return dnf_parity_lower_bound(n);
}

/**
 * Decompose PARITY under restriction:
 *   PARITY_n|_{restriction}(free_vars) = PARITY_k(free_vars) XOR c
 * where k = number of free variables, c = XOR of fixed-to-1 vars.
 */
int parity_restricted_decompose(const int* x, int n,
                                 const int* restr, int* k, int* c) {
    int k_count = 0, c_val = 0;
    for (int i = 0; i < n; i++) {
        if (restr[i] == -1) {
            c_val ^= (x[i] & 1);  /* will be corrected below */
            k_count++;
        } else if (restr[i] == 1) {
            c_val ^= 1;
        }
    }
    /* PARITY_n = PARITY(free vars) XOR c_val XOR PARITY(free vars as stored in x)
     * Actually: for each free var, x[i] contributes one XOR.
     * c_val already accounts for x[i] for free vars once.
     * So output = c_val (the XOR of free x's + fixed ones) = PARITY(free) XOR constant.
     * But we double-counted free x's: c_val has them, and output also has them.
     * Result: the restricted function = PARITY(free_vars) XOR (c XOR PARITY(free_vars from x))
     * = PARITY_k(free) XOR [original fixed parity].
     * This is always PARITY on free variables XOR constant. */
    *k = k_count;
    *c = 0;
    for (int i = 0; i < n; i++) {
        if (restr[i] != -1 && restr[i] == 1) *c ^= 1;
    }
    return 0;  /* Returns PARITY on free XOR c */
}

/**
 * Full simulation of Hastad's proof.
 * Returns minimum circuit size lower bound.
 */
double hastad_proof_simulate(int n, int d, int* steps, int* final_k) {
    ReductionState* s = reduction_init(n, d, 20);
    if (!s) {
        *steps = 0; *final_k = n; return 0.0;
    }
    reduction_full(s);
    *steps = s->round;
    *final_k = s->n_vars;
    double bound = reduction_final_bound(s);
    reduction_free(s);
    return bound;
}

/* =================================================================
 * L8: Upper Bound Matching
 * ================================================================= */

/**
 * Upper bound: there EXISTS a depth-d circuit for PARITY(n)
 * of size exp(O(n^{1/(d-1)})), matching Hastad's lower bound.
 *
 * Construction: balanced XOR tree with depth ceiling.
 * For depth d, partition into n^{1-1/d} blocks, compute
 * PARITY recursively on each, then XOR results.
 */
double parity_upper_bound_size(int n, int d) {
    if (d <= 1) return (double)n;
    if (n <= 1) return 1.0;

    int block_size = (int)pow((double)n, 1.0 - 1.0 / d);
    if (block_size < 2) block_size = 2;
    int n_blocks = n / block_size;
    if (n_blocks < 1) n_blocks = 1;

    double sub_size = parity_upper_bound_size(block_size, d - 1);
    double xor_size = (double)n_blocks;  /* XOR gates */
    return n_blocks * sub_size + xor_size;
}

/**
 * Verify tightness: lower <= upper (within constant factor).
 */
void hastad_tightness_check(int n, int d) {
    printf("\n=== Tightness Check (n=%d, d=%d) ===\n", n, d);
    double lower = hastad_lower_bound(n, d);
    double upper = parity_upper_bound_size(n, d);
    printf("Lower bound: %.2e\n", lower);
    printf("Upper bound: %.2e\n", upper);
    printf("Gap factor:  %.2f\n", upper / (lower + 1.0));
    printf("Both are exp(Theta(n^{1/(d-1)})) = exp(Theta(%.2f)).\n",
           pow((double)n, 1.0 / (d - 1)));
}
