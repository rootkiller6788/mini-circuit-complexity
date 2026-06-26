/* acc0_sat.c -- ACC0 Circuit Satisfiability
 * ===========================================
 * L5: Algorithms -- SAT solvers for ACC0 circuits.
 * L7: Applications -- Practical SAT solving, circuit verification.
 * L8: Advanced -- Williams' algorithmic method, subspace enumeration.
 *
 * ACC0-SAT is the problem: given an ACC0 circuit C, does there exist
 * an input x such that C(x) = 1?
 *
 * Complexity: ACC0-SAT is NP-complete.
 * But Williams (2014) showed it can be solved faster than 2^n
 * for certain parameters, which is the key to his lower bounds.
 *
 * References:
 * - Williams (2014): JACM 61(1)
 * - Impagliazzo, Matthews, Paturi (2012): "Satisfiability for
 *   circuits of small depth", SODA 2012
 */

#include "acc0.h"
#include "acc0_lower_bounds.h"
#include <assert.h>

/* ================================================================
 * SAT Result Structure
 * ================================================================ */

typedef struct {
    int satisfiable;         /* 1 if SAT, 0 if UNSAT, -1 if unknown */
    int *assignment;         /* satisfying assignment (if SAT) */
    int ninputs;             /* number of inputs */
    long long nodes_explored;/* search tree nodes explored */
    double elapsed_sec;      /* wall-clock time */
} ACC0SATResult;

/* ================================================================
 * Exhaustive SAT (Brute Force)
 * ================================================================ */

ACC0SATResult* acc0_sat_exhaustive(ACC0Circuit *c, int timeout_sec) {
    ACC0SATResult *r = (ACC0SATResult*)malloc(sizeof(ACC0SATResult));
    if (!r) return NULL;
    memset(r, 0, sizeof(ACC0SATResult));
    r->satisfiable = 0;
    r->ninputs = c ? c->ninputs : 0;

    if (!c || c->ninputs > 30) {
        r->satisfiable = -1; /* too many inputs */
        return r;
    }

    int n = c->ninputs;
    long long total = 1LL << n;
    r->assignment = (int*)calloc((size_t)n, sizeof(int));
    if (!r->assignment) { r->satisfiable = -1; return r; }

    clock_t start = clock();
    for (long long mask = 0; mask < total; mask++) {
        r->nodes_explored++;
        for (int i = 0; i < n; i++)
            r->assignment[i] = (int)((mask >> i) & 1);

        if (acc0_circuit_eval(c, r->assignment) == 1) {
            r->satisfiable = 1;
            break;
        }

        /* Check timeout */
        if (timeout_sec > 0) {
            double elapsed = (double)(clock() - start) / CLOCKS_PER_SEC;
            if (elapsed > (double)timeout_sec) {
                r->satisfiable = -1; /* timed out */
                break;
            }
        }
    }
    r->elapsed_sec = (double)(clock() - start) / CLOCKS_PER_SEC;

    if (!r->satisfiable && r->assignment) {
        free(r->assignment);
        r->assignment = NULL;
    }
    return r;
}

void acc0_sat_result_free(ACC0SATResult *r) {
    if (!r) return;
    free(r->assignment);
    free(r);
}

/* ================================================================
 * DPLL-like SAT for ACC0 Circuits
 *
 * A simple recursive backtracking SAT solver tailored for ACC0.
 * Uses unit propagation through AND/OR gates.
 * ================================================================ */

/* Partial assignment: -1 = unassigned, 0 = false, 1 = true */
static int dpll_propagate(ACC0Circuit *c, int *assignment, int *propagated) {
    /* Propagate known values through the circuit.
     * Returns: 0 = conflict, 1 = consistent.
     * Sets *propagated to 1 if any value was set. */
    int changed = 0;
    *propagated = 0;

    for (int pass = 0; pass < c->ngates; pass++) {
        int local_change = 0;
        for (int i = 0; i < c->ngates; i++) {
            ACC0Gate *g = &c->gates[i];
            int known = 0, val = 0;

            /* Check if all fanins are known */
            if (g->nfans > 0) {
                int all_known = 1;
                int any_unknown = 0;
                for (int j = 0; j < g->nfans; j++) {
                    if (g->fans[j] < c->ninputs) {
                        if (assignment[g->fans[j]] < 0) { all_known = 0; any_unknown = 1; }
                    }
                }
                known = all_known && !any_unknown;
            } else {
                /* Single/double input: check if both known */
                int k1 = (g->i1 >= 0 && g->i1 < c->ninputs) ? assignment[g->i1] : 1;
                int k2 = (g->i2 >= 0 && g->i2 < c->ninputs) ? assignment[g->i2] : 1;
                if (k1 < 0 && k2 < 0) known = 0;
                else if (k1 >= 0 && k2 >= 0) known = 1;
                else known = 0;
            }

            /* If output gate and known, check consistency */
            /* (Simplified: only apply to output gates) */
            for (int oi = 0; oi < c->noutputs; oi++) {
                if (i == c->outputs[oi] && known) {
                    /* Compute expected value */
                    int *vis = (int*)malloc((size_t)c->ngates * sizeof(int));
                    for (int j = 0; j < c->ngates; j++) vis[j] = -1;
                    val = acc0_gate_eval(c, i, assignment, vis);
                    free(vis);
                    if (val == 0) return 0; /* conflict: output must be 1 */
                }
            }

            /* If output gate can't be 1 given partial assignment, conflict */
            for (int oi = 0; oi < c->noutputs; oi++) {
                if (i == c->outputs[oi]) {
                    int *vis = (int*)malloc((size_t)c->ngates * sizeof(int));
                    for (int j = 0; j < c->ngates; j++) vis[j] = -1;
                    val = acc0_gate_eval(c, i, assignment, vis);
                    free(vis);
                    if (val == 1) return 1; /* already satisfied */
                }
            }
        }
        if (!local_change) break;
        changed = 1;
        *propagated = 1;
    }
    return 1;
}

static int dpll_solve(ACC0Circuit *c, int *assignment, int var, int nvars) {
    /* Base case: all variables assigned */
    if (var >= nvars) {
        return acc0_circuit_eval(c, assignment);
    }

    /* Try assignment[var] = 0 */
    assignment[var] = 0;
    int propagated;
    int cons = dpll_propagate(c, assignment, &propagated);
    if (cons) {
        if (dpll_solve(c, assignment, var + 1, nvars))
            return 1;
    }

    /* Try assignment[var] = 1 */
    assignment[var] = 1;
    cons = dpll_propagate(c, assignment, &propagated);
    if (cons) {
        if (dpll_solve(c, assignment, var + 1, nvars))
            return 1;
    }

    assignment[var] = -1; /* backtrack */
    return 0;
}

int acc0_sat_dpll(ACC0Circuit *c, int *out_assignment) {
    /* DPLL SAT solver for ACC0 circuits.
     * Returns 1 if SAT, 0 if UNSAT.
     * If SAT and out_assignment != NULL, stores assignment.
     *
     * Complexity: worst-case O(2^n), but unit propagation
     * significantly reduces search in practice. */
    if (!c || c->ninputs > 30) return -1;

    int n = c->ninputs;
    int *assignment = (int*)malloc((size_t)n * sizeof(int));
    if (!assignment) return -1;
    for (int i = 0; i < n; i++) assignment[i] = -1;

    int result = dpll_solve(c, assignment, 0, n);

    if (out_assignment && result == 1) {
        memcpy(out_assignment, assignment, (size_t)n * sizeof(int));
    }

    free(assignment);
    return result;
}

/* ================================================================
 * Randomized SAT for ACC0
 *
 * Uses random sampling + local search.
 * For ACC0 circuits, the polynomial method gives a better
 * randomized algorithm, but we implement a simpler heuristic.
 * ================================================================ */

int acc0_sat_randomized(ACC0Circuit *c, int *out_assignment, int max_trials) {
    /* Randomized SAT: sample random assignments.
     * For a circuit with acceptance probability p, expects
     * to find a solution in O(1/p) trials.
     *
     * Returns 1 if SAT, 0 if probably UNSAT.
     * Not complete, but fast. */
    if (!c || c->ninputs <= 0 || c->ninputs > 30) return -1;

    int n = c->ninputs;
    int *x = (int*)malloc((size_t)n * sizeof(int));
    if (!x) return -1;

    for (int trial = 0; trial < max_trials; trial++) {
        for (int i = 0; i < n; i++) x[i] = rand() % 2;
        if (acc0_circuit_eval(c, x) == 1) {
            if (out_assignment)
                memcpy(out_assignment, x, (size_t)n * sizeof(int));
            free(x);
            return 1;
        }

        /* Local search: flip one bit */
        for (int bit = 0; bit < n; bit++) {
            x[bit] ^= 1;
            if (acc0_circuit_eval(c, x) == 1) {
                if (out_assignment)
                    memcpy(out_assignment, x, (size_t)n * sizeof(int));
                free(x);
                return 1;
            }
            x[bit] ^= 1; /* flip back */
        }
    }
    free(x);
    return 0; /* probably UNSAT */
}

/* ================================================================
 * SAT for ACC0[2] Circuits (PARITY-based)
 *
 * When the circuit is over GF(2) (only MOD2 = XOR gates),
 * SAT reduces to linear algebra over GF(2).
 * ================================================================ */

int acc0_sat_mod2(ACC0Circuit *c, int *out_assignment) {
    /* For circuits using only MOD2, AND, OR, NOT:
     * - MOD2 gate: sum(x_i) mod 2 = 0
     * - AND/OR/NOT break linearity
     *
     * If the circuit uses ONLY MOD2 and NOT gates:
     * - It's a linear system over GF(2).
     * - SAT = solving Ax = b over GF(2), which is O(n^3).
     *
     * Here we check if the circuit is purely MOD2+NOT. */
    if (!c || c->ninputs > 30) return -1;

    /* Check if circuit uses only MOD2 and NOT gates */
    int only_mod2 = 1;
    for (int i = 0; i < c->ngates; i++) {
        ACC0GateType t = c->gates[i].type;
        if (t != ACC0_MOD2 && t != ACC0_NOT &&
            t != ACC0_INPUT && t != ACC0_CONST) {
            only_mod2 = 0;
            break;
        }
    }

    if (only_mod2) {
        /* Linear system: MOD2 gate i: sum of inputs = 0 (mod 2).
         * This is equivalent to: XOR of all fan-in = 0.
         * For NOT gates: output = 1 - input.
         *
         * Build matrix A and vector b for Ax = b over GF(2).
         * Then use Gaussian elimination. */
        int n = c->ninputs;
        int m = 0; /* number of MOD2 gates (equations) */

        /* Count MOD2 gates */
        for (int i = 0; i < c->ngates; i++)
            if (c->gates[i].type == ACC0_MOD2) m++;

        if (m == 0) {
            /* No equations: any assignment works */
            if (out_assignment)
                for (int i = 0; i < n; i++) out_assignment[i] = 0;
            return 1;
        }

        /* Build (m+1) x (n+1) augmented matrix over GF(2).
         * For simplicity, allocate on stack if small. */
        int max_dim = n + 1;
        int matrix[32][32]; /* supports up to 30 inputs */
        if (max_dim > 32) return -1;

        memset(matrix, 0, sizeof(matrix));

        int eq = 0;
        for (int i = 0; i < c->ngates && eq < m; i++) {
            if (c->gates[i].type != ACC0_MOD2) continue;
            ACC0Gate *g = &c->gates[i];
            for (int j = 0; j < g->nfans; j++)
                if (g->fans[j] < n)
                    matrix[eq][g->fans[j]] ^= 1;
            /* Right-hand side: MOD2 = 1 means sum = 0 mod 2
             * So equation: sum(x_i) = 0 mod 2 -> b = 0 */
            matrix[eq][n] = 0; /* MOD2 returns 1 if sum=0 mod 2 */
            eq++;
        }

        /* Gaussian elimination over GF(2) */
        int rank = 0;
        for (int col = 0; col < n && rank < m; col++) {
            /* Find pivot */
            int pivot = -1;
            for (int row = rank; row < m; row++) {
                if (matrix[row][col]) { pivot = row; break; }
            }
            if (pivot < 0) continue;

            /* Swap rows */
            if (pivot != rank) {
                for (int j = 0; j <= n; j++) {
                    int tmp = matrix[rank][j];
                    matrix[rank][j] = matrix[pivot][j];
                    matrix[pivot][j] = tmp;
                }
            }

            /* Eliminate other rows */
            for (int row = 0; row < m; row++) {
                if (row != rank && matrix[row][col]) {
                    for (int j = col; j <= n; j++)
                        matrix[row][j] ^= matrix[rank][j];
                }
            }
            rank++;
        }

        /* Check consistency: any row with all zeros in coefficient
         * columns but 1 in RHS is inconsistent */
        for (int row = rank; row < m; row++) {
            int all_zero = 1;
            for (int col = 0; col < n; col++)
                if (matrix[row][col]) { all_zero = 0; break; }
            if (all_zero && matrix[row][n] == 1)
                return 0; /* UNSAT */
        }

        /* Extract solution: free variables get 0 */
        if (out_assignment) {
            for (int i = 0; i < n; i++) out_assignment[i] = 0;
            for (int row = 0; row < rank; row++) {
                int pivot_col = -1;
                for (int col = 0; col < n; col++)
                    if (matrix[row][col]) { pivot_col = col; break; }
                if (pivot_col >= 0)
                    out_assignment[pivot_col] = matrix[row][n];
            }
        }
        return 1; /* SAT */
    }

    /* Circuit has non-linear gates, use brute force */
    return acc0_sat_bruteforce(c, out_assignment);
}

/* ================================================================
 * Counting Solutions
 * ================================================================ */

long long acc0_count_solutions(ACC0Circuit *c) {
    /* Count number of satisfying assignments.
     * Complexity: O(2^n). Only practical for n <= 20. */
    if (!c || c->ninputs > 20) return -1;
    int n = c->ninputs;
    long long count = 0;
    long long total = 1LL << n;
    for (long long mask = 0; mask < total; mask++) {
        int *x = (int*)calloc((size_t)n, sizeof(int));
        for (int i = 0; i < n; i++) x[i] = (int)((mask >> i) & 1);
        if (acc0_circuit_eval(c, x)) count++;
        free(x);
    }
    return count;
}

/* ================================================================
 * Demo Functions
 * ================================================================ */

void acc0_sat_demo_full(void) {
    printf("\n========== ACC0 Circuit-SAT ==========\n\n");

    printf("--- Exhaustive SAT ---\n");
    /* Build circuit: MOD3(x0, x1, x2) = 1 */
    ACC0Circuit *c = acc0_circuit_create(50);
    int ins[3];
    for (int i = 0; i < 3; i++) ins[i] = acc0_add_input(c);
    int m3 = acc0_add_mod(c, 3);
    acc0_set_fanin(c, m3, ins, 3);
    int o[1] = { m3 };
    acc0_set_outputs(c, o, 1);

    int assign[3];
    int sat = acc0_sat_bruteforce(c, assign);
    printf("  MOD3(3): SAT=%d\n", sat);
    if (sat) printf("  assignment: [%d,%d,%d]\n", assign[0],assign[1],assign[2]);

    long long nsol = acc0_count_solutions(c);
    printf("  #solutions: %lld (out of 8)\n", nsol);
    acc0_circuit_free(c);

    /* UNSAT circuit */
    ACC0Circuit *c2 = acc0_circuit_create(20);
    int in0 = acc0_add_input(c2);
    int not0 = acc0_add_not(c2, in0);
    int and0 = acc0_add_and(c2, in0, not0);
    int o2[1] = { and0 };
    acc0_set_outputs(c2, o2, 1);
    int sat2 = acc0_sat_bruteforce(c2, NULL);
    printf("  x AND NOT(x): SAT=%d (UNSAT)\n", sat2);
    acc0_circuit_free(c2);

    printf("\n--- DPLL SAT ---\n");
    ACC0Circuit *c3 = acc0_circuit_create(50);
    int ins3[5];
    for (int i = 0; i < 5; i++) ins3[i] = acc0_add_input(c3);
    int mod5 = acc0_add_mod(c3, 5);
    acc0_set_fanin(c3, mod5, ins3, 5);
    int o3[1] = { mod5 };
    acc0_set_outputs(c3, o3, 1);

    int assign3[5];
    int sat3 = acc0_sat_dpll(c3, assign3);
    printf("  MOD5(5): SAT=%d\n", sat3);
    if (sat3) printf("  assignment: [%d,%d,%d,%d,%d]\n",
        assign3[0],assign3[1],assign3[2],assign3[3],assign3[4]);
    printf("  #solutions: %lld\n", acc0_count_solutions(c3));
    acc0_circuit_free(c3);

    printf("\n--- Randomized SAT ---\n");
    ACC0Circuit *c4 = acc0_circuit_create(50);
    int ins4[6];
    for (int i = 0; i < 6; i++) ins4[i] = acc0_add_input(c4);
    int mod3_2 = acc0_add_mod(c4, 3);
    acc0_set_fanin(c4, mod3_2, ins4, 6);
    int o4[1] = { mod3_2 };
    acc0_set_outputs(c4, o4, 1);

    int assign4[6];
    int sat4 = acc0_sat_randomized(c4, assign4, 1000);
    printf("  MOD3(6) randomized: SAT=%d\n", sat4);
    if (sat4) printf("  assignment: [%d,%d,%d,%d,%d,%d]\n",
        assign4[0],assign4[1],assign4[2],assign4[3],assign4[4],assign4[5]);
    acc0_circuit_free(c4);

    printf("\n--- MOD2 SAT (linear algebra) ---\n");
    ACC0Circuit *c5 = acc0_circuit_create(50);
    int ins5[4];
    for (int i = 0; i < 4; i++) ins5[i] = acc0_add_input(c5);
    int mod2 = acc0_add_mod(c5, 2);
    acc0_set_fanin(c5, mod2, ins5, 4);
    int o5[1] = { mod2 };
    acc0_set_outputs(c5, o5, 1);

    int assign5[4];
    int sat5 = acc0_sat_mod2(c5, assign5);
    printf("  MOD2(4) = PARITY(4): SAT=%d\n", sat5);
    if (sat5) printf("  assignment: [%d,%d,%d,%d]\n",
        assign5[0],assign5[1],assign5[2],assign5[3]);
    printf("  #solutions: %lld (out of 16)\n", acc0_count_solutions(c5));
    acc0_circuit_free(c5);
}

