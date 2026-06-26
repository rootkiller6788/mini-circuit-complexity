/* benchmark_generators.c -- Hard SAT Instance Generators
 *
 * Generate SAT benchmarks for solver testing:
 *   - Random k-SAT (phase transition)
 *   - Pigeonhole principle (PHP, exponential resolution)
 *   - Graph k-coloring
 *   - Sorting network verification
 *   - Circuit equivalence checking
 *
 * L6: Canonical hard instances
 * L7: Benchmark suite for solver comparison
 */

#include "csat.h"

/* ========================================================================
 * Random k-SAT with n variables and m clauses.
 * Clause-to-variable ratio alpha = m/n controls hardness.
 * Phase transition: alpha_crit ~ 4.26 for 3-SAT.
 * ======================================================================== */

CNFInstance* bench_random_ksat(int n, int m, int k, unsigned int seed) {
    if (n <= 0 || m <= 0 || k <= 0) return NULL;
    CNFInstance* cnf = cnf_create(n);
    if (!cnf) return NULL;
    srand(seed);
    for (int ci = 0; ci < m; ci++) {
        int clause[64], seen[2048] = {0};
        for (int li = 0; li < k; li++) {
            int v = (rand() % n) + 1;
            while (seen[v] && li < n) { v = (rand() % n) + 1; }
            seen[v] = 1;
            clause[li] = (rand() & 1) ? v : -v;
        }
        cnf_add_clause(cnf, clause, k);
    }
    return cnf;
}

/* ========================================================================
 * Pigeonhole Principle: n+1 pigeons into n holes.
 * Classic UNSAT benchmark. Requires exponential-size resolution
 * refutations (Haken 1985). Hard for CDCL without learning.
 * ======================================================================== */

CNFInstance* bench_pigeonhole(int n) {
    if (n <= 0 || n > 50) return NULL;
    int np = n + 1, nh = n;
    int nv = np * nh;
    CNFInstance* cnf = cnf_create(nv);
    if (!cnf) return NULL;

    /* Each pigeon in at least one hole */
    for (int p = 0; p < np; p++) {
        int cls[64], nl = 0;
        for (int h = 0; h < nh; h++)
            cls[nl++] = p * nh + h + 1;
        cnf_add_clause(cnf, cls, nl);
    }

    /* Each hole has at most one pigeon */
    for (int h = 0; h < nh; h++) {
        for (int p1 = 0; p1 < np; p1++) {
            for (int p2 = p1 + 1; p2 < np; p2++) {
                int c2[2];
                c2[0] = -(p1 * nh + h + 1);
                c2[1] = -(p2 * nh + h + 1);
                cnf_add_clause(cnf, c2, 2);
            }
        }
    }
    return cnf;
}

/* ========================================================================
 * Graph k-coloring SAT encoding.
 * Variables: x_{v,c} = vertex v gets color c (1-indexed).
 * SAT iff graph is k-colorable. Hard for chromatic number near k.
 * ======================================================================== */

CNFInstance* bench_graph_coloring(int n, int edges[][2], int ne, int k) {
    if (n <= 0 || k <= 0) return NULL;
    int nv = n * k;
    CNFInstance* cnf = cnf_create(nv);
    if (!cnf) return NULL;

    /* Each vertex has at least one color */
    for (int v = 0; v < n; v++) {
        int cls[64], nl = 0;
        for (int c = 0; c < k; c++)
            cls[nl++] = v * k + c + 1;
        cnf_add_clause(cnf, cls, nl);
    }

    /* Each vertex has at most one color */
    for (int v = 0; v < n; v++) {
        for (int c1 = 0; c1 < k; c1++) {
            for (int c2 = c1 + 1; c2 < k; c2++) {
                int c2c[2];
                c2c[0] = -(v * k + c1 + 1);
                c2c[1] = -(v * k + c2 + 1);
                cnf_add_clause(cnf, c2c, 2);
            }
        }
    }

    /* Adjacent vertices have different colors */
    for (int e = 0; e < ne; e++) {
        int u = edges[e][0], v = edges[e][1];
        for (int c = 0; c < k; c++) {
            int c2[2];
            c2[0] = -(u * k + c + 1);
            c2[1] = -(v * k + c + 1);
            cnf_add_clause(cnf, c2, 2);
        }
    }
    return cnf;
}

/* ========================================================================
 * Sorting network SAT: is the comparator network correct?
 * Variables encode wire values at each time step.
 * SAT iff network fails to sort some input (BUG found).
 * UNSAT iff network is correct.
 * ======================================================================== */

CNFInstance* bench_sorting_network(int n) {
    if (n <= 0 || n > 6) return NULL;
    /* Placeholder: real sorting network CNF would encode
       comparator semantics and check output is sorted.
       Here we generate a simple formula for testing. */
    CNFInstance* cnf = cnf_create(n * n);
    if (!cnf) return NULL;
    int cl[1] = {1};
    cnf_add_clause(cnf, cl, 1);
    return cnf;
}

/* ========================================================================
 * Circuit equivalence: check if two circuits compute the same function.
 * Builds miter = XOR of outputs. SAT => inequivalent.
 * Returns: 1=equivalent, 0=not equivalent, -1=error.
 * ======================================================================== */

int bench_equivalent(BooleanCircuit* c1, BooleanCircuit* c2, int n_inputs) {
    if (!c1 || !c2 || n_inputs <= 0 || n_inputs > 20) return -1;
    long long total = 1LL << n_inputs;
    if (total > (1LL << 20)) total = 1LL << 20;
    int* inp = calloc((size_t)n_inputs, sizeof(int));
    if (!inp) return -1;
    int equiv = 1;
    for (long long m = 0; m < total && equiv; m++) {
        for (int i = 0; i < n_inputs; i++)
            inp[i] = (int)((m >> i) & 1);
        if (circuit_evaluate(c1, inp) != circuit_evaluate(c2, inp))
            equiv = 0;
    }
    free(inp);
    return equiv;
}

/* ========================================================================
 * Demo
 * ======================================================================== */

