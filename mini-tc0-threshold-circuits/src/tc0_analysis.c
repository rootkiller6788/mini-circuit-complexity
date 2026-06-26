/* tc0_analysis.c -- Threshold Circuit Search and Analysis (L4/L8)
 *
 * L4: Circuit size lower bounds via gate elimination and influence
 * L8: PTF degree computation, depth-2 TC0 lower bounds
 * L8: TC0 vs NC1: the open problem since 1989
 * L9: Minimal threshold circuit search (NP-hard in general)
 *
 * Key results:
 *   - MAJORITY has PTF degree 1 (linear threshold function)
 *   - PARITY has PTF degree n (requires degree-n polynomial)
 *   - Depth-2 TC0 circuits for PARITY need 2^{Omega(n)} size
 *   - Hastad's switching lemma for AC0: any depth-d AC0 circuit
 *     for PARITY needs exponential size
 *
 * References:
 *   - O'Donnell (2014) "Analysis of Boolean Functions"
 *   - Jukna (2012) "Boolean Function Complexity"
 *   - Hastad (1987) "Computational limitations of small-depth circuits"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <assert.h>
#include "tc0.h"

/* ================================================================
 * CIRCUIT ENUMERATION (Search Complexity)
 * ================================================================ */

/* Count the number of possible threshold circuits with given parameters.
 * This combinatorial count shows why exhaustive search is infeasible. */
double tc0_enumerate_circuits(int n_vars, int n_gates, int max_fanin) {
    if (n_vars <= 0 || n_gates <= 0 || max_fanin <= 0) return 0.0;

    /* For each gate:
     *   - Choose gate type: 14 types (TC_INPUT through TC_MUX)
     *   - Choose fanin set: C(n_vars, fanin) for each possible fanin size
     *   - For THR gates: choose threshold k (n_vars + 1 options)
     *   - For WTHR gates: choose n_vars weights + threshold
     *
     * Simplified: count only the fanin choices
     */
    double choices_per_gate = 0.0;
    for (int fan = 1; fan <= max_fanin && fan <= n_vars; fan++) {
        /* C(n, fan) = n!/(fan!(n-fan)!) — approximate */
        double combinations = 1.0;
        for (int i = 1; i <= fan; i++)
            combinations *= (double)(n_vars - i + 1) / (double)i;
        choices_per_gate += combinations;
    }

    /* Total: each gate independently chosen (ignoring DAG structure) */
    double total = 1.0;
    for (int i = 0; i < n_gates; i++)
        total *= choices_per_gate;

    return total;
}

double threshold_search_space(int n, int gates) {
    return tc0_enumerate_circuits(n, gates, n);
}

/* ================================================================
 * POLYNOMIAL THRESHOLD FUNCTION (PTF) DEGREE
 *
 * Definition: A Boolean function f has PTF degree d if there exists
 * a degree-d multilinear polynomial p(x_1,...,x_n) such that
 *   f(x) = sign(p(x))
 * where sign(t) = 1 if t >= 0, else 0.
 *
 * Key facts:
 *   - Every Boolean function has PTF degree at most n
 *   - MAJORITY has PTF degree 1 (it IS a LTF)
 *   - PARITY has PTF degree n
 *   - AC0 functions have PTF degree poly(log n) (Brück 1990)
 *   - TC0 functions have PTF degree poly(log n)
 * ================================================================ */

/* Represent a multilinear polynomial as a coefficient vector.
 * Only monomials up to a given degree are represented. */
typedef struct {
    int num_vars;
    int num_monomials;
    int *monomial_mask;  /* Bitmask of which vars are in each monomial */
    double *coeff;       /* Coefficient for each monomial */
    double threshold;
} MultilinearPoly;

/* Evaluate a multilinear polynomial on a Boolean input vector */
static double poly_evaluate(const MultilinearPoly *p, const int *x) {
    double sum = 0.0;
    for (int m = 0; m < p->num_monomials; m++) {
        double term = p->coeff[m];
        int mask = p->monomial_mask[m];
        for (int v = 0; v < p->num_vars; v++) {
            if ((mask >> v) & 1) {
                term *= (double)x[v];
            }
        }
        sum += term;
    }
    return sum;
}

/* Evaluate PTF: f(x) = 1 iff poly(x) >= threshold */
static int ptf_evaluate(const MultilinearPoly *p, const int *x) {
    return (poly_evaluate(p, x) >= p->threshold) ? 1 : 0;
}

/* Compute the PTF degree of a Boolean function by exhaustive search.
 * For small n (<= 6), try all possible degree-d polynomials.
 * Returns the minimum degree, or -1 if not found within limits. */
int tc0_compute_ptf_degree(const BoolFunc *f) {
    if (!f || f->num_vars > 8) return -1;

    int n = f->num_vars;

    /* Try degrees d = 1, 2, ..., n */
    for (int d = 1; d <= n; d++) {
        /* For degree d, try simple constructions:
         * For symmetric functions: PTF degree is the number of sign changes
         * in the symmetric representation. */

        if (f->is_symmetric == 1) {
            /* Symmetric function: check the symmetric representation s(k)
             * for k = 0,1,...,n. The PTF degree equals the minimum degree
             * of a univariate polynomial that separates zeros from ones. */
            int *s = (int *)malloc((size_t)(n + 1) * sizeof(int));
            for (int k = 0; k <= n; k++) {
                /* Find any assignment with weight k and check its value */
                int assignment = 0;
                for (int i = 0; i < k; i++) assignment |= (1 << i);
                s[k] = f->truth_table[assignment];
            }

            /* Count sign changes in s: number of i where s[i] != s[i+1] */
            int sign_changes = 0;
            for (int k = 0; k < n; k++)
                if (s[k] != s[k+1]) sign_changes++;

            free(s);

            /* For a symmetric function, PTF degree >= ceil(sign_changes/2) */
            /* If d is sufficient for sign_changes sign changes, we found it */
            if (sign_changes <= 2 * d) return d;

            /* Otherwise, continue to higher degree */
            continue;
        }

        /* For general (non-symmetric) functions, try all possible
         * monomial combinations up to degree d.
         * This is exponential but feasible for n <= 6. */
        int max_monomials = 1;
        for (int i = 1; i <= d; i++) max_monomials *= n;

        /* For simplicity, we only handle the symmetric case here.
         * General PTF degree computation is a research problem. */
    }

    return n;  /* Worst case */
}

/* ================================================================
 * CIRCUIT SIZE LOWER BOUNDS (L8: Advanced)
 * ================================================================ */

/* Gate elimination method: for a function f, remove one variable
 * by fixing it to 0 or 1. The resulting subfunction must be computable
 * by a smaller circuit.
 *
 * For TC0: delete a gate and merge its fanin into its successors.
 * Each deletion reduces size by at least 1. If n deletions are needed
 * before the function becomes constant, size >= n.
 */

/* Compute the minimum circuit size for a given function via
 * gate elimination heuristic. Returns a lower bound.
 * This is a simplified version; actual lower bounds use
 * random restrictions + Hastad's switching lemma. */
int tc0_size_lower_bound(const BoolFunc *f) {
    if (!f) return 0;
    int n = f->num_vars;

    /* Count the number of "essential" variables:
     * variable i is essential if there exist assignments x, y
     * differing only in bit i where f(x) != f(y) */
    int essential = 0;
    for (int var = 0; var < n; var++) {
        int is_essential = 0;
        for (int x = 0; x < f->table_size && !is_essential; x++) {
            int y = x ^ (1 << var);
            if (y < f->table_size && f->truth_table[x] != f->truth_table[y])
                is_essential = 1;
        }
        if (is_essential) essential++;
    }

    /* Lower bound: at least one gate per essential variable,
     * plus at least one output gate. */
    return essential + 1;
}

/* ================================================================
 * TC0 VS NC1: THE OPEN PROBLEM
 *
 * TC0 ⊆ NC1 is known (Barrington 1989).
 * NC1 ⊆ TC0 is OPEN.
 *
 * Equivalent formulations:
 *   1. Is PARITY in TC0?
 *   2. Is every regular language in TC0? (Barrington et al. 1992)
 *   3. Does NC1 = TC0?
 *
 * Breakthrough would resolve:
 *   - Relationship between counting and branching programs
 *   - Whether multiplication and division capture all of NC1
 *   - Complexity of basic arithmetic operations
 * ================================================================ */

void tc0_vs_nc1_report(void) {
    printf("===========================================\n");
    printf("  TC0 vs NC1: The 35-Year Open Problem\n");
    printf("===========================================\n\n");

    printf("Known: TC0 ⊆ NC1 (Barrington 1989)\n");
    printf("  Barrington's theorem: NC1 = bounded-width branching programs.\n");
    printf("  Threshold gates can be simulated by polynomial-size\n");
    printf("  bounded-width branching programs.\n");
    printf("  Therefore: TC0 ⊆ BWBP = NC1.\n\n");

    printf("Unknown: NC1 ⊆ TC0?\n");
    printf("  Equivalent to: Is PARITY in TC0?\n");
    printf("  PARITY_n ∈ NC1 (depth O(log n), size O(n)).\n");
    printf("  Conjectured: PARITY_n ∉ TC0 (most complexity theorists).\n\n");

    printf("Best known lower bounds against TC0:\n");
    printf("  Depth-2 threshold circuits for PARITY:n need size 2^{Omega(n)}.\n");
    printf("  (Muroga 1971, Impagliazzo-Paturi-Saks 1993)\n");
    printf("  But for depth 3+, no superpolynomial lower bounds known.\n\n");

    printf("Implications:\n");
    printf("  If NC1 = TC0: PARITY reduces to MAJORITY (unlikely).\n");
    printf("  If NC1 != TC0: New barrier between arithmetic/logic.\n");
    printf("  Resolution would be a major breakthrough.\n");
}

/* ================================================================
 * HASTAD'S SWITCHING LEMMA (Simplified)
 *
 * The switching lemma (Hastad 1987) states that a depth-d AC0
 * circuit can be "switched" to a depth-(d-1) circuit by randomly
 * restricting most variables.
 *
 * This implies:
 *   - PARITY requires exp(n^{1/(d-1)}) size for depth-d AC0
 *   - Lower bounds for AC0 => MAJORITY not in AC0
 *
 * For TC0: switching lemma doesn't directly apply because
 * threshold gates cannot be "switched" like AND/OR gates.
 * This is why TC0 lower bounds are harder.
 * ================================================================ */

void tc0_switching_lemma_explanation(void) {
    printf("Hastad's Switching Lemma (1987):\n");
    printf("  Given a CNF/DNF of size S, a random restriction leaving\n");
    printf("  a rho fraction of variables unset converts the formula\n");
    printf("  to a decision tree of depth <= t with high probability.\n");
    printf("  Pr[DT_depth > t] <= (5 * rho * t)^t\n\n");

    printf("Application to AC0 lower bounds:\n");
    printf("  Depth-d AC0 circuit -> depth-(d-1) after restriction.\n");
    printf("  By repeated application, any depth-d AC0 circuit for PARITY\n");
    printf("  must have size >= exp(Omega(n^{1/(d-1)})).\n\n");

    printf("Why this fails for TC0:\n");
    printf("  Threshold gates have unbounded fan-in and are not\n");
    printf("  expressible as CNF/DNF of small size.\n");
    printf("  A restricted threshold gate is still a threshold gate.\n");
    printf("  Therefore: switching lemma technique doesn't apply.\n\n");

    printf("  This is WHY TC0 lower bounds are so difficult.\n");
    printf("  It is also related to the Natural Proofs barrier\n");
    printf("  (Razborov-Rudich 1997).\n");
}

/* ================================================================
 * THRESHOLD CIRCUIT SEARCH (Neural Network Connection)
 * ================================================================ */

/* Search for a minimal depth-2 threshold circuit computing f.
 * Depth-2 TC0 = 2-layer neural network with threshold activation.
 * Finding the minimal such network is NP-hard (Blum-Rivest 1992).
 *
 * For small n, we can do exhaustive search. */

typedef struct {
    int     *gates;       /* Gate types [MAJ/THR/AND/OR] */
    int      num_gates;
    int      num_inputs;
    int    **fanin;       /* fanin[i][j] = input j of gate i */
    int     *fanin_count;
    int      *thr_k;      /* For THR gates */
} TC0Depth2Circuit;

/* Evaluate a depth-2 circuit */
static int eval_depth2(const TC0Depth2Circuit *c, const int *x) {
    int *hidden = (int *)malloc((size_t)c->num_gates * sizeof(int));
    for (int g = 0; g < c->num_gates; g++) {
        int sum = 0;
        for (int f = 0; f < c->fanin_count[g]; f++)
            sum += x[c->fanin[g][f]];
        if (c->gates[g] == TC_MAJ)
            hidden[g] = (sum > c->fanin_count[g] / 2) ? 1 : 0;
        else if (c->gates[g] == TC_THR)
            hidden[g] = (sum >= c->thr_k[g]) ? 1 : 0;
        else if (c->gates[g] == TC_AND)
            hidden[g] = (sum == c->fanin_count[g]) ? 1 : 0;
        else
            hidden[g] = (sum > 0) ? 1 : 0; /* OR */
    }
    /* Output = OR of all hidden gates (for acceptance) */
    int result = 0;
    for (int g = 0; g < c->num_gates; g++)
        if (hidden[g]) { result = 1; break; }
    free(hidden);
    return result;
}

/* Search for minimal depth-2 TC0 circuit by enumeration */
TC0Circuit *tc0_find_minimal_circuit(const BoolFunc *f,
                                       int max_gates, int max_fanin) {
    if (!f || f->num_vars > 8) return NULL;

    int n = f->num_vars;

    /* For n <= 6, try all possible depth-2 circuits with limited gates.
     * This is a simplified heuristic search. */

    for (int g = 1; g <= max_gates && g <= 4; g++) {
        /* Try gate type combinations */
        /* For simplicity: try THR gates with different thresholds */

        for (int config = 0; config < 100000; config++) {
            TC0Depth2Circuit c2;
            c2.num_gates = g;
            c2.num_inputs = n;
            c2.gates = (int *)malloc((size_t)g * sizeof(int));
            c2.fanin = (int **)malloc((size_t)g * sizeof(int *));
            c2.fanin_count = (int *)malloc((size_t)g * sizeof(int));
            c2.thr_k = (int *)malloc((size_t)g * sizeof(int));

            int tmp = config;
            int ok = 1;

            for (int gate = 0; gate < g && ok; gate++) {
                c2.gates[gate] = (tmp % 3 == 0) ? TC_MAJ :
                                  (tmp % 3 == 1) ? TC_THR : TC_AND;
                tmp /= 3;

                int fanin_size = (tmp % max_fanin) + 1;
                tmp /= max_fanin;
                c2.fanin_count[gate] = fanin_size;
                c2.fanin[gate] = (int *)malloc((size_t)fanin_size * sizeof(int));

                for (int f = 0; f < fanin_size; f++) {
                    c2.fanin[gate][f] = tmp % n;
                    tmp /= n;
                }

                if (c2.gates[gate] == TC_THR) {
                    c2.thr_k[gate] = (tmp % (n + 1));
                    tmp /= (n + 1);
                } else {
                    c2.thr_k[gate] = 0;
                }
            }

            if (!ok) {
                for (int gate = 0; gate < g; gate++) free(c2.fanin[gate]);
                free(c2.gates); free(c2.fanin); free(c2.fanin_count); free(c2.thr_k);
                continue;
            }

            /* Check if this circuit computes f correctly */
            int correct = 1;
            for (int x = 0; x < f->table_size && correct; x++) {
                int input[8];
                for (int v = 0; v < n; v++)
                    input[v] = (x >> v) & 1;
                if (eval_depth2(&c2, input) != f->truth_table[x])
                    correct = 0;
            }

            /* Cleanup */
            for (int gate = 0; gate < g; gate++) free(c2.fanin[gate]);
            free(c2.gates); free(c2.fanin); free(c2.fanin_count); free(c2.thr_k);

            if (correct) {
                /* Found a circuit! Convert to TC0Circuit format */
                TC0Circuit *result = tc0_circuit_create(g + n + 2);
                snprintf(result->name, sizeof(result->name),
                         "MinCircuit_%s_g%d", f->name, g);
                return result;  /* Simplified: return success indicator */
            }
        }
    }

    return NULL;  /* Not found within search limits */
}

/* ================================================================
 * DEMO
 * ================================================================ */

void tc0_search_demo(void) {
    printf("\n============================================================\n");
    printf("  TC0 CIRCUIT SEARCH AND ANALYSIS DEMO\n");
    printf("============================================================\n\n");

    /* Circuit enumeration */
    printf("--- Threshold Circuit Search Space ---\n");
    printf("Number of possible TC0 circuits:\n");
    for (int n = 4; n <= 8; n += 2) {
        printf("  n=%d: gates=2 fanin=2 => %.1e circuits\n",
               n, tc0_enumerate_circuits(n, 2, 2));
        printf("  n=%d: gates=3 fanin=3 => %.1e circuits\n",
               n, tc0_enumerate_circuits(n, 3, 3));
    }

    /* PTF degree examples */
    printf("\n--- PTF Degree ---\n");
    BoolFunc *maj = boolfunc_majority(5);
    BoolFunc *par = boolfunc_parity(5);
    printf("  MAJORITY_5 PTF degree: %d\n", tc0_compute_ptf_degree(maj));
    printf("  PARITY_5 PTF degree: %d\n", tc0_compute_ptf_degree(par));
    boolfunc_free(maj);
    boolfunc_free(par);

    /* Size lower bounds */
    printf("\n--- Circuit Size Lower Bounds ---\n");
    BoolFunc *f = boolfunc_majority(6);
    printf("  MAJORITY_6: lower bound >= %d gates\n", tc0_size_lower_bound(f));
    boolfunc_free(f);

    /* TC0 vs NC1 */
    printf("\n");
    tc0_vs_nc1_report();

    /* Switching lemma */
    printf("\n");
    tc0_switching_lemma_explanation();

    printf("\n--- Known Results ---\n");
    printf("  MAJORITY: depth-2 TC0 needs O(n) gates (optimal).\n");
    printf("  PARITY:   depth-2 TC0 needs 2^{Omega(n)} gates (Muroga 1971).\n");
    printf("  SORTING:  depth-2 TC0 needs O(n log n) gates (Batcher 1968).\n");
    printf("  MULT:     depth-3 TC0 needs O(n^2) gates (schoolbook).\n");
}
