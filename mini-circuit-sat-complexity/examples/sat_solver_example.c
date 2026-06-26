/* sat_solver_example.c -- SAT Solver Portfolio Comparison
 *
 * Compares brute-force, DPLL, CDCL, lookahead, and random walk
 * on various circuit families. Demonstrates the practical
 * differences between complete and incomplete SAT algorithms.
 */

#include "csat.h"

static void test_on_circuit(const char* name, BooleanCircuit* c, int nv)
{
    printf("  %s (n=%d, gates=%d):
", name, nv, circuit_size(c));

    /* Brute force */
    clock_t t0 = clock();
    int r = csat_brute(c, nv);
    double ms = 1000.0 * (clock() - t0) / CLOCKS_PER_SEC;
    printf("    Brute:  %s (%.2f ms)
", r ? "SAT" : "UNSAT", ms);

    /* DPLL */
    int* assign = (int*)calloc((size_t)nv, sizeof(int));
    if (assign) {
        t0 = clock();
        r = csat_dpll(c, assign, 0, nv);
        ms = 1000.0 * (clock() - t0) / CLOCKS_PER_SEC;
        printf("    DPLL:   %s (%.2f ms)
", r ? "SAT" : "UNSAT", ms);
        free(assign);
    }

    /* Random sampling */
    t0 = clock();
    r = csat_random(c, nv, 5000);
    ms = 1000.0 * (clock() - t0) / CLOCKS_PER_SEC;
    printf("    Random: %s (%.2f ms, 5000 samples)
", r ? "SAT" : "UNSAT", ms);

    /* Random walk */
    t0 = clock();
    r = csat_random_restarts(c, nv, 20, nv * 10);
    ms = 1000.0 * (clock() - t0) / CLOCKS_PER_SEC;
    printf("    Walk:   %s (%.2f ms, 20 restarts)
", r ? "SAT" : "UNSAT", ms);

    printf("
");
}

int main(void)
{
    setbuf(stdout, NULL);
    srand(12345);

    printf("================================================================
");
    printf("  SAT Solver Portfolio Comparison
");
    printf("================================================================

");

    /* Test on various circuit families */
    for (int nv = 4; nv <= 10; nv += 2) {
        BooleanCircuit* c;

        c = circuit_parity(nv);
        test_on_circuit("PARITY", c, nv);
        circuit_free(c);

        c = circuit_majority(nv);
        test_on_circuit("MAJORITY", c, nv);
        circuit_free(c);
    }

    /* CDCL solver test on structured CNF */
    printf("  CDCL on pigeonhole principle:
");
    for (int pn = 2; pn <= 4; pn++) {
        CNFInstance* cnf = bench_pigeonhole(pn);
        CDCLSolver* s = cdcl_create(cnf->nv);
        cdcl_load_cnf(s, cnf);
        clock_t t0 = clock();
        int r = cdcl_solve(s, 100000);
        double ms = 1000.0 * (clock() - t0) / CLOCKS_PER_SEC;
        printf("    PHP(%d): %s (%.2f ms, %d vars, %d clauses)
",
               pn, r == 0 ? "UNSAT" : (r == 1 ? "SAT" : "?"),
               ms, cnf->nv, cnf->nc);
        cdcl_free(s);
        cnf_free(cnf);
    }

    printf("
================================================================
");
    printf("  SOLVER COMPARISON COMPLETE
");
    printf("================================================================
");
    return 0;
}
