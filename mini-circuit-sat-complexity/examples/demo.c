/* demo.c -- End-to-End Examples for Circuit-SAT Complexity
   Uses puts() for simplicity. */

#include "csat.h"

static void sep(void) { puts(""); }

static void demo1(void) {
    puts("========== Example 1: SAT Algorithms on PARITY ==========");
    sep();
    for (int n = 4; n <= 8; n += 2) {
        BooleanCircuit* c = circuit_parity(n);
        printf("PARITY(%d): size=%d, depth=%d\n",
               n, circuit_size(c), circuit_depth(c));
        clock_t t = clock();
        int bf = csat_brute(c, n);
        double ms = 1000.0 * (clock() - t) / CLOCKS_PER_SEC;
        printf("  Brute: %s (%.3f ms)\n", bf ? "SAT" : "UNSAT", ms);
        int* as = calloc(n, sizeof(int));
        t = clock();
        int dp = csat_dpll(c, as, 0, n);
        ms = 1000.0 * (clock() - t) / CLOCKS_PER_SEC;
        printf("  DPLL:  %s (%.3f ms)\n", dp ? "SAT" : "UNSAT", ms);
        free(as); circuit_free(c);
    }
}

static void demo2(void) {
    sep();
    puts("========== Example 2: Circuit to CNF to CDCL ==========");
    sep();
    BooleanCircuit* c = circuit_create(20);
    int x0 = circuit_add_gate(c, INPUT, -1, -1);
    int x1 = circuit_add_gate(c, INPUT, -1, -1);
    int x2 = circuit_add_gate(c, INPUT, -1, -1);
    int nx2 = circuit_add_gate(c, NOT, x2, -1);
    int and1 = circuit_add_gate(c, AND, x0, x1);
    int out  = circuit_add_gate(c, OR, and1, nx2);
    circuit_set_output(c, &out, 1);
    printf("Circuit: (x0 AND x1) OR (NOT x2)\n");
    printf("  Size=%d\n", circuit_size(c));
    printf("\nTruth table:\n");
    for (int m = 0; m < 8; m++) {
        int inp[3] = {m&1, (m>>1)&1, (m>>2)&1};
        printf("  x0=%d x1=%d x2=%d => %d\n",
               inp[0], inp[1], inp[2], circuit_evaluate(c, inp));
    }
    CNFInstance* cnf = csat_to_cnf(c);
    printf("\nTseitin CNF: %d vars, %d clauses\n", cnf->nv, cnf->nc);
    cnf_free(cnf); circuit_free(c);
}

static void demo3(void) {
    sep();
    puts("========== Example 3: Circuit Optimization ==========");
    sep();
    BooleanCircuit* c = circuit_create(30);
    int x0 = circuit_add_gate(c, INPUT, -1, -1);
    int x1 = circuit_add_gate(c, INPUT, -1, -1);
    int c0 = circuit_add_gate(c, CONST0, -1, -1);
    int c1 = circuit_add_gate(c, CONST1, -1, -1);
    int and1 = circuit_add_gate(c, AND, x0, c1);
    int or1  = circuit_add_gate(c, OR, x1, c0);
    int out  = circuit_add_gate(c, AND, and1, or1);
    circuit_set_output(c, &out, 1);
    printf("Before: AND(AND(x0,1), OR(x1,0))  size=%d\n", circuit_size(c));
    int r = optimize_constant_fold(c);
    printf("After constant fold: size=%d (%d simplified)\n", circuit_size(c), r);
    /* Verify: AND(x0,1)=x0, OR(x1,0)=x1, AND(x0,x1)=x0∧x1 */
    int ok = 1;
    for (int m = 0; m < 4; m++) {
        int inp[2] = {m&1, (m>>1)&1};
        if (circuit_evaluate(c, inp) != (inp[0] && inp[1])) ok = 0;
    }
    printf("Correct: %s\n", ok ? "YES" : "NO");
    circuit_free(c);
}

static void demo4(void) {
    sep();
    puts("========== Example 4: Lower Bounds ==========");
    sep();
    puts("Shannon-Lupanov bounds:");
    printf("%4s %16s %16s\n", "n", "Shannon LB", "Lupanov UB");
    for (int n = 1; n <= 10; n++)
        printf("%4d %15.1f %15.1f\n", n, shannon_bound(n), lupanov_bound(n));
    sep();
    puts("Williams speedup estimates:");
    for (int n = 10; n <= 30; n += 10)
        printf("  n=%d: 2^n=%.0e  /n^2=%.0e\n",
               n, pow(2.0,n), csat_williams_speedup(n,2));
}

static void demo5(void) {
    sep();
    puts("========== Example 5: Benchmarks ==========");
    sep();
    puts("Random 3-SAT (n=10):");
    for (double a = 2.0; a <= 5.0; a += 1.0) {
        int m = (int)(a * 10);
        CNFInstance* cnf = bench_random_ksat(10, m, 3, 42);
        if (cnf) {
            printf("  alpha=%.1f m=%d: %d vars, %d clauses\n",
                   cnf_clause_var_ratio(cnf), m, cnf->nv, cnf->nc);
            cnf_free(cnf);
        }
    }
    sep();
    puts("Pigeonhole PHP(3):");
    CNFInstance* php = bench_pigeonhole(3);
    if (php) {
        printf("  %d vars, %d clauses (always UNSAT)\n", php->nv, php->nc);
        cnf_free(php);
    }
}

int main(void) {
    setbuf(stdout, NULL);
    puts("============================================================");
    puts("  Circuit-SAT Complexity -- End-to-End Examples");
    puts("============================================================");
    demo1();
    demo2();
    demo3();
    demo4();
    demo5();
    sep();
    puts("============================================================");
    puts("  All examples completed successfully.");
    puts("============================================================");
    return 0;
}
